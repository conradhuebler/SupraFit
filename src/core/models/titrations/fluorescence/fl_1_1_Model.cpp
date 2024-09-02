/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2024 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "src/core/models/postprocess/statistic.h"

#include "src/core/bc50.h"
#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include "src/core/models/models.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include "fl_1_1_Model.h"

fl_ItoI_Model::fl_ItoI_Model(DataClass* data)
    : AbstractTitrationModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
}

fl_ItoI_Model::fl_ItoI_Model(AbstractTitrationModel* data)
    : AbstractTitrationModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
}

fl_ItoI_Model::~fl_ItoI_Model()
{
}

void fl_ItoI_Model::DeclareOptions()
{
    QStringList host = QStringList() << "yes"
                                     << "no";
    addOption(Host, "Host dynamic", host);
    setOption(Host, "no");

    QStringList guest = QStringList() << "dynamic"
                                      << "silent";
    addOption(Guest, "Guest (dynamic)", guest);
    setOption(Guest, "silent");

    QStringList host_only = QStringList() << "yes"
                                          << "no";
    addOption(HostOnly, "Host only", host);
    setOption(HostOnly, "no");
}

void fl_ItoI_Model::InitialGuess_Private()
{
    qreal factor = 1;
    factor = 1 / InitialHostConcentration(0);

    LocalTable()->setColumn(DependentModel()->firstRow() * factor, 0);
    LocalTable()->setColumn(DependentModel()->firstRow() * factor, 1);
    LocalTable()->setColumn(DependentModel()->lastRow() * factor, 2);
    LocalTable()->setColumn(DependentModel()->lastRow() * factor, 3);
    LocalTable()->setColumn(DependentModel()->lastRow() * factor, 4);

    (*GlobalTable())[0] = GuessK(0);

    Calculate();
}

void fl_ItoI_Model::OptimizeParameters_Private()
{
    QString host = getOption(Host);
    QString guest = getOption(Guest);
    QString hostonly = getOption(HostOnly);

    addGlobalParameter(0);
    //if (host == "no")
    if (hostonly == "no") {
        if (host == "yes") {
            addLocalParameter(0);
        }
        // if (guest == "no")
        addLocalParameter(1);
        if (guest == "dynamic")
            addLocalParameter(2);
        addLocalParameter(3);
    }
}

void fl_ItoI_Model::CalculateVariables()
{
    auto hostguest = getHostGuestPair();
    QString guest_option = getOption(Guest);
    QString host_option = getOption(Host);
    QString hostonly = getOption(HostOnly);

    qreal value = 0;
    const double K = pow(10, GlobalParameter(0));
    for (int i = DataBegin(); i < DataEnd(); ++i) {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        qreal host = ItoI::HostConcentration(host_0, guest_0, GlobalParameter(0));
        qreal complex = host_0 - host;
        qreal guest = guest_0 - complex;

        Vector vector(4);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest_0 - complex;
        vector(3) = complex;

        if (!m_fast)
            SetConcentration(i, vector);

        for (int j = 0; j < SeriesCount(); ++j) {
            double A0 = LocalTable()->data(j, 0);
            double A = LocalTable()->data(j, 1);
            double B = LocalTable()->data(j, 2);
            double AB = A0 - LocalTable()->data(j, 3);
            double tau = 1;
            double guest_part = 0;

            if (host_option == "yes")
                tau = A / A0;
            if (guest_option == "dynamic")
                guest_part = B / A0 * guest;

            if (i == 0)
                value = host * A0;
            else
                value = tau + AB / A * (guest * K) / (1 + guest * K) + guest_part;
            if (i != 0)
                value *= m_model_signal->data(0, j);
            SetValue(i, j, value);
        }
    }
}

QVector<qreal> fl_ItoI_Model::DeCompose(int datapoint, int series) const
{
    QVector<qreal> vector;
    qreal host_0 = InitialHostConcentration(datapoint);

    Vector concentration = getConcentration(datapoint);

    qreal host = concentration(1);

    qreal complex = concentration(3);

    host_0 = 1;

    vector << host / host_0 * LocalTable()->data(series, 0);
    vector << complex / host_0 * LocalTable()->data(series, 1);

    return vector;
}

QSharedPointer<AbstractModel> fl_ItoI_Model::Clone(bool statistics)
{
    QSharedPointer<AbstractModel> model = QSharedPointer<fl_ItoI_Model>(new fl_ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

QString fl_ItoI_Model::ModelInfo() const
{
    QString result = AbstractTitrationModel::ModelInfo();
    result += BC50::ItoI::Format_BC50(GlobalParameter(0));

    return result;
}

QString fl_ItoI_Model::AdditionalOutput() const
{
    QString result;
    return result;

    // double max = 1e3;
    double delta = 1e-3;
    qreal host_0 = 1.0;
    qreal host = 0;
    qreal diff = host_0 - host;
    Vector integral(3);
    qreal end = delta;
    for (end = delta; diff > 1e-5; end += delta) {
        qreal guest_0 = end;
        host = ItoI::HostConcentration(host_0, guest_0, GlobalParameter(0));
        qreal complex = host_0 - host;

        integral(0) += host * delta;
        integral(1) += (guest_0 - complex) * delta;
        integral(2) += complex * delta;

        diff = host_0 - complex;
        // std::cout << end << " " << diff << " " << host << " " << " " << guest_0 - complex << " " << complex << std::endl;
        //std::cout << host << " "
        //          << " " << guest_0 - complex << " " << complex << std::endl;
        //std::cout << integral.transpose() << std::endl;
    }
    integral(0) /= end;
    integral(1) /= end;
    integral(2) /= end;
    std::cout << integral.transpose() << std::endl;
}

QString fl_ItoI_Model::ParameterComment(int parameter) const
{
    Q_UNUSED(parameter)
    return QString("Reaction: A + B &#8652; AB");
}

QString fl_ItoI_Model::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{
    QString result = AbstractTitrationModel::AnalyseMonteCarlo(object, forceAll);

    if (!forceAll)
        return result;

    QString bc = Statistic::MonteCarlo2BC50_1(GlobalParameter(0), object);
    return bc + result;
}

QString fl_ItoI_Model::AnalyseGridSearch(const QJsonObject& object, bool forceAll) const
{
    QString result = AbstractTitrationModel::AnalyseGridSearch(object, forceAll);

    if (!forceAll)
        return result;

    QString bc = Statistic::GridSearch2BC50_1(GlobalParameter(0), object);
    return bc + result;
}

#include "fl_1_1_Model.moc"

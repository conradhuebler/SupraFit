/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QJsonObject>
#include <QtCore/QtMath>

#include <cfloat>
#include <cmath>
#include <iostream>

#include "uv_vis_2_1_1_1_Model.h"

uv_vis_IItoI_ItoI_Model::uv_vis_IItoI_ItoI_Model(DataClass* data)
    : AbstractTitrationModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

uv_vis_IItoI_ItoI_Model::uv_vis_IItoI_ItoI_Model(AbstractTitrationModel* data)
    : AbstractTitrationModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

uv_vis_IItoI_ItoI_Model::~uv_vis_IItoI_ItoI_Model()
{
}

void uv_vis_IItoI_ItoI_Model::DeclareOptions()
{
    QStringList cooperativity = QStringList() << "full"
                                              << "noncooperative"
                                              << "additive"
                                              << "statistical";
    addOption(Cooperativity, "Cooperativity", cooperativity);

    AbstractTitrationModel::DeclareOptions();
}

void uv_vis_IItoI_ItoI_Model::EvaluateOptions()
{
    QString cooperativitiy = getOption(Cooperativity);
    /*
     * Chem. Soc. Rev., 2017, 46, 2622--2637
     * K11 = 4*K21 | K21 = 0.25 K11
     * valid for statistical and noncooperative systems
     */
    auto global_coop = [this]() {
        (*this->GlobalTable())[0] = log10(double(0.25) * qPow(10, (*this->GlobalTable())[1]));
    };
    /*
     * Chem. Soc. Rev., 2017, 46, 2622--2637
     * Y(A2B) = 2Y(AB)
     * valid for statistical and additive systems
     * We first have to subtract the Host_0 Shift and afterwards calculate the new Signal
     */
    auto local_coop = [this]() {
        for (int i = 0; i < this->SeriesCount(); ++i)
            this->LocalTable()->data(i, 1) = 2 * (this->LocalTable()->data(i, 2) - this->LocalTable()->data(i, 0)) + this->LocalTable()->data(i, 0);
    };

    if (cooperativitiy == "noncooperative") {
        global_coop();
    } else if (cooperativitiy == "additive") {
        local_coop();
    } else if (cooperativitiy == "statistical") {
        local_coop();
        global_coop();
    }
    AbstractTitrationModel::EvaluateOptions();
}

void uv_vis_IItoI_ItoI_Model::InitialGuess_Private()
{
    qreal factor = 1;
    factor = 1 / InitialHostConcentration(0);

    int index_21 = 0;

    for (int i = 0; i < DataPoints(); ++i)
        if (XValue(i) < 0.5)
            index_21 = i;

    LocalTable()->setColumn(DependentModel()->firstRow() * factor, 0);
    LocalTable()->setColumn(DependentModel()->Row(index_21) * factor, 2);
    LocalTable()->setColumn(DependentModel()->lastRow() * factor, 3);

    qreal K = GuessK(1);

    (*GlobalTable())[1] = K * 0.8;
    (*GlobalTable())[0] = K / 2.0;

    Calculate();
}

void uv_vis_IItoI_ItoI_Model::CalculateVariables()
{
    auto hostguest = getHostGuestPair();

    qreal K21 = qPow(10, GlobalParameter(0));
    qreal K11 = qPow(10, GlobalParameter(1));

    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        qreal host = IItoI_ItoI::HostConcentration(host_0, guest_0, QList<qreal>() << K21 << K11);
        qreal guest = guest_0 / (K11 * host + K11 * K21 * host * host + 1);
        qreal complex_11 = K11 * host * guest;
        qreal complex_21 = K11 * K21 * host * host * guest;

        Vector vector(5);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;
        vector(3) = complex_21;
        vector(4) = complex_11;

        if (!m_fast)
            SetConcentration(i, vector);

        qreal value = 0;
        for (int j = 0; j < SeriesCount(); ++j) {
            value = host * LocalTable()->data(j, 0) * hostguest.first + guest * LocalTable()->data(j, 1) * hostguest.second + 2 * complex_21 * LocalTable()->data(j, 2) + complex_11 * LocalTable()->data(j, 3);
            SetValue(i, j, value);
        }
    }
}

QVector<qreal> uv_vis_IItoI_ItoI_Model::DeCompose(int datapoint, int series) const
{
    QVector<qreal> vector;

    qreal host_0 = InitialHostConcentration(datapoint);

    Vector concentration = getConcentration(datapoint);

    qreal host = concentration(1);

    qreal complex_21 = concentration(3);
    qreal complex_11 = concentration(4);

    host_0 = 1;

    vector << host / host_0 * LocalTable()->data(series, 0);
    vector << 2 * complex_21 / host_0 * LocalTable()->data(series, 1);
    vector << complex_11 / host_0 * LocalTable()->data(series, 2);

    return vector;
}

void uv_vis_IItoI_ItoI_Model::OptimizeParameters_Private()
{
    QString coop21 = getOption(Cooperativity);
    QString host = getOption(Host);
    QString guest = getOption(Guest);

    if (coop21 == "additive" || coop21 == "full")
        addGlobalParameter(0);

    addGlobalParameter(1);

    if (host == "no")
        addLocalParameter(0);

    if (guest == "no")
        addLocalParameter(1);

    if (coop21 != "additive" && coop21 != "statistical")
        addLocalParameter(2);

    addLocalParameter(3);
}

QSharedPointer<AbstractModel> uv_vis_IItoI_ItoI_Model::Clone(bool statistics)
{
    QSharedPointer<uv_vis_IItoI_ItoI_Model> model = QSharedPointer<uv_vis_IItoI_ItoI_Model>(new uv_vis_IItoI_ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

QString uv_vis_IItoI_ItoI_Model::ParameterComment(int parameter) const
{
    if (parameter == 0)
        return QString("Reaction: AB + A &#8652; A<sub>2</sub>B");
    else
        return QString("Reaction: A + B &#8652; AB");
}

QString uv_vis_IItoI_ItoI_Model::ModelInfo() const
{
    QString result = AbstractTitrationModel::ModelInfo();
    result += BC50::IItoI::Format_BC50(GlobalParameter(0), GlobalParameter(1));

    return result;
}

QString uv_vis_IItoI_ItoI_Model::AdditionalOutput() const
{
    QString result;

    // double max = 1e3;
    /*
    double delta = 1e-3;
    qreal host_0 = 1e-1;
    qreal host = 0;
    qreal diff = host_0 - host;
    Vector integral(4);
    qreal end = delta;

    qreal K21 = qPow(10, GlobalParameter(0));
    qreal K11 = qPow(10, GlobalParameter(1));

    for (end = delta; diff > 1e-5; end += delta) {
        qreal guest_0 = end;

        host = IItoI_ItoI::HostConcentration(host_0, guest_0, QList<qreal>() << K21 << K11);

        qreal guest = guest_0 / (K11 * host + K11 * K21 * host * host + 1);
        qreal complex_11 = K11 * host * guest;
        qreal complex_21 = K11 * K21 * host * host * guest;

        integral(0) += host * delta;
        integral(1) += guest * delta;
        integral(2) += complex_21 * delta;
        integral(3) += complex_11 * delta;

        diff = host;
        // std::cout << end << " " << diff << " " << host << " " << " " << guest_0 - complex << " " << complex << std::endl;
        //std::cout << host << " "
        //    << " " << guest << " " << " "  << complex_21 << " " << complex_11<< std::endl;
        //std::cout << integral.transpose() << std::endl;
    }

    integral(0) /= end;
    integral(1) /= end;
    integral(2) /= end;
    integral(3) /= end;

    std::cout << integral.transpose() << std::endl;

    result += QString("A2B integ  ... %1\n\n").arg(integral(2));
    */
    return result;
}

QString uv_vis_IItoI_ItoI_Model::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{

    QString result = AbstractTitrationModel::AnalyseMonteCarlo(object, forceAll);

    if (!forceAll)
        return result;

    QString bc = Statistic::MonteCarlo2BC50_2_1(GlobalParameter(0), GlobalParameter(1), object);
    return bc + result;
}

QString uv_vis_IItoI_ItoI_Model::AnalyseGridSearch(const QJsonObject& object, bool forceAll) const
{

    QString result = AbstractTitrationModel::AnalyseGridSearch(object, forceAll);

    if (!forceAll)
        return result;

    QString bc = Statistic::GridSearch2BC50_2_1(GlobalParameter(0), GlobalParameter(1), object);
    return bc + result;
}
#include "uv_vis_2_1_1_1_Model.moc"

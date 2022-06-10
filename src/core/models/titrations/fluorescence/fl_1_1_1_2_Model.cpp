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

#include <QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtMath>

#include <cfloat>
#include <cmath>
#include <functional>
#include <iostream>

#include "fl_1_1_1_2_Model.h"

fl_ItoI_ItoII_Model::fl_ItoI_ItoII_Model(DataClass* data)
    : AbstractTitrationModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

fl_ItoI_ItoII_Model::fl_ItoI_ItoII_Model(AbstractTitrationModel* data)
    : AbstractTitrationModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

fl_ItoI_ItoII_Model::~fl_ItoI_ItoII_Model()
{
}

void fl_ItoI_ItoII_Model::DeclareOptions()
{
    QStringList cooperativity = QStringList() << "full"
                                              << "noncooperative"
                                              << "additive"
                                              << "statistical";
    addOption(Cooperativity, "Cooperativity", cooperativity);

    AbstractTitrationModel::DeclareOptions();
}

void fl_ItoI_ItoII_Model::EvaluateOptions()
{
    QString cooperativitiy = getOption(Cooperativity);
    /*
     * Chem. Soc. Rev., 2017, 46, 2622--2637
     * K11 = 4*K12 | K12 = 0.25 K11
     * valid for statistical and noncooperative systems
     */
    auto global_coop = [this]() {
        (*this->GlobalTable())[1] = log10(double(0.25) * qPow(10, (*this->GlobalTable())[0]));
    };

    /*
     * Chem. Soc. Rev., 2017, 46, 2622--2637
     * Y(AB2) = 2Y(AB)
     * valid for statistical and additive systems
     * We first have to subtract the Host_0 Shift and afterwards calculate the new Signal
     */
    auto local_coop = [this]() {
        for (int i = 0; i < this->SeriesCount(); ++i)
            this->LocalTable()->data(i, 2) = 2 * (this->LocalTable()->data(i, 1) - this->LocalTable()->data(i, 0)) + this->LocalTable()->data(i, 0);
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

void fl_ItoI_ItoII_Model::InitialGuess_Private()
{
    qreal factor = 1;
    factor = 1 / InitialHostConcentration(0);

    int index_11 = 0;

    for (int i = 0; i < DataPoints(); ++i)
        if (XValue(i) <= 1)
            index_11 = i;

    LocalTable()->setColumn(DependentModel()->firstRow() * factor, 0);
    LocalTable()->setColumn(DependentModel()->firstRow() * factor, 1);
    LocalTable()->setColumn(DependentModel()->Row(index_11) * factor, 2);
    LocalTable()->setColumn(DependentModel()->lastRow() * factor, 3);
    qreal K = GuessK(0);
    (*GlobalTable())[0] = 0.8 * K;

    (*GlobalTable())[1] = 0.5 * K;

    Calculate();
}

void fl_ItoI_ItoII_Model::OptimizeParameters_Private()
{
    QString coop12 = getOption(Cooperativity);
    QString host = getOption(Host);
    QString guest = getOption(Guest);

    addGlobalParameter(0);

    //if (coop12 == "additive" || coop12 == "full")
    addGlobalParameter(1);

    //if (host == "no")
    addLocalParameter(0);

    //if (guest == "no")
    addLocalParameter(1);

    addLocalParameter(2);

    //if (!(coop12 == "additive" || coop12 == "statistical"))
    addLocalParameter(3);
}

void fl_ItoI_ItoII_Model::CalculateVariables()
{
    auto hostguest = getHostGuestPair();

    qreal K11 = qPow(10, GlobalParameter(0));
    qreal K12 = qPow(10, GlobalParameter(1));

    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);

        qreal host = ItoI_ItoII::HostConcentration(host_0, guest_0, QList<qreal>() << K11 << K12);
        qreal guest = ItoI_ItoII::GuestConcentration(host_0, guest_0, QList<qreal>() << K11 << K12);
        qreal complex_11 = K11 * host * guest;
        qreal complex_12 = K11 * K12 * host * guest * guest;

        Vector vector(5);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;
        vector(3) = complex_11;
        vector(4) = complex_12;

        if (!m_fast)
            SetConcentration(i, vector);

        qreal value = 0;
        for (int j = 0; j < SeriesCount(); ++j) {
#pragma message("examine stuff here")
            if (i == 0) {
                value = (host_0 * LocalTable()->data(j, 0));
            } else {
                value = (host * LocalTable()->data(j, 1))
                    + complex_11 * LocalTable()->data(j, 2)
                    + complex_12 * LocalTable()->data(j, 3);
            }
            SetValue(i, j, value);
        }
    }
}

QVector<qreal> fl_ItoI_ItoII_Model::DeCompose(int datapoint, int series) const
{
    QVector<qreal> vector;

    qreal host_0 = InitialHostConcentration(datapoint);

    Vector concentration = getConcentration(datapoint);

    qreal host = concentration(1);

    qreal complex_11 = concentration(3);
    qreal complex_12 = concentration(4);

    host_0 = 1;

    vector << host / host_0 * LocalTable()->data(series, 0);
    vector << complex_11 / host_0 * LocalTable()->data(series, 1);
    vector << complex_12 / host_0 * LocalTable()->data(series, 2);

    return vector;
}

QSharedPointer<AbstractModel> fl_ItoI_ItoII_Model::Clone(bool statistics)
{
    QSharedPointer<fl_ItoI_ItoII_Model> model = QSharedPointer<fl_ItoI_ItoII_Model>(new fl_ItoI_ItoII_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

QString fl_ItoI_ItoII_Model::ParameterComment(int parameter) const
{
    if (parameter == 0)
        return QString("Reaction: A + A &#8652; AB");
    else
        return QString("Reaction: AB + B &#8652; AB<sub>2</sub>");
}

QString fl_ItoI_ItoII_Model::ModelInfo() const
{
    QString result = AbstractTitrationModel::ModelInfo();
    result += BC50::ItoII::Format_BC50(GlobalParameter(0), GlobalParameter(1));

    return result;
}

QString fl_ItoI_ItoII_Model::AdditionalOutput() const
{
    QString result;

    // double max = 1e3;
    /*
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
        std::cout << host << " "
                  << " " << guest_0 - complex << " " << complex << std::endl;
        //std::cout << integral.transpose() << std::endl;
    }
    integral(0) /= end;
    integral(1) /= end;
    integral(2) /= end;
    std::cout << integral.transpose() << std::endl;
    */
    return result;
}

QString fl_ItoI_ItoII_Model::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{

    QString result = AbstractTitrationModel::AnalyseMonteCarlo(object, forceAll);

    if (!forceAll)
        return result;

    QString bc = Statistic::MonteCarlo2BC50_1_2(GlobalParameter(0), GlobalParameter(1), object);
    return bc + result;
}

QString fl_ItoI_ItoII_Model::AnalyseGridSearch(const QJsonObject& object, bool forceAll) const
{

    QString result = AbstractTitrationModel::AnalyseGridSearch(object, forceAll);

    if (!forceAll)
        return result;

    QString bc = Statistic::GridSearch2BC50_1_2(GlobalParameter(0), GlobalParameter(1), object);
    return bc + result;
}

#include "fl_1_1_1_2_Model.moc"

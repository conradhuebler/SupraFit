/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/models.h"
#include "src/core/toolset.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>

#include "fl_2_1_1_1_Model.h"

fl_IItoI_ItoI_Model::fl_IItoI_ItoI_Model(DataClass* data)
    : AbstractTitrationModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
}

fl_IItoI_ItoI_Model::~fl_IItoI_ItoI_Model()
{
}

void fl_IItoI_ItoI_Model::DeclareOptions()
{
    QStringList cooperativity = QStringList() << "full"
                                              << "noncooperative"
                                              << "additive"
                                              << "statistical";
    addOption(Cooperativity, "Cooperativity", cooperativity);

    // QStringList host = QStringList() << "Host" << "no Host";
    // addOption("Host", host);
    // setOption("Host", "Host");
}

void fl_IItoI_ItoI_Model::EvaluateOptions()
{

    QString cooperativitiy = getOption(Cooperativity);

    auto global_coop = [this]() {
        (*this->GlobalTable())[0] = log10(double(0.25) * qPow(10, (*this->GlobalTable())[1]));
    };

    auto local_coop = [this]() {
        for (int i = 0; i < this->SeriesCount(); ++i)
            this->LocalTable()->data(1, i) = 2 * (this->LocalTable()->data(2, i) - this->LocalTable()->data(0, i)) + this->LocalTable()->data(0, i);
    };

    if (cooperativitiy == "noncooperative") {
        global_coop();
    } else if (cooperativitiy == "additive") {
        local_coop();
    } else if (cooperativitiy == "statistical") {
        local_coop();
        global_coop();
    }

    // QString host = getOption("Host");
    /*qDebug() << host;
    if(host != "Host")
    {
         for(int i = 0; i < SeriesCount(); ++i)
         {
             qDebug() << "bang";
            this->LocalTable()->data(0,i) = 0;
            this->LocalTable()->data(1,i) = 0;
         }
    }*/
}

void fl_IItoI_ItoI_Model::InitialGuess_Private()
{
    (*GlobalTable())[1] = Guess_1_1();
    (*GlobalTable())[0] = (*GlobalTable())[1] / 2;

    qreal factor = InitialHostConcentration(0);

    LocalTable()->setColumn(DependentModel()->firstRow() / factor / 1e3, 0);
    LocalTable()->setColumn(DependentModel()->firstRow() / factor / 1e3, 1);
    LocalTable()->setColumn(DependentModel()->lastRow() / factor / 1e4, 2);
    LocalTable()->setColumn(DependentModel()->lastRow() / factor / 1e4, 3);

    Calculate();
}

void fl_IItoI_ItoI_Model::CalculateVariables()
{
    m_sum_absolute = 0;
    m_sum_squares = 0;

    qreal K21 = qPow(10, GlobalParameter(0));
    qreal K11 = qPow(10, GlobalParameter(1));

    QVector<qreal> F0(SeriesCount());

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
            if (i == 0) {
                F0[j] = host_0 * LocalTable()->data(0, j);
                value = F0[j];
            } else
                value = (host * LocalTable()->data(1, j) + 2 * complex_21 * LocalTable()->data(2, j) + complex_11 * LocalTable()->data(3, j));

            SetValue(i, j, value * 1e3);
        }
    }
}

QVector<qreal> fl_IItoI_ItoI_Model::OptimizeParameters_Private()
{
    QString coop21 = getOption(Cooperativity);

        if (coop21 == "additive" || coop21 == "full")
            addGlobalParameter(0);

        addGlobalParameter(1);

        addLocalParameter(0);
        if (coop21 != "additive" && coop21 != "statistical")
            addLocalParameter(1);
        addLocalParameter(2);

    QVector<qreal> parameter;
    for (int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

QSharedPointer<AbstractModel> fl_IItoI_ItoI_Model::Clone()
{
    QSharedPointer<fl_IItoI_ItoI_Model> model = QSharedPointer<fl_IItoI_ItoI_Model>(new fl_IItoI_ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}


#include "fl_2_1_1_1_Model.moc"

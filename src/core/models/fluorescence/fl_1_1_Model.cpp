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

#include "src/core/AbstractTitrationModel.h"
#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/minimizer.h"
#include "src/core/statistic.h"

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
}

fl_ItoI_Model::~fl_ItoI_Model()
{
}

void fl_ItoI_Model::InitialGuess_Private()
{
    (*GlobalTable())[0] = Guess_1_1();

    qreal factor = InitialHostConcentration(0);

    LocalTable()->setColumn(DependentModel()->firstRow() / factor / 1e3, 0);
    LocalTable()->setColumn(DependentModel()->lastRow() / factor / 1e3, 1);
    LocalTable()->setColumn(DependentModel()->lastRow() / factor / 1e3, 2);

    Calculate();
}

void fl_ItoI_Model::DeclareOptions()
{
    //QStringList method = QStringList() << "Host" << "no Host";
    //addOption("Host", method);
}

void fl_ItoI_Model::EvaluateOptions()
{
    /*
    QString host = getOption("Host");
    if(host != "Host")
    {
         for(int i = 0; i < SeriesCount(); ++i)
         {
            this->LocalTable()->data(0,i) = 0;
            this->LocalTable()->data(1,i) = 0;
         }
    }*/
}

QVector<qreal> fl_ItoI_Model::OptimizeParameters_Private()
{
        addGlobalParameter(0);

        addLocalParameter(0);
        addLocalParameter(1);

    QVector<qreal> parameter;
    for (int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

void fl_ItoI_Model::CalculateVariables()
{
    qreal value;
    QVector<qreal> F0(SeriesCount());
    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        qreal host = ItoI::HostConcentration(host_0, guest_0, GlobalParameter(0));
        qreal complex = host_0 - host;
        Vector vector(4);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest_0 - complex;
        vector(3) = complex;

        if (!m_fast)
            SetConcentration(i, vector);

        for (int j = 0; j < SeriesCount(); ++j) {
            if (i == 0) {
                F0[j] = host_0 * LocalTable()->data(0, j);
                value = F0[j];
            } else
                value = (host * LocalTable()->data(1, j) + complex * LocalTable()->data(2, j));

            SetValue(i, j, value * 1e3);
        }
    }
}

QSharedPointer<AbstractModel> fl_ItoI_Model::Clone()
{
    QSharedPointer<AbstractModel> model = QSharedPointer<fl_ItoI_Model>(new fl_ItoI_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel());
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

QString fl_ItoI_Model::ModelInfo() const
{
    QString result = AbstractTitrationModel::ModelInfo();
    result += BC50::Format_ItoI_BC50(GlobalParameter(0));

    return result;
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

#include "fl_1_1_Model.moc"

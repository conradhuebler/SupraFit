/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/bc50.h"
#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/models.h"
#include "src/core/toolset.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>

#include <functional>

#include "fl_1_1_1_2_Model.h"

fl_ItoI_ItoII_Model::fl_ItoI_ItoII_Model(DataClass* data)
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

    // QStringList host = QStringList() << "Host" << "no Host";
    // addOption("Host", host);
}

void fl_ItoI_ItoII_Model::EvaluateOptions()
{

    QString cooperativitiy = getOption(Cooperativity);

    auto global_coop = [this]() {
        (*this->GlobalTable())[1] = log10(double(0.25) * qPow(10, (*this->GlobalTable())[0]));
    };

    auto local_coop = [this]() {
        for (int i = 0; i < this->SeriesCount(); ++i)
            this->LocalTable()->data(2, i) = 2 * (this->LocalTable()->data(1, i) - this->LocalTable()->data(0, i)) + this->LocalTable()->data(0, i);
    };

    if (cooperativitiy == "noncooperative") {
        global_coop();
    } else if (cooperativitiy == "additive") {
        local_coop();
    } else if (cooperativitiy == "statistical") {
        local_coop();
        global_coop();
    }
    /*
    QString host = getOption("Host");
    if(host != "Host")
    {
         for(int i = 0; i < SeriesCount(); ++i)
         {
            this->LocalTable()->data(0,i) = 0;
            this->LocalTable()->data(1,i) = 0;
         }
    }
    */
}

void fl_ItoI_ItoII_Model::InitialGuess_Private()
{
    (*GlobalTable())[0] = 4;
    (*GlobalTable())[1] = 2;

    qreal factor = InitialHostConcentration(0);

    LocalTable()->setColumn(DependentModel()->firstRow() / factor / 1e3, 0);
    LocalTable()->setColumn(DependentModel()->firstRow() / factor / 1e4, 1);
    LocalTable()->setColumn(DependentModel()->lastRow() / factor / 1e4, 2);
    LocalTable()->setColumn(DependentModel()->lastRow() / factor / 1e4, 3);

    AbstractTitrationModel::Calculate();
}

QVector<qreal> fl_ItoI_ItoII_Model::OptimizeParameters_Private()
{
    QString coop12 = getOption(Cooperativity);
        addGlobalParameter(0);

        if (coop12 == "additive" || coop12 == "full")
            addGlobalParameter(1);

        addLocalParameter(0);
        addLocalParameter(1);
        if (!(coop12 == "additive" || coop12 == "statistical"))
            addLocalParameter(2);

    QVector<qreal> parameter;
    for (int i = 0; i < m_opt_para.size(); ++i)
        parameter << *m_opt_para[i];
    return parameter;
}

void fl_ItoI_ItoII_Model::CalculateVariables()
{
    QString method = getOption(Method);

    qreal K11 = qPow(10, GlobalParameter(0));
    qreal K12 = qPow(10, GlobalParameter(1));

    QVector<qreal> F0(SeriesCount());

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
            if (i == 0) {
                F0[j] = host_0 * LocalTable()->data(0, j);
                value = F0[j];
            } else
                value = (host * LocalTable()->data(1, j) + complex_11 * LocalTable()->data(2, j) + complex_12 * LocalTable()->data(3, j));

            SetValue(i, j, value * 1e3);
        }
    }
}

QSharedPointer<AbstractModel> fl_ItoI_ItoII_Model::Clone()
{
    QSharedPointer<fl_ItoI_ItoII_Model> model = QSharedPointer<fl_ItoI_ItoII_Model>(new fl_ItoI_ItoII_Model(this), &QObject::deleteLater);
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->ImportModel(ExportModel());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

QString fl_ItoI_ItoII_Model::ParameterComment(int parameter) const
{
    if (parameter == 1)
        return QString("Reaction: AB + A &#8652; A<sub>2</sub>B");
    else
        return QString("Reaction: A + B &#8652; AB");
}

QString fl_ItoI_ItoII_Model::ModelInfo() const
{
    QString result = AbstractTitrationModel::ModelInfo();
    result += BC50::Format_ItoII_BC50(GlobalParameter(0), GlobalParameter(1));
    return result;
}

QString fl_ItoI_ItoII_Model::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{

    QString result = AbstractTitrationModel::AnalyseMonteCarlo(object);

    if (!forceAll)
        return result;

    QStringList models = object["controller"].toObject()["raw"].toObject().keys();

    QList<qreal> s, s_sf;

    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
        qreal logK11 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[0];
        qreal logK12 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[1];

        s << BC50::ItoI_ItoII_BC50(logK11, logK12) * 1e6;
        s_sf << BC50::ItoI_ItoII_BC50_SF(logK11, logK12) * 1e6;
    }
    std::sort(s.begin(), s.end());
    std::sort(s_sf.begin(), s_sf.end());

    SupraFit::ConfidenceBar conf = ToolSet::Confidence(s, 95);
    SupraFit::ConfidenceBar conf_sf = ToolSet::Confidence(s_sf, 95);

    qreal BC50 = BC50::ItoI_ItoII_BC50(GlobalParameter(0), GlobalParameter(1)) * 1e6;
    qreal BC50_sf = BC50::ItoI_ItoII_BC50_SF(GlobalParameter(0), GlobalParameter(1)) * 1e6;

    qreal conf_dSl = conf.upper - BC50;
    qreal conf_dSu = BC50 - conf.lower;

    qreal conf_dSl_sf = conf_sf.upper - BC50_sf;
    qreal conf_dSu_sf = BC50_sf - conf_sf.lower;

    result += tr("<p>BC50 %1 [+%2,-%3] %4M ... ").arg(BC50).arg(conf_dSu).arg(conf_dSl).arg(QChar(956));
    result += tr("[%1 - %2] %3M</p>").arg(conf.lower).arg(conf.upper).arg(QChar(956));

    result += tr("<p>BC50 (SF) %1 [+%2,-%3] %4M ... ").arg(BC50_sf).arg(conf_dSu_sf).arg(conf_dSl_sf).arg(QChar(956));
    result += tr("[%1 - %2] %3M</p>").arg(conf_sf.lower).arg(conf_sf.upper).arg(QChar(956));

    return result;
}

#include "fl_1_1_1_2_Model.moc"

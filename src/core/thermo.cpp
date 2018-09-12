/*
 * <one line to give the library's name and an idea of what it does.>
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

#include "thermo.h"

#include <QtCore/QJsonObject>

#include "src/core/models.h"
#include "src/core/toolset.h"

namespace Thermo {

QString Statistic2Thermo(qreal K, qreal H, qreal T, const QJsonObject& object)
{
    QString result;

    qreal K11u = 0, K11l = 0, dH11l = 0, dH11u = 0;

    int index_global = 0;
    int index_local = 0;
    qreal error;
    if (!object.isEmpty()) {
        const QStringList keys = object.keys();
        for (int i = 0; i < keys.size(); ++i) {
            QJsonObject confidence = object[keys[i]].toObject()["confidence"].toObject();
            if (qFuzzyCompare(object[keys[i]].toObject()["value"].toDouble(), K)) {
                K11u = confidence["upper"].toDouble();
                K11l = confidence["lower"].toDouble();
                index_global = object[keys[i]].toObject()["index"].toString().toInt();
                error = 100 - confidence["error"].toDouble();
            }

            if (qFuzzyCompare(object[keys[i]].toObject()["value"].toDouble(), H)) {
                dH11u = confidence["upper"].toDouble();
                dH11l = confidence["lower"].toDouble();
                index_local = object[keys[i]].toObject()["index"].toString().split("|")[0].toInt();
            }
        }
    }

    qreal dG = ToolSet::K2G(K, T);
    qreal dS = ToolSet::GHE(dG, H, T);

    qreal dGl = ToolSet::K2G(K11u, T);
    qreal dGu = ToolSet::K2G(K11l, T);

    qreal conf_dGu = dGu - dG;
    qreal conf_dGl = dG - dGl;


    result += "<table>";
    result += "<tr><td><b>Complexation Constant K </b></td><td>" + Print::printDouble(qPow(10, K)) + "</td>";

    if (!object.isEmpty())
        result += "<td> (+" + Print::printDouble(qPow(10, K11u) - qPow(10, K)) + "/-" + Print::printDouble(qPow(10, K) - qPow(10, K11l)) + ")</td>";
    result += "<td> M</td></tr>";

    if (!object.isEmpty())
        result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td></td><td>M</td></tr>").arg(qPow(10, K11l)).arg(qPow(10, K11u));

    result += "<tr><td><b>Free Enthalpy of Complexation &Delta;G </b></td><td>" + Print::printDouble(dG / 1000.0, 3) + "</td>";
    if (!object.isEmpty())
        result += "<td> (+" + Print::printDouble(conf_dGu / 1000, 3) + "/-" + Print::printDouble(conf_dGl / 1000, 3) + ")</td>";
    result += "<td>kJ/mol</td></tr>";
    if (!object.isEmpty())
        result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td></td><td>kJ/mol</td></tr>").arg(dGl / 1000.0).arg(dGu / 1000.0);

    if (!qFuzzyCompare(H, 0)) {

        QStringList models = object["controller"].toObject()["raw"].toObject().keys();
        QList<qreal> s;

        for (int i = 0; i < models.size(); ++i) {
            QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
            qreal K = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[index_global];
            QVector<qreal> local = ToolSet::String2DoubleVec(model["localParameter"].toObject()["data"].toObject()["0"].toString());
            qreal H = local[index_local];
            s << ToolSet::GHE(ToolSet::K2G(K, T), H, T);
        }

        std::sort(s.begin(), s.end());

        SupraFit::ConfidenceBar conf = ToolSet::Confidence(s, error);

        qreal conf_dH11u = dH11u - H;
        qreal conf_dH11l = H - dH11l;

        qreal conf_dSu = conf.upper - dS;
        qreal conf_dSl = dS - conf.lower;

        result += "<tr><td><b>Enthalpy of Complexation &Delta;H</b></td><td>" + Print::printDouble(H / 1000.0) + "</td>";

        if (!object.isEmpty())
            result += "<td>(" + Print::printDouble(conf_dH11u / 1000.0, 3) + "/-" + Print::printDouble(conf_dH11l / 1000.0, 3) + ")</td>";
        result += "<td>kJ/mol</td></tr>";

        if (!object.isEmpty())
            result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td></td><td>J/(molK)</td></tr>").arg(dH11l / 1000.0).arg(dH11u / 1000.0);

        result += "<tr><td><b>Entropy of Complexation &Delta;S</b></td><td>" + Print::printDouble(dS) + "</td>";
        if (!object.isEmpty())
            result += "<td>(+" + Print::printDouble(conf_dSu, 3) + "/-" + Print::printDouble(conf_dSl, 3) + ")</td>";
        result += "<td>J/(molK)</td></tr>";
        if (!object.isEmpty())
            result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td></td><td>J/(molK)</td></tr>").arg(conf.lower).arg(conf.upper);
    }
    result += "</table>";

    return result;
}

QString Statistic2BC50_1(const qreal logK11, const QJsonObject& object)
{
    QStringList models = object["controller"].toObject()["raw"].toObject().keys();
    qreal error = 100 - object["0"].toObject()["confidence"].toObject()["error"].toDouble();

    QList<qreal> s;

    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
        qreal logK11 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[0];
        s << BC50::ItoI_BC50(logK11) * 1e6;
    }
    std::sort(s.begin(), s.end());

    SupraFit::ConfidenceBar conf = ToolSet::Confidence(s, error);
    qreal BC50 = BC50::ItoI_BC50(logK11) * 1e6;

    qreal conf_dSl = conf.upper - BC50;
    qreal conf_dSu = BC50 - conf.lower;

    QString result;

    result += QString("<p>BC50 %1 [+%2,-%3] %4M ... ").arg(BC50).arg(conf_dSl).arg(conf_dSu).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(conf.lower).arg(conf.upper).arg(QChar(956));

    return result;
}

QString Statistic2BC50_1_2(const qreal logK11, const qreal logK12, const QJsonObject& object)
{
    QStringList models = object["controller"].toObject()["raw"].toObject().keys();
    QList<qreal> s, s_sf;
    qreal error = 100 - object["0"].toObject()["confidence"].toObject()["error"].toDouble();

    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
        qreal logK11 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[0];
        qreal logK12 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[1];

        s << BC50::ItoI_ItoII_BC50(logK11, logK12) * 1e6;
        s_sf << BC50::ItoI_ItoII_BC50_SF(logK11, logK12) * 1e6;
    }

    std::sort(s.begin(), s.end());
    std::sort(s_sf.begin(), s_sf.end());

    SupraFit::ConfidenceBar conf = ToolSet::Confidence(s, error);
    SupraFit::ConfidenceBar conf_sf = ToolSet::Confidence(s_sf, error);

    qreal BC50 = BC50::ItoI_ItoII_BC50(logK11, logK12) * 1e6;
    qreal BC50_sf = BC50::ItoI_ItoII_BC50_SF(logK11, logK12) * 1e6;

    qreal conf_dSl = conf.upper - BC50;
    qreal conf_dSu = BC50 - conf.lower;

    qreal conf_dSl_sf = conf_sf.upper - BC50_sf;
    qreal conf_dSu_sf = BC50_sf - conf_sf.lower;

    QString result;

    result += QString("<p>BC50 %1 [+%2,-%3] %4M ... ").arg(BC50).arg(conf_dSl).arg(conf_dSu).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(conf.lower).arg(conf.upper).arg(QChar(956));

    result += QString("<p>BC50 (SF) %1 [+%2,-%3] %4M ... ").arg(BC50_sf).arg(conf_dSl_sf).arg(conf_dSu_sf).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(conf_sf.lower).arg(conf_sf.upper).arg(QChar(956));

    return result;
}

QString Statistic2BC50_2_1(const qreal logK21, const qreal logK11, const QJsonObject& object)
{
    QString result;
    QList<qreal> s;

    QStringList models = object["controller"].toObject()["raw"].toObject().keys();
    qreal error = 100 - object["0"].toObject()["confidence"].toObject()["error"].toDouble();

    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
        qreal logK21 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[0];
        qreal logK11 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[1];

        s << BC50::IItoI_ItoI_BC50(logK21, logK11) * 1e6;
    }

    std::sort(s.begin(), s.end());

    SupraFit::ConfidenceBar conf = ToolSet::Confidence(s, error);
    qreal BC50 = BC50::IItoI_ItoI_BC50(logK21, logK11) * 1e6;

    qreal conf_dSl = conf.upper - BC50;
    qreal conf_dSu = BC50 - conf.lower;

    result += QString("<p>BC50 %1 [+%2,-%3] %4M ... ").arg(BC50).arg(conf_dSl).arg(conf_dSu).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(conf.lower).arg(conf.upper).arg(QChar(956));

    return result;
}

QString Statistic2BC50_2_2(const qreal logK21, const qreal logK11, const qreal logK12, const QJsonObject& object)
{
    QStringList models = object["controller"].toObject()["raw"].toObject().keys();
    QList<qreal> s, s_sf;
    qreal error = 100 - object["0"].toObject()["confidence"].toObject()["error"].toDouble();

    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
        qreal logK21 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[0];
        qreal logK11 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[1];
        qreal logK12 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[2];

        s << BC50::IItoI_ItoI_ItoII_BC50(logK21, logK11, logK12) * 1e6;
        s_sf << BC50::IItoI_ItoI_ItoII_BC50_SF(logK21, logK11, logK12) * 1e6;
    }

    std::sort(s.begin(), s.end());
    std::sort(s_sf.begin(), s_sf.end());

    SupraFit::ConfidenceBar conf = ToolSet::Confidence(s, error);
    SupraFit::ConfidenceBar conf_sf = ToolSet::Confidence(s_sf, error);

    qreal BC50 = BC50::IItoI_ItoI_ItoII_BC50(logK21, logK11, logK12) * 1e6;
    qreal BC50_sf = BC50::IItoI_ItoI_ItoII_BC50_SF(logK21, logK11, logK12) * 1e6;

    qreal conf_dSl = conf.upper - BC50;
    qreal conf_dSu = BC50 - conf.lower;

    qreal conf_dSl_sf = conf_sf.upper - BC50_sf;
    qreal conf_dSu_sf = BC50_sf - conf_sf.lower;

    QString result;

    result += QString("<p>BC50 %1 [+%2,-%3] %4M ... ").arg(BC50).arg(conf_dSl).arg(conf_dSu).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(conf.lower).arg(conf.upper).arg(QChar(956));

    result += QString("<p>BC50 (SF) %1 [+%2,-%3] %4M ... ").arg(BC50_sf).arg(conf_dSl_sf).arg(conf_dSu_sf).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(conf_sf.lower).arg(conf_sf.upper).arg(QChar(956));
    return result;
}
}

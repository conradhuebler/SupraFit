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

    if (!object.isEmpty()) {
        const QStringList keys = object.keys();
        for (int i = 0; i < keys.size(); ++i) {
            QJsonObject confidence = object[keys[i]].toObject()["confidence"].toObject();
            if (qFuzzyCompare(object[keys[i]].toObject()["value"].toDouble(), K)) {
                K11u = confidence["upper"].toDouble();
                K11l = confidence["lower"].toDouble();
                index_global = object[keys[i]].toObject()["index"].toString().toInt();
                //   qDebug() << index_global << object[keys[i]].toObject()["index"].toString();
            }

            if (qFuzzyCompare(object[keys[i]].toObject()["value"].toDouble(), H)) {
                dH11u = confidence["upper"].toDouble();
                dH11l = confidence["lower"].toDouble();
                index_local = object[keys[i]].toObject()["index"].toString().split("|")[0].toInt();
                //     qDebug() << index_local << object[keys[i]].toObject()["index"].toString();
            }
        }
    }

    QStringList models = object["controller"].toObject()["raw"].toObject().keys();
    QList<qreal> s;

    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
        qreal K = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[index_global];
        QVector<qreal> local = ToolSet::String2DoubleVec(model["localParameter"].toObject()["data"].toObject()["0"].toString());
        qreal H = local[index_local];
        // qDebug() << K << H;
        s << ToolSet::GHE(ToolSet::K2G(K, T), H, T);
    }

    std::sort(s.begin(), s.end());

    SupraFit::ConfidenceBar conf = ToolSet::Confidence(s, 95);

    qreal dG = ToolSet::K2G(K, T);
    qreal dS = ToolSet::GHE(dG, H, T);

    qreal dGl = ToolSet::K2G(K11u, T);
    qreal dGu = ToolSet::K2G(K11l, T);

    qreal conf_dGu = dG - dGu;
    qreal conf_dGl = dG - dGl;

    /*
        qreal dSll = ToolSet::GHE(dGl, dH11l, T);
        qreal dSul = ToolSet::GHE(dGu, dH11l, T);
        qreal dSuu = ToolSet::GHE(dGu, dH11u, T);
        qreal dSlu = ToolSet::GHE(dGl, dH11u, T);
        */
    // qDebug() << dSll << dSuu << dSul << dSlu << conf.lower << conf.upper << dS;

    qreal conf_dH11u = H - dH11u;
    qreal conf_dH11l = H - dH11l;

    qreal conf_dSl = conf.upper - dS;
    qreal conf_dSu = dS - conf.lower;

    result += "<table>";
    result += "<tr><td><b>Complexation Constant K </b></td><td>" + Print::printDouble(qPow(10, K)) + "</td>";

    if (!object.isEmpty())
        result += "<td> (+" + Print::printDouble(qPow(10, K11u) - qPow(10, K)) + "/-" + Print::printDouble(qPow(10, K) - qPow(10, K11l)) + ")</td>";
    result += "<td> M</td></tr>";

    result += "<tr><td><b>Free Enthalpy of Complexation &Delta;G </b></td><td>" + Print::printDouble(dG / 1000.0, 3) + "</td>";
    if (!object.isEmpty())
        result += "<td> (+" + Print::printDouble(conf_dGu / 1000, 3) + "/-" + Print::printDouble(conf_dGl / 1000, 3) + ")</td>";
    result += "<td>kJ/mol</td></tr>";

    if (!qFuzzyCompare(H, 0)) {
        result += "<tr><td><b>Enthalpy of Complexation &Delta;H</b></td><td>" + Print::printDouble(H / 1000.0) + "</td>";
        if (!object.isEmpty())
            result += "<td>(" + Print::printDouble(conf_dH11u / 1000, 3) + "/-" + Print::printDouble(conf_dH11l / 1000, 3) + ")</td>";
        result += "<td>kJ/mol</td></tr>";

        result += "<tr><td><b>Entropy of Complexation &Delta;S</b></td><td>" + Print::printDouble(dS) + "</td>";
        if (!object.isEmpty())
            result += "<td>(+" + Print::printDouble(conf_dSu, 3) + "/-" + Print::printDouble(conf_dSl, 3) + ")</td>";
        result += "<td>J/(molK)</td></tr>";
    }
    result += "</table>";

    return result;
}
}

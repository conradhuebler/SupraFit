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

#include "src/core/bc50.h"
#include "src/core/models.h"
#include "src/core/toolset.h"

#include "src/global.h"

#include <QtCore/QJsonObject>

#include "statistic.h"

namespace Statistic {

QString MonteCarlo2Thermo(int index, qreal T, const QJsonObject& object, bool heat)
{
    QString result;
    qreal K = 0, H = 0;
    qreal K11u = 0, K11l = 0, dH11l = 0, dH11u = 0;


    qreal error;
    if (!object.isEmpty()) {
        const QStringList keys = object.keys();
        for (int i = 0; i < keys.size(); ++i) {
            QJsonObject confidence = object[keys[i]].toObject()["confidence"].toObject();

            if(QString::number(index) == keys[i])
            {
                K11u = confidence["upper"].toDouble();
                K11l = confidence["lower"].toDouble();
                error = 100 - confidence["error"].toDouble();
                K = object[keys[i]].toObject()["value"].toDouble();
            }

            if (object[keys[i]].toObject()["index"].toString() == QString("%1|0").arg(index)) {
                H = object[keys[i]].toObject()["value"].toDouble();
                dH11u = confidence["upper"].toDouble();
                dH11l = confidence["lower"].toDouble();
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
    result += "<tr><td><b>Complexation Constant K </b></td><td>" + Print::printDouble(qPow(10, K), 3) + "";

    if (!object.isEmpty())
        result += " (+" + Print::printDouble(qPow(10, K11u) - qPow(10, K), 3) + "/-" + Print::printDouble(qPow(10, K) - qPow(10, K11l), 3) + ")</td>";
    result += "<td> M</td></tr>";

    if (!object.isEmpty())
        result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>M</td></tr>").arg(Print::printDouble(qPow(10, K11l), 3)).arg(Print::printDouble(qPow(10, K11u), 3));

    result += "<tr><td></td></tr>";
    result += "<tr><td></td></tr>";

    result += "<tr><td><b>Free Enthalpy of Complexation &Delta;G </b></td><td>" + Print::printDouble(dG / 1000.0, 3) + "";
    if (!object.isEmpty())
        result += " (+" + Print::printDouble(conf_dGu / 1000, 3) + "/-" + Print::printDouble(conf_dGl / 1000, 3) + ")";
    result += "</td><td>kJ/mol</td></tr>";
    if (!object.isEmpty())
        result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>kJ/mol</td></tr>").arg(Print::printDouble(dGl / 1000.0, 3)).arg(Print::printDouble(dGu / 1000.0, 3));

    result += "<tr><td></td></tr>";
    result += "<tr><td></td></tr>";

    if (heat) {

        QStringList models = object["controller"].toObject()["raw"].toObject().keys();
        QList<qreal> s;

        for (int i = 0; i < models.size(); ++i) {
            QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
            qreal K = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[index];
            QVector<qreal> local = ToolSet::String2DoubleVec(model["localParameter"].toObject()["data"].toObject()["0"].toString());
            qreal H = local[index];
            s << ToolSet::GHE(ToolSet::K2G(K, T), H, T);
        }

        std::sort(s.begin(), s.end());

        SupraFit::ConfidenceBar conf = ToolSet::Confidence(s, error);

        qreal conf_dH11u = dH11u - H;
        qreal conf_dH11l = H - dH11l;

        qreal conf_dSu = conf.upper - dS;
        qreal conf_dSl = dS - conf.lower;

        result += "<tr><td><b>Enthalpy of Complexation &Delta;H</b></td><td>" + Print::printDouble(H / 1000.0, 3) + "";

        if (!object.isEmpty())
            result += "(" + Print::printDouble(conf_dH11u / 1000.0, 3) + "/-" + Print::printDouble(conf_dH11l / 1000.0, 3) + ")";
        result += "</td><td>kJ/mol</td></tr>";

        if (!object.isEmpty())
            result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>J/(molK)</td></tr>").arg(Print::printDouble(dH11l / 1000.0, 3)).arg(Print::printDouble(dH11u / 1000.0, 3));


        result += "<tr><td></td></tr>";
        result += "<tr><td></td></tr>";

        result += "<tr><td><b>Entropy of Complexation &Delta;S</b></td><td>" + Print::printDouble(dS, 3) + "";
        if (!object.isEmpty())
            result += "(+" + Print::printDouble(conf_dSu, 3) + "/-" + Print::printDouble(conf_dSl, 3) + ")";
        result += "</td><td>J/(molK)</td></tr>";
        if (!object.isEmpty())
            result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>J/(molK)</td></tr>").arg(Print::printDouble(conf.lower, 3)).arg(Print::printDouble(conf.upper, 3));
    }
    result += "</table>";

    return result;
}

QString MonteCarlo2BC50_1(const qreal logK11, const QJsonObject& object)
{
    QStringList models = object["controller"].toObject()["raw"].toObject().keys();
    qreal error = 100 - object["0"].toObject()["confidence"].toObject()["error"].toDouble();

    QList<qreal> s;

    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
        qreal logK11 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[0];
        s << BC50::ItoI::BC50(logK11) * 1e6;
    }
    std::sort(s.begin(), s.end());

    SupraFit::ConfidenceBar conf = ToolSet::Confidence(s, error);
    qreal BC50 = BC50::ItoI::BC50(logK11) * 1e6;

    qreal conf_dSl = conf.upper - BC50;
    qreal conf_dSu = BC50 - conf.lower;

    QString result;

    result += QString("<p>BC50 %1 [+%2,-%3] %4M ... ").arg(BC50).arg(conf_dSl).arg(conf_dSu).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(conf.lower).arg(conf.upper).arg(QChar(956));

    return result;
}

QString MonteCarlo2BC50_1_2(const qreal logK11, const qreal logK12, const QJsonObject& object)
{
    QStringList models = object["controller"].toObject()["raw"].toObject().keys();
    QList<qreal> s, s_sf;
    qreal error = 100 - object["0"].toObject()["confidence"].toObject()["error"].toDouble();

    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
        qreal logK11 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[0];
        qreal logK12 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[1];

        s << BC50::ItoII::BC50(logK11, logK12) * 1e6;
        //     s_sf << BC50::ItoII::BC50_SF(logK11, logK12) * 1e6;
    }

    std::sort(s.begin(), s.end());
    std::sort(s_sf.begin(), s_sf.end());

    SupraFit::ConfidenceBar conf = ToolSet::Confidence(s, error);
    SupraFit::ConfidenceBar conf_sf = ToolSet::Confidence(s_sf, error);

    qreal BC50 = BC50::ItoII::BC50(logK11, logK12) * 1e6;
    //   qreal BC50_sf = BC50::ItoII::BC50_SF(logK11, logK12) * 1e6;

    qreal conf_dSl = conf.upper - BC50;
    qreal conf_dSu = BC50 - conf.lower;

    //   qreal conf_dSl_sf = conf_sf.upper - BC50_sf;
    //   qreal conf_dSu_sf = BC50_sf - conf_sf.lower;

    QString result;

    result += QString("<p>BC50 %1 [+%2,-%3] %4M ... ").arg(BC50).arg(conf_dSl).arg(conf_dSu).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(conf.lower).arg(conf.upper).arg(QChar(956));

    //  result += QString("<p>BC50 (SF) %1 [+%2,-%3] %4M ... ").arg(BC50_sf).arg(conf_dSl_sf).arg(conf_dSu_sf).arg(QChar(956));
    //  result += QString("[%1 - %2] %3M</p>").arg(conf_sf.lower).arg(conf_sf.upper).arg(QChar(956));

    return result;
}

QString MonteCarlo2BC50_2_1(const qreal logK21, const qreal logK11, const QJsonObject& object)
{
    QString result;
    QList<qreal> s;

    QStringList models = object["controller"].toObject()["raw"].toObject().keys();
    qreal error = 100 - object["0"].toObject()["confidence"].toObject()["error"].toDouble();

    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
        qreal logK21 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[0];
        qreal logK11 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[1];

        s << BC50::IItoI::BC50(logK21, logK11) * 1e6;
    }

    std::sort(s.begin(), s.end());

    SupraFit::ConfidenceBar conf = ToolSet::Confidence(s, error);
    qreal BC50 = BC50::IItoI::BC50(logK21, logK11) * 1e6;

    qreal conf_dSl = conf.upper - BC50;
    qreal conf_dSu = BC50 - conf.lower;

    result += QString("<p>BC50 %1 [+%2,-%3] %4M ... ").arg(BC50).arg(conf_dSl).arg(conf_dSu).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(conf.lower).arg(conf.upper).arg(QChar(956));

    return result;
}

QString MonteCarlo2BC50_2_2(const qreal logK21, const qreal logK11, const qreal logK12, const QJsonObject& object)
{
    QStringList models = object["controller"].toObject()["raw"].toObject().keys();
    QList<qreal> s, s_sf;
    qreal error = 100 - object["0"].toObject()["confidence"].toObject()["error"].toDouble();

    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
        qreal logK21 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[0];
        qreal logK11 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[1];
        qreal logK12 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[2];

        s << BC50::IItoII::BC50_A0(logK21, logK11, logK12) * 1e6;
        //s_sf << BC50::IItoII::BC50_SF(logK21, logK11, logK12) * 1e6;
    }

    std::sort(s.begin(), s.end());
    std::sort(s_sf.begin(), s_sf.end());

    SupraFit::ConfidenceBar conf = ToolSet::Confidence(s, error);
    SupraFit::ConfidenceBar conf_sf = ToolSet::Confidence(s_sf, error);

    qreal BC50 = BC50::IItoII::BC50_A0(logK21, logK11, logK12) * 1e6;
    //  qreal BC50_sf = BC50::IItoI_ItoI_ItoII_BC50_SF(logK21, logK11, logK12) * 1e6;

    qreal conf_dSl = conf.upper - BC50;
    qreal conf_dSu = BC50 - conf.lower;

    //  qreal conf_dSl_sf = conf_sf.upper - BC50_sf;
    //  qreal conf_dSu_sf = BC50_sf - conf_sf.lower;

    QString result;

    result += QString("<p>BC50 %1 [+%2,-%3] %4M ... ").arg(BC50).arg(conf_dSl).arg(conf_dSu).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(conf.lower).arg(conf.upper).arg(QChar(956));

    //  result += QString("<p>BC50 (SF) %1 [+%2,-%3] %4M ... ").arg(BC50_sf).arg(conf_dSl_sf).arg(conf_dSu_sf).arg(QChar(956));
    //  result += QString("[%1 - %2] %3M</p>").arg(conf_sf.lower).arg(conf_sf.upper).arg(QChar(956));
    return result;
}

QString GridSearch2Thermo(int index, qreal T, const QJsonObject& object, bool heat)
{
    QString result;
    qreal K = 0, H = 0;

    qreal K11u = 0, K11l = 0, dH11l = 0, dH11u = 0;
    qreal K11u_gs = 0, K11l_gs = 0, dH11l_gs = 0, dH11u_gs = 0;
    qreal dGu_gs = 0, dGl_gs = 0, dSu_gs = 0, dSl_gs = 0;

    if (!object.isEmpty()) {
        const QStringList keys = object.keys();
        for (int i = 0; i < keys.size(); ++i) {
            QJsonObject confidence = object[keys[i]].toObject()["confidence"].toObject();
            if(QString::number(index) == keys[i])
            {
                K = object[keys[i]].toObject()["value"].toDouble();

                K11u = confidence["upper"].toDouble();
                K11l = confidence["lower"].toDouble();
                K11u_gs = K;
                K11l_gs = K;
            }

              if (object[keys[i]].toObject()["index"].toString() == QString("0|%1").arg(index)) {
                H = object[keys[i]].toObject()["value"].toDouble();
                dH11u = confidence["upper"].toDouble();
                dH11l = confidence["lower"].toDouble();
                dH11u_gs = H;
                dH11l_gs = H;
            }
        }
    }
    dGu_gs = ToolSet::K2G(K, T);
    dGl_gs = ToolSet::K2G(K, T);
    dSu_gs = ToolSet::GHE(ToolSet::K2G(K, T), H, T);
    dSl_gs = ToolSet::GHE(ToolSet::K2G(K, T), H, T);

    QStringList models = object["controller"].toObject()["raw"].toObject().keys();

    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
        qreal K = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[index];
        K11l_gs = qMin(K11l_gs, K);
        K11u_gs = qMax(K11u_gs, K);
        QVector<qreal> local = ToolSet::String2DoubleVec(model["localParameter"].toObject()["data"].toObject()["0"].toString());
        qreal H = local[index];

        const qreal G = ToolSet::K2G(K, T);
        const qreal S = ToolSet::GHE(G, H, T);

        const qreal absG = qAbs(G);
        const qreal absS = qAbs(S);
        const qreal absH = qAbs(H);
#warning how do two different signs ( for H and dH11 etc ) work, I will take only one for now
        /* Using signless comparison, because H, G and S can be negative and positive */

        int sign = sgn(dH11l_gs);
        dH11l_gs = sign*qMin(qAbs(dH11l_gs), absH);
        dH11u_gs = sign*qMax(qAbs(dH11u_gs), absH);

        sign = sgn(dGl_gs);
        dGl_gs = sign*qMin(qAbs(dGl_gs), absG);
        dGu_gs = sign*qMax(qAbs(dGu_gs), absG);

        sign = sgn(S);
        dSl_gs = sign*qMin(qAbs(dSl_gs), absS);
        dSu_gs = sign*qMax(qAbs(dSu_gs), absS);
    }

    qreal dG = ToolSet::K2G(K, T);
    qreal dS = ToolSet::GHE(dG, H, T);

    qreal dGl = ToolSet::K2G(K11u, T);
    qreal dGu = ToolSet::K2G(K11l, T);

    qreal conf_dGu = dGu - dG;
    qreal conf_dGl = dG - dGl;

    result += "<table>";
    result += QString("<tr><td><b>Complexation Constant K</b></td><td> %1 ").arg(Print::printDouble(qPow(10, K), 3));

    if (!object.isEmpty())
        result += QString(" (+ %1 /- %2)</td>").arg(Print::printDouble(qPow(10, K11u) - qPow(10, K), 3)).arg(Print::printDouble(qPow(10, K) - qPow(10, K11l), 3));
    result += "<td> M</td></tr>";

    if (!object.isEmpty())
        result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>M</td></tr>").arg(Print::printDouble(qPow(10, K11l), 3)).arg(Print::printDouble(qPow(10, K11u), 3));
    if (models.size()) {
        result += QString("<tr><td colspan'2'>Using all data provided by Weakend Grid Search</td></tr>");
        result += QString("<tr><td><b>Complexation Constant K </b></td><td>%1   ").arg(qPow(10, K));
        result += QString(" (+ %1 /- %2 )</td><td> M</td></tr>").arg(Print::printDouble(qPow(10, K11u_gs) - qPow(10, K), 3)).arg(Print::printDouble(qPow(10, K) - qPow(10, K11l_gs), 3));
        result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>M</td></tr>").arg(Print::printDouble(qPow(10, K11l_gs), 3)).arg(Print::printDouble(qPow(10, K11u_gs), 3));
    }
    result += "<tr><td></td></tr>";
    result += "<tr><td></td></tr>";

    result += "<tr><td><b>Free Enthalpy of Complexation &Delta;G </b></td><td>" + Print::printDouble(dG / 1000.0, 3) + "  ";
    if (!object.isEmpty())
        result += " (+" + Print::printDouble(conf_dGu / 1000.0, 3) + "/-" + Print::printDouble(conf_dGl / 1000.0, 3) + ")</td>";
    result += "<td>kJ/mol</td></tr>";
    if (!object.isEmpty())
        result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>kJ/mol</td></tr>").arg(Print::printDouble(dGl / 1000.0, 3)).arg(Print::printDouble(dGu / 1000.0, 3));

    if (models.size()) {
        result += QString("<tr><td colspan'2'>Using all data provided by Weakend Grid Search</td></tr>");
        result += "<tr><td><b>Free Enthalpy of Complexation &Delta;G  </b></td><td>" + Print::printDouble(dG/1000, 3) + "";
        result += " (+" + Print::printDouble((dGu_gs - dG) / 1000.0, 3) + "/-" + Print::printDouble((dG - dGl_gs) / 1000.0, 3) + ")</td>";
        result += "<td> kJ/mol</td></tr>";
        result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>kJ/mol</td></tr>").arg(Print::printDouble(dGu_gs / 1000.0, 3)).arg(Print::printDouble(dGl_gs / 1000.0, 3));
    }
    result += "<tr><td></td></tr>";
    result += "<tr><td></td></tr>";

    if (heat) {

        result += "<tr><td><b>Enthalpy of Complexation &Delta;H</b></td><td>" + Print::printDouble(H / 1000.0, 3) + "  ";

        if (!object.isEmpty())
            result += "(" + Print::printDouble((dH11u - H) / 1000.0, 3) + "/-" + Print::printDouble((H - dH11l) / 1000.0, 3) + ")</td>";
        result += "<td>kJ/mol</td></tr>";

        if (!object.isEmpty())
            result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>kJ/mol</td></tr>").arg( Print::printDouble(dH11l / 1000.0, 3)).arg( Print::printDouble(dH11u / 1000.0, 3));

        if (models.size()) {
            result += QString("<tr><td colspan'2'>Using all data provided by Weakend Grid Search</td></tr>");
            result += "<tr><td><b>Enthalpy of Complexation &Delta;H  </b></td><td>" + Print::printDouble(H / 1000.0, 3) + "";
            result += " (+" + Print::printDouble((dH11u_gs - H) / 1000.0, 3) + "/-" + Print::printDouble((H - dH11l_gs) / 1000.0, 3) + ")</td>";
            result += "<td> kJ/mol</td></tr>";
            result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>kJ/mol</td></tr>").arg(Print::printDouble(dH11u_gs / 1000.0, 3)).arg(Print::printDouble(dH11l_gs / 1000.0, 3));
        }

        result += "<tr><td></td></tr>";
        result += "<tr><td></td></tr>";

        result += "<tr><td><b>Entropy of Complexation &Delta;S</b></td><td>" + Print::printDouble(dS, 3) + "";
        if (!object.isEmpty())
            result += " (+" + Print::printDouble(dSu_gs - dS, 3) + "/-" + Print::printDouble(dS - dSl_gs, 3) + ")</td>";
        result += "<td>J/(molK)</td></tr>";
        if (models.size())
            result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>J/(molK)</td></tr>").arg(Print::printDouble(dSl_gs, 3)).arg(Print::printDouble(dSu_gs, 3));
    }
    result += "</table>";

    return result;
}

QJsonObject PostGridSearch(const QList<QJsonObject>& models, qreal K, qreal T, int index, qreal H)
{
    qreal K11u_gs = 0, K11l_gs = 0, dH11l_gs = 0, dH11u_gs = 0;
    qreal dGu_gs = 0, dGl_gs = 0, dSu_gs = 0, dSl_gs = 0;

    dGu_gs = ToolSet::K2G(K, T);
    dGl_gs = ToolSet::K2G(K, T);
    dSu_gs = ToolSet::GHE(ToolSet::K2G(K, T), H, T);
    dSl_gs = ToolSet::GHE(ToolSet::K2G(K, T), H, T);

    for (const QJsonObject& model : qAsConst(models)) {
        qreal K = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[index];

        K11l_gs = qMin(K11l_gs, K);
        K11u_gs = qMax(K11u_gs, K);
        QVector<qreal> local = ToolSet::String2DoubleVec(model["localParameter"].toObject()["data"].toObject()["0"].toString());
        qreal H = local[index];

        dH11l_gs = qMin(dH11l_gs, H);
        dH11u_gs = qMax(dH11u_gs, H);

        dGl_gs = qMin(dGl_gs, ToolSet::K2G(K, T));
        dGu_gs = qMax(dGu_gs, ToolSet::K2G(K, T));

        dSl_gs = qMin(dSl_gs, ToolSet::GHE(ToolSet::K2G(K, T), H, T));
        dSu_gs = qMax(dSu_gs, ToolSet::GHE(ToolSet::K2G(K, T), H, T));
    }

    qreal dG = ToolSet::K2G(K, T);
    qreal dS = ToolSet::GHE(dG, H, T);

    QJsonObject result;
    /*
    QPair<int, int> index_pair = m_model.data()->IndexParameters(i);
    if (index_pair.second == 0) {
        result["name"] = m_model.data()->GlobalParameterName(index_pair.first);
        result["type"] = "Global Parameter";
    } else if (index_pair.second == 1) {
        result["name"] = m_model.data()->LocalParameterName(index_pair.first);
        result["type"] = "Local Parameter";
        result["index"] = QString::number(0) + "|" + QString::number(index_pair.first);
    }
    result["value"] = parameter[index];

    QJsonObject confidence;
    confidence["upper"] = upper;
    confidence["lower"] = lower;
    confidence["error"] = m_config.confidence;

    result["confidence"] = confidence;
    QJsonObject data;
    data["x"] = ToolSet::DoubleList2String(x);
    data["y"] = ToolSet::DoubleList2String(y);
    result["data"] = data;*/
    return result;
}

QString GridSearch2BC50_1(const qreal logK11, const QJsonObject& object)
{
    QStringList models = object["controller"].toObject()["raw"].toObject().keys();

    qreal BC50 = BC50::ItoI::BC50(logK11) * 1e6;
    qreal BC50l = BC50::ItoI::BC50(logK11) * 1e6;
    qreal BC50u = BC50::ItoI::BC50(logK11) * 1e6;

    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
        qreal logK11 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[0];
        qreal BC50 = BC50::ItoI::BC50(logK11) * 1e6;
        BC50l = qMin(BC50, BC50l);
        BC50u = qMin(BC50, BC50u);
    }

    qreal conf_dSl = BC50u - BC50;
    qreal conf_dSu = BC50 - BC50l;

    QString result;

    result += QString("<p>BC50 %1 [+%2,-%3] %4M ... ").arg(BC50).arg(conf_dSl).arg(conf_dSu).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(BC50l).arg(BC50u).arg(QChar(956));

    return result;
}

QString GridSearch2BC50_1_2(const qreal logK11, const qreal logK12, const QJsonObject& object)
{
    QStringList models = object["controller"].toObject()["raw"].toObject().keys();

    qreal BC50 = BC50::ItoII::BC50(logK11, logK12) * 1e6;
    qreal BC50u = BC50::ItoII::BC50(logK11, logK12) * 1e6;
    qreal BC50l = BC50::ItoII::BC50(logK11, logK12) * 1e6;

    /*
    qreal BC50_sf = BC50::ItoII::BC50_SF(logK11, logK12) * 1e6;
    qreal BC50_sf_u = BC50::ItoII::BC50_SF(logK11, logK12) * 1e6;
    qreal BC50_sf_l = BC50::ItoII::BC50_SF(logK11, logK12) * 1e6;
*/
    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
        qreal logK11 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[0];
        qreal logK12 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[1];

        qreal BC50 = BC50::ItoII::BC50(logK11, logK12) * 1e6;
        //qreal BC50_sf = BC50::ItoI_ItoII_BC50_SF(logK11, logK12) * 1e6;

        BC50l = qMin(BC50, BC50l);
        BC50u = qMin(BC50, BC50u);

        //BC50_sf_l = qMin(BC50_sf, BC50_sf_l);
        //BC50_sf_u = qMin(BC50_sf, BC50_sf_u);
    }

    qreal conf_dSl = BC50u - BC50;
    qreal conf_dSu = BC50 - BC50l;

    //qreal conf_dSl_sf = BC50_sf_u - BC50_sf;
    //qreal conf_dSu_sf = BC50_sf - BC50_sf_l;

    QString result;

    result += QString("<p>BC50 %1 [+%2,-%3] %4M ... ").arg(BC50).arg(conf_dSl).arg(conf_dSu).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(BC50l).arg(BC50u).arg(QChar(956));

    //result += QString("<p>BC50 (SF) %1 [+%2,-%3] %4M ... ").arg(BC50_sf).arg(conf_dSl_sf).arg(conf_dSu_sf).arg(QChar(956));
    //result += QString("[%1 - %2] %3M</p>").arg(BC50_sf_l).arg(BC50_sf_u).arg(QChar(956));

    return result;
}

QString GridSearch2BC50_2_1(const qreal logK21, const qreal logK11, const QJsonObject& object)
{
    QStringList models = object["controller"].toObject()["raw"].toObject().keys();

    QString result;

    qreal BC50 = BC50::IItoI::BC50(logK21, logK11) * 1e6;
    qreal BC50u = BC50::IItoI::BC50(logK21, logK11) * 1e6;
    qreal BC50l = BC50::IItoI::BC50(logK21, logK11) * 1e6;

    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
        qreal logK21 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[0];
        qreal logK11 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[1];

        qreal BC50 = BC50::IItoI::BC50(logK21, logK11) * 1e6;

        BC50l = qMin(BC50, BC50l);
        BC50u = qMin(BC50, BC50u);
    }

    qreal conf_dSl = BC50u - BC50;
    qreal conf_dSu = BC50 - BC50l;

    result += QString("<p>BC50 %1 [+%2,-%3] %4M ... ").arg(BC50).arg(conf_dSl).arg(conf_dSu).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(BC50l).arg(BC50u).arg(QChar(956));

    return result;
}

QString GridSearch2BC50_2_2(const qreal logK21, const qreal logK11, const qreal logK12, const QJsonObject& object)
{
    QStringList models = object["controller"].toObject()["raw"].toObject().keys();
    QList<qreal> s, s_sf;

    qreal BC50 = BC50::IItoII::BC50_A0(logK21, logK11, logK12) * 1e6;
    qreal BC50u = BC50::IItoII::BC50_A0(logK21, logK11, logK12) * 1e6;
    qreal BC50l = BC50::IItoII::BC50_A0(logK21, logK11, logK12) * 1e6;
    /*
    qreal BC50_sf = BC50::IItoI_ItoI_ItoII_BC50_SF(logK21, logK11, logK12) * 1e6;
    qreal BC50_sf_u = BC50::IItoI_ItoI_ItoII_BC50_SF(logK21, logK11, logK12) * 1e6;
    qreal BC50_sf_l = BC50::IItoI_ItoI_ItoII_BC50_SF(logK21, logK11, logK12) * 1e6;

  */
    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();
        qreal logK21 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[0];
        qreal logK11 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[1];
        qreal logK12 = ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString())[2];

        qreal BC50 = BC50::IItoII::BC50_A0(logK21, logK11, logK12) * 1e6;
        //qreal BC50_sf = BC50::IItoII_BC50_SF(logK21, logK11, logK12) * 1e6;

        BC50l = qMin(BC50, BC50l);
        BC50u = qMin(BC50, BC50u);

        //BC50_sf_l = qMin(BC50_sf, BC50_sf_l);
        //BC50_sf_u = qMin(BC50_sf, BC50_sf_u);
    }

    qreal conf_dSl = BC50u - BC50;
    qreal conf_dSu = BC50 - BC50l;

    //qreal conf_dSl_sf = BC50_sf_u - BC50_sf;
    //qreal conf_dSu_sf = BC50_sf - BC50_sf_l;

    QString result;

    result += QString("<p>BC50 %1 [+%2,-%3] %4M ... ").arg(BC50).arg(conf_dSl).arg(conf_dSu).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(BC50l).arg(BC50u).arg(QChar(956));

    //result += QString("<p>BC50 (SF) %1 [+%2,-%3] %4M ... ").arg(BC50_sf).arg(conf_dSl_sf).arg(conf_dSu_sf).arg(QChar(956));
    //result += QString("[%1 - %2] %3M</p>").arg(BC50_sf_l).arg(BC50_sf_u).arg(QChar(956));

    return result;
}

QString PseudoANOVA(const QPointer<const AbstractModel>& model)
{
    QString result;

    for (int i = 0; i < model->SeriesCount(); ++i) {
        QVector<QVector<qreal>> values;
        QVector<qreal> mean;
        QVector<qreal> std;
        for (int j = 0; j < model->DataPoints(); ++j) {
            QVector<qreal> vector = QVector<qreal>() << model->ModelTable()->data(i, j) << model->DeCompose(j, i);
            if (!mean.size())
                mean = QVector<qreal>(vector.size(), 0);
            values << vector;
            for (int k = 0; k < vector.size(); ++k)
                mean[k] += vector[k];
        }

        for (int k = 0; k < mean.size(); ++k)
            mean[k] /= double(model->DataPoints());
        std = QVector<qreal>(mean.size(), 0);
        for (int j = 0; j < model->DataPoints(); ++j) {
            for (int k = 0; k < mean.size(); ++k) {
                values[j][k] -= mean[k];
                std[k] += qPow(values[j][k], 2);
            }
        }
        for (int k = 0; k < mean.size(); ++k)
            std[k] = qSqrt(std[k] / (double(model->DataPoints() - 1)));
        //qDebug() << mean;
        //qDebug() << std;
        result += QString("Series %1: Contribution:").arg(i);

        for (int k = 1; k < std.size(); ++k)
            result += QString("\t%1").arg(std[k] / model->SEy(i));
        result += "\n";
    }

    return result;
}
}

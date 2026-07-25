/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/toolset.h"

#include "src/core/models/models.h"

#include "src/global.h"

#include <QtCore/QJsonObject>

#include "src/core/units.h"

#include "statistic.h"

namespace Statistic {

/*! \brief Extract the fitted global-parameter vector of every raw Monte-Carlo model in one pass.
 *
 * The BC50/thermodynamics post-processing needs one parameter set per resampled model. Resolving
 * object["controller"]["raw"] inside the per-model loop (as the code did before) re-converts the
 * complete raw block - potentially thousands of full model exports - on every iteration, which
 * turned the confidence text of a 2000-step Monte-Carlo run into seconds of GUI freeze.
 * Claude Generated (2026, MC results-widget performance fix).
 */
static QVector<QVector<qreal>> RawGlobalParameters(const QJsonObject& object)
{
    const QJsonObject raw = object["controller"].toObject()["raw"].toObject();
    const QStringList keys = raw.keys();

    QVector<QVector<qreal>> parameters;
    parameters.reserve(keys.size());
    for (const QString& key : keys) {
        QJsonObject model = raw[key].toObject()["data"].toObject();
        if (model.isEmpty())
            model = raw[key].toObject();
        parameters << ToolSet::String2DoubleVec(model["globalParameter"].toObject()["data"].toObject()["0"].toString());
    }
    return parameters;
}

QString MonteCarlo2Thermo(int index, qreal T, const QJsonObject& object, bool heat)
{
    QString result;
    qreal K = 0, H = 0;
    qreal K11u = 0, K11l = 0, dH11l = 0, dH11u = 0;

    qreal error = 0;
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

    // Claude Generated: unit-aware formatting (SI kJ/mol, J·mol⁻¹·K⁻¹ or calorie-based) so ITC
    // results can be compared with e.g. NanoAnalyze; see Thermo::CurrentEnergyUnit().
    const Units::EnergyUnit unit = Units::currentEnergy();
    auto fmtE = [&](qreal joule, int prec = 3) { return Print::printDouble(joule / unit.energyDivisor, prec); };
    auto fmtS = [&](qreal jPerK, int prec = 3) { return Print::printDouble(jPerK / unit.entropyDivisor, prec); };

    result += "<table>";
    result += "<tr><td><b>Complexation Constant K </b></td><td>" + Print::printDouble(qPow(10, K), 3) + "";

    if (!object.isEmpty())
        result += " (+" + Print::printDouble(qPow(10, K11u) - qPow(10, K), 3) + "/-" + Print::printDouble(qPow(10, K) - qPow(10, K11l), 3) + ")</td>";
    result += "<td> M</td></tr>";

    if (!object.isEmpty())
        result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>M</td></tr>").arg(Print::printDouble(qPow(10, K11l), 3)).arg(Print::printDouble(qPow(10, K11u), 3));

    result += "<tr><td></td></tr>";
    result += "<tr><td></td></tr>";

    result += "<tr><td><b>Free Enthalpy of Complexation &Delta;G </b></td><td>" + fmtE(dG) + "";
    if (!object.isEmpty())
        result += " (+" + fmtE(conf_dGu) + "/-" + fmtE(conf_dGl) + ")";
    result += "</td><td>" + unit.energyLabel + "</td></tr>";
    if (!object.isEmpty())
        result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>%3</td></tr>").arg(fmtE(dGl)).arg(fmtE(dGu)).arg(unit.energyLabel);

    result += "<tr><td></td></tr>";
    result += "<tr><td></td></tr>";

    if (heat) {

        const QJsonObject raw = object["controller"].toObject()["raw"].toObject();
        const QStringList models = raw.keys();
        QList<qreal> s;

        for (int i = 0; i < models.size(); ++i) {

            QJsonObject model = raw[models[i]].toObject()["data"].toObject();
            if (model.isEmpty())
                model = raw[models[i]].toObject();

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

        result += "<tr><td><b>Enthalpy of Complexation &Delta;H</b></td><td>" + fmtE(H) + "";

        if (!object.isEmpty())
            result += "(" + fmtE(conf_dH11u) + "/-" + fmtE(conf_dH11l) + ")";
        result += "</td><td>" + unit.energyLabel + "</td></tr>";

        if (!object.isEmpty())
            result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>%3</td></tr>").arg(fmtE(dH11l)).arg(fmtE(dH11u)).arg(unit.energyLabel);

        result += "<tr><td></td></tr>";
        result += "<tr><td></td></tr>";

        result += "<tr><td><b>Entropy of Complexation &Delta;S</b></td><td>" + fmtS(dS) + "";
        if (!object.isEmpty())
            result += "(+" + fmtS(conf_dSu) + "/-" + fmtS(conf_dSl) + ")";
        result += "</td><td>" + unit.entropyLabel + "</td></tr>";
        if (!object.isEmpty())
            result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>%3</td></tr>").arg(fmtS(conf.lower)).arg(fmtS(conf.upper)).arg(unit.entropyLabel);

        // -TΔS (entropy term of ΔG = ΔH − TΔS) in energy units, matching the NanoAnalyze
        // convention. Range mirrors ΔS scaled by −T (bounds swap because of the sign).
        result += "<tr><td><b>Entropy Term -T&Delta;S</b></td><td>" + fmtE(-T * dS) + "</td><td>" + unit.energyLabel + "</td></tr>";
        if (!object.isEmpty())
            result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>%3</td></tr>").arg(fmtE(-T * conf.upper)).arg(fmtE(-T * conf.lower)).arg(unit.energyLabel);
    }
    result += "</table>";

    return result;
}

QString MonteCarlo2BC50_Speciation(const Eigen::MatrixXi& stoich, const QVector<qreal>& lgBeta, const QJsonObject& object)
{
    const qreal nominal = BC50::FromSpeciation(stoich, lgBeta);
    if (nominal < 0)
        return QString(); // no BC50 defined for this reaction system

    const qreal error = 100 - object["0"].toObject()["confidence"].toObject()["error"].toDouble();

    QList<qreal> s;
    for (const QVector<qreal>& global : RawGlobalParameters(object)) {
        const qreal value = BC50::FromSpeciation(stoich, global);
        if (value > 0)
            s << value * 1e6;
    }
    if (s.isEmpty())
        return QString();

    std::sort(s.begin(), s.end());
    const SupraFit::ConfidenceBar conf = ToolSet::Confidence(s, error);
    const qreal BC50 = nominal * 1e6;

    QString result;
    result += QString("<p>BC50 %1 [+%2,-%3] %4M ... ").arg(BC50).arg(conf.upper - BC50).arg(BC50 - conf.lower).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(conf.lower).arg(conf.upper).arg(QChar(956));
    return result;
}

QString GridSearch2BC50_Speciation(const Eigen::MatrixXi& stoich, const QVector<qreal>& lgBeta, const QJsonObject& object)
{
    const qreal nominal = BC50::FromSpeciation(stoich, lgBeta);
    if (nominal < 0)
        return QString();

    const qreal BC50 = nominal * 1e6;
    qreal lower = BC50, upper = BC50;
    for (const QVector<qreal>& global : RawGlobalParameters(object)) {
        const qreal value = BC50::FromSpeciation(stoich, global);
        if (value <= 0)
            continue;
        lower = qMin(value * 1e6, lower);
        upper = qMax(value * 1e6, upper);
    }

    QString result;
    result += QString("<p>BC50 %1 [+%2,-%3] %4M ... ").arg(BC50).arg(upper - BC50).arg(BC50 - lower).arg(QChar(956));
    result += QString("[%1 - %2] %3M</p>").arg(lower).arg(upper).arg(QChar(956));
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
        QJsonObject model;

        model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject()["data"].toObject();
        if (model.isEmpty())
            model = object["controller"].toObject()["raw"].toObject()[models[i]].toObject();

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
#pragma message("how do two different signs ( for H and dH11 etc ) work, I will take only one for now")
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

    // Claude Generated: unit-aware formatting (see MonteCarlo2Thermo / Thermo::CurrentEnergyUnit()).
    const Units::EnergyUnit unit = Units::currentEnergy();
    auto fmtE = [&](qreal joule, int prec = 3) { return Print::printDouble(joule / unit.energyDivisor, prec); };
    auto fmtS = [&](qreal jPerK, int prec = 3) { return Print::printDouble(jPerK / unit.entropyDivisor, prec); };

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

    result += "<tr><td><b>Free Enthalpy of Complexation &Delta;G </b></td><td>" + fmtE(dG) + "  ";
    if (!object.isEmpty())
        result += " (+" + fmtE(conf_dGu) + "/-" + fmtE(conf_dGl) + ")</td>";
    result += "<td>" + unit.energyLabel + "</td></tr>";
    if (!object.isEmpty())
        result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>%3</td></tr>").arg(fmtE(dGl)).arg(fmtE(dGu)).arg(unit.energyLabel);

    if (models.size()) {
        result += QString("<tr><td colspan'2'>Using all data provided by Weakend Grid Search</td></tr>");
        result += "<tr><td><b>Free Enthalpy of Complexation &Delta;G  </b></td><td>" + fmtE(dG) + "";
        result += " (+" + fmtE(dGu_gs - dG) + "/-" + fmtE(dG - dGl_gs) + ")</td>";
        result += "<td> " + unit.energyLabel + "</td></tr>";
        result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>%3</td></tr>").arg(fmtE(dGu_gs)).arg(fmtE(dGl_gs)).arg(unit.energyLabel);
    }
    result += "<tr><td></td></tr>";
    result += "<tr><td></td></tr>";

    if (heat) {

        result += "<tr><td><b>Enthalpy of Complexation &Delta;H</b></td><td>" + fmtE(H) + "  ";

        if (!object.isEmpty())
            result += "(" + fmtE(dH11u - H) + "/-" + fmtE(H - dH11l) + ")</td>";
        result += "<td>" + unit.energyLabel + "</td></tr>";

        if (!object.isEmpty())
            result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>%3</td></tr>").arg(fmtE(dH11l)).arg(fmtE(dH11u)).arg(unit.energyLabel);

        if (models.size()) {
            result += QString("<tr><td colspan'2'>Using all data provided by Weakend Grid Search</td></tr>");
            result += "<tr><td><b>Enthalpy of Complexation &Delta;H  </b></td><td>" + fmtE(H) + "";
            result += " (+" + fmtE(dH11u_gs - H) + "/-" + fmtE(H - dH11l_gs) + ")</td>";
            result += "<td> " + unit.energyLabel + "</td></tr>";
            result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>%3</td></tr>").arg(fmtE(dH11u_gs)).arg(fmtE(dH11l_gs)).arg(unit.energyLabel);
        }

        result += "<tr><td></td></tr>";
        result += "<tr><td></td></tr>";

        result += "<tr><td><b>Entropy of Complexation &Delta;S</b></td><td>" + fmtS(dS) + "";
        if (!object.isEmpty())
            result += " (+" + fmtS(dSu_gs - dS) + "/-" + fmtS(dS - dSl_gs) + ")</td>";
        result += "<td>" + unit.entropyLabel + "</td></tr>";
        if (models.size())
            result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>%3</td></tr>").arg(fmtS(dSl_gs)).arg(fmtS(dSu_gs)).arg(unit.entropyLabel);

        // -TΔS (entropy term of ΔG = ΔH − TΔS) in energy units, matching the NanoAnalyze convention.
        result += "<tr><td><b>Entropy Term -T&Delta;S</b></td><td>" + fmtE(-T * dS) + "</td><td>" + unit.energyLabel + "</td></tr>";
        if (models.size())
            result += QString("<tr><td><b></b></td><td>[%1 - %2]  </td><td>%3</td></tr>").arg(fmtE(-T * dSu_gs)).arg(fmtE(-T * dSl_gs)).arg(unit.energyLabel);
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
    // qreal dS = ToolSet::GHE(dG, H, T);

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

QString PseudoANOVA(const QPointer<const AbstractModel>& model)
{
#pragma message("never used but might be broken")
    QString result;

    for (int j = 0; j < model->SeriesCount(); ++j) {
        QVector<QVector<qreal>> values;
        QVector<qreal> mean;
        QVector<qreal> std;
        for (int i = 0; i < model->DataPoints(); ++i) {
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
        for (int i = 0; i < model->DataPoints(); ++i) {
            for (int k = 0; k < mean.size(); ++k) {
                values[i][k] -= mean[k];
                std[k] += qPow(values[i][k], 2);
            }
        }
        for (int k = 0; k < mean.size(); ++k)
            std[k] = qSqrt(std[k] / (double(model->DataPoints() - 1)));
        //qDebug() << mean;
        //qDebug() << std;
        result += QString("Series %1: Contribution:").arg(j);

        for (int k = 1; k < std.size(); ++k)
            result += QString("\t%1").arg(std[k] / model->SEy(j));
        result += "\n";
    }

    return result;
}
}

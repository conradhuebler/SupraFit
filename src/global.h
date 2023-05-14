/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2021 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#pragma once

#include "src/global_config.h"
#include "src/version.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QVariant>
#include <QtCore/QVector>
#include <QtCore/QtGlobal>
#include <QtCore/QtMath>

#include <cmath>
#include <iostream>

const QString Unicode_Phi = QChar(0x03C6);
const QString Unicode_beta = QChar(0x03B2);
const QString Unicode_delta = QChar(0x03B4);
const QString Unicode_mu = QChar(0x00B5);
const QString Unicode_sigma = QChar(0x03C3);
const QString Unicode_chi = QChar(0x1D61);
const QString Unicode_epsilion = QChar(0x03B5);
const QString Unicode_theta = QChar(0x03B8);

const QString Unicode_Math_Element = QChar(0x2208);

const QString Unicode_Sup_1 = QChar(0x00B9);
const QString Unicode_Sup_2 = QChar(0x00B2);

const QString Unicode_Sub_0 = QChar(0x2080);
const QString Unicode_Sub_1 = QChar(0x2081);
const QString Unicode_Sub_2 = QChar(0x2082);
const QString Unicode_Sub_3 = QChar(0x2083);
const QString Unicode_Sub_4 = QChar(0x2084);
const QString Unicode_Sub_5 = QChar(0x2085);
const QString Unicode_Sub_6 = QChar(0x2086);
const QString Unicode_Sub_7 = QChar(0x2087);
const QString Unicode_Sub_8 = QChar(0x2088);
const QString Unicode_Sub_9 = QChar(0x2089);

const QString Unicode_Integral = QChar(0x222B);

const QString HashTag = QChar(0x0023);
const QString QMarks = QChar(0x0022);

const QString Unicode_Sub_y = QChar(0x0079); // not really

// const QString Unicode_Sub_AB = QString("%1%2").arg(QChar("A")).arg(QChar("B")); //does not exist yet

struct ParameterBoundary {
    bool limit_lower = false;
    bool limit_upper = false;

    double lower_barrier = 0;
    double upper_barrier = 0;

    double lower_barrier_beta = 100;
    double upper_barrier_beta = 100;

    double lower_barrier_wall = 100;
    double upper_barrier_wall = 100;
};

inline QVector<double> Boundary2Vector(const ParameterBoundary& boundary)
{
    QVector<double> vector(8);
    vector[0] = boundary.limit_lower;
    vector[1] = boundary.limit_upper;
    vector[2] = boundary.lower_barrier;
    vector[3] = boundary.upper_barrier;

    vector[4] = boundary.lower_barrier_beta;
    vector[5] = boundary.upper_barrier_beta;
    vector[6] = boundary.lower_barrier_wall;
    vector[7] = boundary.upper_barrier_wall;

    return vector;
}

inline ParameterBoundary Vector2Boundary(const QVector<double>& vector)
{
    ParameterBoundary boundary;
    if (vector.size() < 8)
        return boundary;
    boundary.limit_lower = vector[0] > 0.5;
    boundary.limit_upper = vector[1] > 0.5;
    boundary.lower_barrier = vector[2];
    boundary.upper_barrier = vector[3];

    boundary.lower_barrier_beta = vector[4];
    boundary.upper_barrier_beta = vector[5];
    boundary.lower_barrier_wall = vector[6];
    boundary.upper_barrier_wall = vector[7];
    return boundary;
}

namespace SupraFit {

struct timer {
    inline timer() { t0 = QDateTime::currentMSecsSinceEpoch(); }
    inline ~timer()
    {
        QTime time(0, 0, 0);
        qint64 t1 = QDateTime::currentMSecsSinceEpoch();

        std::cout << "SupraFit finished after " << time.addMSecs(t1 - t0).toString("hh").toStdString() << " hours "
                  << time.addMSecs(t1 - t0).toString("mm").toStdString() << " minutes "
                  << time.addMSecs(t1 - t0).toString("ss").toStdString() << " seconds "
                  << time.addMSecs(t1 - t0).toString("zzz").toStdString() << " milliseconds!"
                  << std::endl;
    }
    qint64 t0;
};

enum Method {
    MonteCarlo = 1,
    WeakenedGridSearch = 2,
    ModelComparison = 3,
    CrossValidation = 4,
    Reduction = 5,
    FastConfidence = 6,
    GlobalSearch = 7
};

enum Model {
    Data = 0,
    nmr_ItoI = 1,
    nmr_IItoI_ItoI = 2,
    nmr_ItoI_ItoII = 3,
    nmr_IItoI_ItoI_ItoII = 4,
    Michaelis_Menten = 5,
    MonoMolecularModel = 6,
    Arrhenius = 7,
    Eyring = 8,
    itc_ItoI = 10,
    itc_IItoI = 11,
    itc_ItoII = 12,
    itc_IItoII = 13,
    itc_n_ItoI = 14,
    itc_n_ItoII = 15,
    itc_blank = 16,
    itc_any = 17,
    fl_ItoI = 20,
    fl_IItoI_ItoI = 21,
    fl_ItoI_ItoII = 22,
    fl_IItoI_ItoI_ItoII = 23,
    uv_vis_ItoI = 30,
    uv_vis_IItoI_ItoI = 31,
    uv_vis_ItoI_ItoII = 32,
    uv_vis_IItoI_ItoI_ItoII = 33,
    nmr_any = 34,
    uvvis_any = 35,
    ScriptModel = 100,
    Indep_Quadrat = 101,
    Dep_Any = 102,
    DecayRates = 103,
    BiMolecularModel = 107,
    FlexMolecularModel = 108,
    TianModel = 109,
    EvapMModel = 110,
    BETModel = 111,
    MetaModel = 200,
    Unknown = 404
};
struct ConfidenceBar {
    qreal lower = 0;
    qreal upper = 0;
};

inline QString Method2Name(SupraFit::Method type)
{
    switch (type) {
    case SupraFit::Method::WeakenedGridSearch:
        return ("Weakend Grid Search");
        break;

    case SupraFit::Method::ModelComparison:
        return ("Model Comparison");
        break;

    case SupraFit::Method::FastConfidence:
        return ("Simplified Model Comparison");
        break;

    case SupraFit::Method::Reduction:
        return ("Reduction Analysis");
        break;

    case SupraFit::Method::MonteCarlo:
        return ("Monte Carlo Simulation");
        break;
    case SupraFit::Method::CrossValidation:
        return ("Cross Validation");
        break;

    case SupraFit::Method::GlobalSearch:
        return ("Global Search");
        break;
    }
    return QString();
}

inline QString Method2Name(int type)
{
    return Method2Name(SupraFit::Method(type));
}
}

class QString;

extern QString collective_messages;

inline void UpdateRecentListProperty(const QString &file)
{
    QStringList recent = qApp->instance()->property("recent").toStringList();
    recent.removeOne(file);
    recent.prepend(file);
    qApp->instance()->setProperty("recent", recent);
}
QString getDir();
void setLastDir(const QString& str);

inline void Version(QCoreApplication* app, QCommandLineParser* parser)
{
    app->setApplicationName("SupraFit");
    app->setOrganizationName("Conrad Huebler");

    app->setApplicationVersion(version);

    parser->setApplicationDescription("A Open Source Qt6 based fitting tool for supramolecular titration experiments.");
    parser->addHelpOption();
    parser->addVersionOption();
}

inline void myMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QByteArray localMsg = msg.toLocal8Bit();

    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        if (context.line != 0)
            fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        else if (!localMsg.contains("QFont"))
            fprintf(stderr, "Warning: %s \n", localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
}

inline SupraFit::Model Name2Model(const QString& str)
{
    if (str == "1:1-Model")
        return SupraFit::nmr_ItoI;
    else if (str == "2:1/1:1-Model")
        return SupraFit::nmr_IItoI_ItoI;
    else if (str == "1:1/1:2-Model")
        return SupraFit::nmr_ItoI_ItoII;
    else if (str == "2:1/1:1/1:2-Model")
        return SupraFit::nmr_IItoI_ItoI_ItoII;

    else if (str == "nmr_1:1-Model")
        return SupraFit::nmr_ItoI;
    else if (str == "nmr_2:1/1:1-Model")
        return SupraFit::nmr_IItoI_ItoI;
    else if (str == "nmr_1:1/1:2-Model")
        return SupraFit::nmr_ItoI_ItoII;
    else if (str == "nmr_2:1/1:1/1:2-Model")
        return SupraFit::nmr_IItoI_ItoI_ItoII;

    else if (str == "itc_1:1-Model")
        return SupraFit::itc_ItoI;
    else if (str == "itc_2:1/1:1-Model")
        return SupraFit::itc_IItoI;
    else if (str == "itc_1:1/1:2-Model")
        return SupraFit::itc_ItoII;
    else if (str == "itc_2:1/1:1/1:2-Model")
        return SupraFit::itc_IItoII;
    else if (str == "Independent Multiple Site")
        return SupraFit::itc_n_ItoI;
    else if (str == "Two Set Multiple Site")
        return SupraFit::itc_n_ItoII;
    else if (str == "Blank Titration")
        return SupraFit::itc_blank;

    else if (str == "fl_1:1-Model")
        return SupraFit::fl_ItoI;
    else if (str == "fl_2:1/1:1-Model")
        return SupraFit::fl_IItoI_ItoI;
    else if (str == "fl_1:1/1:2-Model")
        return SupraFit::fl_ItoI_ItoII;
    else if (str == "fl_2:1/1:1/1:2-Model")
        return SupraFit::fl_IItoI_ItoI_ItoII;

    else if (str == "uv_vis_1:1-Model")
        return SupraFit::uv_vis_ItoI;
    else if (str == "uv_vis_2:1/1:1-Model")
        return SupraFit::uv_vis_IItoI_ItoI;
    else if (str == "uv_vis_1:1/1:2-Model")
        return SupraFit::uv_vis_ItoI_ItoII;
    else if (str == "uv_vis_2:1/1:1/1:2-Model")

        return SupraFit::uv_vis_IItoI_ItoI_ItoII;
    else if (str == "Monomolecular Kinetics")
        return SupraFit::MonoMolecularModel;
    else if (str == "Michaelis Menten")
        return SupraFit::Michaelis_Menten;
    else if (str == "ScriptModel")
        return SupraFit::ScriptModel;
    else if (str == "Indep. Quadrat")
        return SupraFit::Indep_Quadrat;
    else if (str == "Dep. AnyModel")
        return SupraFit::Dep_Any;
    else if (str == "Meta Model")
        return SupraFit::MetaModel;
    else
        return SupraFit::Unknown;
}

inline QString Model2Name(SupraFit::Model model)
{
    if (model == SupraFit::nmr_ItoI)
        return QString("%1H %2").arg(Unicode_Sup_1).arg("1:1-Model");
    else if (model == SupraFit::nmr_IItoI_ItoI)
        return QString("%1H %2").arg(Unicode_Sup_1).arg("2:1/1:1-Model");
    else if (model == SupraFit::nmr_ItoI_ItoII)
        return QString("%1H %2").arg(Unicode_Sup_1).arg("1:1/1:2-Model");
    else if (model == SupraFit::nmr_IItoI_ItoI_ItoII)
        return QString("%1H %2").arg(Unicode_Sup_1).arg("2:1/1:1/1:2-Model");

    else if (model == SupraFit::nmr_any)
        return QString("%1H %2").arg(Unicode_Sup_1).arg("flexible NMR-Model");

    else if (model == SupraFit::itc_ItoI)
        return "ITC 1:1-Model";
    else if (model == SupraFit::itc_IItoI)
        return "ITC 2:1/1:1-Model";
    else if (model == SupraFit::itc_ItoII)
        return "ITC 1:1/1:2-Model";
    else if (model == SupraFit::itc_IItoII)
        return "ITC 2:1/1:1/1:2-Model";
    else if (model == SupraFit::itc_n_ItoI)
        return "Independent Multiple Site";
    else if (model == SupraFit::itc_n_ItoII)
        return "Two Set Multiple Site";
    else if (model == SupraFit::itc_blank)
        return "Blank Titration";
    else if (model == SupraFit::itc_any)
        return "flexible ITC Model";

    else if (model == SupraFit::fl_ItoI)
        return QString("%1 %2").arg(Unicode_Phi).arg("1:1-Model");
    else if (model == SupraFit::fl_IItoI_ItoI)
        return QString("%1 %2").arg(Unicode_Phi).arg("2:1/1:1-Model");
    else if (model == SupraFit::fl_ItoI_ItoII)
        return QString("%1 %2").arg(Unicode_Phi).arg("1:1/1:2-Model");
    else if (model == SupraFit::fl_IItoI_ItoI_ItoII)
        return QString("%1 %2").arg(Unicode_Phi).arg("2:1/1:1/1:2-Model");

    else if (model == SupraFit::uv_vis_ItoI)
        return QString("UV/VIS %1").arg("1:1-Model");
    else if (model == SupraFit::uv_vis_IItoI_ItoI)
        return QString("UV/VIS %1").arg("2:1/1:1-Model");
    else if (model == SupraFit::uv_vis_ItoI_ItoII)
        return QString("UV/VIS %1").arg("1:1/1:2-Model");
    else if (model == SupraFit::uv_vis_IItoI_ItoI_ItoII)
        return QString("UV/VIS %1").arg("2:1/1:1/1:2-Model");
    else if (model == SupraFit::uvvis_any)
        return "flexible UV/VIS Model";

    else if (model == SupraFit::MonoMolecularModel)
        return "Monomoleculare Kinetics";
    else if (model == SupraFit::BiMolecularModel)
        return "Bimoleculare Kinetics";
    else if (model == SupraFit::FlexMolecularModel)
        return "Flexible kinetic model";

    else if (model == SupraFit::Michaelis_Menten)
        return "Michaelis Menten Kinetics";
    else if (model == SupraFit::ScriptModel)
        return "ScriptModel";
    else if (model == SupraFit::Indep_Quadrat)
        return "Indep. Quadrat";
    else if (model == SupraFit::Dep_Any)
        return "Dep. AnyModel";
    else if (model == SupraFit::MetaModel)
        return "Meta Model";
    else if (model == SupraFit::DecayRates)
        return "Decay Rates";
    else if (model == SupraFit::Arrhenius)
        return "Arrhenius Plot";
    else if (model == SupraFit::Eyring)
        return "Eyring Plot";
    else if (model == SupraFit::TianModel)
        return "TIAN Thermokinetic";
    else if (model == SupraFit::EvapMModel)
        return "Monomolecular Kinetics with Evaporation";
    else if (model == SupraFit::BETModel)
        return "BET Absorption isotherm";
    else
        return "Unknown";
}

inline bool FuzzyCompare(qreal a, qreal b, int prec = 3)
{
    return qAbs(a - b) < qPow(10, -prec);
}

inline QJsonValue AccessCI(const QJsonObject& object, const QString& str)
{
    QStringList keys = object.keys();
    for (const QString& key : qAsConst(keys)) {
        if (key.compare(str, Qt::CaseInsensitive) == 0)
            return object[key];
    }
    return QJsonValue();
}

inline int sgn(qreal val) {
    /* Nice solution taken from here
     * https://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c - but not as template, only for double
     */
    return (0 < val) - (val < 0);
}

/*
inline QString Bool2YesNo(bool value)
{
    return (value ? QString("Yes") : QString("No"));
}*/

inline const QString included() { return QString("#77d740;"); }
inline const QString excluded() { return QString("#e17f7f;"); }

const QJsonObject OptimConfigBlock{
    /* This are the specific definitions, that work around Levenberg-Marquardt */
    { "MaxLevMarInter", 75 },
    { "ErrorConvergence", 5E-7 },
    { "DeltaParameter", 1E-4 },

    /* This definitions control Levenberg-Marquardt routine itself */
    { "LevMar_Factor", 100 },
    { "LevMar_MaxFEv", 100 },
    { "LevMar_Xtol", 1E-10 },
    { "LevMar_Gtol", 1E-10 },
    { "LevMar_Ftol", 1E-10 },
    { "LevMar_epsfcn", 1E-8 },

    /* This settings apply to numeric concentration calculation */
    { "Skip_not_Converged_Concentrations", false },
    { "MaxIterConcentrations", 1500 },
    { "ConcentrationConvergence", 1e-13 }
};

/*
 * Define the names/strings of the
 * used parameter
 */

const QString K11 = QString("lg K%1%1").arg(Unicode_Sub_1);
const QString K21 = QString("lg K%1%2").arg(Unicode_Sub_2).arg(Unicode_Sub_1);
const QString K12 = QString("lg K%1%2").arg(Unicode_Sub_1).arg(Unicode_Sub_2);

const QString K1 = QString("lg K%1").arg(Unicode_Sub_1);
const QString K2 = QString("lg K%1").arg(Unicode_Sub_2);

const QString dHAB = QString("%1H<sub>AB</sub>").arg(Unicode_delta);
const QString dHAB2 = QString("%1H<sub>AB2</sub>").arg(Unicode_delta);
const QString dHA2B = QString("%1H<sub>A2B</sub>").arg(Unicode_delta);

const QString dHAB_ = QString("%1H'<sub>AB</sub>").arg(Unicode_delta);
const QString dHAB2_ = QString("%1H'<sub>AB2</sub>").arg(Unicode_delta);
const QString dHA2B_ = QString("%1H'<sub>A2B</sub>").arg(Unicode_delta);

const QString dH1 = QString("%1H<sub>1</sub>").arg(Unicode_delta);
const QString dH2 = QString("%1H<sub>2</sub>").arg(Unicode_delta);

const QString n1 = QString("n%1").arg(Unicode_Sub_1);
const QString n2 = QString("n%2").arg(Unicode_Sub_2);

const QString msolv = QString("m (%1<sub>solv</sub>)").arg(Unicode_delta);
const QString nsolv = QString("n (%1<sub>solv</sub>)").arg(Unicode_delta);

const QString fx = QString("fx");

const QString AB = QString("AB");
const QString AB2 = QString("AB%1").arg(Unicode_Sub_2);
const QString A2B = QString("A%1B").arg(Unicode_Sub_2);

const QString vmax = QString("v<sub>max</sub>");
const QString Km = QString("K<sub>M</sub>");

const QString S0 = QString("S%1").arg(Unicode_Sub_0);

const QString qAB = QString("q<sub>AB</sub>");
const QString qAB2 = QString("q<sub>AB2</sub>");
const QString qA2B = QString("q<sub>A2B</sub>");
const QString qsolv = QString("q<sub>solv</sub>");

const QString qAB_ = QString("q'<sub>AB</sub>");
const QString qAB2_ = QString("q'<sub>AB2</sub>");
const QString qA2B_ = QString("q'<sub>A2B</sub>");

const QString q = QString("q");

const qreal R = 8.314459;
const qreal log2ln = 2.302585093;
const qreal cal2joule = 4.1868;

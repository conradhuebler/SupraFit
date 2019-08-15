/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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
const QString Unicode_delta = QChar(0x03B4);
const QString Unicode_mu = QChar(0x00B5);
const QString Unicode_sigma = QChar(0x03C3);

const QString Unicode_Sup_1 = QChar(0x00B9);

const QString Unicode_Sub_0 = QChar(0x2080);
const QString Unicode_Sub_1 = QChar(0x2081);
const QString Unicode_Sub_2 = QChar(0x2082);

const QString Unicode_Sub_y = QChar(0x0079); // not really

// const QString Unicode_Sub_AB = QString("%1%2").arg(QChar("A")).arg(QChar("B")); //does not exist yet

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
    ItoI = 1,
    IItoI_ItoI = 2,
    ItoI_ItoII = 3,
    IItoI_ItoI_ItoII = 4,
    Michaelis_Menten = 5,
    MonoMolecularModel = 6,
    itc_ItoI = 10,
    itc_IItoI = 11,
    itc_ItoII = 12,
    itc_IItoII = 13,
    itc_n_ItoI = 14,
    itc_n_ItoII = 15,
    itc_blank = 16,
    fl_ItoI = 20,
    fl_IItoI_ItoI = 21,
    fl_ItoI_ItoII = 22,
    fl_IItoI_ItoI_ItoII = 23,
    ScriptedModel = 100,
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

inline QString aboutHtml()
{
    QString info;
    info = "<div align='center'><img width='350' src=':/misc/logo_small.png'></div>";
    info += "<h4>" + version + "</h4>";
    info += "<p>This is all about SupraFit, nothing else matters< /p>";
    info += "<p>Created by Conrad Hübler</p>";
    info += "<p>Special thanks to <strong>Prof. M. Mazik</strong>, TU Bergakademie Freiberg for her support.</p>";
    info += "<p>Special thanks to <strong>Dr. Sebastian F&ouml;rster</strong> and <strong>Dr. Stefan Kaiser</strong> for finding bugs and constructive feedback.</p>";
    info += "<p>Special thanks to Alexander K&ouml;nig for testing the Quickstart, helpful comments and feedback.</p>";
    info += "<p>Thanks to all encouraged me writing the application.</p>";
    info += "<p>Built-in Icon Theme taken from Oxygens Icon : <a href='http://www.oxygen-icons.org'>http://www.oxygen-icons.org</a></p>";
    info += "<p>SupraFit website on GitHub: <a href='https://github.com/conradhuebler/SupraFit'>https://github.com/conradhuebler/SupraFit</a></p>";
    info += "<p>If you obtain results with SupraFit, I kindly ask to cite: </p><p>C. Hübler, conradhuebler/SupraFit: Version 1.7.0 2019, Zenodo. <a href='http://doi.org/10.5281/zenodo.3364570'>http://doi.org/10.5281/zenodo.3364570</a>.<p>";
    info += "<p>SupraFit has been compilied on " + QString::fromStdString(__DATE__) + " at " + QString::fromStdString(__TIME__) + ".\n";
#ifdef noto_font
    info += "<p>SupraFit uses and provides some selected Google Noto Font, see <a href='https://github.com/googlei18n/noto-fonts'>https://github.com/googlei18n/noto-fonts</a></p>";
#endif
    return info;
}

inline QString about()
{
    QString info = QString();
    info += "\t*********************************************************************************************************\n\n";
    info += "\t" + version + "\n";
    info += "\tThis is all about SupraFit, nothing else matters\n";
    info += "\tCreated by Conrad Hübler\n";
    info += "\t*********************************************************************************************************\n\n";
    info += "\tSpecial thanks to Prof. M. Mazik, TU Bergakademie Freiberg for her support.\n\n";
    info += "\tSpecial thanks to \t Dr. Sebastian Förster \t  and \t Dr. Stefan Kaiser \t for finding bugs and constructive feedback.\n";
    info += "\tSpecial thanks to Alexander König for testing the Quickstart, helpful comments and feedback.\n\n\n";
    info += "\tThanks to all encouraged me writing the application.\n\n";
    info += "\tSupraFit website on GitHub: https://github.com/conradhuebler/SupraFit\n\n";
    info += "\tIf you obtain results with SupraFit, I kindly ask to cite: C. Hübler, conradhuebler/SupraFit: Version 1.7.0 2019, Zenodo. http://doi.org/10.5281/zenodo.3364570\n";
    info += "\tSupraFit has been compilied on " + QString::fromStdString(__DATE__) + " at " + QString::fromStdString(__TIME__) + ".\n\n";
    info += "\t*********************************************************************************************************\n\n";
    return info;
}
}

class QString;

extern QString collective_messages;

QString getDir();
void setLastDir(const QString& str);

inline void Version(QCoreApplication* app, QCommandLineParser* parser)
{
    app->setApplicationName("SupraFit");
    app->setOrganizationName("Conrad Huebler");

    app->setApplicationVersion(version);

    parser->setApplicationDescription("A Open Source Qt5 based fitting tool for supramolecular titration experiments.");
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
        else
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
        return SupraFit::ItoI;

    else if (str == "2:1/1:1-Model")
        return SupraFit::IItoI_ItoI;

    else if (str == "1:1/1:2-Model")
        return SupraFit::ItoI_ItoII;

    else if (str == "2:1/1:1/1:2-Model")
        return SupraFit::IItoI_ItoI_ItoII;

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

    else if (str == "Monomolecular Kinetics")
        return SupraFit::MonoMolecularModel;

    else if (str == "Michaelis Menten")
        return SupraFit::Michaelis_Menten;

    else if (str == "Meta Model")
        return SupraFit::MetaModel;
    else
        return SupraFit::Unknown;
}

inline QString Model2Name(SupraFit::Model model)
{
    if (model == SupraFit::ItoI)
        return QString("%1H %2").arg(Unicode_Sup_1).arg("1:1-Model");

    else if (model == SupraFit::IItoI_ItoI)
        return QString("%1H %2").arg(Unicode_Sup_1).arg("2:1/1:1-Model");

    else if (model == SupraFit::ItoI_ItoII)
        return QString("%1H %2").arg(Unicode_Sup_1).arg("1:1/1:2-Model");

    else if (model == SupraFit::IItoI_ItoI_ItoII)
        return QString("%1H %2").arg(Unicode_Sup_1).arg("2:1/1:1/1:2-Model");

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

    else if (model == SupraFit::fl_ItoI)
        return QString("%1 %2").arg(Unicode_Phi).arg("1:1-Model");

    else if (model == SupraFit::fl_IItoI_ItoI)
        return QString("%1 %2").arg(Unicode_Phi).arg("2:1/1:1-Model");

    else if (model == SupraFit::fl_ItoI_ItoII)
        return QString("%1 %2").arg(Unicode_Phi).arg("1:1/1:2-Model");

    else if (model == SupraFit::fl_IItoI_ItoI_ItoII)
        return QString("%1 %2").arg(Unicode_Phi).arg("2:1/1:1/1:2-Model");

    else if (model == SupraFit::MonoMolecularModel)
        return "Monomolecular Kinetics";

    else if (model == SupraFit::Michaelis_Menten)
        return "Michaelis Menten";

    else if (model == SupraFit::MetaModel)
        return "Meta Model";
    else
        return "Unknown";
}

inline bool FuzzyCompare(qreal a, qreal b, int prec = 3)
{
    return qAbs(a - b) < qPow(10, -prec);
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

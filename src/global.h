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

#pragma once

#include "src/global_config.h"
#include "src/version.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QtGlobal>

#include <QtCore/QVector>

#include <cmath>
namespace SupraFit {

enum Statistic {
    MonteCarlo = 1,
    WeakenedGridSearch = 2,
    ModelComparison = 3,
    CrossValidation = 4,
    Reduction = 5,
    FastConfidence = 6
};

enum Model {
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
    Unknown = 404
};
struct ConfidenceBar {
    qreal lower = 0;
    qreal upper = 0;
};

struct BoxWhisker {
    QList<qreal> mild_outliers, extreme_outliers;
    qreal lower_whisker = 0;
    qreal upper_whisker = 0;
    qreal lower_quantile = 0;
    qreal upper_quantile = 0;
    qreal median = 0;
    qreal mean = 0;
    int count = 0;

    inline qreal UpperNotch() const { return median + (1.58 * (upper_quantile - lower_quantile) / sqrt(count)); }
    inline qreal LowerNotch() const { return median - (1.58 * (upper_quantile - lower_quantile) / sqrt(count)); }
};

inline QString Statistic2Name(SupraFit::Statistic type)
{
    switch (type) {
    case SupraFit::Statistic::WeakenedGridSearch:
        return ("Weakend Grid Search");
        break;

    case SupraFit::Statistic::ModelComparison:
        return ("Model Comparison");
        break;

    case SupraFit::Statistic::FastConfidence:
        return ("Fast Confidence");
        break;

    case SupraFit::Statistic::Reduction:
        return ("Reduction Analysis");
        break;

    case SupraFit::Statistic::MonteCarlo:
        return ("Monte Carlo Simulation");
        break;
    case SupraFit::Statistic::CrossValidation:
        return ("Cross Validation");
        break;
    }
    return QString();
}

inline QString Statistic2Name(int type)
{
    return Statistic2Name(SupraFit::Statistic(type));
}

inline QString aboutHtml()
{
    QString info;
    info = "<h4>" + version + "</h4>";
    info += "<p>This is all about SupraFit, nothing else matters< /p>";
    info += "<p>Created by Conrad Hübler</p>";
    info += "<p>Special thanks to <strong>Prof. M. Mazik</strong>, TU Bergakademie Freiberg for her support.</p>";
    info += "<p>Special thanks to <strong>Dr. Sebastian F&ouml;rster</strong> and <strong>Stefan Kaiser</strong> for finding bugs and constructive feedback.</p>";
    info += "<p>Thanks to all encouraged me writing the application.</p>";
    info += "<p>Built-in Icon Theme taken from Oxygens Icon : http://www.oxygen-icons.org/</p>";
    info += "<p>SupraFit website on GitHub: <a href='https://github.com/contra98/SupraFit'>https://github.com/contra98/SupraFit</a></p>";
    info += "<p>SupraFit has been compilied on " + QString::fromStdString(__DATE__) + " at " + QString::fromStdString(__TIME__) + ".\n";
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
    info += "\tSpecial thanks to \t Dr. Sebastian Förster \t  and \t Stefan Kaiser \t for finding bugs and constructive feedback.\n\n";
    info += "\tThanks to all encouraged me writing the application.\n\n";
    info += "\tBuilt-in Icon Theme taken from Oxygens Icon : http://www.oxygen-icons.org\n";
    info += "\tSupraFit website on GitHub: https://github.com/contra98/SupraFit\n\n";
    info += "\tSupraFit has been compilied on " + QString::fromStdString(__DATE__) + " at " + QString::fromStdString(__TIME__) + ".\n\n";
    info += "\t*********************************************************************************************************\n\n";
    return info;
}
}

enum OptimizationType {
    GlobalParameter = 0x01,
    LocalParameter = 0x02
};

inline OptimizationType operator~(OptimizationType a) { return (OptimizationType) ~(int)a; }
inline OptimizationType operator|(OptimizationType a, OptimizationType b) { return (OptimizationType)((int)a | (int)b); }
inline OptimizationType operator&(OptimizationType a, OptimizationType b) { return (OptimizationType)((int)a & (int)b); }
inline OptimizationType operator^(OptimizationType a, OptimizationType b) { return (OptimizationType)((int)a ^ (int)b); }
inline OptimizationType& operator|=(OptimizationType& a, OptimizationType b) { return (OptimizationType&)((int&)a |= (int)b); }
inline OptimizationType& operator&=(OptimizationType& a, OptimizationType b) { return (OptimizationType&)((int&)a &= (int)b); }
inline OptimizationType& operator^=(OptimizationType& a, OptimizationType b) { return (OptimizationType&)((int&)a ^= (int)b); }

struct OptimizerConfig {
    int MaxIter = 75;
    qreal Constant_Convergence = 1E-3;
    qreal Error_Convergence = 5E-7;

    int LevMar_Constants_PerIter = 1;
    int LevMar_Shifts_PerIter = 1;

    qreal LevMar_Factor = 100;
    qreal LevMar_Xtol = 1E-10;
    qreal LevMar_Gtol = 1E-10;
    qreal LevMar_Ftol = 1E-10;
    qreal LevMar_epsfcn = 1E-8;

    bool skip_not_converged_concentrations = false;
    int single_iter = 1500;
    double concen_convergency = 1e-13;
};

class QString;

extern int printLevel;

void PrintMessage(const QString& str, int Level);
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
    parser->addPositionalArgument("input file", QCoreApplication::translate("main", "File to open."));
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

    else
        return SupraFit::Unknown;
}

inline QString Model2Name(SupraFit::Model model)
{
    if (model == SupraFit::ItoI)
        return "1:1-Model";

    else if (model == SupraFit::IItoI_ItoI)
        return "2:1/1:1-Model";

    else if (model == SupraFit::ItoI_ItoII)
        return "1:1/1:2-Model";

    else if (model == SupraFit::IItoI_ItoI_ItoII)
        return "2:1/1:1/1:2-Model";

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
        return "&Phi; 1:1-Model";

    else if (model == SupraFit::fl_IItoI_ItoI)
        return "&Phi; 2:1/1:1-Model";

    else if (model == SupraFit::fl_ItoI_ItoII)
        return "&Phi; 1:1/1:2-Model";

    else if (model == SupraFit::fl_IItoI_ItoI_ItoII)
        return "&Phi; 2:1/1:1/1:2-Model";

    else if (model == SupraFit::MonoMolecularModel)
        return "Monomolecular Kinetics";

    else if (model == SupraFit::Michaelis_Menten)
        return "Michaelis Menten";

    else
        return "Unknown";
}

inline const QString included() { return QString("#77d740;"); }
inline const QString excluded() { return QString("#e17f7f;"); }


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

#include "src/client/analyser.h"
#include "src/client/simulator.h"

#include "src/global.h"
#include "src/global_config.h"
#include "src/version.h"

#include <QDebug>

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>

#include <QCoreApplication>

#include <iostream>

int main(int argc, char** argv)
{
    qInstallMessageHandler(myMessageOutput);

    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    Version(&app, &parser);

    QCommandLineOption input(QStringList() << "i"
                                           << "input",
        QCoreApplication::translate("main", "Input file name"),
        QCoreApplication::translate("main", "file"),
        QCoreApplication::translate("main", ""));
    parser.addOption(input);

    QCommandLineOption output(QStringList() << "o"
                                            << "output",
        QCoreApplication::translate("main", "Output file name\nThis option can be used to convert files (*.json <-> *.suprafit) or to define a base name for simulated experiments."),
        QCoreApplication::translate("main", "file"),
        QCoreApplication::translate("main", ""));
    parser.addOption(output);

    parser.addOption({ { "l", "list" }, "List stored models." });

    QCommandLineOption p(QStringList() << "p"
                                       << "print",
        QCoreApplication::translate("main", "Print model details. If empty print everything. Respects the statistic arguments\np - Project number\n m - Model number"),
        QCoreApplication::translate("main", "p|m"),
        QCoreApplication::translate("main", "0"));
    parser.addOption(p);

    QCommandLineOption t(QStringList() << "t"
                                       << "task",
        QCoreApplication::translate("main", "Define task to be done:\n a - Analyse Model \n g - Generate data\n o - Optimise Model \n s - Simulate experiments, optimise and analyse according to -s option"),
        QCoreApplication::translate("main", "a g o s"),
        QCoreApplication::translate("main", ""));
    parser.addOption(t);

    QCommandLineOption statistic(QStringList() << "s"
                                               << "statistic",
        QCoreApplication::translate("main", "Statistical post processing to be done\n a - Akaike Information Criterion\n c - Cross Validation\n m - Monte Carlo Simulation\n o - Model Comparison\n r - Reduction Analyse\n w- Weakend Grid Search"),
        QCoreApplication::translate("main", "a c m o r w"),
        QCoreApplication::translate("main", "acmorw"));
    parser.addOption(statistic);

    QCommandLineOption experiments(QStringList() << "e"
                                                 << "experiments",
        QCoreApplication::translate("main", "Number of experiments ( will only be evaluated if -t/--task s"),
        QCoreApplication::translate("main", "number"),
        QCoreApplication::translate("main", "1000"));
    parser.addOption(experiments);

    QCommandLineOption _std(QStringList() << "g"
                                          << "gaussian",
        QCoreApplication::translate("main", "Standard deviation for the gaussian error added in simulation."),
        QCoreApplication::translate("main", "standard deviation"),
        QCoreApplication::translate("main", "0.001"));
    parser.addOption(_std);

    QCommandLineOption threads(QStringList() << "n"
                                             << "nproc",
        QCoreApplication::translate("main", "Number of process to be run"),
        QCoreApplication::translate("main", "threads"),
        QCoreApplication::translate("main", "4"));
    parser.addOption(threads);

    parser.process(app);

    std::cout << SupraFit::about().toStdString() << std::endl;

    const QString infile = parser.value("i");

    if (infile.isEmpty()) {
        std::cout << "SupraFit needs an input file, which is a *.json or *.suprafit document." << std::endl;
        std::cout << "The simplest task for SupraFit to be done is opening a file and writing a project to disk." << std::endl;
        std::cout << "That would be like converting a *.json file to a *.suprafit file or vice versa :-)" << std::endl;
        std::cout << "suprafit_cli -i file.suprafit -o file.json" << std::endl;

        parser.showHelp(0);
    }

    QString outfile = parser.value("o");

    if (outfile.isEmpty())
        outfile = infile;

    const QString print = parser.value("print");
    const QString task = parser.value("task");
    std::cout << "Task is " << task.toStdString() << std::endl;
    int exp = parser.value("e").toInt();
    qreal std = parser.value("g").toDouble();

    bool reduction = parser.value("statistic").contains("r");
    bool crossvalidation = parser.value("statistic").contains("c");
    bool montecarlo = parser.value("statistic").contains("m");
    bool modelcomparison = parser.value("statistic").contains("o");
    bool weakendgrid = parser.value("statistic").contains("w");

    bool list = parser.isSet("l");
    qApp->instance()->setProperty("threads", parser.value("n").toInt());
    qApp->instance()->setProperty("series_confidence", true);

    if (task.isEmpty() || task.isNull()) {
        std::cout << "No task is not set, lets do the standard stuff ..." << std::endl;
        list = parser.isSet("l");
        if (infile == outfile && !list) {
            /*
            std::cout << "Input file and output file are the same AND nothing set to be done." << std::endl;
            std::cout << "For conversation of files type something like: " << std::endl;
            std::cout << "suprafit_cli -i file.suprafit -o file.json" << std::endl;
            */

            /* Lets take this as print model details */

            return 0;
        }
        if (list) {
            /* Here comes some simple json tree analyser */

            return 0;
        }

        if (infile != outfile) {
            /* Lets load the file (if projects, load several and the save them to the new name */

            return 0;
        }
        return 0;
    } else if (task == "a") {
        std::cout << "Reduction Analysis is turn on: " << reduction << std::endl;
        std::cout << "Cross Validation is turn on: " << crossvalidation << std::endl;
        std::cout << "Monte Carlo Simulation is turn on: " << montecarlo << std::endl;
        std::cout << "Model Comparison is turn on: " << modelcomparison << std::endl;
        std::cout << "Weakend Grid Search is turn on: " << weakendgrid << std::endl;
        std::cout << "Number of threads: " << qApp->instance()->property("threads").toInt() << std::endl;
        Analyser analyse;

        analyse.setReduction(reduction);
        analyse.setCrossValidation(crossvalidation);
        analyse.setModelComparison(modelcomparison);
        analyse.setWeakendGridSearch(weakendgrid);
        analyse.setMonteCarlo(montecarlo);
        analyse.setInFile(infile);
        analyse.setOutFile(outfile);
        /* We need to adopt this to work on the models stored */

        return 0;
    } else if (task == "g") {
        /* Something completely different */

        return 0;
    } else if (task == "o") {

        /* Load every model stored in this file and optimise it ... */

        return 0;
    } else if (task == "s") {
        std::cout << "No. Experiments to be simulated " << exp << std::endl;
        std::cout << "Standard Deviation to be used " << std << std::endl;
        std::cout << "Reduction Analysis is turn on: " << reduction << std::endl;
        std::cout << "Cross Validation is turn on: " << crossvalidation << std::endl;
        std::cout << "Monte Carlo Simulation is turn on: " << montecarlo << std::endl;
        std::cout << "Model Comparison is turn on: " << modelcomparison << std::endl;
        std::cout << "Weakend Grid Search is turn on: " << weakendgrid << std::endl;
        std::cout << "Number of threads: " << qApp->instance()->property("threads").toInt() << std::endl;
        Simulator simulator(exp, std);

        simulator.setReduction(reduction);
        simulator.setCrossValidation(crossvalidation);
        simulator.setModelComparison(modelcomparison);
        simulator.setWeakendGridSearch(weakendgrid);
        simulator.setMonteCarlo(montecarlo);
        simulator.setInFile(infile);
        simulator.setOutFile(outfile);
        simulator.FullTest();

        return 0;
    }
}

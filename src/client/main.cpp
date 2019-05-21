/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/capabilities/jobmanager.h"

#include "src/core/models.h"

#include "src/client/analyser.h"
#include "src/client/simulator.h"

#include "src/core/equil.h"
#include "src/core/jsonhandler.h"

#include "src/global.h"
#include "src/global_config.h"
#include "src/version.h"

#include <QDebug>

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>

#ifdef Qt5_10
#include <QtCore/QRandomGenerator>
#endif
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
        QCoreApplication::translate("main", "Define task to be done:\n a - Analyse Model \n c - Concentration solver test \n g - Generate data\n o - Optimise Model \n s - Simulate experiments, optimise and analyse according to -s option"),
        QCoreApplication::translate("main", "a c g o s"),
        QCoreApplication::translate("main", ""));
    parser.addOption(t);

    QCommandLineOption jobfile(QStringList() << "j"
                                             << "job",
        QCoreApplication::translate("main", "Run Content of a Jobfile"),
        QCoreApplication::translate("main", "jobfile"),
        QCoreApplication::translate("main", ""));
    parser.addOption(jobfile);

    QCommandLineOption statistic(QStringList() << "s"
                                               << "statistic",
        QCoreApplication::translate("main", "Statistical post processing to be done\n a - Akaike Information Criterion\n c - Cross Validation\n m - Monte Carlo Simulation\n o - Model Comparison\n r - Reduction Analyse\n w- Weakend Grid Search"),
        QCoreApplication::translate("main", "a c m o r w"),
        QCoreApplication::translate("main", "acmorw"));
    parser.addOption(statistic);

    QCommandLineOption experiments(QStringList() << "e"
                                                 << "experiments",
        QCoreApplication::translate("main", "Number of experiments ( will only be evaluated if -t/--task s or -t/--task c)!"),
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

    const QString print = parser.value("print");
    const QString task = parser.value("task");
    const QString job = parser.value("j");

    if (infile.isEmpty() && (task != "c")) {
        std::cout << "SupraFit needs an input file, which is a *.json or *.suprafit document." << std::endl;
        std::cout << "The simplest task for SupraFit to be done is opening a file and writing a project to disk." << std::endl;
        std::cout << "That would be like converting a *.json file to a *.suprafit file or vice versa :-)" << std::endl;
        std::cout << "suprafit_cli -i file.suprafit -o file.json" << std::endl;

        parser.showHelp(0);
    }

    QString outfile = parser.value("o");

    if (outfile.isEmpty())
        outfile = infile;

    std::cout << "Task is " << job.toStdString() << std::endl;
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

    if (parser.isSet("j") && parser.isSet("i")) {

        for (const QString& str : parser.values("j")) {
            QStringList data_files;
            QJsonObject job;
            JsonHandler::ReadJsonFile(job, str);

            if (job.keys().contains("main")) {

                Simulator* simulator = new Simulator;
                simulator->setInFile(parser.value("i"));
                simulator->setMainJson(job["main"].toObject());
                simulator->setModelsJson(job["model"].toObject());
                simulator->setJobsJson(job["jobs"].toObject());
                data_files = simulator->Generate();
            }
        }

        return 0;

        /*
        QPointer<DataClass> data;
        if (!JsonHandler::ReadJsonFile(m_toplevel, parser.value("i")))
            return 0;
        data = new DataClass(m_toplevel["data"].toObject());
        QStringList keys = m_toplevel.keys();
        QJsonObject toplevel, dataObject;
        dataObject = data->ExportData();

        for (const QString& key : keys) {
            if (key == "data")
                continue;

            SupraFit::Model model = static_cast<SupraFit::Model>(m_toplevel[key].toObject()["model"].toInt());
            QSharedPointer<AbstractModel> t = CreateModel(model, data);
            t->ImportModel(m_toplevel[key].toObject());

            JobManager* manager = new JobManager;
            manager->setModel(t);
            int i = 0;
            for (const QString& str : parser.values("j")) {
                QJsonObject job;
                JsonHandler::ReadJsonFile(job, str);
                manager->AddJob(job);
            }
            manager->RunJobs();
            toplevel["model_" + QString::number(i)] = t->ExportModel(true, false);
            i++;
            delete manager;
        }
        toplevel["data"] = dataObject;
        JsonHandler::WriteJsonFile(toplevel, parser.value("i"));
        */
    }

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
    } else if (task == "c") {
        std::cout << "Concentration solver 2:1/1:1/1:2 test!" << std::endl
                  << std::endl;
#ifdef Qt5_10
        IItoI_ItoI_ItoII_Solver* solver = new IItoI_ItoI_ItoII_Solver();
        OptimizerConfig opt_config;
        opt_config.concen_convergency = 10E-13;
        opt_config.single_iter = 1500;
        solver->setConfig(opt_config);

        qreal all_diff_1 = 0, all_diff_2 = 0;
        int fine = 0, lfine = 0;
        for (int i = 0; i < exp; ++i) {
            qreal K11 = qPow(10, 1 + QRandomGenerator::global()->bounded(4.0));
            qreal K21 = qPow(10, 1 + QRandomGenerator::global()->bounded(4.0));
            qreal K12 = qPow(10, 1 + QRandomGenerator::global()->bounded(4.0));

            qreal A = QRandomGenerator::global()->bounded(1e-4);
            qreal B = QRandomGenerator::global()->bounded(1e-4);

            qreal AB = K11 * A * B;
            qreal A2B = K21 * A * AB;
            qreal AB2 = K12 * AB * B;

            qreal A0 = 2 * A2B + AB + AB2;
            qreal B0 = A2B + AB + 2 * AB2;

            solver->setConstants(QList<qreal>() << K21 << K11 << K12);
            solver->setInput(A0, B0);

            solver->RunTest();

            qreal diff_1 = qAbs(A - solver->Concentrations().first) + qAbs(B - solver->Concentrations().second);
            qreal diff_2 = qAbs(A - solver->ConcentrationsLegacy().first) + qAbs(B - solver->ConcentrationsLegacy().second);

            if (solver->Ok())
                all_diff_1 += diff_1;
            if (solver->LOk())
                all_diff_2 += diff_2;

            std::cout << log10(K21) << "\t"
                      << log10(K11) << "\t"
                      << log10(K12) << "\t"
                      << A << "\t"
                      << B << "\t"
                      << solver->Concentrations().first << "\t"
                      << solver->Concentrations().second << "\t"
                      << diff_1 << "\t" << solver->Ok() << "\t"
                      << solver->ConcentrationsLegacy().first << "\t"
                      << solver->ConcentrationsLegacy().second << "\t"
                      << diff_2 << "\t" << solver->LOk() << std::endl;
            fine += solver->Ok();
            lfine += solver->LOk();
        }
        std::cout << "Mean errors for standard method " << all_diff_1 / double(fine) << "\t and for legacy solver " << all_diff_2 / double(lfine) << std::endl;
        std::cout << "Time for Solver " << solver->Time() << " Fine Calculation = " << fine << std::endl;
        std::cout << "Time for Solver " << solver->LTime() << " Fine Calculation = " << lfine << std::endl;

        delete solver;
#else
        std::cout << "Sorry, but this functions need Qt 5.10 or newer ... " << std::endl;
#endif
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
        /*
        Simulator simulator(exp, std);

        simulator.setReduction(reduction);
        simulator.setCrossValidation(crossvalidation);
        simulator.setModelComparison(modelcomparison);
        simulator.setWeakendGridSearch(weakendgrid);
        simulator.setMonteCarlo(montecarlo);
        simulator.setInFile(infile);
        simulator.setOutFile(outfile);
        simulator.FullTest();
        */
        return 0;
    }
}

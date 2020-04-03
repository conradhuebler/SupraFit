/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 - 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/models.h"

#include "src/client/analyser.h"
#include "src/client/simulator.h"

#include "src/core/equil.h"
#include "src/core/jsonhandler.h"

#include "src/global.h"
#include "src/global_config.h"
#include "src/version.h"

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>

#include <QtCore/QRandomGenerator>
#include <QCoreApplication>

#include <iostream>

int main(int argc, char** argv)
{

    SupraFit::timer t;
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

    QCommandLineOption jobfile(QStringList() << "j"
                                             << "job",
        QCoreApplication::translate("main", "Run Content of a Jobfile"),
        QCoreApplication::translate("main", "jobfile"),
        QCoreApplication::translate("main", ""));
    parser.addOption(jobfile);

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
    const QString job = parser.value("j");

    if (infile.isEmpty() && job.isEmpty()) {
        std::cout << "SupraFit needs an input file, which is a *.json or *.suprafit document." << std::endl;
        std::cout << "The simplest task for SupraFit to be done is opening a file and writing a project to disk." << std::endl;
        std::cout << "That would be like converting a *.json file to a *.suprafit file or vice versa :-)" << std::endl;
        std::cout << "suprafit_cli -i file.suprafit -o file.json" << std::endl
                  << std::endl
                  << std::endl;

        std::cout << "To run a jobfile on a specific input file, run " << std::endl;
        std::cout << "suprafit_cli -i file.suprafit -j jobfile.json " << std::endl
                  << std::endl;

        std::cout << "To print the content of a file just type" << std::endl;
        std::cout << "suprafit_cli -i file.suprafit " << std::endl;
        parser.showHelp(0);
    }

    QString outfile = parser.value("o");

    bool list = parser.isSet("l");
    qApp->instance()->setProperty("threads", parser.value("n").toInt());
    qApp->instance()->setProperty("series_confidence", true);
    qApp->instance()->setProperty("InitialiseRandom", true);
    qApp->instance()->setProperty("StoreRawData", true);

    QJsonObject infile_json;
    JsonHandler::ReadJsonFile(infile_json, infile);
    SupraFitCli* core = new SupraFitCli;
    QVector<QJsonObject> projects;
    if (infile_json.keys().contains("main", Qt::CaseInsensitive)) {
        /**
          Everything is defined in the input file
          */
        core->setControlJson(infile_json);
    } else {
        QJsonObject jobfile_json;
        JsonHandler::ReadJsonFile(jobfile_json, job);
        if (jobfile_json.keys().contains("main", Qt::CaseInsensitive)) {
            /* The jobfile defines everythin */
            core->setControlJson(jobfile_json);
        } else {
            /* There must be a infile and a jobfile having at least some information */
            core->setInFile(infile);
            core->setControlJson(jobfile_json);
        }
    }
    if (!core->LoadFile()) {
        std::cout << "Sorry, input file could not be opened." << std::endl;
        return 0;
    }
    if (core->SimulationData()) {
        Simulator* simulator = new Simulator(core);
        projects = simulator->Data();
    }
    core->Work();
    //core->Pr
    // projects
    // for (const auto& project : qAsConst(projects)) {
    //     core->PerfomeJobs(project, job["Models"].toObject(), job["Jobs"].toObject());
    // }
    //}
    /*
        for (const QString& str : parser.values("j")) {
            QVector<QJsonObject> projects;
            QStringList data_files;
            QJsonObject job;
            JsonHandler::ReadJsonFile(job, str);

            SupraFitCli* simulator = new SupraFitCli;
            simulator->setInFile(parser.value("i"));

            bool analyse = false;
            if (job.keys().contains("analyse"))
                analyse = simulator->setAnalyseJson(job["analyse"].toObject());
            else if (job.keys().contains("Analyse"))
                analyse = simulator->setAnalyseJson(job["Analyse"].toObject());

            if (job.keys().contains("main") || job.keys().contains("Main")) {
                bool generate = false, model = false;
                bool prepare = false;

                if (job["main"].toObject().contains("Prepare")) {
                    simulator->setPreparation(job["main"].toObject()["Prepare"].toObject());
                    prepare = true;
                } else if (job["Main"].toObject().contains("Prepare")) {
                    simulator->setPreparation(job["Main"].toObject()["Prepare"].toObject());
                    prepare = true;
                }

                if (job.contains("main"))
                    job["Main"] = job["main"].toObject();

                if (job.contains("Main"))
                    generate = simulator->setMainJson(job["Main"].toObject());

                if (job.contains("model"))
                    job["Models"] = job["model"].toObject();

                if (job.contains("Models"))
                    model = simulator->setModelsJson(job["Models"].toObject());

                if (job.contains("job"))
                    job["Jobs"] = job["jobs"].toObject();

                if (job.contains("Jobs"))
                    simulator->setJobsJson(job["Jobs"].toObject());

                    if (generate) {
                        projects = simulator->GenerateData();
                    } else if (prepare) {
                        simulator->Prepare();
                        projects = simulator->Data();
                    } else {
                        if (!simulator->LoadFile())
                            return 0;

                        projects = simulator->Data();
                    }

                if (model) {
                    for (const auto& project : qAsConst(projects)) {
                        simulator->PerfomeJobs(project, job["Models"].toObject(), job["Jobs"].toObject());
                    }
                }

                } else {
                    simulator->OpenFile();
                }
                if (analyse)
                    simulator->Analyse(job["analyse"].toObject());

                delete simulator;
        }

        return 0;
    */

    {
        SupraFitCli* suprafitcli = new SupraFitCli;
        suprafitcli->setInFile(infile);
        if (!suprafitcli->LoadFile()) {
            std::cout << "Can not open file " << infile.toStdString() << std::endl
                      << "Sorry, going home." << std::endl;
            return 1;
        }
        suprafitcli->setOutFile(outfile);
        std::cout << "No task is not set, lets do the standard stuff ..." << std::endl;
        list = parser.isSet("l");

        if (infile != outfile) {
            /* Lets load the file (if projects, load several and then save them to the new name */

            if (suprafitcli->LoadFile())
                if (suprafitcli->SaveFile())
                    return 0;
                else
                    return 2; // Cannot save file
            else
                return 1; // cannot load file
        }

        if (!list) {

            std::cout << "Input file and output file are the same AND nothing set to be done." << std::endl;
            std::cout << "For conversation of files type something like: " << std::endl;
            std::cout << "suprafit_cli -i file.suprafit -o file.json" << std::endl;

            /* Lets take this as print model details */
            std::cout << "Print file content." << std::endl;
            suprafitcli->PrintFileContent();
            return 0;
        }
        if (list) {
            /* Here comes some simple json tree analyser */
            std::cout << "Print file strucuture." << std::endl;
            suprafitcli->PrintFileStructure();
            return 0;
        }


        return 0;
    }

    /*
        std::cout << "Concentration solver 2:1/1:1/1:2 test!" << std::endl
                  << std::endl;

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
*/
    return 0;
}

/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 - 2019  Conrad Hübler <Conrad.Huebler@gmx.net>
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
 */

#include <random>

#include <QCoreApplication>

#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>

#include "src/capabilities/jobmanager.h"
#include "src/capabilities/modelcomparison.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/capabilities/resampleanalyse.h"
#include "src/capabilities/weakenedgridsearch.h"

#include "src/core/dataclass.h"
#include "src/core/jsonhandler.h"
#include "src/core/minimizer.h"
#include "src/core/models.h"
#include "src/core/toolset.h"

#include "simulator.h"

Simulator::Simulator()
{
}

Simulator::~Simulator()
{
    if (m_data)
        delete m_data;
}

QStringList Simulator::Generate()
{
    m_extension = ".suprafit";

    QStringList filelist;

    if (m_mainjson.isEmpty())
        return filelist;

    if (m_infile.isEmpty() || m_infile.isNull()) {
        if (m_mainjson.contains("InFile"))
            m_infile = m_mainjson["InFile"].toString();

        if (m_infile.isEmpty() || m_infile.isNull())
            return filelist;
    }

    QJsonObject toplevel;
    JsonHandler::ReadJsonFile(toplevel, m_infile);

    if (toplevel.isEmpty())
        return filelist;

    m_data = new DataClass(toplevel["data"].toObject());
    if (m_data->DataPoints() == 0)
        return filelist;

    qint64 seed = QDateTime::currentMSecsSinceEpoch();
    std::mt19937 rng;
    rng.seed(seed);

    int runs = m_mainjson["MaxSteps"].toInt();
    double std = m_mainjson["Variance"].toDouble();
    if (runs < 1 || std <= 0)
        return filelist;

    if (m_mainjson.contains("OutFile")) {
        m_outfile = m_mainjson["OutFile"].toString();
    }

    QJsonObject table = m_data->DependentModel()->ExportTable(true);
    for (int i = 0; i < runs; ++i) {
        std::cout << "########################################################################################################" << std::endl;
        std::cout << "########################################################################################################" << std::endl
                  << std::endl;
        std::cout << "#####################  Starting run number " << i + 1 << " #######################################################" << std::endl
                  << std::endl;
        std::cout << "Generating new Data Table for Monte Carlo Simulation" << std::endl;
        std::normal_distribution<double> Phi = std::normal_distribution<double>(0, std);
        m_data->DependentModel()->ImportTable(table);
        QPointer<DataTable> model_table = m_data->DependentModel()->PrepareMC(Phi, rng);
#ifdef _DEBUG
        model_table->Debug();
        m_data->DependentModel()->Debug();
#endif
        QPointer<DataClass> data = new DataClass(m_data);
        data->setDependentTable(model_table);
        data->NewUUID();
#ifdef _DEBUG
        data->IndependentModel()->Debug();
        data->DependentModel()->Debug();
#endif
        QJsonObject exp_level, dataObject;
        dataObject = data->ExportData();
        exp_level["data"] = dataObject;
        if (!m_modelsjson.isEmpty()) {
            std::cout << "Loading Models into Dataset" << std::endl;
            QVector<QSharedPointer<AbstractModel>> models = AddModels(m_modelsjson, data);
            std::cout << "Loading " << models.size() << " Models into Dataset finished!" << std::endl;
            if (!m_jobsjson.isEmpty()) {
                std::cout << "Starting jobs ..." << std::endl;
                JobManager* manager = new JobManager;
                for (int model_index = 0; model_index < models.size(); ++model_index) {
                    std::cout << "... model  " << model_index << std::endl;
                    for (const QString& j : m_jobsjson.keys()) {
                        QJsonObject job = m_jobsjson[j].toObject();
                        manager->setModel(models[model_index]);
                        manager->AddJob(job);
                        manager->RunJobs();
                        std::cout << "... model  " << model_index << " job done!" << std::endl;
                    }
                    std::cout << "jobs for model  " << model_index << "  finished!" << std::endl;
                }
                delete manager;
                std::cout << "jobs all done!" << std::endl;
            }

            for (int i = 0; i < models.size(); ++i) {
                exp_level["model_" + QString::number(i)] = models[i]->ExportModel();
                models[i].clear();
            }
            models.clear();
        }
        QString outfile = QString(m_outfile + "_" + QString::number(i) + m_extension);
        if (JsonHandler::WriteJsonFile(exp_level, outfile)) {
            filelist << outfile;
            std::cout << outfile.toStdString() << " successfully written to disk" << std::endl;
        }
    }
    return filelist;
}

QVector<QSharedPointer<AbstractModel>> Simulator::AddModels(const QJsonObject& modelsjson, QPointer<DataClass> data)
{
    QVector<QSharedPointer<AbstractModel>> models;
    for (const QString& str : modelsjson.keys()) {
        SupraFit::Model model = static_cast<SupraFit::Model>(modelsjson[str].toInt());
        QSharedPointer<AbstractModel> t = CreateModel(model, data);

        if (m_mainjson.contains("Guess") && m_mainjson["Guess"].toBool()) {
            t->InitialGuess();

            if (m_mainjson.contains("Fit") && m_mainjson["Fit"].toBool()) {
                QPointer<MonteCarloThread> thread = new MonteCarloThread();
                thread->setModel(t);
                thread->run();
                t->ImportModel(thread->Model());
                t->setFast(false);
                t->Calculate();
                delete thread;
            }
        }
        models << t;
    }
    return models;
}

bool Simulator::LoadFile()
{

    QJsonObject m_toplevel;
    if (JsonHandler::ReadJsonFile(m_toplevel, m_infile)) {
        m_data = new DataClass(m_toplevel["data"].toObject());
        if (m_data->DataPoints() != 0) {
#ifdef _DEBUG
            qDebug() << m_infile << " successfully loaded";
            qDebug() << m_toplevel;
#endif
            return true;
        } else
            return false;
    }
    return false;
}

bool Simulator::FullTest()
{
    if (!LoadFile())
        return false;

    qint64 seed = QDateTime::currentMSecsSinceEpoch();
    std::mt19937 rng;
    rng.seed(seed);
#ifdef _DEBUG
    qDebug() << "Original Table";
    m_data->IndependentModel()->Debug();
    m_data->DependentModel()->Debug();
#endif
    QJsonObject table = m_data->DependentModel()->ExportTable(true);
    for (int i = 0; i < m_runs; ++i) {
        std::cout << "########################################################################################################" << std::endl;
        std::cout << "########################################################################################################" << std::endl
                  << std::endl;
        std::cout << "#####################  Starting run number " << i + 1 << " #######################################################" << std::endl
                  << std::endl;
        std::cout << "Generating new Data Table for Monte Carlo Simulation" << std::endl;
        std::normal_distribution<double> Phi = std::normal_distribution<double>(0, m_std);
        m_data->DependentModel()->ImportTable(table);
        QPointer<DataTable> model_table = m_data->DependentModel()->PrepareMC(Phi, rng);
#ifdef _DEBUG
        model_table->Debug();
        m_data->DependentModel()->Debug();
#endif
        QPointer<DataClass> data = new DataClass(m_data);
        data->setDependentTable(model_table);
        data->NewUUID();
#ifdef _DEBUG
        data->IndependentModel()->Debug();
        data->DependentModel()->Debug();
#endif
        QSharedPointer<AbstractModel> model_1to1 = Test11Model(data);
        // model_1to1->OverrideSystemParameter(m_data->SysPar());

        Test(model_1to1);

        QSharedPointer<AbstractModel> model_2to1 = Test2111Model(data);
        Test(model_2to1);

        QSharedPointer<AbstractModel> model_1to2 = Test1112Model(data);
        Test(model_1to2);

        QSharedPointer<AbstractModel> model_2to2 = Test22Model(data);
        Test(model_2to2);

        QJsonObject toplevel, dataObject;
        dataObject = data->ExportData();

        toplevel["model_0"] = model_1to1->ExportModel();

        toplevel["model_1"] = model_2to1->ExportModel();

        toplevel["model_2"] = model_1to2->ExportModel();

        toplevel["model_3"] = model_2to2->ExportModel();

        toplevel["data"] = dataObject;

        if (JsonHandler::WriteJsonFile(toplevel, QString(m_outfile + "_" + QString::number(i) + m_extension)))
            std::cout << QString(m_outfile + "_" + QString::number(i) + m_extension).toStdString() << " successfully written to disk" << std::endl;

        //         if(model_table)
        //             delete model_table;
    }
    QTimer::singleShot(100, qApp, SLOT(quit()));

    return true;
}

void Simulator::Test(QSharedPointer<AbstractModel> model)
{

    std::cout << "********************************************************************************************************" << std::endl;
    std::cout << "************************    Model analysis starts right now       **************************************" << std::endl;
    std::cout << "********************************************************************************************************" << std::endl
              << std::endl;
    std::cout << "                         " << model->Name().toStdString() << std::endl;
    model->InitialGuess();
    model->Calculate();
    /*
     * First we fit the current model
     */
    QPointer<MonteCarloThread> thread = new MonteCarloThread();
    thread->setModel(model);
    thread->run();
    model->ImportModel(thread->Model());
    model->setFast(false);
    model->Calculate();
    /*
     * Then we should print out the model data
     */

    std::cout << model->Global2Text().toStdString() << std::endl;

    /*
     * We start with statistics
     */
    /*
    QJsonObject statistic;
    QJsonObject toplevel, dataObject;
    JobManager* manager = new JobManager;
    manager->setModel(model);

        QJsonObject job;
        JsonHandler::ReadJsonFile(job, str);
        manager->AddJob(job);

    manager->RunJobs();
    toplevel["model_" + QString::number(i)] = model->ExportModel(true, false);
    i++;
    delete manager;


    if (m_weakendgrid) {
        std::cout << "Weakend Grid Search" << std::endl;
        statistic = GridSearch(model);
        PrintStatistic(statistic, model);
    }*/
}

void Simulator::PrintStatistic(const QJsonObject& object, QSharedPointer<AbstractModel> model)
{
    if (object.isEmpty())
        return;

    model->UpdateStatistic(object);
    //     qDebug() << object;
}

void Simulator::Progress(int i, int max)
{
    m_current += 1;
    double percentage = m_current / 1000.0;

    if ((int(percentage) % 10) != 0) {
        return;
    }

    int barWidth = 70;

    std::cout << "[";
    int pos = barWidth * percentage;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }
    std::cout << "] " << int(percentage * 100.0) << " %\r";
    std::cout.flush();
}

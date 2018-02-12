/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/capabilities/montecarlostatistics.h"
#include "src/capabilities/weakenedgridsearch.h"
#include "src/capabilities/reductionanalyse.h"
#include "src/capabilities/modelcomparison.h"

#include "src/core/dataclass.h"
#include "src/core/jsonhandler.h"
#include "src/core/models.h"
#include "src/core/toolset.h"

#include "console.h"

Console::Console(int runs, double std) : m_runs(runs), m_std(std)
{
        
}

Console::~Console()
{
    if(m_data)
        delete m_data;
}


bool Console::LoadFile(const QString &file)
{    
    m_file = file;
    QJsonObject m_toplevel;
    if(JsonHandler::ReadJsonFile(m_toplevel, m_file))
    {
        m_data = new DataClass(m_toplevel);
        if(m_data->DataPoints() != 0)
        {
#ifdef _DEBUG
      qDebug() << m_file << " successfully loaded";
      qDebug() << m_toplevel;
#endif
            return true;
        }
        else
            return false;
    }
    return false;
}


bool Console::FullTest()
{        
    quint64 seed = QDateTime::currentMSecsSinceEpoch();
    std::mt19937 rng;
    rng.seed(seed);
#ifdef _DEBUG
    qDebug() << "Original Table";
    m_data->IndependentModel()->Debug();
    m_data->DependentModel()->Debug();
#endif
    for(int i = 0; i < m_runs; ++i)
    {
        std::cout << "########################################################################################################" << std::endl;
        std::cout << "########################################################################################################" << std::endl << std::endl;
        std::cout << "#####################  Starting run number "<< i + 1<< " #######################################################" << std::endl << std::endl;
        std::cout << "Generating new Data Table for Monte Carlo Simulation" << std::endl;
        std::normal_distribution<double> Phi = std::normal_distribution<double>(0,m_std);
        QPointer<DataTable > model_table = m_data->DependentModel()->PrepareMC(Phi, rng);
#ifdef _DEBUG
        model_table->Debug();
#endif
        QPointer<DataClass > data = new DataClass(m_data);
        data->setDependentTable(model_table);
#ifdef _DEBUG
        data->IndependentModel()->Debug();
        data->DependentModel()->Debug();
#endif
        QSharedPointer<AbstractModel> model_1to1 = Test11Model(data);
        Test(model_1to1);
        
        QSharedPointer<AbstractModel> model_2to1 = Test2111Model(data);
        Test(model_2to1);
        
        QSharedPointer<AbstractModel> model_1to2 = Test1112Model(data);
        Test(model_1to2);
        
        QJsonObject toplevel, dataObject;
        dataObject = data->ExportData();
        
        toplevel["model_0"] = model_1to1->ExportModel();; 
        toplevel["model_1"] = model_2to1->ExportModel();; 
        toplevel["model_2"] = model_1to2->ExportModel();; 
        toplevel["data"] = dataObject;
        if(        JsonHandler::WriteJsonFile(toplevel, m_file+"_"+QString::number(i) + ".suprafit") )
            std::cout << QString(m_file+"_"+QString::number(i) + ".suprafit").toStdString() << " successfully written to disk" <<std::endl;
        
//         if(model_table)
//             delete model_table;
    }
    QTimer::singleShot(100, qApp, SLOT(quit()));
    
    return true;
}

void Console::Test(QSharedPointer<AbstractModel> model)
{
    
    std::cout << "********************************************************************************************************" << std::endl;
    std::cout << "************************    Model analysis starts right now       **************************************" << std::endl;
    std::cout << "********************************************************************************************************" << std::endl << std::endl;
    std::cout << "                         " << model->Name().toStdString() << std::endl;
    model->InitialGuess();
    model->Calculate();
    /*
     * First we fit the current model
     */
    MCConfig config;
    
    QPointer<MonteCarloThread > thread = new MonteCarloThread(config);
    thread->setModel(model);
    thread->run();
    model->ImportModel(thread->Model());
    
    /*
     * Then we should print out the model data
     */
    
    std::cout << model->Global2Text().toStdString() << std::endl;
    
    /*
     * We start with statistics
     */
    QJsonObject statistic;
    
    if(m_reduction)
    {
        std::cout << "Reduction Analysis" << std::endl;
        statistic = Reduction(model);
        PrintStatistic(statistic, model);
    }
    
    if(m_crossvalidation)
    {
        std::cout << "Cross Validation" << std::endl;
        statistic = CrossValidation(model);
        PrintStatistic(statistic, model);
    }
    
    if(m_montecarlo)
    {
        std::cout << "Monte Carlo Simulation" << std::endl;
        statistic = MonteCarlo(model);
        PrintStatistic(statistic, model);
    }
    
    if(m_modelcomparison)
    {
        std::cout << "Model Comparison" << std::endl;
        statistic = MoCoAnalyse(model);
        PrintStatistic(statistic, model);
    }
    
    if(m_weakendgrid)
    {
        std::cout << "Weakend Grid Search" << std::endl;
        statistic = GridSearch(model);
        PrintStatistic(statistic, model);
    }
}

void Console::PrintStatistic(const QJsonObject& object, QSharedPointer<AbstractModel> model)
{
    if(object.isEmpty())
        return;
    
    model->UpdateStatistic(object);
//     qDebug() << object;
}

QJsonObject Console::MonteCarlo(QSharedPointer<AbstractModel> model)
{
    MCConfig config;
    config.maxsteps = 1000;
    QPointer<MonteCarloStatistics > statistic = new MonteCarloStatistics(config, this);

    statistic->setModel(model);
    statistic->Evaluate();
    model->UpdateStatistic(statistic->Result());
    
    QJsonObject result = statistic->Result();
    delete statistic;
    return result;
}

QJsonObject Console::MoCoAnalyse(QSharedPointer<AbstractModel> model)
{
     MoCoConfig config;
     config.mc_steps = 1000;
     config.f_value = model->finv(0.95);
     config.fisher_statistic = true;
     
     qreal error = model.data()->SumofSquares();
     
     config. maxerror = error*(config.f_value*model.data()->Parameter()/(model.data()->Points()-model.data()->Parameter()) +1);; 
     
     ModelComparison *statistic = new ModelComparison(config, this);
     statistic->setModel(model);
     bool result = statistic->Confidence();
     QJsonObject res = statistic->Result();
     if(!result)
         res = QJsonObject();
     
     delete statistic;
     return res;
}

QJsonObject Console::Reduction(QSharedPointer<AbstractModel> model)
{
    ReductionAnalyse *statistic = new ReductionAnalyse(model->getOptimizerConfig(), OptimizationType::ComplexationConstants | OptimizationType::OptimizeShifts);
    statistic->setModel(model);
    statistic->PlainReduction();
    model->UpdateStatistic(statistic->Result());

    QJsonObject result = statistic->Result();
    
    delete statistic;
    return result;
}

QJsonObject Console::CrossValidation(QSharedPointer<AbstractModel> model)
{
    ReductionAnalyse *statistic = new ReductionAnalyse(model->getOptimizerConfig(), OptimizationType::ComplexationConstants | OptimizationType::OptimizeShifts);
    statistic->setModel(model);
    statistic->CrossValidation(ReductionAnalyse::CVType::LeaveOneOut);
    model->UpdateStatistic(statistic->Result());
    
    statistic->CrossValidation(ReductionAnalyse::CVType::LeaveTwoOut);
    model->UpdateStatistic(statistic->Result());
       
    QJsonObject result = statistic->Result();
    
    delete statistic;
    return result;
}


QJsonObject Console::GridSearch(QSharedPointer<AbstractModel> model)
{
     WGSConfig config;
     config.f_value = model->finv(0.95);
     config.fisher_statistic = true;
     
     qreal error = model.data()->SumofSquares();
     
     config. maxerror = error*(config.f_value*model.data()->Parameter()/(model.data()->Points()-model.data()->Parameter()) +1);; 
     
     WeakenedGridSearch *statistic = new WeakenedGridSearch(config, this);
     statistic->setModel(model);
     statistic->setParameter(model->ExportModel(false));
    
     statistic->ConfidenceAssesment();
     
     QJsonObject result = statistic->Result();

     delete statistic;
     return result;
}
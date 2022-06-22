/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2020 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

//#define _CxxThreadPool_TimeOut 10
//#define _CxxThreadPool_Verbose true

#include "CxxThreadPool.h"

#include "src/global_config.h"

#include "src/core/models/AbstractModel.h"
#include "src/core/models/chaiinterpreter.h"
#include "src/core/models/dukmodelinterpreter.h"
#include "src/core/models/pymodelinterpreter.h"

#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include <QtCore/QCollator>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QtMath>

#include <cmath>
#include <iostream>

#include <libpeakpick/mathhelper.h>
#include <libpeakpick/peakpick.h>

#include "scriptmodel.h"

CalculateThread::CalculateThread(int rows, int cols, DataTable* X, DataTable* Global, DataTable* Local, const QStringList& input_names, const QStringList& global_names, const QStringList& local_names, const QString& execute)
    : m_X(X)
    //, m_Global(Global)
    //, m_Local(Local)
    , m_input_names(input_names)
    , m_global_names(global_names)
    , m_local_names(local_names)
    , m_execute(execute)
    , m_rows(rows)
    , m_cols(cols)
{
    m_chai.setInput(m_X->Table());
    m_chai.setGlobal(Global->Table(), m_global_names);
    m_chai.setLocal(Local->Table());
    m_chai.setInputNames(m_input_names);
    // m_chai.setExecute(m_execute);
    // m_chai.InitialiseChai();
    m_result = new DataTable(rows, cols, NULL);
}

void CalculateThread::UpdateParameter(DataTable* Global, DataTable* Local)
{
    m_chai.setGlobal(Global->Table(), m_global_names);
    m_chai.setLocal(Local->Table());
}

CalculateThread::~CalculateThread()
{
    delete m_result;
}

int CalculateThread::execute()
{
    if (m_end > m_rows)
        m_end = m_rows;

    if (m_start > m_rows)
        m_start = m_rows;
    if (m_start == m_end) {
        m_valid = false;
        return 0;
    }
    m_chai.setInput(m_X->Table());
    // m_chai.setGlobal(m_Global->Table(), m_global_names);
    // m_chai.setLocal(m_Local->Table());
    m_chai.setInputNames(m_input_names);
    m_chai.UpdateChai();
    // QString execute = m_execute_chai.join("\n");
    for (int series = 0; series < m_cols; ++series) {
        for (int i = m_start; i < m_end; ++i) {
            QString cache = m_execute;
            for (int parameter = 0; parameter < m_X->columnCount(); ++parameter) {
                cache.replace(
                    m_input_names[parameter],
                    QString::number(m_X->data(i, parameter)));
            }
            int error = 0;

            double result = m_chai.Evaluate(cache.toUtf8(), error);
            if (error == 1) {
                cache.replace("var", "");
                result = m_chai.Evaluate(cache.toUtf8(), error);
            }
            m_result->data(i, series) = result;
            // m_result(i, series, result);
        }
    }
    return 0;
}

ScriptModel::ScriptModel(DataClass* data)
    : AbstractModel(data)
{
    m_complete = false;
    m_pre_input = { ModelName_Json, InputSize_Json, GlobalParameterSize_Json, LocalParameterSize_Json };
}

ScriptModel::ScriptModel(DataClass* data, const QJsonObject& model)
    : AbstractModel(data)
{
    m_pre_input = { ModelName_Json, InputSize_Json, GlobalParameterSize_Json, LocalParameterSize_Json };
    m_complete = DefineModel(model);
}

ScriptModel::ScriptModel(AbstractModel* data)
    : AbstractModel(data)
{
    m_pre_input = { ModelName_Json, InputSize_Json, GlobalParameterSize_Json, LocalParameterSize_Json };
    m_complete = false;
}

ScriptModel::~ScriptModel()
{
    if (m_threads.size())
        delete m_thread_pool;
}

void ScriptModel::InitialThreads()
{
    if (m_threads.size())
        return;

    setThreads(8);
    int use_threads = Threads();
    // while(use_threads > DataPoints())
    //     use_threads /= 2;
    m_thread_pool = new CxxThreadPool;
    m_thread_pool->setProgressBar(CxxThreadPool::ProgressBarType::None);
    for (int i = 0; i <= use_threads; ++i) {
        CalculateThread* thread = new CalculateThread(DataPoints(), SeriesCount(), IndependentModel(), GlobalParameter(), LocalParameter(), m_input_names, m_global_parameter_names, m_local_parameter_names, m_chai_execute);
        thread->setRange(DataPoints() / use_threads * i, DataPoints() / use_threads * (i + 1));
        m_thread_pool->addThread(thread);
        m_threads << thread;
    }
    m_thread_pool->setActiveThreadCount(use_threads);
}

bool ScriptModel::DefineModel(const QJsonObject& model)
{
    QJsonObject parse = model;
    if (parse.contains("ModelDefinition"))
        parse = model["ModelDefinition"].toObject();
    if (parse.contains("GlobalParameterSize"))
        m_global_parameter_size = parse["GlobalParameterSize"].toInt();
    else
        m_global_parameter_size = 1;

    if (parse.contains("GlobalParameterNames"))
        m_global_parameter_names = parse["GlobalParameterNames"].toString().split("|");
    else {
        for (int i = 0; i < m_global_parameter_size; ++i)
            m_global_parameter_names << QString("A%1").arg(i + 1);
    }
    if (parse.contains("LocalParameterSize"))
        m_local_parameter_size = parse["LocalParameterSize"].toInt();
    else
        m_local_parameter_size = 0;

    if (parse.contains("LocalParameterNames"))
        m_local_parameter_names = parse["LocalParameterNames"].toString().split("|");

    if (parse.contains("InputSize"))
        m_input_size = parse["InputSize"].toInt();
    else
        m_input_size = 1;

    if (parse.contains("Python")) {
        QJsonObject exec = parse["Python"].toObject();
        for (int i = 0; i < exec.size(); ++i)
            m_execute_python << exec[QString::number(i + 1)].toString();
        m_python = true;
    }
    if (parse.contains("ChaiScript")) {
        // m_execute_chai.clear();
        QStringList strings;
        QJsonObject exec = parse["ChaiScript"].toObject();
        for (const QString& key : exec.keys())
            // for (int i = 0; i < exec.size(); ++i)
            strings << exec[key].toString();
        m_chai_execute = strings.join("\n");
        m_python = false;
        m_chai = true;
    }

    if (parse.contains("Duktape")) {
        QJsonObject exec = parse["Duktape"].toObject();
        for (int i = 0; i < exec.size(); ++i)
            m_execute_duktape << exec[QString::number(i + 1)].toString();
        m_python = false;
        m_chai = false;
        m_duktape = true;
    }
    m_chai = true;
    m_python = false;
    m_duktape = false;

    /*
    if (!m_python && !m_chai && !m_duktape) {
        emit Message("Nothing to do, lets just start it.", 1);
        return;
    }
    */
    m_name_cached = parse["Name"].toString();
    m_name = parse["Name"].toString();

    if (parse.contains("InputNames"))
        m_input_names = parse["InputNames"].toString().split("|");
    else
        for (int i = 0; i < m_input_size; ++i)
            m_input_names << QString("X%1").arg(i + 1);

    if (parse.contains("DepModelNames"))
        m_depmodel_names = parse["DepModelNames"].toString().split("|");
    else
        for (int i = 0; i < m_depmodel_names.size(); ++i)
            m_depmodel_names << QString("Y%1").arg(i + 1);

    m_model_definition = GenerateModelDefinition();
    try {
        PrepareParameter(GlobalParameterSize(), LocalParameterSize());

    } catch (int error) {
        if (error == -2) {
            emit Info()->Warning(tr("Parameter missmatch. I will not allow it!"));
            emit Info()->Message(tr("You have %1 independet rows available, yet you choose %2 parameters. If you want fewer parameters than rows, just don't include them in the equations.").arg(IndependentModel()->columnCount()).arg(InputParameterSize()));
            return false;
        }
    }

    for (int i = 0; i < m_input_names.size(); ++i)
        IndependentModel()->setHeaderData(i, Qt::Horizontal, m_input_names[i], Qt::DisplayRole);

    for (int i = 0; i < m_depmodel_names.size(); ++i)
        DependentModel()->setHeaderData(i, Qt::Horizontal, m_depmodel_names[i], Qt::DisplayRole);
#ifdef _Models
    m_interp.setInput(IndependentModel()->Table());
    m_interp.setGlobal(GlobalParameter()->Table(), m_global_parameter_names);
    m_interp.setLocal(LocalParameter()->Table());
    m_interp.setInputNames(m_input_names);
    // m_interp.setExecute(m_execute_chai);
    m_interp.InitialiseChai();
#endif

#ifdef Use_Duktape
    m_duktapeinterp.Initialise();
#endif
    return true;
    // InitialThreads();
}

QJsonObject ScriptModel::GenerateModelDefinition() const
{
    QJsonObject definition;

    definition["GlobalParameterSize"] = m_global_parameter_size;
    definition["GlobalParameterNames"] = m_global_parameter_names.join("|");
    definition["LocalParameterSize"] = m_local_parameter_size;
    definition["LocalParameterNames"] = m_local_parameter_names.join("|");
    definition["InputSize"] = m_input_size;
    definition["InputNames"] = m_input_names.join("|");
    definition["Name"] = m_name;
    definition["DepModelNames"] = m_depmodel_names.join("|");
    QJsonObject chai;
    QStringList lines = m_chai_execute.split("\n");
    for (int i = 0; i < lines.size(); ++i)
        chai[QString::number(i)] = lines[i];
    definition["ChaiScript"] = chai;
    return definition;
}

void ScriptModel::UpdateExecute(const QString &execute) {
  m_chai_execute = execute;
  QJsonObject json;
  QStringList lines = m_chai_execute.split("\n");
  for (int i = 0; i < lines.size(); ++i)
    json[QString::number(i)] = lines[i];
  m_model_definition["ChaiScript"] = json;
}

void ScriptModel::InitialGuess_Private()
{
    InitialiseRandom();
    /*
    QVector<qreal> x, y;

    for (int i = 1; i < DataPoints(); ++i) {
        x << 1 / IndependentModel()->data(0, i);
        y << 1 / DependentModel()->data(0, i);
    }

    PeakPick::LinearRegression regress = LeastSquares(x, y);
    double m_vmax = 1 / regress.n;
    double m_Km = regress.m * m_vmax;
    (*GlobalTable())[0] = m_vmax;
    (*GlobalTable())[1] = m_Km;
    Calculate();*/
}

void ScriptModel::CalculateVariables()
{
    //if (m_python)
    //    CalculatePython();
    //else if (m_chai)
    CalculateChai();
    // else if(m_duktape)
    //CalculateDuktape();
}

void ScriptModel::CalculatePython()
{
#ifdef _Python
    PyModelInterpreter interp;

    interp.setInput(IndependentModel()->Table());
    interp.setModel(ModelTable()->Table());
    interp.setGlobal(GlobalParameter()->Table(), m_global_parameter_names);
    interp.setLocal(LocalParameter()->Table());
    interp.setExecute(m_execute_python);
    interp.InitialisePython();

    for (int i = 0; i < DataPoints(); ++i) {
        for (int j = 0; j < SeriesCount(); ++j) {
            SetValue(i, j, interp.EvaluatePython(j, i));
        }
    }

    interp.FinalisePython();
#else
    emit Info()->Warning(QString("It looks like you open a Scripted Model. Ok, unfortunately SupraFit was compiled without Pyhton Script Support. Beside, the Python Models are not working yet."));
    m_complete = false;
#endif
}

void ScriptModel::CalculateChai()
{
#ifdef _Models
    /*
    setThreads(2);
    int use_threads = Threads();
    //while(use_threads > DataPoints())
    //    use_threads /= 2;
    QVector<CalculateThread *> threads;
    CxxThreadPool *pool = new CxxThreadPool;
    pool->setProgressBar(CxxThreadPool::ProgressBarType::None);
    for(int i = 0; i <= use_threads; ++i)
    {
        CalculateThread *thread = new CalculateThread(DataPoints(), SeriesCount(), IndependentModel(),  GlobalParameter(), LocalParameter(), m_input_names, m_global_parameter_names, m_local_parameter_names, m_chai_execute);
        thread->setRange(DataPoints()/use_threads*i, DataPoints()/use_threads*(i+1));
        pool->addThread(thread);
        threads << thread;
    }
    pool->setActiveThreadCount(use_threads);
    */
    /*
    for(int i = 0; i < m_threads.size(); ++i)
        m_threads[i]->UpdateParameter(GlobalParameter(), LocalParameter());
    m_thread_pool->Reset();
    m_thread_pool->StartAndWait();
    for(const CalculateThread *thread :m_threads)
    {
        if(!thread->isValid())
            continue;

        for(int i = thread->Start(); i < thread->End(); ++i)
        {
            for(int j = 0; j < SeriesCount(); ++j)
                SetValue(i, j, thread->Result()->data(j, i));
        }
    }
    */
    // delete pool;

    m_interp.setGlobal(GlobalParameter()->Table(), m_global_parameter_names);
    m_interp.setLocal(LocalParameter()->Table());
    m_interp.UpdateChai();
    // QString execute = m_execute_chai.join("\n");
    for (int series = 0; series < SeriesCount(); ++series) {
        for (int i = 0; i < DataPoints(); ++i) {
          QString cache = m_chai_execute;
          for (int parameter = 0; parameter < InputParameterSize();
               ++parameter) {
              cache.replace(
                  m_input_names[parameter],
                  QString::number(IndependentModel()->data(i, parameter)));
          }
          int error = 0;
          double result = 0;
          bool ok;
          double tmp_result = cache.toDouble(&ok);
          if (ok)
              result = tmp_result;
          else
              result = m_interp.Evaluate(cache.toUtf8(), error);
          if (error == 1) {
            cache.replace("var", "");
            result = m_interp.Evaluate(cache.toUtf8(), error);
          }
          SetValue(i, series, result);
        }
    }

#else
    emit Info()->Warning(QString("It looks like you open a Scripted Model. Ok, unfortranately SupraFit was compiled without Chai Script Support."));
    m_complete = false;
#endif
}

void ScriptModel::CalculateDuktape()
{

#ifdef Use_Duktape
QString execute = m_execute_chai.join("\n");
    std::vector<std::string> list;
    for (const QString& str : m_global_parameter_names)
        list.push_back(str.toStdString());

    m_duktapeinterp.setGlobal(GlobalParameter()->Table(), list);
    m_duktapeinterp.Update();
    for (int series = 0; series < SeriesCount(); ++series) {
        for (int i = 0; i < DataPoints(); ++i) {
            //QString calculate = QString("%1*%2/(%3+%2)").arg(km).arg(IndependentModel()->data(0, i)).arg(vmax);
            QString calculate = QString("vmax*%1/(Km+%1)").arg(IndependentModel()->data(0, i));

            // QString calculate = QString("%1*%3/(%2+%3)").arg(GlobalParameter()->Table()(0,0)).arg(GlobalParameter()->Table()(0,1)).arg(IndependentModel()->data(0, i));
            // qDebug() << calculate;
            QString t = m_execute_chai[0];
            QString cache = execute;
            cache.replace("S", QString::number((IndependentModel()->data(0, i))));
            QString result(m_duktapeinterp.Evaluate(cache.toUtf8()));
            //QString result(m_duktapeinterp.Evaluate(calculate.toUtf8()));
            double val = result.toDouble();
            SetValue(i, series, val); //row[i]);
        }
    }
    // m_duktapeinterp.Finalise();

#else
    emit Info()->Warning(QString("It looks like you open a Scripted Model. Ok, unfortranately SupraFit was compiled without Duktape Script Support."));
    m_complete = false;
#endif
}

QSharedPointer<AbstractModel> ScriptModel::Clone(bool statistics)
{
    QSharedPointer<ScriptModel> model = QSharedPointer<ScriptModel>(new ScriptModel(this), &QObject::deleteLater);
    model.data()->DefineModel(GenerateModelDefinition());
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

#include "scriptmodel.moc"

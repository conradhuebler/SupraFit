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

#pragma once

#include "CxxThreadPool.h"

#include "src/global.h"
#include "src/global_config.h"

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"

#include "src/core/models/chaiinterpreter.h"
#include "src/core/models/dukmodelinterpreter.h"
#include "src/core/models/exprtkinterpreter.h"

class CalculateThread : public CxxThread {
public:
    CalculateThread(int rows, int cols, DataTable* X, DataTable* Global, DataTable* Local, const QStringList& input_names, const QStringList& global_names, const QStringList& local_names, const QString& execute);
    virtual ~CalculateThread() override;

    virtual int execute() override;

    inline void setRange(int start, int end)
    {
        m_start = start;
        m_end = end;
        qDebug() << m_start << m_end;
    }

    int Start() const { return m_start; }
    int End() const { return m_end; }
    const DataTable* Result() const { return m_result; }
    void UpdateParameter(DataTable* Global, DataTable* Local);

    bool isValid() const { return m_valid; }

private:
    ChaiInterpreter m_chai;
    DataTable *m_X, *m_Y, *m_result;
    QStringList m_input_names, m_global_names, m_local_names;
    QString m_execute;

    int m_start = 0, m_end = 0, m_rows = 0, m_cols = 0;
    bool m_valid = true;
};

const QJsonObject ModelName_Json{
    { "name", "Name" }, // internal name, not to be printed
    { "title", "Model Name" }, // title, to be printed
    { "description", "Give the model a nice name" }, // brief description
    { "value", "Custom Model" }, // default value
    { "type", 3 } // 1 = int, 2 = double, 3 = string
};

const QJsonObject InputSize_Json{
    { "name", "InputSize" }, // internal name, not to be printed
    { "title", "Columns of Input" }, // title, to be printed
    { "description", "Set number of columns, which are used as independent variables" }, // brief description
    { "value", 1 }, // default value
    { "type", 1 }, // 1 = int, 2 = double, 3 = string
    { "once", true }
};

const QJsonObject GlobalParameterSize_Json{
    { "name", "GlobalParameterSize" }, // internal name, not to be printed
    { "title", "Number of global parameters" },
    { "description", "Set number of parameters to be fitted which act globally on several subsets of a data sets" },
    { "value", 1 },
    { "type", 1 }, // 1 = int, 2 = double, 3 = string
    { "once", true }
};

const QJsonObject GlobalParameterGuess_Json{
    { "name", "GlobalParameterGuess" }, // internal name, not to be printed
    { "title", "Set the initial guess of the global parameters" },
    { "description", "Set the initial guess of the global parameters as |-separated list of limits: [min;max]|[min;max]" },
    { "value", "" },
    { "type", 3 }, // 1 = int, 2 = double, 3 = string
    { "once", true }
};

const QJsonObject GlobalParameterNames_Json{
    { "name", "GlobalParameterNames" }, // internal name, not to be printed
    { "title", "Number of global parameters" },
    { "description", "Set number of parameters to be fitted which act globally on several subsets of a data sets" },
    { "value", "" },
    { "type", 3 }, // 1 = int, 2 = double, 3 = string
    { "once", true }
};

const QJsonObject LocalParameterSize_Json{
    { "name", "LocalParameterSize" }, // internal name, not to be printed
    { "title", "Number of local parameters" },
    { "description", "Set number of parameters to be fitted which act locally on a single subset of a data sets" },
    { "value", 1 },
    { "type", 1 }, // 1 = int, 2 = double, 3 = string
    { "once", true }
};

const QJsonObject LocalParameterGuess_Json{
    { "name", "LocalParameterGuess" }, // internal name, not to be printed
    { "title", "Set the initial guess of the local parameters" },
    { "description", "Set the initial guess of the local parameters as |-separated list of limits: [min;max]|[min;max]" },
    { "value", "" },
    { "type", 3 }, // 1 = int, 2 = double, 3 = string
    { "once", true }
};

const QJsonObject LocalParameterNames_Json{
    { "name", "LocalParameterNames" }, // internal name, not to be printed
    { "title", "Number of global parameters" },
    { "description", "Set number of parameters to be fitted which act globally on several subsets of a data sets" },
    { "value", "" },
    { "type", 3 }, // 1 = int, 2 = double, 3 = string
    { "once", true }
};

const QJsonObject PrintX_Json{
    { "name", "PrintX" }, // internal name, not to be printed
    { "title", "Print x values as" },
    { "description", "Define how the x / independet value has to be printed in the chart (e.g. = X1; 2*X1, X2/X1)" },
    { "value", "X1" },
    { "type", 3 } // 1 = int, 2 = double, 3 = string
};

const QJsonObject ChaiScript_Json{
    { "name", "ChaiScript" }, // internal name, not to be printed
    { "title", "Define model equation" },
    { "description", "The model has to be defined here using a working equation. The equation will be evaluated using the ChaiScript Interpreter." },
    { "value", "" },
    { "type", 4 } // 1 = int, 2 = double, 3 = string, 4 = text
};

class ScriptModel : public AbstractModel {
    Q_OBJECT

public:
    ScriptModel(DataClass* data);
    ScriptModel(DataClass* data, const QJsonObject& model);

    ScriptModel(AbstractModel* data);

    void InitialThreads();

    virtual ~ScriptModel() override;

    virtual inline SupraFit::Model SFModel() const override { return SupraFit::ScriptModel; }

    inline int GlobalParameterSize() const override { return m_global_parameter_size; }
    virtual void InitialGuess_Private() override;
    virtual QSharedPointer<AbstractModel> Clone(bool statistics = true) override;
    virtual bool SupportThreads() const override { return true; }
    virtual bool PreventThreads() const override { return false; }

    bool DefineModel() override;

    inline QString getExecute() const { return m_chai_execute; }

    virtual inline int InputParameterSize() const override { return m_input_size; }
    virtual inline QString GlobalParameterName(int i = 0) const override
    {
        if (i < m_global_parameter_names.size())
            return m_global_parameter_names[i];
        else
            return QString();
    }
    virtual inline int LocalParameterSize(int i = 0) const override { return m_local_parameter_size; }

    virtual qreal PrintOutIndependent(int i) const override;

    virtual inline bool SupportSeries() const override { return m_support_series; }

    /*! \brief Define the x axis label for charts
     */
    virtual QString XLabel() const override { return m_xlabel; }

    /*! \brief Define the y axis for charts
     */
    virtual QString YLabel() const override { return m_ylabel; }

    inline bool DemandInput() const override { return true; }
    void UpdateModelDefinition();

private:
    QVector<CalculateThread*> m_threads;
    CxxThreadPool* m_thread_pool;
    QString m_ylabel = QString(), m_xlabel = QString();
    int m_input_size = 0, m_global_parameter_size = 0, m_local_parameter_size = 0;
    bool m_support_series = false, m_chai = false, m_python = false, m_duktape = false;
    QStringList m_global_parameter_names, m_local_parameter_names, m_input_names, m_depmodel_names;
    QStringList m_execute_python, m_execute_chai, m_execute_duktape;
    QString m_chai_execute;
    QString m_calculate_print;
    QMultiMap<double, QString> m_global_names_map;
    mutable QVector<double> m_x_printout;
#ifdef _Models
    ChaiInterpreter m_interp;
#endif
    ExprTkInterpreter m_exprTk;
    bool m_formula_prepared = false;
#ifdef Use_Duktape
     DuktapeModelInterpreter m_duktapeinterp;
#endif

    void CalculateChai();
    void CalculateChaiV1();
    void CalculateChaiV2();

    void CalculateExprTk();

    void CalculatePython();
    void CalculateDuktape();
    void CalculateQJSEngine();

protected:
    virtual void CalculateVariables() override;
};

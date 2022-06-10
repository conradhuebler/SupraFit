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

    bool DefineModel(const QJsonObject& model) override;

    void UpdateExecute(const QString &execute);
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

    virtual qreal PrintOutIndependent(int i) const override
    {
        return IndependentModel()->data(i);
    }

    virtual inline bool SupportSeries() const override { return m_support_series; }

    /*! \brief Define the x axis label for charts
     */
    virtual QString XLabel() const override { return m_xlabel; }

    /*! \brief Define the y axis for charts
     */
    virtual QString YLabel() const override { return m_ylabel; }

    inline bool DemandInput() const override { return true; }

private:
    QVector<CalculateThread*> m_threads;
    CxxThreadPool* m_thread_pool;
    QString m_ylabel = QString(), m_xlabel = QString();
    int m_input_size = 0, m_global_parameter_size = 0, m_local_parameter_size = 0;
    bool m_support_series = false, m_chai = false, m_python = false, m_duktape = false;
    QStringList m_global_parameter_names, m_local_parameter_names, m_input_names, m_depmodel_names;
    QStringList m_execute_python, m_execute_chai, m_execute_duktape;
    QString m_chai_execute;
#ifdef _Models
    ChaiInterpreter m_interp;
#endif
#ifdef Use_Duktape
     DuktapeModelInterpreter m_duktapeinterp;
#endif

    void CalculateChai();
    void CalculatePython();
    void CalculateDuktape();

    QJsonObject GenerateModelDefinition() const;

protected:
    virtual void CalculateVariables() override;
};

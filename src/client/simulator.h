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
 */

#pragma once

#include "suprafit_cli.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "src/core/models.h"

class DataClass;

/**
 * @todo write docs
 */
class Simulator : public SupraFitCli {
    Q_OBJECT

public:
    explicit Simulator();
    virtual ~Simulator();

    void setMainJson(const QJsonObject& mainjson) { m_mainjson = mainjson; }
    void setTopLevel(const QJsonObject& toplevel) { m_toplevel = toplevel; }

    void setModelsJson(const QJsonObject& modelsjson) { m_modelsjson = modelsjson; }
    void setJobsJson(const QJsonObject& jobsjson) { m_jobsjson = jobsjson; }

    QStringList Generate();

    QVector<QSharedPointer<AbstractModel>> AddModels(const QJsonObject& modelsjson, QPointer<DataClass> data);

    bool FullTest();

    bool LoadFile();

private:
    QJsonObject m_toplevel;
    int m_runs;
    double m_std;

    inline QSharedPointer<AbstractModel> Test11Model(QPointer<DataClass> data)
    {
        if (data->IndependentVariableSize() == 1)
            return QSharedPointer<itc_ItoI_Model>(new itc_ItoI_Model(data), &QObject::deleteLater);
        else //(data->IndependentVariableSize() == 2)
            return QSharedPointer<ItoI_Model>(new ItoI_Model(data), &QObject::deleteLater);
    }
    inline QSharedPointer<AbstractModel> Test2111Model(QPointer<DataClass> data)
    {

        if (data->IndependentVariableSize() == 1)
            return QSharedPointer<itc_IItoI_Model>(new itc_IItoI_Model(data), &QObject::deleteLater);
        else //(data->IndependentVariableSize() == 2)
            return QSharedPointer<IItoI_ItoI_Model>(new IItoI_ItoI_Model(data), &QObject::deleteLater);
    }
    inline QSharedPointer<AbstractModel> Test1112Model(QPointer<DataClass> data)
    {
        if (data->IndependentVariableSize() == 1)
            return QSharedPointer<itc_ItoII_Model>(new itc_ItoII_Model(data), &QObject::deleteLater);
        else //(data->IndependentVariableSize() == 2)
            return QSharedPointer<ItoI_ItoII_Model>(new ItoI_ItoII_Model(data), &QObject::deleteLater);
    }

    inline QSharedPointer<AbstractModel> Test22Model(QPointer<DataClass> data)
    {
        if (data->IndependentVariableSize() == 1)
            return QSharedPointer<itc_IItoII_Model>(new itc_IItoII_Model(data), &QObject::deleteLater);
        else //(data->IndependentVariableSize() == 2)
            return QSharedPointer<IItoI_ItoI_ItoII_Model>(new IItoI_ItoI_ItoII_Model(data), &QObject::deleteLater);
    }

    void Test(QSharedPointer<AbstractModel> model);

    QPointer<const DataClass> m_data;


    void PrintStatistic(const QJsonObject& object, QSharedPointer<AbstractModel> model);

    void Progress(int i, int max);

    double m_current = 0.0;

    QJsonObject m_mainjson, m_modelsjson, m_jobsjson;
};

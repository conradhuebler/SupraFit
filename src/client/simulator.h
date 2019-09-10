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

    void setMainJson(const QJsonObject& mainjson)
    {
        m_mainjson = mainjson;
        setDataJson(m_mainjson["data"].toObject());
    }
    void setDataJson(const QJsonObject& datajson) { m_datajson = datajson; }

    void setTopLevel(const QJsonObject& toplevel) { m_toplevel = toplevel; }

    void setModelsJson(const QJsonObject& modelsjson) { m_modelsjson = modelsjson; }
    void setJobsJson(const QJsonObject& jobsjson) { m_jobsjson = jobsjson; }

    QVector<QJsonObject> GenerateData();

    QStringList Generate();

    QVector<QSharedPointer<AbstractModel>> AddModels(const QJsonObject& modelsjson, QPointer<DataClass> data);

private:
    QSharedPointer<AbstractModel> AddModel(int model, QPointer<DataClass> data);
    void CheckStopFile();

    QPointer<const DataClass> m_data;

    /*
    void Progress(int i, int max);
    double m_current = 0.0;
    */
    QJsonObject m_mainjson, m_modelsjson, m_jobsjson, m_datajson;
    bool m_interrupt = false;
signals:
    void Interrupt();
};

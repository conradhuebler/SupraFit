/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 - 2021 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/models.h"

class DataClass;

/**
 * @todo write docs
 */
class Simulator : public SupraFitCli {
    Q_OBJECT

public:
    Simulator();
    Simulator(SupraFitCli* client);
    virtual ~Simulator();


    bool setMainJson(const QJsonObject& mainjson)
    {
        m_main = mainjson;
        setDataJson(m_main["data"].toObject());
        m_extension = "." + m_main["Extension"].toString();
        return !m_data_json.isEmpty();
    }
    void setDataJson(const QJsonObject& datajson) { m_data_json = datajson; }

    bool setAnalyseJson(const QJsonObject& analyse)
    {
        m_analyse = analyse;
        return !m_analyse.isEmpty();
    }

    void setTopLevel(const QJsonObject& toplevel) { m_toplevel = toplevel; }

    bool setModelsJson(const QJsonObject& modelsjson)
    {
        m_models = modelsjson;
        return !m_models.isEmpty();
    }
    void setJobsJson(const QJsonObject& jobsjson) { m_jobs = jobsjson; }

    QVector<QJsonObject> GenerateData();

    // QStringList Generate();

    QJsonObject PerformeJobs(const QJsonObject& data, const QJsonObject& models, const QJsonObject& job);

private:
    void CheckStopFile();

    /*
    void Progress(int i, int max);
    double m_current = 0.0;
    */

};

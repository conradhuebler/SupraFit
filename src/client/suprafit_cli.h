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

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/models/models.h"

class SupraFitCli : public QObject {
    Q_OBJECT
public:
    explicit SupraFitCli();
    ~SupraFitCli();

    bool LoadFile();
    bool SaveFile();
    bool SaveFile(const QString& file, const QJsonObject& data);

    void PrintFileContent(int index = 0);
    void PrintFileStructure();

    void Analyse(const QJsonObject& analyse, const QVector<QJsonObject>& models = QVector<QJsonObject>());

    inline void setInFile(const QString& file)
    {
        m_infile = file;
        m_outfile = file;
    }
    inline void setOutFile(const QString& file)
    {
        m_outfile = file;
        m_outfile.contains(".json") ? m_extension = ".json" : m_extension = ".suprafit";
        m_outfile.remove(".json").remove(".suprafit");
    }
    QVector<QJsonObject> Data() const { return QVector<QJsonObject>() << m_toplevel; }

    void setDataJson(const QJsonObject& datajson) { m_datajson = datajson; }

    bool setMainJson(const QJsonObject& mainjson)
    {
        m_mainjson = mainjson;
        setDataJson(m_mainjson["data"].toObject());
        return !m_datajson.isEmpty();
    }

    inline void setPreparation(const QJsonObject& prepare) { m_prepare = prepare; }

    void OpenFile();
    QStringList ParseInput();
    bool Prepare();

signals:

public slots:

protected:
    QSharedPointer<AbstractModel> AddModel(int model, QPointer<DataClass> data);
    QVector<QSharedPointer<AbstractModel>> AddModels(const QJsonObject& modelsjson, QPointer<DataClass> data);

    QString m_infile = QString(), m_outfile = QString(), m_extension = ".suprafit";
    //QJsonObject m_toplevel;
    //QJsonObject m_mainjson,, m_datajson

    QJsonObject m_mainjson, m_toplevel, m_data_json, m_prepare, m_datajson, m_analysejson, m_jobsjson, m_modelsjson;

    int m_independent_rows = 2, m_start_point = 0;
    int m_series = 0;
    QPointer<const DataClass> m_data;
};

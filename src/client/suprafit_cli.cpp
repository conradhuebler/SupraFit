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

#include <iostream>

#include <QtCore/QFileInfo>
#include <QtCore/QJsonObject>

#include "src/core/models/dataclass.h"
#include "src/core/models/models.h"

#include "src/core/analyse.h"
#include "src/core/filehandler.h"
#include "src/core/jsonhandler.h"
#include "src/core/toolset.h"

#include "suprafit_cli.h"

SupraFitCli::SupraFitCli()
{
}

SupraFitCli::~SupraFitCli()
{
}

bool SupraFitCli::LoadFile()
{

    FileHandler* handler = new FileHandler(m_infile, this);
    handler->setIndependentRows(m_independent_rows);
    handler->setStartPoint(m_start_point);
    handler->setSeriesCount(m_series); // this only important for the subclassed simulator
    handler->LoadFile();
    if (handler->Type() == FileHandler::SupraFit) {
        if (!JsonHandler::ReadJsonFile(m_toplevel, m_infile))
            return false;
    } else if (handler->Type() == FileHandler::dH) {
        m_toplevel = handler->getJsonData();

    } else {
        m_toplevel = handler->getJsonData();
    }

    m_data_json = m_toplevel.value("data").toObject();
    return true;
}

bool SupraFitCli::SaveFile(const QString& file, const QJsonObject& data)
{
    if (JsonHandler::WriteJsonFile(data, file)) {
        std::cout << file.toStdString() << " successfully written to disk" << std::endl;
        return true;
    }
    return false;
}

bool SupraFitCli::SaveFile()
{
    if (JsonHandler::WriteJsonFile(m_toplevel, m_outfile)) {
        std::cout << m_outfile.toStdString() << " successfully written to disk" << std::endl;
        return true;
    }
    return false;
}

void SupraFitCli::PrintFileContent(int index)
{
    int i = 1;
    DataClass* data = new DataClass(m_toplevel["data"].toObject());

    if (data->DataPoints() == 0)
        return;

    std::cout << data->Data2Text().toStdString() << std::endl;

    for (const QString& key : m_toplevel.keys()) {
        if (key.contains("model")) {
            QSharedPointer<AbstractModel> model = JsonHandler::Json2Model(m_toplevel[key].toObject(), data);
            if (index == 0 || i == index) {
                std::cout << Print::Html2Raw(model->ModelInfo()).toStdString() << std::endl;
                std::cout << Print::Html2Raw(model->AnalyseStatistic()).toStdString() << std::endl;
            }
        }
        ++i;
    }
}

void SupraFitCli::PrintFileStructure()
{
    for (const QString& key : m_toplevel.keys()) {
        std::cout << key.toStdString() << std::endl;
    }
}

void SupraFitCli::Analyse(const QJsonObject& analyse, const QVector<QJsonObject>& models)
{
    QVector<QJsonObject> models_json;
    if (models.isEmpty()) {
        for (const QString& str : m_toplevel.keys()) {
            if (str.contains("model"))
                models_json << m_toplevel[str].toObject();
        }
    } else
        models_json = models;

    if (!analyse.isEmpty()) {
        for (const QString& key : analyse.keys()) {
            QJsonObject object = analyse[key].toObject();
            QString text;
            if (object["method"].toInt() == 1) {
                bool local = object["Local"].toBool(false);
                int index = object["Index"].toInt(1);
                text = StatisticTool::CompareMC(models_json, local, index);
            } else if (object["method"].toInt() == 4) {
                bool local = object["Local"].toBool(false);
                int CXO = object["CXO"].toInt(1);
                int X = object["X"].toInt(1);

                text = StatisticTool::CompareCV(models_json, CXO, local, X);
            } else if (object["method"].toInt() == 5) {
                bool local = object["Local"].toBool(false);
                double cutoff = object["CutOff"].toDouble(0);
                text = StatisticTool::AnalyseReductionAnalysis(models_json, local, cutoff);
            }
            std::cout << Print::Html2Raw(text).toStdString() << std::endl;
        }
    }
}

void SupraFitCli::OpenFile()
{
    LoadFile();
}

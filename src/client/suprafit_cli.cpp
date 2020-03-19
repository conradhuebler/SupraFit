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

#include "src/capabilities/montecarlostatistics.h"

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
    ParseInput();

    FileHandler* handler = new FileHandler(m_infile, this);
    handler->setIndependentRows(m_independent_rows);
    handler->setStartPoint(m_start_point);
    handler->setSeriesCount(m_series); // this only important for the subclassed simulator
    if (m_prepare.size()) {
        if (m_prepare.contains("Integration")) {
            QJsonObject integ = m_prepare["Integration"].toObject();
            integ["SupraFit"] = qint_version; // We have to add SupraFit
            handler->setThermogramParameter(integ);
        }
    }
    handler->LoadFile();
    if (handler->Type() == FileHandler::SupraFit) {
        if (!JsonHandler::ReadJsonFile(m_toplevel, m_infile))
            return false;
    } else if (handler->Type() == FileHandler::dH) {
        m_toplevel = handler->getJsonData();

    } else if (handler->Type() == FileHandler::ITC) {
        m_toplevel = handler->getJsonData();
    } else {
        m_toplevel = handler->getJsonData();
    }

    return true;
}

QStringList SupraFitCli::ParseInput()
{
    QStringList filelist;

    if (m_mainjson.isEmpty())
        return filelist;

    if (m_infile.isEmpty() || m_infile.isNull()) {
        if (m_mainjson.contains("InFile"))
            m_infile = m_mainjson["InFile"].toString();

        if (m_infile.isEmpty() || m_infile.isNull())
            return filelist;
    }

    if (m_mainjson.contains("IndependentRows"))
        m_independent_rows = m_mainjson["IndependentRows"].toInt(2);
    else if (m_mainjson.contains("InputSize"))
        m_independent_rows = m_mainjson["InputSize"].toInt(2);

    m_start_point = m_mainjson["StartPoint"].toInt(0);

    if (m_mainjson.contains("OutFile")) {
        m_outfile = m_mainjson["OutFile"].toString();
    }

    if (m_toplevel.isEmpty())
        return filelist;

    if (m_toplevel.keys().contains("data"))

        m_data = new DataClass(m_toplevel["data"].toObject());
    else
        m_data = new DataClass(m_toplevel);

    if (m_data->DataPoints() == 0) {
        QPointer<DataClass> data = new DataClass(m_data);

        QVector<QSharedPointer<AbstractModel>> models = AddModels(m_modelsjson, data);
        QJsonObject exp_level, dataObject;
        dataObject = data->ExportData();
        exp_level["data"] = dataObject;

        for (int i = 0; i < models.size(); ++i) {
            exp_level["model_" + QString::number(i)] = models[i]->ExportModel();
            models[i].clear();
        }
        SaveFile(m_outfile, exp_level);
        return filelist;
    }
    return filelist;
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
    if (JsonHandler::WriteJsonFile(m_toplevel, m_outfile + m_extension)) {
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


QVector<QSharedPointer<AbstractModel>> SupraFitCli::AddModels(const QJsonObject& modelsjson, QPointer<DataClass> data)
{
    QVector<QSharedPointer<AbstractModel>> models;
    for (const QString& str : modelsjson.keys()) {
        SupraFit::Model model;
        QJsonObject options;
        if (modelsjson[str].toObject().contains("model")) {
            model = static_cast<SupraFit::Model>(modelsjson[str].toObject()["model"].toInt());
            options = modelsjson[str].toObject()["options"].toObject();
        } else if (modelsjson[str].toObject().contains("ScriptModel")) {
            model = SupraFit::ScriptModel;
        } else
            model = static_cast<SupraFit::Model>(modelsjson[str].toInt());

        QSharedPointer<AbstractModel> t = CreateModel(model, data);
        if (model == SupraFit::ScriptModel)
            t->DefineModel(modelsjson[str].toObject());

        if (!t)
            continue;

        if (!options.isEmpty())
            t->setOptions(options);

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

QSharedPointer<AbstractModel> SupraFitCli::AddModel(int model, QPointer<DataClass> data)
{
    QSharedPointer<AbstractModel> t = CreateModel(model, data);
    if (!t)
        return t;
    return t;
}

bool SupraFitCli::Prepare()
{

    if (m_infile.isEmpty() || m_infile.isNull()) {
        if (m_mainjson.contains("InFile"))
            m_infile = m_mainjson["InFile"].toString();
    }

    if (m_mainjson.contains("OutFile")) {
        m_outfile = m_mainjson["OutFile"].toString();
    }

    LoadFile();
    m_extension = ".json";
    SaveFile();
    return true;
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

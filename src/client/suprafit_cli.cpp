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

#include "src/core/dataclass.h"
#include "src/core/filehandler.h"
#include "src/core/jsonhandler.h"
#include "src/core/models.h"
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

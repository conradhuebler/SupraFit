/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/dataclass.h"

#include <QtCore/QJsonObject>

#include <QJSEngine>

#include "datagenerator.h"

DataGenerator::DataGenerator(QObject* parent)
    : QObject{ parent }
{
}

bool DataGenerator::Evaluate()
{
    int independent = m_data["independent"].toInt();
    int datapoints = m_data["datapoints"].toInt();
    QStringList equations = m_data["equations"].toString().split("|");
    if (equations.size() != independent)
        return false;

    DataTable* table = new DataTable(independent, datapoints, this);

    QJSEngine myEngine;
    for (int indep = 0; indep < independent; ++indep) {
        QString equation = equations[indep];
        for (int datapoint = 0; datapoint < datapoints; ++datapoint) {
            QString tmp = equation;
            myEngine.globalObject().setProperty("X", datapoint + 1);
            QJSValue value = myEngine.evaluate(tmp);
            if (value.isNumber()) {
                table->data(indep, datapoint) = value.toNumber();
            }
        }
    }
    table->setHeader(equations);
    if (m_table)
        delete m_table;
    m_table = table;
    return true;
}

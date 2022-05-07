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

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QObject>

#include <QJSEngine>

#include "src/core/models/dataclass.h"

class DataGenerator : public QObject {
    Q_OBJECT
public:
    explicit DataGenerator(QObject* parent = nullptr);

    void setJson(const QJsonObject& data) { m_data = data; }
    bool Evaluate();

    DataTable* Table() const { return m_table; }

private:
    QJsonObject m_data;
    QPointer<DataTable> m_table;
signals:
};

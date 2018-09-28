/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/dataclass.h"

#include <QtCore/QMimeData>

class ModelMime : public QMimeData {

    Q_OBJECT

public:
    virtual ~ModelMime() {}

    inline void setDataPointer(DataClass* data) { m_data = data; }
    inline DataClass* DataPointer() const { return m_data; }

    inline void setModel(bool model) { m_model = model; }
    inline bool isModel() const { return m_model; }

    inline void setModelIndex(const QModelIndex& index) { m_index = index; }
    inline QModelIndex Index() const { return m_index; }

    inline void setInstance(const QString& str) { m_instance = str; }
    inline QString Instance() const { return m_instance; }

private:
    DataClass* m_data;
    bool m_model = false;
    QModelIndex m_index;
    QString m_instance;
};

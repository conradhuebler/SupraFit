/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QSharedPointer>

#include "src/core/models/models.h"

#include "src/global.h"

class AbstractModel;
class DataClass;
class QJsonObject;
class QString;
class JsonHandler {
public:
    JsonHandler() {}
    ~JsonHandler() {}
    static bool WriteJsonFile(const QJsonObject& json, const QString& file);
    static bool AppendJsonFile(const QJsonObject& json, const QString& filee);
    static bool ReadJsonFile(QJsonObject& json, const QString& file);

    static QJsonObject LoadFile(const QString& file);

    static QSharedPointer<AbstractModel> Json2Model(const QJsonObject& object, SupraFit::Model model, DataClass* data);
    static QSharedPointer<AbstractModel> Json2Model(const QJsonObject& object, DataClass* data);
};

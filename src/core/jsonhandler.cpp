/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "jsonhandler.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>


bool JsonHandler::ReadJsonFile(QJsonObject& json, const QString& file)
{
    QFile loadFile(file);
     if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QByteArray saveData = loadFile.readAll();

    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));

    json = loadDoc.object();
    return true;
}

bool JsonHandler::WriteJsonFile(const QJsonObject& json, const QString& file, bool binary)
{
    Q_UNUSED(binary)
    QFile saveFile(file);

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QJsonDocument saveDoc(json);
    saveFile.write( saveDoc.toJson()        );
    return true;
}

bool JsonHandler::AppendJsonFile(const QJsonObject& json, const QString& file, bool binary)
{
    Q_UNUSED(binary)
    QFile saveFile(file);

    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QJsonDocument saveDoc(json);
    saveFile.write( saveDoc.toJson()        );
    return true;
}

#include "jsonhandler.moc"

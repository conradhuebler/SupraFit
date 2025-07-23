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

#include <QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "src/core/models/models.h"

#include "src/global.h"

#include "jsonhandler.h"

QJsonObject JsonHandler::LoadFile(const QString& file)
{
    QJsonObject json;
    QFile loadFile(file);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open file!" << loadFile.errorString();
        return json;
    }

    QByteArray saveData = loadFile.readAll();

    QJsonDocument loadDoc;
    QJsonParseError error;
    if (file.contains("json"))
        loadDoc = QJsonDocument::fromJson(saveData, &error);
    else if (file.contains("jdat") || file.contains("suprafit"))
        loadDoc = QJsonDocument::fromJson(qUncompress(saveData), &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON Parse Error:" << error.errorString() << "at offset" << error.offset;
        return json;
    }
    
    json = loadDoc.object();
    qDebug() << "🔍 DEBUG JsonHandler: Loaded JSON with keys:" << json.keys();
    return json;
}

bool JsonHandler::ReadJsonFile(QJsonObject& json, const QString& file)
{
    // Use unified LoadFile method for consistency
    json = LoadFile(file);
    return !json.isEmpty();
}

bool JsonHandler::WriteJsonFile(const QJsonObject& json, const QString& file)
{
    QString filename = file;
    if (!file.contains("json") && !file.contains("jdat") && !file.contains("suprafit"))
        filename += ".suprafit";

    QFile saveFile(filename);

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open file .");
        return false;
    }

    QJsonDocument saveDoc(json);
    if (filename.contains("json"))
        saveFile.write(saveDoc.toJson());
    else if (filename.contains("jdat") || filename.contains("suprafit"))
        saveFile.write(qCompress(saveDoc.toJson(QJsonDocument::Compact), 9));

    return true;
}

bool JsonHandler::AppendJsonFile(const QJsonObject& json, const QString& file)
{
    QFile saveFile(file);

    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qWarning("Couldn't open file.");
        return false;
    }

    QJsonDocument saveDoc(json);
    if (file.contains("json"))
        saveFile.write(saveDoc.toJson());
    else if (file.contains("jdat") || file.contains("suprafit"))
        saveFile.write(qCompress(saveDoc.toJson(QJsonDocument::Compact), 9));
    return true;
}

QSharedPointer<AbstractModel> JsonHandler::Json2Model(const QJsonObject& object, SupraFit::Model model, DataClass* data)
{
    if (object.isEmpty())
        return NULL;

    QSharedPointer<AbstractModel> t = CreateModel(model, data);
    if (!t->LegacyImportModel(object)) {
        t.clear();
        return NULL;
    }
    return t;
}

QSharedPointer<AbstractModel> JsonHandler::Json2Model(const QJsonObject& object, DataClass* data)
{
    if (object.contains("SupraFit"))
        return Json2Model(object, static_cast<SupraFit::Model>(object["model"].toInt()), data);
    else
        return Json2Model(object, Name2Model(object["model"].toString()), data);
}

#include "jsonhandler.moc"

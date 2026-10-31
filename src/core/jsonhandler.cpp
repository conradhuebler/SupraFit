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

#include <algorithm>

#include "jsonhandler.h"

/*! \brief Reads a project or model file and says why it could not (Claude Generated).
 *
 * The format is taken from the content, not from the name: plain JSON starts with `{` (after blanks), everything else is taken as
 * `qCompress`ed JSON (the `.suprafit` / `.jdat` files). A file saved under another name, or in a directory with "json" in its name,
 * is read all the same. \p error gets the reason in words (cannot open, empty, not compressed JSON, JSON error with line and column).
 */
bool JsonHandler::ReadFile(const QString& file, QJsonObject& json, QString* error, QString* format)
{
    auto fail = [&](const QString& reason) {
        if (error)
            *error = reason;
        return false;
    };
    json = QJsonObject();
    QFile loadFile(file);
    if (!loadFile.exists())
        return fail(QObject::tr("The file does not exist."));
    if (!loadFile.open(QIODevice::ReadOnly))
        return fail(QObject::tr("The file cannot be opened: %1").arg(loadFile.errorString()));

    const QByteArray saveData = loadFile.readAll();
    if (saveData.trimmed().isEmpty())
        return fail(QObject::tr("The file is empty."));

    QByteArray text = saveData;
    QString kind = QStringLiteral("json");
    if (saveData.trimmed().at(0) != '{' && saveData.trimmed().at(0) != '[') {
        text = qUncompress(saveData);
        kind = QStringLiteral("compressed");
        if (text.isEmpty())
            return fail(QObject::tr("The file is neither JSON nor compressed JSON (a SupraFit project file is one of the two); it may be damaged or cut off."));
    }
    if (format)
        *format = kind;

    QJsonParseError parse;
    const QJsonDocument doc = QJsonDocument::fromJson(text, &parse);
    if (parse.error != QJsonParseError::NoError) {
        const int offset = std::min<int>(parse.offset, text.size());
        const int line = text.left(offset).count('\n') + 1;
        const int column = offset - text.left(offset).lastIndexOf('\n');
        return fail(QObject::tr("The JSON is damaged: %1 (line %2, column %3).").arg(parse.errorString()).arg(line).arg(column));
    }
    if (!doc.isObject())
        return fail(QObject::tr("The file holds a JSON %1, a SupraFit file holds an object.").arg(doc.isArray() ? QObject::tr("array") : QObject::tr("value")));
    json = doc.object();
    if (json.isEmpty())
        return fail(QObject::tr("The file holds an empty JSON object."));
    return true;
}

QJsonObject JsonHandler::LoadFile(const QString& file)
{
    QJsonObject json;
    QString error;
    if (!ReadFile(file, json, &error))
        qWarning() << "Cannot read" << file << ":" << error;
    return json;
}

bool JsonHandler::ReadJsonFile(QJsonObject& json, const QString& file)
{
    return ReadFile(file, json);
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

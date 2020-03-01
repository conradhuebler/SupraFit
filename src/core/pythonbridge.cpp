/*
 * Python bridge to SupraFit
 * Copyright (C) 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <cstdlib>

#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QString>

#include "filehandler.h"
#include "models/dataclass.h"
#include "models/models.h"
#include "toolset.h"

#include "pythonbridge.h"

CharPtrWrapper* LoadFile(const char* chars)
{
    QString file(chars);
    FileHandler* filehandler = new FileHandler(file);
    filehandler->LoadFile();
    QJsonDocument saveDoc(filehandler->getJsonData());

    CharPtrWrapper* ret;
    ret = (CharPtrWrapper*)malloc(sizeof(CharPtrWrapper));

    QString string = saveDoc.toJson(QJsonDocument::Compact);
    ret->len = string.size();
    ret->data = (char*)malloc(string.size() * sizeof(char));

    for (int i = 0; i < ret->len; ++i)
        ret->data[i] = string[i].toLatin1();
    //std::cout << ret->data << std::endl;
    return ret;
}

void Release(CharPtrWrapper* pWrap)
{
    if (pWrap) {
        free(pWrap->data);
        pWrap->data = NULL;
        pWrap->len = 0;
        free(pWrap);
    }
}

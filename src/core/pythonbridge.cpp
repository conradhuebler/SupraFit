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

#include <QtCore/QDebug>
#include <QtCore/QString>

#include "filehandler.h"
#include "models/dataclass.h"
#include "models/models.h"

#include "pythonbridge.h"

void LoadFile(const char* c)
{
    QString file(c);
    qDebug() << file;
    FileHandler* filehandler = new FileHandler(file);
    filehandler->LoadFile();
    qDebug() << filehandler->getData()->ExportTable(true);
}

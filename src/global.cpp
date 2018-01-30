/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QCoreApplication>

#include <QtCore/QFileInfo>
#include <QtCore/QVariant>
#include <QtCore/QString>


#include <iostream>

#include "src/global_config.h"
#include "src/global.h"

QString getDir()
{
    int dirlevel = qApp->instance()->property("dirlevel").toInt();
    QString lastdir = qApp->instance()->property("lastdir").toString();
    QString workingdir = qApp->instance()->property("workingdir").toString();
    if(dirlevel == 0)
        return QString(".");
    else if(dirlevel == 1)
        return lastdir;
    else if(dirlevel == 2)
        return workingdir;
    return QString(".");
}


void setLastDir(const QString &str)
{
    QFileInfo info(str);
    qApp->instance()->setProperty("lastdir", info.absolutePath());
}

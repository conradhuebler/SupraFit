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

#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include <iostream>

#include "src/global.h"
#include "src/global_config.h"

QString getDir()
{
    int dirlevel = qApp->instance()->property("dirlevel").toInt();
    QString lastdir = qApp->instance()->property("lastdir").toString();
    QString workingdir = qApp->instance()->property("workingdir").toString();
    if (dirlevel == 0)
        return QString(".");
    else if (dirlevel == 1)
        return lastdir;
    else if (dirlevel == 2)
        return workingdir;
    return QString(".");
}

void setLastDir(const QString& str)
{
    qDebug() << str << str.contains("|||");
    QFileInfo info(str);
    QString new_path = str;
    bool add_path = false;
    if (str.contains("|||")) {
        QStringList path = str.split("|||");
        if (path.size() == 2) {
            qDebug() << path << str;
            qApp->instance()->setProperty("lastdir", path[0]);
            new_path = str;
            add_path = true;
        }
    } else {
        if (info.isFile())
            qApp->instance()->setProperty("lastdir", info.absolutePath());
        else
            qApp->instance()->setProperty("lastdir", str);
    }
    QStringList recent = qApp->instance()->property("recent").toStringList();

    if (info.completeSuffix().contains(("suprafit")) || info.completeSuffix().contains(("json")) || info.completeSuffix().contains(("dH")) || info.completeSuffix().contains(("itc")) || info.completeSuffix().contains(("txt")) || info.completeSuffix().contains(("dat")) || info.isDir() || add_path) {
        recent.removeOne(new_path);
        recent.prepend(new_path);
    }

    if(recent.size() > 30)
        recent.removeLast();

    qApp->instance()->setProperty("recent", recent);
    qApp->instance()->setProperty("lastDir", getDir());
    QSettings _settings;
    _settings.beginGroup("main");

    _settings.setValue("recent", qApp->instance()->property("recent"));
    _settings.setValue("lastdir", qApp->instance()->property("lastdir"));
    _settings.endGroup();
}

/*
 * <C++ Test application for python wrapping functions.>
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

#include "src/core/pythonbridge.h"

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>

#include <QCoreApplication>
#include <QtCore/QRandomGenerator>

#include <iostream>

int main(int argc, char** argv)
{
    QString filename = "test.suprafit";
    const char* c = filename.toStdString().c_str();
    auto ref = LoadFile(c);
    std::cout << ref->data << std::endl;
    Release(ref);
    return 0;
}

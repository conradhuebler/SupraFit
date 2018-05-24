/*
 * <one line to give the library's name and an idea of what it does.>
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

#include "ui/mainwindow/suprafitgui.h"

#include "src/global.h"
#include "src/global_config.h"
#include "src/version.h"

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>

#include <QtWidgets/QApplication>

int main(int argc, char** argv)
{

    qInstallMessageHandler(myMessageOutput);

    QApplication app(argc, argv);
    QCommandLineParser parser;

    Version(&app, &parser);
    parser.process(app);

    const QStringList args = parser.positionalArguments();

#ifdef _DEBUG
    qDebug() << "Debug output enabled, good fun!";
#endif

    SupraFitGui mainwindow;
    mainwindow.showMaximized();
    for (const QString& str : qAsConst(args))
        mainwindow.LoadFile(str);
    return app.exec();
}

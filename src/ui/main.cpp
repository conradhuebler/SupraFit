/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2016 - 2021 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "mainwindow/suprafitgui.h"

#include "src/global.h"
#include "src/global_config.h"
#include "src/version.h"

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>

#include <QtGui/QFontDatabase>

#include <QtWidgets/QApplication>

#ifndef _WIN32
#if __GNUC__
// Thanks to
// https://stackoverflow.com/questions/77005/how-to-automatically-generate-a-stacktrace-when-my-program-crashes
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void bt_handler(int sig)
{
    void* array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "SupraFit crashed. Although this is probably unintended, it happened anyway.\n Some kind of backtrace will be printed out!\n\n");
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    fprintf(stderr, "Good-By\n");
    exit(1);
}
#endif
#endif

int main(int argc, char** argv)
{
#ifndef _WIN32
#if __GNUC__
    signal(SIGSEGV, bt_handler);
    signal(SIGABRT, bt_handler);
#endif
#endif

    SupraFit::timer time;
    qInstallMessageHandler(myMessageOutput);

    QApplication app(argc, argv);
#ifdef _WIN32
    //QApplication::setStyle("Fusion");
#endif

    QCommandLineParser parser;

    Version(&app, &parser);

    parser.addPositionalArgument("input file", QCoreApplication::translate("main", "File to open."));
    parser.process(app);

    const QStringList args = parser.positionalArguments();

#ifdef _DEBUG
    qDebug() << "Debug output enabled, good fun!";
#endif

#ifdef noto_font
    QFontDatabase database;
    QStringList fontfamilies = database.families();
    int smbd = 0, med = 0, light = 0, regular = 0;
    if (!fontfamilies.contains("Noto Sans SemBd")) {
        smbd = QFontDatabase::addApplicationFont(":/fonts/NotoSans-SemiBold.ttf");
    }

    if (!fontfamilies.contains("Noto Sans Med")) {
        med = QFontDatabase::addApplicationFont(":/fonts/NotoSans-Medium.ttf");
    }

    if (!fontfamilies.contains("Noto Sans Light")) {
        light = QFontDatabase::addApplicationFont(":/fonts/NotoSans-Light.ttf");
    }

    if (!fontfamilies.contains("Noto Sans")) {
        regular = QFontDatabase::addApplicationFont(":/fonts/NotoSans-Regular.ttf");
    }

    QFont mainfont("Noto Sans");
    mainfont.setPointSize(10);
    mainfont.setWeight(QFont::Medium);
    app.setFont(mainfont);
#endif

    for (int index = 0; index < argc; ++index)
        std::cout << argv[index] << " ";
    std::cout << std::endl
              << std::endl
              << std::endl;

    SupraFitGui mainwindow;
    mainwindow.showMaximized();
    for (const QString& str : qAsConst(args))
        mainwindow.LoadFile(str);
    return app.exec();
}

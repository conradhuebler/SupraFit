/*
 * SupraFit - offscreen benchmark of opening project files in the GUI
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

/*
 * Opens every given project file (or every *.suprafit / *.json below a given directory) in a fresh
 * SupraFitGui, as File > Open does, and measures the wall time until the file is on screen: the
 * synchronous LoadFile(), the queued signals that build the project windows and model widgets, the
 * timers due right after (10 ms) and one rendering of the window. With SUPRAFIT_LOAD_TIMING=1 (set
 * here) the sections marked with LoadTiming::Scope are summed per file, and those of the three
 * slowest files are printed.
 *
 * Manual perf tool, not a ctest. Claude Generated.
 *
 * Usage: benchmark_projectload <file-or-dir> [...]
 */

#include <QtWidgets/QApplication>

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <algorithm>
#include <cstdio>

#include "src/core/loadtiming.h"
#include "src/core/projectmanager.h"
#include "src/ui/instance.h"
#include "src/ui/mainwindow/mainwindow.h"
#include "src/ui/mainwindow/suprafitgui.h"

namespace {

struct Result {
    QString file;
    double sync = 0, total = 0;
    int windows = 0, models = 0;
    QMap<QString, QPair<double, int>> sections;
};

} // namespace

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("SUPRAFIT_LOAD_TIMING", "1");
    QApplication app(argc, argv);
    QTemporaryDir settingsDir;
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, settingsDir.path());
    QCoreApplication::setOrganizationName(QStringLiteral("SupraFitLoadBenchmark"));
    QCoreApplication::setApplicationName(QStringLiteral("benchmark_projectload"));
    {
        QSettings settings;
        settings.beginGroup(QStringLiteral("main"));
        settings.setValue(QStringLiteral("advanced_ui"), false);
    }
    app.setProperty("threads", QThread::idealThreadCount());
    app.setProperty("lineWidth", 20);

    QStringList files;
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (QFileInfo(arg).isDir()) {
            QDirIterator it(arg, { QStringLiteral("*.suprafit"), QStringLiteral("*.json") }, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext())
                files << it.next();
        } else
            files << arg;
    }
    std::sort(files.begin(), files.end());
    if (files.isEmpty()) {
        std::fprintf(stderr, "usage: benchmark_projectload <file-or-dir> [...]\n");
        return 1;
    }

    // the load report of a file with problems is a modal dialog: closed at once, as the GUI tests do
    QTimer closer;
    closer.setInterval(5);
    QObject::connect(&closer, &QTimer::timeout, []() {
        if (QWidget* w = QApplication::activeModalWidget())
            w->close();
    });
    closer.start();

    QList<Result> results;
    for (const QString& file : std::as_const(files)) {
        Instance::setInstance(new Instance);
        SupraFit::ProjectManager::instance().clearAllProjects();
        auto* gui = new SupraFitGui;
        gui->resize(1500, 700);
        gui->show();
        QCoreApplication::processEvents();
        LoadTiming::Reset();

        Result r;
        r.file = file;
        QElapsedTimer timer;
        timer.start();
        gui->LoadFile(file);
        r.sync = timer.nsecsElapsed() / 1e6;
        // the end: what the load posted is done (three rounds of the event queue, the windows post their own events), the timers due
        // in the next 10 ms have run (the axes are fitted 1 ms after a repaint), and the window on screen is rendered once
        for (int round = 0; round < 3; ++round) {
            bool reached = false;
            QTimer::singleShot(0, [&reached]() { reached = true; });
            while (!reached)
                QCoreApplication::processEvents(QEventLoop::AllEvents);
        }
        QElapsedTimer settle;
        settle.start();
        while (settle.elapsed() < 10)
            QCoreApplication::processEvents(QEventLoop::AllEvents);
        gui->grab();
        r.total = timer.nsecsElapsed() / 1e6;
        r.sections = LoadTiming::Sections();
        for (MainWindow* w : gui->ProjectWindows()) {
            ++r.windows;
            r.models += w->ModelCount();
        }
        std::printf("%9.1f ms total %9.1f ms sync %3d windows %4d models  %s\n", r.total, r.sync, r.windows, r.models, qPrintable(QFileInfo(file).fileName()));
        std::fflush(stdout);
        results << r;
        delete gui;
        SupraFit::ProjectManager::instance().clearAllProjects();
        QCoreApplication::processEvents();
    }

    // the sections summed over all files: where the time of a whole corpus goes (Claude Generated)
    if (results.size() > 1) {
        QMap<QString, QPair<double, int>> all;
        double total = 0;
        for (const Result& r : std::as_const(results)) {
            total += r.total;
            for (auto it = r.sections.begin(); it != r.sections.end(); ++it) {
                all[it.key()].first += it.value().first;
                all[it.key()].second += it.value().second;
            }
        }
        std::printf("\n=== sections summed over %lld files (%.1f ms) ===\n", static_cast<long long>(results.size()), total);
        QList<QPair<QString, QPair<double, int>>> rows;
        for (auto it = all.begin(); it != all.end(); ++it)
            rows << qMakePair(it.key(), it.value());
        std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.second.first > b.second.first; });
        for (const auto& row : rows)
            std::printf("  %-36s %10.1f ms %7d calls\n", qPrintable(row.first), row.second.first, row.second.second);
    }

    std::sort(results.begin(), results.end(), [](const Result& a, const Result& b) { return a.total > b.total; });
    const int shown = std::min<int>(3, results.size());
    for (int k = 0; k < shown; ++k) {
        const Result& worst = results[k];
        std::printf("\n=== sections of %s (%.1f ms, %d windows, %d models) ===\n", qPrintable(QFileInfo(worst.file).fileName()), worst.total, worst.windows, worst.models);
        QList<QPair<QString, QPair<double, int>>> rows;
        for (auto it = worst.sections.begin(); it != worst.sections.end(); ++it)
            rows << qMakePair(it.key(), it.value());
        std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.second.first > b.second.first; });
        for (const auto& row : rows)
            std::printf("  %-36s %10.1f ms %7d calls\n", qPrintable(row.first), row.second.first, row.second.second);
    }
    std::fflush(stdout);
    return 0;
}

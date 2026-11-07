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
#include "src/ui/mainwindow/modeldataholder.h"
#include "src/ui/mainwindow/modelwidget.h"
#include "src/ui/guitools/sectionstack.h"
#include "src/core/models/AbstractModel.h"

#include <QtWidgets/QStackedWidget>
#include "src/ui/mainwindow/suprafitgui.h"

namespace {

struct Result {
    QString file;
    double sync = 0, total = 0;
    int windows = 0, models = 0;
    QMap<QString, QPair<double, int>> sections;
};

/*! \brief Let what is posted run (three rounds of the queue) and the timers due in the next 10 ms. */
void Settle()
{
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
}

/*! \brief SUPRAFIT_LOAD_GRAB=<dir>: the tab widget of every project window with each of its tabs on screen, saved as PNG, so two
 * builds can be compared pixel by pixel. The tabs only: the log has times and ids, and the charts fit their axes on a timer.
 * Claude Generated. */
void GrabModelTabs(SupraFitGui* gui, const QString& dir, const QString& file)
{
    // the windows come in the order of their (random) project ids: sorted by what they show, the names are the same in every run
    QList<QPair<QString, MainWindow*>> windows;
    for (MainWindow* window : gui->ProjectWindows()) {
        QString key = window->windowTitle();
        for (TabWidget* tabs : window->findChildren<TabWidget*>())
            for (int i = 0; i < tabs->count(); ++i)
                key += QLatin1Char('|') + tabs->widget(i)->metaObject()->className() + tabs->tabText(i) + tabs->tabToolTip(i);
        for (ModelWidget* model : window->findChildren<ModelWidget*>())
            key += QLatin1Char('|') + model->Model()->Name() + QString::number(model->Model()->SSE(), 'g', 12);
        windows << qMakePair(key, window);
    }
    std::stable_sort(windows.begin(), windows.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    int w = 0;
    for (const auto& entry : std::as_const(windows)) {
        MainWindow* window = entry.second;
        if (auto* stack = qobject_cast<QStackedWidget*>(window->parentWidget()))
            stack->setCurrentWidget(window);
        Settle();
        int t = 0;
        for (TabWidget* tabs : window->findChildren<TabWidget*>()) {
            for (int i = 0; i < tabs->count(); ++i) {
                tabs->setCurrentIndex(i);
                Settle();
                tabs->grab().save(QStringLiteral("%1/%2_w%3_t%4_%5.png").arg(dir, file).arg(w).arg(t).arg(i));
                // SUPRAFIT_LOAD_GRAB_PAGES=1: every page of the model pane as well (Statistics, Detailed, Charts, ...)
                SectionStack* pages = nullptr; // no Q_OBJECT: found by dynamic_cast, the first one is the page list of the pane
                if (qobject_cast<ModelWidget*>(tabs->widget(i)))
                    for (QWidget* child : tabs->widget(i)->findChildren<QWidget*>())
                        if ((pages = dynamic_cast<SectionStack*>(child)))
                            break;
                if (pages && qEnvironmentVariableIntValue("SUPRAFIT_LOAD_GRAB_PAGES") == 1) {
                    const int shown = pages->currentIndex();
                    for (int p = 0; p < pages->count(); ++p) {
                        pages->setCurrentIndex(p);
                        Settle();
                        tabs->grab().save(QStringLiteral("%1/%2_w%3_t%4_%5_p%6.png").arg(dir, file).arg(w).arg(t).arg(i).arg(p));
                    }
                    pages->setCurrentIndex(shown);
                    Settle();
                }
            }
            ++t;
        }
        ++w;
    }
}

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
        // SUPRAFIT_LOAD_APPEARANCE=<0 system, 1 light, 2 dark>: compare the grabs in either colour scheme
        if (qEnvironmentVariableIsSet("SUPRAFIT_LOAD_APPEARANCE"))
            settings.setValue(QStringLiteral("appearance"), qEnvironmentVariableIntValue("SUPRAFIT_LOAD_APPEARANCE"));
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

    const QString grabDir = qEnvironmentVariable("SUPRAFIT_LOAD_GRAB");
    if (!grabDir.isEmpty())
        QDir().mkpath(grabDir);

    QList<Result> results;
    for (const QString& file : std::as_const(files)) {
        Instance::setInstance(new Instance);
        SupraFit::ProjectManager::instance().clearAllProjects();
        auto* gui = new SupraFitGui;
        if (!grabDir.isEmpty())
            gui->setLoadProgressDelay(1 << 30); // the progress in the status bar takes height: a slower run would be laid out otherwise
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
        if (!grabDir.isEmpty())
            GrabModelTabs(gui, grabDir, QFileInfo(file).fileName());
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

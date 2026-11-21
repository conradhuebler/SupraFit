/*
 * SupraFit - offscreen timing of the NMR import dialog with a real JEOL series
 * Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 * Manual perf tool, not a ctest (Claude Generated). Loads the JEOL series T5 (or T1) of
 * SUPRAFIT_JEOL_DIR (default: the operator's titration directory; the files are only read) into
 * NmrImport, shown at 1366x768 on the offscreen platform, and drives the steps of a user through the
 * controls: referencing to CHCl3 7.26, Start/End, "Pick from reference spectrum" with both detection
 * thresholds, a double-click, stack offset, wheel, signals, "Shift from", search window, LB. Every
 * action is timed until the event queue is empty (the paint included), plus the parts of a redraw
 * (track, processed spectra, setTraces). Symptoms are printed as counts (picked signals outside
 * Start..End, the stack after a wheel scaling).
 *
 * Usage: benchmark_nmrimport [T5|T1] [repeats]
 */

#include <charts.h>

#include <QtCore/QCollator>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>

#include <fmt/core.h>

#include <functional>

#include "src/core/nmrhandler.h"
#include "src/ui/dialogs/nmrimport.h"
#include "src/ui/guitools/appearance.h"
#include "src/ui/guitools/importkeyguard.h"
#include "src/ui/widgets/traceeditorwidget.h"

namespace {
QStringList series(const QString& dir, const QString& tag)
{
    const QString pattern = tag == "T1" ? "sb-SB18_T1_b_gluco_*.jdf" : "sb-SB18_T5_CDCl3_ObDGlc_*.jdf";
    QStringList files = QDir(dir).entryList({ pattern }, QDir::Files);
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(files.begin(), files.end(), [&collator](const QString& a, const QString& b) { return collator.compare(a, b) < 0; });
    for (QString& f : files)
        f = QDir(dir).filePath(f);
    return files;
}

/// wall time of action until the event queue is empty [ms]
double timed(const std::function<void()>& action)
{
    QElapsedTimer timer;
    timer.start();
    action();
    QCoreApplication::processEvents();
    return timer.nsecsElapsed() / 1e6;
}

void report(const char* what, double ms, const QString& note = QString())
{
    fmt::print("{:<52} {:>9.1f} ms  {}\n", what, ms, note.toStdString());
}

void wheel(TraceEditorWidget* editor, int delta)
{
    QWidget* viewport = editor->chart()->privateView()->viewport();
    const QPointF at = viewport->rect().center();
    QWheelEvent event(at, viewport->mapToGlobal(at), QPoint(), QPoint(0, delta), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(viewport, &event);
}

/// signals of the handler outside [lo, hi]
int outside(const NmrHandler* handler, double lo, double hi)
{
    int count = 0;
    for (const NmrSignal& s : handler->signalList())
        count += s.position < lo || s.position > hi;
    return count;
}
} // namespace

int main(int argc, char** argv)
{
    setvbuf(stdout, nullptr, _IONBF, 0); // the lines also when a step crashes
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    QTemporaryDir settingsDir;
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, settingsDir.path());
    QCoreApplication::setOrganizationName(QStringLiteral("SupraFitNmrBenchmark"));
    QCoreApplication::setApplicationName(QStringLiteral("benchmark_nmrimport"));
    Appearance::Install();
    qApp->setProperty("StoreRawData", true);

    const QString tag = argc > 1 ? QString(argv[1]) : QStringLiteral("T5");
    const int repeats = argc > 2 ? std::max(1, atoi(argv[2])) : 1;
    const QString dir = qEnvironmentVariable("SUPRAFIT_JEOL_DIR", QStringLiteral("/home/conrad/TUBAF/SB18_Titrationen"));
    const QStringList files = series(dir, tag);
    if (files.isEmpty()) {
        fmt::print("no series {} in {}\n", tag.toStdString(), dir.toStdString());
        return 0;
    }

    for (int run = 0; run < repeats; ++run) {
        fmt::print("--- {} ({} files), run {}\n", tag.toStdString(), files.size(), run + 1);
        NmrImport dialog;
        dialog.keyGuard()->setConfirmHandler([](const QString&) { return true; });
        dialog.resize(1366, 768);
        dialog.show();
        QCoreApplication::processEvents();
        auto* editor = dialog.findChild<TraceEditorWidget*>();
        NmrHandler* handler = dialog.handler();
        auto* offset = dialog.findChild<QDoubleSpinBox*>("nmrReferenceOffset");
        auto* start = dialog.findChild<QDoubleSpinBox*>("traceRangeStart");
        auto* end = dialog.findChild<QDoubleSpinBox*>("traceRangeEnd");
        auto* scope = dialog.findChild<QComboBox*>("nmrThresholdScope");
        auto* pick = dialog.findChild<QPushButton*>("nmrPick");
        auto* stack = dialog.findChild<QDoubleSpinBox*>("traceStackOffset");
        auto* method = dialog.findChild<QComboBox*>("nmrShiftMethod");
        auto* window = dialog.findChild<QDoubleSpinBox*>("nmrSearchWindow");
        auto* lb = dialog.findChild<QDoubleSpinBox*>("nmrLineBroadening");
        auto* removeSignal = dialog.findChild<QPushButton*>("nmrRemoveSignal");
        if (!editor || !offset || !start || !end || !scope || !pick || !stack || !method || !window || !lb || !removeSignal) {
            fmt::print("controls not found\n");
            return 1;
        }

        const double loading = timed([&]() { dialog.addFiles(files); }); // arguments of report() are evaluated in any order
        report("load and transform", loading, QString("%1 points").arg(handler->spectrumCount() ? handler->spectra().at(0).size() : 0));
        const XYTrace chcl3 = handler->referencedSpectra().at(0).range(7.2, 7.3);
        Eigen::Index top = 0;
        chcl3.y().maxCoeff(&top);
        const double target = handler->referenceOffset() + 7.26 - chcl3.x()(top);
        report("reference offset (CHCl3 7.26)", timed([&]() { offset->setValue(target); }));
        report("Start 6.5", timed([&]() { start->setValue(6.5); }));
        report("End 9.5", timed([&]() { end->setValue(9.5); }));

        for (int s : { 0, 1 }) {
            scope->setCurrentIndex(s);
            QCoreApplication::processEvents();
            const double ms = timed([&]() { pick->click(); });
            report(s == 0 ? "Pick (whole spectrum)" : "Pick (search window)", ms,
                QString("%1 signals, %2 outside Start..End").arg(handler->signalList().size()).arg(outside(handler, start->value(), end->value())));
        }
        // Claude Generated: the grouped pick (multiplets, J <= 20 Hz), then back to one signal per peak
        if (auto* group = dialog.findChild<QCheckBox*>("nmrGroupMultiplets")) {
            group->setChecked(true);
            const double ms = timed([&]() { pick->click(); });
            report("Pick, grouped into multiplets", ms, QString("%1 multiplets").arg(handler->signalList().size()));
            group->setChecked(false);
            pick->click();
        }
        // a handful of signals for the following steps
        while (handler->signalList().size() > 6)
            handler->removeSignal(handler->signalList().size() - 1);
        report("refresh with 6 signals", timed([&]() { dialog.refresh(); }));
        const double click = timed([&]() { emit editor->chart()->pointDoubleClicked(QPointF(7.80, 0)); });
        report("double-click at 7.80 (adds a signal)", click, QString("signal at %1").arg(handler->signalList().last().position, 0, 'f', 4));
        report("remove signal", timed([&]() { dialog.findChild<QTableWidget*>("nmrSignals")->setCurrentCell(handler->signalList().size() - 1, 0); removeSignal->click(); }));
        report("stack offset 50 %", timed([&]() { stack->setValue(50); }));
        report("wheel one notch", timed([&]() { wheel(editor, 120); }));
        report("wheel one notch", timed([&]() { wheel(editor, 120); }));
        const double hundred = timed([&]() { stack->setValue(100); });
        report("stack offset 100 % after the wheel", hundred, QString("user view %1, level 1 at %2").arg(editor->hasUserView()).arg(editor->stackOffset(1).y()));
        report("search window 0.02", timed([&]() { window->setValue(0.02); }));
        report("Shift from: lineshape fit", timed([&]() { method->setCurrentIndex(1); }));
        report("Shift from: peak picking", timed([&]() { method->setCurrentIndex(0); }));
        report("LB 2 Hz (FID transform)", timed([&]() { lb->setValue(2.0); dialog.handler(); })); // handler() runs a debounced transform

        // the parts of a redraw
        report("  NmrHandler::track", timed([&]() { handler->track(); }));
        report("  NmrHandler::referencedSpectra", timed([&]() { (void)handler->referencedSpectra(); }));
        report("  NmrHandler::processedSpectra", timed([&]() { (void)handler->processedSpectra(); }));
        report("  TraceEditorWidget::setTraces", timed([&]() { editor->setTraces(handler->referencedSpectra()); }));
        {
            const QSignalBlocker block(editor); // the editor alone, without the series of the dialog
            report("  setTraces without the dialog's series", timed([&]() { editor->setTraces(handler->referencedSpectra()); }));
            report("  wheel without the dialog's series", timed([&]() { wheel(editor, 120); }));
        }
        report("  NmrImport::updateMarkers (chartUpdated)", timed([&]() { emit editor->chartUpdated(); }));
        report("  NmrImport::refresh", timed([&]() { dialog.refresh(); }));
        dialog.hide();
    }
    return 0;
}

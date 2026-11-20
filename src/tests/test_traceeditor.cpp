/*
 * SupraFit - offscreen smoke test of the TraceEditorWidget and the spectra dialog built on it
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
 * WP2 of roadmap/input_data_editing.md. Drives the widget through its API (no clicks): the edited
 * TraceOpList must give the expected JSON, invalid parameters must be refused with a message, the
 * range selection must work on a descending axis, and a stored spectra project must come back in
 * the dialog with its range and steps. Set SUPRAFIT_TRACE_RENDER_DIR to write PNG renders of the
 * dialog (light, dark, 1366x768). Claude Generated.
 */

#include <cmath>

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtGui/QPixmap>
#include <QtTest/QSignalSpy>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>

#include <charts.h>

#include "src/core/models/datatable.h"
#include "src/core/spectrahandler.h"
#include "src/ui/dialogs/spectraimport.h"
#include "src/ui/guitools/appearance.h"
#include "src/ui/widgets/spectrawidget.h"
#include "src/ui/widgets/traceeditorwidget.h"

namespace {
/// two bands per trace, growing with the step; x from 'from' to 'to' in n points
XYTrace band(double from, double to, int n, int step)
{
    Eigen::VectorXd x(n), y(n);
    for (int i = 0; i < n; ++i) {
        x(i) = from + (to - from) * i / double(n - 1);
        const double lo = std::min(from, to), span = std::abs(to - from);
        y(i) = (step + 1) * std::exp(-std::pow((x(i) - (lo + 0.35 * span)) / (0.04 * span), 2))
            + 0.25 * (9 - step) * std::exp(-std::pow((x(i) - (lo + 0.7 * span)) / (0.06 * span), 2)) + 0.02 * step;
    }
    return XYTrace(x, y, QString("step %1").arg(step));
}
} // namespace

class TestTraceEditor : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;

    /// x-y files of a UV-Vis titration, 300..700 nm in 2 nm steps
    QStringList writeSpectra(int count)
    {
        QStringList files;
        for (int s = 0; s < count; ++s) {
            const XYTrace t = band(300, 700, 201, s);
            const QString name = m_dir.filePath(QString("uv_%1.dat").arg(s, 2, 10, QChar('0')));
            QFile file(name);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return {};
            QTextStream stream(&file);
            for (Eigen::Index i = 0; i < t.size(); ++i)
                stream << QString::number(t.x()(i), 'g', 17) << " " << QString::number(t.y()(i) * 1000, 'g', 17) << "\n";
            files << name;
        }
        return files;
    }

    /// project block as DataClass::ExportData writes it for a spectra project
    QJsonObject spectraProject(int count, bool withConcentrations)
    {
        SpectraHandler handler;
        for (const QString& f : writeSpectra(count))
            handler.addSpectrum(f);
        handler.setXRange(350, 650);
        handler.setXValues({ 405, 510 });
        TraceOpList ops;
        ops.append(SmoothOp(7, 2));
        PickPeaksOp pick;
        pick.threshold = 0.2;
        ops.append(pick);
        handler.setOperations(ops);
        QJsonObject project;
        project["raw"] = handler.getSpectraData();
        if (withConcentrations) {
            DataTable table(count, 2, nullptr);
            for (int s = 0; s < count; ++s) {
                table.data(s, 0) = 1e-3;
                table.data(s, 1) = 2e-4 * s;
            }
            table.setHeader({ "host", "guest" });
            project["independent"] = table.ExportTable(true);
        }
        return project;
    }

    void render(QWidget* widget, const QString& name)
    {
        const QString dir = qEnvironmentVariable("SUPRAFIT_TRACE_RENDER_DIR");
        if (dir.isEmpty())
            return;
        QDir().mkpath(dir);
        widget->show();
        QCoreApplication::processEvents();
        QTest::qWait(50);
        QVERIFY(widget->grab().save(QDir(dir).filePath(name + ".png")));
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        qApp->setProperty("StoreRawData", true);
    }

    void editingGivesExpectedJson()
    {
        TraceEditorWidget editor;
        TraceSet set;
        for (int s = 0; s < 3; ++s)
            set.add(band(300, 700, 201, s));
        editor.setTraces(set);
        QCOMPARE(editor.rangeStart(), 300.0);
        QCOMPARE(editor.rangeEnd(), 700.0);
        QSignalSpy changed(&editor, &TraceEditorWidget::operationsChanged);

        QCOMPARE(editor.addOperation("Smooth"), 0);
        QVERIFY(editor.showProcessed());
        QVERIFY(editor.setOperationParameters(0, QJsonObject{ { "Window", 9 }, { "Order", 3 } }));
        // invalid: refused with the message of the core, list unchanged
        QVERIFY(!editor.setOperationParameters(0, QJsonObject{ { "Window", 4 } }));
        QVERIFY(editor.errorText().contains("odd"));
        QCOMPARE(editor.addOperation("Fourier"), -1);

        editor.setRange(400, 600);
        QCOMPARE(editor.addOperation("Range"), 1);
        QCOMPARE(editor.addOperation("Baseline"), 2);
        QVERIFY(editor.setOperationParameters(2, QJsonObject{ { "Excluded", QJsonArray{ QJsonArray{ 420, 460 }, QJsonArray{ 540, 600 } } } }));
        QCOMPARE(editor.addOperation("Normalise"), 3);
        QVERIFY(editor.setOperationParameters(3, QJsonObject{ { "Mode", "MinMax" }, { "RefStart", 430 }, { "RefEnd", 460 } }));
        QVERIFY(editor.setOperationParameters(3, QJsonObject{ { "RefStart", QJsonValue::Null }, { "RefEnd", QJsonValue::Null } }));
        QVERIFY(editor.setOperationEnabled(3, false));
        QVERIFY(editor.moveOperation(1, 0));
        QCOMPARE(editor.addOperation("PickPeaks"), 4);
        QVERIFY(editor.errorText().isEmpty());

        const QJsonObject expected = QJsonDocument::fromJson(R"({"Version": 1, "Operations": [
            {"Type": "Range", "XStart": 400, "XEnd": 600},
            {"Type": "Smooth", "Method": "SavitzkyGolay", "Window": 9, "Order": 3},
            {"Type": "Baseline", "Degree": 1, "Excluded": [[420, 460], [540, 600]]},
            {"Type": "Normalise", "Mode": "MinMax", "Enabled": false},
            {"Type": "PickPeaks", "Threshold": 0.1, "ThresholdMode": "Relative", "MinDistance": 0, "Integrate": false}]})")
                                         .object();
        QCOMPARE(editor.operations().toJson(), expected);
        QCOMPARE(changed.count(), 11); // every successful edit, no failed one

        // peaks of the processed traces: the band at 440 nm inside the range
        QCOMPARE(editor.peaks().size(), 3);
        for (const auto& peaks : editor.peaks()) {
            QVERIFY(!peaks.isEmpty());
            bool found = false;
            for (const TracePeak& p : peaks)
                found = found || std::abs(p.x - 440) < 2;
            QVERIFY(found);
        }

        QVERIFY(editor.removeOperation(4));
        QVERIFY(!editor.removeOperation(10));
        QCOMPARE(editor.operations().size(), 4);

        // a step that fails on the data is reported, not hidden
        QVERIFY(editor.setOperationParameters(1, QJsonObject{ { "Window", 301 }, { "Order", 2 } }));
        QVERIFY(editor.errorText().contains("Smooth"));
    }

    void rangeOnDescendingAxis()
    {
        TraceEditorWidget editor;
        TraceSet set;
        for (int s = 0; s < 2; ++s)
            set.add(band(10, 0, 501, s)); // ppm, high to low
        editor.setTraces(set);
        QVERIFY(editor.chart()->axisX());
        QVERIFY(editor.chart()->axisX()->isReverse());
        QCOMPARE(editor.rangeStart(), 0.0);
        QCOMPARE(editor.rangeEnd(), 10.0);
        QSignalSpy range(&editor, &TraceEditorWidget::rangeChanged);
        // a drag on the reversed axis delivers the x values in screen order, high before low
        emit editor.chart()->addRect(QPointF(7.5, 1), QPointF(2.25, 0));
        QCOMPARE(range.count(), 1);
        QCOMPARE(range.at(0).at(0).toDouble(), 2.25);
        QCOMPARE(range.at(0).at(1).toDouble(), 7.5);
        QCOMPARE(editor.rangeStart(), 2.25);
        editor.setRange(1, 3); // from the host: no signal
        QCOMPARE(range.count(), 1);
        QCOMPARE(editor.addOperation("Range"), 0);
        QCOMPARE(editor.operations().toJson()["Operations"].toArray()[0].toObject()["XEnd"].toDouble(), 3.0);
    }

    void spectraProjectIsRestored()
    {
        const QJsonObject project = spectraProject(4, true);
        SpectraWidget widget;
        widget.setData(project);
        const QJsonObject raw = widget.ProjectData();
        QCOMPARE(raw["XStart"].toDouble(), 350.0);
        QCOMPARE(raw["XEnd"].toDouble(), 650.0);
        QCOMPARE(raw["TraceOps"].toObject(), project["raw"].toObject()["TraceOps"].toObject());
        QCOMPARE(raw["XValues"].toString(), project["raw"].toObject()["XValues"].toString());
        auto* editor = widget.findChild<TraceEditorWidget*>();
        QVERIFY(editor);
        QCOMPARE(editor->rangeStart(), 350.0);
        QCOMPARE(editor->rangeEnd(), 650.0);
        QCOMPARE(editor->operations().size(), 2);
        QVERIFY(editor->showProcessed());
        // concentrations (2) + one column per x value
        QCOMPARE(DataTable(widget.InputTable()).columnCount(), 4);
        // an edit of the steps reaches the project block
        QVERIFY(editor->removeOperation(1));
        QCOMPARE(widget.ProjectData()["TraceOps"].toObject()["Operations"].toArray().size(), 1);
    }

    void projectDataWithoutConcentrations()
    {
        // without a concentration table the spectra block was empty before
        SpectraWidget widget;
        widget.setData(spectraProject(3, false));
        QVERIFY(widget.ProjectData().contains("XValues"));
        QCOMPARE(widget.ProjectData()["Data"].toObject()["Count"].toInt(), 3);
    }

    void projectWithoutRawDataReadsTheFiles()
    {
        qApp->setProperty("StoreRawData", false);
        QJsonObject project = spectraProject(3, true);
        qApp->setProperty("StoreRawData", true);
        QVERIFY(!project["raw"].toObject().contains("Data"));
        SpectraWidget widget;
        widget.setData(project);
        auto* editor = widget.findChild<TraceEditorWidget*>();
        QCOMPARE(editor->traces().size(), 3);
        QCOMPARE(editor->rangeEnd(), 650.0);
        QCOMPARE(editor->operations().size(), 2);
    }

    /*! \brief Viewer 2: the step "Blank region" in the editor: default range, label, text field, button, errors. */
    void blankStepInTheEditor()
    {
        TraceEditorWidget editor;
        TraceSet set;
        for (int s = 0; s < 3; ++s)
            set.add(band(300, 700, 201, s));
        editor.setTraces(set);
        // the whole range would blank everything: no range then
        QCOMPARE(editor.addOperation("Blank"), 0);
        QVERIFY(editor.operations().at(0)->toJson()["Ranges"].toArray().isEmpty());
        QVERIFY(editor.removeOperation(0));
        editor.setRange(420, 460);
        QCOMPARE(editor.addOperation("Blank"), 0);
        QCOMPARE(editor.operations().at(0)->toJson()["Ranges"].toArray(), (QJsonArray{ QJsonArray{ 420, 460 } }));
        QCOMPARE(TraceEditorWidget::operationLabel(editor.operations().at(0)), QString("Blank region 420 to 460"));
        QVERIFY(editor.errorText().isEmpty());
        auto* field = editor.findChild<QLineEdit*>("traceBlankRanges");
        QVERIFY(field);
        QCOMPARE(field->text(), QString("420 460"));
        field->setText("420 460; 620 600");
        emit field->editingFinished();
        QCOMPARE(editor.operations().at(0)->toJson()["Ranges"].toArray().size(), 2);
        QCOMPARE(TraceEditorWidget::operationLabel(editor.operations().at(0)), QString("Blank regions: 2"));
        field->setText("420 460 480");
        emit field->editingFinished();
        QVERIFY(editor.errorText().contains("two numbers"));
        QCOMPARE(editor.operations().at(0)->toJson()["Ranges"].toArray().size(), 2);
        editor.setRange(500, 510);
        auto* take = editor.findChild<QPushButton*>("traceBlankSelected");
        QVERIFY(take);
        take->click();
        QCOMPARE(editor.operations().at(0)->toJson()["Ranges"].toArray().size(), 3);
        QVERIFY(editor.errorText().isEmpty());
        // the processed traces are straight between the neighbours of 420..460
        QVERIFY(editor.setOperationParameters(0, QJsonObject{ { "Ranges", QJsonArray{ QJsonArray{ 0, 1000 } } } }));
        QVERIFY2(editor.errorText().contains("every point"), qPrintable(editor.errorText()));
    }

    /*! \brief Viewer 2: time to build the series of 15 spectra x 131072 points (no rendering: the widget is not
     * shown), to move them by a stack offset and to redraw after a change of the steps. Printed, not pinned
     * (depends on the machine and the build type). */
    void timingOfLargeSeries()
    {
        const int count = 15, n = 131072;
        TraceSet set;
        for (int s = 0; s < count; ++s)
            set.add(band(10, 0, n, s));
        TraceEditorWidget editor;
        QElapsedTimer timer;
        QList<double> build, stack, steps;
        for (int run = 0; run < 3; ++run) {
            TraceSet copy;
            for (int s = 0; s < count; ++s)
                copy.add(set.at(s)); // new ids: a full rebuild
            timer.start();
            editor.setTraces(copy);
            build << timer.nsecsElapsed() / 1e6;
            timer.start();
            editor.setStackPercent(10 + run);
            stack << timer.nsecsElapsed() / 1e6;
            timer.start();
            editor.setOperations(TraceOpList());
            steps << timer.nsecsElapsed() / 1e6;
            editor.setStackPercent(0);
        }
        QCOMPARE(editor.chart()->chart()->series().size(), count + 2);
#ifdef QT_NO_DEBUG
        const char* type = "release";
#else
        const char* type = "debug";
#endif
        qInfo("%d x %d points (%s build): setTraces %.0f / %.0f / %.0f ms, stack offset %.0f / %.0f / %.0f ms, redraw without steps %.0f / %.0f / %.0f ms",
            count, n, type, build[0], build[1], build[2], stack[0], stack[1], stack[2], steps[0], steps[1], steps[2]);
    }

    /*! \brief Viewer 2 renders (SUPRAFIT_TRACE_RENDER_DIR): the spectra dialog unstacked, stacked, with x offset and reversed. */
    void renderStack()
    {
        if (qEnvironmentVariable("SUPRAFIT_TRACE_RENDER_DIR").isEmpty())
            QSKIP("SUPRAFIT_TRACE_RENDER_DIR not set");
        const QJsonObject project = spectraProject(8, true);
        QSettings().remove("traceeditor/spectraimport");
        for (int mode : { int(Appearance::Light), int(Appearance::Dark) }) {
            Appearance::Apply(mode);
            const QString suffix = mode == Appearance::Light ? "_light" : "_dark";
            SpectraImport dialog;
            dialog.setData(project);
            dialog.resize(1366, 768);
            auto* editor = dialog.findChild<TraceEditorWidget*>();
            render(&dialog, "uv_stack_none" + suffix);
            editor->setStackPercent(30);
            render(&dialog, "uv_stack_y" + suffix);
            editor->setStackXPercent(3);
            render(&dialog, "uv_stack_xy" + suffix);
            editor->setStackReversed(true);
            render(&dialog, "uv_stack_reverse" + suffix);
            dialog.hide();
            QSettings().remove("traceeditor/spectraimport");
        }
        Appearance::Apply(Appearance::Light);
    }

    void renderDialog()
    {
        if (qEnvironmentVariable("SUPRAFIT_TRACE_RENDER_DIR").isEmpty())
            QSKIP("SUPRAFIT_TRACE_RENDER_DIR not set");
        const QJsonObject project = spectraProject(8, true);
        for (int mode : { int(Appearance::Light), int(Appearance::Dark) }) {
            Appearance::Apply(mode);
            SpectraImport dialog;
            dialog.setData(project);
            dialog.resize(1280, 800);
            render(&dialog, mode == Appearance::Light ? "spectra_light" : "spectra_dark");
            dialog.resize(1366, 768);
            render(&dialog, mode == Appearance::Light ? "spectra_1366x768" : "spectra_1366x768_dark");
            dialog.hide();
        }
        Appearance::Apply(Appearance::Light);
        // NMR-like descending axis in the bare widget
        TraceEditorWidget editor;
        TraceSet set;
        for (int s = 0; s < 4; ++s)
            set.add(band(10, 0, 801, s));
        editor.setTraces(set);
        editor.setRange(2.5, 4.5);
        editor.addOperation("PickPeaks");
        editor.resize(1000, 560);
        render(&editor, "traceeditor_ppm");
    }
};

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    QTemporaryDir settingsDir;
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, settingsDir.path());
    QCoreApplication::setOrganizationName(QStringLiteral("SupraFitTraceEditorTest"));
    QCoreApplication::setApplicationName(QStringLiteral("test_traceeditor"));
    Appearance::Install();
    TestTraceEditor test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_traceeditor.moc"

/*
 * SupraFit - offscreen smoke test of the NMR import dialog
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
 * WP3 of roadmap/input_data_editing.md. Drives the dialog through its API (no clicks): the result
 * table shows the tracked shifts with a colour per status, a manual value typed into a cell is taken,
 * the table is refused without concentrations or with missing cells, and a stored NMR project comes
 * back. Set SUPRAFIT_TRACE_RENDER_DIR to write PNG renders (light, dark, 1366x768). Claude Generated.
 */

#include <charts.h>

#include <cmath>

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>

#include "src/core/models/datatable.h"
#include "src/core/nmrhandler.h"
#include "src/ui/dialogs/nmrimport.h"
#include "src/ui/guitools/appearance.h"
#include "src/ui/widgets/traceeditorwidget.h"

namespace {
/// three Lorentz lines on a descending ppm axis moving with the step; the third vanishes from 'vanish' on;
/// with 'cross' a fourth line runs into the second at step 5
XYTrace nmrSpectrum(int step, int steps, int vanish = 1000, bool cross = false)
{
    const int n = 1501;
    Eigen::VectorXd x(n), y(n);
    auto fraction = [steps](int k) { return 1.0 - std::exp(-1.5 * k / double(steps - 1)); };
    const double f = fraction(step);
    const double pos[3] = { 7.60 - 0.30 * f, 7.10 + 0.12 * f, 6.95 - 0.15 * f };
    const double meet = 7.10 + 0.12 * fraction(5); // the fourth line meets the second at step 5
    for (int i = 0; i < n; ++i) {
        x(i) = 8.0 - 0.001 * i;
        double v = 0;
        for (int s = 0; s < 3; ++s) {
            if (s == 2 && step >= vanish)
                continue;
            const double u = (x(i) - pos[s]) / 0.005;
            v += (1.0 - 0.2 * s) / (1 + u * u);
        }
        if (cross) {
            const double u = (x(i) - (meet + 0.03 * (5 - step))) / 0.005;
            v += 0.7 / (1 + u * u);
        }
        y(i) = v + 0.002 * std::sin(37.0 * i);
    }
    return XYTrace(x, y, QString("step_%1.txt").arg(step));
}

/// WP4: H2 starts 0.7 FWHM above a static neighbour line G (7.093 ppm) and moves away; FWHM 0.01 ppm
XYTrace overlapSpectrum(int step, int steps)
{
    const int n = 1501;
    Eigen::VectorXd x(n), y(n);
    const double f = 1.0 - std::exp(-1.5 * step / double(steps - 1));
    const double pos[4] = { 7.60 - 0.30 * f, 7.10 + 0.12 * f, 6.95 - 0.15 * f, 7.093 };
    const double height[4] = { 1.0, 0.8, 0.6, 0.6 };
    for (int i = 0; i < n; ++i) {
        x(i) = 8.0 - 0.001 * i;
        double v = 0;
        for (int s = 0; s < 4; ++s) {
            const double u = (x(i) - pos[s]) / 0.005;
            v += height[s] * (0.5 / (1 + u * u) + 0.5 * std::exp(-std::log(2.0) * u * u));
        }
        y(i) = v + 0.002 * std::sin(37.0 * i);
    }
    return XYTrace(x, y, QString("step_%1.txt").arg(step));
}
} // namespace

class TestNmrImport : public QObject {
    Q_OBJECT

    static void fill(NmrHandler* handler, int steps, int vanish = 1000, bool cross = false)
    {
        for (int k = 0; k < steps; ++k)
            handler->addSpectrum(nmrSpectrum(k, steps, vanish, cross));
    }

    static void setOverlapSignals(NmrHandler* handler)
    {
        handler->addSignal(7.60, 0.08, "H1");
        handler->addSignal(7.10, 0.08, "H2");
        handler->addSignal(6.95, 0.08, "H3");
        handler->addSignal(7.093, 0.012, "G");
    }

    static DataTable* concentrations(int steps)
    {
        DataTable* table = new DataTable(steps, 2, nullptr);
        for (int k = 0; k < steps; ++k) {
            table->data(k, 0) = 1e-3;
            table->data(k, 1) = 5e-4 * k;
        }
        table->setHeader({ "Host", "Guest" });
        return table;
    }

    void render(QWidget* widget, const QString& name)
    {
        const QString dir = qEnvironmentVariable("SUPRAFIT_TRACE_RENDER_DIR");
        if (dir.isEmpty())
            return;
        QDir().mkpath(dir);
        widget->show();
        QCoreApplication::processEvents();
        QTest::qWait(80);
        QVERIFY(widget->grab().save(QDir(dir).filePath(name + ".png")));
    }

private slots:
    void initTestCase()
    {
        qApp->setProperty("StoreRawData", true);
    }

    void shiftAxisInTheEditor()
    {
        TraceEditorWidget editor;
        const int index = editor.addOperation("ShiftAxis");
        QCOMPARE(index, 0);
        QVERIFY(editor.setOperationParameters(0, QJsonObject{ { "Offset", -0.05 } }));
        QCOMPARE(editor.operations().at(0)->type(), QString("ShiftAxis"));
        QVERIFY(TraceEditorWidget::operationLabel(editor.operations().at(0)).contains("-0.05"));
        QVERIFY(!editor.setOperationParameters(0, QJsonObject{ { "Offset", "x" } }));
    }

    void trackedTableAndConcentrations()
    {
        const int steps = 10;
        NmrImport dialog;
        fill(dialog.handler(), steps);
        dialog.handler()->addSignal(7.60, 0.08, "H1");
        dialog.handler()->addSignal(7.10, 0.08, "H2");
        dialog.handler()->addSignal(6.95, 0.08, "H3");
        dialog.refresh();
        QTableWidget* results = dialog.findChild<QTableWidget*>("nmrResults");
        QVERIFY(results);
        QCOMPARE(results->rowCount(), steps);
        QCOMPARE(results->columnCount(), 3);
        for (int k = 0; k < steps; ++k)
            for (int s = 0; s < 3; ++s)
                QCOMPARE(dialog.handler()->cell(k, s).status, NmrCellStatus::Ok);
        QCOMPARE(results->item(4, 0)->background().style(), Qt::NoBrush);
        QVERIFY(std::abs(results->item(steps - 1, 0)->text().toDouble() - dialog.handler()->cell(steps - 1, 0).shift) < 1e-4);

        QString error;
        QVERIFY(!dialog.validate(&error));
        QVERIFY(error.contains("concentrations"));
        std::unique_ptr<DataTable> conc(concentrations(steps));
        dialog.setConcentrations(conc.get());
        QVERIFY2(dialog.validate(&error), qPrintable(error));
        QCOMPARE(dialog.IndependentCount(), 2);
        DataTable table(dialog.InputTable());
        QCOMPARE(table.columnCount(), 5);
        QCOMPARE(table.rowCount(), steps);
        QCOMPARE(table.header()[2], QString("δ H1 /ppm"));

        // a value typed into a cell is a manual shift
        results->item(3, 1)->setText("7,15");
        QCoreApplication::processEvents();
        QCOMPARE(dialog.handler()->cell(3, 1).status, NmrCellStatus::Manual);
        QCOMPARE(dialog.handler()->cell(3, 1).shift, 7.15);
        results = dialog.findChild<QTableWidget*>("nmrResults");
        QCOMPARE(results->item(3, 1)->background().color(), NmrImport::statusColor(NmrCellStatus::Manual));

        // the project comes back
        QJsonObject project;
        project["raw"] = dialog.ProjectData();
        project["independent"] = conc->ExportTable(true);
        NmrImport again;
        again.setData(project);
        QCOMPARE(again.handler()->signalList().size(), 3);
        QCOMPARE(again.handler()->cell(3, 1).status, NmrCellStatus::Manual);
        QVERIFY2(again.validate(&error), qPrintable(error));
        QCOMPARE(again.InputTable(), dialog.InputTable());
    }

    void missingAndConflictCellsAreRefused()
    {
        const int steps = 10;
        NmrImport dialog;
        fill(dialog.handler(), steps, 7, true);
        dialog.handler()->addSignal(7.60, 0.05, "H1");
        dialog.handler()->addSignal(7.10, 0.05, "H2");
        dialog.handler()->addSignal(6.95, 0.05, "H3");
        dialog.handler()->addSignal(7.10 + 0.12 * (1.0 - std::exp(-1.5 * 5 / 9.0)) + 0.15, 0.05, "X");
        std::unique_ptr<DataTable> conc(concentrations(steps));
        dialog.setConcentrations(conc.get());
        dialog.refresh();
        QTableWidget* results = dialog.findChild<QTableWidget*>("nmrResults");
        bool missing = false, conflict = false;
        for (int k = 0; k < steps; ++k)
            for (int c = 0; c < 4; ++c) {
                const QColor color = results->item(k, c)->background().color();
                missing = missing || (dialog.handler()->cell(k, c).status == NmrCellStatus::Missing && color == NmrImport::statusColor(NmrCellStatus::Missing));
                conflict = conflict || (dialog.handler()->cell(k, c).status == NmrCellStatus::Conflict && color == NmrImport::statusColor(NmrCellStatus::Conflict));
            }
        QVERIFY(missing);
        QVERIFY(conflict);
        QString error;
        QVERIFY(!dialog.validate(&error));
        QVERIFY2(error.contains("missing") && error.contains("conflict"), qPrintable(error));
        QVERIFY(dialog.InputTable().isEmpty());
    }

    /// WP4: the lineshape fit through the dialog controls; detail columns, fit curves, "fit failed" cells
    void lineshapeFitMode()
    {
        const int steps = 10;
        NmrImport dialog;
        for (int k = 0; k < steps; ++k)
            dialog.handler()->addSpectrum(overlapSpectrum(k, steps));
        setOverlapSignals(dialog.handler());
        dialog.refresh();
        QComboBox* method = dialog.findChild<QComboBox*>("nmrShiftMethod");
        QCheckBox* details = dialog.findChild<QCheckBox*>("nmrFitDetails");
        QVERIFY(method && details);
        QVERIFY(!details->isEnabled());
        // peak picking: H2 and G share one maximum in the first spectra
        QCOMPARE(dialog.handler()->cell(0, 1).status, NmrCellStatus::Conflict);
        method->setCurrentIndex(1);
        QCOMPARE(dialog.handler()->fitSettings().method, NmrShiftMethod::LineshapeFit);
        QVERIFY(details->isEnabled());
        for (int k = 0; k < steps; ++k)
            for (int s = 0; s < 4; ++s) {
                const NmrCell cell = dialog.handler()->cell(k, s);
                QVERIFY2(cell.status == NmrCellStatus::Ok, qPrintable(QString("%1 %2 %3 %4").arg(k).arg(s).arg(NmrHandler::statusName(cell.status), cell.fitMessage)));
                QCOMPARE(cell.shift, cell.fitShift);
            }
        // H2 follows its own line, also where it shares G's maximum or the tracking hands it G's peak
        for (int k = 0; k < steps; ++k) {
            const double h2 = 7.10 + 0.12 * (1.0 - std::exp(-1.5 * k / double(steps - 1)));
            QVERIFY2(std::abs(dialog.handler()->cell(k, 1).shift - h2) < 1e-3, qPrintable(QString("step %1: %2 / %3").arg(k).arg(dialog.handler()->cell(k, 1).shift, 0, 'f', 5).arg(h2, 0, 'f', 5)));
            QVERIFY(std::abs(dialog.handler()->cell(k, 3).shift - 7.093) < 1e-3);
        }
        // the fit separates H2 from G in the first spectrum
        QVERIFY2(std::abs(dialog.handler()->cell(0, 1).shift - 7.10) < 1e-3, qPrintable(QString::number(dialog.handler()->cell(0, 1).shift, 'f', 5)));
        QVERIFY2(std::abs(dialog.handler()->cell(0, 3).shift - 7.093) < 1e-3, qPrintable(QString::number(dialog.handler()->cell(0, 3).shift, 'f', 5)));
        QTableWidget* results = dialog.findChild<QTableWidget*>("nmrResults");
        QCOMPARE(results->columnCount(), 4);
        details->setChecked(true);
        QCOMPARE(results->columnCount(), 20);
        QCOMPARE(results->horizontalHeaderItem(2)->text(), QString("FWHM"));
        QVERIFY(std::abs(results->item(3, 2)->text().toDouble() - 0.01) < 0.001);
        QVERIFY(!(results->item(3, 1)->flags() & Qt::ItemIsEditable));
        // a value typed into a δ column is still a manual shift
        results->item(4, 5)->setText("7.2");
        QCoreApplication::processEvents();
        QCOMPARE(dialog.handler()->cell(4, 1).status, NmrCellStatus::Manual);
        std::unique_ptr<DataTable> conc(concentrations(steps));
        dialog.setConcentrations(conc.get());
        QString error;
        QVERIFY2(dialog.validate(&error), qPrintable(error));

        // failed fits: no shift, refused table, own colour
        QSpinBox* iterations = dialog.findChild<QSpinBox*>("nmrMaxIterations");
        iterations->setValue(1);
        results = dialog.findChild<QTableWidget*>("nmrResults");
        QCOMPARE(dialog.handler()->cell(2, 0).status, NmrCellStatus::FitFailed);
        QCOMPARE(results->item(2, 0)->background().color(), NmrImport::statusColor(NmrCellStatus::FitFailed));
        QVERIFY(!dialog.validate(&error));
        QVERIFY(error.contains("fit failed"));
        // with the peak position as fallback the single lines are back; H2/G sharing one maximum stay in conflict
        dialog.findChild<QCheckBox*>("nmrPeakFallback")->setChecked(true);
        QVERIFY(!dialog.validate(&error));
        QVERIFY2(error.contains("conflict") && !error.contains("fit failed"), qPrintable(error));
        QCOMPARE(dialog.handler()->cell(2, 0).status, NmrCellStatus::Ok);
        QCOMPARE(dialog.handler()->cell(2, 0).shift, dialog.handler()->cell(2, 0).peakShift);
        iterations->setValue(200);
        QVERIFY2(dialog.validate(&error), qPrintable(error));

        // the settings come back with the project
        QJsonObject project;
        project["raw"] = dialog.ProjectData();
        project["independent"] = conc->ExportTable(true);
        NmrImport again;
        again.setData(project);
        QVERIFY(again.handler()->fitSettings() == dialog.handler()->fitSettings());
        QCOMPARE(again.findChild<QComboBox*>("nmrShiftMethod")->currentIndex(), 1);
        QCOMPARE(again.findChild<QSpinBox*>("nmrMaxIterations")->value(), 200);
        QVERIFY(again.findChild<QCheckBox*>("nmrPeakFallback")->isChecked());
        QCOMPARE(again.InputTable(), dialog.InputTable());
    }

    void renderDialog()
    {
        if (qEnvironmentVariable("SUPRAFIT_TRACE_RENDER_DIR").isEmpty())
            QSKIP("SUPRAFIT_TRACE_RENDER_DIR not set");
        const int steps = 10;
        for (int mode : { int(Appearance::Light), int(Appearance::Dark) }) {
            Appearance::Apply(mode);
            const QString suffix = mode == Appearance::Light ? "_light" : "_dark";
            {
                NmrImport dialog;
                fill(dialog.handler(), steps);
                dialog.refresh();
                dialog.resize(1366, 768);
                render(&dialog, "nmr_before_tracking" + suffix);
                dialog.handler()->pickSignals(0.08);
                std::unique_ptr<DataTable> conc(concentrations(steps));
                dialog.setConcentrations(conc.get());
                dialog.refresh();
                render(&dialog, "nmr_tracked" + suffix);
                dialog.findChild<QTabWidget*>()->setCurrentIndex(1);
                render(&dialog, "nmr_table" + suffix);
                dialog.hide();
            }
            {
                NmrImport dialog;
                fill(dialog.handler(), steps, 7, true);
                dialog.handler()->addSignal(7.60, 0.05, "H1");
                dialog.handler()->addSignal(7.10, 0.05, "H2");
                dialog.handler()->addSignal(6.95, 0.05, "H3");
                dialog.handler()->addSignal(7.10 + 0.12 * (1.0 - std::exp(-1.5 * 5 / 9.0)) + 0.15, 0.05, "X");
                dialog.refresh();
                dialog.resize(1366, 768);
                render(&dialog, "nmr_missing_conflict" + suffix);
                dialog.hide();
            }
            {
                // WP4: fit of the overlapping pair, details, fit curve and residual of spectrum 1
                NmrImport dialog;
                for (int k = 0; k < steps; ++k)
                    dialog.handler()->addSpectrum(overlapSpectrum(k, steps));
                setOverlapSignals(dialog.handler());
                dialog.refresh();
                dialog.resize(1366, 768);
                dialog.findChild<QComboBox*>("nmrShiftMethod")->setCurrentIndex(1);
                dialog.findChild<QCheckBox*>("nmrFitDetails")->setChecked(true);
                render(&dialog, "nmr_fit" + suffix);
                // zoom on H2 and G: the fitted model over the two lines and the residual below
                dialog.findChild<TraceEditorWidget*>()->chart()->setXMin(7.04);
                dialog.findChild<TraceEditorWidget*>()->chart()->setXMax(7.16);
                render(&dialog, "nmr_fit_zoom" + suffix);
                dialog.findChild<QSpinBox*>("nmrMaxIterations")->setValue(2);
                render(&dialog, "nmr_fit_failed" + suffix);
                dialog.hide();
            }
        }
        Appearance::Apply(Appearance::Light);
    }
};

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    QTemporaryDir settingsDir;
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, settingsDir.path());
    QCoreApplication::setOrganizationName(QStringLiteral("SupraFitNmrImportTest"));
    QCoreApplication::setApplicationName(QStringLiteral("test_nmrimport"));
    Appearance::Install();
    TestNmrImport test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_nmrimport.moc"

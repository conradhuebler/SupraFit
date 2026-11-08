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

#include <cmath>

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
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
} // namespace

class TestNmrImport : public QObject {
    Q_OBJECT

    static void fill(NmrHandler* handler, int steps, int vanish = 1000, bool cross = false)
    {
        for (int k = 0; k < steps; ++k)
            handler->addSpectrum(nmrSpectrum(k, steps, vanish, cross));
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

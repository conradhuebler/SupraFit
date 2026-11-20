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
 * JEOL import: a synthetic .jdf series shows the panel "Processing (FID)", its controls reach NmrHandler and
 * come back with the project; with SUPRAFIT_JEOL_DIR (default: the operator's titration directory) the renders
 * include a real series.
 */

#include <charts.h>

#include <cmath>

#include <QtCore/QCollator>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QSettings>
#include <QtCore/QMimeData>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUrl>
#include <QtGui/QClipboard>
#include <QtGui/QDropEvent>
#include <QtTest/QtTest>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>

#include "src/core/models/datatable.h"
#include "src/core/nmrhandler.h"
#include "src/ui/dialogs/nmrimport.h"
#include "src/ui/guitools/appearance.h"
#include "src/ui/guitools/dialogmemory.h"
#include "src/ui/guitools/importkeyguard.h"
#include "src/ui/guitools/morelesstoggle.h"
#include "src/ui/guitools/seriessourcebar.h"
#include "src/ui/guitools/statusline.h"
#include "src/ui/widgets/DropTable.h"
#include "src/ui/widgets/concentrationeditor.h"
#include "src/ui/widgets/traceeditorwidget.h"

#include "jdf_test_writer.h"

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
/// a .jdf series of 8 FIDs in dir: a line moving 7.60 -> 7.55 ppm, a static line, a receiver phase per file
void writeJdfSeries(const QString& dir, int steps = 8, int points = 8192, double t2 = 0.3)
{
    QDir().mkpath(dir);
    for (int k = 0; k < steps; ++k) {
        const double f = 1.0 - std::exp(-1.5 * k / double(steps - 1));
        const QByteArray bytes = JdfTest::fidFile({ { 7.60 - 0.05 * f, 1.0, t2 }, { 7.26, 3.0, t2 }, { 6.90 + 0.02 * f, 0.6, t2 } },
            500.159915, 6250, 4.5, points, 19.6875, 37.0 * k - 100, -20, 0.002);
        JdfTest::save(QDir(dir).filePath(QString("titration_%1_single_pulse-1-1.jdf").arg(k + 1)), bytes);
    }
}

/// the real series of the operator, if present (SUPRAFIT_JEOL_DIR)
QStringList realJdfSeries()
{
    const QString dir = qEnvironmentVariable("SUPRAFIT_JEOL_DIR", QStringLiteral("/home/conrad/TUBAF/SB18_Titrationen"));
    QStringList files = QDir(dir).entryList({ "sb-SB18_T5_CDCl3_ObDGlc_*.jdf" }, QDir::Files);
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(files.begin(), files.end(), [&collator](const QString& a, const QString& b) { return collator.compare(a, b) < 0; });
    for (QString& f : files)
        f = QDir(dir).filePath(f);
    return files;
}

/// one spectrum per file as "ppm intensity" lines, as an ASCII export (Claude Generated, UX package 2a)
void writeNmrText(const QString& file, int step, int steps)
{
    const XYTrace trace = nmrSpectrum(step, steps);
    QFile out(file);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream stream(&out);
    for (Eigen::Index i = 0; i < trace.x().size(); ++i)
        stream << QString::number(trace.x()(i), 'g', 12) << " " << QString::number(trace.y()(i), 'g', 12) << "\n";
}

/// the More/Less switch of the lineshape fit options (MoreLessToggle has no Q_OBJECT: found as a tool button)
MoreLessToggle* fitToggle(QWidget* dialog)
{
    return static_cast<MoreLessToggle*>(dialog->findChild<QToolButton*>("nmrFitOptionsToggle"));
}

/// a drop of urls on widget, as the window system delivers it (enter, move, drop)
bool dropUrls(QWidget* widget, const QList<QUrl>& urls)
{
    QMimeData mime;
    mime.setUrls(urls);
    const QPoint at = widget->rect().center();
    QDragEnterEvent enter(at, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(widget, &enter);
    if (!enter.isAccepted())
        return false;
    QDragMoveEvent move(at, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(widget, &move);
    QDropEvent drop(QPointF(at), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(widget, &drop);
    return drop.isAccepted();
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

    QTemporaryDir m_dir;

private slots:
    void initTestCase()
    {
        qApp->setProperty("StoreRawData", true);
        QVERIFY(m_dir.isValid());
    }

    /// JEOL: the FID panel appears with a .jdf series, its controls set the processing of the handler
    void fidProcessingPanel()
    {
        const QString dir = m_dir.filePath("jdf");
        writeJdfSeries(dir, 8, 4096, 0.2);
        NmrImport text;
        fill(text.handler(), 4);
        text.refresh();
        QGroupBox* hidden = text.findChild<QGroupBox*>("nmrFidPanel");
        QVERIFY(hidden && !hidden->isVisibleTo(&text));

        NmrImport dialog;
        QString error;
        QVERIFY2(dialog.setDirectory(dir, ".jdf", &error), qPrintable(error));
        QCOMPARE(dialog.handler()->spectrumCount(), 8);
        QGroupBox* panel = dialog.findChild<QGroupBox*>("nmrFidPanel");
        QVERIFY(panel && panel->isVisibleTo(&dialog));
        QDoubleSpinBox* lb = dialog.findChild<QDoubleSpinBox*>("nmrLineBroadening");
        QDoubleSpinBox* ph0 = dialog.findChild<QDoubleSpinBox*>("nmrPh0");
        QComboBox* zeroFill = dialog.findChild<QComboBox*>("nmrZeroFill");
        QCheckBox* each = dialog.findChild<QCheckBox*>("nmrAutoPhaseEach");
        QPushButton* autoPhase = dialog.findChild<QPushButton*>("nmrAutoPhase");
        QLabel* info = dialog.findChild<QLabel*>("nmrFidInfo");
        QVERIFY(lb && ph0 && zeroFill && each && autoPhase && info);
        // default: every spectrum phased on its own, the phase fields are off
        QVERIFY(each->isChecked());
        QVERIFY(!ph0->isEnabled());
        QVERIFY(info->text().contains("CHLOROFORM-D"));
        lb->setValue(1.5);
        QCOMPARE(dialog.handler()->fidSettings().lineBroadening, 1.5);
        zeroFill->setCurrentIndex(zeroFill->findData(1));
        QCOMPARE(dialog.handler()->fidSettings().zeroFill, 1);
        QCOMPARE(dialog.handler()->spectra().at(0).size(), Eigen::Index(4096));
        // common phase from the reference spectrum, then by hand
        each->setChecked(false);
        QVERIFY(ph0->isEnabled());
        autoPhase->click();
        QVERIFY(!dialog.handler()->fidSettings().autoPhase);
        QCOMPARE(ph0->value(), std::round(dialog.handler()->fidSettings().ph0 * 10) / 10);
        ph0->setValue(12.5);
        QCOMPARE(dialog.handler()->fidSettings().ph0, 12.5);
        QCOMPARE(dialog.handler()->fidPhase(5).ph0, 12.5);
        each->setChecked(true);
        // signals and table work on the transformed spectra
        dialog.handler()->addSignal(7.60, 0.03, "H1");
        dialog.refresh();
        for (int k = 0; k < 8; ++k)
            QCOMPARE(dialog.handler()->cell(k, 0).status, NmrCellStatus::Ok);
        // back with the project
        QJsonObject project;
        project["raw"] = dialog.ProjectData();
        NmrImport again;
        again.setData(project);
        QVERIFY(again.handler()->fidSettings() == dialog.handler()->fidSettings());
        QCOMPARE(again.findChild<QDoubleSpinBox*>("nmrLineBroadening")->value(), 1.5);
        QVERIFY(again.findChild<QGroupBox*>("nmrFidPanel")->isVisibleTo(&again));
        QCOMPARE(again.handler()->cell(7, 0).shift, dialog.handler()->cell(7, 0).shift);
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

    /*! \brief Viewer 2: with a stack offset the tracked markers, the fitted model and its residual of the selected
     * spectrum are drawn with the offset of that spectrum; the spectrum is drawn wider; an excluded spectrum
     * leaves the stack. Claude Generated */
    void stackedFitCurveAndMarkers()
    {
        const int steps = 6;
        NmrImport dialog;
        for (int k = 0; k < steps; ++k)
            dialog.handler()->addSpectrum(overlapSpectrum(k, steps));
        setOverlapSignals(dialog.handler());
        dialog.refresh();
        dialog.findChild<QComboBox*>("nmrShiftMethod")->setCurrentIndex(1);
        auto* editor = dialog.findChild<TraceEditorWidget*>();
        editor->setStackPercent(50);
        const int k = 3;
        QListWidget* spectra = nullptr;
        for (QListWidget* list : dialog.findChildren<QListWidget*>())
            if (list->count() == steps)
                spectra = list;
        QVERIFY(spectra);
        spectra->setCurrentRow(k);
        const QPointF shift = editor->stackOffset(k);
        QVERIFY(shift.y() > 0);
        auto* chart = editor->chart()->chart();
        // the line of spectrum k: wider than the others
        QCOMPARE(qobject_cast<QLineSeries*>(chart->series().at(k))->pen().widthF(), 2.0);
        QCOMPARE(qobject_cast<QLineSeries*>(chart->series().at(0))->pen().widthF(), 1.0);
        const QVector<NmrFitRegion> regions = dialog.handler()->fitRegions(k);
        QVERIFY(!regions.isEmpty());
        const NmrFitRegion& region = regions.first();
        const TraceSet shown = dialog.handler()->processedSpectra();
        const double base = -0.1 * shown.at(k).y().cwiseAbs().maxCoeff();
        int curves = 0, residuals = 0;
        for (QAbstractSeries* series : chart->series()) {
            auto* line = qobject_cast<QLineSeries*>(series);
            if (!line || line->count() != int(region.block.x.size()) || line->at(0).x() != region.block.x(0) + shift.x())
                continue;
            if (line->at(0).y() == region.block.evaluate(region.block.x(0)) + shift.y())
                ++curves;
            else if (line->at(0).y() == base + shift.y() + region.block.residuals(0))
                ++residuals;
        }
        QCOMPARE(curves, 1);
        QCOMPARE(residuals, 1);
        // the tracked position of H1 in spectrum k, on the drawn (moved) spectrum
        const NmrCell cell = dialog.handler()->cell(k, 0);
        bool marker = false;
        for (QAbstractSeries* series : chart->series())
            if (auto* scatter = qobject_cast<QScatterSeries*>(series))
                for (int i = 0; i < scatter->count(); ++i)
                    marker = marker || (scatter->at(i).x() == cell.shift + shift.x() && std::abs(scatter->at(i).y() - (shown.at(k).interpolate(cell.shift) + shift.y())) < 1e-12);
        QVERIFY(marker);
        // an excluded spectrum is not drawn in the stack and does not take a level
        const QPointF before = editor->stackOffset(k);
        spectra->item(1)->setCheckState(Qt::Unchecked);
        QVERIFY(!chart->series().at(1)->isVisible());
        QCOMPARE(editor->stackOffset(1), QPointF(0, 0));
        QVERIFY(std::abs(editor->stackOffset(k).y() - (before.y() - editor->stackOffset(2).y())) < 1e-9 * before.y());
        editor->setStackPercent(0);
        QVERIFY(chart->series().at(1)->isVisible()); // without a stack drawn as before
        QSettings().remove("traceeditor/nmrimport");
    }

    /*! \brief Viewer 2: a change of the referencing offset moves the user's x zoom along with the axis, y stays. */
    void referenceOffsetMovesTheZoom()
    {
        NmrImport dialog;
        fill(dialog.handler(), 4);
        dialog.refresh();
        auto* editor = dialog.findChild<TraceEditorWidget*>();
        editor->chart()->setXRange(7.0, 7.4);
        editor->fitYToVisible();
        QVERIFY(editor->hasUserView());
        const double y0 = editor->chart()->axisY()->min(), y1 = editor->chart()->axisY()->max();
        dialog.findChild<QDoubleSpinBox*>("nmrReferenceOffset")->setValue(0.1);
        QVERIFY(std::abs(editor->chart()->axisX()->min() - 7.1) < 1e-9 && std::abs(editor->chart()->axisX()->max() - 7.5) < 1e-9);
        QCOMPARE(editor->chart()->axisY()->min(), y0);
        QCOMPARE(editor->chart()->axisY()->max(), y1);
    }

    /*! \brief Viewer 2: a signal missing in every spectrum (below 5 % of a dominating standard) gets a hint in the
     * status line and the tooltips; a Blank region step over the standard brings the signal back. */
    void hintWhenEverySpectrumMisses()
    {
        NmrImport dialog;
        for (int k = 0; k < 4; ++k) {
            const int n = 2001;
            Eigen::VectorXd x(n), y(n);
            for (int i = 0; i < n; ++i) {
                x(i) = 10.0 - 0.005 * i;
                y(i) = 100 * std::exp(-std::pow((x(i) - 0.5) / 0.02, 2)) + std::exp(-std::pow((x(i) - (5.0 + 0.01 * k)) / 0.02, 2));
            }
            dialog.handler()->addSpectrum(XYTrace(x, y, QString("s%1").arg(k)));
        }
        dialog.handler()->addSignal(5.0, 0.1, "S");
        dialog.refresh();
        for (int k = 0; k < 4; ++k)
            QCOMPARE(dialog.handler()->cell(k, 0).status, NmrCellStatus::Missing);
        auto* hint = dialog.findChild<StatusLine*>("nmrMissingHint");
        auto* status = dialog.findChild<StatusLine*>("nmrStatus");
        QVERIFY(hint && status);
        QVERIFY2(hint->message().contains("Blank region") && hint->level() == StatusLine::Warning && !hint->isHidden(), qPrintable(hint->message()));
        QTableWidget* results = dialog.findChild<QTableWidget*>("nmrResults");
        QVERIFY(results->item(0, 0)->toolTip().contains("standard"));
        // another message in the status line does not hide or keep the hint
        dialog.addSignalAt(0.5); // the standard: found everywhere
        QVERIFY(status->message().contains("added"));
        QVERIFY(hint->message().startsWith("S:") && !hint->isHidden());
        // values set by hand in every spectrum: nothing missing in the table, no hint
        for (int k = 0; k < 4; ++k)
            dialog.handler()->setManualShift(k, 0, 5.0);
        dialog.refresh();
        QVERIFY2(hint->isHidden() && hint->message().isEmpty(), qPrintable(hint->message()));
        for (int k = 0; k < 4; ++k)
            dialog.handler()->clearManualShift(k, 0);
        dialog.refresh();
        QVERIFY(!hint->isHidden());
        QVERIFY(dialog.handler()->removeSignal(1));
        dialog.refresh();

        auto* editor = dialog.findChild<TraceEditorWidget*>();
        editor->setRange(0.3, 0.7);
        QCOMPARE(editor->addOperation("Blank"), 0);
        QCOMPARE(editor->operations().at(0)->toJson()["Ranges"].toArray().size(), 1);
        for (int k = 0; k < 4; ++k) {
            QCOMPARE(dialog.handler()->cell(k, 0).status, NmrCellStatus::Ok);
            QVERIFY(std::abs(dialog.handler()->cell(k, 0).shift - (5.0 + 0.01 * k)) < 0.003);
        }
        QVERIFY(!dialog.findChild<QTableWidget*>("nmrResults")->item(0, 0)->toolTip().contains("standard"));
        QVERIFY2(hint->isHidden() && hint->message().isEmpty(), qPrintable(hint->message()));
        QVERIFY(!status->message().contains("Blank region"));
        // the lineshape fit with the signal found: no hint either; without the step it comes back
        dialog.findChild<QComboBox*>("nmrShiftMethod")->setCurrentIndex(1);
        QVERIFY(hint->isHidden());
        QVERIFY(editor->setOperationEnabled(0, false));
        QVERIFY(!hint->isHidden());
        QVERIFY(editor->setOperationEnabled(0, true));
        QVERIFY(hint->isHidden());
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

    /// UX package 1 (Claude Generated): Enter does not accept, Cancel asks only after a change, the status line
    /// at the bottom says what OK does (visible on the first page)
    void keyboardRuleAndStatusLine()
    {
        const int steps = 6;
        NmrImport dialog;
        fill(dialog.handler(), steps);
        dialog.handler()->addSignal(7.60, 0.08, "H1");
        std::unique_ptr<DataTable> conc(concentrations(steps));
        dialog.setConcentrations(conc.get());
        dialog.refresh();
        QString error;
        QVERIFY2(dialog.validate(&error), qPrintable(error)); // OK would hand over: Enter must not press it
        QStringList asked;
        bool answer = false;
        dialog.keyGuard()->setConfirmHandler([&](const QString& question) {
            asked << question;
            return answer;
        });
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        QCOMPARE(dialog.findChild<QTabWidget*>()->currentIndex(), 0);
        StatusLine* ready = dialog.findChild<StatusLine*>("nmrReady");
        QVERIFY(ready && ready->isVisible());
        QCOMPARE(ready->level(), StatusLine::Info);
        QVERIFY(ready->message().startsWith(QObject::tr("OK hands over").left(8)));

        // Enter in the start of the range and in the search window
        TraceEditorWidget* editor = dialog.findChild<TraceEditorWidget*>();
        QDoubleSpinBox* start = editor->findChildren<QDoubleSpinBox*>().value(0);
        QDoubleSpinBox* window = nullptr;
        for (QDoubleSpinBox* box : dialog.findChildren<QDoubleSpinBox*>())
            if (box->toolTip().startsWith("Window of new signals"))
                window = box;
        QVERIFY(start && window);
        for (QDoubleSpinBox* box : { start, window }) {
            box->setFocus();
            QTest::keyClick(box, Qt::Key_Return);
            QTest::keyClick(box, Qt::Key_Enter);
            QVERIFY(dialog.isVisible());
        }
        QCOMPARE(dialog.result(), 0);
        // ESC in the chart stays there
        editor->chart()->privateView()->setFocus();
        QTest::keyClick(editor->chart()->privateView(), Qt::Key_Escape);
        QVERIFY(dialog.isVisible());

        // unchanged: Cancel closes without a question
        QVERIFY(!dialog.keyGuard()->isDirty());
        dialog.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel)->click();
        QVERIFY(!dialog.isVisible());
        QCOMPARE(asked.size(), 0);

        // a change: the question, "Keep editing" keeps the dialog
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        dialog.addSignalAt(7.10);
        QVERIFY(dialog.keyGuard()->isDirty());
        dialog.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel)->click();
        QCOMPARE(asked.size(), 1);
        QVERIFY(dialog.isVisible());
        answer = true;
        QTest::keyClick(&dialog, Qt::Key_Escape);
        QCOMPARE(asked.size(), 2);
        QVERIFY(!dialog.isVisible());

        // without concentrations the line says why OK cannot hand over
        NmrImport empty;
        fill(empty.handler(), steps);
        empty.handler()->addSignal(7.60, 0.08, "H1");
        empty.refresh();
        QCOMPARE(empty.findChild<StatusLine*>("nmrReady")->level(), StatusLine::Warning);
        QVERIFY2(empty.findChild<StatusLine*>("nmrReady")->message().contains("concentrations"), qPrintable(empty.findChild<StatusLine*>("nmrReady")->message()));
    }

    /// D1 (Claude Generated): a double-click adds a signal, or only sets the position; the choice is remembered
    void doubleClickAddsSignal()
    {
        NmrImport dialog;
        fill(dialog.handler(), 6);
        dialog.refresh();
        QCheckBox* adds = dialog.findChild<QCheckBox*>("nmrDoubleClickAdds");
        QVERIFY(adds && adds->isChecked());
        TraceEditorWidget* editor = dialog.findChild<TraceEditorWidget*>();
        QVERIFY(editor->hintText().contains(QObject::tr("Double-click: add a signal")));
        emit editor->chart()->pointDoubleClicked(QPointF(7.60, 0.5));
        QCOMPARE(dialog.handler()->signalList().size(), 1);
        QCOMPARE(dialog.handler()->signalList()[0].position, 7.60);

        adds->setChecked(false);
        QCOMPARE(QSettings().value("nmrimport/doubleClickAdds").toBool(), false);
        QVERIFY(editor->hintText().contains(QObject::tr("Double-click: set the position of a new signal")));
        emit editor->chart()->pointDoubleClicked(QPointF(7.10, 0.5));
        QCOMPARE(dialog.handler()->signalList().size(), 1);
        QDoubleSpinBox* position = nullptr;
        for (QDoubleSpinBox* box : dialog.findChildren<QDoubleSpinBox*>())
            if (box->toolTip().startsWith("Position of a new signal"))
                position = box;
        QVERIFY(position);
        QCOMPARE(position->value(), 7.10);

        NmrImport again; // the next dialog starts with the remembered choice
        QVERIFY(!again.findChild<QCheckBox*>("nmrDoubleClickAdds")->isChecked());
        again.findChild<QCheckBox*>("nmrDoubleClickAdds")->setChecked(true);
    }

    /// D2 (Claude Generated): picking, removing a spectrum or a signal asks when hand-set values would be lost
    void destructiveActionsAsk()
    {
        const int steps = 6;
        NmrImport dialog;
        fill(dialog.handler(), steps);
        dialog.handler()->addSignal(7.60, 0.08, "H1");
        dialog.handler()->addSignal(7.10, 0.08, "H2");
        dialog.refresh();
        QVERIFY(dialog.handler()->setManualShift(2, 1, 7.15));
        dialog.refresh();
        QCOMPARE(dialog.manualCount(), 1);
        QStringList asked;
        bool answer = false;
        dialog.keyGuard()->setConfirmHandler([&](const QString& question) {
            asked << question;
            return answer;
        });
        QPushButton *pick = nullptr, *removeSignal = nullptr, *removeSpectrum = nullptr;
        for (QPushButton* button : dialog.findChildren<QPushButton*>()) {
            if (button->text() == QObject::tr("Pick from reference spectrum"))
                pick = button;
            else if (button->text() == QObject::tr("Remove signal"))
                removeSignal = button;
            else if (button->text() == QObject::tr("Remove") && !dialog.findChild<TraceEditorWidget*>()->isAncestorOf(button))
                removeSpectrum = button; // the editor has a "Remove" of its steps
        }
        QVERIFY(pick && removeSignal && removeSpectrum);

        pick->click();
        QCOMPARE(asked.size(), 1);
        QVERIFY2(asked[0].contains("2 signal") && asked[0].contains("1 hand-set"), qPrintable(asked[0]));
        QCOMPARE(dialog.handler()->signalList().size(), 2); // kept
        QCOMPARE(dialog.manualCount(), 1);

        QTableWidget* signalTable = dialog.findChild<QTableWidget*>("nmrSignals");
        signalTable->setCurrentCell(0, 0); // H1 has no hand-set value: no question
        removeSignal->click();
        QCOMPARE(asked.size(), 1);
        QCOMPARE(dialog.handler()->signalList().size(), 1);
        signalTable = dialog.findChild<QTableWidget*>("nmrSignals");
        signalTable->setCurrentCell(0, 0); // H2 (now column 0) has one
        removeSignal->click();
        QCOMPARE(asked.size(), 2);
        QCOMPARE(dialog.handler()->signalList().size(), 1);

        QListWidget* spectra = nullptr;
        for (QListWidget* list : dialog.findChildren<QListWidget*>())
            if (list->toolTip().startsWith("Titration order"))
                spectra = list;
        QVERIFY(spectra);
        spectra->setCurrentRow(2);
        removeSpectrum->click();
        QCOMPARE(asked.size(), 3);
        QVERIFY(asked[2].contains("1 hand-set"));
        QCOMPARE(dialog.handler()->spectrumCount(), steps);
        answer = true;
        spectra->setCurrentRow(2);
        removeSpectrum->click();
        QCOMPARE(dialog.handler()->spectrumCount(), steps - 1);
        QCOMPARE(dialog.manualCount(), 0);
    }

    /*! \brief Viewer 2 renders (SUPRAFIT_TRACE_RENDER_DIR): a series with a dominating CHCl3 line, unstacked,
     * stacked in y, with x offset, reverse order, with a Blank region step, and the fit of one spectrum in the stack. */
    /// every test starts without remembered dialog settings (Claude Generated, UX package 2a)
    void init()
    {
        QSettings().remove("dialogs");
    }

    /// A: options of a new series survive closing the dialog; ph0 belongs to the series (Claude Generated)
    void rememberedOptionsOfANewSeries()
    {
        {
            NmrImport dialog;
            dialog.findChild<QDoubleSpinBox*>("nmrLineBroadening")->setValue(1.5);
            dialog.findChild<QComboBox*>("nmrZeroFill")->setCurrentIndex(2);
            dialog.findChild<QDoubleSpinBox*>("nmrPh0")->setValue(30);
            dialog.findChild<QComboBox*>("nmrShiftMethod")->setCurrentIndex(1);
            dialog.findChild<QSpinBox*>("nmrMaxIterations")->setValue(500);
            dialog.findChild<QDoubleSpinBox*>("nmrFitRange")->setValue(0.05);
            dialog.findChild<QComboBox*>("nmrBaseline")->setCurrentIndex(2);
            dialog.findChild<QComboBox*>("nmrWidthMode")->setCurrentIndex(1);
            dialog.findChild<QCheckBox*>("nmrPeakFallback")->setChecked(true);
            QDoubleSpinBox* window = nullptr;
            for (QDoubleSpinBox* box : dialog.findChildren<QDoubleSpinBox*>())
                if (box->toolTip().startsWith(QObject::tr("Window of new signals").left(10)))
                    window = box;
            QVERIFY(window);
            window->setValue(0.02);
            dialog.sourceBar()->endingBox()->setCurrentText(".asc");
        }
        NmrImport again;
        QCOMPARE(again.findChild<QDoubleSpinBox*>("nmrLineBroadening")->value(), 1.5);
        QCOMPARE(again.handler()->fidSettings().lineBroadening, 1.5);
        QCOMPARE(again.findChild<QComboBox*>("nmrZeroFill")->currentIndex(), 2);
        QCOMPARE(again.findChild<QDoubleSpinBox*>("nmrPh0")->value(), 0.0);
        QCOMPARE(again.findChild<QComboBox*>("nmrShiftMethod")->currentIndex(), 1);
        QCOMPARE(again.handler()->fitSettings().method, NmrShiftMethod::LineshapeFit);
        QCOMPARE(again.handler()->fitSettings().maxIterations, 500);
        QCOMPARE(again.handler()->fitSettings().range, 0.05);
        QCOMPARE(again.handler()->fitSettings().baseline, LineshapeBaseline::Linear);
        QCOMPARE(again.handler()->fitSettings().widthMode, NmrWidthMode::Shared);
        QVERIFY(again.handler()->fitSettings().peakFallback);
        QCOMPARE(again.handler()->defaultWindow(), 0.02);
        QCOMPARE(again.sourceBar()->ending(), QString(".asc"));
        QVERIFY(fitToggle(&again)->isExpanded()); // lineshape fit
    }

    /// A: a reopened project keeps its own values, not the remembered ones (Claude Generated)
    void projectValuesWin()
    {
        QJsonObject project;
        {
            NmrImport dialog;
            fill(dialog.handler(), 4);
            dialog.handler()->addSignal(7.60, 0.08, "H1");
            dialog.refresh();
            project["raw"] = dialog.ProjectData(); // peak picking, window 0.05, 200 iterations
        }
        {
            NmrImport dialog; // remember other values
            dialog.findChild<QComboBox*>("nmrShiftMethod")->setCurrentIndex(1);
            dialog.findChild<QSpinBox*>("nmrMaxIterations")->setValue(700);
            dialog.findChild<QDoubleSpinBox*>("nmrLineBroadening")->setValue(2.0);
        }
        NmrImport reopened;
        QCOMPARE(reopened.handler()->fitSettings().method, NmrShiftMethod::LineshapeFit); // the memory, before the project
        reopened.setData(project);
        QCOMPARE(reopened.handler()->fitSettings().method, NmrShiftMethod::PeakPicking);
        QCOMPARE(reopened.findChild<QComboBox*>("nmrShiftMethod")->currentIndex(), 0);
        QCOMPARE(reopened.handler()->fitSettings().maxIterations, 200);
        QCOMPARE(reopened.findChild<QSpinBox*>("nmrMaxIterations")->value(), 200);
        QCOMPARE(reopened.handler()->fidSettings().lineBroadening, FidSettings().lineBroadening);
        // reading the project wrote nothing into the memory
        QCOMPARE(reopened.memory()->value("maxIterations").toInt(), 700);
    }

    /// A: stored values of the wrong type or outside the limits are ignored (Claude Generated)
    void invalidStoredValuesAreIgnored()
    {
        QSettings settings;
        settings.setValue("dialogs/nmrimport/maxIterations", 999999);
        settings.setValue("dialogs/nmrimport/searchWindow", "abc");
        settings.setValue("dialogs/nmrimport/shiftFrom", 7);
        settings.setValue("dialogs/nmrimport/peakFallback", "maybe");
        settings.setValue("dialogs/nmrimport/lineBroadening", -3.0);
        settings.setValue("dialogs/nmrimport/zeroFill", "2.5");
        settings.setValue("dialogs/nmrimport/fitRange", 99.0);
        settings.setValue("dialogs/nmrimport/geometry", QByteArray("garbage"));
        settings.setValue("dialogs/nmrimport/splitter/main", QByteArray("garbage"));
        NmrImport dialog;
        const NmrFitSettings defaults;
        QCOMPARE(dialog.handler()->fitSettings().maxIterations, defaults.maxIterations);
        QCOMPARE(dialog.handler()->fitSettings().method, defaults.method);
        QCOMPARE(dialog.handler()->fitSettings().range, defaults.range);
        QCOMPARE(dialog.handler()->fitSettings().peakFallback, defaults.peakFallback);
        QCOMPARE(dialog.handler()->fidSettings().lineBroadening, FidSettings().lineBroadening);
        QCOMPARE(dialog.handler()->fidSettings().zeroFill, FidSettings().zeroFill);
        QCOMPARE(dialog.size(), QSize(1280, 800));
        QVERIFY(!fitToggle(&dialog)->isExpanded());
    }

    /// A: geometry and splitters come back; a place on no screen is not used (Claude Generated)
    void geometryAndSplitters()
    {
        QByteArray splitter;
        {
            NmrImport dialog;
            dialog.show();
            QVERIFY(QTest::qWaitForWindowExposed(&dialog));
            dialog.setGeometry(40, 40, 1000, 650);
            QSplitter* main = dialog.findChild<QSplitter*>("nmrMainSplitter");
            main->setSizes({ 200, 500, 300 });
            splitter = main->saveState();
            dialog.hide();
        }
        {
            NmrImport dialog;
            QCOMPARE(dialog.size(), QSize(1000, 650));
            QCOMPARE(dialog.geometry().topLeft(), QPoint(40, 40));
            QCOMPARE(dialog.findChild<QSplitter*>("nmrMainSplitter")->saveState(), splitter);
            dialog.show();
            QVERIFY(QTest::qWaitForWindowExposed(&dialog));
            dialog.setGeometry(6000, 6000, 1100, 700); // a screen that is gone
            dialog.hide();
        }
        NmrImport dialog;
        QCOMPARE(dialog.size(), QSize(1280, 800));
        QVERIFY(!DialogMemory::fitsAScreen(QRect(6000, 6000, 1100, 700)));
        QVERIFY(DialogMemory::fitsAScreen(QRect(10, 10, 1100, 700)));
    }

    /// D: the lineshape options are folded by default and open for a lineshape fit and its project (Claude Generated)
    void fitOptionsMoreLess()
    {
        NmrImport dialog;
        MoreLessToggle* toggle = fitToggle(&dialog);
        QWidget* options = dialog.findChild<QWidget*>("nmrFitOptions");
        QVERIFY(toggle && options);
        QVERIFY(!toggle->isExpanded());
        QCOMPARE(options->maximumHeight(), 0);
        QVERIFY(dialog.findChild<QComboBox*>("nmrShiftMethod")->isVisibleTo(&dialog)); // "Shift from" stays in sight
        dialog.findChild<QComboBox*>("nmrShiftMethod")->setCurrentIndex(1);
        QVERIFY(toggle->isExpanded());
        QVERIFY(options->maximumHeight() > 0);
        dialog.findChild<QComboBox*>("nmrShiftMethod")->setCurrentIndex(0);
        QVERIFY(toggle->isExpanded()); // only opens by itself
        toggle->click(); // the user folds it: kept
        QVERIFY(!toggle->isExpanded());
        QCOMPARE(dialog.memory()->value("fitOptionsExpanded").toBool(), false);

        // a project with a lineshape fit opens them
        fill(dialog.handler(), 4);
        dialog.handler()->addSignal(7.60, 0.08, "H1");
        NmrFitSettings fit;
        fit.method = NmrShiftMethod::LineshapeFit;
        dialog.handler()->setFitSettings(fit);
        QJsonObject project;
        project["raw"] = dialog.ProjectData();
        QSettings().remove("dialogs");
        NmrImport reopened;
        QVERIFY(!fitToggle(&reopened)->isExpanded());
        reopened.setData(project);
        QVERIFY(fitToggle(&reopened)->isExpanded());
    }

    /// B: drops of a directory, files, both and a file that is no spectrum; natural order; the message (Claude Generated)
    void dropFilesAndDirectories()
    {
        const QString dir = m_dir.filePath("drop");
        QDir().mkpath(dir);
        const int steps = 3;
        const QStringList names = { "t_10.txt", "t_2.txt", "t_1.txt" };
        for (int k = 0; k < steps; ++k)
            writeNmrText(QDir(dir).filePath(names[k]), k, steps);
        {
            QFile bad(QDir(dir).filePath("t_5.txt"));
            QVERIFY(bad.open(QIODevice::WriteOnly));
            bad.write("no numbers here\n");
        }
        {
            QFile other(QDir(dir).filePath("notes.md")); // other ending: not taken from the directory
            QVERIFY(other.open(QIODevice::WriteOnly));
        }

        NmrImport dialog;
        dialog.sourceBar()->setEnding(".txt");
        // a directory, dropped on the results table (a child)
        QVERIFY(dropUrls(dialog.findChild<QTableWidget*>("nmrResults"), { QUrl::fromLocalFile(dir) }));
        QCOMPARE(dialog.handler()->spectrumCount(), 3);
        QCOMPARE(dialog.handler()->spectra().at(0).name(), QString("t_1.txt"));
        QCOMPARE(dialog.handler()->spectra().at(1).name(), QString("t_2.txt"));
        QCOMPARE(dialog.handler()->spectra().at(2).name(), QString("t_10.txt"));
        StatusLine* line = dialog.sourceBar()->statusLine();
        QVERIFY2(line->message().startsWith("3 file(s) read, 1 not read"), qPrintable(line->message()));
        QVERIFY(line->message().contains("t_5.txt"));
        QCOMPARE(line->level(), StatusLine::Warning);
        QCOMPARE(dialog.findChild<QLabel*>("seriesSpectraCount")->text(), QString("Spectra: 3"));

        // files dropped on the read-only path field append, in natural order
        QVERIFY(dropUrls(dialog.findChild<QLineEdit*>("seriesPath"), { QUrl::fromLocalFile(QDir(dir).filePath("t_2.txt")), QUrl::fromLocalFile(QDir(dir).filePath("t_10.txt")) }));
        QCOMPARE(dialog.handler()->spectrumCount(), 5);
        QCOMPARE(dialog.handler()->spectra().at(3).name(), QString("t_2.txt"));
        QCOMPARE(dialog.handler()->spectra().at(4).name(), QString("t_10.txt"));
        QCOMPARE(line->message(), QString("2 file(s) read"));
        QCOMPARE(line->level(), StatusLine::Info);

        // mixed: an empty directory, a file, a URL that is no local file
        const QString empty = m_dir.filePath("drop_empty");
        QDir().mkpath(empty);
        QVERIFY(dropUrls(&dialog, { QUrl::fromLocalFile(empty), QUrl::fromLocalFile(QDir(dir).filePath("t_1.txt")), QUrl("https://example.org/a.txt") }));
        QCOMPARE(dialog.handler()->spectrumCount(), 6);
        QVERIFY2(line->message().startsWith("1 file(s) read, 2 not read"), qPrintable(line->message()));
        QVERIFY(line->toolTip().contains("drop_empty"));
        QVERIFY(line->toolTip().contains("not a local file"));

        // a drop of text is not taken
        QMimeData text;
        text.setText("7.25");
        QDragEnterEvent enter(QPoint(5, 5), Qt::CopyAction, &text, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(&dialog, &enter);
        QVERIFY(!enter.isAccepted());

        // "Open directory" of a directory without such a file changes nothing
        QString error;
        QVERIFY(!dialog.setDirectory(empty, ".txt", &error));
        QCOMPARE(dialog.handler()->spectrumCount(), 6);
        QCOMPARE(line->level(), StatusLine::Error);
    }

    /// C: the concentration editor: paste, load from a file (decimal comma), rows against spectra (Claude Generated)
    void concentrationEditor()
    {
        const int steps = 10;
        NmrImport dialog;
        fill(dialog.handler(), steps);
        dialog.handler()->addSignal(7.60, 0.08, "H1");
        dialog.refresh();
        ConcentrationEditor* editor = dialog.concentrationEditor();
        QVERIFY(editor);
        QVERIFY(editor->findChild<QLabel*>("concentrationHint")->text().contains("Ctrl+V"));
        QCOMPARE(editor->statusLine()->level(), StatusLine::Info);

        // three rows for four spectra: warning, OK not possible
        QApplication::clipboard()->setText("0,001\t0\n0,001\t0,0005\n0,001\t0,001\n");
        editor->table()->setFocus();
        QTest::keyClick(editor->table(), Qt::Key_V, Qt::ControlModifier);
        QCOMPARE(editor->rows(), 3);
        QCOMPARE(editor->statusLine()->level(), StatusLine::Warning);
        QVERIFY2(editor->statusLine()->message().startsWith("3 rows for 10 spectra"), qPrintable(editor->statusLine()->message()));
        QVERIFY(dialog.findChild<StatusLine*>("nmrReady")->message().startsWith(QObject::tr("OK not possible yet")));

        // a file with a header and decimal commas
        const QString file = m_dir.filePath("conc.txt");
        {
            QFile out(file);
            QVERIFY(out.open(QIODevice::WriteOnly | QIODevice::Text));
            out.write("Host\tGuest\n");
            for (int k = 0; k < steps; ++k)
                out.write(QString("0,001\t%1\n").arg(QString::number(5e-4 * k).replace('.', ',')).toUtf8());
        }
        QString error;
        QVERIFY2(editor->loadFile(file, &error), qPrintable(error));
        QCOMPARE(editor->rows(), steps);
        QCOMPARE(editor->columns(), 2);
        QCOMPARE(editor->dataTable()->data(3, 1), 1.5e-3);
        QCOMPARE(editor->statusLine()->message(), QString("10 rows for 10 spectra"));
        QVERIFY2(dialog.validate(&error), qPrintable(error));
        QCOMPARE(dialog.IndependentCount(), 2);
        QCOMPARE(DataTable(dialog.InputTable()).rowCount(), steps);

        // one spectrum left out: one row per spectrum or per used spectrum
        dialog.handler()->setSpectrumExcluded(steps - 1, true);
        dialog.refresh();
        QCOMPARE(editor->statusLine()->message(), QString("10 rows for 10 spectra"));
        {
            std::unique_ptr<DataTable> nine(concentrations(steps - 1));
            dialog.setConcentrations(nine.get());
        }
        QCOMPARE(editor->statusLine()->message(), QString("9 rows for 9 used spectra"));
        QVERIFY2(dialog.validate(&error), qPrintable(error));
        {
            std::unique_ptr<DataTable> eight(concentrations(steps - 2));
            dialog.setConcentrations(eight.get());
        }
        QCOMPARE(editor->statusLine()->level(), StatusLine::Warning);
        QVERIFY(editor->statusLine()->message().contains("9 used"));

        // a file without numbers leaves the table as it is
        const QString text = m_dir.filePath("conc_bad.txt");
        {
            QFile out(text);
            QVERIFY(out.open(QIODevice::WriteOnly));
            out.write("nothing\n");
        }
        QVERIFY(!editor->loadFile(text, &error));
        QCOMPARE(editor->rows(), steps - 2);
        QCOMPARE(editor->statusLine()->level(), StatusLine::Error);
    }

    /// renders of the source bar, the concentration editor and the folded/opened fit options (Claude Generated)
    void renderPackage2a()
    {
        if (qEnvironmentVariable("SUPRAFIT_TRACE_RENDER_DIR").isEmpty())
            QSKIP("SUPRAFIT_TRACE_RENDER_DIR not set");
        const QString dir = m_dir.filePath("render2a");
        QDir().mkpath(dir);
        for (int k = 0; k < 6; ++k)
            writeNmrText(QDir(dir).filePath(QString("step_%1.txt").arg(k + 1)), k, 6);
        {
            QFile empty(QDir(dir).filePath("step_9.txt")); // an empty file: "1 not read"
            QVERIFY(empty.open(QIODevice::WriteOnly));
        }
        for (int mode : { int(Appearance::Light), int(Appearance::Dark) }) {
            Appearance::Apply(mode);
            const QString suffix = mode == Appearance::Light ? "_light" : "_dark";
            QSettings().remove("dialogs");
            NmrImport dialog;
            dialog.resize(1366, 768);
            dialog.sourceBar()->setEnding(".txt");
            QVERIFY(dropUrls(&dialog, { QUrl::fromLocalFile(dir) }));
            dialog.handler()->addSignal(7.60, 0.08, "H1");
            dialog.refresh();
            render(&dialog, "nmr2a_source_folded" + suffix);
            fitToggle(&dialog)->setExpanded(true, false);
            QCoreApplication::processEvents();
            render(&dialog, "nmr2a_options_open" + suffix);
            dialog.findChild<QTabWidget*>()->setCurrentIndex(1);
            QApplication::clipboard()->setText("0.001\t0\n0.001\t0.0005\n0.001\t0.001\n");
            dialog.concentrationEditor()->table()->setFocus();
            QTest::keyClick(dialog.concentrationEditor()->table(), Qt::Key_V, Qt::ControlModifier);
            render(&dialog, "nmr2a_concentrations_warning" + suffix);
            dialog.keyGuard()->setConfirmHandler([](const QString&) { return true; });
            dialog.hide();
        }
        Appearance::Apply(Appearance::Light);
        QSettings().remove("dialogs");
    }

    void renderStack()
    {
        if (qEnvironmentVariable("SUPRAFIT_TRACE_RENDER_DIR").isEmpty())
            QSKIP("SUPRAFIT_TRACE_RENDER_DIR not set");
        // the hint line is shown exactly when a signal is missing in every used spectrum
        auto checkHint = [](NmrImport& dialog) {
            const NmrHandler* handler = dialog.handler();
            bool lost = false;
            for (int s : handler->enabledSignals()) {
                int used = 0, missing = 0;
                for (int k = 0; k < handler->spectrumCount(); ++k)
                    if (!handler->isSpectrumExcluded(k)) {
                        ++used;
                        missing += handler->cell(k, s).status == NmrCellStatus::Missing;
                    }
                lost = lost || (used > 0 && missing == used);
            }
            auto* hint = dialog.findChild<StatusLine*>("nmrMissingHint");
            return hint && hint->isHidden() == !lost && hint->message().isEmpty() == !lost;
        };
        const int steps = 8;
        auto spectrum = [steps](int k) {
            const int n = 3001; // below 5000: drawn without OpenGL, visible in an offscreen grab
            Eigen::VectorXd x(n), y(n);
            const double f = 1.0 - std::exp(-1.5 * k / double(steps - 1));
            for (int i = 0; i < n; ++i) {
                x(i) = 8.0 - 0.001 * i;
                const double lines[3][3] = { { 7.26, 0.004, 12.0 }, { 7.60 - 0.30 * f, 0.005, 0.5 }, { 6.20 + 0.10 * f, 0.005, 0.35 } };
                double v = 0;
                for (const auto& l : lines) {
                    const double u = (x(i) - l[0]) / l[1];
                    v += l[2] / (1 + u * u);
                }
                y(i) = v + 0.003 * std::sin(37.0 * i);
            }
            return XYTrace(x, y, QString("step_%1.txt").arg(k));
        };
        QSettings().remove("traceeditor/nmrimport");
        for (int mode : { int(Appearance::Light), int(Appearance::Dark) }) {
            Appearance::Apply(mode);
            const QString suffix = mode == Appearance::Light ? "_light" : "_dark";
            NmrImport dialog;
            for (int k = 0; k < steps; ++k)
                dialog.handler()->addSpectrum(spectrum(k));
            dialog.handler()->addSignal(7.60, 0.08, "H1");
            dialog.handler()->addSignal(6.20, 0.08, "H2");
            dialog.refresh();
            dialog.resize(1366, 768);
            auto* editor = dialog.findChild<TraceEditorWidget*>();
            QVERIFY(checkHint(dialog));
            render(&dialog, "stack_none" + suffix);
            editor->setStackPercent(5); // 5 % of the CHCl3 line
            QVERIFY(checkHint(dialog));
            render(&dialog, "stack_y_chcl3" + suffix);
            // blank CHCl3: the small lines set the scale and the stack
            editor->setRange(7.20, 7.32);
            editor->addOperation("Blank");
            editor->setRange(5.0, 8.0);
            editor->setStackPercent(60);
            QVERIFY(checkHint(dialog));
            render(&dialog, "stack_y_blanked" + suffix);
            editor->setStackXPercent(2);
            QVERIFY(checkHint(dialog));
            render(&dialog, "stack_xy_blanked" + suffix);
            editor->setStackReversed(true);
            QVERIFY(checkHint(dialog));
            render(&dialog, "stack_reverse_blanked" + suffix);
            editor->setStackReversed(false);
            editor->setStackXPercent(0);
            // the fit of spectrum 5 with its residual in the stack
            dialog.findChild<QComboBox*>("nmrShiftMethod")->setCurrentIndex(1);
            for (QListWidget* list : dialog.findChildren<QListWidget*>())
                if (list->count() == steps)
                    list->setCurrentRow(5);
            QVERIFY(checkHint(dialog));
            render(&dialog, "stack_fit" + suffix);
            editor->chart()->setXRange(7.15, 7.70);
            editor->fitYToVisible();
            QVERIFY(checkHint(dialog));
            render(&dialog, "stack_fit_zoom" + suffix);
            dialog.hide();
            QSettings().remove("traceeditor/nmrimport");
        }
        Appearance::Apply(Appearance::Light);
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
            {
                // JEOL: synthetic FID series with the processing panel
                // 2048 points (4096 after zero filling): offscreen grabs do not show series above 5000 points
                // (the chart draws those with OpenGL); T2 0.1 s, so the short FID has decayed (no truncation wiggles)
                const QString dir = m_dir.filePath("jdf_render");
                writeJdfSeries(dir, 8, 2048, 0.1);
                NmrImport dialog;
                dialog.resize(1366, 768);
                dialog.setDirectory(dir, ".jdf");
                dialog.handler()->addSignal(7.60, 0.03, "H1");
                dialog.handler()->addSignal(6.90, 0.03, "H2");
                dialog.refresh();
                render(&dialog, "nmr_jdf_synthetic" + suffix);
                dialog.hide();
            }
            const QStringList real = realJdfSeries();
            if (!real.isEmpty()) {
                // the operator's T5 series: CHCl3 referenced to 7.26 ppm, zoom on H2..H7
                NmrImport dialog;
                dialog.resize(1366, 768);
                QElapsedTimer timer;
                timer.start();
                QString error;
                dialog.addFiles(real, &error);
                const qint64 loading = timer.elapsed();
                const XYTrace& first = dialog.handler()->spectra().at(0);
                const XYTrace chcl3 = first.range(7.2, 7.3);
                Eigen::Index top = 0;
                chcl3.y().maxCoeff(&top);
                dialog.handler()->referenceTo(chcl3.x()(top), 7.26);
                dialog.refresh();
                timer.restart();
                render(&dialog, "nmr_jdf_real" + suffix);
                // H2 (8.90-8.93 ppm) as a range step: below 5000 points, so the offscreen grab shows the lines
                // the intensities differ between the experiments: normalised to CHCl3 first
                TraceOpList operations;
                NormaliseOp normalise(TraceOps::NormaliseMode::Max);
                normalise.useReference = true;
                normalise.refStart = 7.2;
                normalise.refEnd = 7.3;
                operations.append(normalise);
                operations.append(RangeOp(8.89, 8.935));
                dialog.handler()->setOperations(operations);
                dialog.findChild<TraceEditorWidget*>()->setOperations(operations);
                dialog.findChild<TraceEditorWidget*>()->setShowProcessed(true);
                dialog.handler()->addSignal(8.9267, 0.0025, "H2a");
                dialog.handler()->addSignal(8.9222, 0.0025, "H2b");
                dialog.refresh();
                dialog.findChild<TraceEditorWidget*>()->chart()->setXMin(8.89);
                dialog.findChild<TraceEditorWidget*>()->chart()->setXMax(8.935);
                render(&dialog, "nmr_jdf_real_h2" + suffix);
                qInfo("real JEOL series: %d spectra read and transformed in %lld ms, two renders %lld ms", int(real.size()), loading, timer.elapsed());
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

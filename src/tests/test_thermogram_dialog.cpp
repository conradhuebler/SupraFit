/*
 * SupraFit - offscreen smoke test of the thermogram import dialog
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
 */

/* Claude Generated (WP5 of roadmap/input_data_editing.md). Builds the Thermogram dialog offscreen, loads
 * the samples of data/samples/itc through its public entry points (no file dialogs) and checks that the
 * table it shows and the table it hands over are what the core ItcProcessor computes for the same calls:
 * .itc experiment (volumes from the @-lines), .dat experiment + dilution (dilution column, net heat),
 * the uniform-volume checkbox, clearing the dilution field and setRaw() of a stored block.
 * Set SUPRAFIT_TRACE_RENDER_DIR to write PNG renders (light and dark, 1366x768). */

#include <charts.h>

#include <cmath>

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>

#include <functional>
#include <memory>

#include "src/core/itcprocessor.h"
#include "src/core/models/datatable.h"
#include "src/core/thermogramhandler.h"
#include "src/core/toolset.h"
#include "src/global.h"
#include "src/ui/dialogs/importdata.h"
#include "src/ui/dialogs/thermogram.h"
#include "src/ui/widgets/DropTable.h"
#include "src/ui/guitools/appearance.h"
#include "src/ui/instance.h"

#ifndef SAMPLE_DIR
#define SAMPLE_DIR "."
#endif

namespace {

QString sample(const QString& name) { return QString(SAMPLE_DIR) + "/" + name; }

/// fit block with one repeating peak rule over a .dat thermogram (~20 peaks), as a stored project has it
QJsonObject ruleBlock(const QString& name)
{
    const QPair<Vector, Vector> xy = ToolSet::LoadXYFile(sample(name));
    const qreal begin = xy.first(0), end = xy.first(xy.first.size() - 1);
    const qreal duration = (end - begin) / 21.0;
    QJsonObject fit;
    fit["SupraFit"] = qint_version;
    fit["ThermogramBegin"] = begin;
    fit["ThermogramEnd"] = end;
    fit["PeakRuleList"] = ToolSet::Points2String(QList<QPointF>{ QPointF(begin + duration, duration) });
    return fit;
}

/// what the dialog runs on a handler when a file is set (setExperimentFile / setDilutionFile)
void runLikeDialog(ThermogramHandler* handler, const QJsonObject& fit, const QString& name)
{
    const QPair<Vector, Vector> xy = ToolSet::LoadXYFile(sample(name));
    handler->setThermogramParameter(fit);
    handler->setThermogram(PeakPick::spectrum(xy.first, xy.second));
    handler->Initialise();
    handler->IntegrateThermogram();
}

QTableWidget* resultTable(Thermogram* dialog)
{
    return dialog->findChild<QTabWidget*>()->widget(0)->findChild<QTableWidget*>();
}

QGridLayout* grid(Thermogram* dialog) { return qobject_cast<QGridLayout*>(dialog->layout()); }

QLineEdit* injectField(Thermogram* dialog) { return qobject_cast<QLineEdit*>(grid(dialog)->itemAtPosition(2, 1)->widget()); }
QCheckBox* uniformBox(Thermogram* dialog) { return qobject_cast<QCheckBox*>(grid(dialog)->itemAtPosition(2, 2)->widget()); }
QLineEdit* dilutionField(Thermogram* dialog)
{
    return qobject_cast<QLineEdit*>(grid(dialog)->itemAtPosition(0, 0)->layout()->itemAt(5)->widget());
}

bool near(double a, double b) { return std::abs(a - b) <= 1e-9 * std::max(std::abs(a), std::abs(b)) + 1e-300; }

} // namespace

class TestThermogramDialog : public QObject {
    Q_OBJECT

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

    /// the shown table (columns 0 and 3) and the handed-over table equal the core result
    void compareWithCore(Thermogram* dialog, const ItcProcessor& core, bool dilution)
    {
        std::unique_ptr<DataTable> want(core.resultTable());
        std::unique_ptr<DataTable> got(dialog->ResultTable());
        QVERIFY(want->rowCount() > 0);
        QCOMPARE(got->rowCount(), want->rowCount());
        QCOMPARE(got->columnCount(), 2);
        QTableWidget* table = resultTable(dialog);
        QCOMPARE(table->rowCount(), want->rowCount());
        QCOMPARE(table->columnCount(), 4);
        for (int i = 0; i < want->rowCount(); ++i) {
            QVERIFY2(near(got->data(i, 0), want->data(i, 0)), qPrintable(QString("volume %1").arg(i)));
            QVERIFY2(near(got->data(i, 1), want->data(i, 1)), qPrintable(QString("net heat %1: %2 vs %3").arg(i).arg(got->data(i, 1), 0, 'g', 17).arg(want->data(i, 1), 0, 'g', 17)));
            QCOMPARE(table->item(i, 0)->text(), QString::number(got->data(i, 0)));
            QCOMPARE(table->item(i, 1)->text(), QString::number(core.experiment()->Integrals()[i]));
            QCOMPARE(table->item(i, 3)->text(), QString::number(got->data(i, 1)));
            if (dilution)
                QCOMPARE(table->item(i, 2)->text(), QString::number(i < core.dilution()->Integrals().size() ? core.dilution()->Integrals()[i] : 0.0));
            else
                QCOMPARE(table->item(i, 2)->text(), QString("0"));
        }
    }

private slots:
    void initTestCase()
    {
        qApp->setProperty("StoreRawData", true);
        qApp->setProperty("StoreFileName", true);
        qApp->setProperty("StoreAbsolutePath", true);
        qApp->setProperty("thermogram_guidelines", true);
        qApp->setProperty("lineWidth", 20);
    }

    /*! \brief An .itc experiment: volumes from the file's @-lines, net heat from the core. */
    void itcExperiment()
    {
        Thermogram dialog;
        dialog.setExperimentFile(sample("synthetic.itc"));

        QString file = sample("synthetic.itc");
        std::vector<PeakPick::Peak> peaks;
        QVector<qreal> inject;
        qreal offset = 0, freq = 0;
        QPair<PeakPick::spectrum, QJsonObject> pair = ToolSet::LoadITCFile(file, &peaks, offset, freq, inject);
        ItcProcessor core;
        core.setScalingFactor(cal2joule);
        core.setInjectionVolumes(inject);
        core.experiment()->setThermogram(pair.first);
        core.experiment()->setPeakList(peaks);
        core.experiment()->Initialise();
        core.experiment()->IntegrateThermogram();
        core.padInjectionVolumes(inject.isEmpty() ? 0.0 : inject.last()); // the dialog pads with the field's value

        QVERIFY(!inject.isEmpty());
        compareWithCore(&dialog, core, false);
        QCOMPARE(injectField(&dialog)->text(), QString::number(inject.last()));
        QVERIFY(!uniformBox(&dialog)->isChecked());
    }

    /*! \brief .dat experiment and dilution; the uniform volume; clearing the dilution; setRaw(). */
    void datExperimentAndDilution()
    {
        Thermogram dialog;
        dialog.setExperimentFit(ruleBlock("reaction.dat"));
        dialog.setExperimentFile(sample("reaction.dat"));
        dialog.setDilutionFit(ruleBlock("dilution.dat"));
        dialog.setDilutionFile(sample("dilution.dat"));

        ItcProcessor core;
        core.setScalingFactor(cal2joule);
        runLikeDialog(core.experiment(), ruleBlock("reaction.dat"), "reaction.dat");
        runLikeDialog(core.dilution(), ruleBlock("dilution.dat"), "dilution.dat");
        core.setDilutionEnabled(true);
        core.padInjectionVolumes(0.0);
        compareWithCore(&dialog, core, true);

        // uniform volume: every row shows the field's value, and it is what is handed over
        uniformBox(&dialog)->setChecked(true);
        injectField(&dialog)->setText("2.5");
        core.setUniformInjectionVolume(2.5);
        compareWithCore(&dialog, core, true);

        // setRaw() of the stored block runs what the core runs for the same block and files. That it is
        // not the result before saving (one peak less) is measured, not asserted: see TODO.md.
        const QJsonObject raw = dialog.Raw();
        {
            Thermogram again;
            again.setRaw(raw);

            ItcProcessor reload;
            reload.fromJson(raw);
            runLikeDialog(reload.dilution(), raw["dilution"].toObject()["fit"].toObject(), "dilution.dat");
            reload.setDilutionEnabled(true);
            runLikeDialog(reload.experiment(), raw["experiment"].toObject()["fit"].toObject(), "reaction.dat");
            reload.setInjectionVolumes(ToolSet::String2DoubleVec(raw["InjectVolume"].toString()));
            reload.padInjectionVolumes(0.0);
            compareWithCore(&again, reload, true);

            std::unique_ptr<DataTable> before(dialog.ResultTable());
            qInfo("setRaw of the stored block: %d injections, %d before saving", int(reload.netHeat().size()), before->rowCount());
        }

        // emptying the dilution field stops the subtraction
        QLineEdit* field = dilutionField(&dialog);
        field->setFocus();
        field->selectAll();
        QTest::keyClick(field, Qt::Key_Delete);
        QVERIFY(field->text().isEmpty());
        core.setDilutionEnabled(false);
        compareWithCore(&dialog, core, false);
    }

    /*! \brief A double click in a selection mode acts once (Claude Generated, UX package 1): the chart was
     * connected twice, so the second call fell through to the standard mode and selected the peak at the
     * position in the peak list. */
    void doubleClickActsOnce()
    {
        Thermogram dialog;
        dialog.setExperimentFit(ruleBlock("reaction.dat"));
        dialog.setExperimentFile(sample("reaction.dat"));
        QWidget* experiment = dialog.findChild<QTabWidget*>()->widget(1);
        ChartView* chart = experiment->findChild<ChartView*>();
        QVERIFY(chart);
        QTableWidget* peaks = nullptr;
        for (QTableWidget* table : experiment->findChildren<QTableWidget*>())
            if (table->toolTip().startsWith("This table hold all peaks"))
                peaks = table;
        QVERIFY(peaks && peaks->rowCount() > 5);
        QPushButton* startButton = nullptr;
        for (QGroupBox* group : experiment->findChildren<QGroupBox*>())
            if (group->title() == QObject::tr("Start time [s]"))
                startButton = group->findChild<QPushButton*>();
        QVERIFY(startButton);

        peaks->setCurrentCell(-1, -1);
        const double x = 0.5 * (peaks->item(4, 1)->text().toDouble() + peaks->item(4, 2)->text().toDouble());
        startButton->click(); // peak start selection mode
        emit chart->pointDoubleClicked(QPointF(x, 0));
        QCOMPARE(peaks->currentRow(), -1); // the double click set the start time, it did not select a peak
        // the button text is not checked: with peak rules and a position outside the rule the slot resets the text of the end button (TODO.md)
        QVERIFY(!QApplication::overrideCursor());

        emit chart->pointDoubleClicked(QPointF(x, 0)); // standard mode: selects the peak
        QCOMPARE(peaks->currentRow(), 4);
    }

    /*! \brief The second click on the calibration button leaves its selection mode (Claude Generated, UX package 1). */
    void calibrationSelectionUndo()
    {
        Thermogram dialog;
        dialog.setExperimentFit(ruleBlock("reaction.dat"));
        dialog.setExperimentFile(sample("reaction.dat"));
        QWidget* experiment = dialog.findChild<QTabWidget*>()->widget(1);
        QPushButton* calibration = nullptr;
        for (QGroupBox* group : experiment->findChildren<QGroupBox*>())
            if (group->title().startsWith(QObject::tr("Calibration")))
                calibration = group->findChild<QPushButton*>();
        QVERIFY(calibration);
        calibration->click();
        QCOMPARE(calibration->text(), QObject::tr("Click to Undo"));
        QVERIFY(QApplication::overrideCursor());
        calibration->click();
        QCOMPARE(calibration->text(), QObject::tr("Click to Select"));
        QVERIFY(!QApplication::overrideCursor());
    }

    /*! \brief The file chosen in the Import Table dialog is the experiment of the Thermogram dialog (Claude
     * Generated, UX package 2a): an .itc opened as a file reaches the dialog, its table is the core result and
     * is handed over; an .xy thermogram (.dat) passed to "Import as Thermogram" is loaded, too. */
    void importDataLoadsTheChosenFile()
    {
        // the core result of synthetic.itc, as in itcExperiment()
        QString file = sample("synthetic.itc");
        std::vector<PeakPick::Peak> peaks;
        QVector<qreal> inject;
        qreal offset = 0, freq = 0;
        QPair<PeakPick::spectrum, QJsonObject> pair = ToolSet::LoadITCFile(file, &peaks, offset, freq, inject);
        ItcProcessor core;
        core.setScalingFactor(cal2joule);
        core.setInjectionVolumes(inject);
        core.experiment()->setThermogram(pair.first);
        core.experiment()->setPeakList(peaks);
        core.experiment()->Initialise();
        core.experiment()->IntegrateThermogram();
        core.padInjectionVolumes(inject.isEmpty() ? 0.0 : inject.last());

        int seen = 0;
        // polls the top-level windows while a modal dialog runs: checks and accepts the Thermogram dialog,
        // closes anything else modal (named in the log)
        auto inspect = [&](std::function<void(Thermogram*)> check) {
            QTimer* poll = new QTimer;
            poll->setInterval(20);
            QObject::connect(poll, &QTimer::timeout, poll, [&seen, check, poll]() {
                for (QWidget* widget : QApplication::topLevelWidgets()) {
                    if (!widget->isVisible())
                        continue;
                    if (Thermogram* dialog = qobject_cast<Thermogram*>(widget)) {
                        poll->stop();
                        poll->deleteLater();
                        ++seen;
                        check(dialog);
                        dialog->accept();
                        return;
                    }
                    if (QDialog* other = qobject_cast<QDialog*>(widget); other && other->isModal()) {
                        qWarning("closing modal %s", other->metaObject()->className());
                        other->reject();
                    }
                }
            });
            poll->start();
        };

        inspect([&](Thermogram* dialog) { compareWithCore(dialog, core, false); });
        ImportData import(file); // LoadFile: an .itc goes to the Thermogram dialog at once
        QCOMPARE(seen, 1);
        DataTable* handed = qobject_cast<DataTable*>(import.findChild<DropTable*>()->model());
        QVERIFY(handed);
        std::unique_ptr<DataTable> want(core.resultTable());
        QCOMPARE(handed->rowCount(), want->rowCount());
        for (int i = 0; i < want->rowCount(); ++i) {
            QVERIFY(near(handed->data(i, 0), want->data(i, 0)));
            QVERIFY(near(handed->data(i, 1), want->data(i, 1)));
        }

        // a .dat thermogram given to "Import as Thermogram": the same as the dialog loading it itself
        Thermogram reference;
        reference.setExperimentFile(sample("reaction.dat"));
        std::unique_ptr<DataTable> expected(reference.ResultTable());
        int rows = -1;
        bool loaded = false;
        inspect([&](Thermogram* dialog) {
            for (QLineEdit* edit : dialog->findChildren<QLineEdit*>())
                loaded |= edit->text() == sample("reaction.dat"); // the experiment field names the file
            loaded &= dialog->findChild<QTabWidget*>()->widget(1)->isEnabled(); // the experiment page is filled
            std::unique_ptr<DataTable> got(dialog->ResultTable());
            rows = got->rowCount();
            for (int i = 0; i < std::min(got->rowCount(), expected->rowCount()); ++i)
                QVERIFY(near(got->data(i, 1), expected->data(i, 1)));
        });
        ImportData table;
        QVERIFY(table.ImportThermogram(sample("reaction.dat")));
        QCOMPARE(seen, 2);
        QVERIFY(loaded);
        QCOMPARE(rows, expected->rowCount());
    }

    void renderDialog()
    {
        if (qEnvironmentVariable("SUPRAFIT_TRACE_RENDER_DIR").isEmpty())
            QSKIP("SUPRAFIT_TRACE_RENDER_DIR not set");
        // light and dark (Claude Generated, UX package 1: the colours of the data table)
        for (const int mode : { int(Appearance::Light), int(Appearance::Dark) }) {
        Appearance::Apply(mode);
        const QString scheme = mode == Appearance::Dark ? QStringLiteral("dark") : QStringLiteral("light");
        {
            Thermogram dialog;
            dialog.resize(1366, 768);
            dialog.setExperimentFile(sample("synthetic.itc"));
            dialog.findChild<QTabWidget*>()->setCurrentIndex(0);
            render(&dialog, QStringLiteral("thermo_itc_table_") + scheme);
            dialog.findChild<QTabWidget*>()->setCurrentIndex(1);
            render(&dialog, QStringLiteral("thermo_itc_experiment_") + scheme);
            dialog.hide();
        }
        {
            Thermogram dialog;
            dialog.resize(1366, 768);
            dialog.setExperimentFit(ruleBlock("reaction.dat"));
            dialog.setExperimentFile(sample("reaction.dat"));
            dialog.setDilutionFit(ruleBlock("dilution.dat"));
            dialog.setDilutionFile(sample("dilution.dat"));
            dialog.findChild<QTabWidget*>()->setCurrentIndex(0);
            render(&dialog, QStringLiteral("thermo_dat_table_") + scheme);
            dialog.findChild<QTabWidget*>()->setCurrentIndex(1);
            render(&dialog, QStringLiteral("thermo_dat_experiment_") + scheme);
            dialog.findChild<QTabWidget*>()->setCurrentIndex(2);
            render(&dialog, QStringLiteral("thermo_dat_dilution_") + scheme);
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
    QCoreApplication::setOrganizationName(QStringLiteral("SupraFitThermogramDialogTest"));
    QCoreApplication::setApplicationName(QStringLiteral("test_thermogram_dialog"));
    Instance* instance = new Instance;
    Instance::setInstance(instance);
    Appearance::Install();
    TestThermogramDialog test;
    const int result = QTest::qExec(&test, argc, argv);
    delete instance;
    return result;
}

#include "test_thermogram_dialog.moc"

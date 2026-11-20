/*
 * SupraFit - offscreen tests of the UV/Vis spectra import dialog and the import entries of the main window
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

/* Claude Generated (UX package 1 of the import dialogs). Synthetic UV/Vis spectra in a QTemporaryDir, the
 * dialog driven through its API and QTest keys (no file dialogs, no mouse):
 * - a cancelled file/directory dialog keeps the loaded spectra;
 * - pasted concentrations reach the table at once; a typed x value that is no number is refused visibly;
 * - the status line and OK follow the readiness (spectra, x values, rows of the concentrations);
 * - ImportKeyGuard: Enter in an input does not accept, ESC in the chart does not close, Cancel asks only
 *   after a change (the question is answered by a handler, no modal box);
 * - the hint line of the trace editor follows the setting "thermogram_guidelines";
 * - the main window: "Open Spectra" hands the table of the dialog over, "Open NMR Spectra" opens NmrImport.
 * Set SUPRAFIT_TRACE_RENDER_DIR to write PNG renders (light and dark, 1366x768). */

#include <charts.h>

#include <cmath>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableView>

#include "src/core/models/datatable.h"
#include "src/core/spectrahandler.h"
#include "src/ui/dialogs/importdata.h"
#include "src/ui/dialogs/nmrimport.h"
#include "src/ui/dialogs/spectraimport.h"
#include "src/ui/guitools/appearance.h"
#include "src/ui/guitools/importkeyguard.h"
#include "src/ui/guitools/statusline.h"
#include "src/ui/instance.h"
#include "src/ui/mainwindow/suprafitgui.h"
#include "src/ui/widgets/DropTable.h"
#include "src/ui/widgets/spectrawidget.h"
#include "src/ui/widgets/traceeditorwidget.h"

namespace {
/// a UV/Vis band growing with the step and a second one shrinking, 300..700 nm in 2 nm steps (201 points)
void writeSpectrum(const QString& name, int step)
{
    QFile file(name);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream stream(&file);
    for (int i = 0; i < 201; ++i) {
        const double x = 300 + 2.0 * i;
        const double y = (step + 1) * std::exp(-std::pow((x - 440) / 16.0, 2)) + 0.25 * (9 - step) * std::exp(-std::pow((x - 580) / 24.0, 2)) + 0.02 * step;
        stream << QString::number(x, 'g', 17) << " " << QString::number(1000 * y, 'g', 17) << "\n";
    }
}

/// tab-separated concentration rows (host, guest) as copied from a spreadsheet
QString concentrationText(int rows)
{
    QString text;
    for (int k = 0; k < rows; ++k)
        text += QString("%1\t%2\n").arg(1e-3).arg(2e-4 * k);
    return text;
}

QPushButton* okButton(QDialog* dialog)
{
    return dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok);
}
} // namespace

class TestSpectraImport : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;
    QString m_spectra; ///< directory with 4 spectra

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

    void paste(DropTable* table, const QString& text)
    {
        QApplication::clipboard()->setText(text);
        table->setFocus();
        QTest::keyClick(table, Qt::Key_V, Qt::ControlModifier);
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        qApp->setProperty("StoreRawData", true);
        qApp->setProperty("thermogram_guidelines", true);
        m_spectra = m_dir.filePath("uv");
        QDir().mkpath(m_spectra);
        for (int k = 0; k < 4; ++k)
            writeSpectrum(QDir(m_spectra).filePath(QString("uv_%1.dat").arg(k)), k);
    }

    /// A1: an empty choice of the file dialogs leaves the loaded spectra alone
    void cancelledFileDialogKeepsSpectra()
    {
        SpectraImport dialog;
        dialog.openDirectory(m_spectra, ".dat");
        QCOMPARE(dialog.spectraWidget()->spectraCount(), 4);
        dialog.openDirectory(QString(), ".dat");
        QCOMPARE(dialog.spectraWidget()->spectraCount(), 4);
        dialog.openFile(QString());
        QCOMPARE(dialog.spectraWidget()->spectraCount(), 4);
    }

    /// A3/A4: pasted concentrations reach the table; an x value that is no number is refused with a message
    void concentrationsAndXValues()
    {
        SpectraImport dialog;
        dialog.openDirectory(m_spectra, ".dat");
        SpectraWidget* widget = dialog.spectraWidget();
        QLineEdit* x = nullptr;
        for (QLineEdit* edit : widget->findChildren<QLineEdit*>())
            if (edit->placeholderText() == QObject::tr("Enter x value!"))
                x = edit;
        QVERIFY(x);

        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        x->setFocus();
        x->setText("abc");
        QTest::keyClick(x, Qt::Key_Return);
        QVERIFY(dialog.isVisible()); // Enter did not accept
        QCOMPARE(widget->xValueCount(), 0);
        QCOMPARE(dialog.statusLine()->level(), StatusLine::Error);
        QVERIFY(dialog.statusLine()->message().contains("abc"));

        x->setText("450,5");
        QTest::keyClick(x, Qt::Key_Return);
        QCOMPARE(widget->xValueCount(), 1);
        QVERIFY(x->text().isEmpty());
        QVERIFY(widget->addXValueText("580"));
        QCOMPARE(DataTable(widget->InputTable()).columnCount(), 2);

        DropTable* indep = widget->findChild<DropTable*>("spectraConcentrations");
        QVERIFY(indep);
        paste(indep, concentrationText(4));
        QCOMPARE(widget->concentrationColumns(), 2);
        QCOMPARE(DataTable(widget->InputTable()).columnCount(), 4); // 2 concentrations + 2 x values, without a further action
        QCOMPARE(DataTable(widget->InputTable()).rowCount(), 4);
        QVERIFY(widget->ready());
        dialog.keyGuard()->setConfirmHandler([](const QString&) { return true; });
        dialog.hide();
    }

    /// A4: the status line and OK follow the readiness; a row mismatch asks before handing over
    void readinessAndOk()
    {
        SpectraImport dialog;
        QVERIFY(!okButton(&dialog)->isEnabled());
        QCOMPARE(dialog.statusLine()->level(), StatusLine::Warning);
        dialog.openDirectory(m_spectra, ".dat");
        QVERIFY(!okButton(&dialog)->isEnabled()); // spectra, but no x value
        QVERIFY(dialog.statusLine()->message().contains(QObject::tr("No x values").left(10)));
        QVERIFY(dialog.spectraWidget()->addXValueText("440"));
        QVERIFY(okButton(&dialog)->isEnabled());
        QCOMPARE(dialog.statusLine()->level(), StatusLine::Info);

        DropTable* indep = dialog.spectraWidget()->findChild<DropTable*>("spectraConcentrations");
        paste(indep, concentrationText(3));
        QCOMPARE(dialog.spectraWidget()->concentrationRows(), 3);
        QVERIFY(okButton(&dialog)->isEnabled());
        QCOMPARE(dialog.statusLine()->level(), StatusLine::Warning);

        QStringList asked;
        bool answer = false;
        dialog.keyGuard()->setConfirmHandler([&](const QString& question) {
            asked << question;
            return answer;
        });
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        dialog.accept();
        QCOMPARE(asked.size(), 1);
        QVERIFY(asked[0].contains("3 rows"));
        QVERIFY(dialog.isVisible());
        answer = true;
        dialog.accept();
        QVERIFY(!dialog.isVisible());
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
    }

    /// B: Enter in a spin box keeps the dialog open, ESC in the chart only ends its drag, Cancel asks only after a change
    void keyboardRuleAndDiscardQuestion()
    {
        SpectraImport dialog;
        dialog.openDirectory(m_spectra, ".dat");
        QVERIFY(dialog.spectraWidget()->addXValueText("440")); // OK enabled: Enter would press it without the guard
        QStringList asked;
        bool answer = false;
        dialog.keyGuard()->setConfirmHandler([&](const QString& question) {
            asked << question;
            return answer;
        });
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        QVERIFY(!dialog.keyGuard()->isDirty()); // what it was opened with is the unchanged state
        QVERIFY(!okButton(&dialog)->isDefault() && !okButton(&dialog)->autoDefault());

        TraceEditorWidget* editor = dialog.findChild<TraceEditorWidget*>();
        QDoubleSpinBox* start = editor->findChildren<QDoubleSpinBox*>().value(0);
        QVERIFY(start);
        start->setFocus();
        QTest::keyClick(start, Qt::Key_Return);
        QTest::keyClick(start, Qt::Key_Enter);
        QVERIFY(dialog.isVisible());

        // ESC in the chart ends a pending drag there and does not reach the dialog
        editor->chart()->privateView()->setFocus();
        QTest::keyClick(editor->chart()->privateView(), Qt::Key_Escape);
        QVERIFY(dialog.isVisible());

        // unchanged: Cancel closes without a question
        dialog.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel)->click();
        QVERIFY(!dialog.isVisible());
        QCOMPARE(asked.size(), 0);

        // changed: ESC at the dialog asks; "Keep editing" keeps it open, "Discard" closes
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        QVERIFY(dialog.spectraWidget()->addXValueText("580"));
        QVERIFY(dialog.keyGuard()->isDirty());
        QTest::keyClick(&dialog, Qt::Key_Escape);
        QCOMPARE(asked.size(), 1);
        QCOMPARE(asked[0], QObject::tr("Discard the changes?"));
        QVERIFY(dialog.isVisible());
        answer = true;
        dialog.close();
        QCOMPARE(asked.size(), 2);
        QVERIFY(!dialog.isVisible());
        QCOMPARE(dialog.result(), int(QDialog::Rejected));
    }

    /// C: the hint line follows the setting and names the double-click of the host
    void hintLineFollowsTheSetting()
    {
        for (bool on : { true, false }) {
            qApp->setProperty("thermogram_guidelines", on);
            SpectraImport dialog;
            dialog.show();
            QVERIFY(QTest::qWaitForWindowExposed(&dialog));
            StatusLine* hint = dialog.findChild<StatusLine*>("traceEditorHint");
            QVERIFY(hint);
            QCOMPARE(hint->isVisible(), on);
            QVERIFY(hint->message().contains(QObject::tr("Double-click: add an x value")));
            dialog.hide();
        }
        qApp->setProperty("thermogram_guidelines", true);
    }

    /// A2 and E: the entries of the main window. "Open Spectra" hands the table of the dialog to the Import Table
    /// dialog (it was empty before); "Open NMR Spectra" opens the NMR import.
    void mainWindowEntries()
    {
        struct ReinstallInstance {
            ~ReinstallInstance() { Instance::setInstance(new Instance); }
        } reinstallInstance;
        SupraFitGui gui;
        QAction* spectra = nullptr;
        QAction* nmr = gui.findChild<QAction*>("openNmrSpectra");
        for (QAction* action : gui.findChildren<QAction*>())
            if (action->text() == QObject::tr("Open Spectra"))
                spectra = action;
        QVERIFY(spectra && nmr);
        QCOMPARE(nmr->text(), QObject::tr("Open NMR Spectra"));

        QStringList seen;
        int handedColumns = -1;
        QTimer closer;
        closer.setInterval(50);
        QObject::connect(&closer, &QTimer::timeout, [&]() {
            QWidget* w = QApplication::activeModalWidget();
            if (!w)
                return;
            if (auto* import = qobject_cast<SpectraImport*>(w)) {
                seen << "SpectraImport";
                import->openDirectory(m_spectra, ".dat");
                import->spectraWidget()->addXValueText("440");
                import->spectraWidget()->addXValueText("580");
                import->accept();
            } else if (auto* table = qobject_cast<ImportData*>(w)) {
                seen << "ImportData";
                if (QTableView* view = table->findChild<QTableView*>())
                    handedColumns = view->model() ? view->model()->columnCount() : 0;
                table->reject();
            } else if (auto* dialog = qobject_cast<NmrImport*>(w)) {
                seen << "NmrImport";
                dialog->reject(); // unchanged: no question
            } else { // e.g. the first-start message of a fresh installation
                if (auto* box = qobject_cast<QMessageBox*>(w))
                    qInfo("closed a message box: %s", qPrintable(box->text().left(80)));
                w->close();
            }
        });
        closer.start();
        spectra->trigger();
        QCOMPARE(seen.value(0), QString("SpectraImport"));
        QCOMPARE(seen.value(1), QString("ImportData"));
        QCOMPARE(handedColumns, 2);
        seen.clear();
        nmr->trigger();
        QCOMPARE(seen, QStringList{ "NmrImport" });
        closer.stop();
    }

    void renderDialog()
    {
        if (qEnvironmentVariable("SUPRAFIT_TRACE_RENDER_DIR").isEmpty())
            QSKIP("SUPRAFIT_TRACE_RENDER_DIR not set");
        for (int mode : { int(Appearance::Light), int(Appearance::Dark) }) {
            Appearance::Apply(mode);
            const QString scheme = mode == Appearance::Dark ? QStringLiteral("dark") : QStringLiteral("light");
            SpectraImport dialog;
            dialog.resize(1366, 768);
            dialog.openDirectory(m_spectra, ".dat");
            render(&dialog, "uvvis_no_x_" + scheme);
            dialog.spectraWidget()->addXValueText("440");
            dialog.spectraWidget()->addXValueText("580");
            paste(dialog.spectraWidget()->findChild<DropTable*>("spectraConcentrations"), concentrationText(4));
            render(&dialog, "uvvis_ready_" + scheme);
            dialog.keyGuard()->setConfirmHandler([](const QString&) { return true; });
            dialog.hide();
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
    QCoreApplication::setOrganizationName(QStringLiteral("SupraFitSpectraImportTest"));
    QCoreApplication::setApplicationName(QStringLiteral("test_spectraimport"));
    Instance::setInstance(new Instance);
    Appearance::Install();
    TestSpectraImport test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_spectraimport.moc"

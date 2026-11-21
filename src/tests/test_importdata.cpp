/*
 * SupraFit - offscreen tests of the table import dialog (ImportData)
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

/* Claude Generated (R0 of roadmap/data_pipeline_work_packages.md). The dialog is driven through its API, no
 * file dialogs:
 * - "Store Recipe" / "Load Recipe" of "Simulate Data": a stored recipe and a control file whose block "main"
 *   holds the recipe give the same equations, numbers of columns/points and the evaluated table. */

#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>

#include "src/core/jsonhandler.h"
#include "src/core/models/datatable.h"
#include "src/ui/dialogs/importdata.h"
#include "src/ui/instance.h"
#include "src/ui/widgets/DropTable.h"

namespace {
QJsonObject recipe()
{
    QJsonObject data;
    data["independent"] = 2;
    data["datapoints"] = 7;
    data["dependent"] = 3;
    data["equations"] = QStringLiteral("X|X*X");
    return data;
}

/// switches the dialog to "Simulate Data" as a user does before loading a recipe
void enableSimulation(ImportData& dialog)
{
    for (QCheckBox* box : dialog.findChildren<QCheckBox*>())
        if (box->text().startsWith(QStringLiteral("Simulate Data")))
            box->setChecked(true);
}

/// the evaluated recipe(): 7 rows, X and X*X
void checkEvaluated(ImportData& dialog)
{
    QCOMPARE(dialog.Generator(), recipe());
    DropTable* table = dialog.findChild<DropTable*>();
    QVERIFY(table);
    DataTable* model = qobject_cast<DataTable*>(table->model());
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 7);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(6, 0), 7.0);
    QCOMPARE(model->data(6, 1), 49.0);
}
}

class TestImportData : public QObject {
    Q_OBJECT

private slots:
    /// Store Recipe writes Generator(); Load Recipe of that file restores it in a fresh dialog
    void storedRecipeIsLoaded()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString file = dir.filePath("recipe.json");
        QVERIFY(JsonHandler::WriteJsonFile(recipe(), file));

        ImportData dialog;
        enableSimulation(dialog);
        QVERIFY(dialog.LoadRecipe(file));
        checkEvaluated(dialog);

        // and once more through the dialog's own Store Recipe content
        const QString again = dir.filePath("again.json");
        QVERIFY(JsonHandler::WriteJsonFile(dialog.Generator(), again));
        ImportData second;
        enableSimulation(second);
        QVERIFY(second.LoadRecipe(again));
        checkEvaluated(second);
    }

    /// a recipe inside a block "main" (any case) is read from that block of the loaded file
    void recipeInMainBlockIsLoaded()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        for (const QString& key : { QStringLiteral("main"), QStringLiteral("Main") }) {
            QJsonObject wrapped;
            wrapped[key] = recipe();
            const QString file = dir.filePath(key + ".json");
            QVERIFY(JsonHandler::WriteJsonFile(wrapped, file));

            ImportData dialog;
            enableSimulation(dialog);
            QVERIFY(dialog.LoadRecipe(file));
            checkEvaluated(dialog);
        }
    }
};

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    QTemporaryDir settingsDir;
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, settingsDir.path());
    QCoreApplication::setOrganizationName(QStringLiteral("SupraFitImportDataTest"));
    QCoreApplication::setApplicationName(QStringLiteral("test_importdata"));
    Instance::setInstance(new Instance);
    TestImportData test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_importdata.moc"

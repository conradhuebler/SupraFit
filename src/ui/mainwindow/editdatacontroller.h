/*
 * EditDataController - Edit Data of a project: dialog, compatibility check, replacement, undo
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
 * Claude Generated (Edit Data, E1).
 */

#pragma once

#include "src/core/projectmanager.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>

#include <functional>

class DataClass;
class QMessageBox;
class QTimer;
class QWidget;

/*! \brief Edit Data of a thermogram or spectra project, the GUI shell around the core replacement (Claude Generated).
 *
 * Workflow of edit():
 *  1. a job (statistics, search) runs on a model of the project: ask "Stop the running analyses and replace the data?";
 *     yes interrupts them (`JobManager::interruptJobs`) and continues once they ended (the jobs run below the caller on the
 *     stack, so the controller waits on a timer: Outcome::Deferred, then finished()); no ends here;
 *  2. the import dialog of the project (ITC: thermogram dialog, UV/Vis: SpectraImport, NMR: NmrImport) gives the new data
 *     block: ITC as `ImportData::getStoredData()`, UV/Vis and NMR from the table of the dialog (`DataClass::ImportTableData`)
 *     without the second "Import Table" dialog;
 *  3. apply(): `DataClass::checkReplacement` — Identical: "no change" in the log; Compatible: `replaceProjectData(KeepModels)`;
 *     Incompatible: the question with the reasons (default "Create a new project": newProjectRequested(), "Remove the models and
 *     replace the data": DropModels, "Cancel");
 *  4. the summary of the report goes to the log and dataReplaced() tells the window to update its panes.
 * undo() takes the last replacement back (`undoLastDataReplacement`). The dialog step, the incompatible question and the stop
 * question can be replaced (tests); the defaults are modal dialogs on the parent widget.
 */
class EditDataController : public QObject {
    Q_OBJECT
public:
    /*! \brief The answer to the question for incompatible data. */
    enum class Choice {
        NewProject = 0,
        DropModels = 1,
        Cancel = 2
    };
    /*! \brief What edit() / apply() did. */
    enum class Outcome {
        Cancelled = 0, ///< the dialog or a question was cancelled
        Unchanged = 1, ///< the new data are those of the project
        Replaced = 2, ///< the data were replaced (models kept or removed)
        NewProject = 3, ///< a new project is to be made from the new data (newProjectRequested)
        Refused = 4, ///< the core refused (the report says why)
        Deferred = 5 ///< waiting for stopped jobs; finished() follows
    };
    Q_ENUM(Outcome)
    /*! \brief The result of an import dialog: the new data block and, for spectra, the table and raw block of the dialog. */
    struct Input {
        QJsonObject data; ///< the data block as `DataClass::ExportData` writes it
        QJsonObject table; ///< spectra: the table of the dialog (independent and dependent columns)
        QJsonObject raw; ///< spectra: the raw block of the dialog
        int independent = 2; ///< spectra: the number of independent columns of the table
    };
    using DialogHandler = std::function<bool(QWidget* parent, const QSharedPointer<DataClass>& project, Input& input)>;
    using ChoiceHandler = std::function<Choice(QWidget* parent, const QString& title, const DataReplaceCheck& check)>;
    using ConfirmHandler = std::function<bool(QWidget* parent, const QString& title, const QString& text)>;

    explicit EditDataController(QWidget* parent = nullptr);

    /*! \brief Whether Edit Data of this project opens an import dialog (thermogram, spectra); tables are edited in place. */
    static bool usesDialog(const QSharedPointer<DataClass>& project);

    /*! \brief Edit Data of project \p projectId: steps 1-4 of the class description. */
    Outcome edit(const QString& projectId);
    /*! \brief Steps 3-4 with the new data \p input (as an import dialog gives them). */
    Outcome apply(const QString& projectId, const Input& input);
    /*! \brief Takes the last replacement of the data back; false (with a message in the log) if there is none or it is refused. */
    bool undo(const QString& projectId);
    /*! \brief Whether undo() has something to restore. */
    static bool canUndo(const QString& projectId);

    /*! \brief The report of the last replacement (or refusal). */
    inline const SupraFit::DataReplaceReport& lastReport() const { return m_report; }

    inline void setDialogHandler(const DialogHandler& handler) { m_dialog = handler; }
    inline void setChoiceHandler(const ChoiceHandler& handler) { m_choice = handler; }
    inline void setConfirmHandler(const ConfirmHandler& handler) { m_confirm = handler; }
    /*! \brief How long to wait for interrupted jobs before giving up (default 60 s). */
    inline void setJobWaitLimit(int msecs) { m_wait_limit = msecs; }

    /*! \brief The import dialog of the project, modal on \p parent (the default dialog handler). */
    static bool RunImportDialog(QWidget* parent, const QSharedPointer<DataClass>& project, Input& input);
    /*! \brief The question for incompatible data, not shown yet: the reasons of \p check in the text, "Create a new project" the
     *  default button. The buttons carry the object names "editNewProject", "editDropModels" and "editCancel". */
    static QMessageBox* IncompatibleQuestion(QWidget* parent, const QString& title, const DataReplaceCheck& check);

signals:
    void Message(const QString& text, int priority);
    void Warning(const QString& text, int priority);
    /*! \brief A new project from the new data: \p data is the complete block; \p table and \p raw are set for spectra. */
    void newProjectRequested(const QJsonObject& table, const QJsonObject& raw, const QJsonObject& data);
    /*! \brief The data of the project were replaced or the replacement was undone; \p summary says what happened. */
    void dataReplaced(const QString& projectId, const QString& summary);
    /*! \brief An edit() that returned Deferred has ended. */
    void finished(const QString& projectId, EditDataController::Outcome outcome);

private:
    Outcome startEdit(const QString& projectId);
    void waitForJobs();

    QWidget* m_parent;
    DialogHandler m_dialog;
    ChoiceHandler m_choice;
    ConfirmHandler m_confirm;
    SupraFit::DataReplaceReport m_report;
    QTimer* m_wait = nullptr;
    QString m_wait_project;
    QElapsedTimer m_wait_clock;
    int m_wait_limit = 60000;
};

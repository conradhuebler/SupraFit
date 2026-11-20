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

#include "editdatacontroller.h"

#include "src/capabilities/jobmanager.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/nmrhandler.h"

#include "src/ui/dialogs/importdata.h"
#include "src/ui/dialogs/nmrimport.h"
#include "src/ui/dialogs/spectraimport.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QTimer>

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

using SupraFit::DataReplacePolicy;
using SupraFit::DataReplaceReport;
using SupraFit::LoadIssue;
using SupraFit::ProjectManager;

EditDataController::EditDataController(QWidget* parent)
    : QObject(parent)
    , m_parent(parent)
{
    m_dialog = &EditDataController::RunImportDialog;
    m_choice = [](QWidget* parent, const QString& title, const DataReplaceCheck& check) {
        QMessageBox* box = IncompatibleQuestion(parent, title, check);
        box->exec();
        const QString name = box->clickedButton() ? box->clickedButton()->objectName() : QString();
        delete box;
        if (name == QLatin1String("editNewProject"))
            return Choice::NewProject;
        if (name == QLatin1String("editDropModels"))
            return Choice::DropModels;
        return Choice::Cancel;
    };
    m_confirm = [](QWidget* parent, const QString& title, const QString& text) {
        return QMessageBox::question(parent, title, text, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes;
    };
}

bool EditDataController::usesDialog(const QSharedPointer<DataClass>& project)
{
    return project && (project->DataType() == DataClassPrivate::Thermogram || project->DataType() == DataClassPrivate::Spectrum);
}

bool EditDataController::canUndo(const QString& projectId)
{
    return !projectId.isEmpty() && ProjectManager::instance().canUndoDataReplacement(projectId);
}

/*! \brief The import dialog that made the project, filled with its data; on OK \p input holds the new data block. */
bool EditDataController::RunImportDialog(QWidget* parent, const QSharedPointer<DataClass>& project, Input& input)
{
    if (!project)
        return false;
    if (project->DataType() == DataClassPrivate::Thermogram) {
        // the thermogram dialog and the table of ImportData, as for a new thermogram project
        ImportData dialog(project.toWeakRef());
        dialog.setRootDir(project->RootDir());
        if (dialog.exec() != QDialog::Accepted)
            return false;
        input.data = dialog.getStoredData().ExportData();
        return true;
    }
    if (project->DataType() != DataClassPrivate::Spectrum)
        return false;
    if (NmrHandler::isNmrBlock(project->RawData())) {
        NmrImport dialog(parent);
        dialog.setData(project->ExportData());
        if (dialog.exec() != QDialog::Accepted)
            return false;
        input.table = dialog.InputTable();
        input.raw = dialog.ProjectData();
        input.independent = dialog.IndependentCount();
    } else {
        SpectraImport dialog;
        if (parent)
            dialog.setParent(parent, dialog.windowFlags());
        dialog.setData(project->ExportData());
        if (dialog.exec() != QDialog::Accepted)
            return false;
        input.table = dialog.InputTable();
        input.raw = dialog.ProjectData();
        // the columns the project has: the models of the project are bound to them (checkReplacement)
        input.independent = project->IndependentModel() ? project->IndependentModel()->columnCount() : 2;
    }
    // the table of the dialog becomes the data block without the "Import Table" dialog
    DataTable table(input.table);
    input.data = DataClass::ImportTableData(&table, input.independent, DataClassPrivate::Spectrum, input.raw, project->ProjectTitle());
    return !input.data.isEmpty();
}

QMessageBox* EditDataController::IncompatibleQuestion(QWidget* parent, const QString& title, const DataReplaceCheck& check)
{
    QMessageBox* box = new QMessageBox(parent);
    box->setIcon(QMessageBox::Question);
    box->setWindowTitle(tr("Edit Data"));
    box->setText(tr("The new data do not fit the models of %1.").arg(title.toHtmlEscaped()));
    QString info = check.reasons.join(QLatin1Char('\n'));
    info += QLatin1String("\n\n") + tr("Create a new project from the new data, or remove the models of this project and replace its data?");
    box->setInformativeText(info);
    QPushButton* create = box->addButton(tr("Create a new project"), QMessageBox::AcceptRole);
    create->setObjectName(QStringLiteral("editNewProject"));
    QPushButton* drop = box->addButton(tr("Remove the models and replace the data"), QMessageBox::DestructiveRole);
    drop->setObjectName(QStringLiteral("editDropModels"));
    QPushButton* cancel = box->addButton(QMessageBox::Cancel);
    cancel->setObjectName(QStringLiteral("editCancel"));
    box->setDefaultButton(create);
    box->setEscapeButton(cancel);
    return box;
}

EditDataController::Outcome EditDataController::edit(const QString& projectId)
{
    ProjectManager& pm = ProjectManager::instance();
    if (pm.hasRunningJobs(projectId)) {
        if (!m_confirm(m_parent, tr("Edit Data"), tr("Stop the running analyses and replace the data?"))) {
            emit Message(tr("Edit Data cancelled: analyses are running on the project."), 3);
            return Outcome::Cancelled;
        }
        const int asked = JobManager::interruptJobs(projectId);
        emit Message(tr("Stopping %1 running analyses before the data are edited.").arg(asked), 3);
        m_wait_project = projectId;
        m_wait_clock.start();
        if (!m_wait) {
            m_wait = new QTimer(this);
            m_wait->setInterval(100);
            connect(m_wait, &QTimer::timeout, this, &EditDataController::waitForJobs);
        }
        m_wait->start();
        return Outcome::Deferred;
    }
    return startEdit(projectId);
}

/*! \brief Runs on the timer until the interrupted jobs of the waiting project ended, then opens the dialog. */
void EditDataController::waitForJobs()
{
    const QString projectId = m_wait_project;
    if (ProjectManager::instance().hasRunningJobs(projectId)) {
        if (m_wait_clock.elapsed() < m_wait_limit)
            return;
        m_wait->stop();
        emit Warning(tr("The analyses did not stop; the data were not changed."), 1);
        emit finished(projectId, Outcome::Cancelled);
        return;
    }
    m_wait->stop();
    // the clones of a finished job share the data of the project until they are deleted (deleteLater)
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    const Outcome outcome = startEdit(projectId);
    emit finished(projectId, outcome);
}

EditDataController::Outcome EditDataController::startEdit(const QString& projectId)
{
    QSharedPointer<DataClass> project = ProjectManager::instance().getProjectData(projectId);
    if (!project) {
        emit Warning(tr("Edit Data: the project is not open."), 1);
        return Outcome::Refused;
    }
    Input input;
    if (!m_dialog || !m_dialog(m_parent, project, input) || input.data.isEmpty())
        return Outcome::Cancelled;
    return apply(projectId, input);
}

EditDataController::Outcome EditDataController::apply(const QString& projectId, const Input& input)
{
    ProjectManager& pm = ProjectManager::instance();
    QSharedPointer<DataClass> project = pm.getProjectData(projectId);
    if (!project) {
        emit Warning(tr("Edit Data: the project is not open."), 1);
        return Outcome::Refused;
    }
    const QString title = project->ProjectTitle();
    const DataReplaceCheck check = project->checkReplacement(input.data);

    DataReplacePolicy policy = DataReplacePolicy::KeepModels;
    if (check.result == DataReplaceCheck::Identical && !check.rawChanged) {
        emit Message(tr("Edit Data: no change in the data of %1.").arg(title), 3);
        return Outcome::Unchanged;
    }
    if (check.result == DataReplaceCheck::Incompatible) {
        const Choice choice = m_choice(m_parent, title, check);
        if (choice == Choice::Cancel) {
            emit Message(tr("Edit Data cancelled; the data of %1 are unchanged.").arg(title), 3);
            return Outcome::Cancelled;
        }
        if (choice == Choice::NewProject) {
            emit Message(tr("Edit Data: the new data of %1 become a new project.").arg(title), 3);
            emit newProjectRequested(input.table, input.raw, input.data);
            return Outcome::NewProject;
        }
        policy = DataReplacePolicy::DropModels;
    }

    m_report = pm.replaceProjectData(projectId, input.data, policy);
    if (!m_report.replaced) {
        if (m_report.HasErrors())
            emit Warning(tr("Edit Data of %1: %2").arg(title, m_report.Summary()), 1);
        else
            emit Message(tr("Edit Data of %1: %2").arg(title, m_report.Summary()), 3);
        return m_report.HasErrors() ? Outcome::Refused : Outcome::Unchanged;
    }
    const QString summary = tr("Edit Data of %1: %2").arg(title, m_report.Summary());
    emit Message(summary, 3);
    const QString details = m_report.Details();
    if (!details.isEmpty())
        for (const QString& line : details.split(QLatin1Char('\n')))
            emit Message(line, 4);
    emit dataReplaced(projectId, summary);
    return Outcome::Replaced;
}

bool EditDataController::undo(const QString& projectId)
{
    ProjectManager& pm = ProjectManager::instance();
    QSharedPointer<DataClass> project = pm.getProjectData(projectId);
    QString reason;
    if (!project || !pm.undoLastDataReplacement(projectId, &reason)) {
        emit Warning(tr("Undo Edit Data: %1").arg(reason.isEmpty() ? tr("the project is not open.") : reason), 1);
        return false;
    }
    const QString summary = tr("Undo Edit Data: the data and models of %1 are as before the last Edit Data.").arg(project->ProjectTitle());
    emit Message(summary, 3);
    emit dataReplaced(projectId, summary);
    return true;
}

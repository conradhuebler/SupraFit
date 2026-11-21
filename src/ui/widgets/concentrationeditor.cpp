/*
 * ConcentrationEditor - the concentration table of the spectra and NMR import dialogs
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
 * Claude Generated (UX package 2a of the import dialogs).
 */

#include <QtCore/QFileInfo>
#include <QtCore/QPointer>

#include <QtGui/QAction>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include "src/core/filehandler.h"
#include "src/core/models/datatable.h"
#include "src/global.h"
#include "src/ui/guitools/statusline.h"
#include "src/ui/widgets/DropTable.h"

#include "concentrationeditor.h"

ConcentrationEditor::ConcentrationEditor(const QString& title, QWidget* parent)
    : QWidget(parent)
{
    m_title = new QLabel(title);
    m_title->setWordWrap(true);
    m_hint = new QLabel(tr("Paste with Ctrl+V (Ctrl+B appends, Ctrl+Y prepends columns)"));
    m_hint->setObjectName("concentrationHint");
    m_hint->setWordWrap(true);
    m_load = new QPushButton(tr("Load from file"));
    m_load->setObjectName("concentrationLoad");
    m_load->setToolTip(tr("Reads a table file (one row per spectrum; separator detected, decimal comma accepted) and replaces the table."));
    m_table = new DropTable;
    m_table->setToolTip(tr("Concentrations, one row per spectrum: copy a table and paste it here with Ctrl+V (Ctrl+B appends columns, Ctrl+Y puts them in front)."));
    m_status = new StatusLine;
    m_status->setObjectName("concentrationStatus");

    QHBoxLayout* top = new QHBoxLayout;
    top->setContentsMargins(0, 0, 0, 0);
    top->addWidget(m_hint, 1);
    top->addWidget(m_load, 0, Qt::AlignTop);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_title);
    layout->addLayout(top);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_status);
    setLayout(layout);

    connect(m_table, &DropTable::Edited, this, [this]() {
        updateStatus();
        emit Edited();
    });
    // "Clear Table" of the context menu (connected after the table's own slot, so it runs after the clear)
    for (QAction* action : m_table->actions())
        connect(action, &QAction::triggered, this, [this]() {
            updateStatus();
            emit Edited();
        });
    connect(m_load, &QPushButton::clicked, this, [this]() {
        const QString file = QFileDialog::getOpenFileName(this, tr("Load from file"), getDir(), tr("Tables (*.txt *.csv *.dat);;All files (*)"));
        if (file.isEmpty()) // cancelled: the table stays
            return;
        setLastDir(file);
        QString error;
        loadFile(file, &error);
    });
    updateStatus();
}

DataTable* ConcentrationEditor::dataTable() const
{
    return qobject_cast<DataTable*>(m_table->model());
}

int ConcentrationEditor::rows() const
{
    const DataTable* table = dataTable();
    return table && table->isValid() && table->columnCount() > 0 ? table->rowCount() : 0;
}

int ConcentrationEditor::columns() const
{
    const DataTable* table = dataTable();
    return table && table->isValid() && table->rowCount() > 0 ? table->columnCount() : 0;
}

void ConcentrationEditor::adopt(DataTable* table)
{
    QAbstractItemModel* old = m_table->model();
    if (table)
        table->setParent(m_table);
    m_table->setTable(table);
    if (old && old != table && old->parent() == m_table)
        delete old;
}

void ConcentrationEditor::setTable(const DataTable* table)
{
    adopt(table ? new DataTable(*const_cast<DataTable*>(table)) : nullptr);
    updateStatus();
    emit Edited();
}

bool ConcentrationEditor::loadFile(const QString& file, QString* error)
{
    // the reader of "Open Project" for table files: separator detected, decimal comma, header row
    FileHandler handler(file);
    handler.LoadFile();
    QPointer<DataTable> table = handler.getData();
    QString reason;
    if (!QFileInfo(file).isReadable())
        reason = tr("%1 cannot be read.").arg(QFileInfo(file).fileName());
    else if (!table || !table->isValid() || table->rowCount() == 0 || table->columnCount() == 0)
        reason = tr("%1 holds no table of numbers.").arg(QFileInfo(file).fileName());
    if (!reason.isEmpty()) {
        delete table;
        m_status->setMessage(reason, StatusLine::Error);
        if (error)
            *error = reason;
        return false;
    }
    adopt(table);
    updateStatus();
    emit Edited();
    return true;
}

void ConcentrationEditor::setSpectraCount(int count, int used)
{
    m_spectra = count;
    m_used = used >= 0 && used != count ? used : -1;
    updateStatus();
}

void ConcentrationEditor::updateStatus()
{
    const int n = rows();
    if (n == 0)
        m_status->setMessage(m_spectra > 0 ? tr("No concentrations yet for %1 spectra.").arg(m_spectra) : tr("No concentrations yet."), StatusLine::Info);
    else if (m_used >= 0 && n == m_used)
        m_status->setMessage(tr("%1 rows for %2 used spectra").arg(n).arg(m_used), StatusLine::Info);
    else if (m_spectra > 0 && n != m_spectra)
        m_status->setMessage(m_used >= 0 ? tr("%1 rows for %2 spectra (%3 used)\nOne row per spectrum or per used spectrum is needed.").arg(n).arg(m_spectra).arg(m_used)
                                         : tr("%1 rows for %2 spectra\nOne row per spectrum is needed.").arg(n).arg(m_spectra),
            StatusLine::Warning);
    else
        m_status->setMessage(tr("%1 rows for %2 spectra").arg(n).arg(m_spectra), StatusLine::Info);
}

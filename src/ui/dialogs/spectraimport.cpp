/*
 * SpectraImport - dialog of the spectra import and of editing a spectra project again
 * Copyright (C) 2020 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

#include "src/ui/guitools/importkeyguard.h"
#include "src/ui/guitools/statusline.h"

#include <src/ui/widgets/spectrawidget.h>

#include "src/global.h"

#include "spectraimport.h"

SpectraImport::SpectraImport()
{
    setUI();
}

SpectraImport::SpectraImport(const QString& directory)
{
    QStringList path = directory.split("|||");
    setUI();
    if (path.size() == 2) {
        m_path->setText(path[0]);
        m_spectrawidget->setDirectory(path[0], path[1]);
    } else {
        m_path->setText(directory);
        m_spectrawidget->setDirectory(directory, "csv");
    }
}

void SpectraImport::setUI()
{
    QGridLayout* layout = new QGridLayout;

    m_guard = new ImportKeyGuard(this);

    m_file = new QPushButton(tr("Load file"));
    m_file->setToolTip(tr("Reads one file: a single spectrum or a table of spectra."));
    connect(m_file, &QPushButton::clicked, this, &SpectraImport::setFile);

    m_directory = new QPushButton(tr("Open Directory"));
    m_directory->setToolTip(tr("Reads every file of a directory whose name contains the chosen ending, one spectrum per file, in natural order (2 before 10)."));
    connect(m_directory, &QPushButton::clicked, this, &SpectraImport::setDirectory);
    m_path = new QLineEdit;
    m_path->setReadOnly(true);

    m_spectrawidget = new SpectraWidget;

    m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &SpectraImport::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_file_type = new QComboBox;
    m_file_type->addItem("csv");
    m_file_type->addItem("absorb");
    m_file_type->addItem("xy");
    m_file_type->setCurrentText(qApp->instance()->property("LastSpectraType").toString());
    m_file_type->setToolTip(tr("Ending of the spectrum files read by \"Open Directory\": every file whose name contains it."));
    QLabel* type_label = new QLabel(tr("File ending"));
    type_label->setToolTip(m_file_type->toolTip());

    m_status = new StatusLine;
    m_status->setObjectName("spectraStatus");

    QHBoxLayout* load = new QHBoxLayout;
    load->addWidget(type_label);
    load->addWidget(m_file_type);
    load->addWidget(m_directory);
    load->addWidget(m_file);
    load->addWidget(m_path, 1);
    layout->addLayout(load, 0, 0, 1, 4);
    layout->addWidget(m_spectrawidget, 1, 0, 1, 4);

    layout->addWidget(m_status, 2, 0, 1, 3);
    layout->addWidget(m_buttonbox, 2, 3);
    layout->setColumnStretch(0, 1);

    connect(m_spectrawidget, &SpectraWidget::changed, this, [this]() {
        m_notice.clear();
        m_guard->markDirty();
        updateStatus();
    });
    connect(m_spectrawidget, &SpectraWidget::message, this, [this](const QString& text, int) {
        m_notice = text;
        updateStatus();
    });
    m_guard->removeDefaultButtons();
    updateStatus();

    setLayout(layout);
    setWindowTitle(tr("Spectra Analysis and Import"));
    // fits a 1366x768 screen; opens larger where there is room
    setMinimumSize(960, 600);
    resize(1280, 800);
}

void SpectraImport::setDirectory()
{
    openDirectory(QFileDialog::getExistingDirectory(this, tr("Open Directory"), getDir()), m_file_type->currentText());
}

void SpectraImport::openDirectory(const QString& directory, const QString& type)
{
    if (directory.isEmpty()) // the file dialog was cancelled: keep what is loaded
        return;
    QString dir = QString("%1|||%2").arg(directory).arg(type);
    setLastDir(dir);
    m_path->setText(directory);
    m_spectrawidget->clear();
    m_spectrawidget->setDirectory(directory, type);
    qApp->instance()->setProperty("LastSpectraType", type);
}

void SpectraImport::setSpectraFile(const QString& file)
{
    m_path->setText(file);
    m_spectrawidget->setFile(file);
}

void SpectraImport::setFile()
{
    openFile(QFileDialog::getOpenFileName(this, tr("Open file"), getDir()));
}

void SpectraImport::openFile(const QString& file)
{
    if (file.isEmpty()) // the file dialog was cancelled: keep what is loaded
        return;
    setLastDir(file);
    m_path->setText(file);
    m_spectrawidget->clear();
    m_spectrawidget->setFile(file);
}

void SpectraImport::setData(const QJsonObject& data)
{
    m_spectrawidget->setData(data);
    updateStatus();
}

void SpectraImport::accept()
{
    m_spectrawidget->UpdateData();
    if (!m_spectrawidget->hasTable()) { // OK is off then; Enter or a test may still get here
        updateStatus();
        return;
    }
    QString why;
    if (!m_spectrawidget->ready(&why) && !m_guard->confirm(tr("%1 Hand over the table anyway?").arg(why), tr("Hand over")))
        return;
    QDialog::accept();
}

void SpectraImport::reject()
{
    if (m_guard->confirmDiscard())
        QDialog::reject();
}

/*! \brief Status line: the message of a refused input, else what OK hands over or why not; OK is
 * enabled when there is a table to hand over. Claude Generated */
void SpectraImport::updateStatus()
{
    QString why;
    const bool ready = m_spectrawidget->ready(&why);
    const bool table = m_spectrawidget->hasTable();
    if (!m_notice.isEmpty())
        m_status->setMessage(m_notice, StatusLine::Error);
    else
        m_status->setMessage(why, ready ? StatusLine::Info : StatusLine::Warning);
    if (QPushButton* ok = m_buttonbox->button(QDialogButtonBox::Ok))
        ok->setEnabled(table);
}

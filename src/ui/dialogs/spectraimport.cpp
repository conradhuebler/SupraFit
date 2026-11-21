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

#include <QtCore/QFileInfo>
#include <QtWidgets/QSplitter>

#include "src/ui/guitools/dialogmemory.h"
#include "src/ui/guitools/importkeyguard.h"
#include "src/ui/guitools/seriessourcebar.h"
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
    if (path.size() == 2)
        openDirectory(path[0], path[1]);
    else
        openDirectory(directory, "csv");
}

void SpectraImport::setUI()
{
    QGridLayout* layout = new QGridLayout;

    m_guard = new ImportKeyGuard(this);
    m_memory = new DialogMemory(this, "spectraimport");

    // source row: directory, ending, table file, single files, drag and drop (Claude Generated, UX package 2a)
    m_source = new SeriesSourceBar;
    m_source->setObjectName("spectraSource");
    m_source->setEndings({ "csv", "absorb", "xy" }, false);
    m_source->setEnding(qApp->instance()->property("LastSpectraType").toString());
    m_source->setMatch(SeriesSourceBar::Match::Contains);
    m_source->setDirectoryToolTip(tr("Replaces the spectra by every file of a directory whose name contains the chosen ending, one spectrum per file, in natural order (2 before 10)."));
    m_source->setLoadFileAction(tr("Replaces the spectra by a table file: the first row holds the x values, every further row one spectrum."));
    m_source->setAddFilesToolTip(tr("Appends one spectrum per file (x and intensity in columns; csv and absorb by their name). Files or a directory can also be dropped on the dialog."));
    m_source->installDropTarget(this);

    m_spectrawidget = new SpectraWidget;

    m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &SpectraImport::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_status = new StatusLine;
    m_status->setObjectName("spectraStatus");

    layout->addWidget(m_source, 0, 0, 1, 4);
    layout->addWidget(m_spectrawidget, 1, 0, 1, 4);

    layout->addWidget(m_status, 2, 0, 1, 3);
    layout->addWidget(m_buttonbox, 2, 3);
    layout->setColumnStretch(0, 1);
    layout->setRowStretch(1, 1);

    connect(m_source, &SeriesSourceBar::directoryChosen, this, [this](const QString& directory) { openDirectory(directory, m_source->ending()); });
    connect(m_source, &SeriesSourceBar::loadFileChosen, this, &SpectraImport::openFile);
    connect(m_source, &SeriesSourceBar::filesChosen, this, [this](const QStringList& files) { addFiles(files); });
    connect(m_source, &SeriesSourceBar::filesDropped, this, [this](const QStringList& files, const QList<SeriesSourceBar::Failure>& failed) { addFiles(files, failed); });
    connect(m_source, &SeriesSourceBar::endingChanged, this, [](const QString& ending) { qApp->instance()->setProperty("LastSpectraType", ending); });

    connect(m_spectrawidget, &SpectraWidget::changed, this, [this]() {
        m_notice.clear();
        m_guard->markDirty();
        m_source->setSpectraCount(m_spectrawidget->spectraCount());
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

    // remembered place, splitters and VarCovar options (Claude Generated, UX package 2a)
    m_memory->restoreGeometry();
    m_memory->addSplitter(findChild<QSplitter*>("spectraMainSplitter"), "main");
    m_memory->addSplitter(findChild<QSplitter*>("spectraListSplitter"), "list");
    m_spectrawidget->rememberIn(m_memory);
}

void SpectraImport::setDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(this, tr("Open directory"), getDir());
    if (!directory.isEmpty())
        setLastDir(directory);
    openDirectory(directory, m_source->ending());
}

void SpectraImport::openDirectory(const QString& directory, const QString& type)
{
    if (directory.isEmpty()) // the file dialog was cancelled: keep what is loaded
        return;
    QString dir = QString("%1|||%2").arg(directory).arg(type);
    setLastDir(dir);
    m_source->setPath(directory);
    const QStringList files = SeriesSourceBar::filesInDirectory(directory, type, SeriesSourceBar::Match::Contains);
    if (files.isEmpty()) { // nothing to read: the spectra stay
        m_source->reportMessage(tr("No file whose name contains \"%1\" in %2.").arg(type, directory), StatusLine::Error);
        return;
    }
    m_spectrawidget->clear();
    addFiles(files);
    qApp->instance()->setProperty("LastSpectraType", type);
}

void SpectraImport::addFiles(const QStringList& files, const QList<SeriesSourceBar::Failure>& before)
{
    QList<SeriesSourceBar::Failure> failed = before;
    if (!files.isEmpty()) {
        QList<QPair<QString, QString>> reasons;
        const int read = m_spectrawidget->addFiles(files, &reasons);
        for (const auto& reason : reasons)
            failed.append({ reason.first, reason.second });
        m_source->setPath(files.size() == 1 ? files.first() : QFileInfo(files.first()).path());
        m_source->report(read, failed);
    } else
        m_source->report(0, failed);
    m_source->setSpectraCount(m_spectrawidget->spectraCount());
}

void SpectraImport::setSpectraFile(const QString& file)
{
    m_source->setPath(file);
    m_spectrawidget->setFile(file);
    m_source->setSpectraCount(m_spectrawidget->spectraCount());
}

void SpectraImport::setFile()
{
    const QString file = QFileDialog::getOpenFileName(this, tr("Load file"), getDir());
    openFile(file);
}

void SpectraImport::openFile(const QString& file)
{
    if (file.isEmpty()) // the file dialog was cancelled: keep what is loaded
        return;
    setLastDir(file);
    m_source->setPath(file);
    m_spectrawidget->clear();
    m_spectrawidget->setFile(file);
    const int n = m_spectrawidget->spectraCount();
    if (n > 0)
        m_source->report(1, {});
    else
        m_source->report(0, { { file, tr("no spectra found (the first row holds the x values, every further row one spectrum)") } });
    m_source->setSpectraCount(n);
}

void SpectraImport::setData(const QJsonObject& data)
{
    m_spectrawidget->setData(data);
    m_source->setSpectraCount(m_spectrawidget->spectraCount());
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

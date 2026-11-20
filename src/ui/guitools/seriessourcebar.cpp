/*
 * SeriesSourceBar - where the spectra of a series come from: directory, files, drag and drop
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

#include <algorithm>

#include <QtCore/QCollator>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeData>
#include <QtCore/QSet>
#include <QtCore/QUrl>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDragMoveEvent>
#include <QtGui/QDropEvent>

#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include "src/global.h"
#include "src/ui/guitools/statusline.h"

#include "seriessourcebar.h"

SeriesSourceBar::SeriesSourceBar(QWidget* parent)
    : QWidget(parent)
{
    m_directory = new QPushButton(tr("Open directory"));
    m_directory->setObjectName("seriesOpenDirectory");
    m_ending_label = new QLabel(tr("File ending"));
    m_ending = new QComboBox;
    m_ending->setObjectName("seriesFileEnding");
    m_ending->setToolTip(tr("Ending of the spectrum files read by \"Open directory\" and from a dropped directory."));
    m_ending_label->setToolTip(m_ending->toolTip());
    m_load = new QPushButton(tr("Load file"));
    m_load->setObjectName("seriesLoadFile");
    m_load->setVisible(false);
    m_add = new QPushButton(tr("Add files"));
    m_add->setObjectName("seriesAddFiles");
    m_path = new QLineEdit;
    m_path->setObjectName("seriesPath");
    m_path->setReadOnly(true);
    m_path->setPlaceholderText(tr("Drop files or a directory here or on the dialog"));
    m_count = new QLabel;
    m_count->setObjectName("seriesSpectraCount");
    m_status = new StatusLine;
    m_status->setObjectName("seriesSourceStatus");
    m_status->setVisible(false);

    QHBoxLayout* row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->addWidget(m_directory);
    row->addWidget(m_ending_label);
    row->addWidget(m_ending);
    row->addWidget(m_load);
    row->addWidget(m_add);
    row->addWidget(m_path, 1);
    row->addWidget(m_count);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addLayout(row);
    layout->addWidget(m_status);
    setLayout(layout);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum); // one row: the series below gets the height
    setSpectraCount(0);

    connect(m_directory, &QPushButton::clicked, this, [this]() {
        const QString directory = QFileDialog::getExistingDirectory(this, tr("Open directory"), getDir());
        if (directory.isEmpty()) // cancelled: keep what is loaded
            return;
        setLastDir(directory);
        emit directoryChosen(directory);
    });
    connect(m_load, &QPushButton::clicked, this, [this]() {
        const QString file = QFileDialog::getOpenFileName(this, tr("Load file"), getDir());
        if (file.isEmpty())
            return;
        setLastDir(file);
        emit loadFileChosen(file);
    });
    connect(m_add, &QPushButton::clicked, this, [this]() {
        QStringList files = QFileDialog::getOpenFileNames(this, tr("Add files"), getDir(), m_filter);
        if (files.isEmpty())
            return;
        sortNaturally(&files);
        setLastDir(files.first());
        emit filesChosen(files);
    });
    connect(m_ending, &QComboBox::currentTextChanged, this, &SeriesSourceBar::endingChanged);
}

void SeriesSourceBar::setEndings(const QStringList& endings, bool editable)
{
    const QSignalBlocker block(m_ending);
    m_ending->clear();
    m_ending->setEditable(editable);
    m_ending->addItems(endings);
}

QString SeriesSourceBar::ending() const
{
    return m_ending->currentText().trimmed();
}

void SeriesSourceBar::setEnding(const QString& ending)
{
    if (ending.isEmpty())
        return;
    const int index = m_ending->findText(ending);
    if (index >= 0)
        m_ending->setCurrentIndex(index);
    else if (m_ending->isEditable())
        m_ending->setCurrentText(ending);
}

void SeriesSourceBar::setLoadFileAction(const QString& tooltip)
{
    m_load->setToolTip(tooltip);
    m_load->setVisible(!tooltip.isEmpty());
}

void SeriesSourceBar::setDirectoryToolTip(const QString& tooltip)
{
    m_directory->setToolTip(tooltip);
}

void SeriesSourceBar::setAddFilesToolTip(const QString& tooltip)
{
    m_add->setToolTip(tooltip);
}

void SeriesSourceBar::setPath(const QString& path)
{
    m_path->setText(path);
}

QString SeriesSourceBar::path() const
{
    return m_path->text();
}

void SeriesSourceBar::setSpectraCount(int count)
{
    m_count->setText(tr("Spectra: %1").arg(count));
}

QString SeriesSourceBar::reportText(int read, const QList<Failure>& failed)
{
    QString text = failed.isEmpty() ? tr("%1 file(s) read").arg(read)
                                    : tr("%1 file(s) read, %2 not read").arg(read).arg(failed.size());
    QStringList details;
    for (const Failure& failure : failed) {
        const QString name = QFileInfo(failure.file).fileName().isEmpty() ? failure.file : QFileInfo(failure.file).fileName();
        // a reader that names the file itself is not prefixed a second time
        details << (failure.reason.contains(name) ? failure.reason : tr("%1: %2").arg(name, failure.reason));
    }
    if (!details.isEmpty())
        text += QStringLiteral(": ") + details.first() + "\n" + details.join("\n");
    return text;
}

void SeriesSourceBar::report(int read, const QList<Failure>& failed)
{
    const StatusLine::Level level = failed.isEmpty() ? StatusLine::Info : (read > 0 ? StatusLine::Warning : StatusLine::Error);
    m_status->setMessage(reportText(read, failed), level);
    m_status->setVisible(true);
}

void SeriesSourceBar::reportMessage(const QString& text, int level)
{
    m_status->setMessage(text, StatusLine::Level(level));
    m_status->setVisible(!text.isEmpty());
}

void SeriesSourceBar::sortNaturally(QStringList* files)
{
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(files->begin(), files->end(), [&collator](const QString& a, const QString& b) { return collator.compare(a, b) < 0; });
}

QStringList SeriesSourceBar::filesInDirectory(const QString& directory, const QString& ending, Match match)
{
    QStringList files;
    const QDir dir(directory);
    for (const QFileInfo& info : dir.entryInfoList(QDir::Files)) {
        const QString name = info.fileName();
        const bool fits = ending.isEmpty() || (match == Match::EndsWith ? name.endsWith(ending, Qt::CaseInsensitive) : name.contains(ending, Qt::CaseInsensitive));
        if (fits)
            files << info.absoluteFilePath();
    }
    sortNaturally(&files);
    return files;
}

QStringList SeriesSourceBar::filesOfDrop(const QMimeData* mime, const QString& ending, Match match, QList<Failure>* failed)
{
    QStringList files;
    if (!mime)
        return files;
    for (const QUrl& url : mime->urls()) {
        if (!url.isLocalFile()) {
            if (failed)
                failed->append({ url.toString(), tr("not a local file") });
            continue;
        }
        const QString path = url.toLocalFile();
        const QFileInfo info(path);
        if (info.isDir()) {
            const QStringList inside = filesInDirectory(path, ending, match);
            if (inside.isEmpty() && failed)
                failed->append({ path, ending.isEmpty() ? tr("no file in the directory") : tr("no file with the ending %1 in the directory").arg(ending) });
            files << inside;
        } else
            files << info.absoluteFilePath(); // an unreadable file is reported by the host's reader
    }
    sortNaturally(&files);
    files.removeDuplicates();
    return files;
}

void SeriesSourceBar::installDropTarget(QWidget* target)
{
    if (!target)
        return;
    m_target = target;
    target->setAcceptDrops(true); // without a widget that accepts drops, no drag event reaches the dialog
    qApp->installEventFilter(this);
}

bool SeriesSourceBar::eventFilter(QObject* watched, QEvent* event)
{
    const QEvent::Type type = event->type();
    if (type != QEvent::DragEnter && type != QEvent::DragMove && type != QEvent::Drop)
        return QObject::eventFilter(watched, event);
    QWidget* widget = qobject_cast<QWidget*>(watched);
    if (!m_target || !widget || !(widget == m_target || m_target->isAncestorOf(widget)))
        return QObject::eventFilter(watched, event);
    QDropEvent* drop = static_cast<QDropEvent*>(event); // QDragEnterEvent and QDragMoveEvent are QDropEvents
    if (!drop->mimeData() || !drop->mimeData()->hasUrls())
        return QObject::eventFilter(watched, event); // text into a field: the child's own business
    drop->setDropAction(Qt::CopyAction);
    drop->accept();
    if (type == QEvent::Drop) {
        QList<Failure> failed;
        const QStringList files = filesOfDrop(drop->mimeData(), ending(), m_match, &failed);
        if (!files.isEmpty())
            setPath(files.size() == 1 ? files.first() : QFileInfo(files.first()).path());
        emit filesDropped(files, failed);
    }
    return true;
}

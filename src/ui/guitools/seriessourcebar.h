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

#pragma once

#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <QtWidgets/QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QMimeData;
class QPushButton;
class StatusLine;

/*! \brief The source row of the spectra and NMR import dialogs (Claude Generated).
 *
 * "Open directory" (replaces the series by the files of a directory with the chosen ending), the combo
 * "File ending", optionally "Load file" (a host whose core reads one table file in place of the series),
 * "Add files" (appends), the path of the last source and the number of loaded spectra; below it a
 * StatusLine with "n files read, m not read" (the reasons in its tooltip).
 *
 * Drag and drop: installDropTarget() accepts local files and directories dropped anywhere on the dialog,
 * also on child widgets that take text drops themselves (an application event filter sees the drop
 * first). A directory gives its files with the chosen ending, files are taken as they are; together in
 * natural order (2 before 10), without duplicates; a drop appends like "Add files".
 *
 * The bar opens the file dialogs and emits what was chosen; a cancelled file dialog emits nothing. The
 * host reads the files and calls report(). */
class SeriesSourceBar : public QWidget {
    Q_OBJECT
public:
    /// how a file name is compared with the ending
    enum class Match {
        EndsWith, ///< the name ends with it (NMR, case-insensitive)
        Contains ///< the name contains it (UV/Vis "csv", "absorb", "xy")
    };

    /// a file that was not read, and why
    struct Failure {
        QString file;
        QString reason;
    };

    explicit SeriesSourceBar(QWidget* parent = nullptr);

    /*! \brief Items of the ending combo; editable = the user may type another ending. */
    void setEndings(const QStringList& endings, bool editable);
    QString ending() const;
    void setEnding(const QString& ending);
    inline QComboBox* endingBox() const { return m_ending; }
    inline void setMatch(Match match) { m_match = match; }
    /*! \brief Filter of the "Add files" dialog (QFileDialog syntax). */
    inline void setFileFilter(const QString& filter) { m_filter = filter; }
    /*! \brief Shows "Load file" with this tooltip (empty: hidden). */
    void setLoadFileAction(const QString& tooltip);
    void setDirectoryToolTip(const QString& tooltip);
    void setAddFilesToolTip(const QString& tooltip);

    void setPath(const QString& path);
    QString path() const;
    /*! \brief Shows the number of loaded spectra. */
    void setSpectraCount(int count);

    /*! \brief "n files read, m not read" (Info without failures, else Warning; nothing read = Error);
     * every failure with its reason in the tooltip. */
    void report(int read, const QList<Failure>& failed);
    /*! \brief Another message about the source (e.g. a directory without matching files). */
    void reportMessage(const QString& text, int level);
    inline StatusLine* statusLine() const { return m_status; }
    /*! \brief Text of report() (also for tests). */
    static QString reportText(int read, const QList<Failure>& failed);

    /*! \brief Accept dropped files and directories on target and all its children. */
    void installDropTarget(QWidget* target);
    /*! \brief Files of directory whose name matches ending, in natural order. */
    static QStringList filesInDirectory(const QString& directory, const QString& ending, Match match);
    /*! \brief Files of a drop: directories expanded (ending, match), natural order, no duplicates;
     * *failed gets non-local URLs and directories without a matching file. */
    static QStringList filesOfDrop(const QMimeData* mime, const QString& ending, Match match, QList<Failure>* failed);
    /*! \brief Natural order (2 before 10) of file names. */
    static void sortNaturally(QStringList* files);

signals:
    /// "Open directory": the series is to be replaced by the files of directory
    void directoryChosen(const QString& directory);
    /// "Load file" (only shown with setLoadFileAction)
    void loadFileChosen(const QString& file);
    /// "Add files": files to append, natural order
    void filesChosen(const QStringList& files);
    /// a drop on the dialog: files to append and the entries that gave no file
    void filesDropped(const QStringList& files, const QList<SeriesSourceBar::Failure>& failed);
    void endingChanged(const QString& ending);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QPushButton *m_directory, *m_load, *m_add;
    QComboBox* m_ending;
    QLabel *m_ending_label, *m_count;
    QLineEdit* m_path;
    StatusLine* m_status;
    QPointer<QWidget> m_target;
    Match m_match = Match::EndsWith;
    QString m_filter;
};

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

#pragma once

#include <QtWidgets/QDialog>

#include <QtCore/QJsonObject>

#include <src/ui/widgets/spectrawidget.h>

class QPushButton;
class QLineEdit;
class QDialogButtonBox;
class QLabel;

class ImportKeyGuard;
class SpectraWidget;
class StatusLine;

/*! \brief Dialog of the UV/Vis spectra import (SpectraWidget) and of Edit Data of a spectra project.
 *
 * Keyboard rule and discard question of ImportKeyGuard (Enter never presses OK; Cancel, ESC or closing
 * the window ask when something changed). The status line under the spectra says what OK will hand
 * over or why not; OK is off only while there is nothing to hand over (no spectrum or no x value).
 */
class SpectraImport : public QDialog {
    Q_OBJECT
public:
    SpectraImport();
    SpectraImport(const QString& directory);

    virtual ~SpectraImport(){};

    inline SpectraWidget* spectraWidget() const { return m_spectrawidget; }
    inline ImportKeyGuard* keyGuard() const { return m_guard; }
    inline StatusLine* statusLine() const { return m_status; }
    /*! \brief Reads every file of directory whose name contains type (as "Open Directory"); an empty
     * directory changes nothing. Claude Generated */
    void openDirectory(const QString& directory, const QString& type);
    /*! \brief Reads a file (as "Load file"); an empty name changes nothing. Claude Generated */
    void openFile(const QString& file);

    void setSpectraFile(const QString& file);

    void setUI();
    QJsonObject ProjectData() const { return m_spectrawidget->ProjectData(); }
    QJsonObject InputTable() const { return m_spectrawidget->InputTable(); }
    void setData(const QJsonObject& data);

public slots:
    void setFile();
    void setDirectory();
    void accept() override;
    void reject() override;

private:
    SpectraWidget* m_spectrawidget;
    QPushButton *m_directory, *m_file;
    QLineEdit* m_path;
    QDialogButtonBox* m_buttonbox;
    QComboBox* m_file_type;
    StatusLine* m_status;
    ImportKeyGuard* m_guard;
    QString m_notice; ///< message of the last refused input, shown until the next change

    void updateStatus();
};

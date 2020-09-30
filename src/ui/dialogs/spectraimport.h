/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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

class SpectraWidget;

class SpectraImport : public QDialog {
public:
    SpectraImport();
    SpectraImport(const QString& directory);

    virtual ~SpectraImport(){};

    void setUI();
    QJsonObject ProjectData() const { return m_spectrawidget->ProjectData(); }
    QJsonObject InputTable() const { return m_spectrawidget->InputTable(); }
    void setData(const QJsonObject& data);

public slots:
    void setDirectory();

private:
    SpectraWidget* m_spectrawidget;
    QPushButton* m_directory;
    QLineEdit* m_path;
    QDialogButtonBox* m_buttonbox;
    QComboBox* m_file_type;
};

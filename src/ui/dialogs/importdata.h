/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2021 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/dataclass.h"

#include <QtCore/QPointer>
#include <QtCore/QVector>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTableView>

#include <src/ui/widgets/DropTable.h>

class QDialogButtonBox;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QCheckBox;

class DataTable;

class ImportData : public QDialog {
    Q_OBJECT

public:
    ImportData(const QString& file, QWidget* parent = 0);
    ImportData(QWeakPointer<DataClass> data);
    ImportData(QWidget* parent = 0);
    ~ImportData();

    inline DataClass getStoredData() { return *m_storeddata; }
    inline QString ProjectFile() const { return m_projectfile; }
    inline QJsonObject getProject() const
    {
        return m_project;
    }
    void setRootDir(const QString& str) { m_root_dir = str; }
    void setData(const DataTable* model);

    void LoadTable(DataTable* model, int independent = 2);
    void setSpectraData(const QJsonObject& json);

private:
    void setUi();

    void WriteData(const DataTable* model, int independent = 2);

    QPointer<DropTable> m_table, m_sec_table;
    QPointer<QLineEdit> m_line;
    QPointer<QPushButton> m_select, m_export, m_file, m_thermogram, m_spectra;
    QPointer<QSpinBox> m_independent_rows, m_dependent_rows;
    QPointer<QDialogButtonBox> m_buttonbox;
    QPointer<DataClass> m_storeddata;
    QPointer<QCheckBox> m_simulation;
    QString m_filename, m_projectfile;
    QJsonObject m_raw;
    DataClassPrivate::DataType m_type = DataClassPrivate::Table;
    QString m_title, m_root_dir;
    bool m_single = true;
    QJsonObject m_systemparameter, m_project;

private slots:
    void LoadFile();
    void SelectFile();
    void ExportFile();
    void accept() override;
    void NoChanged();
    bool ImportThermogram(const QString& filename);
    bool ImportThermogram();

    bool ImportSpectra(const QString& filename);
};

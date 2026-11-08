/*
 * SpectraWidget - spectra import: spectra list, x values, chart with processing, concentrations
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

#include <charts.h>

#include <QtCore/QJsonObject>

#include <src/core/spectrahandler.h>

class DropTable;
class TraceEditorWidget;

class QLineEdit;
class QListWidget;
class QTableWidget;
class QSplitter;
class QPushButton;
class QTabWidget;
class QSpinBox;
class QCheckBox;

class SpectraWidget : public QWidget {
    Q_OBJECT
public:
    explicit SpectraWidget(QWidget* parent = nullptr);

    void setDirectory(const QString& directry, const QString& type);
    void setFile(const QString& file);
    void addFile(const QString& file);

    void clear();

    void setUI();
    QJsonObject ProjectData() const { return m_project; }
    QJsonObject InputTable() const { return m_input_table; }
    void setData(const QJsonObject& data);

public slots:
    void UpdateSpectra();
    void UpdateData();

signals:

private:
    QTabWidget* m_views;
    QListWidget *m_files, *m_xvalues;
    TraceEditorWidget* m_editor; ///< chart, range and processing steps of the spectra
    DropTable *m_indep, *m_datatable;
    QSplitter *m_main_splitter, *m_list_splitter;
    QLineEdit* m_add_xvalue;
    QPushButton *m_accept_x, *m_varcovar, *m_export_table;
    QSpinBox* m_values;
    SpectraHandler* m_handler;
    QJsonObject m_project, m_input_table;
    QCheckBox *m_cluster, *m_averaged;

    void UpdateXValues();
    void UpdateVerticaLines();

private slots:
    void PointDoubleClicked(const QPointF& point);
    void SaveToFile();
};

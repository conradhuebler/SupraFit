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

    /*! \brief Number of loaded spectra. Claude Generated */
    int spectraCount() const;
    /*! \brief Number of selected x values. Claude Generated */
    int xValueCount() const;
    /*! \brief Rows of the pasted concentration table (0 without one). Claude Generated */
    int concentrationRows() const;
    /*! \brief Columns of the pasted concentration table (0 without one). Claude Generated */
    int concentrationColumns() const;

    /*! \brief Can OK hand over a complete table? True if there is at least one spectrum and one x value and
     * the concentrations (if pasted) have one row per spectrum. *why: the reason, or what OK hands over. Claude Generated */
    bool ready(QString* why = nullptr) const;
    /*! \brief Is there anything OK could hand over (at least one spectrum and one x value)? Claude Generated */
    bool hasTable() const;

    /*! \brief Adds an x value typed by the user ("450.5" or "450,5"); false (and message()) for anything else. Claude Generated */
    bool addXValueText(const QString& text);

public slots:
    void UpdateSpectra();
    void UpdateData();

signals:
    /*! \brief Spectra, x values, processing, range, concentrations or VarCovar options changed. Claude Generated */
    void changed();
    /*! \brief A message for the status line of the dialog (level of StatusLine). Claude Generated */
    void message(const QString& text, int level);

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

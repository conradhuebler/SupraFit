/*
 * FileHandler - File loading and parsing for SupraFit data formats
 * Copyright (C) 2016 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 * 
 * This class handles loading of various file formats (JSON, CSV, ITC, dH)
 * and provides data range extraction functionality for selective data loading.
 * Extended with range selection capabilities by Claude Code AI Assistant.
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

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QPointer>

class DataTable;

class FileHandler : public QObject {
    Q_OBJECT

public:
    enum FileType {
        SupraFit = 1,
        Generic = 2,
        dH = 3,
        ITC = 4,
        CSV = 5
    };

    FileHandler(const QString& filename, QObject* parent = 0);
    FileHandler(QObject* parent = 0);
    ~FileHandler();

    void LoadFile();

    inline bool AllInt() const { return m_allint; }
    inline bool Table() const { return m_table; }
    QPointer<DataTable> getData() const { return m_stored_table; }

    inline bool FileSupported() const { return m_file_supported; }

    inline bool setFileContent(const QString& str)
    {
        m_filecontent = str.split("\n");
        ReadGeneric();
        bool read = CheckForTable();
        return read;
    }

    inline void setFileType(FileType type) { m_filetype = type; }
    inline FileType Type() const { return m_filetype; }
    inline QJsonObject SystemParameter() const { return m_systemparameter; }
    inline void setIndependentRows(int rows) { m_rows = rows; }
    inline void setStartPoint(int point) { m_start_point = point; }
    inline void setSeriesCount(int series) { m_series = series; }
    
    // Enhanced file loading with range selection - Claude Generated
    inline void setStartRow(int startRow) { m_start_row = startRow; }
    inline void setEndRow(int endRow) { m_end_row = endRow; }
    inline void setStartCol(int startCol) { m_start_col = startCol; }
    inline void setEndCol(int endCol) { m_end_col = endCol; }
    inline void setRowRange(int startRow, int endRow) { m_start_row = startRow; m_end_row = endRow; }
    inline void setColRange(int startCol, int endCol) { m_start_col = startCol; m_end_col = endCol; }
    
    // Get a sub-range of the loaded data table
    QPointer<DataTable> getDataRange(int startRow = -1, int endRow = -1, int startCol = -1, int endCol = -1) const;
    inline QJsonObject getJsonData() const { return m_topjson; }
    inline void setThermogram(bool thermogram) { m_thermogram = thermogram; }
    void setThermogramParameter(const QJsonObject& thermogram_parameter)
    {
        m_thermogram_parameter = thermogram_parameter;
        m_thermogram = true;
    }

private:
    void ReadGeneric();
    void ReaddH();
    void ReadJson();
    void ReadITC();
    void ConvertTable();
    void ReadSeperated(const QString& seperator);
    bool CheckForTable();

    void GenerateTable(QVector<QVector<double>>& table, const QStringList& header, int max_columns);

    bool m_table, m_allint, m_file_supported, m_thermogram = false, m_plain_thermogram = false;
    QPointer<DataTable> m_stored_table;

    QString m_filename, m_title;
    QString sep;
    QStringList m_filecontent;
    int m_lines, m_rows = 2, m_start_point = 0, m_series = 0;
    FileType m_filetype;
    
    // Range selection parameters - Claude Generated
    int m_start_row = 0, m_end_row = -1, m_start_col = 0, m_end_col = -1;

    QJsonObject m_systemparameter, m_thermogram_parameter;
    QJsonObject m_topjson;
};

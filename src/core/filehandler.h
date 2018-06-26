/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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
        Generic = 1,
        dH = 2
    };

    FileHandler(const QString& filename, QObject* parent = 0);
    FileHandler(QObject* parent = 0);
    ~FileHandler();

    inline bool AllInt() const { return m_allint; }
    inline bool Table() const { return m_table; }
    QPointer<DataTable> getData() const { return m_stored_table; }

    inline bool FileSupported() const { return m_file_supported; }

    inline void setFileContent(const QString& str)
    {
        m_filecontent = str.split("\n");
        ReadGeneric();
        CheckForTable();
    }

    inline void setFileType(FileType type) { m_filetype = type; }
    inline FileType Type() const { return m_filetype; }
    inline QJsonObject SystemParameter() const { return m_systemparameter; }

private:
    void LoadFile();

    void ReadGeneric();
    void ReaddH();

    bool CheckForTable();

    bool m_table, m_allint, m_file_supported;
    QPointer<DataTable> m_stored_table;

    QString m_filename;
    QString sep;
    QStringList m_filecontent;
    int m_lines;
    FileType m_filetype;

    QJsonObject m_systemparameter;
};

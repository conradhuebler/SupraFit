/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#ifndef FILEHANDLER_H
#define FILEHANDLER_H

class QStandardItemModel;
class FileHandler : public QObject
{
    Q_OBJECT
public:
    FileHandler(const QString &filename, QObject *parent = 0);
    ~FileHandler();
    bool AllInt() const { return m_allint;}
    bool Table() const { return m_table; }
    QPointer<QStandardItemModel >getData() const;
    bool FileSupported()const {return m_file_supported;}
    
private:
    void ReadFile();
    void CheckForTable();
    
    bool m_table, m_allint, m_file_supported;
    QString m_filename;
    QStringList m_filecontent;
    int m_lines;
};

#endif // FILEHANDLER_H

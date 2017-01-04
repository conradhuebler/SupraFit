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
#include <QStandardItemModel>
#include <QPointer>
#include <QtCore/QFile>
#include <QDebug>
#include "filehandler.h"


FileHandler::FileHandler(const QString &filename, QObject *parent) :m_filename(filename), QObject(parent), m_lines(0), m_table(true), m_allint(true), m_file_supported(true)
{
    
    ReadFile();
    CheckForTable();
 
}

FileHandler::~FileHandler()
{
}

void FileHandler::ReadFile()
{
    QFile file(m_filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug() << file.errorString();
//         return; //FIXME Hää
    }

    m_filecontent = QString(file.readAll()).split("\n");
    m_lines = m_filecontent.size();
}

void FileHandler::CheckForTable()
{
    int size = 0;
    
    for(int i = 0; i < m_lines; ++i)
    {
        
        if(size)
            m_table = (size == m_filecontent[i].simplified().split(" ").size());
        size = m_filecontent[i].simplified().split(" ").size();
        
        if(!m_table)
            return;
        
        if(m_table)
        {
            QStringList elements = m_filecontent[i].simplified().split(" ");
            for(int j = 0; j < elements.size(); ++j)
            {
                if(elements[j].contains(QRegExp("[Aa-Zz]")))
                    m_allint = false;
            }
        }
    }
    m_file_supported = m_allint && m_table; 
}

QPointer<QStandardItemModel> FileHandler::getData() const
{
    QPointer<QStandardItemModel > model = new QStandardItemModel;
    foreach(const QString &line, m_filecontent)
    {
        QList<QStandardItem *> row;
        foreach(const QString &item, line.simplified().split(" "))
            row.append(new QStandardItem(QString(item).replace(",", ".")));
        model->appendRow(row);
    }
    return model;
}



#include "filehandler.moc"

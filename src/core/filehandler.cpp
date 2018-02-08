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

#include "src/core/dataclass.h"

#include <QtCore/QPointer>
#include <QtCore/QFile>
#include <QDebug>
#include "filehandler.h"


FileHandler::FileHandler(const QString &filename, QObject *parent) :m_filename(filename), QObject(parent), m_lines(0), m_table(true), m_allint(true), m_file_supported(true)
{
    LoadFile();
    Read();
    CheckForTable();
}

FileHandler::FileHandler(QObject* parent) : QObject(parent), m_lines(0), m_table(true), m_allint(true), m_file_supported(true)
{
}


FileHandler::~FileHandler()
{
}

void FileHandler::LoadFile()
{    
    QFile file(m_filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug() << file.errorString();
//         return; //FIXME Hää
    }
    
    m_filecontent = QString(file.readAll()).split("\n");
}


void FileHandler::Read()
{
    int tab = 0, semi = 0;
    for(const QString &str : qAsConst(m_filecontent))
    {
        tab += str.count("\t");
        semi+= str.count(";");
    }
    if(tab > semi)
        sep = " ";
    else
        sep = ";";
    m_lines = m_filecontent.size();
}

void FileHandler::CheckForTable()
{
    int size = 0;
    
    for(int i = 0; i < m_lines; ++i)
    {
        if(size)
            m_table = (size == m_filecontent[i].split(sep).size());
        size = m_filecontent[i].split(sep).size();
        
        if(!m_table)
            return;
        
        if(m_table)
        {
            QStringList elements = m_filecontent[i].split("\n");
            for(int j = 0; j < elements.size(); ++j)
            {
                if(elements[j].contains(QRegExp("[Aa-Zz]")))
                    m_allint = false;
            }
        }
    }
    m_file_supported = m_allint && m_table; 
}

QPointer<DataTable> FileHandler::getData() const
{
    QPointer<DataTable > model = new DataTable;
    int i = 0;
    for(const QString &line: qAsConst(m_filecontent))
    {
        if(!line.isEmpty() && !line.isNull())
        {
            QVector<qreal> row;
            QStringList header;
            QStringList items = line.simplified().split(sep);
            double sum = 0;
            for(const QString &item: qAsConst(items))
            {
                row.append((QString(item).replace(",", ".")).toDouble());
                sum += (QString(item).replace(",", ".")).toDouble();
                header << item;
            }
            if(!i && !sum)
            {
                for(int j = 0; j < header.size(); ++j)
                    model->setHeaderData(j, Qt::Horizontal, (header[j]), Qt::DisplayRole);
            }
            else
                model->insertRow(row);
            
        }
    }
    return model;
}



#include "filehandler.moc"

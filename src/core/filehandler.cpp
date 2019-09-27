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

#include "src/core/dataclass.h"
#include "src/core/jsonhandler.h"
#include "src/core/models.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QStringList>

#include "filehandler.h"

FileHandler::FileHandler(const QString& filename, QObject* parent)
    : QObject(parent)
    , m_table(true)
    , m_allint(true)
    , m_file_supported(true)
    , m_filename(filename)
    , m_lines(0)
    , m_filetype(FileType::Generic)
{
}

FileHandler::FileHandler(QObject* parent)
    : QObject(parent)
    , m_table(true)
    , m_allint(true)
    , m_file_supported(true)
    , m_lines(0)
    , m_filetype(FileType::Generic)
{
}

FileHandler::~FileHandler()
{
}

void FileHandler::LoadFile()
{
    QFile file(m_filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << file.errorString();
        return;
    }

    QFileInfo info(m_filename);
    m_title = info.baseName();

    if (info.suffix().toLower() == "suprafit" || info.suffix().toLower() == "json")
        m_filetype = FileType::SupraFit;
    else if ((info.suffix()).toLower() == "dh")
        m_filetype = FileType::dH;
    else
        m_filetype = FileType::Generic;

    m_filecontent = QString(file.readAll()).split("\n");
    if (Type() == FileType::SupraFit)
        ReadJson();
    else if (m_filetype == FileType::Generic)
        ReadGeneric();
    else if (m_filetype == FileType::dH)
        ReaddH();
    else
        m_file_supported = false;
}

void FileHandler::ReadGeneric()
{
    int tab = 0, semi = 0;
    for (const QString& str : qAsConst(m_filecontent)) {
        tab += str.count("\t");
        tab += str.count(" ");
        semi += str.count(";");
    }
    if (tab > semi)
        sep = " ";
    else
        sep = ";";
    m_lines = m_filecontent.size();

    if (!CheckForTable())
        return;

    m_stored_table = new DataTable;
    int i = 0;
    //qDebug() << m_filecontent.size();
    if (m_filecontent.size() > 1e4 && qApp->instance()->property("auto_thermo_dialog").toBool())
        return;
    for (const QString& line : qAsConst(m_filecontent)) {
        if (!line.isEmpty() && !line.isNull()) {
            QVector<qreal> row;
            QStringList header;
            QStringList items = line.simplified().split(sep);
            double sum = 0;
            for (const QString& item : qAsConst(items)) {
                row.append((QString(item).replace(",", ".")).toDouble());
                sum += (QString(item).replace(",", ".")).toDouble();
                header << item;
            }
            if (!i && !sum) {
                for (int j = 0; j < header.size(); ++j)
                    m_stored_table->setHeaderData(j, Qt::Horizontal, (header[j]), Qt::DisplayRole);
            } else
                m_stored_table->insertRow(row);
        }
    }
    ConvertTable();
}

void FileHandler::ConvertTable()
{
    bool simulation = false;
    if (m_rows > m_stored_table->columnCount()) {
        m_rows = m_stored_table->columnCount();
        simulation = true;
    }
    DataClass* data = new DataClass;
    DataTable* indep = m_stored_table->BlockColumns(0, m_rows);
    DataTable* dep = m_stored_table->BlockColumns(m_rows, m_stored_table->columnCount() - m_rows);
    for (int i = 0; i < m_start_point && i < dep->rowCount(); ++i)
        dep->DisableRow(i);
    if (simulation)
        dep = indep;
    data->setDependentTable(dep);
    data->setIndependentTable(indep);
    data->setDataType(DataClassPrivate::Table);
    data->setProjectTitle(m_title);
    data->setSystemObject(m_systemparameter);
    m_topjson = data->ExportData();
    if (simulation)
        m_topjson["DataType"] = 10;
    delete data;
}

bool FileHandler::CheckForTable()
{
    // int size = 0;
#pragma message("rethink about it")
    for (int i = 0; i < m_lines; ++i) {
        // qDebug() << m_filecontent[i];
        if (m_filecontent[i].isEmpty())
            continue;
        /*
      if (size)
        {
            qDebug() << m_filecontent[i] << m_filecontent[i].split(sep) << m_filecontent[i].split(sep).size();
            m_table = (size == m_filecontent[i].split(sep).size());
        }
        size = m_filecontent[i].split(sep).size();

        if (!m_table)
            return false;
         */
        if (m_table) {
            QStringList elements = m_filecontent[i].split("\n");
            for (int j = 0; j < elements.size(); ++j) {
                if (elements[j].contains(QRegExp("[Aa-Zz]")))
                    m_allint = false;
            }
        }
    }
    m_file_supported = m_allint && m_table;
    return m_allint && m_table;
}

void FileHandler::ReaddH()
{

    if (m_filecontent.size() < 5)
        return;

    m_rows = 1;

    int number = m_filecontent[1].split(",")[1].toInt();

    QStringList parameter = m_filecontent[2].split(",");
    if (parameter.size() < 4)
        return;

    m_systemparameter[QString::number(AbstractItcModel::Temperature)] = QString::number(parameter[0].toDouble() + 273.15);
    m_systemparameter[QString::number(AbstractItcModel::CellConcentration)] = QString::number(parameter[1].toDouble());
    m_systemparameter[QString::number(AbstractItcModel::SyringeConcentration)] = QString::number(parameter[2].toDouble());
    m_systemparameter[QString::number(AbstractItcModel::CellVolume)] = QString::number(parameter[3].toDouble() * 1000);
    m_stored_table = new DataTable;

    for (int i = 5; i < (number + 5); ++i) {
        QVector<qreal> row;
        QStringList items = m_filecontent[i].split(",");
        for (int j = 0; j < items.size(); ++j)
            row << items[j].toDouble();
        m_stored_table->insertRow(row);
    }
    ConvertTable();
}

void FileHandler::ReadJson()
{
    JsonHandler::ReadJsonFile(m_topjson, m_filename);
}

#include "filehandler.moc"

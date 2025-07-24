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

#include "src/core/jsonhandler.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/models.h"
#include "src/core/thermogramhandler.h"
#include "src/core/toolset.h"

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
    else if ((info.suffix()).toLower() == "itc" || m_thermogram) {

        if ((info.suffix()).toLower() != "itc")
            m_plain_thermogram = true;

        m_filetype = FileType::ITC;
        ReadITC();
        return;
    } else if ((info.suffix()).toLower() == "csv") {
        m_filetype = FileType::CSV;
    } else
        m_filetype = FileType::Generic;

    m_filecontent = QString(file.readAll()).split("\n");
    if (Type() == FileType::SupraFit)
        ReadJson();
    else if (m_filetype == FileType::Generic)
        ReadGeneric();
    else if (m_filetype == FileType::dH)
        ReaddH();
    else if (m_filetype == FileType::CSV) {
        ReadSeperated(",");
    } else
        m_file_supported = false;
}

void FileHandler::ReadSeperated(const QString& seperator)
{
    QStringList header;
    bool header_added = false;
    int index = 0;
    int max_columns = 0;
    QVector<QVector<double>> rows(m_filecontent.size());
    for (const QString& line : qAsConst(m_filecontent)) {
        if (!line.isEmpty()) {
            bool insert_row = true;
            bool read_header = false;
            if (header_added == false) {
                read_header = true;
                insert_row = false;
            }
            QVector<qreal> row;
            QStringList items = line.simplified().replace("\"", "").split(seperator);
            for (const QString& item : qAsConst(items)) {
                bool convert;
                QString(item).replace(",", ".").toDouble(&convert);
                if (convert) {
                    read_header = false;
                    insert_row = true;
                }
                if (read_header && !convert) {
                    QString head = QString(item).replace(HashTag, "");
                    if (!head.isEmpty() && !head.isNull())
                        header << head;
                } else if (item[0] != HashTag)
                    row.append((QString(item).replace(",", ".")).toDouble());
                else
                    insert_row = false;
            }
            if (!header_added) {
                header_added = true;
            }
            if (!read_header && insert_row) {
                rows[index] = row;
                max_columns = qMax(max_columns, row.size());
            }
        }
        index++;
    }
    GenerateTable(rows, header, max_columns);
}

void FileHandler::ReadGeneric()
{
    int tab = 0, semi = 0, comma = 0;
    for (const QString& str : qAsConst(m_filecontent)) {
        tab += str.count("\t");
        tab += str.count(" ");
        semi += str.count(";");
        comma += str.count(",");
    }
    if (tab > semi)
        sep = " ";
    else
        sep = ";";
    m_lines = m_filecontent.size();

    if (!CheckForTable())
        return;

    ReadSeperated(sep);
}

void FileHandler::GenerateTable(QVector<QVector<double>>& table, const QStringList& header, int max_columns)
{
    while (table[table.size() - 1].isEmpty())
        table.removeLast();

    while (table[0].isEmpty())
        table.removeFirst();

    m_stored_table = new DataTable(table.size(), max_columns, 0);
    if (header.size() == max_columns)
        m_stored_table->setHeader(header);
    for (int i = 0; i < table.size(); ++i) {
        for (int j = 0; j < table[i].size(); ++j) {
            m_stored_table->operator()(i, j) = table[i][j];
        }
    }

    ConvertTable();
}

void FileHandler::ConvertTable()
{
    bool simulation = false;
    if (m_rows >= m_stored_table->columnCount()) {
        m_rows = m_stored_table->columnCount();
        simulation = true;
    }
    DataClass* data = new DataClass;
    DataTable* indep = m_stored_table->BlockColumns(0, m_rows);
    DataTable* dep = m_stored_table->BlockColumns(m_rows, m_stored_table->columnCount() - m_rows);
    for (int i = 0; i < m_start_point && i < dep->rowCount(); ++i)
        dep->DisableRow(i);
    if (simulation) {
        dep->clear(m_series, m_stored_table->rowCount()); // rename to stuff to correct rows and cols sometimes
    }
#pragma message("have a look at here, while restructureing stuff")
    data->setDependentRawTable(dep);
    data->setIndependentRawTable(indep);
    // ende

    if (m_filetype == FileType::ITC)
        data->setDataType(DataClassPrivate::Thermogram);
    else
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
    QRegularExpression rx("\\b^[a-zA-Z]+$\\b", QRegularExpression::CaseInsensitiveOption);
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
                if (elements[j].contains(rx))
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

void FileHandler::ReadITC()
{
    m_rows = 1;

    ThermogramHandler* thermogram = new ThermogramHandler;
    thermogram->setThermogramParameter(m_thermogram_parameter);
    qreal offset = 0;
    std::vector<PeakPick::Peak> peak_list;
    qreal freq = 0;
    QVector<qreal> inject;
    if (m_plain_thermogram) {
        QPair<Vector, Vector> pair = ToolSet::LoadXYFile(m_filename);
        PeakPick::spectrum spectrum = PeakPick::spectrum(pair.first, pair.second);
        thermogram->setThermogram(spectrum);
        thermogram->setThermogramParameter(m_thermogram_parameter);
        inject.fill(m_thermogram_parameter["InjectVolume"].toDouble(), m_thermogram_parameter["PeakCount"].toInt());
    } else {
        QPair<PeakPick::spectrum, QJsonObject> pair = ToolSet::LoadITCFile(m_filename, &peak_list, offset, freq, inject);
        PeakPick::spectrum spectrum = pair.first;
        m_systemparameter = pair.second;
        thermogram->setThermogram(spectrum);
        thermogram->setPeakList(peak_list);
    }
    thermogram->Initialise();
    thermogram->UpdatePeaks();
    thermogram->AdjustIntegrationRange();
    thermogram->IntegrateThermogram();

    QVector<qreal> integrals = thermogram->IntegralsScaled();
    m_stored_table = new DataTable;
    for (int i = 0; i < integrals.size(); ++i) {
        QVector<qreal> row;
        row << inject[i] << integrals[i];
        m_stored_table->insertRow(row);
    }
    ConvertTable();
    QJsonObject experiment;
    experiment["fit"] = thermogram->getThermogramParameter();
    experiment["file"] = m_filename;
    QJsonObject raw;
    raw["experiment"] = experiment;
    m_topjson["raw"] = raw;
    delete thermogram;

    QJsonObject data;
    data["data"] = m_topjson;
    m_topjson = data;
}

void FileHandler::ReadJson()
{
    JsonHandler::ReadJsonFile(m_topjson, m_filename);
}

QPointer<DataTable> FileHandler::getDataRange(int startRow, int endRow, int startCol, int endCol) const
{
    if (!m_stored_table) {
        qDebug() << "ERROR: No data table loaded";
        return nullptr;
    }
    
    // Use instance variables if parameters not provided - Claude Generated
    int actualStartRow = (startRow >= 0) ? startRow : m_start_row;
    int actualEndRow = (endRow >= 0) ? endRow : m_end_row;
    int actualStartCol = (startCol >= 0) ? startCol : m_start_col;
    int actualEndCol = (endCol >= 0) ? endCol : m_end_col;
    
    // Validate and adjust ranges
    int maxRows = m_stored_table->rowCount();
    int maxCols = m_stored_table->columnCount();
    
    if (actualStartRow < 0) actualStartRow = 0;
    if (actualStartRow >= maxRows) actualStartRow = maxRows - 1;
    
    if (actualEndRow < 0 || actualEndRow >= maxRows) actualEndRow = maxRows - 1;
    if (actualEndRow < actualStartRow) actualEndRow = actualStartRow;
    
    if (actualStartCol < 0) actualStartCol = 0;
    if (actualStartCol >= maxCols) actualStartCol = maxCols - 1;
    
    if (actualEndCol < 0 || actualEndCol >= maxCols) actualEndCol = maxCols - 1;
    if (actualEndCol < actualStartCol) actualEndCol = actualStartCol;
    
    // Calculate dimensions
    int numRows = actualEndRow - actualStartRow + 1;
    int numCols = actualEndCol - actualStartCol + 1;
    
    qDebug() << QString("Extracting data range: rows %1-%2 (%3 rows), cols %4-%5 (%6 cols)")
                .arg(actualStartRow).arg(actualEndRow).arg(numRows)
                .arg(actualStartCol).arg(actualEndCol).arg(numCols);
    
    // Use DataTable's Block method to extract range
    QPointer<DataTable> rangeTable = m_stored_table->Block(actualStartRow, actualStartCol, numRows, numCols);
    
    if (!rangeTable) {
        qDebug() << "ERROR: Failed to extract data range";
        return nullptr;
    }
    
    qDebug() << QString("Successfully extracted %1x%2 data range").arg(rangeTable->rowCount()).arg(rangeTable->columnCount());
    
    return rangeTable;
}

#include "filehandler.moc"

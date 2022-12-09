/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2022  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/global.h"
#include "src/global_config.h"

#include "src/core/models/datatable.h"
#include "src/core/toolset.h"

#include <Eigen/Dense>

#include <QtCore/QAbstractTableModel>
#include <QtCore/QCollator>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QPointer>
#include <QtCore/QReadWriteLock>
#include <QtCore/QString>
#include <QtCore/QUuid>
#include <QtCore/QtGlobal>

#include <cmath>
#include <iostream>
#include <random>

#include "datatable.h"

DataTable::DataTable(QObject* parent)
    : QAbstractTableModel(parent)
    , m_checkable(false)
    , m_editable(false)
{
}

DataTable::DataTable(int rows, int columns, QObject* parent)
    : QAbstractTableModel(parent)
    , m_checkable(false)
    , m_editable(false)
{
    m_table = Eigen::MatrixXd::Zero(rows, columns);
    m_checked_table = Eigen::MatrixXd::Ones(rows, columns);
    for (int i = 0; i < columns; ++i)
        m_header << QString::number(i + 1);
}

DataTable::DataTable(DataTable& other)
    : QAbstractTableModel(&other) // FIXME whatever
{
    m_table = Eigen::MatrixXd(other.m_table);
    m_header = other.m_header;
    m_checked_table = Eigen::MatrixXd(other.m_checked_table);
    m_checkable = other.m_checkable;
    m_editable = other.m_editable;
}

DataTable::DataTable(DataTable* other) //: QAbstractTableModel(other) FIXME whatever
{
    m_table = Eigen::MatrixXd(other->m_table);
    m_header = other->m_header;
    m_checked_table = Eigen::MatrixXd(other->m_checked_table);
    m_checkable = other->m_checkable;
    m_editable = other->m_editable;
}

DataTable::DataTable(Eigen::MatrixXd table, Eigen::MatrixXd checked_table, const QStringList& header)
    : m_table(table)
    , m_checked_table(checked_table)
    , m_checkable(false)
    , m_editable(false)
{
    if (header.size() != m_table.cols()) {
        for (int i = 0; i < columnCount(); ++i)
            m_header << QString::number(i + 1);
    } else
        m_header = header;
}

DataTable::DataTable(Eigen::MatrixXd table)
    : m_table(table)
    , m_checkable(false)
    , m_editable(false)
{
    for (int i = 0; i < columnCount(); ++i)
        m_header << QString::number(i + 1);
    m_checked_table = Eigen::MatrixXd::Ones(table.rows(), table.cols());
}

DataTable::DataTable(const QJsonObject& table)
    : m_checkable(false)
    , m_editable(false)
{
    ImportTable(table);
}

DataTable::~DataTable()
{
#ifdef _DEBUG
// std::cout << "deleting table " << this << std::endl;
#endif
}

bool DataTable::isValid() const
{
    return m_header.size();
}

void DataTable::clear(int columns, int rows)
{
    m_table = Eigen::MatrixXd::Zero(rows, columns);
    m_checked_table = Eigen::MatrixXd::Ones(rows, columns);
    for (int i = 0; i < columns; ++i)
        m_header << QString::number(i + 1);
}

Vector DataTable::firstRow()
{
    return m_table.row(0);
}

Vector DataTable::lastRow()
{
    return m_table.row(m_table.rows() - 1);
}

void DataTable::Debug(const QString& str) const
{
    std::cout << str.toStdString() << std::endl;
    std::cout << "Table Content" << std::endl;
    std::cout << "Rows: " << m_table.rows() << " Cols: " << m_table.cols() << std::endl;
    std::cout << m_table << std::endl;

    std::cout << "Checked Table" << std::endl;
    std::cout << "Rows: " << m_table.rows() << " Cols: " << m_table.cols() << std::endl;
    std::cout << m_checked_table << std::endl;
}

Qt::ItemFlags DataTable::flags(const QModelIndex& index) const
{
    Q_UNUSED(index);
    Qt::ItemFlags flags;
    if (m_checkable)
        flags = Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsUserCheckable;
    else
        flags = Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;

    if (m_editable)
        flags = flags | Qt::ItemIsEditable;

    return flags;
}

int DataTable::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    if (m_table.size() != 0)
        return m_table.cols();
    return 0;
}

QVariant DataTable::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Orientation::Horizontal) {
            if (section < m_header.size())
                return QVariant(QString(m_header.at(section)));

        } else
            return section + 1;
    }
    return QVariant();
}

bool DataTable::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
{
    if (section < m_header.size() && (role == Qt::DisplayRole || role == Qt::EditRole) && orientation == Qt::Orientation::Horizontal) {
        m_header[section] = value.toString();
        return true;
    }
    return false;
}

int DataTable::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return m_table.rows();
}

QVariant DataTable::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        if (m_editable)
            return Print::printDouble(data(index.row(), index.column()), 8);
        else
            return Print::printDouble(data(index.row(), index.column()), 8);
    else if (role == Qt::CheckStateRole && m_checkable) {
        const bool checked = isChecked(index.row(), index.column());
        return checked;
    } else
        return QVariant();
}

qreal& DataTable::operator[](int column)
{
    QReadLocker locker(&mutex);
    return m_table.operator()(0, column);
}

qreal& DataTable::operator()(int row, int column)
{
    QReadLocker locker(&mutex);
    m_empty = 0;
    return m_table.operator()(row, column);
}

qreal DataTable::data(int row, int column) const
{
    return m_table(row, column);
}

qreal& DataTable::data(int row, int column)
{
    QMutexLocker locker(&m_lock);
    return m_table.operator()(row, column);
}

bool DataTable::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::EditRole) {
        bool ok;
        qreal var = value.toDouble(&ok);
        if (ok) {
            data(index.row(), index.column()) = var;
            emit dataChanged(index, index);
        }
        return ok;
    } else if (role == Qt::CheckStateRole) {
        if (m_checked_table(index.row(), index.column()) == 0)
            m_checked_table(index.row(), index.column()) = 1;
        else
            m_checked_table(index.row(), index.column()) = 0;
        emit dataChanged(index, index);
        emit CheckedStateChanged(index.row(), m_checked_table(index.row(), index.column()));
        return true;
    }

    return false;
}

void DataTable::setCheckedAll(bool checked)
{
    if (checked)
        m_checked_table = Eigen::MatrixXd::Ones(rowCount(), columnCount());
    else
        m_checked_table = Eigen::MatrixXd::Zero(rowCount(), columnCount());
}

void DataTable::CheckRow(int row)
{
    int check = 0;
    for (int i = 0; i < columnCount(); ++i)
        check += m_checked_table(row, i);
    bool checked = check < columnCount() / 2;
    for (int i = 0; i < columnCount(); ++i)
        m_checked_table(row, i) = checked;
    emit layoutChanged();
}

void DataTable::CheckRow(int row, bool check)
{
    for (int i = 0; i < columnCount(); ++i)
        m_checked_table(row, i) = check;
    emit layoutChanged();
}

int DataTable::isRowChecked(int row) const
{
    int check = 0;
    for (int i = 0; i < columnCount(); ++i)
        check += m_checked_table(row, i);
    return check;
}

int DataTable::EnabledRows() const
{
    int value = 0;
    for (int i = 0; i < rowCount(); ++i) {
        value += isRowChecked(i);
    }
    return value;
}

Vector DataTable::DisableRow(int row)
{
    for (int i = 0; i < columnCount(); ++i)
        m_checked_table(row, i) = 0;
    emit layoutChanged();
    return Row(row);
}

void DataTable::EnableAllRows()
{
    for (int j = 0; j < rowCount(); ++j)
        for (int i = 0; i < columnCount(); ++i)
            m_checked_table(j, i) = 1;
    emit layoutChanged();
}

void DataTable::PrintCheckedRows() const
{
    for (int i = 0; i < rowCount(); ++i) {
        int check = 0;
        for (int j = 0; j < columnCount(); ++j)
            check += m_checked_table(i, j);
        std::cout << "Row " << i << " checked: " << check << std::endl;
    }
}

bool DataTable::isChecked(int row, int column) const
{
    return m_checked_table(row, column);
}

void DataTable::setChecked(int row, int column, bool checked)
{
    m_checked_table(row, column) = checked;
}


QPointer<DataTable> DataTable::Block(int row_begin, int column_begin, int row_end, int column_end) const
{
    if (row_begin < 0 || column_begin < 0 || row_begin >= rowCount() || column_begin >= columnCount() || row_end < 0 || column_end < 0 || row_end > rowCount() || column_end > columnCount())
        return new DataTable;

    Eigen::MatrixXd table = m_table.block(row_begin, column_begin, row_end, column_end);
    Eigen::MatrixXd checked_table = m_checked_table.block(row_begin, column_begin, row_end, column_end);
    QStringList header = QStringList(m_header.begin() + column_begin, m_header.begin() + column_end + column_begin);
    return new DataTable(table, checked_table, header);
}

Vector DataTable::Column(int column) const
{
    return m_table.col(column);
}

Vector DataTable::CheckedRow(int row) const
{
    return m_checked_table.row(row);
}

Vector DataTable::Row(int row) const
{
    return m_table.row(row);
}

Vector DataTable::Row(int row, const QList<int>& active) const
{
    Vector vector = m_table.row(row);
    if (vector.cols() != active.size())
        return vector;
    int size = 0;
    for (int i = 0; i < active.size(); ++i)
        if (active[i] == 1)
            size++;
    Vector vect(size);
    int pos = 0;
    for (int i = 0; i < active.size(); ++i) {
        if (active[i] == 1) {
            vect(pos) = vector(i);
            pos++;
        }
    }
    return vect;
}

void DataTable::append(const QPointer<DataTable> table)
{
    QReadLocker locker(&mutex);
    int oldsize = m_table.rows();
    if (m_table.cols() == 0) {
        m_table.conservativeResize(m_table.rows() + table->rowCount(), table->columnCount());
        m_checked_table.conservativeResize(m_checked_table.rows() + table->rowCount(), table->columnCount());
    } else {
        m_table.conservativeResize(m_table.rows() + table->rowCount(), m_table.cols());
        m_checked_table.conservativeResize(m_checked_table.rows() + table->rowCount(), m_checked_table.cols());
    }
    QStringList header = table->header();
    while (m_header.size() < header.size())
        m_header << QString::number(m_header.size() + 1);

    for (int i = 0; i < table->rowCount(); ++i) {
        auto row = table->Row(i);
        auto checked = table->CheckedRow(i);
        for (int j = 0; j < row.size(); ++j) {
            m_table(oldsize + i, j) = row(j);
            m_checked_table(oldsize + i, j) = checked(j);
        }
    }
    return;
    /*
        for (int i = 0; i < table->rowCount(); ++i)
        //    insertRow(table->Row(i), table->CheckedRow(i));
        {

        while (m_header.size() < row.size())
            m_header << QString::number(m_header.size() + 1);

        if (m_table.cols() == 0) {
            m_table.conservativeResize(m_table.rows() + 1, row.size());
            m_checked_table.conservativeResize(m_checked_table.rows() + 1, row.size());
        } else {
            m_table.conservativeResize(m_table.rows() + 1, m_table.cols());
            m_checked_table.conservativeResize(m_checked_table.rows() + 1, m_checked_table.cols());
        }

        // if(checked.size() < row.size())
        //     checked = Vector::Ones(row.size());

        for (int i = 0; i < row.size(); ++i) {
            m_table(m_table.rows() - 1, i) = row(i);
            m_checked_table(m_checked_table.rows() - 1, i) = checked(i);
        }
        }
        */
}
void DataTable::prepend(QPointer<DataTable> table)
{
    for (int i = 0; i < rowCount(); ++i)
        table->insertRow(Row(i), CheckedRow(i));
    ImportTable(table->ExportTable(true));
}
void DataTable::insertRow(const Vector& row, const Vector& checked)
{
    QReadLocker locker(&mutex);
    while (m_header.size() < row.size())
        m_header << QString::number(m_header.size() + 1);

    if (m_table.cols() == 0) {
        m_table.conservativeResize(m_table.rows() + 1, row.size());
        m_checked_table.conservativeResize(m_checked_table.rows() + 1, row.size());
    } else {
        m_table.conservativeResize(m_table.rows() + 1, m_table.cols());
        m_checked_table.conservativeResize(m_checked_table.rows() + 1, m_checked_table.cols());
    }

    // if(checked.size() < row.size())
    //     checked = Vector::Ones(row.size());

    for (int i = 0; i < row.size(); ++i) {
        m_table(m_table.rows() - 1, i) = row(i);
        m_checked_table(m_checked_table.rows() - 1, i) = checked(i);
    }
}

void DataTable::insertRow(const Vector& row, bool zero)
{
    QReadLocker locker(&mutex);
    while (m_header.size() < row.size())
        m_header << QString::number(m_header.size() + 1);

    if (m_table.cols() == 0) {
        m_table.conservativeResize(m_table.rows() + 1, row.size());
        m_checked_table.conservativeResize(m_checked_table.rows() + 1, row.size());
    } else {
        m_table.conservativeResize(m_table.rows() + 1, m_table.cols());
        m_checked_table.conservativeResize(m_checked_table.rows() + 1, m_checked_table.cols());
    }

    for (int i = 0; i < row.size(); ++i) {
        m_table(m_table.rows() - 1, i) = row[i];
        if (zero)
            m_checked_table(m_checked_table.rows() - 1, i) = row[i] != 0;
        else
            m_checked_table(m_checked_table.rows() - 1, i) = 1;
    }
}

void DataTable::insertRow(const QVector<qreal>& row, bool zero)
{
    QReadLocker locker(&mutex);
    while (m_header.size() < row.size())
        m_header << QString::number(m_header.size() + 1);

    if (m_table.cols() == 0) {
        m_table.conservativeResize(m_table.rows() + 1, row.size());
        m_checked_table.conservativeResize(m_checked_table.rows() + 1, row.size());
    } else {
        m_table.conservativeResize(m_table.rows() + 1, m_table.cols());
        m_checked_table.conservativeResize(m_checked_table.rows() + 1, m_checked_table.cols());
    }

    for (int i = 0; i < row.size(); ++i) {
        m_table(m_table.rows() - 1, i) = row[i];
        if (zero)
            m_checked_table(m_checked_table.rows() - 1, i) = row[i] != 0;
        else
            m_checked_table(m_checked_table.rows() - 1, i) = 1;
    }
}

void DataTable::appendColumns(const DataTable& table, bool keep_header)
{
    QReadLocker locker(&mutex);

    int rows = qMax(rowCount(), table.rowCount());
    int cols = columnCount() + table.columnCount();

    Eigen::MatrixXd tab = Eigen::MatrixXd::Zero(rows, cols);
    Eigen::MatrixXd check = Eigen::MatrixXd::Ones(rows, cols);

    for (int i = 0; i < columnCount(); ++i)
        for (int j = 0; j < rowCount(); ++j) {
            tab(i, j) = data(j, i);
            // check(j,i) = m_checked_table(j,i);
        }

    for (int i = 0; i < table.columnCount(); ++i)
        for (int j = 0; j < table.rowCount(); ++j) {
            tab(j, columnCount() + i) = table.data(i, j);
            // check(j,columnCount() + i) = m_checked_table(j,i);
        }
    m_table = tab;
    m_checked_table = check;

    if (keep_header)
        m_header << table.header();
    else {
        while (m_header.size() < m_table.cols())
            m_header << QString::number(m_table.cols());
    }

    if (rowCount() && columnCount())
        emit layoutChanged();
}

void DataTable::prependColumns(const DataTable& table, bool keep_header)
{
    QReadLocker locker(&mutex);

    int rows = qMax(rowCount(), table.rowCount());
    int cols = columnCount() + table.columnCount();

    Eigen::MatrixXd tab = Eigen::MatrixXd::Zero(rows, cols);
    Eigen::MatrixXd check = Eigen::MatrixXd::Ones(rows, cols);

    for (int i = 0; i < table.columnCount(); ++i)
        for (int j = 0; j < table.rowCount(); ++j) {
            tab(i, j) = table.data(j, i);
            // check(j,columnCount() + i) = m_checked_table(j,i);
        }

    for (int i = 0; i < columnCount(); ++i)
        for (int j = 0; j < rowCount(); ++j) {
            tab(table.columnCount() + i, j) = data(j, i);
            // check(j,i) = m_checked_table(j,i);
        }

    m_table = tab;
    m_checked_table = check;

    if (keep_header) {
        for (const auto& head : table.header())
            m_header.prepend(head);
    } else {
        while (m_header.size() < m_table.cols())
            m_header.append(QString::number(m_table.cols()));
    }

    if (rowCount() && columnCount())
        emit layoutChanged();
}

void DataTable::setColumn(const QVector<qreal>& vector, int column)
{
    Q_UNUSED(vector);
    Q_UNUSED(column);
    return;
}

void DataTable::setColumn(const Vector& vector, int column)
{
    if (m_table.cols() >= column)
        m_table.col(column) = vector;
    return;
}

void DataTable::setRow(const QVector<qreal>& vector, int row)
{
    QReadLocker locker(&mutex);
    if (m_table.rows() >= row)
        for (int i = 0; i < vector.size(); ++i)
            m_table(row, i) = vector[i];
    return;
}

void DataTable::setRow(const Vector& vector, int row)
{
    QReadLocker locker(&mutex);
    if (m_table.rows() >= row)
        m_table.row(row) = vector;
    return;
}

QPointer<DataTable> DataTable::PrepareMC(std::normal_distribution<double>& Phi, std::mt19937& rng, QVector<int> cols)
{
    if (cols.size() < columnCount()) {
        cols = QVector<int>(columnCount(), 1);
    }
    QPointer<DataTable> table = new DataTable(this);
    for (int j = 0; j < columnCount(); ++j) {
        if (!cols[j])
            continue;
        for (int i = 0; i < rowCount(); ++i) {
            double randed = Phi(rng);
            table->data(i, j) += randed;
        }
    }
    return table;
}

QPointer<DataTable> DataTable::PrepareMC(QVector<double> stddev, std::mt19937& rng, QVector<int> cols)
{
    while (stddev.size() < columnCount())
        stddev << stddev.last();

    QVector<std::normal_distribution<double>> _Phi;
    for (int i = 0; i < stddev.size(); ++i)
        _Phi << std::normal_distribution<double>(0, stddev[i]);

    if (cols.size() < columnCount()) {
        cols = QVector<int>(columnCount(), 1);
    }
    QPointer<DataTable> table = new DataTable(this);
    for (int j = 0; j < columnCount(); ++j) {
        if (!cols[j])
            continue;
        for (int i = 0; i < rowCount(); ++i) {
            double randed = _Phi[j](rng);
            table->data(i, j) += randed;
        }
    }
    return table;
}

QPointer<DataTable> DataTable::PrepareBootStrap(std::uniform_int_distribution<int>& Uni, std::mt19937& rng, const QVector<qreal>& vector)
{
    QPointer<DataTable> table = new DataTable(this);
    for (int j = 0; j < columnCount(); ++j) {
        for (int i = 0; i < rowCount(); ++i) {
            int randed = Uni(rng);
            table->data(i, j) += vector[randed];
        }
    }
    return table;
}

QString DataTable::ExportAsString() const
{
    QString str;
    str += "#" + m_header.join("\t") + "\n";
    for (int i = 0; i < rowCount(); ++i) {
        for (int j = 0; j < columnCount(); ++j) {
            str += Print::printDouble(data(i, j));
            /*
            if(data(i,j) >= 0)
                str += " ";
            if(qAbs(data(i,j) - int(data(i,j))) > 1E-6) // lets check if the number is an integer
                str += QString::number(data(i,j), 'e', 6);
            else //and
                str += QString::number(data(i,j)); // return no digits*/
            if (j < columnCount() - 1)
                str += "\t";
        }
        str += "\n";
    }
    return str;
}

QStringList DataTable::ExportAsStringList() const
{
    QStringList list;
    for (int i = 0; i < rowCount(); ++i) {
        QString str;
        for (int j = 0; j < columnCount(); ++j) {
            str += Print::printDouble(data(i, j));
            /*
            if(data(i,j) >= 0)
                str += " ";
            if(qAbs(data(i,j) - int(data(i,j))) > 1E-6) // lets check if the number is an integer
                str += Print::printDouble(data(i,j));
            else //and
                str += QString::number(data(i,j)); // return no digits*/
            if (j < columnCount() - 1)
                str += "\t";
        }
        list << str;
    }
    return list;
}

QVector<qreal> DataTable::toVector() const
{
    QVector<qreal> vector;
    for (int i = 0; i < rowCount(); ++i) {
        for (int j = 0; j < columnCount(); ++j) {
            vector << data(i, j);
        }
    }
    return vector;
}

QList<qreal> DataTable::toList() const
{
    QList<qreal> list;
    for (int i = 0; i < rowCount(); ++i) {
        for (int j = 0; j < columnCount(); ++j) {
            list << data(i, j);
        }
    }
    return list;
}

QJsonObject DataTable::ExportTable(bool checked, const QVector<int> checked_table) const
{
    QJsonObject table, data, check;

    for (int i = 0; i < rowCount(); ++i) {
        data[QString::number(i)] = ToolSet::DoubleList2String(Row(i));
        if (checked_table.size() == m_checked_table.row(i).size())
            check[QString::number(i)] = ToolSet::IntVec2String(ToolSet::VecAndVec(m_checked_table.row(i), checked_table));
        else
            check[QString::number(i)] = ToolSet::DoubleList2String(m_checked_table.row(i));
    }

    table["data"] = data;
    if (checked)
        table["checked"] = check;
    table["rows"] = rowCount();
    table["cols"] = columnCount();

    QStringList headers = QStringList() << header();
    table["header"] = headers.join("|");

    return table;
}

bool DataTable::ImportTable(const QJsonObject& table)
{
    int rows = qMax(table["rows"].toInt(), rowCount());
    int cols = qMax(table["cols"].toInt(), columnCount());

    if (rowCount() != rows || cols != columnCount()) {
        m_table = Eigen::MatrixXd::Zero(rows, cols);
        m_checked_table = Eigen::MatrixXd::Ones(rows, cols);
    }

    bool check = table.contains("checked");
    QJsonObject checked;
    QJsonObject data = table["data"].toObject();
    if (check)
        checked = table["checked"].toObject();

    for (int i = 0; i < rows; ++i) {
        QString str = QString::number(i);
        QVector<qreal> vector;
        vector = ToolSet::String2DoubleVec(data[str].toString());
        setRow(vector, i);

        if (check) {
            QVector<qreal> vector;
            vector = ToolSet::String2DoubleVec(checked[str].toString());
            for (int j = 0; j < vector.size(); ++j)
                m_checked_table(i, j) = vector[j];
        }
    }

    m_header = table["header"].toString().split("|");
    emit layoutChanged();
    return true;
}

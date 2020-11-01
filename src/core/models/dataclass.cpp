/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "dataclass.h"

DataTable::DataTable(QObject* parent)
    : QAbstractTableModel(parent)
    , m_checkable(false)
    , m_editable(false)
{
}

DataTable::DataTable(int columns, int rows, QObject* parent)
    : QAbstractTableModel(parent)
    , m_checkable(false)
    , m_editable(false)
{
    // std::cout << "new table with ptr" << this << std::endl;
    m_table = Eigen::MatrixXd::Zero(rows, columns);
    m_checked_table = Eigen::MatrixXd::Ones(rows, columns);
    for (int i = 0; i < columns; ++i)
        m_header << QString::number(i + 1);
}

DataTable::DataTable(DataTable& other)
    : QAbstractTableModel(&other) //FIXME whatever
{
    m_table = Eigen::MatrixXd(other.m_table);
    m_header = other.m_header;
    m_checked_table = Eigen::MatrixXd(other.m_checked_table);
    m_checkable = other.m_checkable;
    m_editable = other.m_editable;
}

DataTable::DataTable(DataTable* other) //: QAbstractTableModel(other) FIXME whatever
{
    //  std::cout << "copying by pointer" << this << std::endl;

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
            return Print::printDouble(data(index.column(), index.row()), 8);
        else
            return Print::printDouble(data(index.column(), index.row()), 8);
    else if (role == Qt::CheckStateRole && m_checkable)
        return isChecked(index.column(), index.row());
    else
        return QVariant();
}

qreal& DataTable::operator[](int column)
{
    QReadLocker locker(&mutex);
#ifdef _DEBUG
    m_empty = 0;

    if (column < m_table.cols()) {
        return m_table.operator()(0, column);
    } else {
        qDebug() << "Column exceeds size of table!";
        return m_empty;
    }
#else
    return m_table.operator()(0, column);
#endif
}

qreal& DataTable::operator()(int column, int row)
{
    QReadLocker locker(&mutex);
    m_empty = 0;
#ifdef _DEBUG
    if (row < m_table.rows())
        if (column < m_table.cols()) {
            return m_table.operator()(row, column);
        } else {
            qDebug() << "Column exceeds size of table!";
            return m_empty;
        }
    else {
        qDebug() << "Row exceeds size of table!";
        return m_empty;
    }
#else
    return m_table.operator()(row, column);
#endif
}

bool DataTable::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::EditRole) {
        bool ok;
        qreal var = value.toDouble(&ok);
        if (ok) {
            data(index.column(), index.row()) = var;
            emit dataChanged(index, index);
        }
        return ok;
    } else if (role == Qt::CheckStateRole) {
        if (m_checked_table(index.row(), index.column()) == 0)
            m_checked_table(index.row(), index.column()) = 1;
        else
            m_checked_table(index.row(), index.column()) = 0;
        emit dataChanged(index, index);
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

bool DataTable::isChecked(int column, int row) const
{
#ifdef _DEBUG
    if (row < m_checked_table.rows())
        if (column < m_checked_table.cols()) {
            return m_checked_table(row, column);
        } else {
            qDebug() << "Column exceeds size of table!";
            return 0;
        }
    else {
        qDebug() << "Row exceeds size of table!";
        return 0;
    }
#else
    return m_checked_table(row, column);
#endif
}

void DataTable::setChecked(int column, int row, bool checked)
{
#ifdef _DEBUG
    if (row < m_checked_table.rows())
        if (column < m_checked_table.cols()) {
            m_checked_table(row, column) = checked;
        } else {
            qDebug() << "Column exceeds size of table!";
        }
    else {
        qDebug() << "Row exceeds size of table!";
    }
#else
    m_checked_table(row, column) = checked;
#endif
}

qreal DataTable::data(int column, int row) const
{
#ifdef _DEBUG
    if (row < m_table.rows())
        if (column < m_table.cols()) {
            return m_table(row, column);
        } else {
            qDebug() << "Column exceeds size of table!";
            return 0;
        }
    else {
        qDebug() << "Row exceeds size of table!";
        return 0;
    }
#else
    return m_table(row, column);
#endif
}

qreal& DataTable::data(int column, int row)
{
    QMutexLocker locker(&m_lock);
#ifdef _DEBUG
    m_empty = 0;
    if (row < m_table.rows())
        if (column < m_table.cols()) {
            return m_table.operator()(row, column);
        } else {
            qDebug() << "Column exceeds size of table!";
            return m_empty;
        }
    else {
        qDebug() << "Row exceeds size of table!";
        return m_empty;
    }
#else
    return m_table.operator()(row, column);
#endif
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
    for (int i = 0; i < table->rowCount(); ++i)
        insertRow(table->Row(i), table->CheckedRow(i));
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
            tab(j, i) = data(i, j);
            //check(j,i) = m_checked_table(j,i);
        }

    for (int i = 0; i < table.columnCount(); ++i)
        for (int j = 0; j < table.rowCount(); ++j) {
            tab(j, columnCount() + i) = table.data(i, j);
            //check(j,columnCount() + i) = m_checked_table(j,i);
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
            tab(j, i) = table.data(i, j);
            //check(j,columnCount() + i) = m_checked_table(j,i);
        }

    for (int i = 0; i < columnCount(); ++i)
        for (int j = 0; j < rowCount(); ++j) {
            tab(j, table.columnCount() + i) = data(i, j);
            //check(j,i) = m_checked_table(j,i);
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
            table->data(j, i) += randed;
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
            table->data(j, i) += randed;
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
            table->data(j, i) += vector[randed];
        }
    }
    return table;
}

QString DataTable::ExportAsString() const
{
    QString str;
    str += "#" + m_header.join("\t") + "\n";
    for (int j = 0; j < rowCount(); ++j) {
        for (int i = 0; i < columnCount(); ++i) {
            str += Print::printDouble(data(i, j));
            /*
            if(data(i,j) >= 0)
                str += " ";
            if(qAbs(data(i,j) - int(data(i,j))) > 1E-6) // lets check if the number is an integer
                str += QString::number(data(i,j), 'e', 6);
            else //and
                str += QString::number(data(i,j)); // return no digits*/
            if (i < columnCount() - 1)
                str += "\t";
        }
        str += "\n";
    }
    return str;
}

QStringList DataTable::ExportAsStringList() const
{
    QStringList list;
    for (int j = 0; j < rowCount(); ++j) {
        QString str;
        for (int i = 0; i < columnCount(); ++i) {
            str += Print::printDouble(data(i, j));
            /*
            if(data(i,j) >= 0)
                str += " ";
            if(qAbs(data(i,j) - int(data(i,j))) > 1E-6) // lets check if the number is an integer
                str += Print::printDouble(data(i,j));
            else //and
                str += QString::number(data(i,j)); // return no digits*/
            if (i < columnCount() - 1)
                str += "\t";
        }
        list << str;
    }
    return list;
}

QVector<qreal> DataTable::toVector() const
{
    QVector<qreal> vector;
    for (int j = 0; j < rowCount(); ++j) {
        for (int i = 0; i < columnCount(); ++i) {
            vector << data(i, j);
        }
    }
    return vector;
}

QList<qreal> DataTable::toList() const
{
    QList<qreal> list;
    for (int j = 0; j < rowCount(); ++j) {
        for (int i = 0; i < columnCount(); ++i) {
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
    int rows = table["rows"].toInt();
    int cols = table["cols"].toInt();

    if (rows != rowCount() || cols != columnCount()) {
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

DataClassPrivate::DataClassPrivate()
    : m_maxsize(0)
    , m_host_assignment(0)
    , m_datatype(DataClassPrivate::Table)
    , m_info(new DataClassPrivateObject)
{
    m_independent_model = new DataTable;
    m_dependent_model = new DataTable;

    QUuid uuid;
    m_uuid = uuid.createUuid().toString();

    m_dependent_model->setCheckable(true);
    if (m_independent_model->columnCount() != m_scaling.size())
        for (int i = 0; i < m_independent_model->columnCount(); ++i)
            m_scaling << 1;
    m_independent_model->setHeaderData(0, Qt::Horizontal, ("Host"), Qt::DisplayPropertyRole);
    m_independent_model->setHeaderData(1, Qt::Horizontal, ("Guest"), Qt::DisplayPropertyRole);
    //  std::cout << "stepping upwards DataClassPrivate::DataClassPrivate(): " << m_ref_counter << this << std::endl;
}

DataClassPrivate::DataClassPrivate(int type)
    : m_maxsize(0)
    , m_host_assignment(0)
    , m_datatype(DataClassPrivate::Table)
    , m_info(new DataClassPrivateObject)
{
    m_independent_model = new DataTable;
    m_dependent_model = new DataTable;
    m_dependent_model->setCheckable(true);

    QUuid uuid;
    m_uuid = uuid.createUuid().toString();

    if (m_independent_model->columnCount() != m_scaling.size())
        for (int i = 0; i < m_independent_model->columnCount(); ++i)
            m_scaling << 1;
    m_independent_model->setHeaderData(0, Qt::Horizontal, ("Host"), Qt::DisplayPropertyRole);
    m_independent_model->setHeaderData(1, Qt::Horizontal, ("Guest"), Qt::DisplayPropertyRole);
    //  std::cout << "stepping upwards DataClassPrivate::DataClassPrivate(): " << m_ref_counter << this  << std::endl;
}

DataClassPrivate::DataClassPrivate(const DataClassPrivate& other)
    : QSharedData(other)
    , m_info(other.m_info)
{
    m_independent_model = new DataTable(other.m_independent_model);
    m_systemObject = other.m_systemObject;
    m_uuid = other.m_uuid;
    m_scaling = other.m_scaling;
    m_host_assignment = other.m_host_assignment;
    m_dependent_model = new DataTable(other.m_dependent_model);
    m_raw_data = other.m_raw_data;
    m_system_parameter = other.m_system_parameter;
    m_datatype = other.m_datatype;
    m_title = other.m_title;
    m_root_dir = other.m_root_dir;
    m_ref_counter++;
    //  std::cout << "stepping upwards DataClassPrivate::DataClassPrivate(const DataClassPrivate& other): " << m_ref_counter << this  << std::endl;
}

DataClassPrivate::DataClassPrivate(const DataClassPrivate* other)
    : m_info(other->m_info)
{
    m_independent_model = new DataTable(other->m_independent_model);
    m_systemObject = other->m_systemObject;
    m_uuid = other->m_uuid;
    m_scaling = other->m_scaling;
    m_host_assignment = other->m_host_assignment;
    m_dependent_model = new DataTable(other->m_dependent_model);
    m_raw_data = other->m_raw_data;
    m_system_parameter = other->m_system_parameter;
    m_datatype = other->m_datatype;
    m_title = other->m_title;
    m_root_dir = other->m_root_dir;
    m_ref_counter++;
    //  std::cout << "stepping upwards DataClassPrivate::DataClassPrivate(const DataClassPrivate* other): " << m_ref_counter << this  << std::endl;
}

DataClassPrivate::~DataClassPrivate()
{
    --m_ref_counter;
    if (m_ref_counter < 1) {
        if (m_independent_model)
            delete m_independent_model;
        if (m_dependent_model)
            delete m_dependent_model;
        if (m_info)
            delete m_info;
#ifdef _DEBUG
        std::cout << "got away with it ..." << this << std::endl;
#endif
    }
#ifdef _DEBUG
    else
        std::cout << "still someone having data: " << m_ref_counter << this << std::endl;
#endif
}

void DataClassPrivate::printReferenz() const
{
    std::cout << this << " has " << m_ref_counter << " instances?" << std::endl;
}

void DataClassPrivate::check()
{
    std::cout << "Check of data " << std::endl;
    std::cout << "Concentration Table ## Row:" << m_independent_model->rowCount() << " Colums: " << m_independent_model->columnCount() << std::endl;
    std::cout << "Signal Table ## Row:" << m_dependent_model->rowCount() << " Colums: " << m_dependent_model->columnCount() << std::endl;
}

DataClass::DataClass(QObject* parent)
    : QObject(parent)
{
    d = new DataClassPrivate;

    connect(Info(), &DataClassPrivateObject::SystemParameterChanged, this, &DataClass::SystemParameterChanged);
    connect(Info(), &DataClassPrivateObject::Update, this, &DataClass::Update);
    connect(Info(), &DataClassPrivateObject::Message, this, &DataClass::Message);
    connect(Info(), &DataClassPrivateObject::Warning, this, &DataClass::Warning);
}

DataClass::DataClass(const QJsonObject& json, int type, QObject* parent)
    : QObject(parent)
{
    d = new DataClassPrivate();

    connect(Info(), &DataClassPrivateObject::SystemParameterChanged, this, &DataClass::SystemParameterChanged);
    connect(Info(), &DataClassPrivateObject::Update, this, &DataClass::Update);
    connect(Info(), &DataClassPrivateObject::Message, this, &DataClass::Message);
    connect(Info(), &DataClassPrivateObject::Warning, this, &DataClass::Warning);

    ImportData(json);
}

DataClass::DataClass(const DataClass& other)
    : QObject()
{
    d = other.d;
    connect(Info(), &DataClassPrivateObject::SystemParameterChanged, this, &DataClass::SystemParameterChanged);
    connect(Info(), &DataClassPrivateObject::Update, this, &DataClass::Update);
    connect(Info(), &DataClassPrivateObject::Message, this, &DataClass::Message);
    connect(Info(), &DataClassPrivateObject::Warning, this, &DataClass::Warning);
}

DataClass::DataClass(const DataClass* other)
    : QObject()
{
    d = other->d;
    connect(Info(), &DataClassPrivateObject::SystemParameterChanged, this, &DataClass::SystemParameterChanged);
    connect(Info(), &DataClassPrivateObject::Update, this, &DataClass::Update);
    connect(Info(), &DataClassPrivateObject::Message, this, &DataClass::Message);
    connect(Info(), &DataClassPrivateObject::Warning, this, &DataClass::Warning);
}

DataClass::~DataClass()
{
#ifdef _DEBUG
    std::cout << "delete dataclass" << std::endl;
    d->printReferenz();
#endif
}

void DataClass::NewUUID()
{
    QUuid uuid;
    d->m_uuid = uuid.createUuid().toString();
}

QList<double> DataClass::getSignals(QList<int> active_signal)
{
    if (active_signal.size() < SeriesCount())
        active_signal = QVector<int>(SeriesCount(), 1).toList();

    QList<double> x;
    for (int j = 0; j < SeriesCount(); ++j) {
        if (active_signal[j] != 1)
            continue;
        for (int i = 0; i < DataPoints(); ++i) {
            if (DependentModel()->isChecked(j, i))
                x.append(DependentModel()->data(j, i));
        }
    }
    return x;
}

void DataClass::SwitchConentrations()
{
    d->m_host_assignment = !HostAssignment();
    if (!d->m_host_assignment) {
        d->m_independent_model->setHeaderData(0, Qt::Horizontal, ("Host (A)"), Qt::DisplayRole);
        d->m_independent_model->setHeaderData(1, Qt::Horizontal, ("Guest (B)"), Qt::DisplayRole);
    } else {
        d->m_independent_model->setHeaderData(0, Qt::Horizontal, ("Guest (A)"), Qt::DisplayRole);
        d->m_independent_model->setHeaderData(1, Qt::Horizontal, ("Host (B)"), Qt::DisplayRole);
    }
}

const QJsonObject DataClass::ExportData() const
{
    QJsonObject json;

    json["independent"] = d->m_independent_model->ExportTable(true);
    json["dependent"] = d->m_dependent_model->ExportTable(true);
    json["system"] = d->m_systemObject;
    json["DataType"] = d->m_datatype;
    json["SupraFit"] = qint_version;
    json["raw"] = d->m_raw_data;
    json["title"] = d->m_title;
    json["uuid"] = d->m_uuid;
    json["git_commit"] = git_commit_hash;
    json["content"] = d->m_content;
    json["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    json["xaxis"] = m_plot_x;
    return json;
}

bool DataClass::ImportData(const QJsonObject& topjson, bool forceUUID)
{
    int fileversion = topjson["SupraFit"].toInt();

    if (fileversion > qint_version) {
        emit Info()->Warning(QString("One does not simply load this file. It appeared after Amon Hen!\nUpdating SupraFit to the latest version will fix this.\nCurrent fileversion is %1, version of saved file is %2").arg(qint_version).arg(fileversion));
        qWarning() << QString("One does not simply load this file. It appeared after Amon Hen!\nUpdating SupraFit to the latest version will fix this.\nCurrent fileversion is %1, version of saved file is %2. Get the most recent version from <a href='https://github.com/conradhuebler/SupraFit'>https://github.com/conradhuebler/SupraFit</a>").arg(qint_version).arg(fileversion);
        return false;
    }

    d->m_systemObject = topjson["system"].toObject();

    d->m_independent_model->ImportTable(topjson["independent"].toObject());
    d->m_dependent_model->ImportTable(topjson["dependent"].toObject());

    d->m_datatype = DataClassPrivate::DataType(topjson["DataType"].toInt());
    d->m_raw_data = topjson["raw"].toObject();
    d->m_title = topjson["title"].toString();
    d->m_content = topjson["content"].toString();
    m_plot_x = topjson["xaxis"].toBool();

    if (forceUUID) {
        if (!topjson["uuid"].toString().isEmpty())
            d->m_uuid = topjson["uuid"].toString();
        else {
            QUuid uuid;
            d->m_uuid = uuid.createUuid().toString();
        }
    }

    if (d->m_independent_model->columnCount() != d->m_scaling.size())
        for (int i = 0; i < d->m_independent_model->columnCount(); ++i)
            d->m_scaling << 1;

    emit Info()->Update();
    return true;
}

bool DataClass::LegacyImportData(const QJsonObject& topjson, bool forceUUID)
{
    int fileversion = topjson["SupraFit"].toInt();

    if (fileversion == qint_version)
        return ImportData(topjson, forceUUID);

    if (fileversion > qint_version) {
        emit Info()->Warning(QString("One does not simply load this file. It appeared after Amon Hen!\nUpdating SupraFit to the latest version will fix this.\nCurrent fileversion is %1, version of saved file is %2").arg(qint_version).arg(fileversion));
        qWarning() << QString("One does not simply load this file. It appeared after Amon Hen!\nUpdating SupraFit to the latest version will fix this.\nCurrent fileversion is %1, version of saved file is %2").arg(qint_version).arg(fileversion);
        return false;
    }

    m_plot_x = topjson["xaxis"].toBool();

    d->m_systemObject = topjson["system"].toObject();

    if (fileversion >= 1601) {
        d->m_independent_model->ImportTable(topjson["independent"].toObject());
        d->m_dependent_model->ImportTable(topjson["dependent"].toObject());
    } else {
        QJsonObject concentrationObject, signalObject;
        concentrationObject = topjson["concentrations"].toObject();
        signalObject = topjson["signals"].toObject();
        if (concentrationObject.isEmpty() || signalObject.isEmpty())
            return false;

        QStringList keys = signalObject.keys();

        QCollator collator;
        collator.setNumericMode(true);
        std::sort(
            keys.begin(),
            keys.end(),
            [&collator](const QString& key1, const QString& key2) {
                return collator.compare(key1, key2) < 0;
            });
        if (DataPoints() == 0) {
            for (const QString& str : qAsConst(keys)) {
                QVector<qreal> concentrationsVector, signalVector;
                concentrationsVector = ToolSet::String2DoubleVec(concentrationObject[str].toString());
                signalVector = ToolSet::String2DoubleVec(signalObject[str].toString());
                d->m_independent_model->insertRow(concentrationsVector);
                d->m_dependent_model->insertRow(signalVector);
            }
            QStringList header = topjson["header"].toString().split("|");
            setHeader(header);
            return true;
        } else if (keys.size() != DataPoints()) {
            qWarning() << "table size doesn't fit to imported data";
            return false;
        }
        for (const QString& str : qAsConst(keys)) {
            QVector<qreal> concentrationsVector, signalVector;
            concentrationsVector = ToolSet::String2DoubleVec(concentrationObject[str].toString());
            signalVector = ToolSet::String2DoubleVec(signalObject[str].toString());
            int row = str.toInt();
            d->m_independent_model->setRow(concentrationsVector, row);
            d->m_dependent_model->setRow(signalVector, row);
        }

        QStringList header = topjson["header"].toString().split("|");
        setHeader(header);
    }
    if (fileversion > 1602) {
        d->m_datatype = DataClassPrivate::DataType(topjson["DataType"].toInt());
        d->m_raw_data = topjson["raw"].toObject();
        d->m_title = topjson["title"].toString();
    }
    if (forceUUID) {
        if (fileversion > 1603) {
            if (!topjson["uuid"].toString().isEmpty())
                d->m_uuid = topjson["uuid"].toString();
            else {
                QUuid uuid;
                d->m_uuid = uuid.createUuid().toString();
            }
        } else {
            QUuid uuid;
            d->m_uuid = uuid.createUuid().toString();
        }
    }

    if (d->m_independent_model->columnCount() != d->m_scaling.size())
        for (int i = 0; i < d->m_independent_model->columnCount(); ++i)
            d->m_scaling << 1;

    return true;
}

void DataClass::LoadSystemParameter()
{
    for (int index : getSystemParameterList()) {
        if (d->m_systemObject[QString::number(index)].toString().isEmpty())
            continue;
        setSystemParameterValue(index, d->m_systemObject[QString::number(index)].toVariant());
    }

    emit Info()->SystemParameterLoaded();
}

void DataClass::setSystemObject(const QJsonObject& object)
{
    d->m_systemObject = object;
}

void DataClass::setHeader(const QStringList& strlist)
{
    if (strlist.size() == (d->m_independent_model->columnCount() + d->m_dependent_model->columnCount())) {
        for (int i = 0; i < strlist.size(); ++i) {
            if (i < d->m_independent_model->columnCount())
                d->m_independent_model->setHeaderData(i, Qt::Horizontal, (strlist[i]), Qt::DisplayRole);
            else
                d->m_dependent_model->setHeaderData(i - d->m_independent_model->columnCount(), Qt::Horizontal, (strlist[i]), Qt::DisplayRole);
        }
    }
}

void DataClass::OverrideInDependentTable(DataTable* table)
{
    d.detach();
    d->m_independent_model = table;
    IndependentModelOverride();
}

void DataClass::OverrideDependentTable(DataTable* table)
{
    d.detach();
    table->setCheckedTable(d->m_dependent_model->CheckedTable());
    d->m_dependent_model = table;
    DependentModelOverride();
}

void DataClass::OverrideCheckedTable(DataTable* table)
{
    d.detach();
    d->m_dependent_model->setCheckedTable(table->CheckedTable());
    CheckedModelOverride();
    DependentModelOverride();
}

void DataClass::addSystemParameter(int index, const QString& str, const QString& description, SystemParameter::Type type)
{
    if (d->m_system_parameter.contains(index))
        return;
    SystemParameter parameter(index, str, description, type);
    d->m_system_parameter.insert(index, parameter);
}

SystemParameter DataClass::getSystemParameter(int index) const
{
    return d->m_system_parameter.value(index);
}

QList<int> DataClass::getSystemParameterList() const
{
    return d->m_system_parameter.keys();
}

void DataClass::setSystemParameterValue(int index, const QVariant& value)
{
    if (!value.isValid())
        return;

    SystemParameter parameter = getSystemParameter(index);
    parameter.setValue(value);
    d->m_system_parameter[index] = parameter;
}

void DataClass::setSystemParameterList(int index, const QStringList& value)
{
    SystemParameter parameter = getSystemParameter(index);
    if (!parameter.isList())
        return;
    parameter.setList(value);
    d->m_system_parameter[index] = parameter;
}

void DataClass::setSystemParameter(const SystemParameter& parameter)
{
    int index = parameter.Index();
    if (d->m_system_parameter.contains(index))
        d->m_system_parameter[index] = parameter;
    emit Info()->SystemParameterChanged();
}

void DataClass::WriteSystemParameter()
{
    QJsonObject systemObject;
    for (const int index : getSystemParameterList()) {
        systemObject[QString::number(index)] = getSystemParameter(index).value().toString();
    }
    d->m_systemObject = systemObject;
}

QJsonObject DataClass::ExportChildren(bool statistics, bool locked)
{
    QJsonObject models;
    if (SFModel() == SupraFit::Data) {
        for (int i = 0; i < d->m_children.size(); ++i) {
            if (d->m_children[i]) {
                if (qobject_cast<const AbstractModel*>(d->m_children[i])) {
                    QPointer<AbstractModel> data = const_cast<AbstractModel*>(qobject_cast<const AbstractModel*>(d->m_children[i]));
                    if (data)
                        models[tr("model_%1").arg(i)] = data->ExportModel(statistics, locked);
                }
            }
        }
    } else {
        AbstractModel* data = qobject_cast<AbstractModel*>(this);
        models[tr("model_0")] = data->ExportModel(statistics, locked);
    }

    return models;
}

void DataClass::AddChildren(QPointer<DataClass> data)
{
    d->m_children << data;
    connect(data, &DataClass::Deleted, this, [this, data]() {
        d->m_children.removeAll(data);
#ifdef _DEBUG
        qDebug() << d->m_children.size();
#endif
    });
}

QString DataClass::Data2Text() const
{
    QString text;
    text += "#### Begin of Data Description ####\n";
    text += "Independent Data Structure :   " + QString::number(DataPoints()) + "\n";
    for (int i = 0; i < IndependentModel()->columnCount(); ++i)
        text += " " + IndependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    text += "\n";
    text += IndependentModel()->ExportAsString();
    text += "\n";
    text += "Dependent Data Structure :          " + QString::number(SeriesCount()) + "\n";
    for (int i = 0; i < DependentModel()->columnCount(); ++i)
        text += " " + DependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    text += "\n";
    text += DependentModel()->ExportAsString();
    text += "\n";
    text += Data2Text_Private();
    text += "#### End of Data Description #####\n";
    text += "******************************************************************************************************\n";
    return text;
}

/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#include "src/core/toolset.h"

#include <Eigen/Dense>

#include <QtCore/QString>
#include <QtCore/QCoreApplication>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QReadWriteLock>
#include <QtCore/QCollator>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QAbstractTableModel>
#include <QPointer>
#include <QDebug>
#include "dataclass.h"
#include <QtGlobal>
#include <cmath>
#include <random>
#include <iostream>

DataTable::DataTable(QObject* parent) : QAbstractTableModel(parent), m_checkable(false)
{
}

DataTable::DataTable(int columns, int rows, QObject* parent) : QAbstractTableModel(parent), m_checkable(false)
{
    m_table = Eigen::MatrixXd::Zero(rows, columns);
    m_checked_table = Eigen::MatrixXd::Ones(rows, columns);
    for(int i = 0; i < columns; ++i)
        m_header << QString::number(i + 1);
}

DataTable::DataTable(DataTable& other) : QAbstractTableModel(&other) //FIXME whatever
{
    m_table = other.m_table;
    m_header = other.m_header;
    m_checked_table = other.m_checked_table;
    m_checkable = other.m_checkable;
}

DataTable::DataTable(DataTable* other)//: QAbstractTableModel(other) FIXME whatever
{
    m_table = other->m_table;
    m_header = other->m_header;
    m_checked_table = other->m_checked_table;
    m_checkable = other->m_checkable;
}


DataTable::~DataTable()
{
    
    
    
}

Vector DataTable::firstRow()
{
    return m_table.row(0);
}


Vector DataTable::lastRow()
{
    return m_table.row(m_table.rows()-1);
}

void DataTable::Debug() const
{
    std::cout << "Rows: " << m_table.rows() << " Cols: " <<m_table.cols() << std::endl;
    std::cout << m_table <<std::endl;
}

Qt::ItemFlags DataTable::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    Qt::ItemFlags flags;
    if(m_checkable)
        flags = Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsUserCheckable;
    else
        flags = Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    
    return flags;
}

int DataTable::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    if(m_table.size() != 0)
        return m_table.cols();
    return 0;
    
}

QVariant DataTable::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Orientation::Horizontal)
    {
        if(section < m_header.size())
            return QVariant(m_header.at(section));
        else 
            return QVariant(section);
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

bool DataTable::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if(section < m_header.size() && role == Qt::EditRole)
    {
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
    if(role == Qt::DisplayRole || role == Qt::EditRole )
        return data(index.column(), index.row());
    else if (role == Qt::CheckStateRole && m_checkable)
        return isChecked(index.column(), index.row()); //m_checked_table(index.column(), index.row());
    else
        return QVariant();
}

bool DataTable::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(role == Qt::EditRole)
    {
        bool ok;
        qreal var = value.toDouble(&ok);
        if(ok)
        {
            data(index.column(), index.row()) = var;
            emit dataChanged(index, index);
        }
        return ok;
    }else if(role == Qt::CheckStateRole)
    {
        if(m_checked_table(index.row(), index.column()) == 0)
            m_checked_table(index.row(), index.column()) = 1;
        else
            m_checked_table(index.row(), index.column()) = 0;
        emit dataChanged(index, index);
        return true;
    }
    return false;
}


bool DataTable::isChecked(int column, int row) const
{
    if(row < m_checked_table.rows())
        if(column < m_checked_table.cols())
        {
            return m_checked_table(row,column);
        }
        else
        {
            qDebug() << "Column exceeds size of table!";
            return 0;
        }
        else
        {
            qDebug() << "Row exceeds size of table!";
            return 0;
        }
}

qreal DataTable::data(int column, int row) const
{

    if(row < m_table.rows())
        if(column < m_table.cols())
        {
            return m_table(row,column);
        }
        else
        {
            qDebug() << "Column exceeds size of table!";
            return 0;
        }
        else
        {
            qDebug() << "Row exceeds size of table!";
            return 0;
        }
}

qreal & DataTable::data(int column, int row)
{
    QReadLocker locker(&mutex);
    m_empty = 0;
    if(row < m_table.rows())
        if(column < m_table.cols())
        {
            return m_table(row,column);
        }
        else
        {
            qDebug() << "Column exceeds size of table!";
            return m_empty;
        }
        else
        {
            qDebug() << "Row exceeds size of table!";
            return m_empty;
        }
}

Vector DataTable::Row(int row)
{
    return m_table.row(row);
}

void DataTable::insertRow(const QVector<qreal> &row)
{
    while(m_header.size() < row.size())
        m_header << QString::number(m_header.size() + 1);
    
    if(m_table.cols() == 0)
    {
        m_table.conservativeResize(m_table.rows() + 1, row.size());
        m_checked_table.conservativeResize(m_checked_table.rows() + 1, row.size());
    }
    else
    {
        m_table.conservativeResize(m_table.rows() + 1, m_table.cols());
        m_checked_table.conservativeResize(m_checked_table.rows() + 1, m_checked_table.cols());
    }
    
    for(int i = 0; i < row.size(); ++i)
    {
        m_table(m_table.rows() -1, i) = row[i];
        m_checked_table(m_checked_table.rows() - 1, i) = 1;
    }
}

void DataTable::setColumn(const QVector<qreal> &vector, int column)
{
    Q_UNUSED(vector);
    Q_UNUSED(column);
    return;
}

void DataTable::setColumn(const Vector &vector, int column)
{
    if(m_table.cols() >= column)
        m_table.col(column) = vector; 
    return;
}

void DataTable::setRow(const QVector<qreal> &vector, int row)
{
    if(m_table.rows() >= row)
        for(int i = 0; i < vector.size(); ++i)
            m_table(row, i) = vector[i];   
    return;
}

void DataTable::setRow(const Vector &vector, int row)
{
    if(m_table.rows() >= row)
        m_table.row(row) = vector;   
    return;
}

DataTable* DataTable::PrepareMC(std::normal_distribution<double> &Phi, std::mt19937 &rng)
{
    DataTable *table = new DataTable(this);
    for(int j = 0; j <  columnCount(); ++j)
        {
            for(int i = 0; i < rowCount(); ++i)
            {
                double  randed = Phi(rng);
                table->data(j,i) += randed;
            }
        }
        return table;
}

DataClassPrivate::DataClassPrivate() : m_maxsize(0), m_host_assignment(0)
{
    m_concentration_model = new DataTable;
    m_signal_model = new DataTable;
    m_signal_model->setCheckable(true);
    m_raw_data = new DataTable;
    if(m_concentration_model->columnCount() != m_scaling.size())
        for(int i = 0; i < m_concentration_model->columnCount(); ++i)
            m_scaling << 1;
    m_concentration_model->setHeaderData(0, Qt::Horizontal, ("Host"), Qt::DisplayPropertyRole);
    m_concentration_model->setHeaderData(1, Qt::Horizontal, ("Guest"), Qt::DisplayPropertyRole);
}

DataClassPrivate::DataClassPrivate(int type) : m_type(type) , m_maxsize(0), m_host_assignment(0)
{
    m_concentration_model = new DataTable;
    m_signal_model = new DataTable;
    m_signal_model->setCheckable(true);
    m_raw_data = new DataTable;    
    if(m_concentration_model->columnCount() != m_scaling.size())
        for(int i = 0; i < m_concentration_model->columnCount(); ++i)
            m_scaling << 1;
    m_concentration_model->setHeaderData(0, Qt::Horizontal, ("Host"), Qt::DisplayPropertyRole);
    m_concentration_model->setHeaderData(1, Qt::Horizontal, ("Guest"), Qt::DisplayPropertyRole);
}


DataClassPrivate::DataClassPrivate(const DataClassPrivate& other) : QSharedData(other)
{
    m_concentration_model = new DataTable(other.m_concentration_model);
    
    m_scaling = other.m_scaling;
    m_host_assignment = other.m_host_assignment;
    m_signal_model = new DataTable(other.m_signal_model);
    m_raw_data = new DataTable(other.m_raw_data);
    m_type = other.m_type;
}

DataClassPrivate::DataClassPrivate(const DataClassPrivate* other) 
{
    m_concentration_model = new DataTable(other->m_concentration_model);
    
    m_scaling = other->m_scaling;
       m_host_assignment = other->m_host_assignment; 
    m_signal_model = new DataTable(other->m_signal_model);
    m_raw_data = new DataTable(other->m_raw_data);
    m_type = other->m_type;
}


DataClassPrivate::~DataClassPrivate()
{
    delete m_concentration_model;
    delete m_signal_model;
    delete m_raw_data;
}


void DataClassPrivate::check()
{
    std::cout << "Check of data " << std::endl;
    std::cout << "Concentration Table ## Row:" << m_concentration_model->rowCount() << " Colums: " << m_concentration_model->columnCount()<< std::endl;
    std::cout << "Signal Table ## Row:" << m_signal_model->rowCount() << " Colums: " << m_signal_model->columnCount()<< std::endl;
    std::cout << "Raw Table ## Row:" << m_raw_data->rowCount() << " Colums: " << m_raw_data->columnCount()<< std::endl;
    
}


DataClass::DataClass(QObject *parent) : QObject(parent)
{
    d = new DataClassPrivate;
}

DataClass::DataClass(const QJsonObject &json, int type, QObject *parent):  QObject(parent)
{
    d = new DataClassPrivate();
    d->m_type = type;
    ImportJSON(json);
    if(d->m_concentration_model->columnCount() != d->m_scaling.size())
    for(int i = 0; i < d->m_concentration_model->columnCount(); ++i)
        d->m_scaling << 1;
}

DataClass::DataClass(int type, QObject *parent) :  QObject(parent)
{
    d = new DataClassPrivate(type);
}

DataClass::DataClass(const DataClass& other): QObject()
{
    d = other.d;
}

DataClass::DataClass(const DataClass* other)
{
    d = other->d;
}

DataClass::~DataClass()
{

}


QList<double>   DataClass::getSignals(QList<int > active_signal)
{
    if(active_signal.size() < SignalCount() )
        active_signal = QVector<int>(SignalCount(), 1).toList();
    
    QList<double > x;
    for(int j = 0; j < SignalCount(); ++j)
    {
        for(int i = 0; i < DataPoints(); ++i)
        {
            if(active_signal[j] == 1)
                x.append(SignalModel()->data(j,i)); //[index] = SignalModel()->data(j,i); 
        }
    }
    return x;
}

void DataClass::SwitchConentrations()
{
    d->m_host_assignment = !HostAssignment();
    if(!d->m_host_assignment)
    {
       d->m_concentration_model->setHeaderData(0, Qt::Horizontal, ("Host"), Qt::DisplayPropertyRole);
       d->m_concentration_model->setHeaderData(1, Qt::Horizontal, ("Guest"), Qt::DisplayPropertyRole);
    }else
    {
       d->m_concentration_model->setHeaderData(0, Qt::Horizontal, ("Guest"), Qt::DisplayPropertyRole);
       d->m_concentration_model->setHeaderData(1, Qt::Horizontal, ("Host"), Qt::DisplayPropertyRole);
    }
}

qreal DataClass::InitialGuestConcentration(int i)
{
    return d->m_concentration_model->data(!HostAssignment(),i)*d->m_scaling[!HostAssignment()];
}

qreal DataClass::InitialHostConcentration(int i)
{
    return d->m_concentration_model->data(HostAssignment(),i)*d->m_scaling[HostAssignment()];
}


const QJsonObject DataClass::ExportJSON() const
{
    QJsonObject json;
    
    QJsonObject concentrationObject, signalObject;
    
    for(int i = 0; i < DataPoints(); ++i)
    {
        concentrationObject[QString::number(i)] = ToolSet::DoubleList2String(d->m_concentration_model->Row(i));
        signalObject[QString::number(i)] = ToolSet::DoubleList2String(d->m_signal_model->Row(i));
    }
    
    json["concentrations"] = concentrationObject;
    json["signals"] = signalObject;
    json["datatype"] = QString("discrete");
    json["header"] = (QStringList() << d->m_concentration_model->header() << d->m_signal_model->header()).join("|");
    return json;
}


bool DataClass::ImportJSON(const QJsonObject &topjson)
{
    QJsonObject concentrationObject, signalObject;
    concentrationObject = topjson["data"].toObject()["concentrations"].toObject();
    signalObject = topjson["data"].toObject()["signals"].toObject();
    
    if(concentrationObject.isEmpty() || signalObject.isEmpty())
        return false;
    
    QStringList keys = signalObject.keys();
    
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(
        keys.begin(),
              keys.end(),
              [&collator](const QString &key1, const QString &key2)
              {
                  return collator.compare(key1, key2) < 0;
              });
    if(DataPoints() == 0)
    {
        for(const QString &str: qAsConst(keys))
        {
            QVector<qreal > concentrationsVector, signalVector;
            concentrationsVector = ToolSet::String2DoubleVec(concentrationObject[str].toString());
            signalVector = ToolSet::String2DoubleVec(signalObject[str].toString());
            d->m_concentration_model->insertRow(concentrationsVector);
            d->m_signal_model->insertRow(signalVector);
        }
            QStringList header = topjson["data"].toObject()["header"].toString().split("|");
            setHeader(header);
        return true;
    }
    else if(keys.size() != DataPoints())
    {
        qWarning() << "table size doesn't fit to imported data";
        return false;
    }
    for(const QString &str: qAsConst(keys))
    {
        QVector<qreal > concentrationsVector, signalVector;
        concentrationsVector = ToolSet::String2DoubleVec(concentrationObject[str].toString());
        signalVector = ToolSet::String2DoubleVec(signalObject[str].toString());
        int row = str.toInt();
        d->m_concentration_model->setRow(concentrationsVector, row);
        d->m_signal_model->setRow(signalVector, row);
    }
    
    if("discrete" == topjson["data"].toObject()["datatype"].toString())
        d->m_type = 1;
    QStringList header = topjson["data"].toObject()["header"].toString().split("|");
    setHeader(header);
    return true;
}

void DataClass::setHeader(const QStringList& strlist)
{
    if(strlist.size() == (d->m_concentration_model->columnCount() + d->m_signal_model->columnCount()))
    {
        for(int i = 0; i < strlist.size(); ++i)
        {
            if(i < d->m_concentration_model->columnCount())
                d->m_concentration_model->setHeaderData(i, Qt::Horizontal, (strlist[i]), Qt::DisplayRole);
            else
                d->m_signal_model->setHeaderData(i - d->m_concentration_model->columnCount(), Qt::Horizontal, (strlist[i]), Qt::DisplayRole);
        }
    }
}

void DataClass::OverrideSignalTable(DataTable *table)
{
    d.detach();
    d->m_signal_model = table;
}

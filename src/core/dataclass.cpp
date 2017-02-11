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

#include <iostream>

DataTable::DataTable(QObject* parent) : QAbstractTableModel(parent)
{
}

DataTable::DataTable(int columns, int rows, QObject* parent) : QAbstractTableModel(parent)
{
    m_table = Eigen::MatrixXd::Zero(rows, columns);
}

DataTable::DataTable(DataTable& other) : QAbstractTableModel(&other) //FIXME whatever
{
    m_table = other.m_table;
}

DataTable::DataTable(DataTable* other)//: QAbstractTableModel(other) FIXME whatever
{
    m_table = other->m_table;
}


DataTable::~DataTable()
{
    
    
    
}

// QVector<qreal> DataTable::firstRow()
// {
//     QVector<qreal> vector;
//     for(int i = 0; i < m_table.cols(); ++i)
//         vector << m_table(0, i);
//     return vector;
// }

QList<qreal> DataTable::firstRow()
{
    QList<qreal> vector;
    for(int i = 0; i < m_table.cols(); ++i)
        vector << m_table(0, i);
    return vector;
}

// QVector<qreal> DataTable::lastRow()
// {
//     QVector<qreal> vector;
//     for(int i = 0; i < m_table.cols(); ++i)
//         vector << m_table(m_table.rows()-1, i);
//     return vector;
// }

QList<qreal> DataTable::lastRow()
{
    QList<qreal> vector;
    for(int i = 0; i < m_table.cols(); ++i)
        vector << m_table(m_table.rows()-1, i);
    return vector;
}

void DataTable::Debug() const
{
    std::cout << "Rows: " << m_table.rows() << " Cols: " <<m_table.cols() << std::endl;
    std::cout << m_table <<std::endl;
}
Qt::ItemFlags DataTable::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
}
int DataTable::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    if(m_table.size() != 0)
        return m_table.cols();
    return 0;
    
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
    else
        return QVariant();
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

// QVector<qreal> DataTable::Row(int row)
// {
//     QVector<qreal> vector;
//     if(row <= m_table.rows())
//         for(int i = 0; i < m_table.cols(); ++i)
//             vector << m_table(row, i);
//     else
//         qDebug() << "Row exceeds size of table!";
//     return vector;
//     
// }

QList<qreal> DataTable::Row(int row)
{
    QList<qreal> vector;
    if(row <= m_table.rows())
        for(int i = 0; i < m_table.cols(); ++i)
            vector << m_table(row, i);
    else
        qDebug() << "Row exceeds size of table!";
    return vector;
    
}

// QVector<qreal> DataTable::Column(int column)
// {
//     QVector<qreal> result;
//     for(int i = 0; i < m_table.size(); ++i)
//     {
//         if(column < m_table[i].size())
//             result << m_table[i][column];
//         else
//             qDebug() << "Column exceeds size of table!";
//     }
//     return result;
// }

// void DataTable::insertColumn(QVector<qreal> column)
// {
//     if(m_table.size() != 0)
//     {
//         if((m_table.first().size() == column.size()))
//             m_table << column.toList();
//     }else
//         m_table << column.toList();
// }

void DataTable::insertRow(QVector<qreal> row)
{
    if(m_table.cols() == 0)
        m_table.conservativeResize(m_table.rows() + 1, row.size());
    else
        m_table.conservativeResize(m_table.rows() + 1, m_table.cols());
    
    for(int i = 0; i < row.size(); ++i)
        m_table(m_table.rows() -1, i) = row[i];
}

void DataTable::setColumn(QVector<qreal> vector, int column)
{
    Q_UNUSED(vector);
    Q_UNUSED(column);
    return;
}

void DataTable::setRow(QVector<qreal> vector, int row)
{
    if(m_table.rows() >= row)
        for(int i = 0; i < vector.size(); ++i)
            m_table(row, i) = vector[i];   
    //     Q_UNUSED(vector);
    //     Q_UNUSED(row);
    return;
}

DataClassPrivate::DataClassPrivate() : m_maxsize(0), m_concentrations(new bool(true))
{
    m_concentration_model = new DataTable;
    m_signal_model = new DataTable;
    m_raw_data = new DataTable;
}

DataClassPrivate::DataClassPrivate(int type) : m_type(type) , m_maxsize(0), m_concentrations(new bool(true))
{
    m_concentration_model = new DataTable;
    m_signal_model = new DataTable;
    m_raw_data = new DataTable;
    if(m_type == 3)
    {
        for(int i = 0; i <= 100; ++i)
        {
            QVector<qreal > vec = QVector<qreal>() << (i)*6/100 << 3;
            m_concentration_model->insertRow(vec);
        }
    }
    
}

DataClassPrivate::DataClassPrivate(const DataClassPrivate& other) : QSharedData(other)
{
    m_concentration_model = new DataTable(other.m_concentration_model);
    m_signal_model = new DataTable(other.m_signal_model);
    m_raw_data = new DataTable(other.m_raw_data);

    m_type = other.m_type;
    
}

DataClassPrivate::DataClassPrivate(const DataClassPrivate* other) 
{
    m_concentration_model = new DataTable(other->m_concentration_model);
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

DataClass::DataClass(QObject *parent) : QObject(parent), m_plotmode(DataClass::HG)
{
    d = new DataClassPrivate;
}

DataClass::DataClass(const QJsonObject &json, int type, QObject *parent):  QObject(parent), m_plotmode(DataClass::HG)
{
    d = new DataClassPrivate(type);
    ImportJSON(json);
    
}

DataClass::DataClass(int type, QObject *parent) :  QObject(parent), m_plotmode(DataClass::HG)
{
    d = new DataClassPrivate(type);
}

DataClass::DataClass(const DataClass& other): QObject()
{
    m_plotmode = other.m_plotmode;
    d = other.d;
}

DataClass::DataClass(const DataClass* other)
{
    m_plotmode = other->m_plotmode;
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
    *d->m_concentrations = !(*d->m_concentrations); 
}

qreal DataClass::XValue(int i) const
{
    
    switch(m_plotmode){
        case DataClass::G:
            if(*d->m_concentrations)
                return d->m_concentration_model->data(0,i);
            else
                return d->m_concentration_model->data(1,i);
            break;
            
        case DataClass::H:   
            if(!(*d->m_concentrations))
                return d->m_concentration_model->data(1,i);
            else
                return d->m_concentration_model->data(0,i);
            break;
            
        case DataClass::HG:
            if(*d->m_concentrations)
                return d->m_concentration_model->data(1,i)/d->m_concentration_model->data(0,i);
            else
                return d->m_concentration_model->data(0,i)/d->m_concentration_model->data(1,i);                
            break;    
            
        case DataClass::GH:
        default:
            if(!(*d->m_concentrations))
                return d->m_concentration_model->data(0,i)/d->m_concentration_model->data(1,i);
            else
                return d->m_concentration_model->data(1,i)/d->m_concentration_model->data(0,i);                   
            break;    
    };
    return 0;
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
    
    return true;
}

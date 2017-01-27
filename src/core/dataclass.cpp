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

#include <QtCore/QCoreApplication>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QReadWriteLock>
#include <QtCore/QCollator>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>


#include <QStandardItemModel>
#include <QAbstractTableModel>
#include <QPointer>
#include <QDebug>
#include <QColor>
#include "dataclass.h"
#include <QtGlobal>

#include <iostream>

DataTable::DataTable(QObject* parent) : QAbstractTableModel(parent)
{
}

DataTable::DataTable(int columns, int rows, QObject* parent) : QAbstractTableModel(parent)
{
    QVector<qreal > vector(columns,0);
    for(int i = 0; i < rows; ++i)
        insertRow(vector);
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

void DataTable::Debug() const
{
    qDebug() << m_table;
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
        return m_table.first().size();
    return 0;
    
}

int DataTable::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return m_table.size();
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

    if(row < m_table.size())
        if(column < m_table[row].size())
        {
            return m_table[row][column];
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
    if(row < m_table.size())
        if(column < m_table[row].size())
        {
            return m_table[row][column];
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

QVector<qreal> DataTable::Row(int row)
{
    QVector<qreal> result;
    if(row < m_table.size())
        result = m_table[row];
    else
        qDebug() << "Row exceeds size of table!";
    return result;
    
}

QVector<qreal> DataTable::Column(int column)
{
    QVector<qreal> result;
    for(int i = 0; i < m_table.size(); ++i)
    {
        if(column < m_table[i].size())
            result << m_table[i][column];
        else
            qDebug() << "Column exceeds size of table!";
    }
    return result;
}

void DataTable::insertColumn(QVector<qreal> column)
{
    if(m_table.size() != 0)
    {
        if((m_table.first().size() == column.size()))
            m_table << column;
    }else
        m_table << column;
}

void DataTable::insertRow(QVector<qreal> row)
{
    
    if(m_table.isEmpty())
        m_table << row;
    else
        if(row.size() == m_table.first().size())
            m_table << row;
        else
            qDebug() << "Wrong number of rows!";
}

void DataTable::setColumn(QVector<qreal> vector, int column)
{
    Q_UNUSED(vector);
    Q_UNUSED(column);
    return;
}

void DataTable::setRow(QVector<qreal> vector, int row)
{
    m_table[row] = vector;
    //     Q_UNUSED(vector);
    //     Q_UNUSED(row);
    return;
}

DataClassPrivate::DataClassPrivate() : m_maxsize(0), m_concentrations(new bool(true))
{
    m_concentration_model = new DataTable;
    m_signal_model = new DataTable;
    m_raw_data = new DataTable;
    
    m_plot_signal = new QStandardItemModel(m_signal_model->rowCount(), m_signal_model->columnCount()+1);
    
}

DataClassPrivate::DataClassPrivate(int type) : m_type(type) , m_maxsize(0), m_concentrations(new bool(true))
{
    m_concentration_model = new DataTable;
    m_signal_model = new DataTable;
    m_raw_data = new DataTable;
    m_plot_signal = new QStandardItemModel(m_signal_model->rowCount(), m_signal_model->columnCount()+1);
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
    m_plot_signal = new QStandardItemModel(m_signal_model->rowCount(), m_signal_model->columnCount()+1);
    
    for(int i = 0; i < other.m_plot_signal_mapper.size(); ++i)
        m_plot_signal_mapper << other.m_plot_signal_mapper[i];
    
    m_type = other.m_type;
    
}

DataClassPrivate::DataClassPrivate(const DataClassPrivate* other) 
{
    m_concentration_model = new DataTable(other->m_concentration_model);
    m_signal_model = new DataTable(other->m_signal_model);
    m_raw_data = new DataTable(other->m_raw_data);
    m_plot_signal = new QStandardItemModel(m_signal_model->rowCount(), m_signal_model->columnCount()+1);
    
    for(int i = 0; i < other->m_plot_signal_mapper.size(); ++i)
        m_plot_signal_mapper << other->m_plot_signal_mapper[i];
    
    m_type = other->m_type;
}


DataClassPrivate::~DataClassPrivate()
{
    delete m_concentration_model;
    delete m_signal_model;
    delete m_raw_data;
    qDeleteAll( m_plot_signal_mapper );
}

DataClass::DataClass(QObject *parent) : QObject(parent), m_plotmode(DataClass::HG)
{
    d = new DataClassPrivate;
    CreateClearPlotModel();
}

DataClass::DataClass(const QJsonObject &json, int type, QObject *parent):  QObject(parent), m_plotmode(DataClass::HG)
{
    d = new DataClassPrivate(type);
    ImportJSON(json);
    CreateClearPlotModel();
    
}

DataClass::DataClass(int type, QObject *parent) :  QObject(parent), m_plotmode(DataClass::HG)
{
    d = new DataClassPrivate(type);
}

DataClass::DataClass(const DataClass& other): QObject()
{
    m_plotmode = other.m_plotmode;
    d = other.d;
    CreateClearPlotModel();
    PlotModel();
}

DataClass::DataClass(const DataClass* other)
{
    m_plotmode = other->m_plotmode;
    d = other->d;
    CreateClearPlotModel();
    PlotModel();
}

DataClass::~DataClass()
{

}

void DataClass::CreateClearPlotModel()
{
    for(int i = 0; i < DataPoints(); ++i)
    {
        QString x = QString::number(XValue(i));
        
        QStandardItem *item;
        item = new QStandardItem(x);
        d->m_plot_signal->setItem(i,0, item);
        for(int j = 0; j < SignalCount(); ++j)
        {
            item = new QStandardItem(QString::number(d->m_signal_model->data(j,i)));
            d->m_plot_signal->setItem(i,j+1, item);
        }
    }
}


QColor DataClass::ColorCode(int i) const
{
    switch(i){
        case 0:
            return Qt::red;
        case 1:
            return Qt::blue;
        case 2:
            return Qt::green;
        case 3:
            return Qt::yellow;
        case 4:
            return Qt::darkRed;
        case 5:
            return Qt::darkBlue;
        case 6:
            return Qt::darkGreen;
        case 7:
            return Qt::magenta;
        case 8:
            return Qt::cyan;
        case 9:
            return Qt::darkYellow;
        case 10:
            return Qt::darkMagenta;
        case 11:
            return Qt::darkCyan;
        case 12:
            return Qt::gray;
        default:
            return Qt::darkGray;
    }
}

QVector<double>   DataClass::getSignals(QVector<int > active_signal)
{
    if(active_signal.size() < SignalCount() )
        active_signal = QVector<int>(SignalCount(), 1);
    
    QVector<double> x(DataPoints()*SignalCount(), 0);
    int index = 0;
    for(int j = 0; j < SignalCount(); ++j)
    {
        for(int i = 0; i < DataPoints(); ++i)
        {
            if(active_signal[j] == 1)
                x[index] = SignalModel()->data(j,i); 
            index++;
        }
    }
    return x;
}

void DataClass::SwitchConentrations()
{
    *d->m_concentrations = !(*d->m_concentrations); 
    PlotModel();
    emit recalculate();
}

void DataClass::PlotModel()
{
    if(d->m_plot_signal_mapper.isEmpty())
    {
        for(int j = 0; j < SignalCount(); ++j)
        {
            QPointer<QtCharts::QVXYModelMapper> model = new QtCharts::QVXYModelMapper;
            model->setModel(d->m_plot_signal);
            model->setXColumn(0);
            model->setYColumn(j + 1);
            d->m_plot_signal_mapper << model;   
        }
        
    }
    
    for(int i = 0; i < DataPoints(); ++i)
    {
        QString x = QString::number(XValue(i));
       
        d->m_plot_signal->item(i,0)->setData(x, Qt::DisplayRole);
        
        for(int j = 0; j < SignalCount(); ++j)
        { 
            d->m_plot_signal->item(i,j+1)->setData(QString::number(d->m_signal_model->data(j,i)), Qt::DisplayRole);
        }
    }
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

QColor DataClass::color(int i) const
{
    if(d->m_plot_signal_mapper.size() <= i)
        return ColorCode(i);
    else
    {
        QPointer<QtCharts::QVXYModelMapper> mapper = d->m_plot_signal_mapper[i];
        if(!mapper)
            return ColorCode(i);
        return d->m_plot_signal_mapper[i]->series()->color();
    }
}

const QJsonObject DataClass::ExportJSON() const
{
    QJsonObject json;
    
    QJsonObject concentrationObject, signalObject;
    
    for(int i = 0; i < DataPoints(); ++i)
    {
        concentrationObject[QString::number(i)] = ToolSet::DoubleVec2String(d->m_concentration_model->Row(i));
        signalObject[QString::number(i)] = ToolSet::DoubleVec2String(d->m_signal_model->Row(i));
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
        foreach(const QString &str, keys)
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
    foreach(const QString &str, keys)
    {
        QVector<qreal > concentrationsVector, signalVector;
        concentrationsVector = ToolSet::String2DoubleVec(concentrationObject[str].toString());
        signalVector = ToolSet::String2DoubleVec(signalObject[str].toString());
        //         qDebug() << str << concentrationsVector << signalVector;
        int row = str.toInt();
        d->m_concentration_model->setRow(concentrationsVector, row);
        d->m_signal_model->setRow(signalVector, row);
    }
    
    return true;
}

void DataClass::MakeThreadSafe()
{
    DataClassPrivate *d_2 = new DataClassPrivate(d);
    d = d_2;
}

DataTable * DataClass::SignalModel()
{  
    QMutexLocker locker(&mutex);
    return d->m_signal_model; 
}

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
#include <QStandardItemModel>
#include <QAbstractTableModel>
#include <QPointer>
#include <QDebug>
#include <QColor>
#include "dataclass.h"
#include <QtGlobal>


DataTable::DataTable(QObject* parent) : QAbstractTableModel(parent)
{
}

DataTable::DataTable(int columns, int rows, QObject* parent) : QAbstractTableModel(parent)
{
    QVector<qreal > vector(columns,0);
    for(int i = 0; i < rows; ++i)
        insertRow(vector);
}

DataTable::DataTable(DataTable& other) //: QAbstractTableModel(&other) FIXME whatever
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
    Q_UNUSED(role)
    return data(index.column(), index.row());
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
    Q_UNUSED(vector);
    Q_UNUSED(row);
    return;
}

DataClass::DataClass(QObject *parent) : QObject(parent), m_maxsize(0), m_plotmode(DataClass::HG)
{
    m_concentration_model = new DataTable(this);
    m_signal_model = new DataTable(this);
    m_raw_data = new DataTable(this);
  
    m_plot_signal = new QStandardItemModel(DataPoints(), SignalCount()+1);
}

DataClass::DataClass(int type, QObject *parent) :  QObject(parent), m_type(type) , m_maxsize(0), m_concentrations(new bool(true)), m_plotmode(DataClass::GH)
{
    m_concentration_model = new DataTable(this);
    m_signal_model = new DataTable(this);
    m_raw_data = new DataTable(this);
    
     if(m_type == 3)
     {
        for(int i = 0; i <= 100; ++i)
        {
            QVector<qreal > vec = QVector<qreal>() << (i)*6/100 << 3;
              m_concentration_model->insertRow(vec);
        }
     }

     m_plot_signal = new QStandardItemModel(DataPoints(), SignalCount()+1);

}

DataClass::DataClass(const DataClass& other): m_maxsize(0), m_concentrations(new bool(true))
{
    m_concentration_model = new DataTable(other.m_concentration_model);
    m_signal_model = new DataTable(other.m_signal_model);
    m_raw_data = new DataTable(other.m_raw_data);
    m_plot_signal = new QStandardItemModel(DataPoints(), SignalCount()+1);
    
    m_type = other.Type();
    m_plotmode = (other.m_plotmode);
}

DataClass::DataClass(const DataClass* other): m_maxsize(0), m_concentrations(new bool(true))
{
    m_concentration_model = new DataTable(other->m_concentration_model);
    m_signal_model = new DataTable(other->m_signal_model);
    m_raw_data = new DataTable(other->m_raw_data);
    m_plot_signal = new QStandardItemModel(DataPoints(), SignalCount()+1);
    
    m_type = other->Type();
    m_plotmode = (other->m_plotmode);
}

DataClass::~DataClass()
{
        qDeleteAll( m_plot_signal_mapper );
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
    if(active_signal.size() < SignalCount() && m_active_signals.size() < SignalCount())
        active_signal = QVector<int>(SignalCount(), 1);
    else
        active_signal = m_active_signals;
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
     *m_concentrations = !(*m_concentrations); 
     PlotModel();
     emit recalculate();
}

void DataClass::PlotModel()
{
    if(m_plot_signal_mapper.isEmpty())
    {
        for(int j = 0; j < SignalCount(); ++j)
        {
            QPointer<QtCharts::QVXYModelMapper> model = new QtCharts::QVXYModelMapper;
            model->setModel(m_plot_signal);
            model->setXColumn(0);
            model->setYColumn(j + 1);
            m_plot_signal_mapper << model;   
            qDebug() << j << m_colors.size();
            if(j <= m_colors.size())
                m_colors << ColorCode(j);
        }

    }
     
     for(int i = 0; i < DataPoints(); ++i)
     {
         QString x = QString::number(XValue(i));
                       
         QStandardItem *item;
            item = new QStandardItem(x);
            m_plot_signal->setItem(i,0, item);
            for(int j = 0; j < SignalCount(); ++j)
            {
                item = new QStandardItem(QString::number(m_signal_model->data(j,i)));
                m_plot_signal->setItem(i,j+1, item);
            }
     }
}


qreal DataClass::XValue(int i) const
{

    switch(m_plotmode){
            case DataClass::G:
                if(*m_concentrations)
                    return m_concentration_model->data(0,i);
                else
                    return m_concentration_model->data(1,i);
            break;
            
            case DataClass::H:   
                if(!(*m_concentrations))
                    return m_concentration_model->data(1,i);
                else
                    return m_concentration_model->data(0,i);
            break;
                
            case DataClass::HG:
                if(*m_concentrations)
                    return m_concentration_model->data(1,i)/m_concentration_model->data(0,i);
                else
                    return m_concentration_model->data(0,i)/m_concentration_model->data(1,i);                
            break;    
            
            case DataClass::GH:
            default:
                if(!(*m_concentrations))
                    return m_concentration_model->data(0,i)/m_concentration_model->data(1,i);
                else
                    return m_concentration_model->data(1,i)/m_concentration_model->data(0,i);                   
            break;    
        };
        return 0;
}


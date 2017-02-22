/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/AbstractModel.h"
#include "src/core/dataclass.h"

#include <QtCharts/QAbstractSeries>
#include <QStandardItemModel>

#include "chartwrapper.h"

// LineSeries::LineSeries(LineSeries *other)
// {
//     setColor(other->pen().color());
//     QVector<QPointF> points = other->pointsVector();
//     for(int i = 0; i < points.size(); ++i)
//         append(points[i]);
// }
// 
// 
// ScatterSeries::ScatterSeries(ScatterSeries *other)
// {
//     setColor(other->pen().color());
//     QVector<QPointF> points = other->pointsVector();
//     for(int i = 0; i < points.size(); ++i)
//         append(points[i]);
// }

 void LineSeries::setColor(const QColor &color) 
 { 
     QPen pen = QtCharts::QLineSeries::pen();
//      pen.setStyle(Qt::DashDotLine);
     pen.setWidth(2);
     pen.setColor(color);
     setPen(pen);     
}
void LineSeries::ShowLine(bool state)
{
    setVisible(state);
}

void LineSeries::ShowLine(int state)
{
    if(state == Qt::Unchecked)
        setVisible(false);
    else if(state == Qt::Checked)
        setVisible(true);   
}

void LineSeries::setName(const QString &str)
{
    QtCharts::QLineSeries::setName(str);   
}

 void ScatterSeries::setColor(const QColor &color) 
 { 
     QPen pen = QtCharts::QScatterSeries::pen();
//      pen.setStyle(Qt::DashDotLine);
     pen.setWidth(2);
     pen.setColor(color);
     setPen(pen);     
}

void ScatterSeries::ShowLine(int state)
{
    if(state == Qt::Unchecked)
        setVisible(false);
    else if(state == Qt::Checked)
        setVisible(true);   
    emit visibleChanged(state);
}


ChartWrapper::ChartWrapper(QObject* parent) : QObject(parent)
{
}

ChartWrapper::~ChartWrapper()
{
     for(int i = 0; i < m_plot_mapper.size(); ++i)
     {
          delete m_plot_mapper[i]->series();
     }
}


void ChartWrapper::setData(QPointer<DataClass> model)
{
    m_model = model; 
    
    CreateModel(); 
    if(qobject_cast<AbstractTitrationModel *>(m_model))
        connect(m_model, SIGNAL(Recalculated()), this, SLOT(UpdateModel()));
    
    if(m_plot_mapper.isEmpty())
    {
        for(int j = 0; j < m_model->SignalCount(); ++j)
        {
            QPointer<QtCharts::QVXYModelMapper> model = new QtCharts::QVXYModelMapper;
            model->setModel(m_plot_signal);
            model->setXColumn(0);
            model->setYColumn(j + 1);
            m_plot_mapper<< model;  
            QPointer<QtCharts::QXYSeries > series;
            if(qobject_cast<AbstractTitrationModel *>(m_model))
                series = new LineSeries;
            else
                series = new ScatterSeries;
            model->setSeries(series);
            m_stored_series << series;
        }
    }
}

void ChartWrapper::CreateModel()
{
    m_plot_signal = new QStandardItemModel(m_table->rowCount(), m_table->columnCount()+1);
    for(int i = 0; i < m_model->DataPoints(); ++i)
    {
        QString x = QString::number(XValue(i));
        
        QStandardItem *item;
        item = new QStandardItem(x);
        m_plot_signal->setItem(i,0, item);
        for(int j = 0; j < m_model->SignalCount(); ++j)
        {
            item = new QStandardItem(QString::number(m_table->data(j,i)));
            m_plot_signal->setItem(i,j+1, item);
        }
    }
}

void ChartWrapper::UpdateModel()
{
    for(int i = 0; i < m_model->DataPoints(); ++i)
    {
        QString x = QString::number(XValue(i));
       
        m_plot_signal->item(i,0)->setData(x, Qt::DisplayRole);
        
        for(int j = 0; j < m_model->SignalCount(); ++j)
        { 
            m_plot_signal->item(i,j+1)->setData(QString::number(m_table->data(j,i)), Qt::DisplayRole);
        }
    }
    emit ModelChanged();
}

QColor ChartWrapper::color(int i) const
{
    if(m_plot_mapper.size() <= i)
        return ColorCode(i);
    else
    {
        QPointer<QtCharts::QVXYModelMapper> mapper = m_plot_mapper[i];
        if(!mapper)
            return ColorCode(i);
        return m_plot_mapper[i]->series()->color();
    }
}

QColor ChartWrapper::ColorCode(int i) const
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

qreal ChartWrapper::XValue(int i) const
{
    
    switch(m_plotmode){
        case PlotMode::G:
                return m_model->InitialGuestConcentration(i); 
            break;
            
        case PlotMode::H:   
                return m_model->InitialHostConcentration(i);
            break;
            
        case PlotMode::HG:
                return m_model->InitialHostConcentration(i)/m_model->InitialGuestConcentration(i);                
            break;    
            
        case PlotMode::GH:
        default:
                return m_model->InitialGuestConcentration(i)/m_model->InitialHostConcentration(i);                   
            break;    
    };
    return 0;
}


#include "chartwrapper.moc"

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
//      pen.setColor(color);
     QScatterSeries::setColor(color);
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


ChartWrapper::ChartWrapper(QObject* parent) : QObject(parent), m_blocked(false), m_transformed(false), m_transpose(false)
{
}

ChartWrapper::~ChartWrapper()
{
     for(int i = 0; i < m_stored_series.size(); ++i)
     {
          m_stored_series[i]->clear();
          delete m_stored_series[i];
     }
}


void ChartWrapper::TransposeModels()
{
    m_transpose = !m_transpose;
}


void ChartWrapper::setData(QSharedPointer<DataClass> model)
{
    m_model = model; 
    
    if(qobject_cast<AbstractModel *>(m_model))
        connect(m_model.data(), SIGNAL(Recalculated()), this, SLOT(UpdateModel()));
    
    if(m_stored_series.isEmpty())
    {
        for(int j = 0; j < m_model->SeriesCount(); ++j)
        { 
            QPointer<QtCharts::QXYSeries > series;
            if(qobject_cast<AbstractModel *>(m_model))
                series = new LineSeries;
            else
                series = new ScatterSeries;
            m_stored_series << series;
        }
    }
    UpdateModel();
}

void ChartWrapper::UpdateModel()
{
    for(int j = 0; j < m_model->SeriesCount(); ++j)
        m_stored_series[j]->clear();
    for(int i = 0; i < m_model->DataPoints(); ++i)
    {
        double x = m_model->PrintOutIndependent(i);
        for(int j = 0; j < m_model->SeriesCount(); ++j)
        {
            if(m_model->DependentModel()->isChecked(j,i))
                m_stored_series[j]->append(x, m_table->data(j,i));
        }
    }
    emit ModelChanged();
}

QColor ChartWrapper::color(int i) const
{
    if(m_stored_series.size() <= i)
        return ColorCode(i);
    else
    {
        QPointer<QtCharts::QXYSeries> series = m_stored_series[i];
        if(!series)
            return ColorCode(i);
        return m_stored_series[i]->color();
    }
}

QString ChartWrapper::ColorList() const
{
    QString list;
    for(int i = 0; i < m_stored_series.size(); ++i)
        list += color(i).name() + "|";
    list.chop(1);
    return list;
}

bool ChartWrapper::setColorList(const QString &str)
{
    QStringList colors = str.split("|");
    if(colors.size() != m_stored_series.size())
        return false;
    for(int i = 0; i < m_stored_series.size(); ++i)
        m_stored_series[i]->setColor(QColor(colors[i]));

    return true;
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

void ChartWrapper::SetBlocked(int blocked) 
{        
    m_blocked = !blocked; 
}

void ChartWrapper::showSeries(int i)
{
    for(int j = 0; j < m_stored_series.size(); ++j)
    {
        if(i == -1 && !m_blocked)
        {
            m_stored_series[j]->setVisible(true);
            continue;
        }
        if(qobject_cast<ScatterSeries *>(m_stored_series[j]))
            m_stored_series[j]->setVisible(i == j);
        else if(i != -1 && m_stored_series[j]->isVisible())
            m_stored_series[j]->setVisible(i == j); 
    }
    emit ShowSeries(i);
}
#include "chartwrapper.moc"

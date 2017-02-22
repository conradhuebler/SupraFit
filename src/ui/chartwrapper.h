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

#ifndef CHARTWRAPPER_H
#define CHARTWRAPPER_H

#include <QtCore/QPointer>
#include <QtCore/QList>
#include <QtCore/QObject>

#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QXYSeries>
#include <QtCharts/QVXYModelMapper>

class DataClass;
class DataTable;
class QStandardItemModel;
class AbstractTitrationModel;


class LineSeries : public QtCharts::QLineSeries
{
  Q_OBJECT
  
public:
    inline LineSeries() {}
    inline ~LineSeries() {}
    
public slots:
    virtual void setColor(const QColor &color); 
    void ShowLine(int state);
    void ShowLine(bool state);
    virtual void setName(const QString &name);
};

class ScatterSeries : public QtCharts::QScatterSeries
{
  Q_OBJECT
  
public:
    inline ScatterSeries() {}
    inline ~ScatterSeries() {}
    
public slots:
    virtual void setColor(const QColor &color); 
    void ShowLine(int state);
    
signals:
    void NameChanged(const QString &str);
    void visibleChanged(int state);
};


class ChartWrapper : public QObject
{
    Q_OBJECT
public:
    enum PlotMode { 
        H = 1, 
        G = 2, 
        HG = 3, 
        GH = 4
    };
    
    ChartWrapper(QObject *parent = 0);
    ~ChartWrapper();
    void setData(QPointer< DataClass > model);
    inline void setDataTable(const DataTable *table) { m_table = table; }
    inline QPointer<QtCharts::QXYSeries > Series(int i) { return m_stored_series[i]; }
    inline void setSeries(QPointer<QtCharts::QXYSeries> series, int i) { m_stored_series[i] = series; }
    QColor color(int i) const; 
    inline void setPlotMode(PlotMode plotmode) { m_plotmode = plotmode; }
    
public slots:
    void UpdateModel();
    void showSeries(int i);
    
private:
    QColor ColorCode(int i) const;
    qreal XValue(int i) const;
    const DataTable *m_table;
    QList<QStandardItemModel *> m_plot_signal_list;
    QList<QPointer<QtCharts::QVXYModelMapper> > m_plot_mapper;
    QList<QPointer<QtCharts::QXYSeries > > m_stored_series;
    PlotMode m_plotmode;
    QPointer< DataClass > m_model;
    
signals:
    void ModelChanged();
    void stopAnimiation();
    void restartAnimation();
    void ShowSeries(int i);
};

#endif // CHARTWRAPPER_H

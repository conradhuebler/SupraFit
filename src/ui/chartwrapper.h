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

#include <QtCharts/QScatterSeries>
#include <QtCharts/QVXYModelMapper>
class DataClass;
class DataTable;
class QStandardItemModel;
class AbstractTitrationModel;

class ChartWrapper : public QObject
{
    Q_OBJECT
public:
    ChartWrapper(QObject *parent = 0);
    ~ChartWrapper();
    void setData(QPointer< DataClass > model);
    inline void setDataTable(const DataTable *table) { m_table = table; }
    inline QPointer<QtCharts::QVXYModelMapper> DataMapper(int i) { return m_plot_mapper[i]; }
    QColor color(int i) const; 
public slots:
    void UpdateModel();
private:
    QColor ColorCode(int i) const;
    void CreateModel();
    const DataTable *m_table;
    QStandardItemModel *m_plot_signal;
    QList<QPointer<QtCharts::QVXYModelMapper> > m_plot_mapper;
    
    QPointer< DataClass > m_model;
};

#endif // CHARTWRAPPER_H

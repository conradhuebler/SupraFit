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

#ifndef MODELDATAHOLDER_H
#define MODELDATAHOLDER_H
#include "core/data/dataclass.h"
#include <QtCharts/QChart>
#include <QtWidgets/QWidget>
#include <QtCore/QPointer>

class ModelWidget;
class DataWidget;
class QTabWidget;
class QPushButton;

class ModelDataHolder : public QWidget
{
    Q_OBJECT

public:
    ModelDataHolder();
    ~ModelDataHolder();
    void setData(DataClass data);
    DataClass *DataPtr() const { return m_data; }
private:
    QPointer<DataWidget > m_datawidget;
    QPointer<QTabWidget > m_models;
    QPointer<QPushButton > m_add;
    DataClass *m_data;
    
private slots:
    void AddModel();
signals:
    void PlotChart(const QVector< QPointer< QtCharts::QLineSeries > > chart);
};

#endif // MODELDATAHOLDER_H

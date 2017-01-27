/*
 * <one line to give the library's name and an idea of what it does.>
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

#ifndef _3DCHARTVIEW_H
#define _3DCHARTVIEW_H

#include <QtCore/QPointer>

#include <QtDataVisualization/Q3DSurface>

#include <QtWidgets/QWidget>

class QPushButton;

class _3DChartViewPrivate : public QtDataVisualization::Q3DSurface
{
    Q_OBJECT
public:
    _3DChartViewPrivate();
    ~_3DChartViewPrivate();
private:
};

class _3DChartView : public QWidget
{
    Q_OBJECT
public:
    _3DChartView(QWidget *parent = 0);
    ~_3DChartView();
    void setData(const QtDataVisualization::QSurfaceDataArray &data);
private:
    QPointer<_3DChartViewPrivate> d;
    QPushButton *m_config;
    QtDataVisualization::QSurfaceDataArray *m_data;
    QtDataVisualization::QSurface3DSeries *m_series;
    void CreateChart();
};

#endif // 3DCHARTVIEW_H

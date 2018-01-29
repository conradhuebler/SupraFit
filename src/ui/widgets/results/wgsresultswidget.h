/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#pragma once

#include "src/ui/widgets/results/resultswidget.h"

#include <QtWidgets/QWidget>

class AbstractSearchClass;
class AbstractModel;
class QJsonObject;

class WGSResultsWidget : public QWidget
{
    Q_OBJECT
    
public:
    WGSResultsWidget(const QJsonObject &data, QSharedPointer<AbstractModel> model, QWidget *parent);
    ~WGSResultsWidget();
    inline bool hasData() const { return has_data; }
    
private:
//     void WriteConfidence(const QJsonObject &result);
    virtual QWidget * ChartWidget();
    
            
    QJsonObject m_data;
    
    ChartWrapper *m_wrapper;
    QList<QJsonObject > m_models ;
        QSharedPointer<AbstractModel > m_model;

    ChartView * MoCoPlot();
    ChartView * WGPlot();
    bool  has_data;
};

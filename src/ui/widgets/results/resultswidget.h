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

#ifndef RESULTSWIDGET_H
#define RESULTSWIDGET_H

#include <QtWidgets/QWidget>

class AbstractSearchClass;
class AbstractModel;

class ResultsWidget : public QWidget
{
    Q_OBJECT
public:
    ResultsWidget();
    ~ResultsWidget();
    
   
protected:
    void setUi();
    virtual QWidget * ChartWidget() = 0;
    inline QSize ChartSize() const { return QSize(400,300); }
    virtual void WriteConfidence(const QList<QJsonObject > &constant_results) { };
    
    QWidget *m_chart_widget;
    QLabel *m_confidence_label;
    QSharedPointer< AbstractModel > m_model;
    
};    

#endif // RESULTSWIDGET_H

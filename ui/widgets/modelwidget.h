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

#ifndef MODELWIDGET_H
#define MODELWIDGET_H

#include <QtGui/qwidget.h>
#include <QDoubleSpinBox>

#include "core/data/dataclass.h"
#include "core/data/modelclass.h"

class QDoubleSpinBox;
class QPushButton;
class QLineEdit;


class ModelWidget : public QWidget
{
    Q_OBJECT

public:
ModelWidget(QPointer< AbstractTitrationModel > model, QWidget *parent = 0);
~ModelWidget();

private:
    QPointer< AbstractTitrationModel > m_model;
    QVector<QPointer<QDoubleSpinBox >  >m_pure_signals;
    QVector< QVector<QPointer<QDoubleSpinBox > > > m_complex_signals;
    QVector<QPointer<QDoubleSpinBox> > m_constants;
    QVector<QPointer<QLineEdit > > m_errors;
    QLineEdit *m_sum_error;
//     QVector<QPointer< ModelLine > > m_model_lines;
    QPushButton *m_switch, *m_minimize; 
    bool m_pending;
//     void Connect();
//     void Disconnect();
private slots:
    void Minimize();
    void Repaint();
    
   
public slots:
    void recalulate();
signals:
    void Fit(QVector< QPointer< QtCharts::QLineSeries > > fit);
    void Error(QVector< QPointer< QtCharts::QLineSeries > > fit);
};

#endif // MODELWIDGET_H

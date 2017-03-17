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

#ifndef MODELELEMENT_H
#define MODELELEMENT_H

#include "src/ui/mainwindow/chartwidget.h"

class SpinBox;


class ModelElement : public QGroupBox
{
    Q_OBJECT
public:
    ModelElement(QSharedPointer<AbstractTitrationModel> model, Charts charts, int no, QWidget *parent = 0);
    ~ModelElement();
    double D0() const;
    QVector<double > D() const;
    bool Include() const;
    
public slots:
    void Update();
    void ToggleSeries(int);
    
private:
    SpinBox *m_d_0;
    QVector<SpinBox * > m_constants;
    QLabel *m_error;
    QPushButton *m_remove, *m_optimize, *m_plot, *m_toggle;
    QCheckBox *m_include, *m_show;
    QSharedPointer<AbstractTitrationModel > m_model;
    QPointer<LineSeries > m_error_series, m_signal_series;
    
    int m_no;
    QColor m_color;
    Charts m_charts;

    void DisableSignal(int state);
    
private slots:
    void ColorChanged(const QColor &color);
    void ChooseColor();
    void togglePlot();
    void toggleActive();
    
signals:
    void ValueChanged();
    void Minimize(int i);
    void SetColor();
    void ActiveSignalChanged();
};

#endif

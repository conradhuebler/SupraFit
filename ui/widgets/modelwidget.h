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

#include <QtWidgets/qwidget.h>
#include <QtWidgets/QGroupBox>
#include <QDoubleSpinBox>
#include <QtWidgets/QVBoxLayout>

#include "core/data/dataclass.h"
#include "core/data/modelclass.h"

class QDoubleSpinBox;
class QPushButton;
class QLineEdit;
class QVBoxLayout;
class QGridLayout;

class ModelElement : public QGroupBox
{
  Q_OBJECT
public:
    ModelElement(QPointer<AbstractTitrationModel> model, int no, QWidget *parent = 0);
    ~ModelElement();
    double D0() const;
    QVector<double > D() const;
private:
    QDoubleSpinBox *m_d_0;
    QVector<QDoubleSpinBox * > m_constants;
    QPointer<QLineEdit > error;
    QPushButton *m_remove;
    QPointer<AbstractTitrationModel > m_model;
    int m_no;
    
private slots:
    void Update();
signals:
    void ValueChanged();
    void Minimize();
    void SetColor();
};




class ModelWidget : public QWidget
{
    Q_OBJECT

public:
ModelWidget(QPointer< AbstractTitrationModel > model, QWidget *parent = 0);
~ModelWidget();
virtual inline QSize sizeHint() const{ return QSize(250,50*m_sign_layout->count()); }
private:
    QPointer< AbstractTitrationModel > m_model;
    QVector<QPointer<QDoubleSpinBox >  >m_pure_signals;
    QVector< QVector<QPointer<QDoubleSpinBox > > > m_complex_signals;
    QVector<QPointer<QDoubleSpinBox> > m_constants;
    QVector<QPointer<ModelElement > > m_model_elements;
    QVector<QPointer<QLineEdit > > m_errors;
    QVector<QPointer< QPushButton > > m_sim_signal_remove;
    QVBoxLayout *m_sign_layout;
    QGridLayout *m_layout;
    QLineEdit *m_sum_error;
    QPushButton *m_switch, *m_minimize, *m_add_sim_signal; 
    bool m_pending;

    void DiscreteUI();
    void EmptyUI();
    
    void CollectParameters();
private slots:
    void Minimize();
    void Repaint();
    void AddSimSignal();
   
public slots:
    void recalulate();
signals:
    void Fit(QVector< QPointer< QtCharts::QLineSeries > > fit);
    void Error(QVector< QPointer< QtCharts::QLineSeries > > fit);
};

#endif // MODELWIDGET_H

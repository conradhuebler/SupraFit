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
#include <QtCharts/QLineSeries>

#include "src/core/dataclass.h"
#include "src/core/AbstractModel.h"

class QDoubleSpinBox;
class QPushButton;
class QLineEdit;
class QVBoxLayout;
class QGridLayout;
class QCheckBox;
class LineSeries;

class ModelElement : public QGroupBox
{
    Q_OBJECT
public:
    ModelElement(QPointer<AbstractTitrationModel> model, int no, QWidget *parent = 0);
    ~ModelElement();
    double D0() const;
    QVector<double > D() const;
    bool Include() const;
    
public slots:
    void Update();
private:
    QDoubleSpinBox *m_d_0;
    QVector<QDoubleSpinBox * > m_constants;
    QPointer<QLineEdit > error;
    QPushButton *m_remove, *m_optimize, *m_plot;
    QCheckBox *m_include, *m_show;
    QPointer<AbstractTitrationModel > m_model;
    QPointer<LineSeries > m_error_series, m_signal_series;
    
    int m_no;
    QColor m_color;
private slots:
    void SetOptimizer();
    void ColorChanged(const QColor &color);
    void ChooseColor();
signals:
    void ValueChanged();
    void Minimize(int i);
    void SetColor();
    void ActiveSignalChanged();
};




class ModelWidget : public QWidget
{
    Q_OBJECT
    
public:
    ModelWidget(QPointer< AbstractTitrationModel > model, QWidget *parent = 0);
    ~ModelWidget();
    virtual inline QSize sizeHint() const{ return QSize(250,50*m_sign_layout->count()); }
    QPointer< AbstractTitrationModel > Model() { return m_model; }
    void setMaxIter(int maxiter);

    
private:
    QPointer< AbstractTitrationModel > m_model;
    QVector<QPointer<QDoubleSpinBox >  >m_pure_signals;
    QVector< QVector<QPointer<QDoubleSpinBox > > > m_complex_signals;
    QVector<QPointer<QDoubleSpinBox> > m_constants;
    QVector<QPointer<ModelElement > > m_model_elements;
    QVector<QPointer<QLineEdit > > m_errors;
    QVector<QPointer< QPushButton > > m_sim_signal_remove;
    QSpinBox *m_maxiter;
    QVBoxLayout *m_sign_layout;
    QGridLayout *m_layout;
    QLineEdit *m_sum_error;
    QPushButton *m_switch, *m_minimize_all, *m_minimize_single, *m_add_sim_signal, *m_new_guess, *m_optim_config, *m_export, *m_import; 
    bool m_pending;
    QVector<int > ActiveSignals();
    void DiscreteUI();
    void EmptyUI();
    
    void CollectParameters();
private slots:
    void GlobalMinimize();
    void LocalMinimize();
    void Repaint();
    void AddSimSignal();
    void CollectActiveSignals();
    void NewGuess();
    void ImportConstants();
    void ExportConstants();
public slots:
    void recalulate();
    void OptimizerSettings();
signals:
    void Fit(QVector< QPointer< QtCharts::QLineSeries > > fit);
    void Error(QVector< QPointer< QtCharts::QLineSeries > > fit);
    void ActiveSignalChanged(QVector<int > active_signals);
};

#endif // MODELWIDGET_H

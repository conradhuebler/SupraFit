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
#include "src/core/minimizer.h"
#include "src/ui/widgets/chartwidget.h"

#include "src/core/dataclass.h"
#include "src/core/AbstractModel.h"

#include "src/ui/dialogs/modeldialog.h"
#include <QApplication>

#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
#include <QtWidgets/QWidget>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QVBoxLayout>

#include <QtCharts/QLineSeries>
#include <QtDataVisualization>

class ModalDialog;
class QDoubleSpinBox;
class QPushButton;
class QLineEdit;
class QVBoxLayout;
class QGridLayout;
class QCheckBox;
class LineSeries;
class AdvancedSearch;
class ChartView;
class _3DChartView;
class OptimizerFlagWidget;

struct  ModelHistoryElement;
struct Charts;

class Waiter
{
public:
    Waiter() {QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));}
    ~Waiter() {QApplication::restoreOverrideCursor();}
};

class SpinBox : public QDoubleSpinBox
{
    Q_OBJECT
public:
    SpinBox(QWidget * parent = 0 );
protected:
    bool valueBeingSet;
public slots:
    void setValue (double val);
private slots:
    void On_valueChanged(double val);
signals:
    void valueChangedNotBySet(double val);
};


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
private:
    SpinBox *m_d_0;
    QVector<SpinBox * > m_constants;
    QPointer<QLineEdit > error;
    QPushButton *m_remove, *m_optimize, *m_plot;
    QCheckBox *m_include, *m_show;
    QSharedPointer<AbstractTitrationModel > m_model;
    QPointer<LineSeries > m_error_series, m_signal_series;
    
    int m_no;
    QColor m_color;
    Charts m_charts;
private slots:
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
    ModelWidget(QSharedPointer< AbstractTitrationModel > model, Charts charts, QWidget *parent = 0);
    ~ModelWidget();
    virtual inline QSize sizeHint() const{ return QSize(250,50*m_sign_layout->count()); }
    QSharedPointer< AbstractTitrationModel > Model() { return m_model; }
    void setMaxIter(int maxiter);
    QSharedPointer<Minimizer > getMinimizer() { return m_minimizer; }
    QSharedPointer<Minimizer > m_minimizer;
public slots:
    void LoadJson(const QJsonObject &object);
private:
    QSharedPointer< AbstractTitrationModel > m_model;
    QVector<QPointer<SpinBox >  >m_pure_signals;
    QVector<QVector <QPointer<SpinBox > > > m_complex_signals;
    QVector<QPointer<SpinBox> > m_constants;
    QVector<QPointer<ModelElement > > m_model_elements;
    QVector<QPointer<QLineEdit > > m_errors;
    QVector<QPointer< QPushButton > > m_sim_signal_remove;
    QPointer<AdvancedSearch> m_advancedsearch;
    QSpinBox *m_maxiter;
    QVBoxLayout *m_sign_layout;
    QGridLayout *m_layout;
    QLineEdit *m_sum_error;
    QPointer< QPushButton > m_switch, m_minimize_all, m_minimize_single, m_add_sim_signal, m_new_guess, m_optim_config, m_export, m_import, m_advanced, m_plot_3d, m_confi; 
    bool m_pending;
    QList<int > ActiveSignals();
    void DiscreteUI();
    void EmptyUI();
    
    void CollectParameters();
    ChartView *view;
    QPointer<_3DChartView > _3dchart;
    QPointer<OptimizerFlagWidget> m_optim_flags;
    ModalDialog *m_statistic_dialog, *m_search_dialog;
//     OptimizationType m_last_run;
    bool m_statistic;
    Charts m_charts;
private slots:
    void GlobalMinimize();
    void LocalMinimize();
    void Repaint();
    void AddSimSignal();
    void CollectActiveSignals();
    void NewGuess();
    void ImportConstants();
    void ExportConstants();
    void setParameter();
    void OpenAdvancedSearch();
    void AdvancedSearchFinished(int runtype);
    void triggerPlot3D();
    void Confidence();
public slots:
    void recalulate();
    void OptimizerSettings();
signals:
    void Fit(QVector< QPointer< QtCharts::QLineSeries > > fit);
    void Error(QVector< QPointer< QtCharts::QLineSeries > > fit);
    void ActiveSignalChanged(QVector<int > active_signals);
    void Update();
//     void AddModel(const QJsonObject &json);
};

#endif // MODELWIDGET_H

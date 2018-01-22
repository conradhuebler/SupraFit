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

#include "src/capabilities/weakenedgridsearch.h"
#include "src/capabilities/modelcomparison.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/capabilities/reductionanalyse.h"

#include "src/core/models.h"
#include "src/core/toolset.h"

#include "src/ui/widgets/optimizerflagwidget.h"
#include "src/ui/guitools/waiter.h"

#include <QtCore/QDateTime>
#include <QtCore/QMutexLocker>
#include <QtCore/QThreadPool>
#include <QtCore/QTimer>

#include <QPropertyAnimation>

#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QRadioButton>

#include "statisticdialog.h"
StatisticDialog::StatisticDialog(QSharedPointer<AbstractModel> model, QWidget *parent) : QDialog(parent), m_model(model), m_runs(1), m_hidden(false)
{
    setUi();
    HideWidget();
    connect(m_model.data(), SIGNAL(Recalculated()), this, SLOT(Update()));
}

StatisticDialog::StatisticDialog(QWidget *parent) : QDialog(parent)
{
    setUi();
    HideWidget();
}

StatisticDialog::~StatisticDialog()
{
    
    
    
}

void StatisticDialog::setVisible(bool visible)
{
    updateUI();
    QDialog::setVisible(visible);
}

void StatisticDialog::updateUI()
{
    if(!m_model)
        return;
    m_cv_f_value->setToolTip(FOutput());
    m_moco_f_value->setToolTip(FOutput());
    m_cv_f_value->setValue(m_model.data()->finv(m_cv_maxerror->value()/100)); 
    m_moco_f_value->setValue(m_model.data()->finv(m_moco_maxerror->value()/100));
    if(m_model.data()->GlobalParameterSize() != 2)
    {
        m_moco_widget->setDisabled(true);
        m_moco_widget->setToolTip("<p>Model Comparison is not enabled for 1:1 and 2:1/1:1/1:2 models.</p><p>For 1:1 models the result are equal to the automatic confidence calculation.</p><p>For more complicated models the confidence region becomes an ellipsoid, which is not supported yet.</br>This may come in later release of suprafit</p>");
    }
}


void StatisticDialog::setUi()
{
    QVBoxLayout *layout = new QVBoxLayout;
    m_tab_widget = new QTabWidget;
    m_tab_widget->addTab(MonteCarloWidget(), tr("Monte Carlo"));
    m_tab_widget->addTab(GridSearchWidget(), tr("Weakened Grid Search"));
    m_moco_widget = ModelComparison();
    m_tab_widget->addTab(m_moco_widget, tr("Model Comparison"));
    m_tab_widget->addTab(CVWidget(), tr("Cross Validation"));
    
    if(m_model)
        m_optim_flags = new OptimizerFlagWidget(m_model.data()->LastOptimzationRun()); 
    else
        m_optim_flags = new OptimizerFlagWidget;
    layout->addWidget(m_tab_widget);
    m_time_info = new QLabel;
    m_progress = new QProgressBar;
    layout->addWidget(m_time_info);
    
    m_use_checked = new QCheckBox(tr("Consider Only Checked Tabs."));
    m_use_checked->setChecked(false);
    
    if(!m_model)
        layout->addWidget(m_use_checked);
    
    m_hide = new QPushButton(tr("Hide"));
    m_interrupt = new QPushButton(tr("Interrupt"));
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(m_progress);
    hLayout->addWidget(m_interrupt);
    m_hide_widget = new QWidget;
    m_hide_widget->setLayout(hLayout);
    layout->addWidget(m_hide_widget);
    m_hide_widget->setMaximumHeight(100);
    layout->addWidget(m_hide);
    connect(m_hide, SIGNAL(clicked()), this, SLOT(reject()));
    connect(m_interrupt, SIGNAL(clicked()), this, SIGNAL(Interrupt()));
    connect(this, SIGNAL(setMaximumSteps(int)), m_progress, SLOT(setMaximum(int)));
    setLayout(layout);
    EnableWidgets();
    updateUI();
    CalculateError();
    setWindowTitle("Statistic Tools");
    setMinimumWidth(1.5*m_tab_widget->sizeHint().width());
}

QWidget *StatisticDialog::MonteCarloWidget()
{
    QWidget *mc_widget = new QWidget;
    QGridLayout *layout = new QGridLayout;
    
    m_mc_steps = new QSpinBox;
    m_mc_steps->setMinimum(1);
    m_mc_steps->setMaximum(1e9);
    m_mc_steps->setValue(1000);
    m_mc_steps->setSingleStep(1e2);
    layout->addWidget(new QLabel(tr("Number of MC Steps:")), 0, 0);
    layout->addWidget(m_mc_steps, 0, 1);
    
    m_varianz_box = new QDoubleSpinBox;
    if(m_model)
    {
        m_varianz_box->setDecimals(5);
        m_varianz_box->setSingleStep(1e-2);
        m_varianz_box->setValue(m_model.data()->StdDeviation());
    }else{
        m_varianz_box->setDisabled(true);
        m_varianz_box->setToolTip(tr("Variance of each model will be set automatically."));
    }
    if(m_model)
    {
        layout->addWidget(new QLabel(tr("Varianz")), 1, 0);
        layout->addWidget(m_varianz_box, 1, 1);
    }
    
    m_original = new QCheckBox;
    m_original->setText(tr("Use Original Data"));
    layout->addWidget(m_original, 2, 0, 1, 2);
    
    m_bootstrap = new QCheckBox;
    m_bootstrap->setText(tr("Bootstrap"));
    layout->addWidget(m_bootstrap, 3, 0, 1, 2);
    
    m_mc = new QPushButton(tr("Simulate"));
    layout->addWidget(m_mc, 4, 0, 1, 2);
    
    connect(m_mc, SIGNAL(clicked()), this, SIGNAL(MCStatistic()));
    connect(m_bootstrap, SIGNAL(stateChanged(int)), this, SLOT(EnableWidgets()));
    mc_widget->setLayout(layout);
    return mc_widget;
}


QWidget * StatisticDialog::GridSearchWidget()
{
    QWidget *cv_widget = new QWidget;
    QGridLayout *layout = new QGridLayout;   
    
    m_cv_increment = new QDoubleSpinBox;
    m_cv_increment->setDecimals(6);
    m_cv_increment->setValue(0.001);
    m_cv_increment->setSingleStep(1e-3);
    layout->addWidget(new QLabel(tr("Increment")), 0, 0);
    layout->addWidget(m_cv_increment, 0, 1, 1, 2);
    
    m_cv_maxerror = new QDoubleSpinBox;
    m_cv_maxerror->setMaximum(100);
    m_cv_maxerror->setSingleStep(0.5);
    m_cv_maxerror->setValue(95);
    m_cv_maxerror->setDecimals(1);
    m_cv_maxerror->setSuffix("%");
    m_cv_f_test = new QCheckBox(tr("Use F-Statistic"));
    m_cv_f_test->setChecked(true);
    m_cv_f_value = new QDoubleSpinBox;
    m_cv_f_value->setMaximum(1000);
    m_cv_f_value->setMinimum(0);
    m_cv_f_value->setValue(m_f_value);
    m_cv_f_value->setDecimals(4);
    m_cv_f_value->setReadOnly(true);
    
    connect(m_cv_maxerror, SIGNAL(valueChanged(qreal)), this, SLOT(CalculateError()));
    connect(m_cv_f_test, SIGNAL(stateChanged(int)), this, SLOT(CalculateError()));
    
    layout->addWidget(new QLabel(tr("Confidence Inerval")), 1, 0);
    layout->addWidget(m_cv_maxerror, 1, 1);
    layout->addWidget(m_cv_f_test, 1, 2);  
    
    if(m_model)
    {
        layout->addWidget(new QLabel(tr("F-Value:")), 2, 0);
        layout->addWidget(m_cv_f_value, 2, 1);
        
        m_cv_error_info = new QLabel;
        layout->addWidget(m_cv_error_info, 3, 0, 1, 3);
    }
    m_cv_steps = new QSpinBox;
    m_cv_steps->setMaximum(1e7);
    m_cv_steps->setValue(5000);
    m_cv_steps->setSingleStep(100);
    
    layout->addWidget(new QLabel(tr("Max. Steps")), 4, 0);
    layout->addWidget(m_cv_steps, 4, 1, 1, 2);
    
    m_cv = new QPushButton(tr("Calculate"));
    layout->addWidget(m_cv, 5, 0, 1, 3);
    
    connect(m_cv, SIGNAL(clicked()), this, SIGNAL(WGStatistic()));
    cv_widget->setLayout(layout);
    return cv_widget;
}


QWidget * StatisticDialog::ModelComparison()
{
    QWidget *mo_widget = new QWidget;
    QGridLayout *layout = new QGridLayout;   
    
    m_moco_global = new QGroupBox(tr("Settings"));
    QGridLayout *global_layout = new QGridLayout;
    
    m_moco_maxerror = new QDoubleSpinBox;
    m_moco_maxerror->setMaximum(100);
    m_moco_maxerror->setSingleStep(0.5);
    m_moco_maxerror->setValue(95);
    m_moco_maxerror->setDecimals(1);
    m_moco_maxerror->setSuffix("%");
    m_moco_f_test = new QCheckBox(tr("Use F-Statistic"));
    m_moco_f_test->setChecked(true);
    
    m_moco_f_value = new QDoubleSpinBox;
    m_moco_f_value->setMaximum(1000);
    m_moco_f_value->setMinimum(0);
    m_moco_f_value->setValue(m_f_value);
    m_moco_f_value->setDecimals(4);
    m_moco_f_value->setReadOnly(true);
    
    connect(m_moco_maxerror, SIGNAL(valueChanged(qreal)), this, SLOT(CalculateError()));
    connect(m_moco_f_test, SIGNAL(stateChanged(int)), this, SLOT(CalculateError()));
    
    global_layout->addWidget(new QLabel(tr("Confidence Intervall")), 0, 0);
    global_layout->addWidget(m_moco_maxerror, 0, 1);
    global_layout->addWidget(m_moco_f_test, 0, 2);
    
    if(m_model)
    {
        global_layout->addWidget(new QLabel(tr("F-Value:")), 1, 0);
        global_layout->addWidget(m_moco_f_value, 1, 1);
        m_moco_error_info = new QLabel;
        global_layout->addWidget(m_moco_error_info, 2, 0, 1, 3);
    }
    
    m_moco_box_multi = new QDoubleSpinBox;
    m_moco_box_multi->setMaximum(20);
    m_moco_box_multi->setSingleStep(0.5);
    m_moco_box_multi->setValue(2);
    m_moco_box_multi->setDecimals(2);
    global_layout->addWidget(new QLabel(tr("Box Scaling")), 3, 0);
    global_layout->addWidget(m_moco_box_multi, 3, 1);
    
    m_moco_global->setLayout(global_layout);
    layout->addWidget(m_moco_global, 0, 0, 1, 3);
    m_moco_monte_carlo = new QGroupBox(tr("Monte Carlo Settings"));
    QGridLayout *monte_layout = new QGridLayout;
    
    m_moco_mc_steps = new QSpinBox;
    m_moco_mc_steps->setMaximum(1e7);
    m_moco_mc_steps->setValue(10000);
    m_moco_mc_steps->setSingleStep(100);
    
    monte_layout->addWidget(new QLabel(tr("Max. Steps")), 1, 0);
    monte_layout->addWidget(m_moco_mc_steps, 1, 1, 1, 2);
    m_moco_monte_carlo->setLayout(monte_layout);
    layout->addWidget(m_moco_monte_carlo, 1, 0, 1, 3);
    
    m_moco = new QPushButton(tr("Start ..."));
    layout->addWidget(m_moco, 5, 0, 1, 3);
    
    connect(m_moco, SIGNAL(clicked()), this, SIGNAL(MoCoStatistic()));
    mo_widget->setLayout(layout);
    return mo_widget;
}


QWidget * StatisticDialog::CVWidget()
{
    QWidget *widget = new QWidget;
    QGridLayout *layout = new QGridLayout;   
    m_cv_loo = new QRadioButton(tr("Leave-One-Out"));
    m_cv_l2o = new QRadioButton(tr("Leave-Two-Out"));
    layout->addWidget(m_cv_loo, 0, 0);
    layout->addWidget(m_cv_l2o, 1, 0);
    m_cross_validate = new QPushButton(tr("Cross Validation"));
    
    layout->addWidget(m_cross_validate, 2,0);
    connect(m_cross_validate, SIGNAL(clicked()), this, SIGNAL(CrossValidation()));
    widget->setLayout(layout);
    return widget;
}


WGSConfig StatisticDialog::getWGSConfig()
{
    WGSConfig config;
    config.increment = m_cv_increment->value();
    config.maxsteps = m_cv_steps->value();
    config.maxerror = m_cv_max;
    config.relax = true;
    config.fisher_statistic = m_cv_f_test->isChecked();
    config.confidence = m_cv_maxerror->value();
    config.f_value = m_cv_f_value->value();
    m_time = 0;
    m_time_0 = QDateTime::currentMSecsSinceEpoch();
    m_progress->setMaximum(m_runs);
    m_progress->setValue(0);
    ShowWidget();
    return config;
}

MoCoConfig StatisticDialog::getMoCoConfig()
{
    MoCoConfig config;
    WGSConfig cv_config;
    config.mc_steps = m_moco_mc_steps->value();
    config.box_multi = m_moco_box_multi->value();
    config.maxerror = m_moco_max;
    config.confidence = m_moco_maxerror->value();
    cv_config.relax = true;
    config.cv_config = cv_config;
    config.fisher_statistic = m_moco_f_test->isChecked();
    config.f_value = m_moco_f_value->value();
    m_time = 0;
    m_time_0 = QDateTime::currentMSecsSinceEpoch();
    m_progress->setMaximum(m_runs*(1+m_moco_mc_steps->value()/update_intervall));
    m_progress->setValue(0);
    ShowWidget();
    return config;
}

MCConfig StatisticDialog::getMCConfig()
{
    MCConfig config;
    config.variance = m_varianz_box->value();
    config.maxsteps = m_mc_steps->value();
    config.original = m_original->isChecked();
    config.bootstrap = m_bootstrap->isChecked();
    m_time = 0;
    m_time_0 = QDateTime::currentMSecsSinceEpoch();
    m_progress->setMaximum(m_runs*(m_mc_steps->value() + m_mc_steps->value()/100));
    m_progress->setValue(0);
    ShowWidget();
    return config;
}

ReductionAnalyse::CVType StatisticDialog::CrossValidationType()
{
    m_time = 0;
    m_time_0 = QDateTime::currentMSecsSinceEpoch();
    m_progress->setMaximum(0);
    m_progress->setValue(0);
    ShowWidget();
    
    if(m_cv_loo->isChecked())
        return ReductionAnalyse::LeaveOneOut;
    else if(m_cv_l2o->isChecked())
        return ReductionAnalyse::LeaveTwoOut;
}

void StatisticDialog::MaximumSteps(int steps)
{
    if(m_hidden)
        ShowWidget();
    m_progress->setMaximum(steps);
}


void StatisticDialog::IncrementProgress(int time)
{
    QMutexLocker locker(&mutex);
    
    m_time += time;
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
    int val = m_progress->value() + 1;
    qreal aver = double(m_time)/val;
    int remain = double(m_progress->maximum() - val)*aver/qAbs(QThreadPool::globalInstance()->maxThreadCount())/1000;
    int used = (t0 - m_time_0)/1000;
    
    if(m_progress->maximum() == -1)
    {
        m_time_info->setText(tr("Nobody knows when this will end.\nBut you hold the door for %2 sec. .").arg(used));
    }else{
        m_time_info->setText(tr("Remaining time approx: %1 sec., elapsed time: %2 sec. .").arg(remain).arg(used));
        m_progress->setValue(val);
    }
    
}

QString StatisticDialog::FOutput() const
{
    QString string;
    if(m_model)
    {
        string +=  "Parameter fitted:<b>" + QString::number(m_model.data()->Parameter()) + "</b>\n";
        string +=  "Number of used Points:<b>" + QString::number(m_model.data()->Points()) + "</b>\n";
        string +=  "Degrees of Freedom:<b>" + QString::number(m_model.data()->Points()-m_model.data()->Parameter()) + "</b>\n";
    }
    return string;
}

void StatisticDialog::ShowWidget()
{        
    QPropertyAnimation *animation = new QPropertyAnimation(m_hide_widget, "maximumHeight");
    animation->setEasingCurve(QEasingCurve::InOutCubic);
    animation->setDuration(200);
    animation->setStartValue(0);
    animation->setEndValue(100);
    animation->start();
    m_tab_widget->setDisabled(true);
    m_hidden = false;
}

void StatisticDialog::HideWidget()
{        
    QPropertyAnimation *animation = new QPropertyAnimation(m_hide_widget, "maximumHeight");
    animation->setEasingCurve(QEasingCurve::InOutCubic);
    animation->setDuration(200);
    animation->setStartValue(100);
    animation->setEndValue(0);
    animation->start();
    m_progress->setMaximum(0);
    m_progress->setMinimum(0);
    m_tab_widget->setDisabled(false);
    m_hidden = true;
}


void StatisticDialog::Update()
{
    m_varianz_box->setValue(m_model.data()->StdDeviation());
    CalculateError();
}

void StatisticDialog::EnableWidgets()
{
    m_varianz_box->setEnabled(!m_bootstrap->isChecked());
}


void StatisticDialog::CalculateError()
{
    if(!m_model.data())
        return;
    updateUI();
    qreal error = m_model.data()->SumofSquares();
    qreal max_moco_error, max_cv_error;
    QString cv_message, moco_message;
    if(m_cv_f_test->isChecked())
    {
        max_cv_error = error*(m_cv_f_value->value()*m_model.data()->Parameter()/(m_model.data()->Points()-m_model.data()->Parameter()) +1);
        cv_message = "The current error is "+ QString::number(error) + ".\nThe maximum error will be " + QString::number(max_cv_error) + ".";
    }else
    {
        max_cv_error = error+error*m_cv_maxerror->value()/double(100);
        cv_message = "The current error is "+ QString::number(error) + ".\nThe maximum error will be " + QString::number(max_cv_error) + ". F-Statistic is not used!";
    }
    
    if(m_moco_f_test->isChecked())
    {
        max_moco_error = error*(m_moco_f_value->value()*m_model.data()->Parameter()/(m_model.data()->Points()-m_model.data()->Parameter()) +1);
        moco_message = "The current error is "+ QString::number(error) + ".\nThe maximum error will be " + QString::number(max_moco_error) + ".";
    }else
    {
        max_moco_error = error+error*m_moco_maxerror->value()/double(100);
        moco_message = "The current error is "+ QString::number(error) + ".\nThe maximum error will be " + QString::number(max_moco_error) + ". F-Statistic is not used!";
    }
    m_moco_max = max_moco_error;
    m_cv_max = max_cv_error;
    
    //     qDebug() << m_model.data()->Error(m_moco_maxerror->value(), m_moco_f_test->isChecked()) << m_moco_max;
    m_cv_error_info->setText(cv_message);
    m_moco_error_info->setText(moco_message);
}

#include "statisticdialog.moc"

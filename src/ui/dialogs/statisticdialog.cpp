/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/capabilities/jobmanager.h"

#include "src/core/models/AbstractModel.h"

#include "src/core/toolset.h"

#include "src/ui/guitools/waiter.h"
#include "src/ui/widgets/buttons/scientificbox.h"

#include <QtCore/QDateTime>
#include <QtCore/QMutexLocker>
#include <QtCore/QThreadPool>
#include <QtCore/QTimer>

#include <QPropertyAnimation>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>

#include "statisticdialog.h"
StatisticDialog::StatisticDialog(QSharedPointer<AbstractModel> model, QWidget* parent)
    : QDialog(parent)
    , m_model(model)
    , m_hidden(false)
{
    setUi();
    m_main_progress->hide();
    HideWidget();
    connect(m_model.toStrongRef().data(), SIGNAL(Recalculated()), this, SLOT(Update()));
}

StatisticDialog::StatisticDialog(QWidget* parent)
    : QDialog(parent)
{
    setUi();
    HideWidget();
}

StatisticDialog::~StatisticDialog()
{
    disconnect(m_model.toStrongRef().data());
    m_model.clear();
}

void StatisticDialog::Attention()
{
    show();
    raise();
    activateWindow();
}

void StatisticDialog::setVisible(bool visible)
{
    updateUI();
    QDialog::setVisible(visible);
}

void StatisticDialog::updateUI()
{
    if (!m_model)
        return;
    m_wgs_f_value->setToolTip(FOutput());
    m_moco_f_value->setToolTip(FOutput());
    m_wgs_f_value->setValue(m_model.toStrongRef().data()->finv(m_wgs_maxerror->value() / 100));
    m_moco_f_value->setValue(m_model.toStrongRef().data()->finv(m_moco_maxerror->value() / 100));
}

void StatisticDialog::setUi()
{
    QVBoxLayout* layout = new QVBoxLayout;
    m_tab_widget = new QTabWidget;
    m_tab_widget->addTab(CVWidget(), tr("Cross Validation/Reduction"));
    m_tab_widget->addTab(MonteCarloWidget(), tr("Monte Carlo"));
    m_tab_widget->addTab(GridSearchWidget(), tr("Weakened Grid Search"));
    m_moco_widget = ModelComparison();
    m_tab_widget->addTab(m_moco_widget, tr("Model Comparison"));

    layout->addWidget(m_tab_widget);
    m_time_info = new QLabel;
    m_message_box = new QLabel;
    m_progress = new QProgressBar;
    m_main_progress = new QProgressBar;
    layout->addWidget(m_time_info);
    layout->addWidget(m_message_box);

    m_use_checked = new QCheckBox(tr("Consider Only Checked Tabs."));
    m_use_checked->setChecked(false);

    if (!m_model)
        layout->addWidget(m_use_checked);

    m_hide = new QPushButton(tr("Hide"));
    m_interrupt = new QPushButton(tr("Interrupt"));
    QHBoxLayout* hLayout = new QHBoxLayout;
    QVBoxLayout* progress_layout = new QVBoxLayout;
    progress_layout->addWidget(m_progress);
    progress_layout->addWidget(m_main_progress);
    hLayout->addLayout(progress_layout);
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
    updateUI();
    CalculateError();
    setWindowTitle("Statistic Tools");
    setMinimumWidth(1.5 * m_tab_widget->sizeHint().width());
}

QWidget* StatisticDialog::MonteCarloWidget()
{
    QWidget* mc_widget = new QWidget;
    QGridLayout* layout = new QGridLayout;

    m_mc_steps = new QSpinBox;
    m_mc_steps->setMinimum(2);
    m_mc_steps->setMaximum(1e9);
    m_mc_steps->setValue(2000);
    m_mc_steps->setSingleStep(1e2);
    layout->addWidget(new QLabel(tr("Number of MC Steps:")), 0, 0);
    layout->addWidget(m_mc_steps, 0, 1);

    m_mc_std = new QRadioButton;
    m_mc_std->setText(tr("%1 from fit").arg(Unicode_sigma));
    m_mc_sey = new QRadioButton;
    m_mc_sey->setText(tr("SE%1").arg(Unicode_Sub_y));
    m_mc_user = new QRadioButton;
    m_mc_user->setText(tr("%1 from user input").arg(Unicode_sigma));

    m_varianz_box = new QDoubleSpinBox;
    m_varianz_box->setDecimals(6);
    m_varianz_box->setSingleStep(1e-2);
    m_varianz_box->setMaximum(1e10);

    layout->addWidget(new QLabel(tr("Standard Deviation %1").arg(Unicode_sigma)), 1, 0);
    layout->addWidget(m_varianz_box, 1, 1);

    QHBoxLayout* hlayout = new QHBoxLayout;

    hlayout->addWidget(new QLabel(tr("is taken as:")));

#ifndef extended_f_test
    hlayout->addWidget(m_mc_sey);
    hlayout->addWidget(m_mc_std);
    hlayout->addWidget(m_mc_user);
#endif

    m_mc_bootstrap = new QRadioButton;
    m_mc_bootstrap->setText(tr("or use bootstrapping"));
    m_mc_bootstrap->setToolTip(tr("Using bootstrapping, no new errors will generated but rather the old error will randomly distributed on the model."));
    hlayout->addWidget(m_mc_bootstrap);

    m_mc_sey->setChecked(true);
    m_varianz_box->setReadOnly(!m_mc_user->isChecked());
    if (m_model)
        m_varianz_box->setValue(m_model.toStrongRef().data()->SEy());

    connect(m_mc_std, &QRadioButton::toggled, m_mc_std, [this]() {
        if (m_mc_std->isChecked() && m_model) {
            m_varianz_box->setValue(m_model.toStrongRef().data()->StdDeviation());
            m_varianz_box->setReadOnly(!m_mc_user->isChecked());
        }
    });

    connect(m_mc_sey, &QRadioButton::toggled, m_mc_sey, [this]() {
        if (m_mc_sey->isChecked() && m_model) {
            m_varianz_box->setValue(m_model.toStrongRef().data()->SEy());
            m_varianz_box->setReadOnly(!m_mc_user->isChecked());
        }
    });

    connect(m_mc_user, &QRadioButton::toggled, m_mc_user, [this]() {
        if (m_mc_user->isChecked()) {
            m_varianz_box->setReadOnly(!m_mc_user->isChecked());
            m_varianz_box->setValue(1e-3);
        }
    });

    connect(m_mc_bootstrap, &QRadioButton::toggled, m_mc_bootstrap, [this]() {
        if (m_mc_user->isChecked()) {
            m_varianz_box->setReadOnly(!m_mc_user->isChecked());
            m_varianz_box->setValue(1e-3);
        }
    });

    layout->addLayout(hlayout, 2, 0, 1, 2);

    m_original = new QCheckBox;
    m_original->setText(tr("Use Original Data"));
    layout->addWidget(m_original, 3, 0);

    QVBoxLayout* indep_layout = new QVBoxLayout;
    if (m_model.toStrongRef().data()) {
        QStringList header = m_model.toStrongRef().data()->IndependentModel()->header();

        indep_layout->addWidget(new QLabel(tr("Create random scatter of independent\nvariables, eg. input concentrations etc!")));

        for (int i = 0; i < m_model.toStrongRef().data()->IndependentVariableSize(); ++i) {

            QCheckBox* checkbox = new QCheckBox(header[i]);
            QDoubleSpinBox* var = new QDoubleSpinBox;
            var->setDecimals(5);
            var->setSingleStep(1e-4);
            var->setDisabled(true);
            connect(checkbox, &QCheckBox::stateChanged, var, &QDoubleSpinBox::setEnabled);
            QHBoxLayout* layout = new QHBoxLayout;
            layout->addWidget(checkbox);
            layout->addWidget(var);
            m_indepdent_checkboxes << checkbox;
            m_indepdent_variance << var;
            indep_layout->addLayout(layout);
        }
    }
    layout->addLayout(indep_layout, 4, 0, 1, 2);
    m_mc = new QPushButton(tr("Simulate"));
    layout->addWidget(m_mc, 5, 0, 1, 2);

    connect(m_mc, &QPushButton::clicked, this, [this]() {
        clearMessages();
        emit RunCalculation(RunMonteCarlo());
    });

    mc_widget->setLayout(layout);
    return mc_widget;
}

QWidget* StatisticDialog::GridSearchWidget()
{
    QWidget* cv_widget = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout;

    QWidget* parameter = new QWidget(this);
    if (m_model) {
        layout->addWidget(new QLabel(tr("Choose parameter to be tested:")));
        QVBoxLayout* layout = new QVBoxLayout;
        for (int i = 0; i < m_model.toStrongRef().data()->GlobalParameterSize(); ++i) {
            QCheckBox* checkbox = new QCheckBox;
            checkbox->setChecked(true);
            m_grid_global << checkbox;
            QHBoxLayout* hlayout = new QHBoxLayout;
            hlayout->addWidget(checkbox);
            hlayout->addWidget(new QLabel("<html>" + m_model.toStrongRef().data()->GlobalParameterName(i) + "</html>"));
            hlayout->addStretch(100);
            layout->addLayout(hlayout);

            connect(m_model.toStrongRef().data(), &AbstractModel::Recalculated, this, [this, i, checkbox]() {
                if (m_model)
                    checkbox->setEnabled(m_model.toStrongRef().data()->GlobalEnabled(i));
            });
        }

        for (int i = 0; i < m_model.toStrongRef().data()->LocalParameterSize(); ++i) {
            QCheckBox* checkbox = new QCheckBox;
            m_grid_local << checkbox;
            QHBoxLayout* hlayout = new QHBoxLayout;
            hlayout->addWidget(checkbox);
            hlayout->addWidget(new QLabel("<html>" + m_model.toStrongRef().data()->LocalParameterName(i) + "</html>"));
            hlayout->addStretch(100);
            layout->addLayout(hlayout);

            connect(m_model.toStrongRef().data(), &AbstractModel::Recalculated, this, [this, i, checkbox]() {
                if (m_model)
                    checkbox->setEnabled(m_model.toStrongRef().data()->LocalEnabled(i));
            });
        }
        parameter->setLayout(layout);
    }
    layout->addWidget(parameter);

    m_store_wgsearch = new QCheckBox(tr("Store intermediate Models"));
    m_store_wgsearch->setToolTip(tr("If checked, SupraFit will try to store ALL intermedate results. They are necessary to compute the confidence range for the single parameters \nAND dervied values, such as entropy. The downside are hugh files and sometimes data, that is to large to be handled resulting in an empty output!"));
    layout->addWidget(m_store_wgsearch);

    m_wgs_maxerror = new QDoubleSpinBox;
    m_wgs_maxerror->setMaximum(100);
    m_wgs_maxerror->setSingleStep(0.5);
    m_wgs_maxerror->setValue(95);
    m_wgs_maxerror->setDecimals(1);
    m_wgs_maxerror->setSuffix("%");

    m_wgs_f_value = new QDoubleSpinBox(this);
    m_wgs_f_value->setMaximum(1000);
    m_wgs_f_value->setMinimum(0);
    m_wgs_f_value->setValue(m_f_value);
    m_wgs_f_value->setDecimals(4);
    m_wgs_f_value->setReadOnly(true);

    connect(m_wgs_maxerror, SIGNAL(valueChanged(qreal)), this, SLOT(CalculateError()));

    QHBoxLayout* hlayout = new QHBoxLayout;

    hlayout->addWidget(new QLabel(tr("Define Confidence Interval")));
    hlayout->addWidget(m_wgs_maxerror);

    layout->addLayout(hlayout);
    if (m_model) {
        hlayout = new QHBoxLayout;
        hlayout->addWidget(new QLabel(tr("F-Value:")));
        hlayout->addWidget(m_wgs_f_value);
        layout->addLayout(hlayout);

        m_wgs_error_info = new QLabel;
        layout->addWidget(m_wgs_error_info);
    } else
        m_wgs_f_value->hide();
    hlayout = new QHBoxLayout;

    m_radio_wgs_sse = new QRadioButton(tr("SSE"));
    connect(m_radio_wgs_sse, &QRadioButton::toggled, this, [this](int state) {
        if (state) {
            m_ParameterIndex = 0;
            m_error_name = m_radio_wgs_sse->text();
            CalculateError();
        }
    });
    m_radio_wgs_ssy = new QRadioButton(tr("SEy"));
    connect(m_radio_wgs_ssy, &QRadioButton::toggled, this, [this](int state) {
        if (state) {
            m_ParameterIndex = 1;
            m_error_name = m_radio_wgs_ssy->text();
            CalculateError();
        }
    });
    m_radio_wgs_chi = new QRadioButton(tr("%1%2").arg(Unicode_chi).arg(Unicode_Sup_2));
    connect(m_radio_wgs_chi, &QRadioButton::toggled, this, [this](int state) {
        if (state) {
            m_ParameterIndex = 2;
            m_error_name = m_radio_wgs_chi->text();
            CalculateError();
        }
    });
    m_radio_wgs_sigma = new QRadioButton(tr("%1").arg(Unicode_sigma));
    connect(m_radio_wgs_sigma, &QRadioButton::toggled, this, [this](int state) {
        if (state) {
            m_ParameterIndex = 3;
            m_error_name = m_radio_wgs_sigma->text();
            CalculateError();
        }
    });

    hlayout->addWidget(m_radio_wgs_sse);
    hlayout->addWidget(m_radio_wgs_ssy);
    hlayout->addWidget(m_radio_wgs_chi);
    hlayout->addWidget(m_radio_wgs_sigma);

#ifndef extended_f_test
    m_radio_wgs_sse->setHidden(true);
    m_radio_wgs_ssy->setHidden(true);
    m_radio_wgs_chi->setHidden(true);
    m_radio_wgs_sigma->setHidden(true);
#endif

    m_ParameterIndex = 0;
    m_error_name = "SSE";

    layout->addLayout(hlayout);

    m_gridScalingFactor = new QSpinBox;
    m_gridScalingFactor->setMinimum(-10);
    m_gridScalingFactor->setMaximum(10);
    m_gridScalingFactor->setValue(-4);
    m_gridScalingFactor->setToolTip(tr("Set the scaling factor for each step. The lower the value, the smaller the step and the more precise is the result. On the other hand, more steps are needed or \'Max SSE Convergency Counter\' has to be increased."));
    hlayout = new QHBoxLayout;
    hlayout->addWidget(new QLabel(tr("Scaling Factor for Single Steps:")));
    hlayout->addWidget(m_gridScalingFactor);

    m_wgs_steps = new QSpinBox;
    m_wgs_steps->setMaximum(1e7);
    m_wgs_steps->setValue(2e3);
    m_wgs_steps->setSingleStep(100);
    m_wgs_steps->setToolTip(tr("Define the maximal number of steps for the lower and upper limit seperatly."));
    hlayout->addWidget(new QLabel(tr("Maximal steps:")));
    hlayout->addWidget(m_wgs_steps);

    layout->addLayout(hlayout);

    hlayout = new QHBoxLayout;

    m_wgs_err_conv = new ScientificBox;
    m_wgs_err_conv->setDecimals(10);
    m_wgs_err_conv->setValue(1E-9);
    m_wgs_err_conv->setSingleStep(1e-9);
    m_wgs_err_conv->setToolTip(tr("Set the cutoff, where the difference between calculated SSE is considered as zero."));

    m_gridErrorConvergencyCounter = new QSpinBox;
    m_gridErrorConvergencyCounter->setRange(1, 1e8);
    m_gridErrorConvergencyCounter->setSingleStep(10);
    m_gridErrorConvergencyCounter->setValue(5);
    m_gridErrorConvergencyCounter->setToolTip(tr("Set the maximal number of zero change in SSE ( <  \'SSE Convergence\'), before the stopping."));

    hlayout->addWidget(new QLabel(tr("SSE Convergence:")));
    hlayout->addWidget(m_wgs_err_conv);
    hlayout->addWidget(new QLabel(tr("Max SSE Convergency Counter")));
    hlayout->addWidget(m_gridErrorConvergencyCounter);

    layout->addLayout(hlayout);

    hlayout = new QHBoxLayout;

    m_gridOvershotCounter = new QSpinBox;
    m_gridOvershotCounter->setRange(1, 1e8);
    m_gridOvershotCounter->setSingleStep(10);
    m_gridOvershotCounter->setValue(5);
    m_gridOvershotCounter->setToolTip(tr("Set the maximal number of allowed steps, where SSE > SSE(max). The procedure will stop after that number is reached."));
    m_gridErrorDecreaseCounter = new QSpinBox;
    m_gridErrorDecreaseCounter->setRange(1, 1e8);
    m_gridErrorDecreaseCounter->setSingleStep(10);
    m_gridErrorDecreaseCounter->setValue(50);
    m_gridErrorDecreaseCounter->setToolTip("Set the maximal number of allowed steps, where SSE < SSE(fit). The procedure will stop after that number is reached.");

    hlayout->addWidget(new QLabel(tr("Max SSE Overshot Counter:")));
    hlayout->addWidget(m_gridOvershotCounter);
    hlayout->addWidget(new QLabel(tr("Max SSE Decrease Counter:")));
    hlayout->addWidget(m_gridErrorDecreaseCounter);

    layout->addLayout(hlayout);

    hlayout = new QHBoxLayout;

    m_cv = new QPushButton(tr("Calculate"));
    layout->addWidget(m_cv);

    connect(m_cv, &QPushButton::clicked, this, [this]() {
        clearMessages();
        emit RunCalculation(RunGridSearch());
    });
    cv_widget->setLayout(layout);
    return cv_widget;
}

QWidget* StatisticDialog::ModelComparison()
{
    QWidget* mo_widget = new QWidget;
    QGridLayout* layout = new QGridLayout;

    QGroupBox* parameter = new QGroupBox(tr("Parameter"));
    if (m_model) {
        layout->addWidget(new QLabel(tr("Choose parameter to be tested:")));
        QVBoxLayout* layout = new QVBoxLayout;
        for (int i = 0; i < m_model.toStrongRef().data()->GlobalParameterSize(); ++i) {
            QCheckBox* checkbox = new QCheckBox;
            checkbox->setChecked(true);
            m_moco_global << checkbox;

            QDoubleSpinBox* doublespin = new QDoubleSpinBox;
            doublespin->setMinimum(0);
            doublespin->setMaximum(50);
            doublespin->setValue(1.5);
            doublespin->setToolTip(tr("Set the scaling factor for %1.").arg(m_model.toStrongRef().data()->GlobalParameterName(i)));
            m_glob_box_scaling << doublespin;

            QHBoxLayout* hlayout = new QHBoxLayout;
            hlayout->addWidget(checkbox);
            QLabel* label = new QLabel("<html>" + m_model.toStrongRef().data()->GlobalParameterName(i) + "</html>");
            label->setFixedWidth(100);
            hlayout->addWidget(label);
            hlayout->addWidget(doublespin);
            hlayout->addStretch(100);
            layout->addLayout(hlayout);

            connect(m_model.toStrongRef().data(), &AbstractModel::Recalculated, this, [this, i, checkbox]() {
                if (m_model) {
                    checkbox->setEnabled(m_model.toStrongRef().data()->GlobalEnabled(i));
                }
            });
        }

        for (int i = 0; i < m_model.toStrongRef().data()->LocalParameterSize(); ++i) {
            QCheckBox* checkbox = new QCheckBox;
            m_moco_local << checkbox;

            QDoubleSpinBox* doublespin = new QDoubleSpinBox;
            doublespin->setMinimum(0);
            doublespin->setMaximum(50);
            doublespin->setValue(1.50);
            doublespin->setToolTip(tr("Set the scaling factor for %1.").arg(m_model.toStrongRef().data()->LocalParameterName(i)));

            m_loc_box_scaling << doublespin;

            QHBoxLayout* hlayout = new QHBoxLayout;
            hlayout->addWidget(checkbox);
            QLabel* label = new QLabel("<html>" + m_model.toStrongRef().data()->LocalParameterName(i) + "</html>");
            label->setFixedWidth(100);
            hlayout->addWidget(label);
            hlayout->addWidget(doublespin);
            hlayout->addStretch(100);
            layout->addLayout(hlayout);

            connect(m_model.toStrongRef().data(), &AbstractModel::Recalculated, this, [this, i, checkbox]() {
                if (m_model) {
                    checkbox->setEnabled(m_model.toStrongRef().data()->LocalEnabled(i));
                }
            });
        }
        parameter->setLayout(layout);
    }
    layout->addWidget(parameter, 0, 0, 1, 3);

    m_moco_global_settings = new QGroupBox(tr("Settings"));
    QGridLayout* global_layout = new QGridLayout;

    m_moco_maxerror = new QDoubleSpinBox;
    m_moco_maxerror->setMaximum(100);
    m_moco_maxerror->setSingleStep(0.5);
    m_moco_maxerror->setValue(95);
    m_moco_maxerror->setDecimals(1);
    m_moco_maxerror->setSuffix("%");

    m_moco_f_value = new QDoubleSpinBox(this);
    m_moco_f_value->setMaximum(1000);
    m_moco_f_value->setMinimum(0);
    m_moco_f_value->setValue(m_f_value);
    m_moco_f_value->setDecimals(4);
    m_moco_f_value->setReadOnly(true);

    connect(m_moco_maxerror, SIGNAL(valueChanged(qreal)), this, SLOT(CalculateError()));

    QHBoxLayout* hlayout = new QHBoxLayout;

    m_radio_moco_sse = new QRadioButton(tr("SSE"));
    m_radio_moco_sse->setChecked(true);

    connect(m_radio_moco_sse, &QRadioButton::toggled, this, [this](int state) {
        if (state) {
            m_ParameterIndex = 0;
            m_error_name = m_radio_moco_sse->text();
            CalculateError();
        }
    });
    m_radio_moco_ssy = new QRadioButton(tr("SEy"));
    connect(m_radio_moco_ssy, &QRadioButton::toggled, this, [this](int state) {
        if (state) {
            m_ParameterIndex = 1;
            m_error_name = m_radio_moco_ssy->text();
            CalculateError();
        }
    });
    m_radio_moco_chi = new QRadioButton(tr("%1%2").arg(Unicode_chi).arg(Unicode_Sup_2));
    connect(m_radio_moco_chi, &QRadioButton::toggled, this, [this](int state) {
        if (state) {
            m_ParameterIndex = 2;
            m_error_name = m_radio_moco_chi->text();
            CalculateError();
        }
    });
    m_radio_moco_sigma = new QRadioButton(tr("%1").arg(Unicode_sigma));
    connect(m_radio_moco_sigma, &QRadioButton::toggled, this, [this](int state) {
        if (state) {
            m_ParameterIndex = 3;
            m_error_name = m_radio_moco_sigma->text();
            CalculateError();
        }
    });

    hlayout->addWidget(m_radio_moco_sse);
    hlayout->addWidget(m_radio_moco_ssy);
    hlayout->addWidget(m_radio_moco_chi);
    hlayout->addWidget(m_radio_moco_sigma);

    m_radio_moco_sse->setHidden(true);
    m_radio_moco_ssy->setHidden(true);
    m_radio_moco_chi->setHidden(true);
    m_radio_moco_sigma->setHidden(true);

    global_layout->addLayout(hlayout, 0, 0, 1, 2);

    global_layout->addWidget(new QLabel(tr("Confidence Interval")), 0, 0);
    global_layout->addWidget(m_moco_maxerror, 0, 1);

    if (m_model) {
        global_layout->addWidget(new QLabel(tr("F-Value:")), 1, 0);
        global_layout->addWidget(m_moco_f_value, 1, 1);
        m_moco_error_info = new QLabel;
        global_layout->addWidget(m_moco_error_info, 2, 0, 1, 3);
    } else
        m_moco_f_value->hide();

    m_moco_global_settings->setLayout(global_layout);
    layout->addWidget(m_moco_global_settings, 1, 0, 1, 3);
    m_moco_monte_carlo = new QGroupBox(tr("Monte Carlo Settings"));
    QGridLayout* monte_layout = new QGridLayout;

    m_moco_mc_steps = new QSpinBox;
    m_moco_mc_steps->setMaximum(1e7);
    m_moco_mc_steps->setValue(20000);
    m_moco_mc_steps->setSingleStep(100);

    monte_layout->addWidget(new QLabel(tr("Max. Steps")), 1, 0);
    monte_layout->addWidget(m_moco_mc_steps, 1, 1, 1, 2);
    m_moco_monte_carlo->setLayout(monte_layout);
    layout->addWidget(m_moco_monte_carlo, 2, 0, 1, 3);

    m_moco = new QPushButton(tr("Start ..."));
    layout->addWidget(m_moco, 5, 0, 1, 3);

    connect(m_moco, &QPushButton::clicked, this, [this]() {
        clearMessages();
        emit RunCalculation(RunModelComparison());
    });
    mo_widget->setLayout(layout);
    return mo_widget;
}

QWidget* StatisticDialog::CVWidget()
{
    QWidget* widget = new QWidget;
    QGridLayout* layout = new QGridLayout;
    m_radio_cv_loo = new QRadioButton(tr("Leave-One-Out"));
    m_radio_cv_l2o = new QRadioButton(tr("Leave-Two-Out"));
    m_radio_cv_lxo = new QRadioButton(tr("Leave-X-Out"));

    m_cv_lxo = new QSpinBox;
    m_cv_lxo->setMinimum(1);
    m_cv_lxo->setPrefix(tr("X = "));
    if (m_model) {
        m_cv_lxo->setMaximum(m_model.toStrongRef().data()->DataPoints() - 1);
        QLabel* max_runs = new QLabel;
        connect(m_cv_lxo, qOverload<int>(&QSpinBox::valueChanged), max_runs, [max_runs, this, layout](int value) {
            long double maxsteps = tgammal(m_model.toStrongRef().data()->DataPoints() + 1) / (tgammal(value + 1) * tgammal(m_model.toStrongRef().data()->DataPoints() - value + 1));

            max_runs->setText(tr("Maximal number of runs = %1").arg(QString::number(maxsteps, 'g')));
        });
        max_runs->setMaximumHeight(10);
        layout->addWidget(max_runs, 3, 1, 1, 2);
    }
    m_cv_lxo->setValue(3);

    m_cv_runs = new QSpinBox;
    m_cv_runs->setMinimum(1);
    m_cv_runs->setMaximum(1e10);
    m_cv_runs->setValue(1e4);
    m_cv_runs->setSuffix(tr(" # steps"));

    QGroupBox* cv_algo = new QGroupBox;

    m_cv_premap = new QRadioButton(tr("Precalculation"));
    m_cv_premap->setToolTip(tr("Precalculates the whole map systematically - more than # steps."));

    m_cv_automap = new QRadioButton(tr("Automatic"));
    m_cv_automap->setToolTip(tr("Automatically chooses between either full precalculation or random initialisation!"));

    m_cv_randmap = new QRadioButton(tr("Random initialisation"));
    m_cv_randmap->setToolTip(tr("Calculates only # steps of random LXO experiments!"));

    cv_algo->setMaximumHeight(100);
    QHBoxLayout* cv_algo_layout = new QHBoxLayout;
    cv_algo->setTitle(tr("Map Calculation Strategy in LXO"));
    cv_algo_layout->addWidget(m_cv_premap);
    cv_algo_layout->addWidget(m_cv_automap);
    cv_algo_layout->addWidget(m_cv_randmap);

    m_cv_automap->setChecked(true);

    cv_algo->setLayout(cv_algo_layout);

    m_radio_cv_loo->setChecked(true);

    layout->addWidget(m_radio_cv_loo, 0, 0);
    layout->addWidget(m_radio_cv_l2o, 1, 0);
    layout->addWidget(m_radio_cv_lxo, 2, 0);
    layout->addWidget(m_cv_lxo, 2, 1);
    layout->addWidget(m_cv_runs, 2, 2);
    if (qApp->instance()->property("advanced_ui").toBool())
        layout->addWidget(cv_algo, 4, 1, 1, 2);

    connect(m_cv_lxo, qOverload<int>(&QSpinBox::valueChanged), m_radio_cv_lxo, &QRadioButton::setChecked);
    connect(m_cv_runs, qOverload<int>(&QSpinBox::valueChanged), m_radio_cv_lxo, &QRadioButton::setChecked);

    m_cross_validate = new QPushButton(tr("Cross Validation"));
    m_reduction = new QPushButton(tr("Reduction Analysis"));

    layout->addWidget(m_cross_validate, 5, 0, 1, 3);
    layout->addWidget(m_reduction, 6, 0, 1, 3);
    connect(m_cross_validate, &QPushButton::clicked, this, [this]() {
        clearMessages();
        emit RunCalculation(RunCrossValidation());
    });
    connect(m_reduction, &QPushButton::clicked, this, [this]() {
        clearMessages();
        emit RunCalculation(RunReductionAnalyse());
    });
    widget->setLayout(layout);
    return widget;
}

void StatisticDialog::clearMessages()
{
    m_message_box->clear();
}

QJsonObject StatisticDialog::RunGridSearch() const
{
    QJsonObject controller = GridSearchConfigBlock;

    controller["MaxSteps"] = m_wgs_steps->value();
    controller["MaxParameter"] = m_wgs_max;
    controller["f-value"] = m_wgs_f_value->value();
    controller["Method"] = SupraFit::Method::WeakenedGridSearch;
    controller["confidence"] = m_wgs_maxerror->value();

    controller["ErrorConvergency"] = m_wgs_err_conv->value();
    controller["OverShotCounter"] = m_gridOvershotCounter->value();
    controller["ErrorDecreaseCounter"] = m_gridErrorDecreaseCounter->value();
    controller["ErrorConvergencyCounter"] = m_gridErrorConvergencyCounter->value();
    controller["StepScalingFactor"] = m_gridScalingFactor->value();

    controller["StoreRaw"] = m_store_wgsearch->isChecked();

    controller["ParameterIndex"] = m_ParameterIndex;

    QList<int> glob_param, local_param;
    int max = 0;
    for (int i = 0; i < m_grid_global.size(); ++i) {
        if (m_model.toStrongRef().data()->GlobalTable()->isChecked(i, 0))
            glob_param << (m_grid_global[i]->isChecked() * m_grid_global[i]->isEnabled());
        max += m_grid_global[i]->isChecked();
    }

    for (int i = 0; i < m_grid_local.size(); ++i) {
        for (int j = 0; j < m_model.toStrongRef().data()->SeriesCount(); ++j) {
            if (m_model.toStrongRef().data()->LocalTable()->isChecked(i, j))
                local_param << (m_grid_local[i]->isChecked() * m_grid_local[i]->isEnabled());
        }
        max += m_model.toStrongRef().data()->SeriesCount() * m_grid_local[i]->isChecked();
    }

    controller["GlobalParameterList"] = ToolSet::IntList2String(glob_param);
    controller["LocalParameterList"] = ToolSet::IntList2String(local_param);
    return controller;
}

QJsonObject StatisticDialog::RunModelComparison() const
{
    QJsonObject controller = ModelComparisonConfigBlock;

    controller["MaxSteps"] = m_moco_mc_steps->value();
    controller["MaxParameter"] = m_moco_max;
    controller["f-value"] = m_moco_f_value->value();
    controller["Method"] = SupraFit::Method::ModelComparison;
    controller["confidence"] = m_moco_maxerror->value();

    QVector<double> glob_box, local_box;
    QList<int> glob_param, local_param;
    int max = 0;
    for (int i = 0; i < m_moco_global.size(); ++i) {
        if (m_model.toStrongRef().data()->GlobalTable()->isChecked(i, 0))
            glob_param << (m_moco_global[i]->isChecked() * m_moco_global[i]->isEnabled());
        glob_box << m_glob_box_scaling[i]->value();
        max += m_moco_global[i]->isChecked();
    }

    for (int i = 0; i < m_moco_local.size(); ++i) {
        for (int j = 0; j < m_model.toStrongRef().data()->SeriesCount(); ++j) {
            if (m_model.toStrongRef().data()->LocalTable()->isChecked(i, j))
                local_param << (m_moco_local[i]->isChecked() * m_moco_local[i]->isEnabled());
            local_box << m_loc_box_scaling[i]->value();
        }
        max += m_model.toStrongRef().data()->SeriesCount() * m_moco_local[i]->isChecked();
    }
    controller["ParameterIndex"] = m_ParameterIndex;

    controller["GlobalParameterList"] = ToolSet::IntList2String(glob_param);
    controller["LocalParameterList"] = ToolSet::IntList2String(local_param);
    controller["GlobalParameterScalingList"] = ToolSet::DoubleVec2String(glob_box);
    controller["LocalParameterScalingList"] = ToolSet::DoubleVec2String(local_box);

    return controller;
}

QJsonObject StatisticDialog::RunMonteCarlo() const
{
    QJsonObject controller = MonteCarloConfigBlock;
    controller["MaxSteps"] = m_mc_steps->value();
    controller["Variance"] = m_varianz_box->value();

    if (m_mc_std->isChecked())
        controller["VarianceSource"] = 3;
    else if (m_mc_sey->isChecked())
        controller["VarianceSource"] = 2;
    else if (m_mc_user->isChecked())
        controller["VarianceSource"] = 1;
    else
        controller["VarianceSource"] = 4;

    controller["OriginalData"] = m_original->isChecked();
    controller["Method"] = SupraFit::Method::MonteCarlo;

    QVector<qreal> indep_variance;
    for (int i = 0; i < m_indepdent_checkboxes.size(); ++i) {
        if (m_indepdent_checkboxes[i]->isChecked())
            indep_variance << m_indepdent_variance[i]->value();
        else
            indep_variance << 0;
    }

    controller["IndependentRowVariance"] = ToolSet::DoubleVec2String(indep_variance);
    return controller;
}


QJsonObject StatisticDialog::RunReductionAnalyse() const
{
    QJsonObject controller = ResampleConfigBlock;

    controller["Method"] = SupraFit::Method::Reduction;
    return controller;
}

QJsonObject StatisticDialog::RunCrossValidation() const
{
    QJsonObject controller = ResampleConfigBlock;

    controller["Method"] = SupraFit::Method::CrossValidation;
    if (m_radio_cv_loo->isChecked())
        controller["CXO"] = 1;
    else if (m_radio_cv_l2o->isChecked())
        controller["CXO"] = 2;
    else {
        controller["CXO"] = 3;
        controller["X"] = m_cv_lxo->value();
        controller["MaxSteps"] = m_cv_runs->value();
    }
    if (m_cv_automap->isChecked())
        controller["Algorithm"] = 2;
    else if (m_cv_premap->isChecked())
        controller["Algorithm"] = 1;
    else
        controller["Algorithm"] = 3;

    return controller;
}

void StatisticDialog::MaximumSteps(int steps)
{
    m_progress->setMaximum(steps);
}

void StatisticDialog::MaximumMainSteps(int steps)
{
    m_main_steps = 0;
    m_main_progress->setMaximum(steps * 100);
    m_main_progress->setValue(0);
}

void StatisticDialog::IncrementProgress(int time)
{
    QMutexLocker locker(&mutex);
    if (m_hidden)
        ShowWidget();

    m_time += time;

    qint64 t0 = QDateTime::currentMSecsSinceEpoch();
    int val = m_progress->value() + 1;
    qreal aver = double(m_time) / val;
    int remain = double(m_progress->maximum() - val) * aver / qAbs(QThreadPool::globalInstance()->maxThreadCount()) / 1000;
    int sec = (t0 - m_time_0) / 1000;
    double msec = ((t0 - m_time_0) / 1000.0 - sec) * 1000;
    if (m_progress->maximum() == -1) {
        m_time_info->setText(tr("Nobody knows when this will end.\nBut you hold the door for %2 sec, %3 msec. .").arg(sec).arg(msec));
    } else {
        m_time_info->setText(tr("Remaining time approx: %1 sec., elapsed time: %2 sec, %3 msec .").arg(remain).arg(sec).arg(msec));
        m_progress->setValue(val);
        m_main_progress->setValue(m_main_steps * 100 + val / double(m_progress->maximum()) * 100);
    }
}

void StatisticDialog::IncrementMainProgress()
{
    m_main_steps++;
}

QString StatisticDialog::FOutput() const
{
    QString string;
    if (m_model) {
        string += "Parameter fitted:<b>" + QString::number(m_model.toStrongRef().data()->Parameter()) + "</b>\n";
        string += "Number of used Points:<b>" + QString::number(m_model.toStrongRef().data()->Points()) + "</b>\n";
        string += "Degrees of Freedom:<b>" + QString::number(m_model.toStrongRef().data()->Points() - m_model.toStrongRef().data()->Parameter()) + "</b>\n";
    }
    return string;
}

void StatisticDialog::ShowWidget()
{
    m_time = 0;
    m_time_0 = QDateTime::currentMSecsSinceEpoch();
    m_progress->setValue(0);
    m_hidden = false;
}

void StatisticDialog::HideWidget()
{
    clearMessages();
    hide();
}

void StatisticDialog::Update()
{
    if (m_mc_sey->isChecked())
        m_varianz_box->setValue(m_model.toStrongRef().data()->SEy());
    else if (m_mc_std->isChecked())
        m_varianz_box->setValue(m_model.toStrongRef().data()->StdDeviation());
    CalculateError();
}

void StatisticDialog::Message(const QString& str)
{
    m_message_box->setText(str);
}

void StatisticDialog::CalculateError()
{
    if (!m_model.toStrongRef().data())
        return;
    updateUI();
    qreal error = m_model.toStrongRef().data()->StatisticVector()[m_ParameterIndex];
    qreal max_moco_error, max_wgs_error;
    QString wgs_message, moco_message;

    max_wgs_error = error * (m_wgs_f_value->value() * m_model.toStrongRef().data()->Parameter() / (m_model.toStrongRef().data()->Points() - m_model.toStrongRef().data()->Parameter()) + 1);
    wgs_message = "The current " + m_error_name + " is " + QString::number(error) + ".\nThe maximum " + m_error_name + " will be " + QString::number(max_wgs_error) + ".";

    max_moco_error = error * (m_moco_f_value->value() * m_model.toStrongRef().data()->Parameter() / (m_model.toStrongRef().data()->Points() - m_model.toStrongRef().data()->Parameter()) + 1);
    moco_message = "The current " + m_error_name + " is " + QString::number(error) + ".\nThe maximum " + m_error_name + " will be " + QString::number(max_moco_error) + ".";

    m_moco_max = max_moco_error;
    m_wgs_max = max_wgs_error;

    m_wgs_error_info->setText(wgs_message);
    m_moco_error_info->setText(moco_message);
}

#include "statisticdialog.moc"

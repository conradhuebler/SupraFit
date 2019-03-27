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

#include "src/capabilities/modelcomparison.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/capabilities/reductionanalyse.h"
#include "src/capabilities/weakenedgridsearch.h"

#include "src/core/models.h"
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
    , m_runs(1)
    , m_hidden(false)
{
    setUi();
    HideWidget();
    connect(m_model.data(), SIGNAL(Recalculated()), this, SLOT(Update()));
}

StatisticDialog::StatisticDialog(QWidget* parent)
    : QDialog(parent)
{
    setUi();
    HideWidget();
}

StatisticDialog::~StatisticDialog()
{
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
    m_wgs_f_value->setValue(m_model.data()->finv(m_wgs_maxerror->value() / 100));
    m_moco_f_value->setValue(m_model.data()->finv(m_moco_maxerror->value() / 100));
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
    m_progress = new QProgressBar;
    layout->addWidget(m_time_info);

    m_use_checked = new QCheckBox(tr("Consider Only Checked Tabs."));
    m_use_checked->setChecked(false);

    if (!m_model)
        layout->addWidget(m_use_checked);

    m_hide = new QPushButton(tr("Hide"));
    m_interrupt = new QPushButton(tr("Interrupt"));
    QHBoxLayout* hLayout = new QHBoxLayout;
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
    setMinimumWidth(1.5 * m_tab_widget->sizeHint().width());
}

QWidget* StatisticDialog::MonteCarloWidget()
{
    QWidget* mc_widget = new QWidget;
    QGridLayout* layout = new QGridLayout;

    m_mc_steps = new QSpinBox;
    m_mc_steps->setMinimum(2);
    m_mc_steps->setMaximum(1e9);
    m_mc_steps->setValue(1000);
    m_mc_steps->setSingleStep(1e2);
    layout->addWidget(new QLabel(tr("Number of MC Steps:")), 0, 0);
    layout->addWidget(m_mc_steps, 0, 1);

    m_mc_std = new QRadioButton; //(tr("<html>Input &sigma;</html>"));
    m_mc_std->setMaximumWidth(30);
    m_mc_sey = new QRadioButton; //(tr("<html>SE<sub>y</sub>"));
    m_mc_sey->setMaximumWidth(30);
    m_mc_user = new QRadioButton; //(tr("<html>User defined &sigma;</html>"));
    m_mc_user->setMaximumWidth(30);

    m_varianz_box = new QDoubleSpinBox;
    m_varianz_box->setDecimals(6);
    m_varianz_box->setSingleStep(1e-2);
    m_varianz_box->setMaximum(1e10);

    layout->addWidget(new QLabel(tr("<html>Standard Deviation &sigma;</html>")), 1, 0);
    layout->addWidget(m_varianz_box, 1, 1);

    QHBoxLayout* hlayout = new QHBoxLayout;

    hlayout->addWidget(new QLabel(tr("is taken as:")));

    hlayout->addWidget(m_mc_sey);
    QLabel* txt = new QLabel(tr("<html>SE<sub>y</sub> from Model</html>"));
    hlayout->addWidget(txt);


    hlayout->addWidget(m_mc_std);
    txt = new QLabel(tr("<html>&sigma; from Model"));
    hlayout->addWidget(txt);

    hlayout->addWidget(m_mc_user);
    txt = new QLabel(tr("<html>manually defined &sigma;</html>"));
    hlayout->addWidget(txt);

    m_mc_sey->setChecked(true);
    m_varianz_box->setReadOnly(!m_mc_user->isChecked());
    if (m_model)
        m_varianz_box->setValue(m_model.data()->SEy());

    connect(m_mc_std, &QRadioButton::toggled, m_mc_std, [this]() {
        if (m_mc_std->isChecked() && m_model) {
            m_varianz_box->setValue(m_model.data()->StdDeviation());
            m_varianz_box->setReadOnly(!m_mc_user->isChecked());
        }
    });

    connect(m_mc_sey, &QRadioButton::toggled, m_mc_sey, [this]() {
        if (m_mc_sey->isChecked() && m_model) {
            m_varianz_box->setValue(m_model.data()->SEy());
            m_varianz_box->setReadOnly(!m_mc_user->isChecked());
        }
    });

    connect(m_mc_user, &QRadioButton::toggled, m_mc_user, [this]() {
        if (m_mc_user->isChecked()) {
            m_varianz_box->setReadOnly(!m_mc_user->isChecked());
            m_varianz_box->setValue(1e-3);
        }
    });

    layout->addLayout(hlayout, 2, 0, 1, 2);

    m_original = new QCheckBox;
    m_original->setText(tr("Use Original Data"));
    layout->addWidget(m_original, 3, 0);

    m_bootstrap = new QCheckBox;
    m_bootstrap->setText(tr("Bootstrap"));
    layout->addWidget(m_bootstrap, 3, 1);

    QVBoxLayout* indep_layout = new QVBoxLayout;
    if (m_model.data()) {
        QStringList header = m_model.data()->IndependentModel()->header();

        indep_layout->addWidget(new QLabel(tr("Create random scatter of independent\nvariables, eg. input concentrations etc!")));

        for (int i = 0; i < m_model.data()->IndependentVariableSize(); ++i) {

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
        //emit MCStatistic(getMCConfig());
        emit RunCalculation(RunMonteCarlo());
    });

    connect(m_bootstrap, SIGNAL(stateChanged(int)), this, SLOT(EnableWidgets()));
    mc_widget->setLayout(layout);
    return mc_widget;
}

QWidget* StatisticDialog::GridSearchWidget()
{
    WGSConfig config;

    QWidget* cv_widget = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout;

    QWidget* parameter = new QWidget(this);
    if (m_model) {
        layout->addWidget(new QLabel(tr("Choose parameter to be tested:")));
        QVBoxLayout* layout = new QVBoxLayout;
        for (int i = 0; i < m_model.data()->GlobalParameterSize(); ++i) {
            QCheckBox* checkbox = new QCheckBox;
            checkbox->setChecked(true);
            m_grid_global << checkbox;
            QHBoxLayout* hlayout = new QHBoxLayout;
            hlayout->addWidget(checkbox);
            hlayout->addWidget(new QLabel("<html>" + m_model.data()->GlobalParameterName(i) + "</html>"));
            hlayout->addStretch(100);
            layout->addLayout(hlayout);
        }

        for (int i = 0; i < m_model.data()->LocalParameterSize(); ++i) {
            QCheckBox* checkbox = new QCheckBox;
            m_grid_local << checkbox;
            QHBoxLayout* hlayout = new QHBoxLayout;
            hlayout->addWidget(checkbox);
            hlayout->addWidget(new QLabel("<html>" + m_model.data()->LocalParameterName(i) + "</html>"));
            hlayout->addStretch(100);
            layout->addLayout(hlayout);
        }
        parameter->setLayout(layout);
    }
    layout->addWidget(parameter);

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
    m_store_wgsearch = new QCheckBox(tr("Store intermediate Models"));
    m_store_wgsearch->setToolTip(tr("If checked, SupraFit will try to store ALL intermedate results. They are necessary to compute the confidence range for the single parameters \nAND dervied values, such as entropy. The downside are hugh files and sometimes data, that is to large to be handled resulting in an empty output!"));
    layout->addWidget(m_store_wgsearch);

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

    m_gridScalingFactor = new QSpinBox;
    m_gridScalingFactor->setMinimum(-10);
    m_gridScalingFactor->setMaximum(10);
    m_gridScalingFactor->setValue(config.ScalingFactor);

    hlayout = new QHBoxLayout;
    hlayout->addWidget(new QLabel(tr("Scaling Factor for Single Steps:")));
    hlayout->addWidget(m_gridScalingFactor);

    m_wgs_steps = new QSpinBox;
    m_wgs_steps->setMaximum(1e7);
    m_wgs_steps->setValue(2e3);
    m_wgs_steps->setSingleStep(100);

    hlayout->addWidget(new QLabel(tr("Maximal steps:")));
    hlayout->addWidget(m_wgs_steps);

    layout->addLayout(hlayout);

    hlayout = new QHBoxLayout;

    m_wgs_err_conv = new ScientificBox;
    m_wgs_err_conv->setDecimals(10);
    m_wgs_err_conv->setValue(1E-9);
    m_wgs_err_conv->setSingleStep(1e-9);

    m_gridErrorConvergencyCounter = new QSpinBox;
    m_gridErrorConvergencyCounter->setRange(1, 1e8);
    m_gridErrorConvergencyCounter->setSingleStep(10);
    m_gridErrorConvergencyCounter->setValue(config.ErrorConvergencyCounter);

    hlayout->addWidget(new QLabel(tr("SSE Convergence:")));
    hlayout->addWidget(m_wgs_err_conv);
    hlayout->addWidget(new QLabel(tr("Max SSE Convergency Counter")));
    hlayout->addWidget(m_gridErrorConvergencyCounter);

    layout->addLayout(hlayout);

    hlayout = new QHBoxLayout;

    m_gridOvershotCounter = new QSpinBox;
    m_gridOvershotCounter->setRange(1, 1e8);
    m_gridOvershotCounter->setSingleStep(10);
    m_gridOvershotCounter->setValue(config.OvershotCounter);

    m_gridErrorDecreaseCounter = new QSpinBox;
    m_gridErrorDecreaseCounter->setRange(1, 1e8);
    m_gridErrorDecreaseCounter->setSingleStep(10);
    m_gridErrorDecreaseCounter->setValue(config.ErrorDecreaseCounter);

    hlayout->addWidget(new QLabel(tr("Max SSE Overshot Counter:")));
    hlayout->addWidget(m_gridOvershotCounter);
    hlayout->addWidget(new QLabel(tr("Max SSE Decrease Counter:")));
    hlayout->addWidget(m_gridErrorDecreaseCounter);

    layout->addLayout(hlayout);

    hlayout = new QHBoxLayout;

    m_cv = new QPushButton(tr("Calculate"));
    layout->addWidget(m_cv);

    connect(m_cv, &QPushButton::clicked, this, [this]() {
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
        for (int i = 0; i < m_model.data()->GlobalParameterSize(); ++i) {
            QCheckBox* checkbox = new QCheckBox;
            checkbox->setChecked(true);
            m_moco_global << checkbox;
            QHBoxLayout* hlayout = new QHBoxLayout;
            hlayout->addWidget(checkbox);
            hlayout->addWidget(new QLabel("<html>" + m_model.data()->GlobalParameterName(i) + "</html>"));
            hlayout->addStretch(100);
            layout->addLayout(hlayout);
        }

        for (int i = 0; i < m_model.data()->LocalParameterSize(); ++i) {
            QCheckBox* checkbox = new QCheckBox;
            m_moco_local << checkbox;
            QHBoxLayout* hlayout = new QHBoxLayout;
            hlayout->addWidget(checkbox);
            hlayout->addWidget(new QLabel("<html>" + m_model.data()->LocalParameterName(i) + "</html>"));
            hlayout->addStretch(100);
            layout->addLayout(hlayout);
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

    global_layout->addWidget(new QLabel(tr("Confidence Intervall")), 0, 0);
    global_layout->addWidget(m_moco_maxerror, 0, 1);

    if (m_model) {
        global_layout->addWidget(new QLabel(tr("F-Value:")), 1, 0);
        global_layout->addWidget(m_moco_f_value, 1, 1);
        m_moco_error_info = new QLabel;
        global_layout->addWidget(m_moco_error_info, 2, 0, 1, 3);
    } else
        m_moco_f_value->hide();

    m_moco_box_multi = new QDoubleSpinBox;
    m_moco_box_multi->setMaximum(1000);
    m_moco_box_multi->setSingleStep(0.5);
    m_moco_box_multi->setValue(4);
    m_moco_box_multi->setDecimals(2);
    global_layout->addWidget(new QLabel(tr("Box Scaling")), 3, 0);
    global_layout->addWidget(m_moco_box_multi, 3, 1);

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
        emit RunCalculation(RunModelComparison());
    });
    mo_widget->setLayout(layout);
    return mo_widget;
}

QWidget* StatisticDialog::CVWidget()
{
    QWidget* widget = new QWidget;
    QGridLayout* layout = new QGridLayout;
    m_wgs_loo = new QRadioButton(tr("Leave-One-Out"));
    m_wgs_l2o = new QRadioButton(tr("Leave-Two-Out"));
    m_wgs_loo->setChecked(true);
    layout->addWidget(m_wgs_loo, 0, 0);
    layout->addWidget(m_wgs_l2o, 1, 0);
    m_cross_validate = new QPushButton(tr("Cross Validation"));
    m_reduction = new QPushButton(tr("Reduction Analysis"));

    layout->addWidget(m_cross_validate, 2, 0);
    layout->addWidget(m_reduction, 3, 0);
    connect(m_cross_validate, &QPushButton::clicked, this, [this]() {
        emit RunCalculation(RunCrossValidation());
    });
    connect(m_reduction, &QPushButton::clicked, this, &StatisticDialog::Reduction);

    widget->setLayout(layout);
    return widget;
}

QJsonObject StatisticDialog::RunGridSearch() const
{
    QJsonObject controller;

    controller["MaxSteps"] = m_wgs_steps->value();
    //controller["increment"] = m_config.increment;
    controller["maxerror"] = m_wgs_max;
    controller["f-value"] = m_wgs_f_value->value();
    controller["method"] = SupraFit::Statistic::WeakenedGridSearch;
    controller["confidence"] = m_wgs_maxerror->value();

    controller["ErrorConvergency"] = m_wgs_err_conv->value();
    controller["OverShotCounter"] = m_gridOvershotCounter->value();
    controller["ErrorDecreaseCounter"] = m_gridErrorDecreaseCounter->value();
    controller["ErrorConvergencyCounter"] = m_gridErrorConvergencyCounter->value();
    controller["StepScalingFactor"] = m_gridScalingFactor->value();

    controller["StoreIntermediate"] = m_store_wgsearch->isChecked();

    QList<int> glob_param, local_param;
    int max = 0;
    for (int i = 0; i < m_grid_global.size(); ++i) {
        glob_param << m_grid_global[i]->isChecked();
        max += m_grid_global[i]->isChecked();
    }

    for (int i = 0; i < m_grid_local.size(); ++i) {
        local_param << m_grid_local[i]->isChecked();
        max += m_model.data()->SeriesCount() * m_grid_local[i]->isChecked();
    }

    controller["GlobalParameterList"] = ToolSet::IntList2String(glob_param);
    controller["LocalParameterList"] = ToolSet::IntList2String(local_param);
    /*  
    m_time = 0;
    m_time_0 = QDateTime::currentMSecsSinceEpoch();
    m_progress->setMaximum(2 * max);
    m_progress->setValue(0);
    
    ShowWidget();
*/
    return controller;
}

WGSConfig StatisticDialog::getWGSConfig()
{
    WGSConfig config;

    return config;
}

QJsonObject StatisticDialog::RunModelComparison() const
{
    QJsonObject controller;

    controller["MaxSteps"] = m_moco_mc_steps->value();
    controller["maxerror"] = m_moco_max;
    controller["f-value"] = m_moco_f_value->value();
    controller["method"] = SupraFit::Statistic::ModelComparison;
    controller["confidence"] = m_moco_maxerror->value();

    controller["BoxMultiplier"] = m_moco_box_multi->value();

    QList<int> glob_param, local_param;
    int max = 0;
    for (int i = 0; i < m_moco_global.size(); ++i) {
        glob_param << m_moco_global[i]->isChecked();
        max += m_moco_global[i]->isChecked();
    }

    for (int i = 0; i < m_moco_local.size(); ++i) {
        for (int j = 0; j < m_model.data()->SeriesCount(); ++j)
            local_param << m_moco_local[i]->isChecked();
        max += m_model.data()->SeriesCount() * m_moco_local[i]->isChecked();
    }

    controller["GlobalParameterList"] = ToolSet::IntList2String(glob_param);
    controller["LocalParameterList"] = ToolSet::IntList2String(local_param);
    /*
    m_time = 0;
    m_time_0 = QDateTime::currentMSecsSinceEpoch();
    m_progress->setMaximum(m_runs * (1 + m_moco_mc_steps->value() / update_intervall));
    m_progress->setValue(0);
    ShowWidget();
    */
    return controller;
}

MoCoConfig StatisticDialog::getMoCoConfig()
{
    MoCoConfig config;
    WGSConfig cv_config;
    cv_config.relax = true;
    config.cv_config = cv_config;

    return config;
}

QJsonObject StatisticDialog::RunMonteCarlo() const
{
    QJsonObject controller;
    controller["MaxSteps"] = m_mc_steps->value();
    controller["Variance"] = m_varianz_box->value();

    if (m_mc_std->isChecked())
        controller["VarianceSource"] = "sigma";
    else if (m_mc_sey->isChecked())
        controller["VarianceSource"] = "SEy";
    else
        controller["VarianceSource"] = "UserDefined";

    controller["OriginalError"] = m_original->isChecked();
    controller["Bootstrap"] = m_bootstrap->isChecked();
    controller["method"] = SupraFit::Statistic::MonteCarlo;

    QVector<qreal> indep_variance;
    for (int i = 0; i < m_indepdent_checkboxes.size(); ++i) {
        if (m_indepdent_checkboxes[i]->isChecked())
            indep_variance << m_indepdent_variance[i]->value();
        else
            indep_variance << 0;
    }

    controller["IndependentRowVariance"] = ToolSet::DoubleVec2String(indep_variance);

    /*    
    m_time = 0;
    m_time_0 = QDateTime::currentMSecsSinceEpoch();
    m_progress->setMaximum(m_runs * (m_mc_steps->value() + m_mc_steps->value() / 100));
    m_progress->setValue(0);
    ShowWidget();
    */
    return controller;
}

MCConfig StatisticDialog::getMCConfig()
{
    MCConfig config;
    config.variance = m_varianz_box->value();
    config.maxsteps = m_mc_steps->value();
    config.original = m_original->isChecked();
    config.bootstrap = m_bootstrap->isChecked();
    QVector<qreal> indep_variance;
    for (int i = 0; i < m_indepdent_checkboxes.size(); ++i) {
        if (m_indepdent_checkboxes[i]->isChecked())
            indep_variance << m_indepdent_variance[i]->value();
        else
            indep_variance << 0;
    }
    config.indep_variance = indep_variance;

    return config;
}

QJsonObject StatisticDialog::RunReductionAnalyse() const
{
    QJsonObject controller;

    controller["method"] = SupraFit::Statistic::Reduction;
    /*
    m_time = 0;
    m_time_0 = QDateTime::currentMSecsSinceEpoch();
    m_progress->setMaximum(0);
    m_progress->setValue(0);
    ShowWidget();
    */
    return controller;
}

QJsonObject StatisticDialog::RunCrossValidation() const
{
    QJsonObject controller;

    controller["method"] = SupraFit::Statistic::CrossValidation;
    if (m_wgs_loo->isChecked())
        controller["CVType"] = ReductionAnalyse::LeaveOneOut;
    else
        controller["CVType"] = ReductionAnalyse::LeaveTwoOut;

    /*
    m_time = 0;
    m_time_0 = QDateTime::currentMSecsSinceEpoch();
    m_progress->setMaximum(0);
    m_progress->setValue(0);
    ShowWidget();
    */
    return controller;
}

ReductionAnalyse::CVType StatisticDialog::CrossValidationType()
{

    if (m_wgs_loo->isChecked())
        return ReductionAnalyse::LeaveOneOut;
    else //if (m_wgs_l2o->isChecked())
        return ReductionAnalyse::LeaveTwoOut;
}

void StatisticDialog::MaximumSteps(int steps)
{
    if (m_hidden)
        ShowWidget();
    m_time = 0;
    m_time_0 = QDateTime::currentMSecsSinceEpoch();
    m_progress->setValue(0);
    m_progress->setMaximum(steps);
}

void StatisticDialog::IncrementProgress(int time)
{
    QMutexLocker locker(&mutex);
    if (m_hidden)
        ShowWidget();

    QTime timer(0,0);

    m_time += time;
    timer = timer.addMSecs(m_time);

    qint64 t0 = QDateTime::currentMSecsSinceEpoch();
    int val = m_progress->value() + 1;
    qreal aver = double(m_time) / val;
    int remain = double(m_progress->maximum() - val) * aver / qAbs(QThreadPool::globalInstance()->maxThreadCount()) / 1000;
    int used = (t0 - m_time_0) / 1000;

    if (m_progress->maximum() == -1) {
        m_time_info->setText(tr("Nobody knows when this will end.\nBut you hold the door for %2 sec. .").arg(used));
    } else {
        m_time_info->setText(tr("Remaining time approx: %1 sec., elapsed time: %2 . .").arg(remain).arg(timer.toString("mm:ss.zzz")));
        m_progress->setValue(val);
    }
}

QString StatisticDialog::FOutput() const
{
    QString string;
    if (m_model) {
        string += "Parameter fitted:<b>" + QString::number(m_model.data()->Parameter()) + "</b>\n";
        string += "Number of used Points:<b>" + QString::number(m_model.data()->Points()) + "</b>\n";
        string += "Degrees of Freedom:<b>" + QString::number(m_model.data()->Points() - m_model.data()->Parameter()) + "</b>\n";
    }
    return string;
}

void StatisticDialog::ShowWidget()
{
    return;
    QPropertyAnimation* animation = new QPropertyAnimation(m_hide_widget, "maximumHeight");
    animation->setEasingCurve(QEasingCurve::InOutCubic);
    animation->setDuration(200);
    animation->setStartValue(0);
    animation->setEndValue(100);
    animation->start();
    m_tab_widget->setDisabled(true);
    m_hidden = false;
    delete animation;
}

void StatisticDialog::HideWidget()
{
    hide();

    return;
    QPropertyAnimation* animation = new QPropertyAnimation(m_hide_widget, "maximumHeight");
    animation->setEasingCurve(QEasingCurve::InOutCubic);
    animation->setDuration(200);
    animation->setStartValue(100);
    animation->setEndValue(0);
    animation->start();
    m_progress->setMaximum(0);
    m_progress->setMinimum(0);
    m_tab_widget->setDisabled(false);
    m_hidden = true;
    delete animation;
}

void StatisticDialog::Update()
{
    if (m_mc_sey->isChecked())
        m_varianz_box->setValue(m_model.data()->SEy());
    else if (m_mc_std->isChecked())
        m_varianz_box->setValue(m_model.data()->StdDeviation());
    CalculateError();
}

void StatisticDialog::EnableWidgets()
{
    m_varianz_box->setEnabled(!m_bootstrap->isChecked());
}

void StatisticDialog::CalculateError()
{
    if (!m_model.data())
        return;
    updateUI();
    qreal error = m_model.data()->SumofSquares();
    qreal max_moco_error, max_wgs_error;
    QString wgs_message, moco_message;

    max_wgs_error = error * (m_wgs_f_value->value() * m_model.data()->Parameter() / (m_model.data()->Points() - m_model.data()->Parameter()) + 1);
    wgs_message = "The current error is " + QString::number(error) + ".\nThe maximum error will be " + QString::number(max_wgs_error) + ".";

    max_moco_error = error * (m_moco_f_value->value() * m_model.data()->Parameter() / (m_model.data()->Points() - m_model.data()->Parameter()) + 1);
    moco_message = "The current error is " + QString::number(error) + ".\nThe maximum error will be " + QString::number(max_moco_error) + ".";

    m_moco_max = max_moco_error;
    m_wgs_max = max_wgs_error;

    m_wgs_error_info->setText(wgs_message);
    m_moco_error_info->setText(moco_message);
}

#include "statisticdialog.moc"

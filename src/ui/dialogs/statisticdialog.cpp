/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/ui/guitools/flowlayout.h"
#include "src/ui/guitools/morelesstoggle.h"
#include "src/ui/widgets/analysisparts.h"
#include "src/ui/guitools/waiter.h"
#include "src/ui/widgets/buttons/scientificbox.h"

#include <QtCore/QDateTime>
#include <QtCore/QMutexLocker>
#include <QtCore/QThreadPool>
#include <QtCore/QTimer>

#include <QPropertyAnimation>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabBar>
#include <QtWidgets/QTabWidget>

#include "src/ui/instance.h"
#include "src/ui/guitools/windowhost.h"
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

void StatisticDialog::setEmbedded(bool embedded)
{
    m_embedded = embedded;
    if (embedded) {
        setWindowFlags(Qt::Widget);
        setMinimumWidth(0);
        m_tab_widget->tabBar()->hide(); // the pane lists the analyses
        m_tab_widget->setDocumentMode(true); // no pane frame around the forms either
        layout()->setContentsMargins(0, 0, 0, 0); // the page area already has its own margins
        m_hide->hide(); // there is nothing to hide: the pane's page list does the navigation
        m_hide_widget->hide(); // progress and Interrupt only while a job runs
    }
}

QStringList StatisticDialog::TabTitles() const
{
    QStringList titles;
    for (int i = 0; i < m_tab_widget->count(); ++i)
        titles << m_tab_widget->tabText(i);
    return titles;
}

void StatisticDialog::setCurrentTab(int index)
{
    m_tab_widget->setCurrentIndex(index);
}

void StatisticDialog::reject()
{
    if (!m_embedded)
        QDialog::reject(); // Escape must not blank an embedded page
}

void StatisticDialog::Attention()
{
    if (m_embedded) {
        emit AttentionRequested();
        return;
    }
    WindowHost::Show(this, QStringLiteral("statistics"), parentWidget());
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
    updateLandscape();
}

/*! \brief Sampling needs variable projection: say so instead of offering a button that cannot work. Claude Generated. */
void StatisticDialog::updateLandscape()
{
    if (!m_model || !m_ls_run)
        return;
    const bool usable = m_model.toStrongRef().data()->SupportsVarPro();
    m_ls_run->setEnabled(usable);
    m_ls_note->setVisible(!usable);
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
    if (m_model) // the project-wide dialog has no single model to sample
        m_tab_widget->addTab(LandscapeWidget(), tr("Landscape Sampling"));

    layout->addWidget(m_tab_widget);
    m_time_info = new QLabel;
    m_message_box = new QLabel;
    m_message_box->setWordWrap(true); // a stage text must not widen the pane on a small screen
    m_time_info->setWordWrap(true);
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
    connect(this, &StatisticDialog::setCurrentProgress, m_progress, &QProgressBar::setValue);
    connect(this, &StatisticDialog::setCurrentProgress, m_main_progress, &QProgressBar::setValue);

    setLayout(layout);
    updateUI();
    CalculateError();
    setWindowTitle("Statistic Tools");
    setMinimumWidth(1.5 * m_tab_widget->sizeHint().width());
}

/*! \brief Tab of the Monte Carlo simulation. In sight: the number of steps, where the noise comes from (and the standard deviation it
 * gives), whether the original data are used; behind "..more.." the scatter of the independent variables. Claude Generated. */
QWidget* StatisticDialog::MonteCarloWidget()
{
    QWidget* widget = new QWidget;
    QVBoxLayout* top = new QVBoxLayout;
    QFormLayout* form = new QFormLayout;
    form->setContentsMargins(0, 0, 0, 0);

    m_mc_steps = new QSpinBox;
    m_mc_steps->setMinimum(2);
    m_mc_steps->setMaximum(1e9);
    m_mc_steps->setValue(2000);
    m_mc_steps->setSingleStep(1e2);
    form->addRow(tr("Number of MC steps:"), m_mc_steps);

    // where the noise comes from; the data of the entries are the VarianceSource of the job
    m_mc_noise = new QComboBox;
    m_mc_noise->addItem(tr("SE%1 of the fit").arg(Unicode_Sub_y), 2);
    m_mc_noise->addItem(tr("%1 of the fit").arg(Unicode_sigma), 3);
    m_mc_noise->addItem(tr("%1 from user input").arg(Unicode_sigma), 1);
    m_mc_noise->addItem(tr("Bootstrapping"), 4);
    m_mc_noise->setItemData(3, tr("Using bootstrapping, no new errors will generated but rather the old error will randomly distributed on the model."), Qt::ToolTipRole);
    form->addRow(tr("Noise:"), m_mc_noise);

    m_varianz_box = new QDoubleSpinBox;
    m_varianz_box->setDecimals(6);
    m_varianz_box->setSingleStep(1e-2);
    m_varianz_box->setMaximum(1e10);
    m_varianz_box->setToolTip(tr("The standard deviation the noise has: given by the fit, or entered when it comes from user input."));
    form->addRow(tr("Standard deviation %1:").arg(Unicode_sigma), m_varianz_box);
    top->addLayout(form);

    m_original = new QCheckBox(tr("Use original data"));
    top->addWidget(m_original);

    // the value belongs to the source, only the user input can be edited, bootstrapping needs none
    auto noiseChanged = [this]() {
        const int source = m_mc_noise->currentData().toInt();
        const QSharedPointer<AbstractModel> model = m_model.toStrongRef();
        m_varianz_box->setReadOnly(source != 1);
        m_varianz_box->setEnabled(source != 4);
        if (source == 1)
            m_varianz_box->setValue(1e-3);
        else if (model && source == 2)
            m_varianz_box->setValue(model->SEy());
        else if (model && source == 3)
            m_varianz_box->setValue(model->StdDeviation());
    };
    connect(m_mc_noise, &QComboBox::currentIndexChanged, this, noiseChanged);
    noiseChanged();

    if (m_model && m_model.toStrongRef()->IndependentVariableSize() > 0) {
        QWidget* more = new QWidget;
        QVBoxLayout* inner = new QVBoxLayout;
        inner->setContentsMargins(0, 0, 0, 0);
        const QStringList header = m_model.toStrongRef()->IndependentModel()->header();
        inner->addWidget(new QLabel(tr("Create random scatter of independent variables, e.g. input concentrations:")));
        for (int i = 0; i < m_model.toStrongRef()->IndependentVariableSize(); ++i) {
            QCheckBox* checkbox = new QCheckBox(header[i]);
            QDoubleSpinBox* var = new QDoubleSpinBox;
            var->setDecimals(5);
            var->setSingleStep(1e-4);
            var->setDisabled(true);
            connect(checkbox, &QCheckBox::stateChanged, var, &QDoubleSpinBox::setEnabled);
            QHBoxLayout* row = new QHBoxLayout;
            row->addWidget(checkbox);
            row->addWidget(var, 1);
            m_indepdent_checkboxes << checkbox;
            m_indepdent_variance << var;
            inner->addLayout(row);
        }
        more->setLayout(inner);
        AddFolded(top, more, QStringLiteral("mc_more"));
    }

    m_mc = new QPushButton(tr("Simulate"));
    top->addWidget(m_mc);
    top->addStretch(1);
    connect(m_mc, &QPushButton::clicked, this, [this]() {
        clearMessages();
        emit RunCalculation(RunMonteCarlo());
    });
    widget->setLayout(top);
    return widget;
}

/*! \brief Tab of the landscape sampling: confidence region of every constant from the sum of squares itself, and other
 * minima. The few things one usually sets are in sight, everything about the sampler's steps, its stopping and the bias is
 * behind "..more.."; what only Langevin uses is shown only for Langevin. Claude Generated. */
QWidget* StatisticDialog::LandscapeWidget()
{
    QWidget* widget = new QWidget;
    QVBoxLayout* top = new QVBoxLayout;
    QGridLayout* layout = new QGridLayout;

    QLabel* info = new QLabel;
    info->setWordWrap(true);
    top->addWidget(info);

    m_ls_task = new QComboBox;
    m_ls_task->addItem(tr("Confidence intervals"), QStringLiteral("Confidence"));
    m_ls_task->addItem(tr("Find other minima"), QStringLiteral("Minima"));
    m_ls_task->setToolTip(tr("Confidence intervals: samples the sum of squares around the fit and reads the interval of every constant from it. "
                             "Find other minima: lets the walkers climb out of the basin of the fit, up to a multiple of its sum of squares, and "
                             "refits the minima they find."));
    layout->addWidget(new QLabel(tr("Task:")), 0, 0);
    layout->addWidget(m_ls_task, 0, 1);

    QLabel* confidenceLabel = new QLabel(tr("Confidence:"));
    m_ls_confidence = new QDoubleSpinBox;
    m_ls_confidence->setRange(50, 99.9);
    m_ls_confidence->setDecimals(1);
    m_ls_confidence->setValue(95);
    m_ls_confidence->setSuffix(" %");
    layout->addWidget(confidenceLabel, 1, 0);
    layout->addWidget(m_ls_confidence, 1, 1);

    m_ls_steps = new QSpinBox;
    m_ls_steps->setRange(1000, 10000000);
    m_ls_steps->setSingleStep(10000);
    m_ls_steps->setValue(50000);
    m_ls_steps->setToolTip(tr("Steps of one walker; it stops earlier when it finds nothing new."));
    layout->addWidget(new QLabel(tr("Steps per walker:")), 2, 0);
    layout->addWidget(m_ls_steps, 2, 1);

    m_ls_sampler = new QComboBox;
    m_ls_sampler->addItem(tr("Metropolis"), QStringLiteral("Metropolis"));
    m_ls_sampler->addItem(tr("Langevin"), QStringLiteral("Langevin"));
    m_ls_sampler->setToolTip(tr("Metropolis needs the sum of squares only; Langevin follows its numerical gradient."));
    layout->addWidget(new QLabel(tr("Sampler:")), 3, 0);
    layout->addWidget(m_ls_sampler, 3, 1);

    m_ls_surface = new QCheckBox(tr("2D surfaces of the pairs"));
    m_ls_surface->setToolTip(tr("Records the points of the walk and offers the sum of squares over every pair of constants (up to six pairs) as height "
                               "lines on the result page: the shape of the confidence region and the correlation of the constants."));
    layout->addWidget(m_ls_surface, 4, 0, 1, 2);
    top->addLayout(layout);

    // everything else, folded away
    QWidget* more = new QWidget;
    more->setObjectName(QStringLiteral("landscape_more"));
    QGridLayout* grid = new QGridLayout;
    grid->setContentsMargins(0, 0, 0, 0);
    QList<QWidget*> langevinOnly;
    int row = 0;
    auto addRow = [&](const QString& label, QWidget* field, bool langevin = false) {
        QLabel* l = new QLabel(label);
        grid->addWidget(l, row, 0);
        grid->addWidget(field, row, 1);
        ++row;
        if (langevin)
            langevinOnly << l << field;
    };
    auto addHeading = [&](const QString& text) {
        QLabel* l = new QLabel(QStringLiteral("<b>%1</b>").arg(text));
        grid->addWidget(l, row++, 0, 1, 2);
    };

    addHeading(tr("Sampler"));
    m_ls_walkers = new QSpinBox;
    m_ls_walkers->setRange(0, 64);
    m_ls_walkers->setSpecialValueText(tr("one per thread"));
    m_ls_walkers->setToolTip(tr("Walkers sampling in parallel, each on its own copy of the model."));
    addRow(tr("Walkers:"), m_ls_walkers);

    m_ls_step = new QDoubleSpinBox;
    m_ls_step->setRange(0, 1);
    m_ls_step->setDecimals(4);
    m_ls_step->setSingleStep(0.005);
    m_ls_step->setSpecialValueText(tr("automatic"));
    m_ls_step->setToolTip(tr("Step of the sampler (for Langevin the step of the integration) as a fraction of the sampled range; the start value, "
                             "the walker adapts it to the shape of the surface. A step that is too large shows as nearly all moves being rejected."));
    addRow(tr("Step size (integration):"), m_ls_step);

    m_ls_gradient_step = new QDoubleSpinBox;
    m_ls_gradient_step->setRange(1e-8, 0.1);
    m_ls_gradient_step->setDecimals(8);
    m_ls_gradient_step->setSingleStep(1e-4);
    m_ls_gradient_step->setValue(1e-3);
    m_ls_gradient_step->setToolTip(tr("Step of the finite differences that give the gradient of the sum of squares (Langevin), as a fraction of the sampled range. "
                                      "Too small a step reads the noise of the fit, too large one smooths the surface."));
    addRow(tr("Gradient step (finite differences):"), m_ls_gradient_step, true);

    m_ls_central = new QCheckBox(tr("Central differences"));
    m_ls_central->setToolTip(tr("Central instead of forward differences for the gradient: more accurate, twice the evaluations per step (Langevin)."));
    grid->addWidget(m_ls_central, row++, 0, 1, 2);
    langevinOnly << m_ls_central;

    m_ls_correction = new QCheckBox(tr("Metropolis correction"));
    m_ls_correction->setChecked(true);
    m_ls_correction->setToolTip(tr("Accept or reject each Langevin step like a Metropolis move: the walk then samples the surface exactly and the step is "
                                   "regulated to the acceptance. Without it the explicit step samples a broadened distribution (rougher profile) but needs "
                                   "no gradient at rejected steps."));
    grid->addWidget(m_ls_correction, row++, 0, 1, 2);
    langevinOnly << m_ls_correction;

    m_ls_range = new QDoubleSpinBox;
    m_ls_range->setRange(0, 50);
    m_ls_range->setDecimals(1);
    m_ls_range->setSpecialValueText(tr("automatic"));
    m_ls_range->setToolTip(tr("Half width of the sampled range around the fit, in log10 units of the constants. Automatic: derived from the fit, 6 when looking for other minima."));
    addRow(tr("Range (log10):"), m_ls_range);

    m_ls_polish = new QSpinBox;
    m_ls_polish->setRange(0, 20);
    m_ls_polish->setValue(5);
    m_ls_polish->setToolTip(tr("How many minima found by the walkers are refitted."));
    addRow(tr("Minima to refit:"), m_ls_polish);

    addHeading(tr("Stopping and barriers"));
    QList<QWidget*> searchOnly, confidenceOnly;
    m_ls_search_wall = new QDoubleSpinBox;
    m_ls_search_wall->setRange(1.5, 1000);
    m_ls_search_wall->setDecimals(1);
    m_ls_search_wall->setValue(10);
    m_ls_search_wall->setToolTip(tr("Height of the wall as a multiple of the sum of squares of the fit. Moves above the wall are rejected: another "
                                    "minimum behind a barrier higher than the wall is not found. A wrong basin of a titration fit lies at several times "
                                    "the optimum."));
    {
        QLabel* l = new QLabel(tr("Wall (x SSE of the fit):"));
        grid->addWidget(l, row, 0);
        grid->addWidget(m_ls_search_wall, row, 1);
        ++row;
        searchOnly << l << m_ls_search_wall;
    }
    m_ls_wall = new QDoubleSpinBox;
    m_ls_wall->setRange(1.1, 100);
    m_ls_wall->setDecimals(1);
    m_ls_wall->setValue(3.0);
    m_ls_wall->setToolTip(tr("Height of the wall in units of how far the limit of the confidence region lies above the fit. Moves above the wall "
                             "are rejected; another minimum behind a barrier higher than the wall is not found."));
    {
        QLabel* l = new QLabel(tr("Wall height:"));
        grid->addWidget(l, row, 0);
        grid->addWidget(m_ls_wall, row, 1);
        ++row;
        confidenceOnly << l << m_ls_wall;
    }

    m_ls_min_steps = new QSpinBox;
    m_ls_min_steps->setRange(0, 10000000);
    m_ls_min_steps->setSingleStep(1000);
    m_ls_min_steps->setValue(2000);
    m_ls_min_steps->setToolTip(tr("A walker does not stop before this many steps."));
    addRow(tr("Minimum steps:"), m_ls_min_steps);

    m_ls_patience = new QSpinBox;
    m_ls_patience->setRange(1, 100000);
    m_ls_patience->setValue(100);
    m_ls_patience->setToolTip(tr("A walker stops after this many hills in a row (one per 20 steps) that opened up no new region and no better minimum."));
    addRow(tr("Patience:"), m_ls_patience);

    m_ls_converged = new QDoubleSpinBox;
    m_ls_converged->setRange(0.001, 1);
    m_ls_converged->setDecimals(3);
    m_ls_converged->setSingleStep(0.01);
    m_ls_converged->setValue(0.1);
    m_ls_converged->setToolTip(tr("A walker only counts towards its patience while the hill height at its position is below this fraction of the "
                                  "first hill, i.e. while the bias there has filled up. Smaller: longer runs, higher barriers are taken down."));
    addRow(tr("Bias convergence:"), m_ls_converged);

    m_ls_bias = new QDoubleSpinBox;
    m_ls_bias->setRange(0, 10);
    m_ls_bias->setDecimals(2);
    m_ls_bias->setSingleStep(0.1);
    m_ls_bias->setValue(0.5);
    m_ls_bias->setToolTip(tr("Well-tempering temperature in units of the wall's height above the fit; barriers up to about 2.3 times this are taken "
                             "down. 0 switches well-tempering off."));
    addRow(tr("Bias strength:"), m_ls_bias);

    m_ls_hill_width = new QDoubleSpinBox;
    m_ls_hill_width->setRange(0.005, 0.5);
    m_ls_hill_width->setDecimals(3);
    m_ls_hill_width->setSingleStep(0.005);
    m_ls_hill_width->setValue(0.05);
    m_ls_hill_width->setToolTip(tr("Width of one hill as a fraction of the sampled range. Hills much wider than a narrow valley fill it too early."));
    addRow(tr("Hill width:"), m_ls_hill_width);

    more->setLayout(grid);
    MoreLessToggle* toggle = new MoreLessToggle(more, QStringLiteral("landscape_more"));
    top->addWidget(toggle, 0, Qt::AlignLeft);
    top->addWidget(more);

    // what only Langevin uses is shown for Langevin only
    auto showLangevin = [this, langevinOnly]() {
        const bool langevin = m_ls_sampler->currentData().toString() == QLatin1String("Langevin");
        for (QWidget* w : langevinOnly)
            w->setVisible(langevin);
    };
    connect(m_ls_sampler, &QComboBox::currentIndexChanged, this, showLangevin);

    // what belongs to the task: the confidence is not asked for a search, the wall of a search is a multiple of the SSE
    confidenceOnly << confidenceLabel << m_ls_confidence;
    auto showTask = [this, info, searchOnly, confidenceOnly]() {
        const bool search = m_ls_task->currentData().toString() == QLatin1String("Minima");
        for (QWidget* w : searchOnly)
            w->setVisible(search);
        for (QWidget* w : confidenceOnly)
            w->setVisible(!search);
        info->setText(search ? tr("Lets the walkers climb out of the basin of the fit and refits the minima they find. Another minimum is only found "
                                  "if the wall is higher than the barrier to it.")
                             : tr("Samples the sum of squares over the global parameters (the linear parameters are fitted for each point) and reads "
                                  "the confidence interval of every constant from it, also for correlated constants. Minima other than the fit are "
                                  "refitted and listed."));
        m_ls_polish->setValue(search ? 10 : 5);
    };
    connect(m_ls_task, &QComboBox::currentIndexChanged, this, showTask);

    m_ls_note = new QLabel(tr("This model has no variable projection: the landscape can only be sampled for models that fit their linear parameters out."));
    m_ls_note->setWordWrap(true);
    top->addWidget(m_ls_note);

    m_ls_run = new QPushButton(tr("Sample"));
    top->addWidget(m_ls_run);
    top->addStretch(1);

    connect(m_ls_run, &QPushButton::clicked, this, [this]() {
        clearMessages();
        emit RunCalculation(RunLandscape());
    });
    if (m_model)
        connect(m_model.toStrongRef().data(), &AbstractModel::Recalculated, this, &StatisticDialog::updateLandscape);

    widget->setLayout(top);
    // only now: setVisible(true) on a widget without a parent shows it as a window of its own
    showLangevin();
    showTask();
    updateLandscape();
    if (m_model) {
        const int globals = m_model.toStrongRef().data()->GlobalParameterSize();
        m_ls_surface->setChecked(globals >= 2 && globals <= 4);
        m_ls_surface->setEnabled(globals >= 2);
    } else
        m_ls_surface->setEnabled(false);
    return widget;
}

/*! \brief Tab of the Weakened Grid Search: the confidence limits of single parameters. In sight what one sets: the parameters, the
 * confidence; behind "..more.." the size of a step and when a search stops. Claude Generated. */
QWidget* StatisticDialog::GridSearchWidget()
{
    QWidget* widget = new QWidget;
    QVBoxLayout* top = new QVBoxLayout;

    m_wgs_parameters = new ParameterSelector(m_model.toStrongRef(), false);
    top->addWidget(m_wgs_parameters);

    m_wgs_maxerror = new QDoubleSpinBox;
    m_wgs_maxerror->setMaximum(100);
    m_wgs_maxerror->setSingleStep(0.5);
    m_wgs_maxerror->setValue(95);
    m_wgs_maxerror->setDecimals(1);
    m_wgs_maxerror->setSuffix("%");
    connect(m_wgs_maxerror, &QDoubleSpinBox::valueChanged, this, &StatisticDialog::CalculateError);
    QHBoxLayout* row = new QHBoxLayout;
    row->addWidget(new QLabel(tr("Confidence:")));
    row->addWidget(m_wgs_maxerror, 1);
    top->addLayout(row);
    m_wgs_error_info = new QLabel; // F value and the limit of the sum of squares, once there is a model
    m_wgs_error_info->setWordWrap(true);
    top->addWidget(m_wgs_error_info);

    QWidget* more = new QWidget;
    QFormLayout* form = new QFormLayout;
    form->setContentsMargins(0, 0, 0, 0);

    m_store_wgsearch = new QCheckBox(tr("Store intermediate models"));
    m_store_wgsearch->setToolTip(tr("If checked, SupraFit will try to store ALL intermedate results. They are necessary to compute the confidence range for the single parameters \nAND dervied values, such as entropy. The downside are hugh files and sometimes data, that is to large to be handled resulting in an empty output!"));
    form->addRow(m_store_wgsearch);

    m_gridScalingFactor = new QSpinBox;
    m_gridScalingFactor->setMinimum(-10);
    m_gridScalingFactor->setMaximum(10);
    m_gridScalingFactor->setValue(-4);
    m_gridScalingFactor->setToolTip(tr("Set the scaling factor for each step. The lower the value, the smaller the step and the more precise is the result. On the other hand, more steps are needed or the counters below have to be increased."));
    form->addRow(tr("Scaling factor of a step:"), m_gridScalingFactor);

    m_wgs_steps = new QSpinBox;
    m_wgs_steps->setMaximum(1e7);
    m_wgs_steps->setValue(2e3);
    m_wgs_steps->setSingleStep(100);
    m_wgs_steps->setToolTip(tr("Define the maximal number of steps for the lower and upper limit seperatly."));
    form->addRow(tr("Maximal steps (each direction):"), m_wgs_steps);

    m_wgs_err_conv = new ScientificBox;
    m_wgs_err_conv->setDecimals(10);
    m_wgs_err_conv->setValue(1E-9);
    m_wgs_err_conv->setSingleStep(1e-9);
    m_wgs_err_conv->setToolTip(tr("Set the cutoff, where the difference between calculated SSE is considered as zero."));
    form->addRow(tr("Change of the SSE counted as zero:"), m_wgs_err_conv);

    m_gridErrorConvergencyCounter = new QSpinBox;
    m_gridErrorConvergencyCounter->setRange(1, 1e8);
    m_gridErrorConvergencyCounter->setSingleStep(10);
    m_gridErrorConvergencyCounter->setValue(5);
    m_gridErrorConvergencyCounter->setToolTip(tr("Set the maximal number of zero change in SSE before the stopping."));
    form->addRow(tr("Stop after steps without change:"), m_gridErrorConvergencyCounter);

    m_gridOvershotCounter = new QSpinBox;
    m_gridOvershotCounter->setRange(1, 1e8);
    m_gridOvershotCounter->setSingleStep(10);
    m_gridOvershotCounter->setValue(5);
    m_gridOvershotCounter->setToolTip(tr("Set the maximal number of allowed steps, where SSE > SSE(max). The procedure will stop after that number is reached."));
    form->addRow(tr("Stop after steps above the limit:"), m_gridOvershotCounter);

    m_gridErrorDecreaseCounter = new QSpinBox;
    m_gridErrorDecreaseCounter->setRange(1, 1e8);
    m_gridErrorDecreaseCounter->setSingleStep(10);
    m_gridErrorDecreaseCounter->setValue(50);
    m_gridErrorDecreaseCounter->setToolTip(tr("Set the maximal number of allowed steps, where SSE < SSE(fit). The procedure will stop after that number is reached."));
    form->addRow(tr("Stop after steps below the fit:"), m_gridErrorDecreaseCounter);

    more->setLayout(form);
    AddFolded(top, more, QStringLiteral("wgs_more"));

    m_cv = new QPushButton(tr("Calculate"));
    top->addWidget(m_cv);
    top->addStretch(1);
    connect(m_cv, &QPushButton::clicked, this, [this]() {
        clearMessages();
        emit RunCalculation(RunGridSearch());
    });
    widget->setLayout(top);
    return widget;
}

/*! \brief Tab of the Model Comparison: confidence limits from random steps. In sight: parameters, confidence, steps; behind "..more..":
 * the factor of the box of every parameter. Claude Generated. */
QWidget* StatisticDialog::ModelComparison()
{
    QWidget* widget = new QWidget;
    QVBoxLayout* top = new QVBoxLayout;

    m_moco_parameters = new ParameterSelector(m_model.toStrongRef(), true);
    top->addWidget(m_moco_parameters);

    m_moco_maxerror = new QDoubleSpinBox;
    m_moco_maxerror->setMaximum(100);
    m_moco_maxerror->setSingleStep(0.5);
    m_moco_maxerror->setValue(95);
    m_moco_maxerror->setDecimals(1);
    m_moco_maxerror->setSuffix("%");
    connect(m_moco_maxerror, &QDoubleSpinBox::valueChanged, this, &StatisticDialog::CalculateError);
    QHBoxLayout* row = new QHBoxLayout;
    row->addWidget(new QLabel(tr("Confidence:")));
    row->addWidget(m_moco_maxerror, 1);
    top->addLayout(row);
    m_moco_error_info = new QLabel;
    m_moco_error_info->setWordWrap(true);
    top->addWidget(m_moco_error_info);

    m_moco_mc_steps = new QSpinBox;
    m_moco_mc_steps->setMaximum(1e9);
    m_moco_mc_steps->setMinimum(10);
    m_moco_mc_steps->setSingleStep(1000);
    m_moco_mc_steps->setValue(20000);
    m_moco_mc_steps->setToolTip(tr("Number of random steps."));
    row = new QHBoxLayout;
    row->addWidget(new QLabel(tr("Steps:")));
    row->addWidget(m_moco_mc_steps, 1);
    top->addLayout(row);

    if (m_moco_parameters->ScalingWidget()) {
        QWidget* more = new QWidget;
        QVBoxLayout* inner = new QVBoxLayout;
        inner->setContentsMargins(0, 0, 0, 0);
        inner->addWidget(new QLabel(tr("Factor of the box of each parameter:")));
        inner->addWidget(m_moco_parameters->ScalingWidget());
        more->setLayout(inner);
        AddFolded(top, more, QStringLiteral("moco_more"));
    }

    m_moco = new QPushButton(tr("Calculate"));
    top->addWidget(m_moco);
    top->addStretch(1);
    connect(m_moco, &QPushButton::clicked, this, [this]() {
        clearMessages();
        emit RunCalculation(RunModelComparison());
    });
    widget->setLayout(top);
    return widget;
}

/*! \brief Tab of Cross Validation and Reduction Analysis. In sight: how many points are left out; behind "..more..": how the map of the
 * Leave-X-Out experiments is made. Claude Generated. */
QWidget* StatisticDialog::CVWidget()
{
    QWidget* widget = new QWidget;
    QVBoxLayout* top = new QVBoxLayout;
    m_radio_cv_loo = new QRadioButton(tr("Leave-One-Out"));
    m_radio_cv_l2o = new QRadioButton(tr("Leave-Two-Out"));
    m_radio_cv_lxo = new QRadioButton(tr("Leave-X-Out"));

    m_cv_lxo = new QSpinBox;
    m_cv_lxo->setMinimum(1);
    m_cv_lxo->setPrefix(tr("X = "));

    m_cv_runs = new QSpinBox;
    m_cv_runs->setMinimum(1);
    m_cv_runs->setMaximum(10000000);
    m_cv_runs->setValue(10000);
    m_cv_runs->setSuffix(tr(" # steps"));

    QLabel* max_runs = new QLabel;
    max_runs->setWordWrap(true);
    if (m_model) {
        m_cv_lxo->setMaximum(m_model.toStrongRef().data()->DataPoints() - 1);
        connect(m_cv_lxo, qOverload<int>(&QSpinBox::valueChanged), max_runs, [max_runs, this](int value) {
            int n = m_model.toStrongRef().data()->DataPoints();
            int k = value;

            // Safety check to prevent overflow - Claude Generated
            if (k > n || k < 0 || n <= 0) {
                max_runs->setText(tr("Invalid combination parameters"));
                return;
            }

            // Calculate binomial coefficient C(n,k) with overflow protection - Claude Generated
            long double maxsteps = 1.0;
            if (k > n - k)
                k = n - k; // Take advantage of symmetry

            for (int i = 0; i < k; ++i) {
                maxsteps = maxsteps * (n - i) / (i + 1);
                // Check for overflow - Claude Generated
                if (maxsteps > 1e15) {
                    this->m_cv_runs->setMaximum(maxsteps);
                    max_runs->setText(tr("Maximal number of runs > 10^15 (too large)"));
                    return;
                }
            }

            max_runs->setText(tr("Maximal number of runs = %1").arg(QString::number(maxsteps, 'g')));
        });
    }
    m_cv_lxo->setValue(3);

    top->addWidget(m_radio_cv_loo);
    top->addWidget(m_radio_cv_l2o);
    QHBoxLayout* lxo = new QHBoxLayout;
    lxo->addWidget(m_radio_cv_lxo);
    lxo->addWidget(m_cv_lxo, 1);
    lxo->addWidget(m_cv_runs, 1);
    top->addLayout(lxo);
    top->addWidget(max_runs);
    m_radio_cv_loo->setChecked(true);
    connect(m_cv_lxo, qOverload<int>(&QSpinBox::valueChanged), m_radio_cv_lxo, &QRadioButton::setChecked);
    connect(m_cv_runs, qOverload<int>(&QSpinBox::valueChanged), m_radio_cv_lxo, &QRadioButton::setChecked);

    // the map of the Leave-X-Out experiments
    QWidget* more = new QWidget;
    QVBoxLayout* inner = new QVBoxLayout;
    inner->setContentsMargins(0, 0, 0, 0);
    QGroupBox* cv_algo = new QGroupBox(tr("Map calculation strategy in LXO"));
    m_cv_premap = new QRadioButton(tr("Precalculation"));
    m_cv_premap->setToolTip(tr("Precalculates the whole map systematically - more than # steps."));
    m_cv_automap = new QRadioButton(tr("Automatic"));
    m_cv_automap->setToolTip(tr("Automatically chooses between either full precalculation or random initialisation!"));
    m_cv_randmap = new QRadioButton(tr("Random initialisation"));
    m_cv_randmap->setToolTip(tr("Calculates only # steps of random LXO experiments!"));
    QVBoxLayout* cv_algo_layout = new QVBoxLayout;
    cv_algo_layout->addWidget(m_cv_premap);
    cv_algo_layout->addWidget(m_cv_automap);
    cv_algo_layout->addWidget(m_cv_randmap);
    m_cv_automap->setChecked(true);
    cv_algo->setLayout(cv_algo_layout);
    inner->addWidget(cv_algo);
    more->setLayout(inner);
    AddFolded(top, more, QStringLiteral("cv_more"));

    m_cross_validate = new QPushButton(tr("Cross Validation"));
    m_reduction = new QPushButton(tr("Reduction Analysis"));
    top->addWidget(m_cross_validate);
    top->addWidget(m_reduction);
    top->addStretch(1);
    connect(m_cross_validate, &QPushButton::clicked, this, [this]() {
        clearMessages();
        emit RunCalculation(RunCrossValidation());
    });
    connect(m_reduction, &QPushButton::clicked, this, [this]() {
        clearMessages();
        emit RunCalculation(RunReductionAnalyse());
    });
    widget->setLayout(top);
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
    controller["f-value"] = m_wgs_f;
    controller["Method"] = SupraFit::Method::WeakenedGridSearch;
    controller["confidence"] = m_wgs_maxerror->value();

    controller["ErrorConvergency"] = m_wgs_err_conv->value();
    controller["OverShotCounter"] = m_gridOvershotCounter->value();
    controller["ErrorDecreaseCounter"] = m_gridErrorDecreaseCounter->value();
    controller["ErrorConvergencyCounter"] = m_gridErrorConvergencyCounter->value();
    controller["StepScalingFactor"] = m_gridScalingFactor->value();

    controller["StoreRaw"] = m_store_wgsearch->isChecked();

    controller["ParameterIndex"] = 0; // the sum of squares

    m_wgs_parameters->AddTo(controller);
    return controller;
}

QJsonObject StatisticDialog::RunModelComparison() const
{
    QJsonObject controller = ModelComparisonConfigBlock;

    controller["MaxSteps"] = m_moco_mc_steps->value();
    controller["MaxParameter"] = m_moco_max;
    controller["f-value"] = m_moco_f;
    controller["Method"] = SupraFit::Method::ModelComparison;
    controller["confidence"] = m_moco_maxerror->value();

    controller["ParameterIndex"] = 0; // the sum of squares

    m_moco_parameters->AddTo(controller);
    return controller;
}

QJsonObject StatisticDialog::RunLandscape() const
{
    QJsonObject controller = LandscapeConfigBlock;
    controller["Method"] = SupraFit::Method::LandscapeSampling;
    controller["confidence"] = m_ls_confidence->value();
    controller["Walkers"] = m_ls_walkers->value();
    controller["MaxSteps"] = m_ls_steps->value();
    controller["Sampler"] = m_ls_sampler->currentData().toString();
    controller["StepSize"] = m_ls_step->value();
    controller["GradientStep"] = m_ls_gradient_step->value();
    controller["CentralGradient"] = m_ls_central->isChecked();
    controller["LangevinCorrection"] = m_ls_correction->isChecked();
    controller["Mode"] = m_ls_task->currentData().toString();
    controller["SearchWall"] = m_ls_search_wall->value();
    controller["CutFactor"] = m_ls_wall->value();
    controller["MinSteps"] = m_ls_min_steps->value();
    controller["Patience"] = m_ls_patience->value();
    controller["MinHillFraction"] = m_ls_converged->value();
    controller["BiasFactor"] = m_ls_bias->value();
    controller["HillWidth"] = m_ls_hill_width->value();
    controller["Range"] = m_ls_range->value();
    controller["Polish"] = m_ls_polish->value();
    controller["Surface"] = m_ls_surface->isChecked();
    return controller;
}

QJsonObject StatisticDialog::RunMonteCarlo() const
{
    QJsonObject controller = MonteCarloConfigBlock;
    controller["MaxSteps"] = m_mc_steps->value();
    controller["Variance"] = m_varianz_box->value();

    controller["VarianceSource"] = m_mc_noise->currentData().toInt(); // 1 user input, 2 SEy, 3 sigma of the fit, 4 bootstrapping

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
        // Ensure positive MaxSteps value - Claude Generated
        int maxSteps = m_cv_runs->value();
        if (maxSteps <= 0) {
            maxSteps = 1000; // Default fallback value
        }
        controller["MaxSteps"] = maxSteps;
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
    if (time < 1)
        return;

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
        // m_progress->setValue(val);
        emit setCurrentProgress(val);
        // m_main_progress->setValue(m_main_steps * 100 + val / double(m_progress->maximum()) * 100);
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
    if (m_embedded)
        m_hide_widget->show();
}

void StatisticDialog::HideWidget()
{
    clearMessages();
    if (m_embedded)
        m_hide_widget->hide();
    else
        hide();
}

void StatisticDialog::Update()
{
    const int source = m_mc_noise->currentData().toInt();
    if (source == 2)
        m_varianz_box->setValue(m_model.toStrongRef().data()->SEy());
    else if (source == 3)
        m_varianz_box->setValue(m_model.toStrongRef().data()->StdDeviation());
    CalculateError();
}

void StatisticDialog::Message(const QString& str)
{
    m_message_box->setText(str);
}

void StatisticDialog::Stage(const QString& text, bool restart)
{
    m_message_box->setText(text);
    if (restart) { // the time estimate belongs to the stage, the elapsed time to the whole job
        QMutexLocker locker(&mutex);
        m_time = 0;
        m_progress->setValue(0);
    }
}

/*! \brief F value and the limit of the sum of squares for the two confidences; the line under each says what they are. Claude Generated. */
void StatisticDialog::CalculateError()
{
    const QSharedPointer<AbstractModel> model = m_model.toStrongRef();
    if (!model)
        return;
    updateUI();
    const double sse = model->SSE();
    auto describe = [&](QDoubleSpinBox* confidence, QLabel* info, qreal& f, qreal& limit) {
        const double p = confidence->value() / 100.0;
        f = model->finv(p);
        limit = model->ErrorfTestThreshold(p);
        info->setText(tr("F = %1: the sum of squares may rise from %2 to %3.").arg(f, 0, 'g', 5).arg(sse, 0, 'g', 6).arg(limit, 0, 'g', 6));
        info->setToolTip(FOutput());
    };
    describe(m_wgs_maxerror, m_wgs_error_info, m_wgs_f, m_wgs_max);
    describe(m_moco_maxerror, m_moco_error_info, m_moco_f, m_moco_max);
}

#include "statisticdialog.moc"

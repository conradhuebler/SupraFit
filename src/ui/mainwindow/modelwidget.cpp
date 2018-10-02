/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/global.h"
#include "src/version.h"

#include "src/capabilities/abstractsearchclass.h"
#include "src/capabilities/modelcomparison.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/capabilities/reductionanalyse.h"
#include "src/capabilities/weakenedgridsearch.h"

#include "src/core/AbstractModel.h"
#include "src/core/dataclass.h"
#include "src/core/jsonhandler.h"
#include "src/core/minimizer.h"
#include "src/core/toolset.h"

#include "src/ui/dialogs/advancedsearch.h"
#include "src/ui/dialogs/configdialog.h"
#include "src/ui/dialogs/modeldialog.h"
#include "src/ui/dialogs/resultsdialog.h"
#include "src/ui/dialogs/statisticdialog.h"

#include "src/ui/guitools/chartwrapper.h"

#include "src/ui/mainwindow/chartwidget.h"

#include "src/ui/widgets/buttons/spinbox.h"
#include "src/ui/widgets/chartview.h"
#include "src/ui/widgets/extwidget.h"
#include "src/ui/widgets/listchart.h"
#include "src/ui/widgets/modelactions.h"
#include "src/ui/widgets/modelelement.h"
#include "src/ui/widgets/optionswidget.h"
#include "src/ui/widgets/parameterwidget.h"
#include "src/ui/widgets/results/mcresultswidget.h"
#include "src/ui/widgets/results/resultswidget.h"
#include "src/ui/widgets/results/searchresultwidget.h"
#include "src/ui/widgets/statisticwidget.h"
#include "src/ui/widgets/systemparameterwidget.h"

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QXYSeries>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSettings>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QtMath>

#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTableView>
#include <QtWidgets/QTextEdit>

#include <iostream>
#include <random>

#include "modelwidget.h"

ModelWidget::ModelWidget(QSharedPointer<AbstractModel> model, Charts charts, bool readonly, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_charts(charts)
    , m_pending(false)
    , m_minimizer(QSharedPointer<Minimizer>(new Minimizer(true, this), &QObject::deleteLater))
    , m_statistic(false)
    , m_val_readonly(readonly)
{
    // m_model->SystemParameterChanged();
#warning might have been important

    m_model_widget = new QWidget;
    Data2Text();
    m_minimizer->setModel(m_model);

    m_advancedsearch = new AdvancedSearch(this);
    m_advancedsearch->setModel(m_model);
    connect(m_advancedsearch, SIGNAL(MultiScanFinished()), this, SLOT(MultiScanFinished()));

    m_statistic_dialog = new StatisticDialog(m_model, this);
    connect(m_statistic_dialog, &StatisticDialog::MCStatistic, this, &ModelWidget::MCStatistic);
    connect(m_statistic_dialog, &StatisticDialog::WGStatistic, this, &ModelWidget::WGStatistic);
    connect(m_statistic_dialog, &StatisticDialog::MoCoStatistic, this, &ModelWidget::MoCoStatistic);
    connect(m_statistic_dialog, &StatisticDialog::CrossValidation, this, &ModelWidget::CVAnalyse);
    connect(m_statistic_dialog, &StatisticDialog::Reduction, this, &ModelWidget::DoReductionAnalyse);


    connect(this, SIGNAL(ToggleSeries(int)), m_charts.error_wrapper.data(), SLOT(SetBlocked(int)));
    connect(this, SIGNAL(ToggleSeries(int)), m_charts.signal_wrapper.data(), SLOT(SetBlocked(int)));

    m_dialogs = new ModalDialog(this);
    m_dialogs->setWindowTitle("Information " + m_model->Name() + " | " + qApp->instance()->property("projectname").toString());

    m_statistic_widget = new StatisticWidget(m_model, this);

    m_results = new ResultsDialog(m_model, m_charts.signal_wrapper.data(), this);
    connect(m_results, &ResultsDialog::LoadModel, this, &ModelWidget::LoadJson);
    connect(m_results, &ResultsDialog::AddModel, this, &ModelWidget::AddModel);

    m_global_box = new QCheckBox(tr("Global Parameter"));
    m_global_box->setChecked(true);

    m_local_box = new QCheckBox(tr("Local Parameter"));
    m_local_box->setChecked(true);

    m_layout = new QVBoxLayout;
    QHBoxLayout* const_layout = new QHBoxLayout;
    for (int i = 0; i < m_model->GlobalParameterSize(); ++i) {
        QGroupBox* group = new QGroupBox;
        group->setFixedWidth(180);
        QHBoxLayout* hlayout = new QHBoxLayout;
        QPointer<SpinBox> constant = new SpinBox;
        QPointer<QCheckBox> check = new QCheckBox;
        check->setChecked(true);
        m_constants << constant;
        constant->setDecimals(4);

        constant->setPrefix(m_model->GlobalParameterPrefix(i));
        constant->setSingleStep(m_model->GlobalParameter(i) / 100);

        constant->setValue(m_model->GlobalParameter(i));
        constant->setMaximum(1e9);
        constant->setMinimum(-1e9);
        constant->setReadOnly(m_val_readonly);
        connect(constant, SIGNAL(valueChangedNotBySet(double)), this, SLOT(recalculate()));
        connect(m_model.data(), &AbstractModel::Recalculated, this,
            [i, constant, this, check]() {
                if (this->m_model && constant) {
                    if (this->m_model->GlobalEnabled(i)) {
                        constant->setStyleSheet("background-color: " + included());
                        check->setEnabled(true);
                        check->setChecked(m_model->GlobalTable()->isChecked(i, 0));
                    } else {
                        check->setEnabled(false);
                        constant->setStyleSheet("background-color: " + excluded());
                    }
                }
            });
        connect(check, &QCheckBox::stateChanged, check, [i, this](int state) {
            if (this->m_model) {
                m_model->GlobalTable()->setChecked(i, 0, state);
            }
        });
        connect(m_global_box, &QCheckBox::stateChanged, check, [check](int state) {
            check->setChecked(state);
        });

        hlayout->addWidget(new QLabel(m_model->GlobalParameterName(i)));
        hlayout->addWidget(constant);
        hlayout->addWidget(check);
        group->setLayout(hlayout);
        const_layout->addWidget(group);
    }
    const_layout->addStretch(100);

    QVBoxLayout* fit_layout = new QVBoxLayout;
    fit_layout->addWidget(m_global_box);
    fit_layout->addWidget(m_local_box);

    if (!m_val_readonly)
        const_layout->addLayout(fit_layout);

    m_minimize_all = new QPushButton(tr("Fit"));

    QAction* minimize_normal = new QAction(tr("Tight"), this);
    connect(minimize_normal, SIGNAL(triggered()), this, SLOT(GlobalMinimize()));

    QAction* minimize_loose = new QAction(tr("Loose"), this);
    connect(minimize_loose, SIGNAL(triggered()), this, SLOT(GlobalMinimizeLoose()));

    QAction* fast_conf = new QAction(tr("Confidence"), this);
    connect(fast_conf, SIGNAL(triggered()), this, SLOT(FastConfidence()));

    QMenu* menu = new QMenu(this);
    menu->addAction(minimize_normal);
    menu->addAction(minimize_loose);
    menu->addAction(fast_conf);
    menu->setDefaultAction(minimize_normal);
    m_minimize_all->setMenu(menu);

    if (!m_val_readonly)
        const_layout->addWidget(m_minimize_all);

    m_layout->addLayout(const_layout);
    m_layout->addWidget(new extWidget(m_model, this));

    //    if (!m_val_readonly) {
    m_model_options_widget = new OptionsWidget(m_model);
    if (m_model->getAllOptions().size())
        m_layout->addWidget(m_model_options_widget);
    //    }

    m_sign_layout = new QVBoxLayout;

    m_sign_layout->setAlignment(Qt::AlignTop);
    m_converged_label = new QLabel;

    m_readonly = new QCheckBox(tr("Read Only"));
    QHBoxLayout* head = new QHBoxLayout;
    head->addWidget(m_converged_label);

    if (!m_val_readonly)
        head->addWidget(m_readonly);

    m_sign_layout->addLayout(head);
    if (m_model->SupportSeries()) {
        if (m_model->LocalParameterSize()) {
            for (int i = 0; i < m_charts.signal_wrapper->SeriesSize(); ++i) {
                ModelElement* el = new ModelElement(m_model, m_charts, i);
                connect(el, SIGNAL(ValueChanged()), this, SLOT(recalculate()));
                connect(el, SIGNAL(ActiveSignalChanged()), this, SLOT(CollectActiveSignals()));
                connect(this, SIGNAL(Update()), el, SLOT(Update()));
                connect(this, SIGNAL(ToggleSeries(int)), el, SLOT(ToggleSeries(int)));

                if (!m_val_readonly)
                    connect(m_readonly, &QCheckBox::stateChanged, el, &ModelElement::setReadOnly);

                connect(m_local_box, &QCheckBox::stateChanged, el, &ModelElement::LocalCheckState);
                el->setReadOnly(m_val_readonly);
                m_sign_layout->addWidget(el);
                m_model_elements << el;
            }
        }
    } else {
        QLineEdit* name = new QLineEdit(m_model->Name());
        name->setPlaceholderText(m_model->Name());
        name->setClearButtonEnabled(true);
        m_sign_layout->addWidget(name);
        connect(name, &QLineEdit::textChanged, m_charts.signal_wrapper->Series(0), [this, name]() {
            if (name) {
                if (name->text() != this->m_charts.signal_wrapper->Series(0)->name()) {
                    this->m_charts.signal_wrapper->Series(0)->setName(name->text());
                    this->m_charts.error_wrapper->Series(0)->setName(name->text());
                }
            }
        });

        connect(m_charts.signal_wrapper->Series(0), &LineSeries::nameChanged, m_charts.signal_wrapper->Series(0), [this, name]() {
            this->m_charts.signal_wrapper->Series(0)->setName(name->text());
            this->m_charts.error_wrapper->Series(0)->setName(name->text());
        });
        m_charts.signal_wrapper->Series(0)->setName(m_model->Name());

        m_local_parameter = new LocalParameterWidget(m_model);
        m_sign_layout->addWidget(m_local_parameter);

        if (!m_val_readonly)
            connect(m_readonly, &QCheckBox::stateChanged, m_local_parameter, &LocalParameterWidget::setReadOnly);
        m_local_parameter->setReadOnly(m_val_readonly);
        connect(this, &ModelWidget::ToggleSeries, this,
            [this]() {
                m_charts.signal_wrapper->Series(0)->setVisible(!m_charts.signal_wrapper->Series(0)->isVisible());
                m_charts.error_wrapper->Series(0)->setVisible(!m_charts.error_wrapper->Series(0)->isVisible());
            });

            connect(m_local_box, &QCheckBox::stateChanged, m_local_parameter, &LocalParameterWidget::LocalCheckState);
    }

    if (m_model->getSystemParameterList().size())
        m_sign_layout->addWidget(new SPOverview(m_model.data(), m_val_readonly));

    QWidget* scroll = new QWidget;
    scroll->setLayout(m_sign_layout);
    QScrollArea* area = new QScrollArea;
    area->setWidgetResizable(true);
    area->setWidget(scroll);

    m_layout->addWidget(area);

    DiscreteUI();

    resizeButtons();
    m_model_widget->setLayout(m_layout);
    m_splitter = new QSplitter(this);
    m_splitter->setOrientation(Qt::Vertical);
    m_splitter->addWidget(m_model_widget);
    m_splitter->addWidget(m_statistic_widget);
    connect(m_splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(SplitterResized()));
    QVBoxLayout* vlayout = new QVBoxLayout;
    vlayout->addWidget(m_splitter);
    setLayout(vlayout);
    QSettings settings;
    settings.beginGroup("model");
    m_splitter->restoreState(settings.value("splitterSizes").toByteArray());
    settings.endGroup();
    m_last_model = m_model->ExportModel(true, true);

    connect(m_model.data(), &AbstractModel::Recalculated, this, &ModelWidget::Repaint);
    m_model->Calculate();
}

ModelWidget::~ModelWidget()
{

    m_charts.signal_wrapper.clear();
    m_charts.error_wrapper.clear();
    m_charts.data_wrapper.clear();

    // m_minimizer.clear();

    m_statistic_dialog->hide();
    delete m_statistic_dialog;

    m_model.clear();
    delete m_model.data();
}

void ModelWidget::setColorList(const QString& str)
{
    QStringList colors = str.split("|");
    if (colors.size() == m_model_elements.size()) {
        for (int i = 0; i < m_model_elements.size(); ++i)
            m_model_elements[i]->ChangeColor(QColor(colors[i]));
    }
}

void ModelWidget::setKeys(const QString& str)
{
    if (str.isEmpty() || str.isNull())
        return;
    QStringList labels = str.split("|");
    if (labels.size() == m_charts.signal_wrapper->SeriesSize()) {
        for (int i = 0; i < m_charts.signal_wrapper->SeriesSize(); ++i) {
            if (m_model_elements.size() <= i)
                continue;
            m_model_elements[i]->setLabel(labels[i]);
        }
    }
}

void ModelWidget::SplitterResized()
{
    QSettings settings;
    settings.beginGroup("model");
    settings.setValue("splitterSizes", m_splitter->saveState());
    settings.endGroup();
}

void ModelWidget::DiscreteUI()
{
    m_actions = new ModelActions;

    connect(m_actions, SIGNAL(NewGuess()), this, SLOT(NewGuess()));
    connect(m_actions, SIGNAL(LocalMinimize()), this, SLOT(LocalMinimize()));
    connect(m_actions, SIGNAL(OptimizerSettings()), this, SLOT(OptimizerSettings()));
    connect(m_actions, SIGNAL(ImportConstants()), this, SLOT(ImportConstants()));
    connect(m_actions, SIGNAL(ExportConstants()), this, SLOT(ExportConstants()));
    connect(m_actions, SIGNAL(OpenAdvancedSearch()), this, SLOT(OpenAdvancedSearch()));
    connect(m_actions, SIGNAL(TogglePlot()), this, SLOT(TogglePlot()));
    connect(m_actions, SIGNAL(ToggleStatisticDialog()), this, SLOT(ToggleStatisticDialog()));
    connect(m_actions, SIGNAL(Save2File()), this, SLOT(Save2File()));
    connect(m_actions, SIGNAL(ExportSimModel()), this, SLOT(ExportSimModel()));
    connect(m_actions, &ModelActions::Restore, this, &ModelWidget::Restore);
    connect(m_actions, &ModelActions::Detailed, this, &ModelWidget::Detailed);

    m_layout->addWidget(m_actions);
    m_actions->LocalEnabled(m_model->SupportSeries());
}

void ModelWidget::resizeButtons()
{
    m_minimize_all->setMaximumSize(70, 30);
    m_minimize_all->setStyleSheet("background-color: #77d740;");
}

void ModelWidget::EmptyUI()
{
    // //     m_add_sim_signal = new QPushButton(tr("Add Signal"));
    // //     connect(m_add_sim_signal, SIGNAL(clicked()), this, SLOT(AddSimSignal()));
    // //     m_layout->addWidget(m_add_sim_signal);
}

void ModelWidget::setParameter()
{
    for (int j = 0; j < m_model->GlobalParameterSize(); ++j) {
        if (qAbs(m_constants[j]->value() - m_model->GlobalParameter(j)) > 1e-5) // lets do no update if the model was calculated with the recently set constants
            m_constants[j]->setValue(m_model->GlobalParameter(j));
    }
    emit Update();
}

void ModelWidget::Repaint()
{

    if (m_model->Type() == 3)
        return;
    m_pending = true;
    setParameter();
    qreal error = 0;
    for (int j = 0; j < m_model_elements.size(); ++j) {
        error += m_model->SumOfErrors(j);
        m_model_elements[j]->Update();
    }
    m_pending = false;
    m_minimize_all->setEnabled(true);

    QTimer::singleShot(1, m_statistic_widget, SLOT(Update()));

    QString converged;
    if (!m_model->isConverged())
        converged = "<font color =\'red\'>Optimisation did not converge.</font>\n";
    else
        converged = "Optimisation converged!";

    QString corrupt;
    if (m_model->isCorrupt())
        corrupt = "<font color =\'red\'><strong>Model is corrupt, probably numerical errors in concentration calculation. Please don't rely on these results.</strong></font>\n";

    m_converged_label->setText("<html><p>" + converged + "</p><p>" + corrupt + "</p></html>");

    /* Model2Text();
    QTextDocument doc;
    doc.setHtml(m_statistic_widget->Overview());
    m_logging += "\n\n" + doc.toPlainText();*/
}

void ModelWidget::recalculate()
{
    if (m_pending)
        return;
    m_pending = true;
    CollectParameters();
    m_model->Calculate();
    QTimer::singleShot(1, this, SLOT(Repaint()));
    m_pending = false;
}

void ModelWidget::CollectParameters()
{
    QList<qreal> pure_signals, constants;
    QVector<QList<qreal>> complex_signals;
    complex_signals.resize(m_model->GlobalParameterSize());
    QList<int> active_signals;
    for (int i = 0; i < m_model_elements.size(); ++i) {
        active_signals << m_model_elements[i]->Include();
        m_model->setLocalParameterSeries(m_model_elements[i]->D(), i);
    }

    for (int i = 0; i < m_model->GlobalParameterSize(); ++i)
        constants << m_constants[i]->value();
    m_model->setActiveSignals(active_signals);
    m_model->setGlobalParameter(constants);
}

void ModelWidget::GlobalMinimizeLoose()
{
    OptimizerConfig config = m_model->getOptimizerConfig();
    config.Constant_Convergence = 1E-1;
    MinimizeModel(config);
}

void ModelWidget::GlobalMinimize()
{
    OptimizerConfig config = m_model->getOptimizerConfig();
    MinimizeModel(config);
}

void ModelWidget::MinimizeModel(const OptimizerConfig& config)
{
    Waiter wait;
    if (m_pending)
        return;
    m_pending = true;

    CollectParameters();
    QJsonObject json = m_model->ExportModel();
    QList<int> locked = m_model->LockedParameters();
    m_minimizer->setParameter(json);

    m_model->setOptimizerConfig(config);
    int result;
    result = m_minimizer->Minimize();

    json = m_minimizer->Parameter();
    m_last_model = json;
    m_model->ImportModel(json);
    m_model->OptimizeParameters();
    Repaint();
    if (qApp->instance()->property("auto_confidence").toBool())
        FastConfidence();
    else
        m_statistic_widget->Update();

    QSettings settings;
    settings.beginGroup("minimizer");
    settings.endGroup();

    if (!result)
        emit Warning(tr("The optimization did not converge within the cycles! Rerun optimisation or increase number of steps."), 1);
    m_statistic = false;
    m_pending = false;
}

void ModelWidget::ToggleStatisticDialog()
{
    m_statistic_dialog->Attention();
}


void ModelWidget::MCStatistic(MCConfig config)
{
    Waiter wait;

    config.optimizer_config = m_model->getOptimizerConfig();

    QPointer<MonteCarloStatistics> statistic = new MonteCarloStatistics(config, this);
    connect(m_statistic_dialog, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(this, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), m_statistic_dialog, SLOT(IncrementProgress(int)), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)), Qt::DirectConnection);
    statistic->setModel(m_model);
    statistic->Evaluate();

    LoadStatistic(statistic->Result(), statistic->Models());
    delete statistic;

    QString buff = m_statistic_widget->Statistic();
    buff.remove("<tr>");
    buff.remove("<table>");
    buff.remove("</tr>");
    buff.remove("</table>");
    buff.replace("</td>", "\t");
    buff.replace("<td>", "\t");

    QTextDocument doc;
    doc.setHtml(buff);

    m_logging += "\n\n" + doc.toPlainText();
}

void ModelWidget::FastConfidence()
{
    MoCoConfig config;

    qreal f_value = m_model.data()->finv(qApp->instance()->property("p_value").toDouble() / 100);
    qreal error = m_model.data()->SumofSquares();
    config.maxerror = error * (f_value * m_model.data()->Parameter() / (m_model.data()->Points() - m_model.data()->Parameter()) + 1);
    config.optimizer_config = m_model->getOptimizerConfig();
    config.fisher_statistic = true;
    config.confidence = qApp->instance()->property("p_value").toDouble();
    ModelComparison* statistic = new ModelComparison(config, this);
    statistic->setModel(m_model);
    bool series = qApp->instance()->property("series_confidence").toBool();
    statistic->FastConfidence(series);
    m_model->UpdateStatistic(statistic->Result());
    m_fast_confidence = statistic->Results();
    delete statistic;
}

void ModelWidget::CVAnalyse(ReductionAnalyse::CVType type)
{
    Waiter wait;
    ReductionAnalyse* statistic = new ReductionAnalyse(m_model->getOptimizerConfig());
    connect(m_statistic_dialog, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(this, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(statistic, SIGNAL(MaximumSteps(int)), m_statistic_dialog, SLOT(MaximumSteps(int)), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), m_statistic_dialog, SLOT(IncrementProgress(int)), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)), Qt::DirectConnection);
    statistic->setModel(m_model);
    statistic->CrossValidation(type);
    LoadStatistic(statistic->Result(), statistic->Models());
    emit IncrementProgress(1);
    delete statistic;
}

void ModelWidget::DoReductionAnalyse()
{
    Waiter wait;
    ReductionAnalyse* statistic = new ReductionAnalyse(m_model->getOptimizerConfig());

    connect(m_statistic_dialog, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(this, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(statistic, SIGNAL(MaximumSteps(int)), m_statistic_dialog, SLOT(MaximumSteps(int)), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), m_statistic_dialog, SLOT(IncrementProgress(int)), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)), Qt::DirectConnection);

    statistic->setModel(m_model);
    statistic->PlainReduction();
    LoadStatistic(statistic->Result(), statistic->Models());
    emit IncrementProgress(1);
    delete statistic;
}
/*
void ModelWidget::WGStatistic()
{
    WGSConfig config = m_statistic_dialog->getWGSConfig();
    WGStatistic(config);
}*/

void ModelWidget::WGStatistic(WGSConfig config)
{
    Waiter wait;

    config.optimizer_config = m_model->getOptimizerConfig();

    if (config.maxerror < 1E-8)
        config.maxerror = m_model->Error(config.confidence, config.fisher_statistic);

    WeakenedGridSearch* statistic = new WeakenedGridSearch(config, this);

    connect(m_statistic_dialog, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(this, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), m_statistic_dialog, SLOT(IncrementProgress(int)), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)), Qt::DirectConnection);
    connect(statistic, SIGNAL(setMaximumSteps(int)), m_statistic_dialog, SIGNAL(setMaximumSteps(int)), Qt::DirectConnection);

    QJsonObject json = m_model->ExportModel(false);
    statistic->setModel(m_model);
    statistic->setParameter(json);

    if (!statistic->ConfidenceAssesment()) {
        emit Warning("The optimization seems not to be converged with respect to at least one constants!\nShowing the results anyway.", 1);
    }

    LoadStatistic(statistic->Result(), statistic->Models());
    emit IncrementProgress(1);
    delete statistic;
}
/*
void ModelWidget::MoCoStatistic()
{
    MoCoConfig config = m_statistic_dialog->getMoCoConfig();
    MoCoStatistic(config);
}
*/
void ModelWidget::MoCoStatistic(MoCoConfig config)
{
    Waiter wait;

    config.optimizer_config = m_model->getOptimizerConfig();

    if (config.maxerror < 1E-8)
        config.maxerror = m_model->Error(config.confidence, config.fisher_statistic);

    ModelComparison* statistic = new ModelComparison(config, this);
    if (m_fast_confidence.size())
        statistic->setResults(m_fast_confidence);
    connect(m_statistic_dialog, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(this, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), m_statistic_dialog, SLOT(IncrementProgress(int)), Qt::DirectConnection);
    connect(statistic, SIGNAL(setMaximumSteps(int)), m_statistic_dialog, SIGNAL(setMaximumSteps(int)), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)), Qt::DirectConnection);

    QJsonObject json = m_model->ExportModel(false);
    statistic->setModel(m_model);
    bool result = statistic->Confidence();
    if (result)
        LoadStatistic(statistic->Result(), statistic->Models());
    else
        QMessageBox::information(this, tr("Not done"), tr("No calculation where done, because there is only one parameter of interest."));
    m_statistic_dialog->HideWidget();
    delete statistic;
}

void ModelWidget::LoadStatistic(const QJsonObject& data, const QList<QJsonObject>& models)
{
    int index = m_model->UpdateStatistic(data);
    m_results->Attention();
    m_statistic_dialog->HideWidget();
    SupraFit::Statistic type = SupraFit::Statistic(data["controller"].toObject()["method"].toInt());
    m_results->ShowResult(type, index);
}

void ModelWidget::LocalMinimize()
{

    if (m_pending)
        return;
    Waiter wait;
    CollectParameters();
    int result = 0;
    for (int i = 0; i < m_model->SeriesCount(); ++i) {
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        QSharedPointer<AbstractModel> model = m_model->Clone();
        QJsonObject parameter = m_model->ExportModel(false);
        model->ImportModel(parameter, false);

        QList<int> active_signals = QVector<int>(m_model_elements.size(), 0).toList();
        active_signals[i] = 1;
        model->setActiveSignals(active_signals);
        OptimizerConfig config = model->getOptimizerConfig();
        model->setOptimizerConfig(config);
        m_minimizer->setModel(model);
        model->setName(tr("%1 Series %2").arg(model->Name().remove("Model")).arg(i + 1));
        result += m_minimizer->Minimize();
        QJsonObject json = m_minimizer->Parameter();
        emit AddModel(json);
        model.clear();
    }

    if (result < m_model->SeriesCount())
        emit Warning(tr("The optimization did not converge within the cycles! Rerun optimisation or increase number of steps."), 1);

    m_minimizer->setModel(m_model);
    m_statistic = false;
    m_pending = false;
}

QList<int> ModelWidget::ActiveSignals()
{
    QList<int> active_signals;
    for (int i = 0; i < m_model_elements.size(); ++i)
        active_signals << m_model_elements[i]->Include();
    return active_signals;
}

void ModelWidget::CollectActiveSignals()
{
    QList<int> active_signals = ActiveSignals();
    m_model->setActiveSignals(active_signals);
}

void ModelWidget::NewGuess()
{
    int r = QMessageBox::warning(this, tr("New Guess."),
        tr("Really create a new guess?"),
        QMessageBox::Yes | QMessageBox::Default,
        QMessageBox::No | QMessageBox::Escape);

    if (r == QMessageBox::No)
        return;
    m_model->InitialGuess();
    //QList<qreal> constants = m_model->GlobalParameter();
    for (int j = 0; j < m_model->GlobalParameterSize(); ++j)
        m_constants[j]->setValue(m_model->GlobalParameter(j));
    emit Update();
}

void ModelWidget::OptimizerSettings()
{
    OptimizerDialog dialog(m_model->getOptimizerConfig(), this);
    if (dialog.exec() == QDialog::Accepted) {
        m_model->setOptimizerConfig(dialog.Config());
    }
}
void ModelWidget::ExportConstants()
{
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("Json File (*.json);;Binary (*.suprafit);;All files (*.*)"));
    if (!str.isEmpty()) {
        setLastDir(str);
        QJsonObject gameObject = m_model->ExportModel();
        JsonHandler::WriteJsonFile(gameObject, str);
    }
}

void ModelWidget::ImportConstants()
{
    QString str = QFileDialog::getOpenFileName(this, tr("Open File"), getDir(), tr("Json File (*.json);;Binary (*.suprafit);;All files (*.*)"));
    if (!str.isEmpty()) {
        setLastDir(str);
        QJsonObject object;
        if (JsonHandler::ReadJsonFile(object, str))
            LoadJson(object);
        else
            qDebug() << "loading failed";
    }
}

void ModelWidget::LoadJson(const QJsonObject& object)
{
    Waiter wait;
    m_model->ImportModel(object);

    for (int j = 0; j < m_model->GlobalParameterSize(); ++j)
        m_constants[j]->setValue(m_model->GlobalParameter(j));

    if (qApp->instance()->property("auto_confidence").toBool())
        FastConfidence();
    else
        m_statistic_widget->Update();

    Repaint();
}

void ModelWidget::OpenAdvancedSearch()
{
    if (!m_advancedsearch.isNull())
        m_advancedsearch->show();
}

void ModelWidget::ExportSimModel()
{
    bool ok;
    qreal scatter = QInputDialog::getDouble(this, tr("Set Standard Deviation"), tr("Set Standard Deviation for scatter"), m_model->SEy(), 0, 2147483647, 4, &ok);
    if (ok) {
        QString content = m_model->RandomInput(0, scatter);

        QString filename = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), m_model->RandomExportSuffix());
        if (!filename.isEmpty()) {
            setLastDir(filename);
            QFile file(filename);
            if (file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
                QTextStream stream(&file);
                stream << content;
            }
        }
    }
}

void ModelWidget::TogglePlot()
{
    m_results->Attention();
}

void ModelWidget::MultiScanFinished()
{
    LoadStatistic(m_advancedsearch->globalSearch()->Result());
}

void ModelWidget::HideAllWindows()
{
    m_advancedsearch->hide();
    m_statistic_dialog->hide();
}

void ModelWidget::Data2Text()
{
    QString text;
    text += "******************************************************************************************************\n";
    text += "This is a SupraFit save file for " + m_model->Name() + "\n";
    text += "SupraFit has been compilied on " + QString::fromStdString(__DATE__) + " at " + QString::fromStdString(__TIME__) + "\n";
    text += "Git Branch used was " + git_branch + " - Commit Hash: " + git_commit_hash + " at " + git_date + ".\n";
    text += "******************************************************************************************************\n";
    text += "\n";
    text += m_model->Data2Text();
    m_logging += text;
}

void ModelWidget::Model2Text()
{
    m_logging += m_model->Model2Text();
}

void ModelWidget::Save2File()
{
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("All files (*.*)"));
    if (!str.isEmpty()) {
        setLastDir(str);
        QFile file(str);
        if (file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << m_logging << endl;
        }
    }
}

void ModelWidget::ChangeColor()
{
    QColor color = QColorDialog::getColor(tr("Choose Color for Series"));
    setColor(color);
}

void ModelWidget::setColor(const QColor& color)
{
    if (!color.isValid())
        return;
    if (m_model->SupportSeries()) {
        for (int i = 0; i < m_model_elements.size(); ++i) {
            m_model_elements[i]->ChangeColor(color);
        }
    } else {
        m_charts.signal_wrapper->Series(0)->setColor(color);
        m_charts.error_wrapper->Series(0)->setColor(color);
    }
    emit ColorChanged(color);
}

void ModelWidget::Restore()
{
    m_model->ImportModel(m_last_model);
    Repaint();
}

void ModelWidget::Detailed()
{
    QTextEdit* text = new QTextEdit;
    m_model->setFast(false);
    m_model->Calculate();
    text->setText("<html><pre>" + m_model->Data2Text() + "\n" + m_model->Model2Text() + "</ br>" + m_statistic_widget->Statistic() + "</pre></html>");
    m_dialogs->setWidget(text, tr("Detailed Information on Calculation Results"));
    m_dialogs->Attention();
}

QString ModelWidget::Keys() const
{
    QString label("");
    for (int i = 0; i < m_charts.signal_wrapper->SeriesSize(); ++i) {
        label += m_charts.signal_wrapper->Series(i)->name() + "|";
    }
    label.chop(1);
    return label;
}

#include "modelwidget.moc"

/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <charts.h>

#include "src/global.h"
#include "src/version.h"

#include "src/capabilities/jobmanager.h"

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"

#include "src/core/jsonhandler.h"
#include "src/core/libmath.h"
#include "src/core/minimizer.h"
#include "src/core/toolset.h"

#include "src/ui/dialogs/advancedsearch.h"
#include "src/ui/dialogs/configdialog.h"
#include "src/ui/dialogs/genericwidgetdialog.h"
#include "src/ui/dialogs/modaldialog.h"
#include "src/ui/dialogs/parameterdialog.h"
#include "src/ui/dialogs/resultsdialog.h"
#include "src/ui/dialogs/statisticdialog.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/guitools/flowlayout.h"
#include "src/ui/guitools/guitools.h"

#include "src/ui/mainwindow/chartwidget.h"

#include "src/ui/widgets/buttons/spinbox.h"
#include "src/ui/widgets/exportsimulationwidget.h"
#include "src/ui/widgets/modelactions.h"
#include "src/ui/widgets/modelchart.h"
#include "src/ui/widgets/modelelement.h"
#include "src/ui/widgets/optionswidget.h"
#include "src/ui/widgets/parameterwidget.h"
#include "src/ui/widgets/preparewidget.h"
#include "src/ui/widgets/results/mcresultswidget.h"
#include "src/ui/widgets/results/resultswidget.h"
#include "src/ui/widgets/results/searchresultwidget.h"
#include "src/ui/widgets/statisticwidget.h"
#include "src/ui/widgets/systemparameterwidget.h"
#include "src/ui/widgets/textwidget.h"

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QXYSeries>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRandomGenerator>
#include <QtCore/QSettings>
#include <QtCore/QSignalBlocker>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QtMath>

#include <QtGui/QAction>

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

#include <iostream>
#include <random>

#include "modelwidget.h"

class CPBar : public QWidget {
    Q_OBJECT
    qreal p; // progress 0.0 to 1.0
public:
    CPBar(QWidget* p = 0)
        : QWidget(p)
        , p(0)
    {
        setMinimumSize(21, 21);
    }
    void upd(qreal pp)
    {
        if (p == pp)
            return;
        p = pp;
        update();
    }
    void paintEvent(QPaintEvent*)
    {
        qreal pd = p * 360;
        qreal rd = 360 - pd;
        QPainter p(this);
        p.fillRect(rect(), Qt::white);
        p.translate(4, 4);
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath path, path2;
        path.moveTo(100, 0);
        path.arcTo(QRectF(0, 0, 20, 20), 90, -pd);
        QPen pen, pen2;
        pen.setCapStyle(Qt::FlatCap);
        pen.setColor(QColor("#30b7e0"));
        pen.setWidth(8);
        p.strokePath(path, pen);
        path2.moveTo(100, 0);
        pen2.setWidth(8);
        pen2.setColor(QColor("#d7d7d7"));
        pen2.setCapStyle(Qt::FlatCap);
        pen2.setDashPattern(QVector<qreal>{ 0.5, 1.105 });
        path2.arcTo(QRectF(0, 0, 200, 200), 90, rd);
        pen2.setDashOffset(2.2);
        p.strokePath(path2, pen2);
    }
};

ModelWidget::ModelWidget(QSharedPointer<AbstractModel> model, Charts charts, bool readonly, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_charts(charts)
    , m_pending(false)
    , m_minimizer(QSharedPointer<Minimizer>(new Minimizer(false, this), &QObject::deleteLater))
    , m_statistic(false)
    , m_val_readonly(readonly)
{
    m_splitter = new QSplitter(this);
    m_splitter->setObjectName("modelSplitter");
    m_splitter->setOrientation(Qt::Vertical);

    m_model_widget = new QWidget;
    m_script_timer = new QTimer;
    m_script_timer->setSingleShot(true);

    Data2Text();
    m_minimizer->setModel(m_model);

    m_advancedsearch = new AdvancedSearch(this);
    if (!m_model->isSimulation())
        m_advancedsearch->setModel(m_model);

    m_statistic_dialog = new StatisticDialog(m_model, this);

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

    m_layout = new QVBoxLayout;
    FlowLayout* const_layout = new FlowLayout;
    for (int i = 0; i < m_model->GlobalParameterSize(); ++i) {
        QGroupBox* group = new QGroupBox;
        group->setAlignment(Qt::AlignHCenter);
        group->setTitle(m_model->GlobalParameterName(i));
        QHBoxLayout* hlayout = new QHBoxLayout;
        QPointer<SpinBox> constant = new SpinBox;
        QPointer<QCheckBox> check = new QCheckBox;
        check->setChecked(true);
        m_constants << constant;
        constant->setDecimals(4);

        QPushButton *more_action = new QPushButton(tr("..."));
        more_action->setMaximumWidth(20);
        connect(more_action, &QPushButton::clicked, this, [this, i](){
            ParameterBoundary boundary = m_model.data()->getGlobalBoundary(i);
            double value = m_model->GlobalParameter(i);
            ParameterDialog dialog(boundary, value, m_model->SSE(), m_model->GlobalParameterName(i));
            if(dialog.exec() == QDialog::Accepted)
            {
                boundary = dialog.Boundary();
                value = dialog.Value();
                if (dialog.Bisection()) {
                    QSharedPointer<AbstractModel> model = m_model->Clone(false);
                    boundary = dialog.Boundary();
                    double result = NewtonRoot(model, i, boundary.lower_barrier, boundary.upper_barrier, 1e-7);
                    m_model->setGlobalParameter(result, i);
                    m_model->Calculate();
                } else {
                    m_model->UpdateGlobalBoundary(i, boundary);
                    m_model->setGlobalParameter(value, i);
                    m_model->Calculate();
                }
            }
        });

        constant->setPrefix(m_model->GlobalParameterPrefix(i));
        constant->setSingleStep(m_model->GlobalParameter(i) / 100);
        constant->setMaximum(1e9);
        constant->setMinimum(-1e9);
        constant->setValue(m_model->GlobalParameter(i));

        constant->setReadOnly(m_val_readonly);
        connect(constant, SIGNAL(valueChangedNotBySet(double)), this, SLOT(recalculate()));
        connect(m_model.data(), &AbstractModel::Recalculated, this,
            [i, constant, this, check]() {
                if (this->m_model && constant) {
                    if (this->m_model->GlobalEnabled(i)) {
                        constant->setStyleSheet("background-color: " + included());
                        check->setEnabled(true);
                        check->setChecked(m_model->GlobalTable()->isChecked(0, i));
                    } else {
                        check->setEnabled(false);
                        constant->setStyleSheet("background-color: " + excluded());
                    }
                }
            });
        connect(check, &QCheckBox::stateChanged, check, [i, this](int state) {
            if (this->m_model) {
                m_model->GlobalTable()->setChecked(0, i, state);
            }
        });
        connect(m_global_box, &QCheckBox::stateChanged, check, [check](int state) {
            check->setChecked(state);
        });

        hlayout->addWidget(constant);
        hlayout->addWidget(check);
        hlayout->addWidget(more_action);

        check->setHidden(m_model.data()->isSimulation());
        group->setLayout(hlayout);
        const_layout->addWidget(group);
    }

    m_local_box = new QCheckBox(tr("Local Parameter"));
    m_local_box->setChecked(true);

    m_global_box->setHidden(m_model->isSimulation() || m_val_readonly);
    m_local_box->setHidden(m_model->isSimulation() || m_val_readonly);

    m_minimize_all = new QPushButton(tr("Fit"));
    QAction* minimize_normal = new QAction(tr("Tight"), this);
    connect(minimize_normal, SIGNAL(triggered()), this, SLOT(GlobalMinimize()));

    QAction* minimize_loose = new QAction(tr("Progress"), this);
    connect(minimize_loose, SIGNAL(triggered()), this, SLOT(History()));

    QAction* fast_conf = new QAction(tr("Simplified MOC"), this);
    fast_conf->setToolTip(tr("Simplified Model Comparison, each parameter is varied independently of the remaining parameters."));
    connect(fast_conf, SIGNAL(triggered()), this, SLOT(FastConfidence()));

    QMenu* menu = new QMenu(m_minimize_all);
    menu->addAction(minimize_normal);
    menu->addAction(minimize_loose);
    menu->addAction(fast_conf);
    menu->setDefaultAction(minimize_normal);
    m_minimize_all->setMenu(menu);


    QGridLayout* fit_layout = new QGridLayout;
    fit_layout->addWidget(new ExportSimulationWidget(m_model, this), 0, 0);
    fit_layout->addWidget(m_global_box, 0, 1);
    fit_layout->addWidget(m_local_box, 0, 2);
    if (m_model->Type() != DataClassPrivate::Simulation)
        fit_layout->addWidget(m_minimize_all, 0, 3);

    QGroupBox* fitbox = new QGroupBox;
    fitbox->setTitle(tr("Fit model"));
    fitbox->setAlignment(Qt::AlignHCenter);
    fitbox->setLayout(fit_layout),

        m_layout->addWidget(fitbox);
    m_layout->addLayout(const_layout);


    QHBoxLayout* name_layout = new QHBoxLayout;
    name_layout->addWidget(new QLabel(tr("<h4>Model Name</h4>")));
    m_model_name = new QLineEdit;
    m_model_name->setText(m_model->Name());
    m_model_name->setPlaceholderText(SupraFit::Method2Name(m_model->SFModel()));
    m_model_name->setClearButtonEnabled(true);
    connect(m_model_name, &QLineEdit::textChanged, m_model.data(), &AbstractModel::setName);
    name_layout->addWidget(m_model_name);

    if (!m_model->SupportSeries()) {
        m_legend = new QCheckBox;
        m_legend->setText(tr("Show in Legend"));
        m_legend->setToolTip(tr("Show Line Series in legend"));
        m_legend->setChecked(false);
        connect(m_legend, &QCheckBox::stateChanged, this, [this](bool legend) {
            qobject_cast<LineSeries*>(m_charts.signal_wrapper->Series(0))->setShowInLegend(legend);
            qobject_cast<LineSeries*>(m_charts.error_wrapper->Series(0))->setShowInLegend(legend);
        });
        name_layout->addWidget(m_legend);
        QSpinBox* series = new QSpinBox;
        series->setRange(1, m_model->DependentModel()->rowCount());
        name_layout->addWidget(series);
        connect(series, &QSpinBox::valueChanged, this, [this](int value) {
            m_model->setAppliedSeries(value - 1);
        });
    }

    m_layout->addLayout(name_layout);

    QWidget* widget = new QWidget;
    widget->setLayout(m_layout);
    m_splitter->addWidget(widget);

    m_model_options_widget = new OptionsWidget(m_model);
    //m_layout->addWidget(m_model_options_widget);
    if (m_model->getAllOptions().size() == 0)
        m_model_options_widget->setMaximumHeight(0);
    //m_splitter->addWidget(m_model_options_widget);

    m_model_options = m_model_options_widget;

    m_layout = new QVBoxLayout;

    m_sign_layout = new QVBoxLayout;

    m_sign_layout->setAlignment(Qt::AlignTop);
    m_converged_label = new QLabel;

    m_readonly = new QCheckBox(tr("Read Only"));
    QHBoxLayout* head = new QHBoxLayout;

    if (!m_model->isSimulation())
        head->addWidget(m_converged_label);

    if (!m_val_readonly && !m_model->isSimulation())
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
        connect(m_model_name, &QLineEdit::textChanged, m_charts.signal_wrapper->Series(0), [this]() {
            if (m_model_name) {
                if (m_model_name->text() != this->m_charts.signal_wrapper->Series(0)->name()) {
                    this->m_charts.signal_wrapper->Series(0)->setName(m_model_name->text());
                    this->m_charts.error_wrapper->Series(0)->setName(m_model_name->text());
                }
            }
        });

        connect(m_charts.signal_wrapper->Series(0), &LineSeries::nameChanged, m_charts.signal_wrapper->Series(0), [this]() {
            this->m_charts.signal_wrapper->Series(0)->setName(m_model_name->text());
            this->m_charts.error_wrapper->Series(0)->setName(m_model_name->text());
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
    widget = new QWidget;

    QWidget* scroll = new QWidget;
    scroll->setLayout(m_sign_layout);
    QScrollArea* area = new QScrollArea;
    area->setWidgetResizable(true);
    area->setWidget(scroll);
    m_model_parameter = area;

    QTabWidget* model_tab = new QTabWidget;

    if (m_model->OptionsCount())
        model_tab->addTab(m_model_options, "Model Options");
    if (m_model->SystemParameterCount())
        AddSystemParameterTab(model_tab);
    if (m_model->SFModel() == SupraFit::ScriptModel)
        AddScriptModelTab(model_tab);

    m_splitter->addWidget(model_tab);

    if (m_model->LocalParameterSize())
        m_splitter->addWidget(m_model_parameter);

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
    connect(m_actions, &ModelActions::Charts, this, [this]() {
        this->m_charts_dialogs->Attention();
    });

    //m_splitter->addWidget(m_actions);
    m_actions->LocalEnabled(m_model->SupportSeries());
    m_actions->setSimulation(m_model->isSimulation());

    resizeButtons();

    //m_splitter->addWidget(m_statistic_widget);
    m_statistic_widget->setHidden(m_model->isSimulation());

    connect(m_splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(SplitterResized()));
    QVBoxLayout* vlayout = new QVBoxLayout;
    vlayout->addWidget(m_splitter);
    vlayout->addWidget(m_actions);
    vlayout->addWidget(m_statistic_widget);
    m_statistic_widget->setMaximumHeight(200);

    setLayout(vlayout);
    m_last_model = m_model->ExportModel(true, true);

    connect(m_model.data(), &AbstractModel::Recalculated, this, &ModelWidget::Repaint);
    connect(m_model.data(), &AbstractModel::ChartUpdated, this, &ModelWidget::ChartUpdated);

#pragma message("check if this Calculation is mandantory, it takes much time for Scripted Models")
    // if (m_model->SFModel() != SupraFit::ScriptModel)
    // m_model->Calculate();

    m_charts_dialogs = new ModalDialog(this);
    m_charts_dialogs->setWindowTitle("Collected Charts for " + m_model->Name() + " | " + qApp->instance()->property("projectname").toString());
    for (const QString& str : m_model->Charts()) {
        ModelChartWidget* w = new ModelChartWidget(m_model, str, this);
        m_charts_dialogs->setWidget(w, str);
    }
    m_jobmanager = new JobManager(this);
    m_jobmanager->setModel(m_model);

    connect(m_jobmanager, &JobManager::prepare, m_statistic_dialog, &StatisticDialog::MaximumSteps);
    connect(m_jobmanager, &JobManager::incremented, m_statistic_dialog, &StatisticDialog::IncrementProgress, Qt::DirectConnection);

    connect(m_jobmanager, &JobManager::prepare, m_advancedsearch, &AdvancedSearch::MaximumSteps);
    connect(m_jobmanager, &JobManager::incremented, m_advancedsearch, &AdvancedSearch::IncrementProgress, Qt::DirectConnection);

    connect(m_jobmanager, &JobManager::incremented, this, &ModelWidget::IncrementProgress, Qt::DirectConnection);
    connect(m_jobmanager, &JobManager::prepare, this, &ModelWidget::MaximumSteps);
    connect(m_jobmanager, &JobManager::Message, this, &ModelWidget::Message);
    //connect(m_jobmanager, &JobManager::started, this, &ModelWidget::started);
    //connect(m_jobmanager, &JobManager::finished, this, &ModelWidget::finished, Qt::DirectConnection);

    connect(m_jobmanager, &JobManager::started, m_statistic_dialog, &StatisticDialog::ShowWidget);
    connect(m_jobmanager, &JobManager::finished, m_statistic_dialog, &StatisticDialog::HideWidget);
    connect(m_jobmanager, &JobManager::finished, m_advancedsearch, &AdvancedSearch::HideWidget);

    connect(m_statistic_dialog, &StatisticDialog::Interrupt, m_jobmanager, &JobManager::Interrupt, Qt::DirectConnection);
    connect(m_advancedsearch, &AdvancedSearch::Interrupt, m_jobmanager, &JobManager::Interrupt, Qt::DirectConnection);

    connect(m_jobmanager, &JobManager::Message, m_statistic_dialog, &StatisticDialog::Message);

    connect(m_statistic_dialog, &StatisticDialog::RunCalculation, m_jobmanager, [this](const QJsonObject& job) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        this->m_jobmanager->AddSingleJob(job);
        this->m_jobmanager->RunJobs();
    });
    connect(m_jobmanager, &JobManager::ShowResult, this, [this](SupraFit::Method type, int index) {
        if (type != SupraFit::Method::FastConfidence) {
            this->m_results->Attention();
            this->m_results->ShowResult(type, index);
        }

        QApplication::restoreOverrideCursor();
    });

    //connect(m_advancedsearch, &AdvancedSearch::Interrupt, m_jobmanager, &JobManager::Interrupt); //, Qt::DirectConnection);
    connect(m_advancedsearch, &AdvancedSearch::RunCalculation, m_jobmanager, [this](const QJsonObject& job) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        this->m_jobmanager->AddSingleJob(job);
        this->m_jobmanager->RunJobs();
    });
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_SetUpFinished = true;
    /*
    if (m_model->isSimulation()) {
        QTimer::singleShot(10, this, &ModelWidget::recalculate);
    }
*/
    //    for (int i = 0; i < m_splitter->count(); ++i)
    //        m_splitter->setCollapsible(i, false);

    QString model_ident = QString("model %1").arg(m_model->SFModel());

    QSettings settings;
    settings.beginGroup("model");
    m_splitter->restoreState(settings.value(model_ident).toByteArray());
    settings.endGroup();
    emit m_model->Recalculated();
}

ModelWidget::~ModelWidget()
{
    SplitterResized();

    m_charts.signal_wrapper.clear();
    m_charts.error_wrapper.clear();
    m_charts.data_wrapper.clear();

    // m_minimizer.clear();

    m_statistic_dialog->hide();
    delete m_statistic_dialog;

    m_model.clear();
    delete m_model.data();
}

void ModelWidget::AddScriptModelTab(QTabWidget* model_tab)
{
    QPushButton* update = new QPushButton;
    update->setIcon(Icon("dialog-ok-apply"));
    QScrollArea* scrollArea = new QScrollArea;
    PrepareWidget* widget = new PrepareWidget(m_model->getModelDefinitionBlock(), false, this);
    QSignalBlocker block(widget);
    QProgressBar* bar = new QProgressBar;
    connect(m_script_timer, &QTimer::timeout, this, [this, widget, bar]() {
        if (!m_model)
            return;
        m_model->UpdateModelDefiniton(widget->getObject());
        bar->setValue(0);
    });
    auto timer = [bar, this]() {
        while (m_script_timer->isActive()) {
            QApplication::processEvents();
            bar->setValue(qApp->instance()->property("ScriptTimeout").toInt() - m_script_timer->remainingTime());
        }
    };
    connect(widget, &PrepareWidget::changed, this, [this, timer, bar]() {
        if (qApp->instance()->property("ScriptTimeout").toInt() < 1)
            return;
        m_script_timer->start(qApp->instance()->property("ScriptTimeout").toInt());
        bar->setMaximum(qApp->instance()->property("ScriptTimeout").toInt());
        timer();
    });
    widget->AddTableHeader(m_model->IndependentModel()->header());
    widget->AddTableHeader(m_model->DependentModel()->header());

    scrollArea->setWidget(widget);

    model_tab->addTab(scrollArea, "Model Definition");
    QPushButton* save = new QPushButton;
    save->setToolTip(tr("Click here to export the current model definition!"));
    connect(save, &QPushButton::clicked, this, [this]() {
        QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("Json File (*.json);;Binary (*.suprafit);;All files (*.*)"));
        if (!str.isEmpty()) {
            setLastDir(str);
            m_model->Calculate();
            qobject_cast<ScriptModel*>(m_model)->UpdateModelDefinition();
            QJsonObject json = m_model->ExportModel(false, false)["ModelDefinition"].toObject();
            JsonHandler::WriteJsonFile(json, str);
        }
    });

    connect(update, &QPushButton::clicked, this, [this, widget, bar]() {
        if (!m_model)
            return;
        m_model->UpdateModelDefiniton(widget->getObject());
        bar->setValue(0);
    });
    save->setIcon(Icon("document-save"));

    QPushButton* duplicate = new QPushButton;
    duplicate->setToolTip(tr("Duplicate current model"));
    duplicate->setIcon(Icon("list-add-blue"));
    connect(duplicate, &QPushButton::clicked, this, [this]() {
        m_model->Calculate();
        qobject_cast<ScriptModel*>(m_model)->UpdateModelDefinition();
        QJsonObject json = m_model->ExportModel(false, false)["ModelDefinition"].toObject();
        emit DuplicateScriptModel(json);
    });

    auto tabbar = model_tab->tabBar();
    tabbar->setTabButton(model_tab->count() - 1, QTabBar::LeftSide, bar);
    tabbar->setTabText(model_tab->count() - 1, "");

    QWidget* buttonBar = new QWidget;
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonBar->setLayout(buttonLayout);
    buttonLayout->addWidget(update);
    buttonLayout->addWidget(save);
    buttonLayout->addWidget(duplicate);

    tabbar->setTabButton(model_tab->count() - 1, QTabBar::RightSide, buttonBar);

    bar->setFormat("Model Definition");
    bar->setValue(0);
    bar->setMaximum(qApp->instance()->property("ScriptTimeout").toInt());
    bar->setMinimumWidth(150);
    model_tab->setStyleSheet("QTabBar::tab { height: 35px; width: 250px; }");
}

void ModelWidget::AddSystemParameterTab(QTabWidget* model_tab)
{
    if (m_model->getSystemParameterList().size() == 0)
        return;

    QPushButton* load = new QPushButton;
    load->setIcon(Icon("document-export"));
    load->setToolTip(tr("Click here to load experimental conditions / system parameters"));

    QPushButton* save = new QPushButton;
    save->setIcon(Icon("document-import"));
    save->setToolTip(tr("Click here to export the current experimental conditions / system parameters!"));
    connect(save, &QPushButton::clicked, this, [this]() {
        QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("Json File (*.json);;Binary (*.suprafit);;All files (*.*)"));
        if (!str.isEmpty()) {
            setLastDir(str);
            QJsonObject json = m_model->ExportData()["system"].toObject();
            JsonHandler::WriteJsonFile(json, str);
        }
    });
    m_system_parameter = new SPOverview(m_model.data(), m_val_readonly);

    model_tab->addTab(m_system_parameter, "System Parameter");
    auto tabbar = model_tab->tabBar();
    tabbar->setTabButton(model_tab->count() - 1, QTabBar::RightSide, save);
    tabbar->setTabButton(model_tab->count() - 1, QTabBar::LeftSide, load);

    connect(load, &QPushButton::clicked, this, [this]() {
        QString str = QFileDialog::getOpenFileName(this, tr("Save File"), getDir(), tr("Json File (*.json);;Binary (*.suprafit);;All files (*.*)"));
        if (!str.isEmpty()) {
            setLastDir(str);
            QJsonObject json;
            JsonHandler::ReadJsonFile(json, str);
            qDebug() << json;
            m_model->setSystemObject(json);
            m_model->LoadSystemParameter();
            emit m_model->SystemParameterChanged();
        }
    });
}

void ModelWidget::setColorList(const QString& str)
{
    QStringList colors = str.split("|");
    if (colors.size() == m_model_elements.size()) {
        for (int i = 0; i < m_model_elements.size(); ++i) {
            if (QColor(colors[i]).isValid())
                m_model_elements[i]->ChangeColor(QColor(colors[i]));
            else
                m_model_elements[i]->ChangeColor(Qt::green);
        }
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
    QString model_ident = QString("model %1").arg(m_model->SFModel());
    QSettings settings;
    settings.beginGroup("model");
    settings.setValue(model_ident, m_splitter->saveState());
    settings.endGroup();
}

void ModelWidget::resizeButtons()
{
    m_minimize_all->setMaximumSize(70, 30);
    m_minimize_all->setStyleSheet("background-color: #77d740;");
}

void ModelWidget::setParameter()
{
    if (m_model->isSimulation())
        return;
    for (int j = 0; j < m_model->GlobalParameterSize(); ++j) {
        if (qAbs(m_constants[j]->value() - m_model->GlobalParameter(j)) > 1e-5) // lets do no update if the model was calculated with the recently set constants
            m_constants[j]->setValue(m_model->GlobalParameter(j));
    }
    emit Update();
}

void ModelWidget::Repaint()
{
    if (!m_SetUpFinished)
        return;

    //if (m_model->Type() == 3)
    //    return;
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

void ModelWidget::History()
{
    QTabWidget* chart_tab = new QTabWidget;
    for (int i = 0; i < m_optimisationhistory.size(); ++i) {
        QList<QPointF> sse_points;
        QVector<QList<QPointF>> points(m_optimisationhistory[i].parameter[0].size());
        for (int j = 0; j < m_optimisationhistory[i].sse.size(); ++j) {
            sse_points.append(QPointF(j, m_optimisationhistory[i].sse[j]));
            for (int k = 0; k < points.size(); ++k) {
                points[k].append(QPointF(j, m_optimisationhistory[i].parameter[j][k]));
            }
        }
        LineSeries* sse = new LineSeries;
        sse->replace(sse_points);
        QList<LineSeries*> param;
        for (int k = 0; k < points.size(); ++k) {
            LineSeries* p = new LineSeries;
            p->replace(points[k]);
            param << p;
        }
        ListChart* chart = new ListChart;
        chart->addSeries(sse, 0, sse->color(), "Sum of Squared Errors", false);
        for (int k = 0; k < points.size(); ++k) {
            chart->addSeries(param[k], 1 + k, m_optimisationhistory[i].colors[k], m_optimisationhistory[i].names[k], false);
        }
        chart_tab->addTab(chart, QString("Optimisation No. %1").arg(1 + i));
    }
    GenericWidgetDialog dialog("Optimsation history", chart_tab, this);
    dialog.exec();
}

void ModelWidget::GlobalMinimize()
{
    QJsonObject config = m_model->getOptimizerConfig();
    MinimizeModel(config);
}

void ModelWidget::MinimizeModel(const QJsonObject& config)
{
    Waiter wait;
    if (m_pending)
        return;
    m_pending = true;

    CollectParameters();
    QJsonObject json = m_model->ExportModel(false, false);
    QList<int> locked = m_model->LockedParameters();
    m_minimizer->setParameter(json);

    m_model->setOptimizerConfig(config);
    int result;
    result = m_minimizer->Minimize();

    OptimisationHistory history = m_minimizer->History();
    int series = 0;
    for (int i = 0; i < m_model->OptimizeParameters().size(); ++i) {
        auto pair = m_model->IndexParameters(i);
        if (pair.second == 0) {
            history.names << m_model->GlobalParameterName(pair.first);
            history.colors << ChartWrapper::ColorCode(pair.first).toRgb().name();
        } else {
            history.names << m_model->LocalParameterName(pair.first);
            history.colors << m_charts.signal_wrapper->Series(series)->color().toRgb().name();

            series++;
            if (series >= m_model->SeriesCount())
                series = 0;
        }
    }
    m_optimisationhistory << history;
    json = m_minimizer->Parameter();
    m_last_model = json;
    m_model->ImportModel(json, false);
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
        emit m_model->Info()->Warning(tr("The optimization did not converge within the cycles! Rerun optimisation or increase number of steps."), 1);
    m_statistic = false;
    m_pending = false;
}

void ModelWidget::ToggleStatisticDialog()
{
    m_statistic_dialog->Attention();
}

void ModelWidget::FastConfidence()
{
    Waiter wait;

    QJsonObject job(ModelComparisonConfigBlock);

    job["FastConfidenceSteps"] = qApp->instance()->property("FastConfidenceSteps").toInt();
    job["FastConfidenceScaling"] = qApp->instance()->property("FastConfidenceScaling").toInt();
    qreal f_value = m_model.data()->finv(qApp->instance()->property("p_value").toDouble());
    qreal error = m_model.data()->SSE();
    job["MaxParameter"] = error * (f_value * m_model.data()->Parameter() / (m_model.data()->Points() - m_model.data()->Parameter()) + 1);
    job["confidence"] = qApp->instance()->property("p_value").toDouble();
    job["f_value"] = f_value;
    job["IncludeSeries"] = qApp->instance()->property("series_confidence").toBool();
    job["Method"] = SupraFit::Method::FastConfidence;

    m_jobmanager->AddSingleJob(job);
    m_jobmanager->RunJobs();
}

void ModelWidget::setJob(const QJsonObject& job)
{
    Waiter wait;
    m_jobmanager->AddSingleJob(job);
    m_jobmanager->RunJobs();
}

void ModelWidget::Interrupt()
{
    m_jobmanager->Interrupt();
}

void ModelWidget::LoadStatistic(const QJsonObject& data)
{
    int index = m_model->UpdateStatistic(data);
    m_results->Attention();
    m_statistic_dialog->HideWidget();
    SupraFit::Method type = SupraFit::Method(AccessCI(data["controller"].toObject(), "Method").toInt());
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
        QJsonObject config = model->getOptimizerConfig();
        model->setOptimizerConfig(config);
        m_minimizer->setModel(model);
        QString name = m_model->DependentModel()->headerData(i, Qt::Horizontal).toString();
        model->setName(tr("%1 (%2 Series %3)").arg(name).arg(model->Name().remove("Model")).arg(i + 1));
        result += m_minimizer->Minimize();
        QJsonObject json = m_minimizer->Parameter();
        emit AddModel(json);
        model.clear();
    }

    if (result < m_model->SeriesCount())
        emit m_model->Info()->Warning(tr("The optimization did not converge within the cycles! Rerun optimisation or increase number of steps."), 1);

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
            stream << m_logging << Qt::endl;
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

QColor ModelWidget::ActiveColor() const
{
    if (!m_model->SupportSeries())
        return m_charts.signal_wrapper->Series(0)->color();

    return QColor();
}

void ModelWidget::Restore()
{
    m_model->ImportModel(m_last_model, false);
    Repaint();
}

void ModelWidget::Detailed()
{
    TextWidget* text = new TextWidget;
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

void ModelWidget::ChartUpdated(const QString& str)
{
    if (!m_SetUpFinished)
        return;

    if (m_charts_dialogs->contains(str))
        return;
    ModelChartWidget* w = new ModelChartWidget(m_model, str, this);
    m_charts_dialogs->setWidget(w, str);
}

#include "modelwidget.moc"

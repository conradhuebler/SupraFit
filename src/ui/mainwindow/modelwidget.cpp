/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/models/scriptmodel.h" // ScriptModel class (was transitive via models.h)

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
#include "src/ui/guitools/gripsplitter.h"
#include "src/ui/guitools/sectionstack.h"
#include "src/ui/guitools/windowhost.h"
#include "src/ui/guitools/guitools.h"

#include "src/ui/mainwindow/chartwidget.h"

#include "src/ui/widgets/buttons/spinbox.h"
#include "src/ui/widgets/exportsimulationwidget.h"
#include "src/ui/widgets/modelactions.h"
#include "src/ui/widgets/modelchart.h"
#include "src/ui/widgets/dynamicmodelwidget.h"
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
#include <QtGui/QActionGroup>

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
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTableView>

#include <iostream>
#include <random>

#include "src/core/phasetiming.h"

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
        p.fillRect(rect(), palette().color(QPalette::Base));
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
        pen2.setColor(palette().color(QPalette::Mid));
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
    // Claude Generated - Debug Messages for successful model loading
    if (model) {
        qDebug() << "✅ ModelWidget: Model successfully loaded -" 
                 << "Name:" << model->Name()
                 << "ID:" << model->SFModel()
                 << "UUID:" << model->UUID()
                 << "Parameters:" << model->GlobalParameterSize() << "global," << model->LocalParameterSize() << "local";
    } else {
        qDebug() << "❌ ModelWidget: Null model passed to constructor";
    }
    m_splitter = new GripSplitter(Qt::Vertical, this);
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

    connect(this, &ModelWidget::ToggleSeries, m_charts.error_wrapper.data(), &ChartWrapper::SetBlocked);
    connect(this, &ModelWidget::ToggleSeries, m_charts.signal_wrapper.data(), &ChartWrapper::SetBlocked);


    m_statistic_widget = new StatisticWidget(m_model, this);

    m_results = new ResultsDialog(m_model, m_charts.signal_wrapper.data(), this);
    connect(m_results, &ResultsDialog::LoadModel, this, &ModelWidget::LoadJson);
    connect(m_results, &ResultsDialog::AddModel, this, &ModelWidget::AddModel);

    m_global_box = new QCheckBox(tr("Global Parameter"));
    m_global_box->setChecked(true);

    m_layout = new QVBoxLayout;
    // Global parameters as a grid that uses the full width: up to four per row, equal column widths.
    QGridLayout* const_layout = new QGridLayout;
    const int const_columns = qMax(1, qMin(4, m_model->GlobalParameterSize()));
    for (int c = 0; c < const_columns; ++c)
        const_layout->setColumnStretch(c, 1);
    for (int i = 0; i < m_model->GlobalParameterSize(); ++i) {
        // name (rich text, e.g. lg K<sub>11</sub>) in front of the value, all in one framed row
        QGroupBox* group = new QGroupBox;
        QHBoxLayout* hlayout = new QHBoxLayout;
        QLabel* name = new QLabel(QString("<html><b>%1</b></html>").arg(m_model->GlobalParameterName(i)));
        hlayout->addWidget(name);
        QPointer<SpinBox> constant = new SpinBox;
        QPointer<QCheckBox> check = new QCheckBox;
        check->setChecked(true);
        m_constants << constant;
        constant->setDecimals(4);

        QPushButton *more_action = new QPushButton(tr("..."));
        more_action->setMaximumWidth(24);
        more_action->setToolTip(tr("Boundaries of %1 and root search").arg(m_model->GlobalParameterName(i)));
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
                        constant->setStyleSheet("background-color: " + included() + " color: black;");
                        check->setEnabled(true);
                        check->setChecked(m_model->GlobalTable()->isChecked(0, i));
                    } else {
                        check->setEnabled(false);
                        constant->setStyleSheet("background-color: " + excluded() + " color: black;");
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

        if (m_model.data()->isSimulation())
            check->hide(); // parentless until group->setLayout(): setHidden(false) would show it as a window
        check->setToolTip(tr("Optimise %1 in the fit").arg(m_model->GlobalParameterName(i)));
        hlayout->setContentsMargins(6, 2, 6, 4);
        constant->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        group->setLayout(hlayout);
        const_layout->addWidget(group, i / const_columns, i % const_columns);
    }

    m_local_box = new QCheckBox(tr("Local Parameter"));
    m_local_box->setChecked(true);

    if (m_model->isSimulation() || m_val_readonly) { // hide() only: both boxes are parentless until fit_flow takes them
        m_global_box->hide();
        m_local_box->hide();
    }

    m_minimize_all = new QPushButton(tr("Fit"));
    QAction* minimize_normal = new QAction(tr("Tight"), this);
    connect(minimize_normal, &QAction::triggered, this, &ModelWidget::GlobalMinimize);

    QAction* minimize_loose = new QAction(tr("Progress"), this);
    connect(minimize_loose, &QAction::triggered, this, &ModelWidget::History);

    QAction* fast_conf = new QAction(tr("Simplified MOC"), this);
    fast_conf->setToolTip(tr("Simplified Model Comparison, each parameter is varied independently of the remaining parameters."));
    connect(fast_conf, &QAction::triggered, this, &ModelWidget::FastConfidence);

    /* Fit-solver selection (LevMar vs. VarPro), appended to the Fit menu. The choice is written into the
       model's optimizer config ("FitSolver") and thus propagates to the statistical post-processing,
       whose fold re-fits inherit the key via Clone(). VarPro is only honoured by models with
       SupportsVarPro() (nmr_any / uvvis_any); it is greyed out otherwise. Claude Generated. */
    QAction* solver_levmar = new QAction(tr("LevMar (classic)"), this);
    solver_levmar->setCheckable(true);
    solver_levmar->setData(QStringLiteral("LevMar"));
    QAction* solver_varpro = new QAction(tr("VarPro (projection)"), this);
    solver_varpro->setCheckable(true);
    solver_varpro->setData(QStringLiteral("VarPro"));
    QActionGroup* solver_group = new QActionGroup(this);
    solver_group->setExclusive(true);
    solver_group->addAction(solver_levmar);
    solver_group->addAction(solver_varpro);
    m_solver_levmar = solver_levmar;
    m_solver_varpro = solver_varpro;
    connect(solver_levmar, &QAction::triggered, this, [this]() {
        SetFitSolver(QStringLiteral("LevMar"));
        GlobalMinimize(); /* selecting a solver immediately (re)fits with it — Claude Generated */
    });
    connect(solver_varpro, &QAction::triggered, this, [this]() {
        SetFitSolver(QStringLiteral("VarPro"));
        GlobalMinimize(); /* selecting a solver immediately (re)fits with it — Claude Generated */
    });

    /* Speciation-solver selection (LevMar/Newton vs. legacy BFGS) for the reaction-driven *_any models.
       Writes the "SpeciationSolver" optimizer-config key, which AbstractTitrationModel/itc_any push into
       their embedded SpeciationEngine on setOptimizerConfig(); inherited by MC/CV/RA via Clone(). Only
       enabled for models with UsesSpeciationEngine() (the fixed models use closed-form roots). CG. */
    QAction* spec_levmar = new QAction(tr("LevMar / Newton (default)"), this);
    spec_levmar->setCheckable(true);
    spec_levmar->setData(QStringLiteral("LevMar"));
    QAction* spec_bfgs = new QAction(tr("BFGS (legacy)"), this);
    spec_bfgs->setCheckable(true);
    spec_bfgs->setData(QStringLiteral("BFGS"));
    QActionGroup* spec_group = new QActionGroup(this);
    spec_group->setExclusive(true);
    spec_group->addAction(spec_levmar);
    spec_group->addAction(spec_bfgs);
    m_speciation_levmar = spec_levmar;
    m_speciation_bfgs = spec_bfgs;
    connect(spec_levmar, &QAction::triggered, this, [this]() {
        SetSpeciationSolver(QStringLiteral("LevMar"));
        GlobalMinimize(); /* re-fit with the chosen speciation solver — Claude Generated */
    });
    connect(spec_bfgs, &QAction::triggered, this, [this]() {
        SetSpeciationSolver(QStringLiteral("BFGS"));
        GlobalMinimize(); /* re-fit with the chosen speciation solver — Claude Generated */
    });

    QAction* global_search = new QAction(tr("Global Search..."), this);
    global_search->setToolTip(tr("Scan the parameter space from many start values (opens the Scan window)"));
    connect(global_search, &QAction::triggered, this, &ModelWidget::OpenAdvancedSearch);

    QAction* local_fits = new QAction(tr("Local Fits"), this);
    local_fits->setToolTip(tr("Fit every data series on its own (one model per series)"));
    local_fits->setEnabled(m_model->SupportSeries());
    connect(local_fits, &QAction::triggered, this, &ModelWidget::LocalMinimize);

    QMenu* menu = new QMenu(m_minimize_all);
    menu->addAction(minimize_normal);
    menu->addAction(minimize_loose);
    menu->addAction(fast_conf);
    menu->addAction(global_search);
    menu->addAction(local_fits);
    menu->addSeparator();
    menu->addAction(solver_levmar);
    menu->addAction(solver_varpro);
    QMenu* spec_menu = menu->addMenu(tr("Speciation solver"));
    spec_menu->setToolTip(tr("Equilibrium-concentration solver used by the reaction-driven models."));
    spec_menu->addAction(spec_levmar);
    spec_menu->addAction(spec_bfgs);
    m_speciation_menu = spec_menu;
    connect(menu, &QMenu::aboutToShow, this, &ModelWidget::UpdateSolverMenu);
    menu->setDefaultAction(minimize_normal);
    m_minimize_all->setMenu(menu);


    m_global_box->setToolTip(tr("Optimise the global parameters (stability constants)"));
    m_local_box->setToolTip(tr("Optimise the local parameters (signals of the pure species)"));
    m_minimize_all->setToolTip(tr("Fit the model to the data; the arrow offers fit variants and solvers"));

    /* Header (model pane, top to bottom; Claude Generated, UI redesign WP7):
     *   model name (large, centred, editable)
     *   fit row: Global/Local checkboxes, Fit, convergence status, Read Only
     *   global parameters (grid over the full width)
     *   action buttons (Scan … Charts)
     * Goodness-of-fit numbers live with the fit results, the simulated-data exports in their own tab. */
    m_model_name = new QLineEdit;
    m_model_name->setText(m_model->Name());
    m_model_name->setPlaceholderText(SupraFit::Method2Name(m_model->SFModel()));
    m_model_name->setClearButtonEnabled(true);
    m_model_name->setAlignment(Qt::AlignCenter);
    m_model_name->setFrame(false);
    m_model_name->setToolTip(tr("Model name, shown in the tab, the charts and the results; click to edit"));
    {
        QFont font = m_model_name->font();
        font.setPointSizeF(font.pointSizeF() * 1.35);
        font.setBold(true);
        m_model_name->setFont(font);
    }
    m_model_name->setBackgroundRole(QPalette::Window);
    connect(m_model_name, &QLineEdit::textChanged, m_model.data(), &AbstractModel::setName);

    QHBoxLayout* name_layout = new QHBoxLayout;
    name_layout->addWidget(m_model_name, 1);

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
        series->setPrefix(tr("Series "));
        series->setToolTip(tr("Data series the model is applied to"));
        name_layout->addWidget(series);
        connect(series, &QSpinBox::valueChanged, this, [this](int value) {
            m_model->setAppliedSeries(value - 1);
        });
    }

    m_converged_label = new QLabel;
    m_converged_label->setToolTip(tr("State of the last optimisation"));
    m_readonly = new QCheckBox(tr("Read Only"));
    m_readonly->setToolTip(tr("Lock the local parameters of all data series against editing"));

    // Fit row: the options and the status flow on the left (and wrap when narrow), the Fit button
    // keeps its place at the right end of the first line.
    QWidget* fit_options = new QWidget;
    FlowLayout* fit_flow = new FlowLayout(0, 12, 4);
    fit_flow->addWidget(m_global_box);
    fit_flow->addWidget(m_local_box);
    if (!m_model->isSimulation())
        fit_flow->addWidget(m_converged_label);
    if (!m_val_readonly && !m_model->isSimulation() && m_model->LocalParameterSize())
        fit_flow->addWidget(m_readonly);
    fit_options->setLayout(fit_flow);
    QHBoxLayout* fit_row = new QHBoxLayout;
    fit_row->addWidget(fit_options, 1);
    if (m_model->Type() != DataClassPrivate::Simulation)
        fit_row->addWidget(m_minimize_all, 0, Qt::AlignTop | Qt::AlignRight);

    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->addLayout(name_layout);
    m_layout->addLayout(fit_row);
    m_layout->addLayout(const_layout);

    m_simulation_export = new ExportSimulationWidget(m_model, this);

    m_model_options_widget = new OptionsWidget(m_model);
    //m_layout->addWidget(m_model_options_widget);
    if (m_model->getAllOptions().size() == 0)
        m_model_options_widget->setMaximumHeight(0);
    //m_splitter->addWidget(m_model_options_widget);

    m_model_options = m_model_options_widget;

    m_sign_layout = new QVBoxLayout;

    m_sign_layout->setAlignment(Qt::AlignTop);
    m_sign_layout->setSpacing(4);
    m_sign_layout->setContentsMargins(2, 2, 2, 2);
#ifdef DEBUG_ON
    // Diagnostic for scripted models: these values decide whether the per-series parameter widgets
    // (shift editors + optimise checkboxes) are built at all. Claude Generated.
    if (m_model->SFModel() == SupraFit::ScriptModel) {
        QString active, checked;
        for (int s = 0; s < m_model->SeriesCount(); ++s)
            active += m_model->ActiveSignals(s) ? QStringLiteral("1") : QStringLiteral("0");
        for (int s = 0; s < m_model->LocalTable()->rowCount(); ++s)
            for (int p = 0; p < m_model->LocalTable()->columnCount(); ++p)
                checked += m_model->LocalTable()->isChecked(s, p) ? QStringLiteral("1") : QStringLiteral("0");
        qDebug().noquote() << QString("[ScriptModel] SupportSeries=%1 LocalParameterSize=%2 GlobalParameterSize=%3 SeriesCount=%4 ChartSeries=%5 optParams=%6 activeSignals=%7 localChecked=%8")
                                  .arg(m_model->SupportSeries())
                                  .arg(m_model->LocalParameterSize())
                                  .arg(m_model->GlobalParameterSize())
                                  .arg(m_model->SeriesCount())
                                  .arg(m_charts.signal_wrapper ? m_charts.signal_wrapper->SeriesSize() : -1)
                                  .arg(m_model->CollectOptimizationParameters().size())
                                  .arg(active)
                                  .arg(checked);
    }
#endif

    if (m_model->SupportSeries()) {
        if (m_model->LocalParameterSize()) {
            for (int i = 0; i < m_charts.signal_wrapper->SeriesSize(); ++i) {
                ModelElement* el = new ModelElement(m_model, m_charts, i);
                connect(el, &ModelElement::ValueChanged, this, &ModelWidget::recalculate);
                connect(el, &ModelElement::ActiveSignalChanged, this, &ModelWidget::CollectActiveSignals);
                connect(this, &ModelWidget::Update, el, &ModelElement::Update);
                connect(this, &ModelWidget::ToggleSeries, el, &ModelElement::ToggleSeries);

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
    QWidget* scroll = new QWidget;
    scroll->setLayout(m_sign_layout);
    QScrollArea* area = new QScrollArea;
    area->setWidgetResizable(true);
    area->setWidget(scroll);
    m_model_parameter = area;

    // Model pages (options, system parameters, definition, table, simulated data) behind a slim list
    // on the left instead of a tab bar: no extra row of height. Claude Generated (UI redesign WP8).
    SectionStack* model_tab = new SectionStack;
    m_pages = model_tab;

    // Statistic tools and the model's own charts (e.g. concentration profiles) are pages of the list, no
    // windows of their own. Both dialogs are used as plain widgets here. Claude Generated (WP1b).
    m_statistic_dialog->setEmbedded(true);
    connect(m_statistic_dialog, &StatisticDialog::AttentionRequested, this, [this]() { OpenStatistics(false); });
    m_charts_dialogs = new ModalDialog(this);
    m_charts_dialogs->setEmbedded(true);
    m_charts_dialogs->setWindowTitle("Collected Charts for " + m_model->Name() + " | " + qApp->instance()->property("projectname").toString());

    // The data series are a page of the list as well (default). Pinned they stay beside the pages as before;
    // unpinned they use the whole page area, which suits many series. Built here, placed after the pages.
    QGroupBox* series_box = nullptr;
    QWidget* series_host = nullptr;
    int series_page = -1;
    QToolButton* series_pin = nullptr;
    if (m_model->LocalParameterSize()) {
        series_box = new QGroupBox(tr("Data series"));
        series_box->setObjectName("series_box");
        series_box->setToolTip(tr("Local parameters of each data series, their inclusion in the fit and their appearance in the charts"));
        series_pin = new QToolButton;
        series_pin->setObjectName("series_pin");
        series_pin->setText(tr("Pin"));
        series_pin->setCheckable(true);
        series_pin->setAutoRaise(true);
        series_pin->setToolTip(tr("Keep the data series beside the pages instead of listing them as a page"));
        QVBoxLayout* series_layout = new QVBoxLayout;
        series_layout->setContentsMargins(4, 4, 4, 4);
        QHBoxLayout* series_top = new QHBoxLayout;
        series_top->addStretch();
        series_top->addWidget(series_pin);
        series_layout->addLayout(series_top);
        series_layout->addWidget(m_model_parameter);
        series_box->setLayout(series_layout);

        series_host = new QWidget;
        QVBoxLayout* host_layout = new QVBoxLayout;
        host_layout->setContentsMargins(0, 0, 0, 0);
        series_host->setLayout(host_layout);
        series_page = model_tab->addPage(series_host, tr("Data series"), nullptr, Icon("view-list-text"));
    }

    if (m_model->OptionsCount())
        model_tab->addPage(m_model_options, tr("Model Options"), nullptr, Icon("configure"));
    if (m_model->SystemParameterCount())
        AddSystemParameterTab(model_tab);
    if (m_model->SFModel() == SupraFit::ScriptModel)
        AddScriptModelTab(model_tab);
    // Scalable table view for models with many species (additive: the classic view is unchanged).
    if (m_model->UseDynamicParameterWidget())
        model_tab->addPage(new DynamicModelWidget(m_model, this), tr("Parameter Table"), nullptr, Icon("kspread"));

    if (!m_model->isSimulation()) {
        // One page for everything about statistics: the analyses to start and the finished results. The list
        // replaces the page list while the page is shown (SectionStack group page); the page itself switches
        // between the statistic dialog and the results view.
        m_results->setEmbedded(true);
        connect(m_results, &ResultsDialog::AttentionRequested, this, [this]() { OpenStatistics(true); });
        m_statistic_stack = new QStackedWidget;
        m_statistic_stack->addWidget(m_statistic_dialog);
        m_statistic_stack->addWidget(m_results);
        m_stat_page = model_tab->addGroupPage(
            m_statistic_stack, tr("Statistics"),
            [this]() {
                QList<SectionStack::SubEntry> rows;
                for (const StatEntry& e : StatisticEntries())
                    rows << SectionStack::SubEntry{ e.title, e.kind == 0, e.icon, e.tooltip };
                return rows;
            },
            [this](int row) {
                const QList<StatEntry> entries = StatisticEntries();
                if (row < 0 || row >= entries.size())
                    return;
                const StatEntry& e = entries[row];
                if (e.kind == 1) {
                    m_statistic_stack->setCurrentIndex(0);
                    m_statistic_dialog->setCurrentTab(e.index);
                } else if (e.kind == 2) {
                    m_statistic_stack->setCurrentIndex(1);
                    m_results->ShowEntry(e.type, e.result);
                }
            },
            [this](int row, const QPoint& pos) {
                const QList<StatEntry> entries = StatisticEntries();
                if (row < 0 || row >= entries.size() || entries[row].kind != 2)
                    return;
                const StatEntry e = entries[row];
                QMenu menu;
                menu.addAction(tr("Open in window"), this, [this, e]() { m_results->DetachEntry(e.type, e.result); });
                menu.addSeparator();
                menu.addAction(tr("Save data"), this, [this, e]() { m_results->SaveEntry(e.type, e.result); });
                menu.addAction(tr("Drop raw data"), this, [this, e]() { m_results->DropRawDataOf(e.type, e.result); });
                menu.addAction(tr("Remove"), this, [this, e]() { m_results->RemoveEntry(e.type, e.result); });
                menu.exec(pos);
            },
            Icon("applications-education-science"));
        connect(m_results, &ResultsDialog::ListChanged, this, [this]() { m_pages->refreshGroup(m_stat_page); });
        m_statistic_dialog->show(); // hidden by its constructor; a layout does not show an explicitly hidden widget
    }
    // The model's own charts (concentration profiles, heat charts): one entry per chart in the list that replaces the
    // page list, like the statistic tools; no tabs inside. The page is listed once there is a chart.
    m_charts_page = model_tab->addGroupPage(
        m_charts_dialogs, tr("Charts"),
        [this]() {
            QList<SectionStack::SubEntry> rows;
            for (const QString& name : m_charts_dialogs->TabNames()) {
                QString title = name;
                title.replace(QStringLiteral(" Chart"), QString()); // "Concentration Chart" -> "Concentration"
                rows << SectionStack::SubEntry{ title, false, Icon("charts"), title == name ? QString() : name };
            }
            return rows;
        },
        [this](int row) { m_charts_dialogs->setCurrentTab(row); },
        [this](int row, const QPoint& pos) {
            QMenu menu;
            menu.addAction(tr("Close chart"), this, [this, row]() {
                m_charts_dialogs->CloseTab(row);
                if (m_charts_dialogs->Count() == 0)
                    m_pages->setPageHidden(m_charts_page, true);
                else
                    m_pages->refreshGroup(m_charts_page);
            });
            menu.exec(pos);
        },
        Icon("charts"));
    m_charts_dialogs->show();
    model_tab->setPageHidden(m_charts_page, m_model->Charts().isEmpty());

    // The detailed data (input data, model, parameters and statistics as text) is a page like the others; it is
    // written when the page is chosen and follows a recalculation while it is shown.
    m_detailed_text = new TextWidget;
    m_detailed_timer = new QTimer(this);
    m_detailed_timer->setSingleShot(true);
    connect(m_detailed_timer, &QTimer::timeout, this, [this]() { RefreshDetailed(false); });
    connect(m_model.data(), &AbstractModel::Recalculated, this, [this]() {
        // follow a recalculation while the page is shown; not while a fit runs (fast mode), and not with the
        // statistics, which take their time: those are rewritten when the page is opened or the box is toggled
        if (m_pages && m_pages->currentTitle() == tr("Detailed") && !m_model->isFast() && !m_detailed_statistics->isChecked())
            m_detailed_timer->start(200); // one refresh for a series of recalculations
    });
    // The statistical analyses of the stored results (Monte Carlo, cross validation, ...) are the expensive part of
    // this text (BC50 integrals for every result): they are an opt-in in the header of the page, remembered.
    QWidget* detailed_tools = new QWidget;
    QHBoxLayout* detailed_tools_layout = new QHBoxLayout(detailed_tools);
    detailed_tools_layout->setContentsMargins(0, 0, 0, 0);
    m_detailed_statistics = new QCheckBox(tr("Include the statistical analyses"));
    m_detailed_statistics->setToolTip(tr("Adds the analysis of every stored result (Monte Carlo, cross validation, ...). This can take a while."));
    m_detailed_statistics->setChecked(QSettings().value(QStringLiteral("ui/detailed_statistics"), false).toBool());
    connect(m_detailed_statistics, &QCheckBox::toggled, this, [this](bool on) {
        QSettings().setValue(QStringLiteral("ui/detailed_statistics"), on);
        RefreshDetailed(false);
    });
    detailed_tools_layout->addWidget(m_detailed_statistics);
    model_tab->addPage(m_detailed_text, tr("Detailed"), detailed_tools, Icon("help-about"));

    // Simulated data sets from this model (drag an entry onto the project list).
    {
        QWidget* page = new QWidget;
        QVBoxLayout* page_layout = new QVBoxLayout;
        page_layout->setContentsMargins(0, 0, 0, 0);
        QLabel* hint = new QLabel(tr("Drag an entry onto the project list to create a data set simulated from this model."));
        hint->setWordWrap(true);
        page_layout->addWidget(hint);
        // Without the advanced setting only part of the simulation tools is offered: say where the rest is.
        QLabel* more = new QLabel(tr("More simulation tools are shown with the setting \"Show advanced options and simulation tools in GUI\"."));
        more->setWordWrap(true);
        more->setEnabled(false);
        if (qApp->instance()->property("advanced_ui").toBool())
            more->hide(); // never setVisible(true) here: the label has no parent yet and would flash as a window
        connect(m_simulation_export, &ExportSimulationWidget::AdvancedChanged, more, [more](bool advanced) { more->setVisible(!advanced); });
        page_layout->addWidget(more);
        page_layout->addWidget(m_simulation_export);
        page_layout->addStretch();
        page->setLayout(page_layout);
        model_tab->addPage(page, tr("Simulated Data"), nullptr, Icon("window-duplicate"));
    }

    m_actions = new ModelActions;
    m_actions->setPageNavigation(true); // Statistics, Results and Charts are pages of the list, the global search is in the Fit menu

    connect(m_actions, &ModelActions::NewGuess, this, &ModelWidget::NewGuess);
    connect(m_actions, &ModelActions::OptimizerSettings, this, &ModelWidget::OptimizerSettings);
    connect(m_actions, &ModelActions::ImportConstants, this, &ModelWidget::ImportConstants);
    connect(m_actions, &ModelActions::ExportConstants, this, &ModelWidget::ExportConstants);
    connect(m_actions, &ModelActions::Save2File, this, &ModelWidget::Save2File);
    connect(m_actions, &ModelActions::ExportSimModel, this, &ModelWidget::ExportSimModel);
    connect(m_actions, &ModelActions::Restore, this, &ModelWidget::Restore);

    //m_splitter->addWidget(m_actions);
    m_actions->LocalEnabled(m_model->SupportSeries());
    m_actions->setSimulation(m_model->isSimulation());

    resizeButtons();

    /* Layout (operator mockup): header with name, fit row, global parameters and the action buttons on
     * top; below it the model tabs (options, system parameters, simulated data, …) and the data series
     * (beside each other when there is room), and the fit results across the bottom. Both splitters are
     * user-adjustable and stored per model type. Claude Generated. */
    m_layout->addWidget(m_actions);
    QWidget* header = new QWidget;
    header->setLayout(m_layout);

    m_series_splitter = new GripSplitter(Qt::Horizontal);
    m_series_splitter->setObjectName("modelSeriesSplitter");
    m_series_splitter->addWidget(model_tab);
    model_tab->setMinimumWidth(300);
    // Where the data series sit: beside the pages (pinned) or as a page of the list. Reparenting happens
    // before the first show; on a later toggle the widget is shown again explicitly after reparenting.
    const QString pin_key = QStringLiteral("model/model %1 series pinned").arg(m_model->SFModel());
    auto placeSeries = [this, model_tab, series_box, series_host, series_page](bool pinned, bool shown) {
        if (pinned) {
            m_series_splitter->addWidget(series_box);
            m_series_splitter->setStretchFactor(0, 2);
            m_series_splitter->setStretchFactor(1, 3);
            series_box->setTitle(tr("Data series"));
            series_box->setFlat(false);
        } else {
            series_host->layout()->addWidget(series_box);
            series_box->setTitle(QString());
            series_box->setFlat(true);
        }
        model_tab->setPageHidden(series_page, pinned);
        if (shown)
            series_box->show();
    };
    if (series_box) {
        const bool pinned = QSettings().value(pin_key, false).toBool();
        series_pin->setChecked(pinned);
        placeSeries(pinned, false);
        connect(series_pin, &QToolButton::toggled, this, [this, model_tab, series_page, placeSeries, pin_key](bool pinned) {
            placeSeries(pinned, true);
            if (pinned)
                UpdateSeriesOrientation(); // beside or below, with the sizes last used in that direction
            else
                model_tab->setCurrentIndex(series_page);
            QSettings().setValue(pin_key, pinned);
            SplitterResized();
        });
    }
    m_splitter->addWidget(m_series_splitter);

    QGroupBox* results_box = new QGroupBox(tr("Fit results"));
    results_box->setToolTip(tr("Goodness of fit and statistics of the current model"));
    QVBoxLayout* results_layout = new QVBoxLayout;
    results_layout->setContentsMargins(4, 4, 4, 4);
    results_layout->addWidget(m_simulation_export->MetricsWidget());
    results_layout->addWidget(m_statistic_widget);
    m_statistic_widget->setMinimumHeight(140);
    results_box->setLayout(results_layout);
    if (m_model->isSimulation())
        results_box->hide(); // parentless until m_splitter->addWidget: setHidden(false) would show a window
    m_splitter->addWidget(results_box);
    m_splitter->setStretchFactor(0, m_model->LocalParameterSize() ? 2 : 1);
    m_splitter->setStretchFactor(1, 1);

    m_statistic_widget->setHidden(m_model->isSimulation());

    connect(m_splitter, &QSplitter::splitterMoved, this, &ModelWidget::SplitterResized);
    connect(m_series_splitter, &QSplitter::splitterMoved, this, &ModelWidget::SplitterResized);
    QVBoxLayout* vlayout = new QVBoxLayout;
    vlayout->setContentsMargins(4, 4, 4, 4);
    vlayout->addWidget(header);
    vlayout->addWidget(m_splitter, 1);

    // The whole pane scrolls when it gets less room than its parts ask for (header, page list, fit results):
    // a 1024x600 screen has no use for a pane that insists on 1000 px of height. Claude Generated (WP7).
    QWidget* content = new QWidget;
    content->setLayout(vlayout);
    QScrollArea* pane = new QScrollArea;
    pane->setObjectName("model_pane_scroll");
    pane->setWidgetResizable(true);
    pane->setFrameShape(QFrame::NoFrame);
    pane->setWidget(content);
    QVBoxLayout* outer = new QVBoxLayout;
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(pane);
    setLayout(outer);
    m_last_model = m_model->ExportModel(true, true);

    connect(m_model.data(), &AbstractModel::Recalculated, this, &ModelWidget::Repaint);
    connect(m_model.data(), &AbstractModel::ChartUpdated, this, &ModelWidget::ChartUpdated);

#pragma message("check if this Calculation is mandantory, it takes much time for Scripted Models")
    // if (m_model->SFModel() != SupraFit::ScriptModel)
    // m_model->Calculate();

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
        // Phase timing starts at the click, not inside the job: the table generation that precedes
        // the first worker thread is part of what the user waits for. Claude Generated.
        PhaseTiming::Begin(QString("%1 on %2")
                               .arg(SupraFit::Method2Name(AccessCI(job, "Method").toInt()))
                               .arg(m_model->Name()));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        this->m_jobmanager->AddSingleJob(job);
        this->m_jobmanager->RunJobs();
    });
    connect(m_jobmanager, &JobManager::ShowResult, this, [this](SupraFit::Method type, int index) {
        if (type != SupraFit::Method::FastConfidence)
            ShowStatisticResult(type, index);

        QApplication::restoreOverrideCursor();
        // The chart is on screen now — this is the end of what the user perceives as "running".
        PhaseTiming::Mark(QStringLiteral("render results widget + chart"));
        PhaseTiming::End();
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
    // No pane may collapse to nothing: the per-series parameter pane (the shift editors) is the last
    // one added and would otherwise silently vanish. Claude Generated.
    for (QSplitter* splitter : { m_splitter, m_series_splitter })
        for (int i = 0; i < splitter->count(); ++i)
            splitter->setCollapsible(i, false);

    QSettings settings;
    settings.beginGroup("model");
    m_splitter->restoreState(settings.value(SplitterSettingsKey()).toByteArray());
    // the state of the series splitter is restored by UpdateSeriesOrientation(), once the width is known
    // pages are stored by title: the set of pages differs between models, an index would point elsewhere
    model_tab->setCurrentTitle(settings.value(QString("model %1 page title").arg(m_model->SFModel())).toString());
    settings.endGroup();
    const int model_type = m_model->SFModel();
    model_tab->onPageChanged([this, model_type, model_tab](int) {
        QSettings settings;
        settings.beginGroup("model");
        settings.setValue(QString("model %1 page title").arg(model_type), model_tab->currentTitle());
        settings.endGroup();
        if (model_tab->currentTitle() == tr("Detailed"))
            RefreshDetailed(true);
    });
    if (model_tab->currentTitle() == tr("Detailed"))
        RefreshDetailed(true); // restored as the last page
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

void ModelWidget::AddScriptModelTab(SectionStack* model_tab)
{
    QPushButton* update = new QPushButton;
    update->setIcon(Icon("dialog-ok-apply"));
    QScrollArea* scrollArea = new QScrollArea;
    PrepareWidget* widget = new PrepareWidget(m_model->getModelDefinitionBlock(), false, this);
    // Same live guidance as the creation dialog: which identifiers the equation may use for the
    // current reaction system, and which globals are the stability constants. Claude Generated.
    widget->setInputColumns(m_model->IndependentModel()->columnCount());
    widget->AddScriptGuidance();
    if (ScriptModel* script = qobject_cast<ScriptModel*>(m_model.data()))
        widget->setEquilibriumMode(script->UsesSpeciation());
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

    // The only scroll bar of the page: the form follows the page width (one or two columns), so there is no
    // horizontal scrolling and no scroll area inside a scroll area. Claude Generated (WP2).
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(widget);

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

    update->setToolTip(tr("Apply the model definition now"));

    // page header tools: delayed-update progress, apply, save and duplicate
    QWidget* buttonBar = new QWidget;
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonBar->setLayout(buttonLayout);
    buttonLayout->addWidget(bar);
    buttonLayout->addWidget(update);
    buttonLayout->addWidget(save);
    buttonLayout->addWidget(duplicate);

    bar->setFormat(tr("Update pending"));
    bar->setValue(0);
    bar->setMaximum(qApp->instance()->property("ScriptTimeout").toInt());
    bar->setMinimumWidth(120);
    bar->setToolTip(tr("Time until the edited definition is applied"));
    model_tab->addPage(scrollArea, tr("Model Definition"), buttonBar, Icon("document-edit"));
}

void ModelWidget::AddSystemParameterTab(SectionStack* model_tab)
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

    QWidget* tools = new QWidget;
    QHBoxLayout* tools_layout = new QHBoxLayout;
    tools_layout->setContentsMargins(0, 0, 0, 0);
    tools_layout->addWidget(load);
    tools_layout->addWidget(save);
    tools->setLayout(tools_layout);
    model_tab->addPage(m_system_parameter, tr("System Parameter"), tools, Icon("text-field"));

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

QString ModelWidget::SplitterSettingsKey() const
{
    /* The saved splitter geometry must be keyed by the pane COUNT as well, not just the model id: a
     * scripted model has a different number of panes depending on whether it declares local
     * parameters (the per-series parameter pane is only added when LocalParameterSize() > 0). All
     * scripted models shared the key "model 100", so a state saved for a 2-pane layout was restored
     * into a 3-pane one and QSplitter gave the extra pane height 0 — the per-series shift editors
     * disappeared entirely even though the model was fully defined. Claude Generated. */
    // "layout 4": header, tabs | data series over fit results (2026 redesign); older states do not fit it.
    return QString("layout 4 model %1 panes %2").arg(m_model->SFModel()).arg(m_series_splitter->count());
}

/*! \brief Put the data series beside the model tabs when there is room, otherwise below them, so
 * the model tab stays usable in narrow workspaces (small screens, open agent dock). Claude Generated. */
void ModelWidget::UpdateSeriesOrientation()
{
    if (!m_series_splitter || m_series_splitter->count() < 2 || width() <= 0)
        return;
    const Qt::Orientation orientation = width() >= 900 ? Qt::Horizontal : Qt::Vertical;
    if (m_series_state_restored && m_series_splitter->orientation() == orientation)
        return;

    // Each direction has its own sizes: what looks right beside each other is wrong below each other. The
    // sizes the user set last in this direction come back; without any, the tabs get what they need
    // (at most a third) below, and the series the larger share beside. Claude Generated.
    if (m_series_state_restored)
        SplitterResized(); // keep what was set in the direction that is being left
    m_series_splitter->setOrientation(orientation);
    m_series_state_restored = true;
    QSettings settings;
    settings.beginGroup("model");
    const bool restored = m_series_splitter->restoreState(settings.value(SeriesStateKey()).toByteArray());
    settings.endGroup();
    if (!restored) {
        const int total = orientation == Qt::Horizontal ? m_series_splitter->width() : m_series_splitter->height();
        const int first = orientation == Qt::Horizontal ? total * 2 / 5 : qMin(total / 3, m_series_splitter->widget(0)->minimumSizeHint().height() + 40);
        m_series_splitter->setSizes({ first, total - first });
    }
}

QString ModelWidget::SeriesStateKey() const
{
    return SplitterSettingsKey() + (m_series_splitter->orientation() == Qt::Horizontal ? " series beside" : " series below");
}

void ModelWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    UpdateSeriesOrientation();
}

void ModelWidget::SplitterResized()
{
    QSettings settings;
    settings.beginGroup("model");
    settings.setValue(SplitterSettingsKey(), m_splitter->saveState());
    if (m_series_state_restored) // before that the splitter still has its construction state
        settings.setValue(SeriesStateKey(), m_series_splitter->saveState());
    settings.endGroup();
}

void ModelWidget::resizeButtons()
{
    m_minimize_all->setMinimumWidth(90);
    m_minimize_all->setMaximumSize(120, 32);
    m_minimize_all->setStyleSheet("background-color: #77d740; color: black;");
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

    QTimer::singleShot(1, m_statistic_widget, &StatisticWidget::Update);

    // Status chip next to the Fit button; the fixed colours carry the meaning in light and dark mode.
    QString text, colour;
    if (m_model->isCorrupt()) {
        text = tr("Model corrupt");
        colour = "#c62828";
        m_converged_label->setToolTip(tr("Probably numerical errors in the concentration calculation; do not rely on these results."));
    } else if (!m_model->isConverged()) {
        text = tr("Not converged");
        colour = "#e65100";
        m_converged_label->setToolTip(tr("The last optimisation did not converge."));
    } else {
        text = tr("Converged");
        colour = "#2e7d32";
        m_converged_label->setToolTip(tr("The last optimisation converged."));
    }
    m_converged_label->setText(text);
    m_converged_label->setStyleSheet(QString("QLabel { background: %1; color: white; border-radius: 9px; padding: 2px 10px; font-weight: bold; }").arg(colour));

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
    QTimer::singleShot(1, this, &ModelWidget::Repaint);
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

void ModelWidget::SetFitSolver(const QString& solver)
{
    /* Write the FitSolver choice into the model's optimizer config. Persisted with the model and
       inherited by every statistical re-fit via Clone(), so MC/CV/RA use the same solver. Claude Generated. */
    QJsonObject config = m_model->getOptimizerConfig();
    config["FitSolver"] = solver;
    m_model->setOptimizerConfig(config);
}

void ModelWidget::SetSpeciationSolver(const QString& method)
{
    /* Write the speciation-solver choice into the model's optimizer config. The model's
       setOptimizerConfig() override pushes it into the embedded SpeciationEngine; MC/CV/RA inherit the
       key via Clone(). Claude Generated. */
    QJsonObject config = m_model->getOptimizerConfig();
    config["SpeciationSolver"] = method;
    m_model->setOptimizerConfig(config);
}

void ModelWidget::UpdateSolverMenu()
{
    /* Refresh check marks and the VarPro enable state when the menu opens, so the display never
       lags behind a config changed elsewhere (e.g. OptimizerSettings) or a freshly loaded model. */
    const QString solver = m_model->getOptimizerConfig().value("FitSolver").toString(QStringLiteral("LevMar"));
    const bool varpro = m_model->SupportsVarPro();
    m_solver_varpro->setEnabled(varpro);
    m_solver_varpro->setToolTip(varpro ? QString() : tr("Only available for nmr_any / uvvis_any models."));
    const bool use_varpro = (varpro && solver == QLatin1String("VarPro"));
    m_solver_levmar->setChecked(!use_varpro);
    m_solver_varpro->setChecked(use_varpro);

    /* Speciation solver: only meaningful for the reaction-driven models that use the SpeciationEngine. */
    const bool uses_engine = m_model->UsesSpeciationEngine();
    m_speciation_menu->setEnabled(uses_engine);
    m_speciation_menu->setToolTip(uses_engine ? tr("Equilibrium-concentration solver used by the reaction-driven models.")
                                              : tr("Only the reaction-driven models (nmr_any / uvvis_any / fl_any / itc_any) use a speciation solver."));
    const QString spec = m_model->getOptimizerConfig().value("SpeciationSolver").toString(QStringLiteral("LevMar"));
    const bool use_bfgs = (spec == QLatin1String("BFGS"));
    m_speciation_bfgs->setChecked(use_bfgs);
    m_speciation_levmar->setChecked(!use_bfgs);
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
    for (int i = 0; i < m_model->CollectOptimizationParameters().size(); ++i) {
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
    m_model->CollectOptimizationParameters();
    Repaint();

    // Claude Generated: Clean replacement for timer hack - immediate state application
    // Apply current state immediately after Repaint() to ensure LineSeries remain visible
    if (m_charts.data_wrapper) {
#ifdef DEBUG_ON
        qDebug() << "🔍 DEBUG MinimizeModel: m_charts.data_wrapper valid:" << m_charts.data_wrapper.data()
                 << "about to call applyStateToSeries()";
#endif
        m_charts.data_wrapper->applyStateToSeries();
        qDebug() << "✅ DEBUG MinimizeModel: Applied immediate applyStateToSeries() after fitting";
#ifdef DEBUG_ON
        qDebug() << "🔍 DEBUG MinimizeModel: applyStateToSeries() call completed";
#endif
    }
#ifdef DEBUG_ON
    else {
        qDebug() << "⚠️  WARNING MinimizeModel: m_charts.data_wrapper is null";
    }
#endif

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
    m_statistic_dialog->HideWidget();
    SupraFit::Method type = SupraFit::Method(AccessCI(data["controller"].toObject(), "Method").toInt());
    ShowStatisticResult(type, index);
}

/*! \brief Rows of the Statistics list: the analyses to start, then the finished results (if any).
 * Claude Generated. */
QList<ModelWidget::StatEntry> ModelWidget::StatisticEntries() const
{
    // The list is narrow: short names, the full ones are the tooltips; one icon per method for the analysis
    // to start and for its results.
    struct Analysis {
        const char* tab; ///< start of the tab title in the statistic dialog
        const char* shortName;
        SupraFit::Method method;
    };
    static const Analysis analyses[] = {
        { "Cross", QT_TR_NOOP("Cross Validation"), SupraFit::Method::CrossValidation },
        { "Monte", QT_TR_NOOP("Monte Carlo"), SupraFit::Method::MonteCarlo },
        { "Weakened", QT_TR_NOOP("Grid Search"), SupraFit::Method::WeakenedGridSearch },
        { "Model", QT_TR_NOOP("Model Comparison"), SupraFit::Method::ModelComparison },
    };

    QList<StatEntry> rows;
    rows << StatEntry{ tr("New analysis"), 0, -1, SupraFit::Method::MonteCarlo, -1, QIcon(), QString() };
    const QStringList tabs = m_statistic_dialog->TabTitles();
    for (int i = 0; i < tabs.size(); ++i) {
        QString title = tabs[i];
        QIcon icon;
        for (const Analysis& a : analyses)
            if (tabs[i].startsWith(QLatin1String(a.tab))) {
                title = tr(a.shortName);
                icon = MethodIcon(a.method);
            }
        rows << StatEntry{ title, 1, i, SupraFit::Method::MonteCarlo, -1, icon, title == tabs[i] ? QString() : tabs[i] };
    }
    const QList<ResultsDialog::Entry> results = m_results->Entries();
    if (!results.isEmpty())
        rows << StatEntry{ tr("Results"), 0, -1, SupraFit::Method::MonteCarlo, -1, QIcon(), QString() };
    for (const ResultsDialog::Entry& e : results)
        rows << StatEntry{ ResultListTitle(e.type, e.index), 2, -1, e.type, e.index, MethodIcon(e.type), e.title };
    return rows;
}

/*! \brief Short, numbered name of a finished analysis as listed on the Statistics page ("Monte Carlo #1"). */
QString ModelWidget::ResultListTitle(SupraFit::Method type, int index)
{
    QString name;
    switch (type) {
    case SupraFit::Method::MonteCarlo:
        name = tr("Monte Carlo");
        break;
    case SupraFit::Method::CrossValidation:
        name = tr("Cross Validation");
        break;
    case SupraFit::Method::WeakenedGridSearch:
        name = tr("Grid Search");
        break;
    case SupraFit::Method::ModelComparison:
        name = tr("Model Comparison");
        break;
    case SupraFit::Method::Reduction:
        name = tr("Reduction");
        break;
    case SupraFit::Method::GlobalSearch:
        name = tr("Global Search");
        break;
    default:
        name = SupraFit::Method2Name(type);
    }
    return tr("%1 #%2").arg(name).arg(index + 1);
}

/*! \brief Icon of an analysis method; the project only ships a few, so some are shared. */
QIcon ModelWidget::MethodIcon(SupraFit::Method type)
{
    switch (type) {
    case SupraFit::Method::MonteCarlo:
        return Icon("applications-education-science");
    case SupraFit::Method::CrossValidation:
    case SupraFit::Method::Reduction:
        return Icon("split");
    case SupraFit::Method::WeakenedGridSearch:
    case SupraFit::Method::GlobalSearch:
        return Icon("kspread");
    case SupraFit::Method::ModelComparison:
        return Icon("fork");
    default:
        return QIcon();
    }
}

/*! \brief Show the Statistics page with its list; \p results selects the newest finished analysis. */
void ModelWidget::OpenStatistics(bool results)
{
    if (m_stat_page < 0) { // simulations have no statistic page: the old window
        m_results->Attention();
        return;
    }
    m_pages->setCurrentTitle(tr("Statistics"), true);
    if (!results)
        return;
    const QList<ResultsDialog::Entry> entries = m_results->Entries();
    if (!entries.isEmpty())
        m_pages->selectSub(ResultListTitle(entries.last().type, entries.last().index));
}

/*! \brief A finished analysis: list it and show it on the Statistics page. */
void ModelWidget::ShowStatisticResult(SupraFit::Method type, int index)
{
    m_results->ShowResult(type, index); // opens the result tab
    if (m_stat_page < 0) {
        m_results->Attention();
        return;
    }
    m_pages->setCurrentTitle(tr("Statistics"), true);
    m_pages->refreshGroup(m_stat_page);
    m_pages->selectSub(ResultListTitle(type, index));
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
        WindowHost::Show(m_advancedsearch.data(), QStringLiteral("scan"), this); // same place and size every time
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
    OpenStatistics(true);
}

void ModelWidget::MultiScanFinished()
{
    LoadStatistic(m_advancedsearch->globalSearch()->Result());
}

void ModelWidget::HideAllWindows()
{
    m_advancedsearch->hide(); // the statistic tools are a page now, not a window
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

/*! \brief Write the page "Detailed": input data, model, parameters and, if asked for, the statistical analyses of
 * the stored results as text. The model is only calculated again when it still is in the fast mode of a fit
 * (the tables are those of the last calculation otherwise, which is what the rest of the pane shows). Claude Generated. */
void ModelWidget::RefreshDetailed(bool recalculate)
{
    if (!m_detailed_text || !m_detailed_statistics)
        return;
    if (recalculate && m_model->isFast()) {
        m_model->setFast(false);
        m_model->Calculate();
    }
    // the box only matters when there is something to analyse
    const int results = m_model->getMCStatisticResult() + m_model->getCVStatisticResult() + m_model->getWGStatisticResult()
        + m_model->getMoCoStatisticResult() + m_model->getReductionStatisticResults() + m_model->SearchSize();
    m_detailed_statistics->setVisible(results > 0);
    const bool withStatistics = results > 0 && m_detailed_statistics->isChecked();
    if (withStatistics)
        QApplication::setOverrideCursor(Qt::WaitCursor);
    m_detailed_text->setText("<html><pre>" + m_model->Data2Text() + "\n" + m_model->Model2Text(withStatistics) + "</ br>" + m_statistic_widget->Statistic() + "</pre></html>");
    if (withStatistics)
        QApplication::restoreOverrideCursor();
}

void ModelWidget::Detailed()
{
    m_pages->setCurrentTitle(tr("Detailed")); // the page writes itself when it is chosen
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
    m_pages->setPageHidden(m_charts_page, false);
    m_pages->refreshGroup(m_charts_page); // a chart came: it is listed
}

#include "modelwidget.moc"

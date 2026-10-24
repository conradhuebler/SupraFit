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

#include "src/global.h"
#include "src/core/models/meta_model.h" // MetaModel class (was transitive via models.h)
#include "src/core/models/scriptmodel.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtGui/QPainter> // ScriptModel + ScriptModelPreset (predefined-model menu)
#include "src/core/models/titrations/AbstractTitrationModel.h" // reaction/data mismatch reporting
#include "src/global_config.h"

#include "src/capabilities/jobmanager.h"
#include "src/capabilities/modelcomparison.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/capabilities/resampleanalyse.h"
#include "src/capabilities/weakenedgridsearch.h"

#include "src/core/analyse.h"
#include "src/core/jsonhandler.h"
#include "src/core/projectmanager.h"
#include "src/core/reactionparser.h"
#include "src/core/models/models.h"

#include "src/ui/dialogs/comparedialog.h"
#include "src/ui/dialogs/genericwidgetdialog.h"
#include "src/ui/dialogs/importdata.h"
#include "src/ui/dialogs/spectraimport.h"
#include "src/ui/dialogs/statisticdialog.h"

#include "src/ui/guitools/guitools.h"

#include "src/ui/mainwindow/datawidget.h"
#include "src/ui/mainwindow/metamodelwidget.h"
#include "src/ui/mainwindow/modelwidget.h"

#include "src/ui/widgets/preparewidget.h"
#include "src/ui/widgets/reactioneditorwidget.h"
#include "src/ui/widgets/textwidget.h"

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include <QtGui/QColor>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolButton>

#include <QtCore/QSignalBlocker>

#include "src/ui/instance.h"
#include "modeldataholder.h"

void ToolButton::ChangeColor(const QColor& color)
{
    setStyleSheet("background-color:" + color.name() + ";");
}

TabWidget::TabWidget(QWidget* parent)
    : QTabWidget(parent)
{
    setStyleSheet("QTabBar::tab { height: 32px;}");
}

void TabWidget::setHeight(int height)
{
    m_height = qMax(height, m_height);
    setStyleSheet(QString("QTabBar::tab { height: %1px;}").arg(m_height));
}

void TabWidget::addModelsTab(QPointer<ModelWidget> modelwidget)
{
    addTab(modelwidget, QString());

    QCheckBox* hide = new QCheckBox;
    hide->setMaximumSize(20, 20);
    hide->setChecked(true);
    hide->setToolTip(tr("Toggle Series in Charts"));

    modelwidget->setCheckbox(hide);

    ToolButton* color = new ToolButton;
    color->setMaximumSize(15, 15);
    QStringList colors = modelwidget->Chart().signal_wrapper->ColorList().split("|");
    colors.removeDuplicates();
    if (colors.size() > 1) { /*
        QPalette palette = color->palette();
        QLinearGradient gradient(color->rect().topLeft(), color->rect().bottomLeft());
        gradient.setColorAt(0.0, QColor(255, 0, 0, 127));
        gradient.setColorAt(1.0, QColor(0, 0, 255, 127));
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        palette.setBrush(QPalette::Button, QBrush(gradient));
        color->setPalette(palette);*/
        color->ChangeColor(Qt::green);
#pragma message "restore or improve some colorfull stuff"
    } else
        color->ChangeColor(QColor(colors.first()));

    QWidget* tools = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(hide);
    layout->addWidget(color);
    QLabel* title_label = new QLabel(modelwidget->Model()->Name());
    title_label->setWordWrap(true);

    setHeight(modelwidget->Model()->Name().size() / 35 * 32);
    layout->addWidget(title_label);
    tools->setLayout(layout);

    connect(hide, &QCheckBox::stateChanged, modelwidget, &ModelWidget::ToggleSeries);
    connect(color, &QPushButton::clicked, modelwidget, &ModelWidget::ChangeColor);
    connect(modelwidget, &ModelWidget::ColorChanged, color, &ToolButton::ChangeColor);

    connect(modelwidget->Model().data(), &AbstractModel::ModelNameChanged, title_label, [title_label](const QString& str) {
        title_label->setText(str);
    });

    setCurrentWidget(modelwidget);
    tabBar()->setTabButton(currentIndex(), QTabBar::LeftSide, tools);

    if (!modelwidget->Model()->SupportSeries()) {
        modelwidget->setColor(ChartWrapper::ColorCode(currentIndex()));
    }
}

void TabWidget::setDataTab(QPointer<DataWidget> datawidget)
{
    addTab(datawidget, "Data Overview");
    tabBar()->setTabButton(0, QTabBar::RightSide, 0);
    m_datawidget = datawidget;
}

void TabWidget::setMetaTab(QPointer<MetaModelWidget> datawidget)
{
    addTab(datawidget, "MetaModel Overview");
    tabBar()->setTabButton(0, QTabBar::RightSide, 0);
    m_metamodelwidget = datawidget;
}

MDHDockTitleBar::MDHDockTitleBar()
{
    // The workspace actions live in three menus (Data, Model, Evaluation) that the main window puts
    // into its toolbar for the project on screen; this widget is the empty title bar of the workspace
    // dock. Claude Generated (menu restructure).
    QHBoxLayout* layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    /* Data menu */
    m_data_menu = new QMenu(tr("Data"), this);
    m_data_menu->setToolTipsVisible(true);
    m_edit_data = m_data_menu->addAction(Icon("document-edit"), tr("Edit Data"));
    m_edit_data->setToolTip(tr("Edit the data table of the current project"));
    connect(m_edit_data, &QAction::triggered, this, &MDHDockTitleBar::EditData);

    m_split_data = m_data_menu->addAction(Icon("split"), tr("Split Data Set"));
    m_split_data->setToolTip(tr("Split the data set column-wise into separate projects"));
    connect(m_split_data, &QAction::triggered, this, &MDHDockTitleBar::SplitData);

    /* Evaluation menu */
    m_evaluation_menu = new QMenu(tr("Evaluation"), this);
    m_evaluation_menu->setToolTipsVisible(true);
    m_optimize = m_evaluation_menu->addAction(Icon("go-down"), tr("Optimise All"));
    m_optimize->setToolTip(tr("Optimise all checked models"));
    connect(m_optimize, &QAction::triggered, this, &MDHDockTitleBar::OptimizeAll);

    m_statistics = m_evaluation_menu->addAction(Icon("fork"), tr("Statistics"));
    m_statistics->setToolTip(tr("Statistical post-processing (Monte Carlo, cross validation, …) for all models"));
    connect(m_statistics, &QAction::triggered, this, &MDHDockTitleBar::ShowStatistics);

    m_analyse = m_evaluation_menu->addAction(Icon("help-hint"), tr("Analyse and Compare"));
    m_analyse->setToolTip(tr("Compare the models by their statistical results (Monte Carlo, cross validation, reduction analysis, AIC)"));
    connect(m_analyse, &QAction::triggered, this, &MDHDockTitleBar::Compare);

    m_evaluation_menu->addSeparator();
    m_close_all = m_evaluation_menu->addAction(Icon("trash-empty"), tr("Remove All Models"));
    m_close_all->setToolTip(tr("Remove all models from the current project"));
    m_close_all->setDisabled(true);
    connect(m_close_all, &QAction::triggered, this, &MDHDockTitleBar::CloseAll);

    auto addModel = [this](SupraFit::Model model) -> QAction* {
        QAction* action = new QAction(this);
        action->setText(tr("%1").arg(Model2Name(model)));
        action->setData(model);
        connect(action, &QAction::triggered, this, &MDHDockTitleBar::PrepareAddModel);
        return action;
    };

    // Attach a reaction-preset submenu to an N-component *_any action: "Define reactions…" opens the
    // usual dialog, while a preset entry adds the model straight away with its reactions pre-set
    // (ModelDataHolder::AddModel skips the dialog when PresetReactions() is non-empty). maxComponents
    // filters presets whose reaction system exceeds the model's component limit (e.g. itc_any is
    // 2-component); a section header is only added when its section retains at least one entry, so no
    // empty "Three components" section shows up for the 2-component case. Claude Generated.
    auto attachPresets = [this](QAction* anyAction, SupraFit::Model model, int maxComponents = INT_MAX) {
        QMenu* sub = new QMenu(this);
        QAction* custom = sub->addAction(tr("Define reactions…"));
        custom->setData(model);
        connect(custom, &QAction::triggered, this, &MDHDockTitleBar::PrepareAddModel);
        const QVector<ReactionEditorWidget::Preset> presets = ReactionEditorWidget::Presets();
        for (int idx = 0; idx < presets.size(); ++idx) {
            const ReactionEditorWidget::Preset& p = presets[idx];
            if (p.isHeader()) {
                // Look ahead: only add the section if at least one entry before the next header passes.
                bool any = false;
                for (int j = idx + 1; j < presets.size() && !presets[j].isHeader(); ++j) {
                    if (ReactionParser::Parse(presets[j].reactions).components.size() <= maxComponents) {
                        any = true;
                        break;
                    }
                }
                if (any)
                    sub->addSection(p.name);
                continue;
            }
            if (ReactionParser::Parse(p.reactions).components.size() > maxComponents)
                continue;
            QAction* a = sub->addAction(p.name);
            a->setToolTip(QString(p.reactions).replace('\n', QStringLiteral(" ; ")));
            const QString reactions = p.reactions;
            connect(a, &QAction::triggered, this, [this, model, reactions]() {
                m_model_choosen = model;
                m_preset_reactions = reactions;
                emit AddModel();
            });
        }
        anyAction->setMenu(sub);
    };

    //#ifdef NMR_Models
    m_nmr_model << addModel(SupraFit::nmr_ItoI);
    m_nmr_model << addModel(SupraFit::nmr_IItoI_ItoI);
    m_nmr_model << addModel(SupraFit::nmr_ItoI_ItoII);
    m_nmr_model << addModel(SupraFit::nmr_IItoI_ItoI_ItoII);
    QAction* nmrAny = addModel(SupraFit::nmr_any);
    attachPresets(nmrAny, SupraFit::nmr_any);
    m_nmr_model << nmrAny;

    //#endif

    //#ifdef Fluorescence_Models
    m_fl_model << addModel(SupraFit::fl_ItoI);
    m_fl_model << addModel(SupraFit::fl_IItoI_ItoI);
    m_fl_model << addModel(SupraFit::fl_ItoI_ItoII);
    m_fl_model << addModel(SupraFit::fl_IItoI_ItoI_ItoII);
    QAction* flAny = addModel(SupraFit::fl_any);
    attachPresets(flAny, SupraFit::fl_any); // N-component: full preset set incl. three-component systems
    m_fl_model << flAny;
    //#endif

    //#ifdef UV_VIS_Models
    m_uv_vis_model << addModel(SupraFit::uv_vis_ItoI);
    m_uv_vis_model << addModel(SupraFit::uv_vis_IItoI_ItoI);
    m_uv_vis_model << addModel(SupraFit::uv_vis_ItoI_ItoII);
    m_uv_vis_model << addModel(SupraFit::uv_vis_IItoI_ItoI_ItoII);
    QAction* uvvisAny = addModel(SupraFit::uvvis_any);
    attachPresets(uvvisAny, SupraFit::uvvis_any);
    m_uv_vis_model << uvvisAny;

    //#endif

#ifdef Kinetic_Models
    m_kinetcs_model << addModel(SupraFit::Michaelis_Menten);

    m_kinetcs_model << addModel(SupraFit::MonoMolecularModel);
    m_kinetcs_model << addModel(SupraFit::BiMolecularModel);
    m_kinetcs_model << addModel(SupraFit::FlexMolecularModel);
    m_kinetcs_model << addModel(SupraFit::TianModel);

    m_kinetcs_model << addModel(SupraFit::DecayRates);
#endif

#ifdef ITC_Models
    m_itc_fixed_model << addModel(SupraFit::itc_ItoI);
    m_itc_fixed_model << addModel(SupraFit::itc_IItoI);
    m_itc_fixed_model << addModel(SupraFit::itc_ItoII);
    m_itc_fixed_model << addModel(SupraFit::itc_IItoII);
    QAction* itcAny = addModel(SupraFit::itc_any);
    attachPresets(itcAny, SupraFit::itc_any, /*maxComponents=*/2); // ITC is 2-component (host/guest from protocol)
    m_itc_fixed_model << itcAny;

    //m_itc_flex_model << addModel(SupraFit::itc_n_ItoI);
    m_itc_flex_model << addModel(SupraFit::itc_n_ItoII);

    m_itc_fixed_model << addModel(SupraFit::itc_blank);

    m_calorimetry << addModel(SupraFit::TianModel);
#endif

    /* Model menu: scripted models, model files, then the experiment submenus (filled in addToMenu,
     * because the available experiments depend on the number of independent columns). */
    m_model_menu = new QMenu(tr("Model"), this);
    m_model_menu->setToolTipsVisible(true);

    // A dropdown of predefined scripted models (Michaelis-Menten, Hill, …): the first entry opens the
    // usual blank definition dialog, each preset opens it pre-filled. Mirrors the reaction-preset
    // submenus of the *_any models. Claude Generated.
    QMenu* scriptPresets = m_model_menu->addMenu(Icon("list-add"), tr("New Model"));
    scriptPresets->menuAction()->setToolTip(tr("Define a model by its equation, empty or from a predefined model"));
    QAction* customModel = scriptPresets->addAction(tr("Custom (empty)"));
    connect(customModel, &QAction::triggered, this, [this]() { emit this->NewModel(); });
    scriptPresets->addSection(tr("Predefined models"));
    const QVector<ScriptModelPreset> presets = ScriptModel::Presets();
    for (int i = 0; i < presets.size(); ++i) {
        QAction* a = scriptPresets->addAction(presets[i].name);
        a->setToolTip(tr("%1 — needs %2 independent column(s)").arg(presets[i].name).arg(presets[i].inputSize));
        connect(a, &QAction::triggered, this, [this, i]() { emit this->NewModelFromPreset(i); });
        m_script_preset_actions << qMakePair(QPointer<QAction>(a), presets[i].inputSize);
    }
    // Grey out presets that need more independent columns than the current data provides (0 = unknown
    // -> leave everything enabled). Re-evaluated every time the menu opens. Claude Generated.
    connect(scriptPresets, &QMenu::aboutToShow, this, [this]() {
        for (const auto& entry : m_script_preset_actions) {
            if (entry.first)
                entry.first->setEnabled(m_independent_count <= 0 || entry.second <= m_independent_count);
        }
    });
    scriptPresets->setToolTipsVisible(true);

    QAction* addScript = m_model_menu->addAction(Icon("document-open"), tr("Load Model File…"));
    addScript->setToolTip(tr("Add a model stored in a model file (*.json)"));
    connect(addScript, &QAction::triggered, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, tr("Custom Model"), getDir(), "*.json");
        emit this->AddScriptModel(file);
    });

    m_model_menu->addSection(tr("Experiment"));
    auto experimentMenu = [this](const QString& title) {
        QMenu* menu = m_model_menu->addMenu(Icon("list-add"), title);
        menu->setToolTipsVisible(true);
        menu->menuAction()->setVisible(false); // shown by addToMenu when the data fits the experiment
        return QPointer<QMenu>(menu);
    };
    m_nmr_menu = experimentMenu(tr("NMR Titration"));
    m_uv_vis_menu = experimentMenu(tr("UV/Vis Titration"));
    m_fl_menu = experimentMenu(tr("Fluorescence Titration"));
    m_itc_menu = experimentMenu(tr("Calorimetry"));
    m_kinetics_menu = experimentMenu(tr("Kinetics"));

    m_model_menu->addSeparator();
    m_any_model = m_model_menu->addAction(QIcon(":/icons/list-add-blue.png"), tr("Any Model…"));
    m_any_model->setToolTip(tr("Add a model by its numeric model ID"));
    connect(m_any_model, &QAction::triggered, this, [this]() {
        bool ok;
        int old = qApp->instance()->property("last_any").toInt();
        int i = QInputDialog::getInt(this, tr("Add model by ID"),
            tr("ID:"), old, 1, 404, 1, &ok);
        if (ok) {
            this->m_model_choosen = i;
            qApp->instance()->setProperty("last_any", i);
            emit this->AddModel();
        }
    });
    auto showAdvanced = [this]() { m_any_model->setVisible(qApp->instance()->property("advanced_ui").toBool()); };
    showAdvanced();
    if (Instance::GlobalInstance())
        connect(Instance::GlobalInstance(), &Instance::ConfigurationChanged, this, showAdvanced); // live, no restart
}

void MDHDockTitleBar::setEnabled(bool enabled)
{
    m_enabled = enabled;
    emit ToolsChanged();
}

void MDHDockTitleBar::addToMenu(int IndependetCount)
{
    // Fill the experiment submenus with the models (stoichiometries) that fit the data: one independent
    // column means calorimetry/kinetics, two mean host/guest titrations (NMR, UV/Vis, fluorescence).
    auto fill = [](QMenu* menu, const QVector<QPointer<QAction>>& list) {
        for (const QPointer<QAction>& ptr : std::as_const(list))
            if (ptr)
                menu->addAction(ptr);
        menu->menuAction()->setVisible(!menu->isEmpty());
    };

    for (QMenu* menu : { m_nmr_menu.data(), m_uv_vis_menu.data(), m_fl_menu.data(), m_itc_menu.data(), m_kinetics_menu.data() }) {
        menu->clear();
        menu->menuAction()->setVisible(false);
    }

    if (IndependetCount == 1) {
        fill(m_kinetics_menu, m_kinetcs_model);
        m_itc_menu->addSection(tr("Fixed Stoichiometry"));
        fill(m_itc_menu, m_itc_fixed_model);
        m_itc_menu->addSection(tr("Flexible Stoichiometry"));
        fill(m_itc_menu, m_itc_flex_model);
        m_itc_menu->addSection(tr("Thermokinetics"));
        fill(m_itc_menu, m_calorimetry);
        m_itc_menu->menuAction()->setVisible(!(m_itc_fixed_model.isEmpty() && m_itc_flex_model.isEmpty() && m_calorimetry.isEmpty()));
    } else if (IndependetCount == 2) {
        fill(m_nmr_menu, m_nmr_model);
        fill(m_uv_vis_menu, m_uv_vis_model);
        fill(m_fl_menu, m_fl_model);
    }
}

void MDHDockTitleBar::setEditDataToggles(bool toggles)
{
    m_edit_data->setCheckable(toggles);
    m_edit_data->setToolTip(toggles ? tr("Make the data tables of the current project editable (checked) or read-only")
                                    : tr("Edit the raw data of the current project in its import dialog"));
}

void MDHDockTitleBar::setEditDataChecked(bool checked)
{
    QSignalBlocker block(m_edit_data);
    m_edit_data->setChecked(checked);
}

void MDHDockTitleBar::HideModelTools()
{
    m_tools_hidden = true;
    emit ToolsChanged();
}

void MDHDockTitleBar::EnableBatch(bool enabled)
{
    m_close_all->setEnabled(enabled);
    m_statistics->setEnabled(enabled);
    m_optimize->setEnabled(enabled);
    m_analyse->setEnabled(enabled);
}

void MDHDockTitleBar::RequestModel(const ModelCatalog::Choice& choice)
{
    switch (choice.kind) {
    case ModelCatalog::Choice::Kind::FixedModel:
        m_model_choosen = choice.model;
        m_preset_reactions.clear(); // plain add, as from the menu
        emit AddModel();
        break;
    case ModelCatalog::Choice::Kind::AnyModelWithReactions:
        m_model_choosen = choice.model;
        m_preset_reactions = choice.reactions; // the dialog is skipped, as for the preset submenus
        emit AddModel();
        break;
    case ModelCatalog::Choice::Kind::ScriptPreset:
        emit NewModelFromPreset(choice.scriptPreset);
        break;
    case ModelCatalog::Choice::Kind::ModelFile:
        emit AddScriptModel(choice.file);
        break;
    case ModelCatalog::Choice::Kind::None:
        break;
    }
}

void MDHDockTitleBar::PrepareAddModel()
{
    m_model_choosen = qobject_cast<QAction*>(sender())->data().toInt();
    m_preset_reactions.clear(); // plain add: no preset, show the definition dialog as usual
    emit AddModel();
}

ModelDataHolder::ModelDataHolder()
    : m_TitleBarWidget(new MDHDockTitleBar)
    , m_history(true)
{
    QGridLayout* layout = new QGridLayout;

    setLayout(layout);

    m_modelsWidget = new TabWidget(this);
    m_modelsWidget->setTabsClosable(true);
    m_modelsWidget->setMovable(true);
    connect(m_modelsWidget, &QTabWidget::tabCloseRequested, this, &ModelDataHolder::RemoveTab);
    connect(m_modelsWidget, &QTabWidget::currentChanged, this, &ModelDataHolder::HideSubWindows);

    m_statistic_dialog = new StatisticDialog(this);
    connect(m_statistic_dialog, &StatisticDialog::RunCalculation, this, &ModelDataHolder::RunJobs);
    connect(m_statistic_dialog, &StatisticDialog::Interrupt, this, [this]() {
        this->m_allow_loop = false;
    });

    m_compare_dialog = new CompareDialog(this);
    connect(m_compare_dialog, &CompareDialog::CompareReduction, this, &ModelDataHolder::CompareReduction);
    connect(m_compare_dialog, &CompareDialog::CompareAIC, this, &ModelDataHolder::CompareAIC);
    connect(m_compare_dialog, &CompareDialog::CompareCV, this, &ModelDataHolder::CompareCV);
    connect(m_compare_dialog, &CompareDialog::CompareMC, this, &ModelDataHolder::CompareMC);

    connect(m_TitleBarWidget, qOverload<>(&MDHDockTitleBar::AddModel), this, static_cast<void (ModelDataHolder::*)()>(&ModelDataHolder::AddModel));
    connect(m_TitleBarWidget, &MDHDockTitleBar::AddScriptModel, this, &ModelDataHolder::AddScriptModel);
    connect(m_TitleBarWidget, &MDHDockTitleBar::NewModel, this, &ModelDataHolder::NewModel);
    connect(m_TitleBarWidget, &MDHDockTitleBar::NewModelFromPreset, this, &ModelDataHolder::NewModelFromPreset);

    connect(m_TitleBarWidget, &MDHDockTitleBar::ShowStatistics, m_statistic_dialog, &StatisticDialog::Attention);
    connect(m_TitleBarWidget, &MDHDockTitleBar::OptimizeAll, this, &ModelDataHolder::OptimizeAll);
    connect(m_TitleBarWidget, &MDHDockTitleBar::CloseAll, this, &ModelDataHolder::CloseAll);
    connect(m_TitleBarWidget, &MDHDockTitleBar::EditData, this, &ModelDataHolder::EditData);
    connect(m_TitleBarWidget, &MDHDockTitleBar::Compare, this, [this]() {
        if (this->m_compare_dialog) {
            m_compare_dialog->setCutoff(this->m_ReductionCutoff);
            m_compare_dialog->SetComparison(Compare());
            m_compare_dialog->show();
        }
    });
    connect(m_TitleBarWidget, &MDHDockTitleBar::SplitData, this, &ModelDataHolder::SplitData);

    layout->addWidget(m_modelsWidget, 1, 0);
    ActiveBatch();
}

ModelDataHolder::~ModelDataHolder()
{
    for (int i = 0; i < m_modelsWidget->count(); ++i) {
        if (qobject_cast<ModelWidget*>(m_modelsWidget->widget(i))) {
            ModelWidget* model = qobject_cast<ModelWidget*>(m_modelsWidget->widget(i));
            m_modelsWidget->removeTab(i);
            delete model;
        }
    }
    m_wrapper.clear();
    m_data.clear();
}

// Claude Generated - ProjectManager Integration for setData
void ModelDataHolder::setData(QSharedPointer<DataClass> data, QSharedPointer<ChartWrapper> wrapper)
{
    m_data = data;
    //connect(m_data.data(), &DataClass::Warning, this, &ModelDataHolder::Message);
    m_TitleBarWidget->setEnabled(true);
    m_wrapper = wrapper;
    
    // Claude Generated - Check if this DataClass is managed by ProjectManager
    if (data) {
        SupraFit::ProjectManager& projectManager = SupraFit::ProjectManager::instance();
        QString dataUuid = data->UUID();
        
        // Check if this data is managed by ProjectManager
        if (projectManager.hasProject(dataUuid)) {
            m_currentProjectId = dataUuid;
            qDebug() << "ModelDataHolder::setData: Data is managed by ProjectManager, project ID:" << m_currentProjectId;
        } else {
            // Clear project ID if not managed by ProjectManager (legacy data)
            m_currentProjectId.clear();
            qDebug() << "ModelDataHolder::setData: Data not managed by ProjectManager, using legacy mode";
        }
    }
    if (!qobject_cast<MetaModel*>(data)) {
#ifdef DEBUG_ON
        qDebug() << "ModelDataHolder::setData: Creating DataWidget for non-MetaModel data";
        // Claude Generated - ChartWrapper validation before creating DataWidget
        if (wrapper) {
            qDebug() << "📊 ModelDataHolder::setData: ChartWrapper provided, series count:" << wrapper->SeriesSize();
        } else {
            qDebug() << "⚠️ ModelDataHolder::setData: ChartWrapper is null - DataWidget will receive empty wrapper";
        }
#endif
        m_datawidget = new DataWidget;
        m_datawidget->setData(m_data, wrapper);
        m_modelsWidget->setDataTab(m_datawidget);
        
        // Claude Generated - Safety check for IndependentModel to prevent crash
        auto dataRef = m_data.toStrongRef();
        if (dataRef && dataRef->IndependentModel()) {
            m_TitleBarWidget->addToMenu(dataRef->IndependentModel()->columnCount());
            const bool dialog_edit = dataRef->ExportData()["SupraFit"].toInt() >= 1602
                && (dataRef->DataType() == DataClassPrivate::Thermogram || dataRef->DataType() == DataClassPrivate::Spectrum);
            m_TitleBarWidget->setEditDataToggles(!dialog_edit);
            m_TitleBarWidget->setEditDataChecked(dataRef->DependentModel()->isEditable());
            m_TitleBarWidget->setIndependentCount(dataRef->IndependentModel()->columnCount());
            qDebug() << "✅ DEBUG setData: Successfully added menu with" << dataRef->IndependentModel()->columnCount() << "columns";
        } else {
            qWarning() << "❌ DEBUG setData: IndependentModel is null, skipping menu setup";
        }
        
        connect(m_datawidget, &DataWidget::NameChanged, this, &ModelDataHolder::SetProjectTabName);
        connect(m_datawidget, SIGNAL(recalculate()), this, SIGNAL(recalculate()));
#ifdef DEBUG_ON
        qDebug() << "ModelDataHolder::setData: DataWidget created and configured successfully";
#endif
    } else {
#ifdef DEBUG_ON
        qDebug() << "ModelDataHolder::setData: Creating MetaModelWidget for MetaModel data";
#endif
        m_metamodelwidget = new MetaModelWidget();
        m_metamodelwidget->setMetaModel(qobject_cast<MetaModel*>(data));
        m_modelsWidget->setMetaTab(m_metamodelwidget);
        m_TitleBarWidget->HideModelTools();

        //connect(m_metamodelwidget, &MetaModelWidget::Message, this, &ModelDataHolder::Message);
        //connect(m_metamodelwidget, &MetaModelWidget::Warning, this, &ModelDataHolder::Warning);
    }
    if (m_data.toStrongRef().data()->isSimulation()) {
        QJsonObject raw = m_data.toStrongRef().data()->RawData();
        QJsonObject models = raw["models"].toObject();
        for (const QString& key : models.keys()) {
            if (models[key].isString()) {
                QString file = models[key].toString();
                Json2Model(JsonHandler::LoadFile(file));
            } else if (models[key].isObject()) {
                QJsonObject model = models[key].toObject();
                Json2Model(model);
            } else
                AddModel(models[key].toInt());
        }
    }
}

void ModelDataHolder::SetProjectTabName()
{
    m_modelsWidget->setTabText(0, "Overview");
    emit nameChanged();
}

void ModelDataHolder::setDataWeakRef(QWeakPointer<DataClass> weakData, QSharedPointer<ChartWrapper> wrapper)
{
    // Claude Generated: WeakPointer implementation to prevent exit crash
    auto strongData = weakData.toStrongRef();
    if (!strongData) {
        qWarning() << "ModelDataHolder::setDataWeakRef: DataClass already destroyed";
        return;
    }
    
    // Use the existing setData method with the strong reference
    // This ensures all the existing logic is preserved
    setData(strongData, wrapper);
}


namespace {
/*! \brief Draw the definition currently in the dialog against the real data: build a throwaway
 * ScriptModel, seed it and calculate. Answers "do my numbers look sensible?" before the model is
 * ever created, which was impossible before. Claude Generated. */
void RefreshScriptPreview(QChartView* view, PrepareWidget* fields, QPointer<DataClass> data)
{
    if (!view || !fields || !data)
        return;

    // QChartView::setChart RELEASES the previous chart instead of deleting it, so every refresh — and
    // this runs on every keystroke — leaked a whole chart with all its series. Claude Generated.
    QChart* previous = view->chart();

    QChart* chart = new QChart;
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data);
    bool ok = false;
    if (model) {
        model->setModelDefinition(fields->getObject());
        ok = model->DefineModel(QJsonObject()) && model->Complete();
    }
    if (!ok) {
        chart->setTitle(QObject::tr("Definition incomplete — no preview"));
        view->setChart(chart);
        delete previous;
        return;
    }

    model->InitialGuess();
    model->Calculate();

    const int series = model->SeriesCount();
    for (int j = 0; j < series && j < 4; ++j) { // four curves are plenty for a thumbnail
        QScatterSeries* measured = new QScatterSeries;
        measured->setMarkerSize(6);
        measured->setName(QObject::tr("data %1").arg(j + 1));
        QLineSeries* computed = new QLineSeries;
        computed->setName(QObject::tr("model %1").arg(j + 1));
        for (int i = 0; i < model->DataPoints(); ++i) {
            const double x = model->PrintOutIndependent(i);
            measured->append(x, model->DependentModel()->data(i, j));
            computed->append(x, model->ModelTable()->data(i, j));
        }
        chart->addSeries(measured);
        chart->addSeries(computed);
    }
    chart->createDefaultAxes();
    chart->setTitle(QObject::tr("Preview at the initial guess"));
    view->setChart(chart);
    delete previous;
}
}

void ModelDataHolder::NewModel()
{
    NewScriptModelDialog(QVector<QJsonObject>());
}

void ModelDataHolder::NewModelFromPreset(int index)
{
    const QVector<ScriptModelPreset> presets = ScriptModel::Presets();
    if (index < 0 || index >= presets.size())
        return;

    // A preset picked from the MENU creates the model straight away — the same UX the reaction presets
    // of the *_any models have (AddModel skips the dialog when PresetReactions() is set). Presets
    // offered INSIDE the dialog fill the fields instead. Claude Generated.
    QSharedPointer<AbstractModel> t = CreateModel(SupraFit::ScriptModel, m_data);
    if (!t)
        return;
    // a preset written for fewer inputs than the data has columns is applied with the data's column count
    const int columns = (m_data && m_data.toStrongRef() && m_data.toStrongRef()->IndependentModel())
        ? m_data.toStrongRef()->IndependentModel()->columnCount()
        : 0;
    QHash<QString, QJsonObject> elements;
    for (const QJsonObject& descriptor : ScriptModel::PresetBlockFor(presets[index], columns))
        elements[descriptor["name"].toString()] = descriptor;
    t->setModelDefinition(elements);
    t->DefineModel(QJsonObject());
    if (!t->Complete()) {
        QMessageBox::warning(this, tr("Model not complete"),
            tr("The preset \"%1\" could not be applied to this data set. It needs %2 independent "
               "column(s).")
                .arg(presets[index].name)
                .arg(presets[index].inputSize));
        return;
    }
    t->InitialGuess();
    m_history = false;
    ActiveModel(t);
}

void ModelDataHolder::NewScriptModelDialog(const QVector<QJsonObject>& presetBlock)
{
    QSharedPointer<AbstractModel> t = CreateModel(SupraFit::ScriptModel, m_data);
    if (t->DemandInput()) {
        // Pre-fill the dialog from the preset block, or fall back to the blank template. Claude Generated.
        const QVector<QJsonObject> block = presetBlock.isEmpty() ? t->getInputBlock() : presetBlock;
        PrepareWidget* prepareWidget = new PrepareWidget(block, true, this);

        // Click-to-fill presets beside the fields. Only those the data can actually satisfy are
        // offered — a preset needing two independent columns is useless on a one-column data set.
        const int available = (m_data && m_data.toStrongRef() && m_data.toStrongRef()->IndependentModel())
            ? m_data.toStrongRef()->IndependentModel()->columnCount()
            : 0;
        QVector<QPair<QString, QVector<QJsonObject>>> presets;
        for (const ScriptModelPreset& preset : ScriptModel::Presets()) {
            if (available <= 0 || preset.inputSize <= available)
                presets << qMakePair(preset.name, ScriptModel::PresetBlockFor(preset, available));
        }
        prepareWidget->setInputColumns(available); // the model takes exactly as many inputs as the data has columns
        prepareWidget->AddPresets(presets);
        // Makes the otherwise invisible Reactions <-> Equation coupling explicit.
        prepareWidget->AddScriptGuidance();
        // The preset determines the kind of model; a blank dialog starts as a free equation, so the
        // reaction editor stays out of the way until it is actually wanted. Claude Generated.
        bool equilibrium = false;
        for (const QJsonObject& descriptor : block) {
            if (descriptor["name"].toString() == QLatin1String("Reactions")
                && !descriptor["value"].toString().trimmed().isEmpty())
                equilibrium = true;
        }
        prepareWidget->setEquilibriumMode(equilibrium);

        // Live preview against the real data, debounced so typing does not rebuild a model per key.
        QChartView* preview = new QChartView;
        preview->setRenderHint(QPainter::Antialiasing);
        prepareWidget->AddPreview(preview);
        QTimer* debounce = new QTimer(prepareWidget);
        debounce->setSingleShot(true);
        debounce->setInterval(400);
        connect(debounce, &QTimer::timeout, prepareWidget,
            [preview, prepareWidget, this]() { RefreshScriptPreview(preview, prepareWidget, m_data.toStrongRef().data()); });
        connect(prepareWidget, &PrepareWidget::changed, debounce, qOverload<>(&QTimer::start));
        RefreshScriptPreview(preview, prepareWidget, m_data.toStrongRef().data());

        GenericWidgetDialog dialog("Define Model", prepareWidget);
        if (dialog.exec() == QDialog::Accepted) {
            QHash<QString, QJsonObject> elements = prepareWidget->getObject();
            QJsonObject definition;

            QJsonObject model;
            model["ModelDefinition"] = definition;
            t->setModelDefinition(elements);
            t->DefineModel(model);
            if (!t->Complete())
                return;

            t->InitialGuess();
            m_history = false;
            ActiveModel(t);
        }
    }
}

void ModelDataHolder::AddModel()
{
    MDHDockTitleBar* bar = qobject_cast<MDHDockTitleBar*>(sender());
    AddModel(bar->Model(), bar->PresetReactions());
}

void ModelDataHolder::AddScriptModel(const QString& string)
{
    QFile file(string);
    if (!file.open(QIODevice::ReadOnly))
        return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject model = doc.object();
    QSharedPointer<AbstractModel> t = CreateModel(model, m_data);
    t->InitialGuess();
    m_history = false;
    ActiveModel(t);
}

void ModelDataHolder::AddModel(int model, const QString& presetReactions)
{
    QSharedPointer<AbstractModel> t = CreateModel(model, m_data);
    if (!t) {
        emit m_data.toStrongRef()->Message(tr("Tried to add %1 to the workspace.").arg(Model2Name(static_cast<SupraFit::Model>(model))), 1);
        emit m_data.toStrongRef()->Warning("But one does not simply add a model to a data set, where the number of input variables differ.", 1);
        return;
    }

    // A reaction system needs one independent concentration column per component; on a mismatch the
    // model stays undefined. Warn the user instead of silently adding a broken model. Claude Generated.
    auto reactionDataMatches = [this, &t]() -> bool {
        if (AbstractTitrationModel* tm = qobject_cast<AbstractTitrationModel*>(t.data())) {
            const int requested = tm->ReactionComponentMismatch();
            if (requested > 0) {
                QMessageBox::warning(this, tr("Reaction / data mismatch"),
                    tr("The reaction equations define %1 components, but this data set provides %2 "
                       "concentration column(s).\n\nImport data with one concentration column per "
                       "component (%1 in total), or edit the reactions, then add the model again.")
                        .arg(requested)
                        .arg(t->InputParameterSize()));
                return false;
            }
        }
        return true;
    };

    if (!presetReactions.isEmpty()) {
        // Preset chosen from the Add-model submenu: define straight from the preset and skip the
        // "Define Model" dialog. Claude Generated.
        QJsonObject reactions;
        reactions["name"] = QStringLiteral("Reactions");
        reactions["type"] = 6;
        reactions["value"] = presetReactions;
        QHash<QString, QJsonObject> elements;
        elements[QStringLiteral("Reactions")] = reactions;
        t->setModelDefinition(elements);
        t->DefineModel(QJsonObject());
        if (!reactionDataMatches() || !t->Complete())
            return;
    } else if (t->DemandInput()) {
        PrepareWidget* prepareWidget = new PrepareWidget(t->getInputBlock(), true, this);
        GenericWidgetDialog dialog("Define Model", prepareWidget);
        if (dialog.exec() == QDialog::Accepted) {
            QHash<QString, QJsonObject> elements = prepareWidget->getObject();
            QJsonObject definition;

            QJsonObject model;
            model["ModelDefinition"] = definition;
            t->setModelDefinition(elements);

            t->DefineModel(model);

            if (!reactionDataMatches() || !t->Complete())
                return;
        } else
            return;
    }

    t->InitialGuess();
    m_history = false;
    ActiveModel(t);
}

void ModelDataHolder::Json2Model(const QJsonObject& object)
{
    QSharedPointer<AbstractModel> model = JsonHandler::Json2Model(object, m_data.toStrongRef().data());
    if (model) {
        ActiveModel(model, object);
    }
}

void ModelDataHolder::ActiveModel(QSharedPointer<AbstractModel> t, const QJsonObject& object, bool readonly)
{
    // Claude Generated - Add comprehensive safety checks
    if (!t) {
        qWarning() << "ModelDataHolder::ActiveModel: Invalid model provided";
        return;
    }
    
    if (!m_charts) {
        qWarning() << "ModelDataHolder::ActiveModel: Charts wrapper is null";
        return;
    }
    
    qDebug() << "🔍 DEBUG ModelDataHolder::ActiveModel: Starting widget creation for model" << t->Name();
    
    t->setFast(false);

    qDebug() << "🔍 DEBUG ModelDataHolder::ActiveModel: Adding model to charts";
    
    // Claude Generated - Additional safety check before addModel to prevent crashes
    if (!m_charts) {
        qWarning() << "❌ ModelDataHolder::ActiveModel: m_charts is null during addModel call";
        return;
    }
    
    Charts charts = m_charts->addModel(t);
    
    // Claude Generated - Verify charts creation was successful 
    if (!charts.signal_wrapper || !charts.error_wrapper || !charts.data_wrapper) {
        qWarning() << "❌ ModelDataHolder::ActiveModel: Chart creation failed - one or more wrappers are null";
        if (!charts.signal_wrapper) qWarning() << "   - signal_wrapper is null";
        if (!charts.error_wrapper) qWarning() << "   - error_wrapper is null"; 
        if (!charts.data_wrapper) qWarning() << "   - data_wrapper is null";
        return;
    }
    qDebug() << "DEBUG ModelDataHolder::ActiveModel: Charts created successfully";

    qDebug() << "🔍 DEBUG ModelDataHolder::ActiveModel: Creating ModelWidget for model" << t->Name();
    QPointer<ModelWidget> modelwidget = nullptr;
    
    try {
        modelwidget = new ModelWidget(t, charts, readonly);
        if (!modelwidget) {
            qWarning() << "ModelDataHolder::ActiveModel: Failed to create ModelWidget for model" << t->Name();
            return;
        }
        
        // Claude Generated: Reparent ChartWrappers to ModelWidget for proper cleanup
        if (charts.signal_wrapper) {
            charts.signal_wrapper->setParent(modelwidget);
#ifdef DEBUG_ON
            qDebug() << "🔧 Reparented signal_wrapper to ModelWidget for proper cleanup";
#endif
        }
        if (charts.error_wrapper) {
            charts.error_wrapper->setParent(modelwidget);
#ifdef DEBUG_ON
            qDebug() << "🔧 Reparented error_wrapper to ModelWidget for proper cleanup";
#endif
        }
        
        qDebug() << "✅ DEBUG ModelDataHolder::ActiveModel: Successfully created ModelWidget for model" << t->Name();
    } catch (const std::exception& e) {
        qWarning() << "ModelDataHolder::ActiveModel: Exception creating ModelWidget:" << e.what();
        return;
    } catch (...) {
        qWarning() << "ModelDataHolder::ActiveModel: Unknown exception creating ModelWidget for model" << t->Name();
        return;
    }
    modelwidget->setColorList(object["colors"].toString());
    modelwidget->setKeys(object["keys"].toString());
    connect(modelwidget, &ModelWidget::AddModel, this, [this](const QJsonObject& object) { this->Json2Model(object); });
    connect(modelwidget, &ModelWidget::DuplicateScriptModel, this, [this](const QJsonObject& object) {
        QSharedPointer<AbstractModel> t = CreateModel(object, m_data);
        t->InitialGuess();
        m_history = false;
        ActiveModel(t);
    });

    connect(modelwidget->getMinimizer().data(), SIGNAL(Message(QString, int)), this, SIGNAL(Message(QString, int)), Qt::DirectConnection);
    connect(modelwidget->getMinimizer().data(), SIGNAL(Warning(QString, int)), this, SIGNAL(MessageBox(QString, int)), Qt::DirectConnection);
    connect(modelwidget, SIGNAL(Warning(QString, int)), this, SIGNAL(MessageBox(QString, int)), Qt::DirectConnection);
    connect(this, &ModelDataHolder::recalculate, modelwidget, &ModelWidget::recalculate);

    connect(modelwidget->Jobs(), &JobManager::incremented, m_statistic_dialog, &StatisticDialog::IncrementProgress, Qt::DirectConnection);
    connect(modelwidget->Jobs(), &JobManager::prepare, m_statistic_dialog, &StatisticDialog::MaximumSteps, Qt::DirectConnection);
    connect(modelwidget->Jobs(), &JobManager::Message, m_statistic_dialog, &StatisticDialog::Message, Qt::DirectConnection);

    connect(m_statistic_dialog, &StatisticDialog::Interrupt, modelwidget, &ModelWidget::Interrupt, Qt::DirectConnection);

    m_modelsWidget->addModelsTab(modelwidget);
    m_last_tab = m_modelsWidget->currentIndex();
    m_models << t;
    m_model_widgets << modelwidget;

    ActiveBatch();

    m_ReductionCutoff = qMax(m_ReductionCutoff, t->ReductionCutOff());
    m_last_modelwidget = modelwidget;
    if (m_metamodelwidget) {
        QStringList colors = modelwidget->Chart().signal_wrapper->ColorList().split("|");
        colors.removeDuplicates();
        QColor color;
        if (colors.size() > 1) {
            color = Qt::green;
        } else
            color = (QColor(colors.first()));
        m_metamodelwidget->LinkModel(t, color);
        connect(modelwidget, &ModelWidget::ColorChanged, this, [this, t](const QColor& color) {
            m_metamodelwidget->UpdateColor(t, color);
        });
    }
    
    // Claude Generated - ProjectManager Integration for model tracking
    if (!m_currentProjectId.isEmpty() && t) {
        SupraFit::ProjectManager& projectManager = SupraFit::ProjectManager::instance();
        bool success = projectManager.addModelToProject(t, m_currentProjectId);
        if (success) {
            qDebug() << "ModelDataHolder::ActiveModel: Successfully registered model with ProjectManager for project" << m_currentProjectId;
        } else {
            qDebug() << "ModelDataHolder::ActiveModel: Failed to register model with ProjectManager for project" << m_currentProjectId;
        }
    }
    
    emit ModelAdded();

    // Claude Generated: Fix for LineSeries visibility issue in individual model addition
    // Claude Generated: Clean replacement for timer hack - immediate state application
    if (m_charts && m_charts->getDataWrapper()) {
#ifdef DEBUG_ON
        qDebug() << "🔍 DEBUG ActiveModel: m_charts valid, getDataWrapper() returns:" << m_charts->getDataWrapper().data()
                 << "about to call setAllSeriesVisible(true)";
#endif
        // Ensure all series are visible by default - NO TIMER NEEDED
        m_charts->getDataWrapper()->setAllSeriesVisible(true);
        qDebug() << "✅ DEBUG ActiveModel: Applied immediate setAllSeriesVisible(true) for individual model addition";
#ifdef DEBUG_ON
        qDebug() << "🔍 DEBUG ActiveModel: setAllSeriesVisible(true) call completed";
#endif
    }
#ifdef DEBUG_ON
    else {
        qDebug() << "⚠️  WARNING ActiveModel: m_charts or getDataWrapper() is null - m_charts:"
                 << (m_charts ? "valid" : "null")
                 << "getDataWrapper():" << (m_charts && m_charts->getDataWrapper() ? "valid" : "null");
    }
#endif
}

void ModelDataHolder::createModelWidgetFromModel(QSharedPointer<AbstractModel> model)
{
    // Claude Generated - Create ModelWidget from ProjectManager-loaded model with safety checks
    if (!model) {
        qWarning() << "ModelDataHolder::createModelWidgetFromModel: Invalid model provided";
        return;
    }
    
    // Safety checks before widget creation - Claude Generated
    if (!m_charts) {
        qWarning() << "ModelDataHolder::createModelWidgetFromModel: Charts wrapper is null";
        return;
    }
    
    if (!m_data || !m_data.toStrongRef()) {
        qWarning() << "ModelDataHolder::createModelWidgetFromModel: Data reference is invalid";
        return;
    }
    
    qDebug() << "🔍 DEBUG ModelDataHolder::createModelWidgetFromModel: Creating widget for model" << model->Name() << "ModelUUID:" << model->ModelUUID();
    
    try {
        // Use existing ActiveModel logic with empty JSON object (no color/key info from ProjectManager)
        QJsonObject emptyObject;
        ActiveModel(model, emptyObject);
        qDebug() << "✅ DEBUG ModelDataHolder::createModelWidgetFromModel: Successfully created widget for model" << model->Name();
    } catch (const std::exception& e) {
        qWarning() << "ModelDataHolder::createModelWidgetFromModel: Exception during widget creation:" << e.what();
    } catch (...) {
        qWarning() << "ModelDataHolder::createModelWidgetFromModel: Unknown exception during widget creation for model" << model->Name();
    }
}

void ModelDataHolder::ensureModelWidget(QSharedPointer<AbstractModel> model)
{
    if (!model || findModelTabByUUID(model->ModelUUID()) != -1)
        return;
    createModelWidgetFromModel(model);
}

void ModelDataHolder::syncModelsWithProjectManager()
{
    // Claude Generated - Synchronize ModelDataHolder with ProjectManager models
    if (m_currentProjectId.isEmpty()) {
        qDebug() << "ModelDataHolder::syncModelsWithProjectManager: No current project ID set";
        return;
    }
    
    // Claude Generated - CRITICAL FIX for race condition: Ensure ChartWidget is initialized before model sync
    if (m_charts && m_data) {
        auto dataRef = m_data.toStrongRef();
        if (dataRef) {
            qDebug() << "🔍 DEBUG syncModelsWithProjectManager: Initializing ChartWidget before model sync";
            m_charts->setRawData(dataRef);
            qDebug() << "✅ DEBUG syncModelsWithProjectManager: ChartWidget initialized successfully";
        }
    } else {
        qWarning() << "❌ DEBUG syncModelsWithProjectManager: Cannot initialize ChartWidget - m_charts or m_data is null";
        if (!m_charts) qWarning() << "   - m_charts is null";
        if (!m_data) qWarning() << "   - m_data is null";
    }
    
    qDebug() << "🔍 DEBUG ModelDataHolder::syncModelsWithProjectManager: Syncing models for project" << m_currentProjectId;
    
    // Get all models from ProjectManager for current project
    auto models = SupraFit::ProjectManager::instance().getProjectModels(m_currentProjectId);
    
    qDebug() << "🔍 DEBUG ModelDataHolder::syncModelsWithProjectManager: Found" << models.size() << "models to sync";

    // Claude Generated FIX: Prevent widget duplication on project reload
    // Create ModelWidgets for all models - check for existing widgets first
    for (auto& model : models) {
        if (model) {
            QString modelUUID = model->ModelUUID();

            // Check if widget already exists for this model
            int existingTab = findModelTabByUUID(modelUUID);
            if (existingTab != -1) {
                qDebug() << "⏭️  DEBUG syncModelsWithProjectManager: Model" << model->Name()
                         << "UUID:" << modelUUID << "already has widget at tab" << existingTab
                         << "- skipping duplicate creation";
                continue;  // Skip creation, widget already exists
            }

            void* modelPtr = model.data();
            qDebug() << "🔍 DEBUG ModelDataHolder::syncModelsWithProjectManager: Creating NEW widget for model" << model->Name() << "Pointer:" << modelPtr << "UUID:" << modelUUID;
            createModelWidgetFromModel(model);
        }
    }

    // Claude Generated: Fix for LineSeries visibility issue in project loading
    // Claude Generated: Clean replacement for timer hack - immediate state application
    // After all models are loaded, ensure all series are visible
    if (m_charts && !models.empty()) {
        qDebug() << "🔧 DEBUG syncModelsWithProjectManager: Applying immediate setAllSeriesVisible(true) for" << models.size() << "models";
        // Apply state immediately - NO TIMER NEEDED
        if (m_charts && m_charts->getDataWrapper()) {
            m_charts->getDataWrapper()->setAllSeriesVisible(true);
            qDebug() << "✅ DEBUG syncModelsWithProjectManager: Applied immediate setAllSeriesVisible(true) for project loading";
        }
    }

    qDebug() << "✅ DEBUG ModelDataHolder::syncModelsWithProjectManager: Synchronization complete";
}

void ModelDataHolder::addMetaModel(QSharedPointer<AbstractModel> t)
{
    ActiveModel(t, QJsonObject(), true);
}

void ModelDataHolder::ActiveBatch()
{
    m_TitleBarWidget->EnableBatch(m_modelsWidget->count() > 2);
}

void ModelDataHolder::RemoveTab(int i)
{
    if (qobject_cast<ModelWidget*>(m_modelsWidget->widget(i))) {
        ModelWidget* model = qobject_cast<ModelWidget*>(m_modelsWidget->widget(i));
        
        // Claude Generated: Get strong reference to AbstractModel before deleting ModelWidget to prevent use-after-free
        QSharedPointer<AbstractModel> modelPtr = model->Model();
        AbstractModel* rawModel = modelPtr.data();
        
        for (int j = 0; j < m_model_widgets.size(); ++j) {
            if (m_model_widgets[j] == model)
                m_model_widgets.remove(j);
        }
        m_modelsWidget->removeTab(i);
        m_models.remove(m_models.indexOf(modelPtr));
        
        // Delete ModelWidget first, then handle AbstractModel cleanup
        delete model;

        if (qobject_cast<MetaModel*>(m_data))
            qobject_cast<MetaModel*>(m_data)->RemoveModel(rawModel);
        else {
            // Let QSharedPointer handle deletion automatically when modelPtr goes out of scope
            // No manual delete needed - prevents double deletion
        }
        emit ModelRemoved();
    }
    ActiveBatch();
}

void ModelDataHolder::setSettings(const QJsonObject& config)
{
    for (int i = 0; i < m_modelsWidget->count(); ++i) {
        ModelWidget* w = qobject_cast<ModelWidget*>(m_modelsWidget->widget(i));
        if (w != 0) {
            w->Model()->setOptimizerConfig(config);
            m_config = config;
        }
    }
}

void ModelDataHolder::SaveCurrentModels(const QString& file)
{
    QJsonObject toplevel;
    for (int i = 1; i < m_modelsWidget->count(); i++) {
        if (qobject_cast<ModelWidget*>(m_modelsWidget->widget(i))) {
            ModelWidget* model = qobject_cast<ModelWidget*>(m_modelsWidget->widget(i));
            QJsonObject obj = model->Model()->ExportModel();
            obj["colors"] = model->Chart().signal_wrapper->ColorList();
            obj["keys"] = model->Keys();
            toplevel["model_" + QString::number(i)] = obj;
        }
    }
    JsonHandler::WriteJsonFile(toplevel, file);
}

QJsonObject ModelDataHolder::SaveWorkspace()
{
    QJsonObject toplevel, data;
    data = m_data.toStrongRef()->ExportData();

    if (m_datawidget) {
        for (int i = 1; i < m_modelsWidget->count(); i++) {
            if (qobject_cast<ModelWidget*>(m_modelsWidget->widget(i))) {
                ModelWidget* model = qobject_cast<ModelWidget*>(m_modelsWidget->widget(i));
                QJsonObject obj = model->Model()->ExportModel();
                obj["colors"] = model->Chart().signal_wrapper->ColorList();
                obj["keys"] = model->Keys();
                data["colors"] = model->Chart().data_wrapper->ColorList();
                toplevel["model_" + QString::number(i)] = obj;
            }
        }
        toplevel["data"] = data;
    } else {
        toplevel["data"] = m_metamodelwidget->Model()->ExportModel(true, false);
    }
    return toplevel;
}

QJsonObject ModelDataHolder::SaveModel(int index)
{
    QJsonObject toplevel, data;
    data = m_data.toStrongRef()->ExportData();

    if (qobject_cast<ModelWidget*>(m_modelsWidget->widget(index))) {
        ModelWidget* model = qobject_cast<ModelWidget*>(m_modelsWidget->widget(index));
        QJsonObject obj = model->Model()->ExportModel();
        obj["colors"] = model->Chart().signal_wrapper->ColorList();
        obj["keys"] = model->Keys();
        data["colors"] = model->Chart().data_wrapper->ColorList();
        toplevel["model_0"] = obj;
    }

    toplevel["data"] = data;
    return toplevel;
}

void ModelDataHolder::AddToWorkspace(const QJsonObject& object)
{
    QStringList keys = object.keys();
    setEnabled(false);
    m_wrapper.toStrongRef()->stopAnimiation();

    for (const QString& key : qAsConst(keys)) {
        if (key == "data")
            continue;

        QApplication::processEvents();
        QJsonObject model = object[key].toObject();
        Json2Model(model);
    }

    setEnabled(true);
    m_wrapper.toStrongRef()->restartAnimation();
    emit ModelAdded();
}

void ModelDataHolder::LoadCurrentProject(const QJsonObject& object)
{
    QStringList keys = object.keys();
    if (keys.contains("data") && keys.contains("model")) {
        if (m_modelsWidget->currentIndex() == 0) {
            if (m_modelsWidget->count() < 2)
                return;
            m_modelsWidget->setCurrentIndex(1);
        }
        QMessageBox::StandardButton replay;
        replay = QMessageBox::information(this, tr("Override Model."), tr("Do you want to override the current loaded model [Y]\nor just add to workspace [N]?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (replay == QMessageBox::Yes) {
            ModelWidget* model = qobject_cast<ModelWidget*>(m_modelsWidget->currentWidget());
            model->LoadJson(object);
        } else if (replay == QMessageBox::No) {
            AddToWorkspace(object);
        }
    }
}

void ModelDataHolder::CloseAll()
{
    QMessageBox::StandardButton replay;
    QString app_name = QString(qApp->instance()->applicationName());
    replay = QMessageBox::information(this, tr("Remove All."), tr("Do you really want to remove all models from the current project?"), QMessageBox::Yes | QMessageBox::No);
    if (replay == QMessageBox::Yes) {
        CloseAllForced();
    }
}

void ModelDataHolder::CloseAllForced()
{
    // Claude Generated: Fix off-by-one error - valid indices are 0 to count()-1
    for (int i = m_modelsWidget->count() - 1; i >= 0; --i)
        RemoveTab(i);
}

void ModelDataHolder::RunJobs(const QJsonObject& job)
{
    Waiter wait;

    int run = 0;
    for (int i = 0; i < m_model_widgets.size(); ++i) {
        if (!m_model_widgets[i])
            continue;
        if (m_statistic_dialog->UseChecked() && !m_model_widgets[i]->isChecked())
            continue;
        run++;
    }
    m_statistic_dialog->MaximumMainSteps(run);

    m_allow_loop = true;
    for (int i = 0; i < m_model_widgets.size(); ++i) {
        if (!m_model_widgets[i])
            continue;

        if (m_statistic_dialog->UseChecked() && !m_model_widgets[i]->isChecked())
            continue;

        if (i < m_modelsWidget->count())
            m_modelsWidget->setCurrentIndex(i + 1);

        m_statistic_dialog->ShowWidget();
        m_model_widgets[i]->setJob(job);
        m_statistic_dialog->IncrementMainProgress();
        if (!m_allow_loop)
            break;
    }
    m_statistic_dialog->HideWidget();
}

void ModelDataHolder::OptimizeAll()
{
    for (int i = 1; i < m_modelsWidget->count(); i++) {
        if (qobject_cast<ModelWidget*>(m_modelsWidget->widget(i))) {
            if (!m_model_widgets[i - 1]->isChecked())
                continue;
            m_modelsWidget->setCurrentIndex(i);
            ModelWidget* model = qobject_cast<ModelWidget*>(m_modelsWidget->widget(i));
            model->GlobalMinimize();
        }
    }
}

void ModelDataHolder::HideSubWindows(int index)
{
    if (qobject_cast<ModelWidget*>(m_modelsWidget->widget(m_last_tab))) {
        ModelWidget* model = qobject_cast<ModelWidget*>(m_modelsWidget->widget(m_last_tab));
        model->HideAllWindows();
        m_last_tab = index;
    }
}

void ModelDataHolder::CompareAIC()
{
    QVector<QWeakPointer<AbstractModel>> models;
    for (int i = 1; i < m_modelsWidget->count(); i++) {
        if (!m_model_widgets[i - 1]->isChecked())
            continue;
        if (qobject_cast<ModelWidget*>(m_modelsWidget->widget(i))) {
            ModelWidget* modelwidget = qobject_cast<ModelWidget*>(m_modelsWidget->widget(i));
            models << modelwidget->Model();
        }
    }

    QString result = StatisticTool::CompareAIC(models);

    QHBoxLayout* layout = new QHBoxLayout;
    QTextEdit* text = new QTextEdit;
    text->setText("<html><pre>" + result + "</pre></html>");
    layout->addWidget(text);
    QDialog dialog(this);
    dialog.setLayout(layout);
    dialog.resize(1024, 800);
    dialog.exec();
}

void ModelDataHolder::CompareCV()
{
    if (!m_compare_dialog)
        return;

    int cvtype = m_compare_dialog->CVType();
    int x = m_compare_dialog->CVX();
    QVector<QJsonObject> models;
    for (int i = 1; i < m_modelsWidget->count(); i++) {
        if (!m_model_widgets[i - 1]->isChecked())
            continue;
        if (qobject_cast<ModelWidget*>(m_modelsWidget->widget(i))) {
            ModelWidget* modelwidget = qobject_cast<ModelWidget*>(m_modelsWidget->widget(i));
            QJsonObject model = modelwidget->Model()->ExportModel();
            models << model;
        }
    }

    QString result = StatisticTool::CompareCV(models, cvtype, m_compare_dialog->CVLocal(), m_compare_dialog->CVX());

    QHBoxLayout* layout = new QHBoxLayout;
    TextWidget* text = new TextWidget;
    // result.replace(",", ".");
    text->setText("<html><pre>" + result + "</pre></html>");
    layout->addWidget(text);
    QDialog dialog(this);
    dialog.setLayout(layout);
    dialog.resize(1024, 800);
    dialog.exec();
}

void ModelDataHolder::CompareReduction()
{
    if (!m_compare_dialog)
        return;

    qreal cutoff = m_compare_dialog->CutOff();
    bool local = m_compare_dialog->RedLocal();

    QVector<QJsonObject> models;
    for (int i = 1; i < m_modelsWidget->count(); i++) {
        if (!m_model_widgets[i - 1]->isChecked())
            continue;
        if (qobject_cast<ModelWidget*>(m_modelsWidget->widget(i))) {
            ModelWidget* modelwidget = qobject_cast<ModelWidget*>(m_modelsWidget->widget(i));
            QJsonObject model = modelwidget->Model()->ExportModel(true);
            models << model;
        }
    }

    QString result = StatisticTool::AnalyseReductionAnalysis(models, local, cutoff);

    QHBoxLayout* layout = new QHBoxLayout;
    TextWidget* text = new TextWidget;
    // result.replace(",", ".");

    text->setText("<html><pre>" + result + "</pre></html>");
    layout->addWidget(text);
    QDialog dialog(this);
    dialog.setLayout(layout);
    dialog.resize(1024, 800);
    dialog.exec();
}

void ModelDataHolder::CompareMC()
{
    if (!m_compare_dialog)
        return;

    QVector<QJsonObject> models;
    for (int i = 1; i < m_modelsWidget->count(); i++) {
        if (!m_model_widgets[i - 1]->isChecked())
            continue;
        if (qobject_cast<ModelWidget*>(m_modelsWidget->widget(i))) {
            ModelWidget* modelwidget = qobject_cast<ModelWidget*>(m_modelsWidget->widget(i));
            QJsonObject model = modelwidget->Model()->ExportModel();
            models << model;
        }
    }

    QString result = StatisticTool::CompareMC(models, m_compare_dialog->CVLocal(), m_compare_dialog->CVX());

    QHBoxLayout* layout = new QHBoxLayout;
    TextWidget* text = new TextWidget;
    //    result.replace(",",".");

    text->setText("<html><pre>" + result + "</pre></html>");
    layout->addWidget(text);
    QDialog dialog(this);
    dialog.setLayout(layout);
    dialog.resize(1024, 800);
    dialog.exec();
}

void ModelDataHolder::EditData()
{
    int version = m_data.toStrongRef()->ExportData()["SupraFit"].toInt();
    if (version < 1602) {
        QMessageBox::information(this, tr("Old SupraFit file"), tr("This is an older SupraFit file, you can only edit the table in Workspace!"));
        ToggleTablesEditable();
    } else {
        if (m_data.toStrongRef()->DataType() == DataClassPrivate::Thermogram) {
            ImportData dialog(m_data);
            dialog.setRootDir(m_data.toStrongRef()->RootDir());
            if (dialog.exec() == QDialog::Accepted) { // I dont like this either ....
                {
                    if (m_data.toStrongRef()->DataType() == DataClassPrivate::Thermogram) {
                        m_data.toStrongRef()->ImportData(dialog.getStoredData().ExportData(), false);
                        emit m_data.toStrongRef()->Info()->Update();
                    }
                }
            }
            emit m_data.toStrongRef()->Info()->Update();

        } else if (m_data.toStrongRef()->DataType() == DataClassPrivate::Spectrum) {
            SpectraImport dialog;
            dialog.setData(m_data.toStrongRef().data()->ExportData());
            if (dialog.exec() == QDialog::Accepted) {
                SpectraEdited(dialog.InputTable(), dialog.ProjectData());
            }
        } else {
            ToggleTablesEditable();
        }
    }
}

/*! \brief Switch both data tables between editable and read-only, show the data tab and report the
 * new state in the log; the Edit Data action shows it as its checked state. Claude Generated. */
void ModelDataHolder::ToggleTablesEditable()
{
    auto data = m_data.toStrongRef();
    if (!data)
        return;
    const bool editable = !data->DependentModel()->isEditable();
    data->IndependentModel()->setEditable(editable);
    data->DependentModel()->setEditable(editable);
    m_TitleBarWidget->setEditDataChecked(editable);
    if (editable)
        m_modelsWidget->setCurrentIndex(0); // the data tab holds the tables
    emit Message(editable ? tr("Data tables of %1 are editable now.").arg(data->ProjectTitle())
                          : tr("Data tables of %1 are read-only again.").arg(data->ProjectTitle()),
        3);
}

void ModelDataHolder::setCurrentTab(int index)
{
    if (index < m_modelsWidget->count())
        m_modelsWidget->setCurrentIndex(index);
}

QString ModelDataHolder::Compare() const
{
    QString compare;

    compare += "<h4>Models Overview - Some important information right away</h4>";
    compare += "<table>";
    compare += "<tr><th>model name</th><th># parameter</th><th>SSE</th><th>SE<sub>y</sub></th><th>&sigma;</th></tr>";

    for (const QWeakPointer<AbstractModel>& model : m_models) {
        compare += tr("<tr><td>%1</td><td align='center'>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>").arg(model.toStrongRef()->Name()).arg(model.toStrongRef()->Parameter()).arg(Print::printDouble(model.toStrongRef()->SSE(), 6)).arg(Print::printDouble(model.toStrongRef()->SEy(), 6)).arg(Print::printDouble(model.toStrongRef()->StdDeviation(), 6));
    }
    compare += "</table>";
    compare += "<h5>More information about the Monte Carlo methods and Resampling plans can be found at:</h5>";
    compare += "<a href='https://doi.org/10.7717/peerj-achem.23'>C. Hübler, PeerJ Analytical Chemistry, 2022, 4:e23 https://doi.org/10.7717/peerj-achem.23</a>";

    return compare;
}

void ModelDataHolder::SplitData()
{
    if (!m_data)
        return;

    DataTable* dep = new DataTable;
    dep->ImportTable(m_data.toStrongRef().data()->DependentModel()->ExportTable(true));

    DataTable* dep_raw = new DataTable;
    dep_raw->ImportTable(m_data.toStrongRef().data()->DependentRawModel()->ExportTable(true));
    QStringList header = dep->header();
    QJsonObject project = m_data.toStrongRef().data()->ExportData();
    for (int i = 0; i < dep->columnCount(); ++i) {
        DataTable* tmp1 = dep->BlockColumns(i, 1);
        DataTable* tmp2 = dep_raw->BlockColumns(i, 1);

        project["dependent"] = tmp1->ExportTable(true);
        project["dependent_raw"] = tmp2->ExportTable(true);

        delete tmp1;
        delete tmp2;
        QUuid uuid;
        project["uuid"] = uuid.createUuid().toString();
        project["title"] = header[i];
        QJsonObject d;
        d["data"] = project;
        emit AddProject(d);
    }
}

// Claude Generated - ProjectManager Integration Methods Implementation
bool ModelDataHolder::setDataFromProjectManager(const QString& projectId, QSharedPointer<ChartWrapper> wrapper)
{
    if (projectId.isEmpty()) {
        qWarning() << "ModelDataHolder::setDataFromProjectManager: Empty project ID provided";
        return false;
    }

    SupraFit::ProjectManager& projectManager = SupraFit::ProjectManager::instance();
    QSharedPointer<DataClass> dataClass = projectManager.getProjectData(projectId);
    
    if (!dataClass) {
        qWarning() << "ModelDataHolder::setDataFromProjectManager: Project not found in ProjectManager:" << projectId;
        return false;
    }

    qDebug() << "ModelDataHolder::setDataFromProjectManager: Setting data from ProjectManager for project" << projectId;

    // Claude Generated - CRITICAL FIX: Get the populated ChartWrapper from ChartWidget
    // ChartWidget::setRawData() creates and returns a populated ChartWrapper
    qDebug() << "🔧 DEBUG setDataFromProjectManager: m_charts=" << (m_charts ? "valid" : "null")
             << "dataClass=" << (dataClass ? "valid" : "null");

    QSharedPointer<ChartWrapper> populatedWrapper;
    if (m_charts && dataClass) {
        qDebug() << "🔧 ModelDataHolder::setDataFromProjectManager: Initializing ChartWidget to get populated wrapper";
        populatedWrapper = m_charts->setRawData(dataClass);

        if (populatedWrapper && populatedWrapper->SeriesSize() > 0) {
            qDebug() << "✅ ModelDataHolder::setDataFromProjectManager: Got populated wrapper with" << populatedWrapper->SeriesSize() << "series";
        } else {
            qDebug() << "❌ ModelDataHolder::setDataFromProjectManager: ChartWidget returned empty wrapper";
            return false;
        }
    } else {
        qDebug() << "❌ ModelDataHolder::setDataFromProjectManager: Cannot initialize ChartWidget - m_charts or dataClass is null";
        return false; // Claude Generated - Fail early if ChartWidget can't be initialized
    }

    // Now use the populated wrapper from ChartWidget instead of the original empty one
    setData(dataClass, populatedWrapper);

    // Claude Generated FIX 3: Project reload handling
    // Check if we're reloading the same project to inform user
    bool isReload = (m_currentProjectId == projectId);
    if (isReload) {
        qDebug() << "⚠️  DEBUG setDataFromProjectManager: Reloading project" << projectId
                 << "- existing widgets will be preserved and validated";
        // Note: With Fix 1 (idempotent sync), duplicate widgets are prevented
        // by UUID checking in syncModelsWithProjectManager()
    }

    // Store the project ID for future reference
    m_currentProjectId = projectId;

    // Synchronize models after successful data loading
    // Fix 1 ensures that if widgets already exist for models, they won't be duplicated
    syncModelsWithProjectManager();

    qDebug() << "ModelDataHolder::setDataFromProjectManager: Successfully set data for project" << projectId;
    return true;
}

QString ModelDataHolder::getCurrentProjectId() const
{
    return m_currentProjectId;
}

int ModelDataHolder::findModelTabByUUID(const QString& modelUUID) const
{
    // Claude Generated - Find tab index for model with given UUID with comprehensive debug logging
    qDebug() << "🔍 DEBUG findModelTabByUUID: Looking for model UUID:" << modelUUID;
    qDebug() << "🔍 DEBUG findModelTabByUUID: Total tabs in modelsWidget:" << m_modelsWidget->count();
    
    // Tab 0 is always the data overview tab, models start at index 1
    for (int i = 1; i < m_modelsWidget->count(); ++i) {
        ModelWidget* modelWidget = qobject_cast<ModelWidget*>(m_modelsWidget->widget(i));
        qDebug() << "🔍 DEBUG findModelTabByUUID: Tab" << i << "ModelWidget:" << (modelWidget ? "exists" : "null");
        
        if (modelWidget && modelWidget->Model()) {
            QString tabModelUUID = modelWidget->Model()->ModelUUID();
            qDebug() << "🔍 DEBUG findModelTabByUUID: Tab" << i << "Model UUID:" << tabModelUUID;
            qDebug() << "🔍 DEBUG findModelTabByUUID: Tab" << i << "UUID match:" << (tabModelUUID == modelUUID);
            
            if (tabModelUUID == modelUUID) {
                qDebug() << "✅ DEBUG findModelTabByUUID: Found model at tab index" << i;
                return i;
            }
        } else {
            qDebug() << "❌ DEBUG findModelTabByUUID: Tab" << i << "has no model or widget";
        }
    }
    qDebug() << "❌ DEBUG findModelTabByUUID: Model not found, returning -1";
    return -1; // Model not found
}

int ModelDataHolder::findModelTabByPointer(void* modelPointer) const
{
    // Claude Generated - Find tab index for model with given model pointer
    qDebug() << "🔍 DEBUG findModelTabByPointer: Looking for model pointer:" << modelPointer;
    qDebug() << "🔍 DEBUG findModelTabByPointer: Total tabs in modelsWidget:" << m_modelsWidget->count();
    
    // Tab 0 is always the data overview tab, models start at index 1
    for (int i = 1; i < m_modelsWidget->count(); ++i) {
        ModelWidget* modelWidget = qobject_cast<ModelWidget*>(m_modelsWidget->widget(i));
        qDebug() << "🔍 DEBUG findModelTabByPointer: Tab" << i << "ModelWidget:" << (modelWidget ? "exists" : "null");
        
        if (modelWidget && modelWidget->Model()) {
            void* tabModelPointer = modelWidget->Model().data();
            qDebug() << "🔍 DEBUG findModelTabByPointer: Tab" << i << "Model pointer:" << tabModelPointer;
            qDebug() << "🔍 DEBUG findModelTabByPointer: Tab" << i << "Pointer match:" << (tabModelPointer == modelPointer);
            
            if (tabModelPointer == modelPointer) {
                qDebug() << "✅ DEBUG findModelTabByPointer: Found model at tab index" << i;
                return i;
            }
        } else {
            qDebug() << "❌ DEBUG findModelTabByPointer: Tab" << i << "has no model or widget";
        }
    }
    qDebug() << "❌ DEBUG findModelTabByPointer: Model not found, returning -1";
    return -1; // Model not found
}

// Claude Generated - Find tab by model pointer and child index to differentiate duplicate model instances
int ModelDataHolder::findModelTabByChildIndex(void* modelPointer, int childIndex) const
{
    qDebug() << "🔍 DEBUG findModelTabByChildIndex: Looking for model pointer:" << modelPointer << "childIndex:" << childIndex;
    qDebug() << "🔍 DEBUG findModelTabByChildIndex: Total tabs in modelsWidget:" << m_modelsWidget->count();
    
    int matchingPointerCount = 0;
    
    // Tab 0 is always the data overview tab, models start at index 1
    for (int i = 1; i < m_modelsWidget->count(); ++i) {
        ModelWidget* modelWidget = qobject_cast<ModelWidget*>(m_modelsWidget->widget(i));
        qDebug() << "🔍 DEBUG findModelTabByChildIndex: Tab" << i << "ModelWidget:" << (modelWidget ? "exists" : "null");
        
        if (modelWidget && modelWidget->Model()) {
            void* tabModelPointer = modelWidget->Model().data();
            qDebug() << "🔍 DEBUG findModelTabByChildIndex: Tab" << i << "Model pointer:" << tabModelPointer;
            
            if (tabModelPointer == modelPointer) {
                qDebug() << "🔍 DEBUG findModelTabByChildIndex: Tab" << i << "Pointer matches, checking child index";
                qDebug() << "🔍 DEBUG findModelTabByChildIndex: Tab" << i << "Current match count:" << matchingPointerCount << "Target index:" << childIndex;
                
                if (matchingPointerCount == childIndex) {
                    qDebug() << "✅ DEBUG findModelTabByChildIndex: Found model at tab index" << i << "for child index" << childIndex;
                    return i;
                }
                matchingPointerCount++;
            }
        } else {
            qDebug() << "❌ DEBUG findModelTabByChildIndex: Tab" << i << "has no model or widget";
        }
    }
    qDebug() << "❌ DEBUG findModelTabByChildIndex: Model not found, returning -1";
    return -1; // Model not found
}



#include "modeldataholder.moc"

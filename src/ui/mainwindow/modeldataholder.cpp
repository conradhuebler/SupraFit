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

#include "src/global.h"
#include "src/global_config.h"

#include "src/capabilities/jobmanager.h"
#include "src/capabilities/modelcomparison.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/capabilities/resampleanalyse.h"
#include "src/capabilities/weakenedgridsearch.h"

#include "src/core/analyse.h"
#include "src/core/jsonhandler.h"
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

#include "modeldataholder.h"

void ToolButton::ChangeColor(const QColor& color)
{
    setStyleSheet("background-color:" + color.name() + ";");
}

TabWidget::TabWidget(QWidget* parent)
    : QTabWidget(parent)
{
    setStyleSheet("QTabBar::tab { height: 20px;}");
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
    layout->addWidget(title_label);
    tools->setLayout(layout);

    connect(hide, SIGNAL(stateChanged(int)), modelwidget, SIGNAL(ToggleSeries(int)));
    connect(color, SIGNAL(clicked()), modelwidget, SLOT(ChangeColor()));
    connect(modelwidget, SIGNAL(ColorChanged(QColor)), color, SLOT(ChangeColor(QColor)));

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
    addTab(datawidget, "Overview: " + qApp->instance()->property("projectname").toString());
    tabBar()->setTabButton(0, QTabBar::RightSide, 0);
    m_datawidget = datawidget;
}

void TabWidget::setMetaTab(QPointer<MetaModelWidget> datawidget)
{
    addTab(datawidget, "Overview: " + qApp->instance()->property("projectname").toString());
    tabBar()->setTabButton(0, QTabBar::RightSide, 0);
    m_metamodelwidget = datawidget;
}

MDHDockTitleBar::MDHDockTitleBar()
{

    m_buttons = new QWidget;
    m_buttons->setEnabled(false);

    /*
    m_hide = new QPushButton;
    m_hide->setFlat(true);
    m_hide->setIcon(QIcon::fromTheme("tab-close"));
    */
    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(new QLabel("Workspace"));
    layout->addWidget(m_buttons);

    m_edit_data = new QPushButton(tr("Edit Data"));
    m_edit_data->setFlat(true);
    m_edit_data->setIcon(Icon("document-edit"));
    connect(m_edit_data, &QPushButton::clicked, this, &MDHDockTitleBar::EditData);

    m_add_nmr = new QPushButton(tr("NMR Titratrion"));
    m_add_nmr->setFlat(true);
    m_add_nmr->setIcon(Icon("list-add"));

    m_add_uv_vis = new QPushButton(tr("UV/VIS Titratrion"));
    m_add_uv_vis->setFlat(true);
    m_add_uv_vis->setIcon(Icon("list-add"));

    m_add_fl = new QPushButton(tr("Fluorescence Titratrion"));
    m_add_fl->setFlat(true);
    m_add_fl->setIcon(Icon("list-add"));

    m_add_itc = new QPushButton(tr("Calorimetry"));
    m_add_itc->setFlat(true);
    m_add_itc->setIcon(Icon("list-add"));

    m_add_kinetics = new QPushButton(tr("Kinetics"));
    m_add_kinetics->setFlat(true);
    m_add_kinetics->setIcon(Icon("list-add"));

    m_optimize = new QPushButton(tr("Optimise All"));
    m_optimize->setToolTip(tr("Optimise all checked models"));
    m_optimize->setFlat(true);
    m_optimize->setIcon(Icon("go-down"));
    connect(m_optimize, &QPushButton::clicked, this, &MDHDockTitleBar::OptimizeAll);

    m_statistics = new QPushButton(tr("Statistics"));
    m_statistics->setToolTip(tr("Open Statistics Dialog for all models."));
    m_statistics->setFlat(true);
    m_statistics->setIcon(Icon("fork"));
    connect(m_statistics, &QPushButton::clicked, this, &MDHDockTitleBar::ShowStatistics);

    m_analyse = new QPushButton(tr("Analyse and Compare"));
    m_analyse->setToolTip(tr("Open Compare Dialog to compare all models some of the statistical results like Monte Carlo, CrossValidation and Reduction Analysis."));
    m_analyse->setFlat(true);
    m_analyse->setIcon(Icon("help-hint"));
    connect(m_analyse, &QPushButton::clicked, this, &MDHDockTitleBar::Compare);

    m_close_all = new QPushButton(tr("Remove All"));
    m_close_all->setToolTip(tr("Remove all models from the current project."));
    m_close_all->setFlat(true);
    m_close_all->setIcon(Icon("trash-empty"));
    m_close_all->setDisabled(true);
    connect(m_close_all, &QPushButton::clicked, this, &MDHDockTitleBar::CloseAll);


    auto addModel = [this](SupraFit::Model model) -> QAction* {
        QAction* action = new QAction(this);
        action->setText(tr("%1").arg(Model2Name(model)));
        action->setData(model);
        connect(action, &QAction::triggered, this, &MDHDockTitleBar::PrepareAddModel);
        return action;
    };

    //#ifdef NMR_Models
    m_nmr_model << addModel(SupraFit::nmr_ItoI);
    m_nmr_model << addModel(SupraFit::nmr_IItoI_ItoI);
    m_nmr_model << addModel(SupraFit::nmr_ItoI_ItoII);
    m_nmr_model << addModel(SupraFit::nmr_IItoI_ItoI_ItoII);
    m_nmr_model << addModel(SupraFit::nmr_any);

    //#endif

    //#ifdef Fluorescence_Models
    m_fl_model << addModel(SupraFit::fl_ItoI);
    m_fl_model << addModel(SupraFit::fl_IItoI_ItoI);
    m_fl_model << addModel(SupraFit::fl_ItoI_ItoII);
    m_fl_model << addModel(SupraFit::fl_IItoI_ItoI_ItoII);
    //#endif

    //#ifdef UV_VIS_Models
    m_uv_vis_model << addModel(SupraFit::uv_vis_ItoI);
    m_uv_vis_model << addModel(SupraFit::uv_vis_IItoI_ItoI);
    m_uv_vis_model << addModel(SupraFit::uv_vis_ItoI_ItoII);
    m_uv_vis_model << addModel(SupraFit::uv_vis_IItoI_ItoI_ItoII);
    m_uv_vis_model << addModel(SupraFit::uvvis_any);

    //#endif

#ifdef Kinetic_Models
    m_kinetcs_model << addModel(SupraFit::Michaelis_Menten);

    m_kinetcs_model << addModel(SupraFit::MonoMolecularModel);
    m_kinetcs_model << addModel(SupraFit::DecayRates);
#endif

#ifdef ITC_Models
    m_itc_fixed_model << addModel(SupraFit::itc_ItoI);
    m_itc_fixed_model << addModel(SupraFit::itc_IItoI);
    m_itc_fixed_model << addModel(SupraFit::itc_ItoII);
    m_itc_fixed_model << addModel(SupraFit::itc_IItoII);
    m_itc_fixed_model << addModel(SupraFit::itc_any);

    //m_itc_flex_model << addModel(SupraFit::itc_n_ItoI);
    m_itc_flex_model << addModel(SupraFit::itc_n_ItoII);

    m_itc_fixed_model << addModel(SupraFit::itc_blank);
#endif

    m_add_script = new QPushButton(tr("Load Model file"));
    m_add_script->setFlat(true);
    m_add_script->setIcon(Icon("list-add"));

    connect(m_add_script, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, tr("Custom Model"), getDir(), "*.json");
        emit this->AddScriptModel(file);
    });

    m_define_model = new QPushButton(tr("New Model"));
    m_define_model->setFlat(true);
    m_define_model->setIcon(Icon("list-add"));

    connect(m_define_model, &QPushButton::clicked, this, [this]() {
        emit this->NewModel();
    });

    m_any_model = new QPushButton(tr("Any Model"));
    m_any_model->setFlat(true);
    m_any_model->setIcon(QIcon(":/icons/list-add-blue.png"));
    connect(m_any_model, &QPushButton::clicked, this, [this]() {
        bool ok;
        int old = qApp->instance()->property("last_any").toInt();
        int i = QInputDialog::getInt(this, tr("I want user input of static model ID"),
            tr("ID:"), old, 1, 404, 1, &ok);
        if (ok) {
            this->m_model_choosen = i;
            qApp->instance()->setProperty("last_any", i);
            emit this->AddModel();
        }
    });

    QVBoxLayout* vlayout = new QVBoxLayout;

    QHBoxLayout* buttons = new QHBoxLayout;
    buttons->addWidget(m_define_model);
    buttons->addWidget(m_add_script);
    buttons->addWidget(m_add_nmr);
    buttons->addWidget(m_add_uv_vis);
    buttons->addWidget(m_add_fl);
    buttons->addWidget(m_add_itc);
    buttons->addWidget(m_add_kinetics);
    buttons->addWidget(m_any_model);
    buttons->addWidget(m_edit_data);

    m_any_model->setVisible(qApp->instance()->property("advanced_ui").toBool());

    vlayout->addLayout(buttons);

    buttons = new QHBoxLayout;
    buttons->addWidget(m_optimize);
    buttons->addStretch();
    buttons->addWidget(m_statistics);
    buttons->addWidget(m_analyse);
    buttons->addStretch();
    buttons->addWidget(m_close_all);

    vlayout->addLayout(buttons);

    m_buttons->setLayout(vlayout);
    //layout->addStretch();
    //layout->addWidget(m_hide);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    setLayout(layout);
}

void MDHDockTitleBar::addToMenu(int IndependetCount)
{

    auto addMenu = [](const QVector<QPointer<QAction>>& list, QMenu* menu) {
        for (const QPointer<QAction>& ptr : qAsConst(list))
            menu->addAction(ptr);
    };

    QMenu* menu = new QMenu(this);
    if (IndependetCount == 1) {
        addMenu(m_kinetcs_model, menu);
        m_add_kinetics->setMenu(menu);
        m_add_nmr->hide();
        m_add_uv_vis->hide();
        m_add_fl->hide();
        menu = new QMenu;
        QAction* action = menu->addSection(tr("Fixed Stoichiometry"));
        addMenu(m_itc_fixed_model, menu);
        m_add_itc->setMenu(menu);
        action = menu->addSection(tr("Flexible Stoichiometry"));
        addMenu(m_itc_flex_model, menu);
    } else if (IndependetCount == 2) {
        addMenu(m_nmr_model, menu);
        m_add_nmr->setMenu(menu);

        menu = new QMenu;
        addMenu(m_uv_vis_model, menu);
        m_add_uv_vis->setMenu(menu);

        menu = new QMenu;
        addMenu(m_fl_model, menu);
        m_add_fl->setMenu(menu);
        m_add_kinetics->hide();
        m_add_itc->hide();
    }
    menu = new QMenu(this);
}

void MDHDockTitleBar::HideModelTools()
{
    m_buttons->hide();
}

void MDHDockTitleBar::EnableBatch(bool enabled)
{
    m_close_all->setEnabled(enabled);
    m_statistics->setEnabled(enabled);
    m_optimize->setEnabled(enabled);
    m_analyse->setEnabled(enabled);
}

void MDHDockTitleBar::PrepareAddModel()
{
    m_model_choosen = qobject_cast<QAction*>(sender())->data().toInt();
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
    connect(m_modelsWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(RemoveTab(int)));
    connect(m_modelsWidget, SIGNAL(currentChanged(int)), this, SLOT(HideSubWindows(int)));

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

    connect(m_TitleBarWidget, &MDHDockTitleBar::ShowStatistics, m_statistic_dialog, &StatisticDialog::show);
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

void ModelDataHolder::setData(QSharedPointer<DataClass> data, QSharedPointer<ChartWrapper> wrapper)
{
    m_data = data;
    //connect(m_data.data(), &DataClass::Warning, this, &ModelDataHolder::Message);
    m_TitleBarWidget->setEnabled(true);
    m_wrapper = wrapper;
    if (!qobject_cast<MetaModel*>(data)) {
        m_datawidget = new DataWidget;
        m_datawidget->setData(m_data, wrapper);
        m_modelsWidget->setDataTab(m_datawidget);
        m_TitleBarWidget->addToMenu(m_data.toStrongRef()->IndependentModel()->columnCount());
        connect(m_datawidget, SIGNAL(NameChanged()), this, SLOT(SetProjectTabName()));
        connect(m_datawidget, SIGNAL(recalculate()), this, SIGNAL(recalculate()));
    } else {
        m_metamodelwidget = new MetaModelWidget;
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
            AddModel(models[key].toInt());
        }
    }
}

void ModelDataHolder::SetProjectTabName()
{
    m_modelsWidget->setTabText(0, "Overview: " + qApp->instance()->property("projectname").toString());
    emit nameChanged();
}

void ModelDataHolder::NewModel()
{
    QSharedPointer<AbstractModel> t = CreateModel(SupraFit::ScriptModel, m_data);
    if (t->DemandInput()) {
        PrepareWidget* prepareWidget = new PrepareWidget(t->getInputBlock(), true, this);
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
    int model = qobject_cast<MDHDockTitleBar*>(sender())->Model();
    AddModel(model);
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

void ModelDataHolder::AddModel(int model)
{
    QSharedPointer<AbstractModel> t = CreateModel(model, m_data);
    if (!t) {
        emit m_data.toStrongRef()->Message(tr("Tried to add %1 to the workspace.").arg(Model2Name(static_cast<SupraFit::Model>(model))), 1);
        emit m_data.toStrongRef()->Warning("But one does not simply add a model to a data set, where the number of input variables differ.", 1);
        return;
    }
    if (t->DemandInput()) {
        PrepareWidget* prepareWidget = new PrepareWidget(t->getInputBlock(), true, this);
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
    t->setFast(false);

    Charts charts = m_charts->addModel(t);
    QPointer<ModelWidget> modelwidget = new ModelWidget(t, charts, readonly);
    modelwidget->setColorList(object["colors"].toString());
    modelwidget->setKeys(object["keys"].toString());
    connect(modelwidget, &ModelWidget::AddModel, this, [this](const QJsonObject& object) { this->Json2Model(object); });
    connect(modelwidget->getMinimizer().data(), SIGNAL(Message(QString, int)), this, SIGNAL(Message(QString, int)), Qt::DirectConnection);
    connect(modelwidget->getMinimizer().data(), SIGNAL(Warning(QString, int)), this, SIGNAL(MessageBox(QString, int)), Qt::DirectConnection);
    connect(modelwidget, SIGNAL(Warning(QString, int)), this, SIGNAL(MessageBox(QString, int)), Qt::DirectConnection);
    connect(this, SIGNAL(recalculate()), modelwidget, SLOT(recalculate()));

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
    emit ModelAdded();
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
        QPointer<AbstractModel> m = model->Model().data();
        for (int j = 0; j < m_model_widgets.size(); ++j) {
            if (m_model_widgets[j] == model)
                m_model_widgets.remove(j);
        }
        m_modelsWidget->removeTab(i);
        m_models.remove(m_models.indexOf(model->Model()));
        delete model;
#pragma message("if some strange crashes occur, check this here")

        if (qobject_cast<MetaModel*>(m_data))
            qobject_cast<MetaModel*>(m_data)->RemoveModel(m);
        else {
            if (m)
                delete m;
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
    for (int i = m_modelsWidget->count(); i > 0; --i)
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
            //if (!m_model_widgets[i - 1])
            //    continue;
            if (!m_model_widgets[i - 1]->isChecked())
                continue;
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
        m_data.toStrongRef()->IndependentModel()->setEditable(!m_data.toStrongRef()->IndependentModel()->isEditable());
        m_data.toStrongRef()->DependentModel()->setEditable(!m_data.toStrongRef()->DependentModel()->isEditable());
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
            m_data.toStrongRef()->IndependentModel()->setEditable(!m_data.toStrongRef()->IndependentModel()->isEditable());
            m_data.toStrongRef()->DependentModel()->setEditable(!m_data.toStrongRef()->DependentModel()->isEditable());
        }
    }
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

    return compare;
}

#include "modeldataholder.moc"

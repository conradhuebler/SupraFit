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
#include "src/global_config.h"

#include "src/capabilities/modelcomparison.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/capabilities/reductionanalyse.h"
#include "src/capabilities/weakenedgridsearch.h"

#include "src/core/analyse.h"
#include "src/core/jsonhandler.h"
#include "src/core/models.h"

#include "src/ui/dialogs/comparedialog.h"
#include "src/ui/dialogs/importdata.h"
#include "src/ui/dialogs/statisticdialog.h"
#include "src/ui/guitools/instance.h"
#include "src/ui/mainwindow/datawidget.h"
#include "src/ui/mainwindow/metamodelwidget.h"
#include "src/ui/mainwindow/modelwidget.h"

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
    if (colors.size() > 1) {
        QPalette palette = color->palette();
        QLinearGradient gradient(color->rect().topLeft(), color->rect().bottomLeft());
        gradient.setColorAt(0.0, QColor(255, 0, 0, 127));
        gradient.setColorAt(1.0, QColor(0, 0, 255, 127));
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        palette.setBrush(QPalette::Button, QBrush(gradient));
        color->setPalette(palette);
    } else
        color->ChangeColor(QColor(colors.first()));

    QWidget* tools = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(hide);
    layout->addWidget(color);
    QLabel* label = new QLabel("<html>" + modelwidget->Model()->Name() + "</html>");
    layout->addWidget(label);
    tools->setLayout(layout);

    connect(hide, SIGNAL(stateChanged(int)), modelwidget, SIGNAL(ToggleSeries(int)));
    connect(color, SIGNAL(clicked()), modelwidget, SLOT(ChangeColor()));
    connect(modelwidget, SIGNAL(ColorChanged(QColor)), color, SLOT(ChangeColor(QColor)));

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

    m_hide = new QPushButton;
    m_hide->setFlat(true);
    m_hide->setIcon(QIcon::fromTheme("tab-close"));

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(new QLabel("Workspace"));
    layout->addWidget(m_buttons);

    m_edit_data = new QPushButton(tr("Edit Data"));
    m_edit_data->setFlat(true);
    m_edit_data->setIcon(Icon("document-edit"));
    connect(m_edit_data, &QPushButton::clicked, this, &MDHDockTitleBar::EditData);

    m_add_nmr = new QPushButton(tr("Titratrion"));
    m_add_nmr->setFlat(true);
    m_add_nmr->setIcon(Icon("list-add"));

    m_add_itc = new QPushButton(tr("Calorimetry"));
    m_add_itc->setFlat(true);
    m_add_itc->setIcon(Icon("list-add"));

    m_add_kinetics = new QPushButton(tr("Kinetics"));
    m_add_kinetics->setFlat(true);
    m_add_kinetics->setIcon(Icon("list-add"));

    m_optimize = new QPushButton(tr("Optimize All"));
    m_optimize->setFlat(true);
    m_optimize->setIcon(Icon("go-down"));
    connect(m_optimize, &QPushButton::clicked, this, &MDHDockTitleBar::OptimizeAll);

    m_statistics = new QPushButton(tr("Statistics"));
    m_statistics->setFlat(true);
    m_statistics->setIcon(Icon("fork"));
    connect(m_statistics, &QPushButton::clicked, this, &MDHDockTitleBar::ShowStatistics);

    m_analyse = new QPushButton(tr("Analyse and Compare"));
    m_analyse->setFlat(true);
    m_analyse->setIcon(Icon("help-hint"));
    connect(m_analyse, &QPushButton::clicked, this, &MDHDockTitleBar::Compare);

    m_close_all = new QPushButton(tr("Close All"));
    m_close_all->setFlat(true);
    m_close_all->setIcon(Icon("trash-empty"));
    m_close_all->setDisabled(true);
    connect(m_close_all, &QPushButton::clicked, this, &MDHDockTitleBar::CloseAll);


    auto addModel = [this](SupraFit::Model model) -> QAction* {
        QAction* action = new QAction(this);
        action->setText(Model2Name(model));
        action->setData(model);
        connect(action, &QAction::triggered, this, &MDHDockTitleBar::PrepareAddModel);
        return action;
    };

#ifdef NMR_Models
    m_nmr_model << addModel(SupraFit::ItoI);
    m_nmr_model << addModel(SupraFit::IItoI_ItoI);
    m_nmr_model << addModel(SupraFit::ItoI_ItoII);
    m_nmr_model << addModel(SupraFit::IItoI_ItoI_ItoII);
#endif

#ifdef Fluorescence_Models
    m_fl_model << addModel(SupraFit::fl_ItoI);
    m_fl_model << addModel(SupraFit::fl_IItoI_ItoI);
    m_fl_model << addModel(SupraFit::fl_ItoI_ItoII);
    m_fl_model << addModel(SupraFit::fl_ItoI);
#endif

#ifdef Kinetic_Models
    m_kinetcs_model << addModel(SupraFit::Michaelis_Menten);

    m_kinetcs_model << addModel(SupraFit::MonoMolecularModel);
#endif

#ifdef ITC_Models
    m_itc_fixed_model << addModel(SupraFit::itc_ItoI);
    m_itc_fixed_model << addModel(SupraFit::itc_IItoI);
    m_itc_fixed_model << addModel(SupraFit::itc_ItoII);
    m_itc_fixed_model << addModel(SupraFit::itc_IItoII);

    //m_itc_flex_model << addModel(SupraFit::itc_n_ItoI);
    m_itc_flex_model << addModel(SupraFit::itc_n_ItoII);

    m_itc_fixed_model << addModel(SupraFit::itc_blank);
#endif

    m_script_action = new QAction(this);
    m_script_action->setText(tr("Scripted Models"));

#ifdef experimentel
    ParseScriptedModels();
    m_independet_2 << m_script_action;
#endif

    QVBoxLayout* vlayout = new QVBoxLayout;

    QHBoxLayout* buttons = new QHBoxLayout;
    buttons->addWidget(m_add_nmr);
    buttons->addWidget(m_add_itc);
    buttons->addWidget(m_add_kinetics);
    buttons->addWidget(m_edit_data);

    vlayout->addLayout(buttons);

    buttons = new QHBoxLayout;
    buttons->addWidget(m_optimize);
    buttons->addWidget(m_statistics);
    buttons->addWidget(m_analyse);
    buttons->addWidget(m_close_all);

    vlayout->addLayout(buttons);

    m_buttons->setLayout(vlayout);
    layout->addStretch();
    layout->addWidget(m_hide);
    setLayout(layout);
}

void MDHDockTitleBar::addToMenu(int IndependetCount)
{

    auto addMenu = [](const QVector<QPointer<QAction>>& list, QMenu* menu) {
        for (const QPointer<QAction>& ptr : qAsConst(list))
            menu->addAction(ptr);
    };

    QMenu* menu = new QMenu;
    if (IndependetCount == 1) {
        addMenu(m_kinetcs_model, menu);
        m_add_kinetics->setMenu(menu);
        m_add_nmr->hide();
        menu = new QMenu;
        QAction* action = menu->addSection(tr("Fixed Stoichiometry"));
        addMenu(m_itc_fixed_model, menu);
        m_add_itc->setMenu(menu);
        action = menu->addSection(tr("Flexible Stoichiometry"));
        addMenu(m_itc_flex_model, menu);
    } else if (IndependetCount == 2) {
        QAction* action = menu->addSection(tr("NMR/UV VIS"));
        addMenu(m_nmr_model, menu);
        action = menu->addSection(tr("Fluorescence"));
        addMenu(m_fl_model, menu);
        m_add_nmr->setMenu(menu);
        m_add_kinetics->hide();
        m_add_itc->hide();
    }
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
}

void MDHDockTitleBar::PrepareAddModel()
{
    m_last_action = qobject_cast<QAction*>(sender());
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
    connect(m_statistic_dialog, &StatisticDialog::MCStatistic, this, &ModelDataHolder::MCStatistic);
    connect(m_statistic_dialog, &StatisticDialog::WGStatistic, this, &ModelDataHolder::WGStatistic);
    connect(m_statistic_dialog, &StatisticDialog::MoCoStatistic, this, &ModelDataHolder::MoCoStatistic);
    connect(m_statistic_dialog, &StatisticDialog::Reduction, this, &ModelDataHolder::ReductionStatistic);
    connect(m_statistic_dialog, &StatisticDialog::CrossValidation, this, &ModelDataHolder::CVStatistic);

    m_compare_dialog = new CompareDialog(this);
    connect(m_compare_dialog, &CompareDialog::CompareReduction, this, &ModelDataHolder::CompareReduction);
    connect(m_compare_dialog, &CompareDialog::CompareAIC, this, &ModelDataHolder::CompareAIC);

    connect(m_TitleBarWidget, &MDHDockTitleBar::AddModel, this, static_cast<void (ModelDataHolder::*)()>(&ModelDataHolder::AddModel));
    connect(m_TitleBarWidget, &MDHDockTitleBar::ShowStatistics, m_statistic_dialog, &StatisticDialog::show);
    connect(m_TitleBarWidget, &MDHDockTitleBar::OptimizeAll, this, &ModelDataHolder::OptimizeAll);
    connect(m_TitleBarWidget, &MDHDockTitleBar::CloseAll, this, &ModelDataHolder::CloseAll);
    connect(m_TitleBarWidget, &MDHDockTitleBar::EditData, this, &ModelDataHolder::EditData);
    connect(m_TitleBarWidget, &MDHDockTitleBar::Compare, this, [this]() {
        if (this->m_compare_dialog) {
            m_compare_dialog->setCutoff(1.8);
            m_compare_dialog->show();
        }
    });

    layout->addWidget(m_modelsWidget, 1, 0);
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
    m_data.clear();
}

void ModelDataHolder::setData(QSharedPointer<DataClass> data, QSharedPointer<ChartWrapper> wrapper)
{
    m_data = data;
    m_TitleBarWidget->setEnabled(true);
    m_wrapper = wrapper;
    if (!qobject_cast<MetaModel*>(data)) {
        m_datawidget = new DataWidget;
        m_datawidget->setData(m_data, wrapper);
        m_modelsWidget->setDataTab(m_datawidget);
        m_TitleBarWidget->addToMenu(m_data->IndependentModel()->columnCount());
        connect(m_datawidget, SIGNAL(NameChanged()), this, SLOT(SetProjectTabName()));
        connect(m_datawidget, SIGNAL(recalculate()), this, SIGNAL(recalculate()));
    } else {
        m_metamodelwidget = new MetaModelWidget;
        m_metamodelwidget->setMetaModel(qobject_cast<MetaModel*>(data));
        m_modelsWidget->setMetaTab(m_metamodelwidget);
        m_TitleBarWidget->HideModelTools();
    }
}

void ModelDataHolder::SetProjectTabName()
{
    m_modelsWidget->setTabText(0, "Overview: " + qApp->instance()->property("projectname").toString());
    emit nameChanged();
}

void ModelDataHolder::AddModel()
{
    const QAction* action = qobject_cast<MDHDockTitleBar*>(sender())->lastAction();
    AddModel(action->data().toInt());
}

void ModelDataHolder::AddModel(int model)
{
    QSharedPointer<AbstractModel> t = CreateModel(model, m_data);
    t->InitialGuess();
    m_history = false;
    ActiveModel(t);
}

void ModelDataHolder::Json2Model(const QJsonObject& object)
{

    if (object.contains("SupraFit"))
        Json2Model(object, (SupraFit::Model)object["model"].toInt());
    else
        Json2Model(object, Name2Model(object["model"].toString()));
}

void ModelDataHolder::Json2Model(const QJsonObject& object, SupraFit::Model model)
{
#ifdef _DEBUG
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
#endif
    if (object.isEmpty())
        return;
    QSharedPointer<AbstractModel> t = CreateModel(model, m_data);
    t->ImportModel(object);
    ActiveModel(t, object);
#ifdef _DEBUG
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    qDebug() << "model loaded within" << t1 - t0 << " msecs";
#endif
}

void ModelDataHolder::ActiveModel(QSharedPointer<AbstractModel> t, const QJsonObject& object, bool readonly)
{
    t->setFast(false);

    Charts charts = m_charts->addModel(t);
    ModelWidget* modelwidget = new ModelWidget(t, charts, readonly);
    modelwidget->setColorList(object["colors"].toString());
    modelwidget->setKeys(object["keys"].toString());
    t->setOptimizerConfig(m_config);
    connect(modelwidget, &ModelWidget::AddModel, this, [this](const QJsonObject& object) { this->Json2Model(object); });
    connect(modelwidget->getMinimizer().data(), SIGNAL(Message(QString, int)), this, SIGNAL(Message(QString, int)), Qt::DirectConnection);
    connect(modelwidget->getMinimizer().data(), SIGNAL(Warning(QString, int)), this, SIGNAL(MessageBox(QString, int)), Qt::DirectConnection);
    connect(modelwidget, SIGNAL(Warning(QString, int)), this, SIGNAL(MessageBox(QString, int)), Qt::DirectConnection);
    connect(this, SIGNAL(recalculate()), modelwidget, SLOT(recalculate()));

    connect(modelwidget->getMinimizer().data(), SIGNAL(RequestCrashFile()), this, SLOT(CreateCrashFile()), Qt::DirectConnection);
    connect(modelwidget->getMinimizer().data(), SIGNAL(RequestRemoveCrashFile()), this, SLOT(RemoveCrashFile()), Qt::DirectConnection);
    connect(modelwidget->getMinimizer().data(), SIGNAL(InsertModel(QJsonObject, int)), this, SIGNAL(InsertModel(QJsonObject, int)), Qt::DirectConnection);

    connect(modelwidget, SIGNAL(IncrementProgress(int)), m_statistic_dialog, SLOT(IncrementProgress(int)), Qt::DirectConnection);
    connect(m_statistic_dialog, SIGNAL(Interrupt()), modelwidget, SIGNAL(Interrupt()));
    connect(m_statistic_dialog, SIGNAL(Interrupt()), this, SLOT(Interrupt()));

    m_modelsWidget->addModelsTab(modelwidget);
    m_last_tab = m_modelsWidget->currentIndex();
    m_models << t;
    m_model_widgets << modelwidget;

    /*
     * Some models are loaded from history, this should no be added again
     * after not adding them, we allow the next models to be added to history again
     */
    if (m_history)
        modelwidget->getMinimizer()->addToHistory();
    else
        m_history = true;
    ActiveBatch();
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
        AbstractModel* m = model->Model().data();
        m_modelsWidget->removeTab(i);
        m_models.remove(m_models.indexOf(model->Model()));
        delete model;
        if (qobject_cast<MetaModel*>(m_data))
            qobject_cast<MetaModel*>(m_data)->RemoveModel(m);
        emit ModelRemoved();
    }
    ActiveBatch();
}

void ModelDataHolder::setSettings(const OptimizerConfig& config)
{
    for (int i = 0; i < m_modelsWidget->count(); ++i) {
        ModelWidget* w = qobject_cast<ModelWidget*>(m_modelsWidget->widget(i));
        if (w != 0) {
            w->Model()->setOptimizerConfig(config);
            m_config = config;
        }
    }
}

bool ModelDataHolder::CheckCrashFile()
{
    QString filename = qApp->instance()->property("projectpath").toString() + ".crashsave.suprafit";
    return QFile::exists(filename);
}

void ModelDataHolder::CreateCrashFile()
{
    RemoveCrashFile();
    QString filename = qApp->instance()->property("projectpath").toString() + ".crashsave.suprafit";
    for (int i = 0; i < m_models.size(); ++i) {
        if (!m_models[i].isNull()) {
            QJsonObject obj = m_models[i].data()->ExportModel();
            JsonHandler::AppendJsonFile(obj, filename);
        }
    }
}

void ModelDataHolder::RemoveCrashFile()
{
    if (CheckCrashFile()) {
        QString filename = qApp->instance()->property("projectpath").toString() + ".crashsave.suprafit";
        QFile::remove(filename);
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
    data = m_data->ExportData();

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
    data = m_data->ExportData();

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
    m_wrapper.data()->stopAnimiation();
    /*
     * Dont load to many models to the workspace, this is slow and confusing
     */
    int i = m_models.size();
    for (const QString& key : qAsConst(keys)) {
        if (key == "data")
            continue;

        QApplication::processEvents();
        /*
     * We cannot stick with that for now, we have no old models stack
     */
        QJsonObject model = object[key].toObject();
        //if (i++ < 5)
        Json2Model(model);
        //else
        //  emit InsertModel(model);
    }

    setEnabled(true);
    m_wrapper.data()->restartAnimation();
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
    replay = QMessageBox::information(this, tr("Close All."), tr("Do you really want to close all models on the workspace?"), QMessageBox::Yes | QMessageBox::No);
    if (replay == QMessageBox::Yes) {
        CloseAllForced();
    }
}

void ModelDataHolder::CloseAllForced()
{
    for (int i = m_modelsWidget->count(); i > 0; --i)
        RemoveTab(i);
}

int ModelDataHolder::Runs(bool moco) const
{
    int run = 0;
    for (int i = 0; i < m_model_widgets.size(); ++i) {
        if (!m_model_widgets[i])
            continue;
        if (m_statistic_dialog->UseChecked() && !m_model_widgets[i]->isChecked())
            continue;
        if (moco && m_model_widgets[i]->Model()->GlobalParameterSize() != 2)
            continue;
        run++;
    }
    return run;
}

void ModelDataHolder::WGStatistic(const WGSConfig& config)
{
    m_statistic_dialog->setRuns(Runs());
    m_statistic_dialog->setRuns(m_models.size());
    for (int i = 0; i < m_model_widgets.size(); ++i) {
        if (!m_model_widgets[i])
            continue;

        if (m_statistic_dialog->UseChecked() && !m_model_widgets[i]->isChecked())
            continue;

        m_model_widgets[i]->WGStatistic(config);

        if (!m_allow_loop)
            break;
    }
    m_statistic_dialog->HideWidget();
}

void ModelDataHolder::MCStatistic(MCConfig config)
{
    m_statistic_dialog->setRuns(Runs());
    m_allow_loop = true;

    for (int i = 0; i < m_model_widgets.size(); ++i) {
        if (!m_model_widgets[i])
            continue;

        if (m_statistic_dialog->UseChecked() && !m_model_widgets[i]->isChecked())
            continue;

        config.variance = m_model_widgets[i]->Model()->StdDeviation();
        m_model_widgets[i]->MCStatistic(config);

        if (!m_allow_loop)
            break;
    }
    m_statistic_dialog->HideWidget();
}

void ModelDataHolder::ReductionStatistic()
{
    m_statistic_dialog->setRuns(Runs());
    m_allow_loop = true;

    for (int i = 0; i < m_model_widgets.size(); ++i) {
        if (!m_model_widgets[i])
            continue;

        if (m_statistic_dialog->UseChecked() && !m_model_widgets[i]->isChecked())
            continue;

        m_model_widgets[i]->DoReductionAnalyse();

        if (!m_allow_loop)
            break;
    }
    m_statistic_dialog->HideWidget();
}

void ModelDataHolder::CVStatistic(ReductionAnalyse::CVType type)
{
    m_statistic_dialog->setRuns(Runs());
    m_allow_loop = true;

    for (int i = 0; i < m_model_widgets.size(); ++i) {
        if (!m_model_widgets[i])
            continue;

        if (m_statistic_dialog->UseChecked() && !m_model_widgets[i]->isChecked())
            continue;

        m_model_widgets[i]->CVAnalyse(type);

        if (!m_allow_loop)
            break;
    }
    m_statistic_dialog->HideWidget();
}

void ModelDataHolder::MoCoStatistic(MoCoConfig config)
{
    m_allow_loop = true;
    m_statistic_dialog->setRuns(Runs(true));

    config.maxerror = 0;
    for (int i = 0; i < m_model_widgets.size(); ++i) {
        if (!m_model_widgets[i])
            continue;

        if (m_statistic_dialog->UseChecked() && !m_model_widgets[i]->isChecked())
            continue;

        if (m_model_widgets[i]->Model()->GlobalParameterSize() == 2)
            m_model_widgets[i]->MoCoStatistic(config);

        if (!m_allow_loop)
            break;
    }
    m_statistic_dialog->HideWidget();
}

void ModelDataHolder::OptimizeAll()
{
    for (int i = 1; i < m_modelsWidget->count(); i++) {
        if (qobject_cast<ModelWidget*>(m_modelsWidget->widget(i))) {
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

void ModelDataHolder::CompareReduction()
{
    if (!m_compare_dialog)
        return;

    qreal cutoff = m_compare_dialog->CutOff();
    bool local = m_compare_dialog->Local();

    QVector<QPair<QJsonObject, QVector<int>>> models;
    for (int i = 1; i < m_modelsWidget->count(); i++) {
        if (qobject_cast<ModelWidget*>(m_modelsWidget->widget(i))) {
            ModelWidget* modelwidget = qobject_cast<ModelWidget*>(m_modelsWidget->widget(i));
            QJsonObject model = modelwidget->Model()->ExportModel();
            QPair<QJsonObject, QVector<int>> pair;
            pair.first = model;
            QVector<int> parameter;
            if (local)
                for (int i = 0; i < modelwidget->Model()->MaxParameter(); ++i)
                    parameter << i;
            else
                for (int i = 0; i < modelwidget->Model()->GlobalParameterSize(); ++i)
                    parameter << i;

            pair.second = parameter;
            models << pair;
        }
    }

    QString result = StatisticTool::AnalyseReductionAnalysis(models, cutoff);

    QHBoxLayout* layout = new QHBoxLayout;
    QTextEdit* text = new QTextEdit;
    text->setText("<html><pre>" + result + "</pre></html>");
    layout->addWidget(text);
    QDialog dialog(this);
    dialog.setLayout(layout);
    dialog.resize(1024, 800);
    dialog.exec();
}

void ModelDataHolder::CompareAIC()
{
    QVector<QWeakPointer<AbstractModel>> models;
    for (int i = 1; i < m_modelsWidget->count(); i++) {
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

void ModelDataHolder::EditData()
{
    int version = m_data->ExportData()["SupraFit"].toInt();
    if (version < 1602) {
        QMessageBox::information(this, tr("Old SupraFit file"), tr("This is an older SupraFit file, you can only edit the table in Workspace!"));
        m_data->IndependentModel()->setEditable(!m_data->IndependentModel()->isEditable());
        m_data->DependentModel()->setEditable(!m_data->DependentModel()->isEditable());
    } else {
        if (m_data->DataType() == DataClassPrivate::Thermogram) {
            ImportData dialog(m_data);
            if (dialog.exec() == QDialog::Accepted) { // I dont like this either ....
                {
                    if (m_data->DataType() == DataClassPrivate::Thermogram)
                        m_data->ImportData(dialog.getStoredData().ExportData());
                }
            }
        } else {
            m_data->IndependentModel()->setEditable(!m_data->IndependentModel()->isEditable());
            m_data->DependentModel()->setEditable(!m_data->DependentModel()->isEditable());
        }
    }
}

void ModelDataHolder::setCurrentTab(int index)
{
    if (index < m_modelsWidget->count())
        m_modelsWidget->setCurrentIndex(index);
}

#include "modeldataholder.moc"

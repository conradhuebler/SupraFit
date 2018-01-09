/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/global_config.h"

#include "src/capabilities/weakenedgridsearch.h"
#include "src/capabilities/modelcomparison.h"
#include "src/capabilities/montecarlostatistics.h"

#include "src/core/jsonhandler.h"
#include "src/core/models.h"

#include "src/ui/mainwindow/datawidget.h"
#include "src/ui/mainwindow/modelwidget.h"
#include "src/ui/dialogs/statisticdialog.h"

#include <QApplication>

#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include <QtGui/QColor>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QToolButton>

#include "modeldataholder.h"

void ToolButton::ChangeColor(const QColor &color)
{
    setStyleSheet("background-color:" + color.name()+ ";");
}

TabWidget::TabWidget(QWidget *parent) : QTabWidget(parent)
{
    setStyleSheet("QTabBar::tab { height: 20px;}");
}

void TabWidget::addModelsTab(QPointer<ModelWidget> modelwidget)
{
    addTab(modelwidget, modelwidget->Model()->Name());
    
    QCheckBox *hide = new QCheckBox;
    hide->setMaximumSize(20,20);
    hide->setChecked(true);
    hide->setToolTip(tr("Toggle Series in Charts"));
    
    modelwidget->setCheckbox(hide);
    
    ToolButton *color = new ToolButton;
    color->setMaximumSize(15,15);
    QStringList colors = modelwidget->Chart().signal_wrapper->ColorList().split("|");
    colors.removeDuplicates();
    if(colors.size() > 1)
    {
        QPalette palette = color->palette();
        QLinearGradient gradient(color->rect().topLeft(),color->rect().bottomLeft());
        gradient.setColorAt(0.0, QColor(255, 0, 0, 127));
        gradient.setColorAt(1.0, QColor(0, 0, 255, 127));
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        palette.setBrush(QPalette::Button, QBrush(gradient));
        color->setPalette(palette);
    }else
        color->ChangeColor(QColor(colors.first()));
    
    QWidget *tools = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(hide);
    layout->addWidget(color);
    tools->setLayout(layout);
    
    connect(hide, SIGNAL(stateChanged(int)), modelwidget, SIGNAL(ToggleSeries(int)));
    connect(color, SIGNAL(clicked()), modelwidget, SLOT(ChangeColor()));
    connect(modelwidget, SIGNAL(ColorChanged(QColor)), color, SLOT(ChangeColor(QColor)));
    
    setCurrentWidget(modelwidget);
    tabBar()->setTabButton(currentIndex(), QTabBar::LeftSide, tools);
}

void TabWidget::setDataTab(QPointer<DataWidget> datawidget)
{
    addTab(datawidget, "Overview: " + qApp->instance()->property("projectname").toString());
    tabBar()->setTabButton(0, QTabBar::RightSide, 0);
    m_datawidget = datawidget;
}


MDHDockTitleBar::MDHDockTitleBar()
{
    
    m_buttons = new QWidget;
    m_buttons->setEnabled(false);
    
    m_hide = new QPushButton;
    m_hide->setFlat(true);
    m_hide->setIcon(QIcon::fromTheme("tab-close"));
    
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(new QLabel("Workspace"));
    layout->addWidget(m_buttons);
    
    
    m_add_nmr = new QPushButton(tr("Titratrion"));
    m_add_nmr->setFlat(true);
    
    m_add_itc = new QPushButton(tr("Calorimetry"));
    m_add_itc->setFlat(true);
    
    m_add_kinetics = new QPushButton(tr("Kinetics"));
    m_add_kinetics->setFlat(true);
    
    m_optimize = new QPushButton(tr("Optimize All"));
    m_optimize->setFlat(true);
    connect(m_optimize, &QPushButton::clicked, this, &MDHDockTitleBar::OptimizeAll);
    
    m_statistics = new QPushButton(tr("Statistics"));
    m_statistics->setFlat(true);
    connect(m_statistics, &QPushButton::clicked, this, &MDHDockTitleBar::ShowStatistics);
    
    m_close_all = new QPushButton(tr("Close All"));
    m_close_all->setFlat(true);
    m_close_all->setDisabled(true);
    connect(m_close_all, &QPushButton::clicked, this, &MDHDockTitleBar::CloseAll);

#ifdef NMR_Models
    QAction *ItoI_action = new QAction(this);
    ItoI_action->setText(tr("1:1-Model"));
    ItoI_action->setData(SupraFit::ItoI);
    connect(ItoI_action, &QAction::triggered, this, &MDHDockTitleBar::PrepareAddModel);
    m_nmr_model << ItoI_action;
    
    QAction *IItoI_ItoI_action = new QAction(this);
    IItoI_ItoI_action->setText(tr("2:1/1:1-Model"));
    IItoI_ItoI_action->setData(SupraFit::IItoI_ItoI);
    connect(IItoI_ItoI_action, &QAction::triggered, this, &MDHDockTitleBar::PrepareAddModel);
    m_nmr_model << IItoI_ItoI_action;
    
    QAction *ItoI_ItoII_action = new QAction(this);
    ItoI_ItoII_action->setText(tr("1:1/1:2-Model"));
    ItoI_ItoII_action->setData(SupraFit::ItoI_ItoII);
    connect(ItoI_ItoII_action, &QAction::triggered, this, &MDHDockTitleBar::PrepareAddModel);
    m_nmr_model << ItoI_ItoII_action;
    
    QAction *II_I_ItoI_ItoII_action = new QAction(this);
    II_I_ItoI_ItoII_action->setText(tr("2:1/1:1/1:2-Model"));
    II_I_ItoI_ItoII_action->setData(SupraFit::IItoI_ItoI_ItoII);
    connect(II_I_ItoI_ItoII_action, &QAction::triggered, this, &MDHDockTitleBar::PrepareAddModel);
    m_nmr_model << II_I_ItoI_ItoII_action;
#endif
    
#ifdef Fluorescence_Models
    QAction *fl_ItoI_action = new QAction(this);
    fl_ItoI_action->setText(tr("1:1-Model"));
    fl_ItoI_action->setData(SupraFit::fl_ItoI);
    connect(fl_ItoI_action, &QAction::triggered, this, &MDHDockTitleBar::PrepareAddModel);
    m_fl_model << fl_ItoI_action;
    
     QAction *fl_IItoI_ItoI_action = new QAction(this);
    fl_IItoI_ItoI_action->setText(tr("2:1/1:1-Model"));
    fl_IItoI_ItoI_action->setData(SupraFit::fl_IItoI_ItoI);
    connect(fl_IItoI_ItoI_action, &QAction::triggered, this, &MDHDockTitleBar::PrepareAddModel);
    m_fl_model << fl_IItoI_ItoI_action;
    
    QAction *fl_ItoI_ItoII_action = new QAction(this);
    fl_ItoI_ItoII_action->setText(tr("1:1/1:2-Model"));
    fl_ItoI_ItoII_action->setData(SupraFit::fl_ItoI_ItoII);
    connect(fl_ItoI_ItoII_action, &QAction::triggered, this, &MDHDockTitleBar::PrepareAddModel);
    m_fl_model << fl_ItoI_ItoII_action;
    
#endif
    
#ifdef Kinetic_Models
    QAction *mm_action = new QAction(this);
    mm_action->setText(tr("Michaelis Menten"));
    mm_action->setData(SupraFit::Michaelis_Menten);
    connect(mm_action, &QAction::triggered, this, &MDHDockTitleBar::PrepareAddModel);
    m_kinetcs_model << mm_action;
    
    QAction *first_order_action = new QAction(this);
    first_order_action->setText(tr("First Order Kinetics"));
    first_order_action->setData(SupraFit::First_Order_Kinetics);
    connect(first_order_action, &QAction::triggered, this, &MDHDockTitleBar::PrepareAddModel);
    m_kinetcs_model << first_order_action;
#endif
    
#ifdef ITC_Models
    QAction *itc_ItoI_action = new QAction(this);
    itc_ItoI_action->setText(tr("1:1-Model"));
    itc_ItoI_action->setData(SupraFit::itc_ItoI);
    connect(itc_ItoI_action, &QAction::triggered, this, &MDHDockTitleBar::PrepareAddModel);
    m_itc_model << itc_ItoI_action;
#endif
    
    m_script_action = new QAction(this);
    m_script_action->setText(tr("Scripted Models"));
    
#ifdef experimentel
    ParseScriptedModels();
    m_independet_2 << m_script_action;
#endif
    
    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addWidget(m_add_nmr);
    buttons->addWidget(m_add_itc);
    buttons->addWidget(m_add_kinetics);
    buttons->addWidget(m_optimize);
    buttons->addWidget(m_statistics);
    buttons->addWidget(m_close_all);
    
    m_buttons->setLayout(buttons);
    layout->addStretch();
    layout->addWidget(m_hide);
    setLayout(layout);
}

void MDHDockTitleBar::addToMenu(int IndependetCount)
{
    
    auto addMenu = [](const QVector<QPointer<QAction > > &list, QMenu *menu){
        for(const QPointer<QAction > & ptr : qAsConst(list))
            menu->addAction(ptr); 
    };
    
    QMenu *menu = new QMenu;
    if(IndependetCount == 1)
    {
        addMenu(m_kinetcs_model, menu);
        m_add_kinetics->setMenu(menu);
        m_add_nmr->hide();
        m_add_itc->hide();
    }
    else if(IndependetCount == 2)
    {
        QAction *action = menu->addSection(tr("NMR/UV VIS"));
        addMenu(m_nmr_model, menu);
        action = menu->addSection(tr("Fluorescence"));
        addMenu(m_fl_model, menu);
        m_add_nmr->setMenu(menu);
        menu = new QMenu;
        addMenu(m_itc_model, menu);
        m_add_itc->setMenu(menu);
        m_add_kinetics->hide();
    }
}

void MDHDockTitleBar::EnableBatch( bool enabled)
{
    m_close_all->setEnabled(enabled);
    m_statistics->setEnabled(enabled);
    m_optimize->setEnabled(enabled);
}

void MDHDockTitleBar::PrepareAddModel()
{
    m_last_action =  qobject_cast<QAction *>(sender());
    emit AddModel();
}



ModelDataHolder::ModelDataHolder() : m_TitleBarWidget(new MDHDockTitleBar), m_history(true)
{
    QGridLayout *layout = new QGridLayout;
    
    setLayout(layout);
    
    m_datawidget = new DataWidget;
    connect(m_datawidget, SIGNAL(NameChanged()), this, SLOT(SetProjectTabName()));
    connect(m_datawidget, SIGNAL(recalculate()), this, SIGNAL(recalculate()));
    m_modelsWidget = new TabWidget(this);
    m_modelsWidget->setTabsClosable(true);
    m_modelsWidget->setMovable(true);
    connect(m_modelsWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(RemoveTab(int)));
    connect(m_modelsWidget, SIGNAL(currentChanged(int)), this, SLOT(HideSubWindows(int)));
     
    m_statistic_dialog = new StatisticDialog(this);
    connect(m_statistic_dialog, SIGNAL(MCStatistic()), this, SLOT(MCStatistic()));
    connect(m_statistic_dialog, SIGNAL(WGStatistic()), this, SLOT(WGStatistic()));
    connect(m_statistic_dialog, SIGNAL(MoCoStatistic()), this, SLOT(MoCoStatistic()));
    
    connect(m_TitleBarWidget, &MDHDockTitleBar::AddModel, this, static_cast<void(ModelDataHolder::*)()>(&ModelDataHolder::AddModel));
    connect(m_TitleBarWidget, &MDHDockTitleBar::CloseAll, this, &ModelDataHolder::CloseAll);
    connect(m_TitleBarWidget, &MDHDockTitleBar::WGStatistic, this, &ModelDataHolder::WGStatistic);
    connect(m_TitleBarWidget, &MDHDockTitleBar::MCStatistic, this, &ModelDataHolder::MCStatistic);
    connect(m_TitleBarWidget, &MDHDockTitleBar::ShowStatistics, m_statistic_dialog, &StatisticDialog::show);
    connect(m_TitleBarWidget, &MDHDockTitleBar::MoCoStatistic, this, &ModelDataHolder::MoCoStatistic);
    connect(m_TitleBarWidget, &MDHDockTitleBar::OptimizeAll, this, &ModelDataHolder::OptimizeAll);
    
    layout->addWidget(m_modelsWidget, 1, 0);
}

ModelDataHolder::~ModelDataHolder()
{
    for(int i = 0; i < m_modelsWidget->count(); ++i)
    if(qobject_cast<ModelWidget *>(m_modelsWidget->widget(i)))
    {
        ModelWidget *model = qobject_cast<ModelWidget *>(m_modelsWidget->widget(i));
        m_modelsWidget->removeTab(i);
        delete model;
    }
}

void ModelDataHolder::setData(QSharedPointer<DataClass> data, QSharedPointer<ChartWrapper> wrapper)
{
    m_data = data;
    m_TitleBarWidget->setEnabled(true);
    m_datawidget->setData(m_data, wrapper);
    m_modelsWidget->setDataTab(m_datawidget); 
    m_TitleBarWidget->addToMenu(m_data->IndependentModel()->columnCount());
}


void ModelDataHolder::SetProjectTabName()
{
    m_modelsWidget->setTabText(0, "Overview: " + qApp->instance()->property("projectname").toString());
    emit nameChanged();
}

void ModelDataHolder::AddModel()
{
    const QAction *action = qobject_cast<MDHDockTitleBar*>(sender())->lastAction();
    AddModel(action->data().toInt());
}

void ModelDataHolder::AddModel(int model)
{
    QSharedPointer<AbstractModel > t;
    
    switch(model){
        case 1:
            t =  QSharedPointer<ItoI_Model>(new ItoI_Model(m_data.data()), &QObject::deleteLater);
            break;
        case 2:
            t = QSharedPointer<IItoI_ItoI_Model>(new IItoI_ItoI_Model(m_data.data()), &QObject::deleteLater);
            break;
        case 3:
            t = QSharedPointer<ItoI_ItoII_Model>(new ItoI_ItoII_Model(m_data.data()),  &QObject::deleteLater);
            break;
        case 4:
            t = QSharedPointer<IItoI_ItoI_ItoII_Model>(new IItoI_ItoI_ItoII_Model(m_data.data()),  &QObject::deleteLater);
            break;
        case 5:
            t = QSharedPointer<Michaelis_Menten_Model>(new Michaelis_Menten_Model(m_data.data()),  &QObject::deleteLater);
            break;
        case 6:
            t = QSharedPointer<Kinetic_First_Order_Model>(new Kinetic_First_Order_Model(m_data.data()),  &QObject::deleteLater);
            break;
        case SupraFit::itc_ItoI:
            t = QSharedPointer<itc_ItoI_Model>(new itc_ItoI_Model(m_data.data()),  &QObject::deleteLater);
            break;
        case SupraFit::fl_ItoI:
            t =  QSharedPointer<fl_ItoI_Model>(new fl_ItoI_Model(m_data.data()), &QObject::deleteLater);
            break;
        case SupraFit::fl_ItoI_ItoII:
            t =  QSharedPointer<fl_ItoI_ItoII_Model>(new fl_ItoI_ItoII_Model(m_data.data()), &QObject::deleteLater);
            break;
        case SupraFit::fl_IItoI_ItoI:
             t = QSharedPointer<fl_IItoI_ItoI_Model>(new fl_IItoI_ItoI_Model(m_data.data()), &QObject::deleteLater);
            break;
        default:
            t.clear();
            return; 
            
    };
    m_history = false;
    ActiveModel(t);
}

void ModelDataHolder::AddModel(const QJsonObject &json)
{
#ifdef experimental
    QSharedPointer<AbstractModel > t =  QSharedPointer<ScriptModel>(new ScriptModel(m_data.data(), json), &QObject::deleteLater);
    m_history = false;
    ActiveModel(t);
#endif
}

void ModelDataHolder::AddModelScript()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        QString str = action->data().toString();
        
        if(!str.isEmpty())
        {
            QJsonObject object;
            if(JsonHandler::ReadJsonFile(object, str))
            {
                AddModel(object);
            }
            else
                qDebug() << "loading failed";
        }
    }
}


void ModelDataHolder::ParseScriptedModels()
{
#warning reintroduce sometimes
    /*
    QMenu *menu = new QMenu;
    QDirIterator it(":/data/models/", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString name = it.next();
        
        QAction *sub_model = new QAction(this);
        sub_model->setData(name);
        sub_model->setText(name.remove(".json").remove(":/data/models/"));
        connect(sub_model, SIGNAL(triggered()), this, SLOT(AddModelScript()));
        menu->addAction(sub_model);
    }
    m_script_action->setMenu(menu);
    */
}

void ModelDataHolder::Json2Model(const QJsonObject &object, const QString &str)
{
    
    QSharedPointer<AbstractModel > t;
    
    /*
     * WARNING and FIXME I dont like this!
     */
    if(str == "1:1-Model")
    {
        t = QSharedPointer<ItoI_Model>(new ItoI_Model(m_data.data()), &QObject::deleteLater);
    }
    else if(str == "2:1/1:1-Model")
    {
        t = QSharedPointer<IItoI_ItoI_Model>(new IItoI_ItoI_Model(m_data.data()), &QObject::deleteLater);
    }
    else if(str == "1:1/1:2-Model"){
        t =  QSharedPointer<ItoI_ItoII_Model>(new ItoI_ItoII_Model(m_data.data()),  &QObject::deleteLater);
    }
    else if(str == "2:1/1:1/1:2-Model"){
        t = QSharedPointer<IItoI_ItoI_ItoII_Model>(new IItoI_ItoI_ItoII_Model(m_data.data()),  &QObject::deleteLater);
    }
    else if(str == "itc_1:1-Model")
        t = QSharedPointer<itc_ItoI_Model>(new itc_ItoI_Model(m_data.data()),  &QObject::deleteLater);
    else if(str == "fl_1:1-Model")   
        t =  QSharedPointer<fl_ItoI_Model>(new fl_ItoI_Model(m_data.data()), &QObject::deleteLater);
    else
    {
        t.clear();
        return; 
    }
    t->ImportModel(object);
    ActiveModel(t, object);
}

void ModelDataHolder::ActiveModel(QSharedPointer<AbstractModel> t, const QJsonObject &object)
{
    Charts charts = m_charts->addModel(t); 
    ModelWidget *modelwidget = new ModelWidget(t, charts);
    
    if(!object.isEmpty())
    {
        charts.signal_wrapper->setColorList(object["colors"].toString());
        charts.error_wrapper->setColorList(object["colors"].toString());
        charts.data_wrapper->setColorList(object["colors"].toString());
    }
    
    t->setOptimizerConfig(m_config);
    connect(modelwidget, SIGNAL(AddModel(const QJsonObject)), this, SLOT(AddToWorkspace(const QJsonObject)));
    connect(modelwidget->getMinimizer().data(), SIGNAL(Message(QString, int)), this, SIGNAL(Message(QString, int)), Qt::DirectConnection);
    connect(modelwidget->getMinimizer().data(), SIGNAL(Warning(QString, int)), this, SIGNAL(MessageBox(QString, int)), Qt::DirectConnection);
    connect(modelwidget, SIGNAL(Warning(QString, int)), this, SIGNAL(MessageBox(QString, int)), Qt::DirectConnection);
    connect(this, SIGNAL(recalculate()), modelwidget, SLOT(recalulate()));
    
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
    if(m_history)
        modelwidget->getMinimizer()->addToHistory();
    else
        m_history = true;
    ActiveBatch();
}

void ModelDataHolder::ActiveBatch()
{
    m_TitleBarWidget->EnableBatch(m_modelsWidget->count() > 2);

}

void ModelDataHolder::RemoveTab(int i)
{
    if(qobject_cast<ModelWidget *>(m_modelsWidget->widget(i)))
    {
        ModelWidget *model = qobject_cast<ModelWidget *>(m_modelsWidget->widget(i));
        m_modelsWidget->removeTab(i);
        delete model;
    }
    ActiveBatch();
}

void ModelDataHolder::setSettings(const OptimizerConfig &config)
{
    for(int i = 0; i < m_modelsWidget->count(); ++i)
    {
        ModelWidget *w = qobject_cast<ModelWidget *>(m_modelsWidget->widget(i));
        if(w != 0)
        {
            w->Model()->setOptimizerConfig(config);
            m_config = config;
        }
    }
}

bool ModelDataHolder::CheckCrashFile()
{
    QString filename = qApp->instance()->property("projectpath").toString() + ".crashsave.json";
    return QFile::exists(filename);
}

void ModelDataHolder::CreateCrashFile()
{
    RemoveCrashFile();
    QString filename = qApp->instance()->property("projectpath").toString() + ".crashsave.json";
    for(int i = 0; i < m_models.size(); ++i)
    {
        if(!m_models[i].isNull())
        {
            QJsonObject obj = m_models[i].data()->ExportModel();
            JsonHandler::AppendJsonFile(obj, filename);        
        }
    }   
}

void ModelDataHolder::RemoveCrashFile()
{
    if(CheckCrashFile())
    {
        QString filename = qApp->instance()->property("projectpath").toString() + ".crashsave.json";
        QFile::remove(filename);
    }
}

void ModelDataHolder::SaveCurrentModels(const QString &file)
{
    QJsonObject toplevel;
    for(int i = 1; i < m_modelsWidget->count(); i++)
    {
        if(qobject_cast<ModelWidget *>(m_modelsWidget->widget(i)))
        {
            ModelWidget *model = qobject_cast<ModelWidget *>(m_modelsWidget->widget(i));
            QJsonObject obj = model->Model()->ExportModel();
            obj["colors"] = model->Chart().signal_wrapper->ColorList();
            toplevel["model_" + QString::number(i)] = obj; 
        }
    }    
    JsonHandler::WriteJsonFile(toplevel, file);   
}

void ModelDataHolder::SaveWorkspace(const QString &file)
{
    QJsonObject toplevel, data;
    data = m_data->ExportData();
        
    for(int i = 1; i < m_modelsWidget->count(); i++)
    {
        if(qobject_cast<ModelWidget *>(m_modelsWidget->widget(i)))
        {
            ModelWidget *model = qobject_cast<ModelWidget *>(m_modelsWidget->widget(i));
            QJsonObject obj = model->Model()->ExportModel();
            obj["colors"] = model->Chart().signal_wrapper->ColorList();
            data["colors"] = model->Chart().data_wrapper->ColorList();
            toplevel["model_" + QString::number(i)] = obj; 
        }
    } 
    toplevel["data"] = data;
    JsonHandler::WriteJsonFile(toplevel, file);
}

void ModelDataHolder::AddToWorkspace(const QJsonObject &object)
{
    Waiter wait;
    QStringList keys = object.keys();
    
    /*
     * If the json contains only one model, then we have probely only "data" and "model" as keys
     * and we can load them directly
     * else we iter through all keys which may be model_x keys containing "data" and "models"
     */
    if(keys.contains("data") && keys.contains("model"))
    {
        // we don't allow this model to be added to addToHistory
        m_history = false;
        Json2Model(object, object["model"].toString());
    }
    else
    {
        int i = m_models.size();
        for(const QString &str : qAsConst(keys))
        {
            /*
             * Dont load to many models to the workspace, this is slow and confusing
             */
            QJsonObject model = object[str].toObject();
            if(i++ < 5)
                Json2Model(model, model["model"].toString());
            else
                emit InsertModel(model);
            QApplication::processEvents();
        }
    }
}

void ModelDataHolder::LoadCurrentProject(const QJsonObject& object)
{
    QStringList keys = object.keys();
    if(keys.contains("data") && keys.contains("model"))
    {
        if(m_modelsWidget->currentIndex() == 0)
        {
            if(m_modelsWidget->count() < 2)
                return;
            m_modelsWidget->setCurrentIndex(1);
        }
        QMessageBox::StandardButton replay;
        replay = QMessageBox::information(this, tr("Override Model."), tr("Do you want to override the current loaded model [Y]\nor just add to workspace [N]?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if(replay == QMessageBox::Yes) 
        {
            ModelWidget *model = qobject_cast<ModelWidget *>(m_modelsWidget->currentWidget());
            model->LoadJson(object);
        }
        else if(replay == QMessageBox::No)
        {
            AddToWorkspace(object);
        }
    }
}

void ModelDataHolder::CloseAll()
{
    QMessageBox::StandardButton replay;
    QString app_name = QString(qApp->instance()->applicationName());
    replay = QMessageBox::information(this, tr("Close All."), tr("Do you really want to close all models on the workspace?"), QMessageBox::Yes | QMessageBox::No);
    if(replay == QMessageBox::Yes) 
    {
        for(int i = m_modelsWidget->count(); i > 0; --i)
            RemoveTab(i);
    }
}

int ModelDataHolder::Runs(bool moco) const
{
    int run = 0;
    for(int i = 0; i < m_model_widgets.size(); ++i)
    {
        if(!m_model_widgets[i])
            continue;
        if(m_statistic_dialog->UseChecked() && !m_model_widgets[i]->isChecked())
            continue;
        if(moco && m_model_widgets[i]->Model()->GlobalParameterSize() != 2)
            continue;
        run++;
    }
    return run;
}

void ModelDataHolder::WGStatistic()
{ 
    m_statistic_dialog->setRuns(Runs());
    m_statistic_dialog->setRuns(m_models.size());
    WGSConfig config = m_statistic_dialog->getWGSConfig();
    for(int i = 0; i < m_model_widgets.size(); ++i)
    {
        if(!m_model_widgets[i])
            continue;
            
        if(m_statistic_dialog->UseChecked() && !m_model_widgets[i]->isChecked())
            continue;
        
        m_model_widgets[i]->WGStatistic(config);

        if(!m_allow_loop)
            break;
    }
    m_statistic_dialog->HideWidget();
}

void ModelDataHolder::MCStatistic()
{
    m_statistic_dialog->setRuns(Runs());
    MCConfig config = m_statistic_dialog->getMCConfig();
    m_allow_loop = true;
    
    for(int i = 0; i < m_model_widgets.size(); ++i)
    {
        if(!m_model_widgets[i])
            continue;
            
        if(m_statistic_dialog->UseChecked() && !m_model_widgets[i]->isChecked())
            continue;
        
        config.variance = m_model_widgets[i]->Model()->StdDeviation();
        m_model_widgets[i]->MCStatistic(config);
        
        if(!m_allow_loop)
            break;
    }
    m_statistic_dialog->HideWidget();
}

void ModelDataHolder::MoCoStatistic()
{
    m_allow_loop = true;
    m_statistic_dialog->setRuns(Runs(true));
    
    MoCoConfig config = m_statistic_dialog->getMoCoConfig();
    config.maxerror = 0;
     for(int i = 0; i < m_model_widgets.size(); ++i)
    {
        if(!m_model_widgets[i])
            continue;
        
        if(m_statistic_dialog->UseChecked() && !m_model_widgets[i]->isChecked())
            continue;
        
        if(m_model_widgets[i]->Model()->GlobalParameterSize() == 2)
            m_model_widgets[i]->MoCoStatistic(config);
        
        if(!m_allow_loop)
            break;
    }
    m_statistic_dialog->HideWidget();
}


void ModelDataHolder::OptimizeAll()
{
    for(int i = 1; i < m_modelsWidget->count(); i++)
    {
        if(qobject_cast<ModelWidget *>(m_modelsWidget->widget(i)))
        {
            ModelWidget *model = qobject_cast<ModelWidget *>(m_modelsWidget->widget(i));
            model->GlobalMinimize();
        }
    } 
}

void ModelDataHolder::HideSubWindows(int index)
{
    if(qobject_cast<ModelWidget *>(m_modelsWidget->widget(m_last_tab)))
    {
        ModelWidget *model = qobject_cast<ModelWidget *>(m_modelsWidget->widget(m_last_tab));
        model->HideAllWindows();
        m_last_tab = index;
    }
}
#include "modeldataholder.moc"

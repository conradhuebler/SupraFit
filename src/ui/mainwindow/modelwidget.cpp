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

#include "src/global.h"
#include "src/version.h"

#include "src/capabilities/reductionanalyse.h"
#include "src/capabilities/weakenedgridsearch.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/capabilities/modelcomparison.h"

#include "src/core/toolset.h"
#include "src/core/dataclass.h"
#include "src/core/jsonhandler.h"
#include "src/core/AbstractModel.h"
#include "src/core/minimizer.h"

#include "src/ui/widgets/buttons/spinbox.h"
#include "src/ui/dialogs/advancedsearch.h"
#include "src/ui/dialogs/configdialog.h"
#include "src/ui/dialogs/modeldialog.h"
#include "src/ui/dialogs/statisticdialog.h"
#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/widgets/chartview.h"
#include "src/ui/widgets/3dchartview.h"
#include "src/ui/widgets/optimizerflagwidget.h"
#include "src/ui/widgets/stackedwidget.h"
#include "src/ui/widgets/statisticwidget.h"
#include "src/ui/widgets/modelelement.h"
#include "src/ui/widgets/modelactions.h"
#include "src/ui/widgets/results/cvresultwidget.h"
#include "src/ui/widgets/results/wgsresultswidget.h"
#include "src/ui/widgets/results/mcresultswidget.h"
#include "src/ui/widgets/results/searchresultwidget.h"
#include "src/ui/widgets/optionswidget.h"
#include "src/ui/mainwindow/chartwidget.h"

#include <QtMath>
#include "cmath"
#include <QApplication>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QTimer>

#include <QtWidgets/QAction>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableView>

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QXYSeries>

#include <QtDataVisualization>

#include <QDebug>

#include <iostream>
#include <random>

#include "modelwidget.h"


ModelWidget::ModelWidget(QSharedPointer<AbstractModel > model,  Charts charts, QWidget *parent ) : QWidget(parent), m_model(model), m_charts(charts), m_pending(false), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(true, this), &QObject::deleteLater)), m_statistic(false)
{
    m_model_widget = new QWidget;
    Data2Text();
    m_minimizer->setModel(m_model);
    m_advancedsearch = new AdvancedSearch(this);
    m_advancedsearch->setModel(m_model);
    
    m_statistic_dialog = new StatisticDialog(m_model, this);
    connect(m_statistic_dialog, SIGNAL(MCStatistic()), this, SLOT(MCStatistic()));
    connect(m_statistic_dialog, SIGNAL(WGStatistic()), this, SLOT(WGStatistic()));
    connect(m_statistic_dialog, SIGNAL(MoCoStatistic()), this, SLOT(MoCoStatistic()));
    connect(m_statistic_dialog, SIGNAL(CrossValidation()), this, SLOT(CVAnalyse()));
    
    connect(m_advancedsearch, SIGNAL(PlotFinished(int)), this, SLOT(PlotFinished(int)));
    connect(m_advancedsearch, SIGNAL(MultiScanFinished()), this, SLOT(MultiScanFinished()));
    
    connect(this, SIGNAL(ToggleSeries(int)), m_charts.error_wrapper, SLOT(SetBlocked(int)));
    connect(this, SIGNAL(ToggleSeries(int)), m_charts.signal_wrapper, SLOT(SetBlocked(int)));
    
    m_search_result = new ModalDialog;
    m_search_result->setWindowTitle("Charts for " + m_model->Name());
    
    m_statistic_result = new ModalDialog;
    m_statistic_result->setWindowTitle("Statistics for " + m_model->Name());
    
    m_statistic_widget = new StatisticWidget(m_model, this),
    m_table_result= new ModalDialog;
    m_table_result->setWindowTitle("Search Results " + m_model->Name());
    
    m_concentrations_result = new ModalDialog;
    m_concentrations_result->setWindowTitle("Concentration Table for " + m_model->Name());
    
    m_layout = new QGridLayout;
    QLabel *pure_shift = new QLabel(tr("Constants:"));
    QHBoxLayout *const_layout = new QHBoxLayout;
    const_layout->addWidget(pure_shift, 0, 0);
    for(int i = 0; i < m_model->GlobalParameterSize(); ++i)
    {
        QPointer<SpinBox >constant = new SpinBox;
        m_constants << constant;
        constant->setDecimals(4);

        constant->setPrefix(m_model->GlobalParameterPrefix());
        constant->setSingleStep(m_model->GlobalParameter(i)/100);

        constant->setValue(m_model->GlobalParameter()[i]);
        constant->setMaximum(1e4);
        constant->setMaximumWidth(150);
        connect(constant, SIGNAL(valueChangedNotBySet(double)), this, SLOT(recalulate()));
        const_layout->addWidget(new QLabel(m_model->GlobalParameterName(i)));
        const_layout->addWidget(constant);
    }
    m_bc_50 = new QLabel(tr("BC50_0"));
    const_layout->addWidget(m_bc_50);
    const_layout->addStretch(100);
    
    m_minimize_all = new QPushButton(tr("Fit"));
    
    QAction *minimize_normal = new QAction(tr("Tight"));
    connect(minimize_normal, SIGNAL(triggered()), this, SLOT(GlobalMinimize()));
    
    QAction *minimize_loose = new QAction(tr("Loose"));
    connect(minimize_loose, SIGNAL(triggered()), this, SLOT(GlobalMinimizeLoose()));
    
    QAction *fast_conf = new QAction(tr("Confidence"));
    connect(fast_conf, SIGNAL(triggered()), this, SLOT(FastConfidence()));
    
    QAction *reduct = new QAction(tr("Reduction Analyse"));
    connect(reduct, SIGNAL(triggered()), this, SLOT(DoReductionAnalyse()));
    
    QMenu *menu = new QMenu;
    menu->addAction(minimize_normal);
    menu->addAction(minimize_loose);
    menu->addAction(fast_conf);
    menu->addAction(reduct);
    menu->setDefaultAction(minimize_normal);
    m_minimize_all->setMenu(menu);
    
    const_layout->addWidget(m_minimize_all);
    m_layout->addLayout(const_layout, 0, 0, 1, m_model->GlobalParameterSize()+3);
    
    m_model_options_widget = new OptionsWidget(m_model);
    if(m_model->getAllOptions().size())
        m_layout->addWidget(m_model_options_widget, 1, 0, 1, m_model->GlobalParameterSize()+3);
    
    m_sign_layout = new QVBoxLayout;
    
    m_sign_layout->setAlignment(Qt::AlignTop); 
    m_converged_label = new QLabel;
    m_sign_layout->addWidget(m_converged_label);
    
    if(m_model->LocalParameterSize())
    {
        for(int i = 0; i < m_charts.signal_wrapper->SeriesSize(); ++i)
        {
            ModelElement *el = new ModelElement(m_model, m_charts, i);
            connect(el, SIGNAL(ValueChanged()), this, SLOT(recalulate()));
            connect(el, SIGNAL(ActiveSignalChanged()), this, SLOT(CollectActiveSignals()));
            connect(this, SIGNAL(Update()), el, SLOT(Update()));
            connect(this, SIGNAL(ToggleSeries(int)), el, SLOT(ToggleSeries(int)));
            m_sign_layout->addWidget(el);
            m_model_elements << el;
        }
    }
    QWidget *scroll = new QWidget;
    scroll->setLayout(m_sign_layout);
    QScrollArea *area = new QScrollArea;
    area->setWidgetResizable(true);
    area->setWidget(scroll);
    
    m_layout->addWidget(area, 3,0,1,m_model->GlobalParameterSize()+3);
    
    if(m_model->Type() == 1)
        DiscreteUI();
    else if(m_model->Type() == 3)
        EmptyUI();
    
    resizeButtons();
    m_model_widget->setLayout(m_layout);
    m_splitter = new QSplitter(this);
    m_splitter->setOrientation(Qt::Vertical);
    m_splitter->addWidget(m_model_widget);
    m_splitter->addWidget(m_statistic_widget);
    connect(m_splitter, SIGNAL(splitterMoved(int,int)), this, SLOT(SplitterResized()));
    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->addWidget(m_splitter);
    setLayout(vlayout);
    QSettings settings;
    settings.beginGroup("model");
    m_splitter->restoreState(settings.value("splitterSizes").toByteArray());
    settings.endGroup();
    settings.beginGroup("minimizer");
    m_optim_flags->setFlags(settings.value("flags", 11).toInt());
    settings.endGroup();
    m_model->Calculate();
    LoadStatistics();
    QTimer::singleShot(1, this, SLOT(Repaint()));;
}

ModelWidget::~ModelWidget()
{
    delete m_charts.signal_wrapper;
    delete m_charts.error_wrapper;
    
    m_model.clear();
    if(_3dchart)
        delete _3dchart;
    
    m_statistic_dialog->hide();
    m_search_result->hide();
    m_concentrations_result->hide();
    m_statistic_result->hide();
    
    delete m_statistic_result;
    delete m_search_result;
    delete m_table_result;
    delete m_concentrations_result;
    delete m_statistic_dialog;
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
    ModelActions *actions = new ModelActions;
    
    connect(actions, SIGNAL(NewGuess()), this, SLOT(NewGuess()));
    connect(actions, SIGNAL(LocalMinimize()), this, SLOT(LocalMinimize()));
    connect(actions, SIGNAL(OptimizerSettings()), this, SLOT(OptimizerSettings()));
    connect(actions, SIGNAL(ImportConstants()), this, SLOT(ImportConstants()));
    connect(actions, SIGNAL(ExportConstants()), this, SLOT(ExportConstants()));
    connect(actions, SIGNAL(OpenAdvancedSearch()), this, SLOT(OpenAdvancedSearch()));
    connect(actions, SIGNAL(TogglePlot3D()), this, SLOT(TogglePlot3D()));
    connect(actions, SIGNAL(TogglePlot()), this, SLOT(TogglePlot()));
    connect(actions, SIGNAL(ToggleStatisticDialog()), this, SLOT(ToggleStatisticDialog()));
    connect(actions, SIGNAL(Save2File()), this, SLOT(Save2File()));
    connect(actions, SIGNAL(ToggleConcentrations()), this, SLOT(ToggleConcentrations()));
    connect(actions, SIGNAL(ToggleSearch()), this, SLOT(ToggleSearchTable()));
    connect(actions, SIGNAL(ExportSimModel()), this, SLOT(ExportSimModel()));
    
    m_layout->addWidget(actions, 4, 0,1,m_model->GlobalParameterSize()+3);  
    m_optim_flags = new OptimizerFlagWidget(m_model->LastOptimzationRun());
    m_layout->addWidget(m_optim_flags, 5, 0,1,m_model->GlobalParameterSize()+3);  
   
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
    QList<qreal > constants = m_model->GlobalParameter();
    for(int j = 0; j < constants.size(); ++j)
        m_constants[j]->setValue(constants[j]);
    emit Update();
}

void ModelWidget::Repaint()
{
    
    if(m_model->Type() == 3)
        return;
    m_pending = true;
    setParameter();
    qreal error = 0;
    for(int j = 0; j < m_model_elements.size(); ++j)
    {
        error += m_model->SumOfErrors(j);
        m_model_elements[j]->Update();
    }
    m_pending = false;
    m_minimize_all->setEnabled(true);

    qreal bc50 = 0;
    if(qobject_cast<AbstractTitrationModel *>(m_model))
        bc50 = qobject_cast<AbstractTitrationModel *>(m_model)->BC50()*1E6;
    
    QString format_text = tr("BC50<sub>0</sub>: %1").arg(bc50);
    QChar mu = QChar(956);
    format_text += QString(" [") + mu + QString("M]");
    if(bc50 > 0)
        m_bc_50->setText(format_text);
    else
        m_bc_50->clear();
    
    QString converged;
    if(!m_model->isConverged())
        converged = "<font color =\'red\'>Calculation did not converge.</font>\n";
    else
        converged = "Calculation converged";
    m_converged_label->setText(converged);
    
    Model2Text();
    QTextDocument doc;
    doc.setHtml(m_statistic_widget->Overview());
    m_logging += "\n\n" +  doc.toPlainText();
}



void ModelWidget::recalulate()
{
    if(m_pending)
        return;
    m_pending = true;
    CollectParameters();
    m_model->Calculate();
    QTimer::singleShot(1, this, SLOT(Repaint()));
    QTimer::singleShot(1, m_statistic_widget, SLOT(Update()));
    m_pending = false;
}

void ModelWidget::CollectParameters()
{
    QList<qreal > pure_signals, constants;
    QVector<QList <qreal > > complex_signals;
    complex_signals.resize(m_model->GlobalParameterSize());
    QList<int > active_signals;
    for(int i = 0; i < m_model_elements.size(); ++i)
    {
        active_signals <<  m_model_elements[i]->Include();
        m_model->setLocalParameterSeries(m_model_elements[i]->D(), i);

    }

    for(int i = 0; i < m_model->GlobalParameterSize(); ++i)
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
    if(m_pending)
        return; 
    m_pending = true;
    
    CollectParameters();
    QJsonObject json = m_model->ExportModel();
    m_minimizer->setParameter(json);
    
    m_model->setOptimizerConfig(config);
    int result;
    result = m_minimizer->Minimize(m_optim_flags->getFlags());
    
    json = m_minimizer->Parameter();
    m_model->ImportModel(json);
    Repaint();
    m_model->OptimizeParameters(m_optim_flags->getFlags());
    if(qApp->instance()->property("auto_confidence").toBool())
        FastConfidence();
    
    QSettings settings;
    settings.beginGroup("minimizer");
    settings.setValue("flags", m_optim_flags->getFlags());
    settings.endGroup();
    
    if(!result)
        emit Warning(tr("The optimization did not converge within the cycles! Rerun optimisation or increase number of steps."), 1);
    
    m_statistic = false;
    m_pending = false; 
}

void ModelWidget::ToggleStatisticDialog()
{
    m_statistic_dialog->show();
}

void ModelWidget::ToggleSearchTable()
{
    m_table_result->show();
}


void ModelWidget::MCStatistic()
{
    MCConfig config = m_statistic_dialog->getMCConfig();
    MCStatistic(config);
}

void ModelWidget::MCStatistic(MCConfig config)
{
    Waiter wait;

    config.optimizer_config = m_model->getOptimizerConfig();
    config.runtype = m_optim_flags->getFlags();
    
    QPointer<MonteCarloStatistics > monte_carlo = new MonteCarloStatistics(config, this);
    connect(m_statistic_dialog, SIGNAL(Interrupt()), monte_carlo, SLOT(Interrupt()), Qt::DirectConnection);
    connect(this, SIGNAL(Interrupt()), monte_carlo, SLOT(Interrupt()), Qt::DirectConnection);
    connect(monte_carlo, SIGNAL(IncrementProgress(int)), m_statistic_dialog, SLOT(IncrementProgress(int)), Qt::DirectConnection);
    connect(monte_carlo, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)), Qt::DirectConnection);
    monte_carlo->setModel(m_model);
    monte_carlo->Evaluate();
    
    QList<QJsonObject> result = monte_carlo->Results();
    QList<QJsonObject > models = monte_carlo->Models(); 
    
    delete monte_carlo;
    
    MCResultsWidget *mcsresult = new MCResultsWidget(result, m_model, models);
    mcsresult->setModels(models);
   
    QString buff = m_statistic_widget->Statistic();
    buff.remove("<tr>");
    buff.remove("<table>");
    buff.remove("</tr>");
    buff.remove("</table>");
    buff.replace("</td>", "\t");
    buff.replace("<td>", "\t");
    
    QTextDocument doc;
    doc.setHtml(buff);
    
    m_logging += "\n\n" +  doc.toPlainText();
    m_statistic_result->setWidget(mcsresult, "Monte Carlo Simulation for " + m_model->Name());
    m_statistic_result->show();  
    m_statistic_dialog->HideWidget();
}

void ModelWidget::FastConfidence()
{
    MoCoConfig config;
    
    qreal f_value = m_model.data()->finv(qApp->instance()->property("p_value").toDouble()/100);
    qreal error = m_model.data()->SumofSquares();
    config.maxerror = error*(f_value*m_model.data()->Parameter()/(m_model.data()->Points()-m_model.data()->Parameter()) +1);
    config.optimizer_config = m_model->getOptimizerConfig();
    config.runtype = m_optim_flags->getFlags();
    config.fisher_statistic = true;
    config.confidence = qApp->instance()->property("p_value").toDouble();
    ModelComparison *statistic = new ModelComparison(config, this);
    QJsonObject json = m_model->ExportModel(false);
    statistic->setModel(m_model);    
    statistic->FastConfidence();
    QList<QJsonObject > constant_results = statistic->Results();
    for(int i = 0; i < constant_results.size(); ++i)
    {
        m_model->setMoCoStatistic(constant_results[i], i);
    }
    delete statistic;
}

void ModelWidget::CVAnalyse()
{
    Waiter wait;
    ReductionAnalyse *analyse = new ReductionAnalyse(m_model->getOptimizerConfig(), m_optim_flags->getFlags());
    analyse->setModel(m_model);
    analyse->CrossValidation(ReductionAnalyse::LeaveOnOut);
    CVResultsWidget *cvresults = new CVResultsWidget(analyse, m_model, m_statistic_result);
    m_statistic_result->setWidget(cvresults, "Cross Validation Analyse" + m_model->Name());
    m_statistic_result->show();
//     qDebug() << analyse->ModelData();
    emit AddModel(analyse->ModelData());
}

void ModelWidget::DoReductionAnalyse()
{
    Waiter wait;
    ReductionAnalyse *analyse = new ReductionAnalyse(m_model->getOptimizerConfig(), m_optim_flags->getFlags());
    analyse->setModel(m_model);
    analyse->PlainReduction();
    QtCharts::QChart *chart = new QtCharts::QChart;
    
    chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    QPointer<ChartView > view = new ChartView(chart);
    QList<QList< QPointF> > series = analyse->Series();
    for(int i = 0; i < series.size(); ++i)
    {
        LineSeries *serie = new LineSeries;
        serie->append(series[i]);
        serie->setName( m_model->GlobalParameterName(i));
        view->addSeries(serie, true);
    }
    m_statistic_result->setWidget(view, "Reduction Analyse");
    m_statistic_result->show();
    delete analyse;
}

void ModelWidget::WGStatistic()
{
    WGSConfig config = m_statistic_dialog->getWGSConfig();
    WGStatistic(config);
}

void ModelWidget::WGStatistic(WGSConfig config)
{
    Waiter wait;
    
    config.optimizer_config = m_model->getOptimizerConfig();
    config.runtype = m_optim_flags->getFlags();
    
     if(config.maxerror < 1E-8)
        config.maxerror = m_model->Error(config.confidence, config.fisher_statistic);
     
    WeakenedGridSearch *statistic = new WeakenedGridSearch(config, this);
    
    connect(m_statistic_dialog, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(this, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), m_statistic_dialog, SLOT(IncrementProgress(int)), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)), Qt::DirectConnection);
    
    QJsonObject json = m_model->ExportModel(false);
    statistic->setModel(m_model);
    statistic->setParameter(json);
    
    if(!statistic->ConfidenceAssesment())
    {
        emit Warning("The optimization seems not to be converged with respect to at least one constants!\nShowing the results anyway.", 1);
    }
    QList<QJsonObject> data = statistic->Results();
    WGSResultsWidget *resultwidget = new WGSResultsWidget(data, m_model, false, m_statistic_result);
    m_statistic_result->setWidget(resultwidget, "Weakened Grid Search for " + m_model->Name());
    m_statistic_result->show();  
    emit IncrementProgress(1);
    m_statistic_dialog->HideWidget();
}

void ModelWidget::MoCoStatistic()
{
    MoCoConfig config = m_statistic_dialog->getMoCoConfig();
    MoCoStatistic(config);
}

void ModelWidget::MoCoStatistic(MoCoConfig config)
{
    Waiter wait;
 
    config.optimizer_config = m_model->getOptimizerConfig();
    config.runtype = m_optim_flags->getFlags();
    
    if(config.maxerror < 1E-8)
        config.maxerror = m_model->Error(config.confidence, config.fisher_statistic);

    ModelComparison *statistic = new ModelComparison(config, this);
    
    connect(m_statistic_dialog, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(this, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), m_statistic_dialog, SLOT(IncrementProgress(int)), Qt::DirectConnection);
    connect(statistic, SIGNAL(setMaximumSteps(int)), m_statistic_dialog, SIGNAL(setMaximumSteps(int)), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)), Qt::DirectConnection);
    
    QJsonObject json = m_model->ExportModel(false);
    statistic->setModel(m_model);
    bool result = statistic->Confidence();
    QList<QJsonObject> data = statistic->Results();
    if(result)
    {
        WGSResultsWidget *resultwidget = new WGSResultsWidget(data, m_model, true, m_statistic_result);
        m_statistic_result->setWidget(resultwidget, "Model Comparison for " + m_model->Name());
        m_statistic_result->show();
    }else
        QMessageBox::information(this, tr("Not done"), tr("No calculation where done, because there is only one parameter of interest."));
    m_statistic_dialog->HideWidget();
}

void ModelWidget::LoadStatistics()
{
    /* We load the MC statistcs from model
     */
    
    QList<QJsonObject> result;

    for(int i = 0; i < m_model->getMCStatisticResult(); ++i)
    {
         result << m_model->getMCStatisticResult(i);
         QApplication::processEvents();
    }
    
    if(result.size())
    {
        MCResultsWidget *mcsresult = new MCResultsWidget(result, m_model);
        m_statistic_result->setWidget(mcsresult, "Monte Carlo Simulation for " + m_model->Name());
    }
    result.clear();
    
    for(int i = 0; i < m_model->getWGStatisticResult(); ++i)
    {
         result << m_model->getWGStatisticResult(i);
         QApplication::processEvents();
    }
    
    if(result.size())
    {
        WGSResultsWidget *mcsresult = new WGSResultsWidget(result, m_model, false, m_statistic_result);
        m_statistic_result->setWidget(mcsresult, "Weakend Grid Search " + m_model->Name());
    }
    result.clear();
    
    for(int i = 0; i < m_model->getMoCoStatisticResult(); ++i)
    {
         if(m_model->getMoCoStatisticResult(i)["method"] == "model comparison")
            result << m_model->getMoCoStatisticResult(i);
         QApplication::processEvents();
    }

    if(result.size())
    {
        WGSResultsWidget *mcsresult = new WGSResultsWidget(result, m_model, true, m_statistic_result);
        m_statistic_result->setWidget(mcsresult, "Model Comparison " + m_model->Name());
    }
}


void ModelWidget::LocalMinimize()
{
    
    if(m_pending)
        return;
    Waiter wait;
    CollectParameters();
    m_local_fits.clear();
    int result = 0;
    for(int i = 0; i < m_model->SeriesCount(); ++i)
    {
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        
        QSharedPointer<AbstractModel > model = m_model->Clone();
        QJsonObject parameter = m_model->ExportModel(false);
        model->ImportModel(parameter, false);

        QList<int > active_signals = QVector<int>(m_model_elements.size(), 0).toList();
        active_signals[i] = 1;
        model->setActiveSignals(active_signals);
        OptimizerConfig config = model->getOptimizerConfig();
        model->setOptimizerConfig(config);
        m_minimizer->setModel(model);
        
        result += m_minimizer->Minimize(m_optim_flags->getFlags());
        
        QJsonObject json = m_minimizer->Parameter();
        m_local_fits << json;
        
        QSettings settings;
        settings.beginGroup("minimizer");
        settings.setValue("flags", m_optim_flags->getFlags());
        settings.endGroup();
    }  
    
    if(result < m_model->SeriesCount())
        emit Warning(tr("The optimization did not converge within the cycles! Rerun optimisation or increase number of steps."), 1);
    
    m_minimizer->setModel(m_model);
    m_statistic = false;
    m_pending = false; 
}


QList<int> ModelWidget::ActiveSignals()
{
    QList<int > active_signals;
    for(int i = 0; i < m_model_elements.size(); ++i)
        active_signals << m_model_elements[i]->Include();
    return active_signals;
}

void ModelWidget::CollectActiveSignals()
{
    QList<int > active_signals = ActiveSignals();
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
    QList<qreal > constants = m_model->GlobalParameter();
    for(int j = 0; j < constants.size(); ++j)
        m_constants[j]->setValue(constants[j]);
    emit Update();
}

void ModelWidget::OptimizerSettings()
{
    OptimizerDialog dialog(m_model->getOptimizerConfig(), this);
    if(dialog.exec() == QDialog::Accepted)
    {
        m_model->setOptimizerConfig(dialog.Config());
    }
}
void ModelWidget::ExportConstants()
{
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("Json File (*.json);;Binary (*.jdat);;All files (*.*)" ));
    if(!str.isEmpty())
    {
        setLastDir(str);
        QJsonObject gameObject = m_model->ExportModel();
        JsonHandler::WriteJsonFile(gameObject, str);
    }
    
}

void ModelWidget::ImportConstants()
{
    QString str = QFileDialog::getOpenFileName(this, tr("Open File"), getDir(), tr("Json File (*.json);;Binary (*.jdat);;All files (*.*)" ));
    if(!str.isEmpty())
    {
        setLastDir(str);
        QJsonObject object;
        if(JsonHandler::ReadJsonFile(object, str))
            LoadJson(object);
        else
            qDebug() << "loading failed";
    }
}


void ModelWidget::LoadJson(const QJsonObject& object)
{
    m_model->ImportModel(object);

    QList<qreal > constants = m_model->GlobalParameter();
    for(int j = 0; j < constants.size(); ++j)
        m_constants[j]->setValue(constants[j]);
    Repaint();
    m_optim_flags->setFlags(m_model->LastOptimzationRun());
}

void ModelWidget::OpenAdvancedSearch()
{
    if(!m_advancedsearch.isNull())
        m_advancedsearch->show();    
}


void ModelWidget::ExportSimModel()
{
    bool ok;
    qreal scatter = QInputDialog::getDouble(this, tr("Set Standard Deviation"), tr("Set Standard Deviation for scatter"), m_model->StdDeviation(), 0, 2147483647, 4, &ok);
    if(ok)
    {
        quint64 seed = QDateTime::currentMSecsSinceEpoch();
        std::mt19937 rng;
        rng.seed(seed);
        std::normal_distribution<double> Phi = std::normal_distribution<double>(0,scatter);
        DataTable *model_table = m_model->ModelTable()->PrepareMC(Phi, rng);
        QStringList model = model_table->ExportAsStringList();
        QStringList concentrations = m_model->IndependentModel()->ExportAsStringList();
        
        QString first = concentrations.first();
        QStringList host = first.split("\t");
        /*
         * Add pure shifts only, when concentration table don't provide them
         */
        if(host[1].toDouble() != 0 )
        {
            model.prepend(ToolSet::DoubleList2String(m_model->getLocalParameterColumn(0).toList(), QString("\t")));
            concentrations.prepend(QString( host[0] + "\t" + QString("0")));
        }
                
        if(model.size() != concentrations.size())
        {
            QMessageBox::warning(this, tr("Strange"), tr("Tables don't fit, sorry!"));
            return;
        }
        
        QString filename = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("All files (*.*)" ));
        if(!filename.isEmpty())
        {
            setLastDir(filename);
            QFile file( filename );
            if ( file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text ) )
            {
                QTextStream stream( &file );
                for(int i = 0; i < model.size(); ++i)
                    stream << concentrations[i] << "\t" << model[i] << endl;
            }
        } 
    }
}

void ModelWidget::TogglePlot()
{
    m_statistic_result->show();
}


void ModelWidget::TogglePlot3D()
{
    if(_3dchart)
        _3dchart->setVisible(_3dchart->isVisible());
}


void ModelWidget::PlotFinished(int runtype)
{
    if(runtype == 1)
    {
        
        if(!_3dchart)
        {
            _3dchart = new _3DChartView;
            m_search_result->setWidget(_3dchart, "3D Plot");
        }
        else
            _3dchart = qobject_cast<_3DChartView *>(m_search_result->Widget());
        
        _3dchart->setMaxZ(m_advancedsearch->MaxError());
        _3dchart->setMaxX(m_advancedsearch->MaxX());
        _3dchart->setMinX(m_advancedsearch->MinX());
        _3dchart->setMaxY(m_advancedsearch->MaxY());
        _3dchart->setMinY(m_advancedsearch->MinY());      
        _3dchart->setData(m_advancedsearch->dataArray());
        
//         m_plot_3d->setEnabled(true);
    }
    else if(runtype == 2)
    {
        QList<QList<QPointF> > series = m_advancedsearch->Series();
        QtCharts::QChart *chart = new QtCharts::QChart;
        chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
        ChartView *view = new ChartView(chart);
        for(int i = 0; i < series.size(); ++i)
        {
            QtCharts::QLineSeries *xy_series = new QtCharts::QLineSeries(this);
            xy_series->append(series[i]);
            view->addSeries(xy_series);
        }
        m_search_result->setWidget(view, "Simple Plot");
        
    }
    m_search_result->show();
}


void ModelWidget::MultiScanFinished()
{
    try{
    SearchResultWidget *table = new SearchResultWidget(m_advancedsearch->globalSearch(), m_model, this);
    connect(table, SIGNAL(LoadModel(const QJsonObject)), this, SLOT(LoadJson(const QJsonObject)));
    connect(table, SIGNAL(AddModel(const QJsonObject)), this, SIGNAL(AddModel(const QJsonObject)));
    
    m_table_result->setWidget(table, "Scan Results");

    m_advancedsearch->hide();
    m_table_result->show();
    }
    
    catch (int val)
    {
        if(val == 1)
            qDebug() << "model empty, should not happen at all.";
    }
}

void ModelWidget::ToggleConcentrations()
{
#warning remove me 
    if(qobject_cast<AbstractTitrationModel *>(m_model))
    {
    QTableView *table = new QTableView;
    table->setModel(qobject_cast<AbstractTitrationModel *>(m_model)->getConcentrations());
    m_concentrations_result->setWidget(table);
    m_concentrations_result->show();
    }
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
    text += "SupraFit has been compilied on " +  QString::fromStdString(__DATE__) + " at " +QString::fromStdString( __TIME__) + "\n";
    text += "Git Branch used was " + git_branch+ " - Commit Hash: " + git_commit_hash + " at "+ git_date + ".\n";
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
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("All files (*.*)" ));
    if(!str.isEmpty())
    {
        setLastDir(str);
        QFile file( str );
        if ( file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text ) )
        {
            QTextStream stream( &file );
            stream << m_logging << endl;
        }
    } 
}

void ModelWidget::ChangeColor()
{
    QColor color = QColorDialog::getColor(tr("Choose Color for Series"));
    if(!color.isValid())
        return;
    
    for(int i = 0; i < m_model_elements.size(); ++i)
    {
        m_model_elements[i]->ChangeColor(color);
    }
    emit ColorChanged(color);
}
#include "modelwidget.moc"

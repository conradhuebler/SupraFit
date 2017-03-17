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

#include "src/capabilities/continuousvariation.h"
#include "src/capabilities/montecarlostatistics.h"

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
#include "src/ui/widgets/statisticwidget.h"
#include "src/ui/widgets/modelelement.h"
#include "src/ui/widgets/modelactions.h"
#include "src/ui/widgets/modeltablewidget.h"
#include "src/ui/mainwindow/chartwidget.h"

#include <QtMath>
#include "cmath"
#include <QApplication>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QTimer>

#include <QtWidgets/QAction>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>

#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMenu>
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


ModelWidget::ModelWidget(QSharedPointer<AbstractTitrationModel > model,  Charts charts, QWidget *parent ) : QWidget(parent), m_model(model), m_charts(charts), m_pending(false), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(this), &QObject::deleteLater)), m_statistic(false)
{
    Data2Text();
    m_minimizer->setModel(m_model);
    m_advancedsearch = new AdvancedSearch(this);
    m_advancedsearch->setModel(m_model);
    
    m_statistic_dialog = new StatisticDialog(m_model, this);
    connect(m_statistic_dialog, SIGNAL(MCStatistic()), this, SLOT(MCStatistic()));
    connect(m_statistic_dialog, SIGNAL(CVStatistic()), this, SLOT(CVStatistic()));
    connect(m_statistic_dialog, SIGNAL(MoCoStatistic()), this, SLOT(MoCoStatistic()));
    
    connect(m_advancedsearch, SIGNAL(PlotFinished(int)), this, SLOT(PlotFinished(int)));
    connect(m_advancedsearch, SIGNAL(MultiScanFinished(int)), this, SLOT(MultiScanFinished(int)));
    m_search_result = new ModalDialog;
    m_statistic_result = new ModalDialog;
    m_statistic_widget = new StatisticWidget(m_model, this),
    m_table_result= new ModalDialog;
    m_concentrations_result = new ModalDialog;
    
    m_layout = new QGridLayout;
    QLabel *pure_shift = new QLabel(tr("Constants:"));
    QHBoxLayout *const_layout = new QHBoxLayout;
    const_layout->addWidget(pure_shift, 0, 0);
    for(int i = 0; i < m_model->ConstantSize(); ++i)
    {
        QPointer<SpinBox >constant = new SpinBox;
        m_constants << constant;
        constant->setSingleStep(1e-2);
        constant->setDecimals(4);
        constant->setPrefix(m_model->ConstantNames()[i] + "=10^");
        constant->setValue(m_model->Constants()[i]);
        constant->setMaximumWidth(150);
        connect(constant, SIGNAL(valueChangedNotBySet(double)), this, SLOT(recalulate()));
        const_layout->addWidget(constant);
    }
    m_bc_50 = new QLabel(tr("BC50_0"));
    const_layout->addWidget(m_bc_50);
    const_layout->addStretch(100);
    m_minimize_all = new QPushButton(tr("Fit"));
    
    connect(m_minimize_all, SIGNAL(clicked()), this, SLOT(GlobalMinimize()));
    const_layout->addWidget(m_minimize_all);
    m_layout->addLayout(const_layout, 0, 0, 1, m_model->ConstantSize()+3);
    m_sign_layout = new QVBoxLayout;
    
    m_sign_layout->setAlignment(Qt::AlignTop);
    
    
    for(int i = 0; i < m_model->SignalCount(); ++i)
    {
        ModelElement *el = new ModelElement(m_model, m_charts, i);
        connect(el, SIGNAL(ValueChanged()), this, SLOT(recalulate()));
        connect(el, SIGNAL(ActiveSignalChanged()), this, SLOT(CollectActiveSignals()));
        connect(this, SIGNAL(Update()), el, SLOT(Update()));
        connect(this, SIGNAL(ToggleSeries(int)), el, SLOT(ToggleSeries(int)));
        m_sign_layout->addWidget(el);
        m_model_elements << el;
    }
    
    m_layout->addLayout(m_sign_layout,2,0,1,m_model->ConstantSize()+3);
    

    if(m_model->Type() == 1)
        DiscreteUI();
    else if(m_model->Type() == 3)
        EmptyUI();
    
    m_layout->addWidget(m_statistic_widget, 7, 0, 1, m_model->ConstantSize()+3);
    resizeButtons();
    setLayout(m_layout);
    m_model->Calculate();
    QTimer::singleShot(1, this, SLOT(Repaint()));;
}

ModelWidget::~ModelWidget()
{
    delete m_charts.signal_wrapper;
    delete m_charts.error_wrapper;
    
    m_model.clear();
    if(_3dchart)
        delete _3dchart;
    
    delete m_statistic_result;
    delete m_search_result;
    delete m_table_result;
    delete m_concentrations_result;
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
    
    m_layout->addWidget(actions, 3, 0,1,m_model->ConstantSize()+3);  
    m_optim_flags = new OptimizerFlagWidget(m_model->LastOptimzationRun());
    m_layout->addWidget(m_optim_flags, 4, 0,1,m_model->ConstantSize()+3);  
   
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
    QList<qreal > constants = m_model->Constants();
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

    qreal bc50 = m_model->BC50()*1E6;
    QString format_text = tr("BC50<sub>0</sub>: %1").arg(bc50);
    QChar mu = QChar(956);
    format_text += QString(" [") + mu + QString("M]");
    m_bc_50->setText(format_text);
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
    QTimer::singleShot(1, this, SLOT(Repaint()));;
    m_pending = false;
}

void ModelWidget::CollectParameters()
{
    QList<qreal > pure_signals, constants;
    QVector<QList <qreal > > complex_signals;
    complex_signals.resize(m_model->ConstantSize());
    QList<int > active_signals;
    for(int i = 0; i < m_model_elements.size(); ++i)
    {
        pure_signals << m_model_elements[i]->D0();
        active_signals <<  m_model_elements[i]->Include();
        for(int j = 0; j < m_model_elements[i]->D().size(); ++j)
        {
            complex_signals[j] << m_model_elements[i]->D()[j];
        }
    }
    for(int j = 0; j < m_model->ConstantSize(); ++j)
        m_model->setComplexSignals(complex_signals[j], j);
    for(int i = 0; i < m_model->ConstantSize(); ++i)
        constants << m_constants[i]->value();
    m_model->setActiveSignals(active_signals);
    m_model->setConstants(constants);
    m_model->setPureSignals(pure_signals);
}


void ModelWidget::GlobalMinimize()
{
    Waiter wait;
    if(m_pending)
        return;
    
    m_pending = true;
    CollectParameters();
    QJsonObject json = m_model->ExportJSON();
    m_minimizer->setParameter(json);
    OptimizerConfig config = m_model->getOptimizerConfig();
    
    m_model->setOptimizerConfig(config);
    int result;
    result = m_minimizer->Minimize(m_optim_flags->getFlags());
    if(result == 1)
    {
        json = m_minimizer->Parameter();
        m_model->ImportJSON(json);
        m_model->Calculate();
        Repaint();
        m_model->setLastOptimzationRun(m_optim_flags->getFlags());
        if(qApp->instance()->property("auto_confidence").toBool())
            FastConfidence();
    }

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
    
    QList<QList<QPointF > >series = monte_carlo->getSeries();
    QList<QJsonObject > constant_results = monte_carlo->getResult();
    QWidget *resultwidget = new QWidget;
    QGridLayout *layout = new QGridLayout;
    resultwidget->setLayout(layout);
    
    QtCharts::QChart *chart = new QtCharts::QChart;
    
    chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    ChartView *view = new ChartView(chart);
    layout->addWidget(view, 0, 0, 1, 7);
    bool formated = false;
    for(int i = 0; i < constant_results.size(); ++i)
        m_model->setMCStatistic(constant_results[i], i);
    for(int i = 0; i < series.size(); ++i)
    {
        QtCharts::QLineSeries *xy_series = new QtCharts::QLineSeries(this);
        xy_series->append(series[i]);
        view->addSeries(xy_series);
        if(!formated)
            view->formatAxis();
        formated = true;
        
        
        QtCharts::QLineSeries *current_constant= new QtCharts::QLineSeries();
        *current_constant << QPointF(m_model->Constant(i), 0) << QPointF(m_model->Constant(i), view->YMax());
        current_constant->setColor(xy_series->color());
        view->addSeries(current_constant);
        QtCharts::QLineSeries *series1 = new QtCharts::QLineSeries();
        QtCharts::QLineSeries *series2 = new QtCharts::QLineSeries();
        
        QJsonObject confidence = constant_results[i]["confidence"].toObject();
        *series1 << QPointF(confidence["lower_5"].toVariant().toDouble(), 0) << QPointF(confidence["lower_5"].toVariant().toDouble(), view->YMax());
        *series2 << QPointF(confidence["upper_5"].toVariant().toDouble(), 0) << QPointF(confidence["upper_5"].toVariant().toDouble(), view->YMax());
        QtCharts::QAreaSeries *area_series = new QtCharts::QAreaSeries(series1, series2);
        QPen pen(0x059605);
        pen.setWidth(3);
        area_series->setPen(pen);
        
        QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
        gradient.setColorAt(0.0, xy_series->color());
        gradient.setColorAt(1.0, 0x26f626);
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        area_series->setBrush(gradient);
        area_series->setOpacity(0.4);
        view->addSeries(area_series);
        
        QString text;
        if(i == 0)
        {
            text += "MC Steps: " + QString::number(constant_results[i]["controller"].toObject()["steps"].toInt()) + "\t";
            if(constant_results[i]["controller"].toObject()["bootstrap"].toBool())
                text += "Bootstrapped ";
            else
                text += "Variance = " + QString::number(constant_results[i]["controller"].toObject()["variance"].toDouble()) + " ";
            
            if(constant_results[i]["controller"].toObject()["original"].toBool())
                text += "operated on original data\n";
            else
                text += "operated on modelled data\n";
        }
        text  += m_statistic_widget->TextFromConfidence(constant_results[i]) + "\n";
        QLabel *label = new QLabel(text);
        label->setTextFormat(Qt::RichText);
        layout->addWidget(label, i + 1, 0);
 
    }
    // FIXME all that stuff will be cleaned up some times ...
    
    if(m_model->ConstantSize() == 2)
    {
        QList<QPointF > data = ToolSet::fromModelsList(monte_carlo->Models());
        QWidget *resultwidget_ellipsoid = new QWidget;
        QGridLayout *layout_ellipsoid = new QGridLayout;
        resultwidget_ellipsoid->setLayout(layout_ellipsoid);
        QtCharts::QChart *chart_ellipsoid = new QtCharts::QChart;
        chart_ellipsoid->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
        ChartView *view = new ChartView(chart_ellipsoid);
        layout_ellipsoid->addWidget(view, 0, 0, 1, 7);
        QtCharts::QScatterSeries *xy_series = new QtCharts::QScatterSeries(this);
        xy_series->append(data);
        xy_series->setMarkerSize(8);
        view->addSeries(xy_series);
        m_statistic_result->setWidget(resultwidget_ellipsoid, "Monte Carlo Simulation for " + m_model->Name());
    }
    
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
    m_statistic_result->setWidget(resultwidget, "Monte Carlo Simulation for " + m_model->Name());
    m_statistic_result->show();  
    delete monte_carlo;
}

void ModelWidget::FastConfidence()
{
    CVConfig config;
    config.relax = false;
    config.increment = qApp->instance()->property("fast_increment").toDouble();
    qreal error = m_model.data()->SumofSquares();
    config.maxerror = error+error*0.05;
    config.optimizer_config = m_model->getOptimizerConfig();
    config.runtype = m_optim_flags->getFlags();
    ContinuousVariation *statistic = new ContinuousVariation(config, this);
    QJsonObject json = m_model->ExportJSON(false);
    statistic->setModel(m_model);
    statistic->setParameter(json);
    
    statistic->FastConfidence();
    QList<QJsonObject > constant_results = statistic->Results();
    for(int i = 0; i < constant_results.size(); ++i)
    {
        m_model->setMoCoStatistic(constant_results[i], i);
    }
}

void ModelWidget::CVStatistic()
{
    CVConfig config = m_statistic_dialog->getCVConfig();
    CVStatistic(config);
}

void ModelWidget::CVStatistic(CVConfig config)
{
    Waiter wait;
    
    config.optimizer_config = m_model->getOptimizerConfig();
    config.runtype = m_optim_flags->getFlags();
    ContinuousVariation *statistic = new ContinuousVariation(config, this);
    
    connect(m_statistic_dialog, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(this, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), m_statistic_dialog, SLOT(IncrementProgress(int)), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)), Qt::DirectConnection);

    QJsonObject json = m_model->ExportJSON(false);
    statistic->setModel(m_model);
    statistic->setParameter(json);
    
    if(!statistic->ConfidenceAssesment())
    {
        emit Warning("The optimization seems not to be converged with respect to at least one constants!\nShowing the results anyway.", 1);
    }
    
    QList<QJsonObject > constant_results = statistic->Results();
    QList<QList<QPointF > >series = statistic->Series();
    QWidget *resultwidget = new QWidget;
    QGridLayout *layout = new QGridLayout;
    resultwidget->setLayout(layout);
    
    QtCharts::QChart *chart = new QtCharts::QChart;
    chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    ChartView *view = new ChartView(chart);
    layout->addWidget(view, 0, 0, 1, 7);
    for(int i = 0; i < constant_results.size(); ++i)
    {
        QtCharts::QLineSeries *xy_series = new QtCharts::QLineSeries(this);
        xy_series->append(series[i]);
        view->addSeries(xy_series);
        m_model->setCVStatistic(constant_results[i], i);
        
        QtCharts::QLineSeries *current_constant= new QtCharts::QLineSeries();
        *current_constant << QPointF(m_model->Constant(i), m_model->SumofSquares()) << QPointF(m_model->Constant(i), m_model->SumofSquares()*1.1);
        current_constant->setColor(xy_series->color());
        view->addSeries(current_constant);
        
        QString text;
        if(i == 0)
        {
            text += "Maxsteps: " + QString::number(constant_results[i]["controller"].toObject()["steps"].toInt()) + "\t";
            text += "Increment = " + QString::number(constant_results[i]["controller"].toObject()["increment"].toDouble()) + "\t";
            text += "Max Error = " + QString::number(constant_results[i]["controller"].toObject()["maxerror"].toDouble()) + "\n";
        }
        text  += m_statistic_widget->TextFromConfidence(constant_results[i]) + "\n";
        QLabel *label = new QLabel(text);
        label->setTextFormat(Qt::RichText);
        layout->addWidget(label, i + 1, 0);
    }
    m_statistic_result->setWidget(resultwidget, "Continuous Variation for " + m_model->Name());
    m_statistic_result->show();
    
    delete statistic;
}

void ModelWidget::MoCoStatistic()
{
    CVConfig config = m_statistic_dialog->getMoCoConfig();
    MoCoStatistic(config);
}

void ModelWidget::MoCoStatistic(CVConfig config)
{
    Waiter wait;
    
    config.optimizer_config = m_model->getOptimizerConfig();
    config.runtype = m_optim_flags->getFlags();
    ContinuousVariation *statistic = new ContinuousVariation(config, this);
    
    connect(m_statistic_dialog, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(this, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), m_statistic_dialog, SLOT(IncrementProgress(int)), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)), Qt::DirectConnection);

    QJsonObject json = m_model->ExportJSON(false);
    statistic->setModel(m_model);
    statistic->setParameter(json);
    statistic->EllipsoideConfidence();
     
    QList<QJsonObject > constant_results = statistic->Results();
    QList<QList<QPointF > >series = statistic->Series();
    QWidget *resultwidget = new QWidget;
    QGridLayout *layout = new QGridLayout;
    resultwidget->setLayout(layout);
    
    QtCharts::QChart *chart = new QtCharts::QChart;
    chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    ChartView *view = new ChartView(chart);
    layout->addWidget(view, 0, 0, 1, 7);
    QtCharts::QScatterSeries *xy_series = new QtCharts::QScatterSeries(this);
    xy_series->append(ToolSet::fromModelsList(statistic->Models()));
    xy_series->setMarkerSize(8);
    view->addSeries(xy_series);
    m_statistic_result->setWidget(resultwidget, "Continuous Variation for " + m_model->Name());
    m_statistic_result->show();
    
    delete statistic;
}


void ModelWidget::LocalMinimize()
{
    
    if(m_pending)
        return;
    Waiter wait;
    CollectParameters();
    m_local_fits.clear();
    for(int i = 0; i < m_model->SignalCount(); ++i)
    {
        
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        QSharedPointer<AbstractTitrationModel > model = m_model->Clone();
        QList<int > active_signals = QVector<int>(m_model_elements.size(), 0).toList();
        active_signals[i] = 1;
        model->setActiveSignals(active_signals);
        QVector<int > v(10,0);
        OptimizerConfig config = model->getOptimizerConfig();
        model->setOptimizerConfig(config);
        m_minimizer->setModel(model);
        int result;
        model->ActiveSignals();
        result = m_minimizer->Minimize(m_optim_flags->getFlags());
        
        if(result == 1)
        {
            QJsonObject json = m_minimizer->Parameter();
            m_local_fits << json;
        }
    }  
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
    QList<qreal > constants = m_model->Constants();
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
        QJsonObject gameObject = m_model->ExportJSON();
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
    m_model->ImportJSON(object);
    m_model->Calculate();
    QList<qreal > constants = m_model->Constants();
    for(int j = 0; j < constants.size(); ++j)
        m_constants[j]->setValue(constants[j]);
    Repaint();
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
        QStringList concentrations = m_model->ConcentrationModel()->ExportAsStringList();
        
        QString first = concentrations.first();
        QStringList host = first.split("\t");
        /*
         * Add pure shifts only, when concentration table don't provide them
         */
        if(host[1].toDouble() != 0 )
        {
            model.prepend(ToolSet::DoubleList2String(m_model->PureParameter(), QString("\t")));
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


void ModelWidget::MultiScanFinished(int runtype)
{
    ModelTableWidget *table = new ModelTableWidget;
    connect(table, SIGNAL(LoadModel(const QJsonObject)), this, SLOT(LoadJson(const QJsonObject)));
    connect(table, SIGNAL(AddModel(const QJsonObject)), this, SIGNAL(AddModel(const QJsonObject)));
    table->setModel(m_model);
    table->setInputList(m_advancedsearch->FullList());
    table->setModelList(m_advancedsearch->ModelList());
    m_table_result->setWidget(table, "Scan Results");

    if(m_model->ConstantSize() == 0)
    {
        QList<QPointF > data = ToolSet::fromModelsList(m_advancedsearch->ModelList());
        QWidget *resultwidget_ellipsoid = new QWidget;
        QGridLayout *layout_ellipsoid = new QGridLayout;
        resultwidget_ellipsoid->setLayout(layout_ellipsoid);
        QtCharts::QChart *chart_ellipsoid = new QtCharts::QChart;
        chart_ellipsoid->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
        ChartView *view = new ChartView(chart_ellipsoid);
        layout_ellipsoid->addWidget(view, 0, 0, 1, 7);
        QtCharts::QScatterSeries *xy_series = new QtCharts::QScatterSeries(this);
        xy_series->append(data);
        xy_series->setMarkerSize(8);
        view->addSeries(xy_series);
        m_table_result->setWidget(resultwidget_ellipsoid, "Scattering " + m_model->Name());
    }

    m_advancedsearch->hide();
    m_table_result->show();
}

void ModelWidget::ToggleConcentrations()
{
    QTableView *table = new QTableView;
    table->setModel(m_model->getConcentrations());
    m_concentrations_result->setWidget(table);
    m_concentrations_result->show();
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
    text += "Git Branch used was " + git_branch+ " - Commit Hash: " + git_commit_hash + "as tagged as "+ git_tag + ".\n";
    text += "******************************************************************************************************\n";
    text += "\n";
    text += "#### Begin of Data Description ####\n";
    text += "Concentrations :   " + QString::number(m_model->DataPoints())  + "\n";
    for(int i = 0; i < m_model->ConcentrationModel()->columnCount(); ++i)
        text += m_model->ConcentrationModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    text += "\n";
    text += m_model->ConcentrationModel()->ExportAsString();
    text += "\n";
    text += "Signals :          " + QString::number(m_model->SignalCount()) + "\n";
    for(int i = 0; i < m_model->SignalModel()->columnCount(); ++i)
        text += m_model->SignalModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    text += "\n";
    text += m_model->SignalModel()->ExportAsString();
    text += "\n";
    text += "#### End of Data Description #####\n";
    text += "******************************************************************************************************\n";
    m_logging += text;
}

void ModelWidget::Model2Text()
{
    QString text;
    text += "\n";
    text += "******************************************************************************************************\n";
    text += "#### Current Model Results #####\n";
    text += "Equilibrium Model Calculation with complexation constants:\n";
    for(int i = 0; i < m_model->ConstantSize(); ++i)
        text += m_model->ConstantNames()[i] + ":\t" + QString::number(m_model->Constant(i))+ "\n";
    for(int i = 0; i < m_model->ConcentrationModel()->columnCount(); ++i)
        text += m_model->ConcentrationModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    for(int i = 0; i < m_model->ConstantSize(); ++i)
        text += m_model->ConstantNames()[i] + "\t";
    text += "\n";
    text += m_model->getConcentrations()->ExportAsString();
    text += "\n";
    text += "\n";
    text += "Equilibrium Model Signal Calculation with complexation constants:\n";
    for(int i = 0; i < m_model->SignalModel()->columnCount(); ++i)
        text += m_model->SignalModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    text += "\n";
    text += m_model->ModelTable()->ExportAsString();
    text += "\n";
    text += "Errors obtained from that calculcation:\n";
    for(int i = 0; i < m_model->SignalModel()->columnCount(); ++i)
        text += m_model->SignalModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    text += "\n";
    text += m_model->ErrorTable()->ExportAsString();
    text += "\n";
    text += "## Current Model Results Done ####\n";
    m_logging += text;
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
#include "modelwidget.moc"

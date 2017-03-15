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

#include "src/core/toolset.h"
#include "src/core/dataclass.h"
#include "src/core/jsonhandler.h"
#include "src/core/AbstractModel.h"
#include "src/core/minimizer.h"
#include "src/capabilities/continuousvariation.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/ui/dialogs/configdialog.h"
#include "src/ui/dialogs/advancedsearch.h"
#include "src/ui/widgets/3dchartview.h"
#include "src/ui/widgets/optimizerflagwidget.h"
#include "src/ui/widgets/chartwidget.h"
#include "src/ui/chartwrapper.h"
#include "src/ui/widgets/statisticwidget.h"
#include "src/ui/widgets/modeltablewidget.h"
#include "src/ui/widgets/modelactions.h"

#include "src/ui/dialogs/modeldialog.h"
#include "src/ui/dialogs/statisticdialog.h"

#include "chartwidget.h"
#include "chartview.h"

#include <QtMath>
#include "cmath"
#include <QApplication>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QTimer>

#include <QtWidgets/QInputDialog>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QTableView>
#include <QtWidgets/QAction>
#include <QtWidgets/QMenu>


#include <QtCharts/QChart>
#include <QtCharts/QXYSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>

#include <QtDataVisualization>

#include <QDebug>

#include <iostream>
#include <random>

#include "modelwidget.h"

SpinBox::SpinBox(QWidget * parent)
: QDoubleSpinBox(parent)
, valueBeingSet(false)
{
    connect(this,SIGNAL(valueChanged(double)),this,SLOT(On_valueChanged(double)));
}

void    SpinBox::setValue ( double val )
{
    valueBeingSet = true;
    QDoubleSpinBox::setValue(val);
    valueBeingSet = false;
}

void SpinBox::On_valueChanged(double val)
{
    if(!valueBeingSet)
        emit valueChangedNotBySet(val);
}


ModelElement::ModelElement(QSharedPointer<AbstractTitrationModel> model, Charts charts, int no, QWidget* parent) : QGroupBox(parent), m_model(model), m_charts(charts), m_no(no)
{
    QVBoxLayout *layout = new QVBoxLayout;
    QHBoxLayout *shifts = new QHBoxLayout;
    m_d_0 = new SpinBox;
    shifts->addWidget(m_d_0);
    m_d_0->setSingleStep(1e-2);
    m_d_0->setDecimals(4);
    m_d_0->setSuffix(" ppm");
    m_d_0->setValue(m_model->PureSignal(m_no));
    m_d_0->setToolTip(tr("Shift of the pure - non silent substrat"));
    m_d_0->setMaximumWidth(130);
    connect(m_d_0, SIGNAL(valueChangedNotBySet(double)), this, SIGNAL(ValueChanged()));
    
    for(int i = 0; i < m_model->ConstantSize(); ++i)
    {
        QPointer<SpinBox >constant = new SpinBox;
        m_constants << constant;
        constant->setSingleStep(1e-2);
        constant->setDecimals(4);
        constant->setSuffix("ppm");
        constant->setValue(m_model->Pair(i, m_no).second);
        constant->setToolTip(tr("Shift of the pure %1 complex").arg(m_model->ConstantNames()[i]));
        constant->setMaximumWidth(130);
        connect(constant, SIGNAL(valueChangedNotBySet(double)), this, SIGNAL(ValueChanged()));
        shifts->addWidget(constant, 0);
    }
    
    if(m_model->Type() != 3)
    {
        m_error = new QLabel;
        shifts->addStretch(300);
        shifts->addWidget(m_error); 
        m_error->setText("Sum of Squares: <b>" + QString::number(m_model->SumOfErrors(m_no)) + "</b>");
    } 
    layout->addLayout(shifts);
    QHBoxLayout *tools = new QHBoxLayout;
    m_include = new QCheckBox(this);
    m_include->setText("Include");
    m_include->setToolTip(tr("If checked, this signal will be included in model generation. "));
    m_include->setChecked(m_model->ActiveSignals()[m_no]);
    connect(m_include, SIGNAL(clicked()), this, SLOT(toggleActive()));
    tools->addWidget(m_include);
    m_error_series = qobject_cast<LineSeries *>(m_charts.signal_wrapper->Series(m_no));
    m_signal_series = qobject_cast<LineSeries *>(m_charts.error_wrapper->Series(m_no));
    m_error_series->setVisible(m_model->ActiveSignals()[m_no]);
    m_signal_series->setVisible(m_model->ActiveSignals()[m_no]);
    m_show = new QCheckBox;
    m_show->setText(tr("Show in Plot"));
    m_show->setToolTip(tr("Show this Curve in Model and Error Plot"));
    m_show->setChecked(m_model->ActiveSignals()[m_no]);
    tools->addWidget(m_show);
    
    m_plot = new QPushButton;
    m_plot->setText(tr("Color"));
    m_plot->setFlat(true);
    tools->addWidget(m_plot);
    setLayout(layout);
    
    m_toggle = new QPushButton(tr("Single Plot"));
    m_toggle->setFlat(true);
    m_toggle->setCheckable(true);
    tools->addWidget(m_toggle);
    connect(m_toggle, SIGNAL(clicked()), this, SLOT(togglePlot()));
    layout->addLayout(tools);
    setMaximumHeight(75);
    setMinimumHeight(75); 
    ColorChanged(m_charts.data_wrapper->color(m_no));
    connect(m_charts.data_wrapper->Series(m_no), SIGNAL(colorChanged(QColor)), this, SLOT(ColorChanged(QColor)));
    connect(m_plot, SIGNAL(clicked()), this, SLOT(ChooseColor()));
    connect(m_show, SIGNAL(stateChanged(int)), m_signal_series, SLOT(ShowLine(int)));
    connect(m_show, SIGNAL(stateChanged(int)), m_error_series, SLOT(ShowLine(int)));    
    toggleActive();
}

ModelElement::~ModelElement()
{
    
}

void ModelElement::toggleActive()
{
    int state = m_include->isChecked();
    DisableSignal(state);
    emit ActiveSignalChanged();
}

void ModelElement::DisableSignal(int state)
{
    m_show->setChecked(state);
    m_error_series->ShowLine(state);
    m_signal_series->ShowLine(state);
    m_d_0->setEnabled(m_include->isChecked());
    for(int i = 0; i < m_model->ConstantSize(); ++i)
    {
        m_constants[i]->setEnabled(m_include->isChecked());
    }
}

bool ModelElement::Include() const
{
    return m_include->isChecked();
}


double ModelElement::D0() const
{
    
    return m_d_0->value();
}


QVector<double > ModelElement::D() const
{
    QVector<double > numbers;
    for(int i = 0; i < m_constants.size(); ++i)
        numbers << m_constants[i]->value();
    return numbers;
}

void ModelElement::Update()
{
    m_include->setChecked(m_model->ActiveSignals()[m_no]);
    DisableSignal(m_model->ActiveSignals()[m_no]);
    if(!m_include->isChecked())
        return;
    m_d_0->setValue(m_model->PureSignal(m_no));
    for(int i = 0; i < m_model->ConstantSize(); ++i)
    {
        m_constants[i]->setValue(m_model->Pair(i, m_no).second);
    }
    if(m_model->Type() != 3)
        m_error->setText("Sum of Squares: " + QString::number(m_model->SumOfErrors(m_no)));
    
}

void ModelElement::ColorChanged(const QColor &color)
{
    
    #ifdef _WIN32
    setStyleSheet("background-color:" + color.name()+ ";");
    #else
    QPalette pal = palette();
    pal.setColor(QPalette::Background,color);
    setPalette(pal); 
    #endif
    
    m_color = color;
}


void ModelElement::ChooseColor()
{
    
    QColor color = QColorDialog::getColor(m_color, this, tr("Choose Color for Series"));
    if(!color.isValid())
        return;
    
    m_signal_series->setColor(color);
    m_error_series->setColor(color);
    ColorChanged(color);
}

void ModelElement::ToggleSeries(int i)
{
    m_signal_series->setVisible(i);
    m_error_series->setVisible(i);
    m_show->setChecked(i);
}

void ModelElement::togglePlot()
{
    if(m_toggle->isChecked())
        m_charts.data_wrapper->showSeries(m_no); 
    else
        m_charts.data_wrapper->showSeries(-1);
}

ModelWidget::ModelWidget(QSharedPointer<AbstractTitrationModel > model,  Charts charts, QWidget *parent ) : QWidget(parent), m_model(model), m_charts(charts), m_pending(false), m_minimizer(QSharedPointer<Minimizer>(new Minimizer(this), &QObject::deleteLater)), m_statistic(false)
{
    Data2Text();
    m_minimizer->setModel(m_model);
    m_advancedsearch = new AdvancedSearch(this);
    m_advancedsearch->setModel(m_model);
    
    m_statistic_dialog = new StatisticDialog(m_model, this);
    connect(m_statistic_dialog, SIGNAL(MCStatistic()), this, SLOT(MCStatistic()));
    connect(m_statistic_dialog, SIGNAL(CVStatistic()), this, SLOT(CVStatistic()));
    
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
    //     m_statistic_dialog->hide();
    delete m_statistic_result;
    //     m_search_result->hide();
    delete m_search_result;
    //     m_table_dialog->hide();
    delete m_table_result;
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
//     m_logging += m_statistic_widget->Overview();
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
    view = new ChartView(chart);
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
    view = new ChartView(chart);
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
        view = new ChartView(chart);
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

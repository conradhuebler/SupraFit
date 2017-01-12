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
#include "src/core/dataclass.h"
#include "src/core/jsonhandler.h"
#include "src/core/AbstractModel.h"
#include "src/ui/dialogs/configdialog.h"
#include "src/ui/widgets/modelhistorywidget.h"

#include "chartwidget.h"

#include <QtMath>
#include "cmath"
#include <QApplication>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtWidgets/QFileDialog>
#include <QtCore/QTimer>
#include <QtWidgets/QGroupBox>
#include <QGridLayout>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>
#include <QPushButton>
#include <QtWidgets/QLineEdit>
#include <QDebug>
#include <QCheckBox>
#include <QMessageBox>
#include <QColorDialog>
#include "modelwidget.h"

ModelElement::ModelElement(QPointer<AbstractTitrationModel> model, int no, QWidget* parent) : QGroupBox(parent), m_model(model), m_no(no)
{
    QGridLayout *layout = new QGridLayout;
    m_d_0 = new QDoubleSpinBox;
    layout->addWidget(m_d_0, 0, 0);
    m_d_0->setSingleStep(1e-2);
    m_d_0->setDecimals(4);
    m_d_0->setSuffix(" ppm");
    m_d_0->setValue(m_model->PureSignal(m_no));
    
    connect(m_d_0, SIGNAL(valueChanged(double)), this, SIGNAL(ValueChanged()));
    
    for(int i = 0; i < m_model->ConstantSize(); ++i)
    {
        QPointer<QDoubleSpinBox >constant = new QDoubleSpinBox;
        m_constants << constant;
        constant->setSingleStep(1e-2);
        constant->setDecimals(4);
        constant->setSuffix("ppm");
        constant->setValue(m_model->Pair(i, m_no).second);
        connect(constant, SIGNAL(valueChanged(double)), this, SIGNAL(ValueChanged()));
        layout->addWidget(constant, 0, i + 1);
    }
    
    
    if(m_model->Type() != 3)
    {
        error = new QLineEdit;
        error->setReadOnly(true);
        layout->addWidget(error, 0, m_model->ConstantSize() + 1); 
        error->setText(QString::number(m_model->SumOfErrors(m_no)));
    } 
    
    
    
    m_include = new QCheckBox(this);
    m_include->setText("Include");
    m_include->setToolTip(tr("Include in Model Generation"));
    m_include->setChecked(m_model->ActiveSignals()[m_no]);
    connect(m_include, SIGNAL(stateChanged(int)), this, SIGNAL(ActiveSignalChanged()));
    layout->addWidget(m_include, 1, 0);
    m_error_series = qobject_cast<LineSeries *>(m_model->ModelMapper(m_no)->series());
    m_signal_series = qobject_cast<LineSeries *>(m_model->ErrorMapper(m_no)->series());
    m_error_series->setVisible(m_model->ActiveSignals()[m_no]);
    m_signal_series->setVisible(m_model->ActiveSignals()[m_no]);
    m_show = new QCheckBox;
    m_show->setText(tr("Show in Plot"));
    m_show->setToolTip(tr("Show this Curve in Model and Error Plot"));
    m_show->setChecked(m_model->ActiveSignals()[m_no]);
    layout->addWidget(m_show,1,1);
    
    m_plot = new QPushButton;
    m_plot->setText(tr("Color"));
    m_plot->setFlat(true);
    layout->addWidget(m_plot, 1, 2);
    setLayout(layout);
    
    setMaximumHeight(75);
    setMinimumHeight(75); 
    ColorChanged(m_model->color(m_no));
    connect(m_model->DataMapper(m_no)->series(), SIGNAL(colorChanged(QColor)), this, SLOT(ColorChanged(QColor)));
    connect(m_plot, SIGNAL(clicked()), this, SLOT(ChooseColor()));
    connect(m_show, SIGNAL(stateChanged(int)), m_signal_series, SLOT(ShowLine(int)));
    connect(m_show, SIGNAL(stateChanged(int)), m_error_series, SLOT(ShowLine(int)));
    connect(m_model, SIGNAL(Recalculated()), this, SLOT(Update()));
    
}

ModelElement::~ModelElement()
{
    delete m_model;
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
    
    m_d_0->setValue(m_model->PureSignal(m_no));
    
    for(int i = 0; i < m_model->ConstantSize(); ++i)
    {
        m_constants[i]->setValue(m_model->Pair(i, m_no).second);
    }
    if(m_model->Type() != 3)
        error->setText(QString::number(m_model->SumOfErrors(m_no)));
    
}

void ModelElement::SetOptimizer()
{
    
    
    
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

ModelWidget::ModelWidget(QPointer<AbstractTitrationModel > model, QWidget *parent ) : m_model(model), QWidget(parent), m_pending(false)
{
    qDebug() << m_model->Constants();
    m_layout = new QGridLayout;
    QLabel *pure_shift = new QLabel(tr("Pure Shift"));
    m_layout->addWidget(pure_shift, 0, 0);
    for(int i = 0; i < m_model->ConstantSize(); ++i)
    {
        QPointer<QDoubleSpinBox >constant = new QDoubleSpinBox;
        m_constants << constant;
        constant->setSingleStep(1e-2);
        constant->setDecimals(4);
        constant->setPrefix("10^");
        constant->setValue(m_model->Constants()[i]);
        connect(constant, SIGNAL(valueChanged(double)), this, SLOT(recalulate()));
        
        m_layout->addWidget(constant, 0, i+1);
    }
    m_layout->addWidget( new QLabel(tr("Error")), 0, m_model->ConstantSize()+2);
    m_sign_layout = new QVBoxLayout;
    m_sign_layout->setAlignment(Qt::AlignTop);
    
    
    for(int i = 0; i < m_model->SignalCount(); ++i)
    {
        ModelElement *el = new ModelElement(m_model, i);
        connect(el, SIGNAL(ValueChanged()), this, SLOT(recalulate()));
        connect(el, SIGNAL(ActiveSignalChanged()), this, SLOT(CollectActiveSignals()));
        m_sign_layout->addWidget(el);
        m_model_elements << el;
    }
    m_layout->addLayout(m_sign_layout,2,0,1,m_model->ConstantSize()+3);
    
    m_new_guess = new QPushButton(tr("New Guess"));
    connect(m_new_guess, SIGNAL(clicked()), this, SLOT(NewGuess()));
    if(m_model->Type() == 1)
        DiscreteUI();
    else if(m_model->Type() == 3)
        EmptyUI();
    
    
//     m_modelhistorydialog = new ModelHistoryDialog(&m_history, this);
//     connect(m_modelhistorydialog, SIGNAL(AddModel(QJsonObject)), this, SIGNAL(AddModel(QJsonObject)));
//     connect(m_modelhistorydialog, SIGNAL(LoadModel(QJsonObject)), this, SLOT(LoadJson(QJsonObject)));
    setLayout(m_layout);
    m_model->CalculateSignal();
    QTimer::singleShot(1, this, SLOT(Repaint()));;
}

ModelWidget::~ModelWidget()
{
    delete m_model;
}



void ModelWidget::DiscreteUI()
{
    m_minimize_all = new QPushButton(tr("Global Fit"));
    m_minimize_single = new QPushButton(tr("Local Fits"));
    m_optim_config = new QPushButton(tr("Fit Settings"));
    m_import = new QPushButton(tr("Load Constants"));
    m_export = new QPushButton(tr("Save Constants"));
//     m_showhistory = new QPushButton(tr("Show History"));
    m_maxiter = new QSpinBox;
    m_maxiter->setValue(20);
    m_maxiter->setMaximum(999999);
    QHBoxLayout *mini = new QHBoxLayout;
    
    
    connect(m_minimize_all, SIGNAL(clicked()), this, SLOT(GlobalMinimize()));
    connect(m_minimize_single, SIGNAL(clicked()), this, SLOT(LocalMinimize()));
    connect(m_optim_config, SIGNAL(clicked()), this, SLOT(OptimizerSettings()));
    connect(m_import, SIGNAL(clicked()), this, SLOT(ImportConstants()));
    connect(m_export, SIGNAL(clicked()), this, SLOT(ExportConstants()));
//     connect(m_showhistory, SIGNAL(clicked()), this, SLOT(ShowHistory()));
    m_sum_error = new QLineEdit;
    m_sum_error->setReadOnly(true);
    
    mini->addWidget(m_new_guess);
    mini->addWidget(m_minimize_all);
    mini->addWidget(m_minimize_single);
    mini->addWidget(m_optim_config);
    m_layout->addLayout(mini, 3, 0,1,m_model->ConstantSize()+3);
    QHBoxLayout *mini_data = new QHBoxLayout;
    mini_data->addWidget(m_import);
    mini_data->addWidget(m_export);
//     mini_data->addWidget(m_showhistory);
    m_layout->addLayout(mini_data, 4, 0,1,m_model->ConstantSize()+3 );
    QHBoxLayout *mini2 = new QHBoxLayout;
    mini2->addWidget(new QLabel(tr("No. of max. Iter.")));
    mini2->addWidget(m_maxiter);
    mini2->addWidget(new QLabel(tr("Sum of Error:")));
    mini2->addWidget(m_sum_error);
    m_layout->addLayout(mini2, 5, 0,1,m_model->ConstantSize()+3);
    
}

void ModelWidget::EmptyUI()
{
    m_add_sim_signal = new QPushButton(tr("Add Signal"));
    connect(m_add_sim_signal, SIGNAL(clicked()), this, SLOT(AddSimSignal()));
    m_layout->addWidget(m_add_sim_signal);
}



void ModelWidget::Repaint()
{
    if(m_model->Type() == 3)
        return;
    m_pending = true;
    qreal error = 0;
    for(int j = 0; j < m_model_elements.size(); ++j)
    {
        error += m_model->SumOfErrors(j);
        m_model_elements[j]->Update();
    }
    
    m_sum_error->setText(QString::number(error));
    m_pending = false;
}



void ModelWidget::recalulate()
{
    if(m_pending)
        return;
    m_pending = true;
    
    CollectParameters();
    m_model->CalculateSignal();
    QTimer::singleShot(1, this, SLOT(Repaint()));;
    m_pending = false;
}

void ModelWidget::CollectParameters()
{
    QVector<qreal > pure_signals, constants;
    QVector<QVector <qreal > > complex_signals;
    complex_signals.resize(m_model->ConstantSize());
    QVector<int > active_signals(m_model_elements.size(), 0);
    for(int i = 0; i < m_model_elements.size(); ++i)
    {
        pure_signals << m_model_elements[i]->D0();
        active_signals[i] = m_model_elements[i]->Include();
        for(int j = 0; j < m_model_elements[i]->D().size(); ++j)
        {
            complex_signals[j] << m_model_elements[i]->D()[j];
        }
    }
    for(int j = 0; j < m_model->ConstantSize(); ++j)
        m_model->setComplexSignals(complex_signals[j], j + 1);
    for(int i = 0; i < m_model->ConstantSize(); ++i)
        constants << m_constants[i]->value();
    m_model->setActiveSignals(active_signals);
    m_model->setConstants(constants);
    m_model->setPureSignals(pure_signals);
}


void ModelWidget::GlobalMinimize()
{
    emit RequestCrashFile();
    if(m_maxiter->value() > 10000)
    {
        int r = QMessageBox::warning(this, tr("So viel."),
                                     tr("Wirklich so lange rechnen? "),
                                     QMessageBox::Yes | QMessageBox::Default,
                                     QMessageBox::No | QMessageBox::Escape);
        if (r == QMessageBox::No)
            return;
    }
    if(m_pending)
        return;
    m_pending = true;
    CollectParameters();
    QVector<int > v(10,0);
    OptimizerConfig config = m_model->getOptimizerConfig();
    config.MaxIter = m_maxiter->value();
    m_model->setOptimizerConfig(config);
    
    QVector<qreal > constants = m_model->Minimize();
    addToHistory();
    for(int j = 0; j < constants.size(); ++j)
        m_constants[j]->setValue(constants[j]);
    Repaint();
    m_pending = false; 
    emit RequestRemoveCrashFile();
}


void ModelWidget::LocalMinimize()
{
    emit RequestCrashFile();
    if(m_maxiter->value() > 10000)
    {
        int r = QMessageBox::warning(this, tr("So viel."),
                                     tr("Wirklich so lange rechnen? "),
                                     QMessageBox::Yes | QMessageBox::Default,
                                     QMessageBox::No | QMessageBox::Escape);
        if (r == QMessageBox::No)
            return;
    }
    if(m_pending)
        return;
    //     m_pending = true;
    CollectParameters();
    
    for(int i = 0; i < m_model->SignalCount(); ++i)
    {
        QApplication::processEvents();
        QVector<int > active_signals(m_model_elements.size(), 0);
        active_signals[i] = 1;
        m_model->setActiveSignals(active_signals);
        QVector<int > v(10,0);
        qDebug() <<"Start Minimize";
        OptimizerConfig config = m_model->getOptimizerConfig();
        config.MaxIter = m_maxiter->value();
        m_model->setOptimizerConfig(config);
        
        
        QVector<qreal > constants = m_model->Minimize();
        addToHistory();
        qDebug() <<"Minimize done";
    }
    qDebug() << "Constants set.";
    qDebug() << "leaving";
    
    emit RequestRemoveCrashFile();
    
}


void ModelWidget::AddSimSignal()
{
    
    ModelElement *el = new ModelElement(m_model, m_model_elements.size());
    //     m_model->addRow(m_model_elements.size()); 
    emit m_model->RowAdded();
    m_sign_layout->addWidget(el);
    m_model_elements << el;
    connect(el, SIGNAL(ValueChanged()), this, SLOT(recalulate()));
    
}

QVector<int> ModelWidget::ActiveSignals()
{
    QVector<int > active_signals(m_model_elements.size(), 0);
    for(int i = 0; i < m_model_elements.size(); ++i)
        active_signals[i] = m_model_elements[i]->Include();
    return active_signals;
}

void ModelWidget::CollectActiveSignals()
{
    QVector<int > active_signals = ActiveSignals();
    //     emit ActiveSignalChanged(active_signals);
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
    QVector<qreal > constants = m_model->Constants();
    for(int j = 0; j < constants.size(); ++j)
        m_constants[j]->setValue(constants[j]);
}

void ModelWidget::setMaxIter(int maxiter)
{
    m_maxiter->setValue(maxiter);
}

void ModelWidget::OptimizerSettings()
{
    OptimizerDialog dialog(m_model->getOptimizerConfig(), this);
    if(dialog.exec() == QDialog::Accepted)
    {
        m_model->setOptimizerConfig(dialog.Config());
        m_maxiter->setValue(dialog.Config().MaxIter);
    }
    
}
void ModelWidget::ExportConstants()
{
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), ".", tr("Json File (*.json);;Binary (*.jdat);;All files (*.*)" ));
    if(!str.isEmpty())
    {
        QJsonObject gameObject = m_model->ExportJSON(true);
        JsonHandler::WriteJsonFile(gameObject, str);
    }
    
}

void ModelWidget::ImportConstants()
{
    QString str = QFileDialog::getOpenFileName(this, tr("Save File"), ".", tr("Json File (*.json);;Binary (*.jdat);;All files (*.*)" ));
    if(!str.isEmpty())
    {
        QJsonObject object;
        if(JsonHandler::ReadJsonFile(object, str))
        {
            LoadJson(object);
            //             m_model->CalculateSignal();
        }
        else
            qDebug() << "loading failed";
    }
}


void ModelWidget::LoadJson(const QJsonObject& object)
{
    m_model->ImportJSON(object);
            QVector<qreal > constants = m_model->Constants();
            for(int j = 0; j < constants.size(); ++j)
                m_constants[j]->setValue(constants[j]);
}


void ModelWidget::addToHistory()
{
    QJsonObject model = m_model->ExportJSON(true);
    ModelHistoryElement element;
    element.model = model;
    element.active_signals = m_model->ActiveSignals();
    qreal error = 0;
    for(int j = 0; j < m_model_elements.size(); ++j)
    {
        error += m_model->SumOfErrors(j);
        m_model_elements[j]->Update();
    }
    element.error = error;
    emit InsertModel(element);
//     m_history[m_history.size()] = element;
}

// void ModelWidget::ShowHistory()
// {
//     m_modelhistorydialog->show();
// }


#include "modelwidget.moc"

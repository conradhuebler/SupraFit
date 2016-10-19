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
#include "core/data/dataclass.h"
#include "core/data/modelclass.h"
#include <QtMath>
#include "cmath"
#include <QApplication>
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
#include "modelwidget.h"

ModelElement::ModelElement(QPointer<AbstractTitrationModel> model, int no, QWidget* parent) : QGroupBox(parent), m_model(model), m_no(no)
{
    QHBoxLayout *layout = new QHBoxLayout;
    m_d_0 = new QDoubleSpinBox;
    layout->addWidget(m_d_0);
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
        layout->addWidget(constant);
    }
    
    m_handle = new QCheckBox(this);
    
    QPalette pal = palette();
    pal.setColor(QPalette::Window,m_model->ColorCode(no));
    m_handle->setAutoFillBackground(true);
    setPalette(pal);
    m_handle->setText("Use");
    m_handle->setChecked(true);
    connect(m_handle, SIGNAL(stateChanged(int)), this, SIGNAL(ActiveSignalChanged()));
    layout->addWidget(m_handle);
    if(m_model->Type() != 3)
    {
    error = new QLineEdit;
    error->setReadOnly(true);
    layout->addWidget(error); 
    error->setText(QString::number(m_model->SumOfErrors(m_no)));
    }
    setLayout(layout);
    
    setMaximumHeight(50);
    setMinimumHeight(50);
    
    connect(m_model, SIGNAL(Recalculated()), this, SLOT(Update()));
    
}

ModelElement::~ModelElement()
{
}

bool ModelElement::Handle() const
{
    return m_handle->isChecked();
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
    
    
    for(int i = 0; i < m_model->MaxVars(); ++i)
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
    
    
    setLayout(m_layout);
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
    m_maxiter = new QSpinBox;
    m_maxiter->setValue(20);
    m_maxiter->setMaximum(999999);
    QHBoxLayout *mini = new QHBoxLayout;
    
    
    connect(m_minimize_all, SIGNAL(clicked()), this, SLOT(GlobalMinimize()));
    connect(m_minimize_single, SIGNAL(clicked()), this, SLOT(LocalMinimize()));

        
    m_sum_error = new QLineEdit;
    m_sum_error->setReadOnly(true);
    
    mini->addWidget(m_new_guess);
    mini->addWidget(m_minimize_all);
    mini->addWidget(m_minimize_single);
    mini->addWidget(new QLabel(tr("No. of max. Iter.")));
    mini->addWidget(m_maxiter);
    mini->addWidget(new QLabel(tr("Sum of Error:")));
    mini->addWidget(m_sum_error);
    m_layout->addLayout(mini, 3, 0,1,2);
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
    qreal error = 0;
    for(int j = 0; j < m_model_elements.size(); ++j)
        error += m_model->SumOfErrors(j);

    m_sum_error->setText(QString::number(error));
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
        active_signals[i] = m_model_elements[i]->Handle();
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
                QVector<int > v(10,0);
        qDebug() <<"Start Minimize";
        QVector<qreal > constants = m_model->Minimize(m_maxiter->value());
        qDebug() <<"Minimize done";
//          QVector<qreal > constants =  m_model->Constants();
        qDebug() << constants;
        for(int j = 0; j < constants.size(); ++j)
            m_constants[j]->setValue(constants[j]);
        qDebug() << "Constants set.";
    qDebug() << "leaving";
}


void ModelWidget::LocalMinimize()
{
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
        QVector<qreal > constants = m_model->Minimize(m_maxiter->value());
        qDebug() <<"Minimize done";
        }
//         QVector<qreal > constants =  m_model->Constants();
//         qDebug() << constants;
//         for(int j = 0; j < constants.size(); ++j)
//             m_constants[j]->setValue(constants[j]);
        qDebug() << "Constants set.";
    qDebug() << "leaving"; 
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
        active_signals[i] = m_model_elements[i]->Handle();
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


#include "modelwidget.moc"
#include <QCheckBox>

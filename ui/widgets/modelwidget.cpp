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

#include <QtCore/QTimer>
#include <QGridLayout>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>
#include <QPushButton>
#include <QtWidgets/QLineEdit>
#include <QDebug>
#include "modelwidget.h"
ModelWidget::ModelWidget(QPointer<AbstractTitrationModel > model, QWidget *parent ) : m_model(model), QWidget(parent), m_pending(false)
{
    qDebug() << m_model->Constants();
    QGridLayout *layout = new QGridLayout;
    QVBoxLayout *sign_layout = new QVBoxLayout;
    QLabel *pure_shift = new QLabel(tr("Pure Shift"));
    sign_layout->addWidget(pure_shift);
    for(int i = 0; i < m_model->MaxVars(); ++i)
    {
        QDoubleSpinBox *signal = new QDoubleSpinBox;
        signal->setSingleStep(1e-2);
        signal->setDecimals(4);
        signal->setSuffix(" ppm");
        signal->setValue(m_model->PureSignal(i));
        connect(signal, SIGNAL(valueChanged(QString)), this, SLOT(recalulate()));
        m_pure_signals << signal;
        sign_layout->addWidget(signal);
    }
    QHBoxLayout *complex_layout = new QHBoxLayout;
    for(int i = 0; i < m_model->ConstantSize(); ++i)
    {
        QPointer<QDoubleSpinBox >constant = new QDoubleSpinBox;
            m_constants << constant;
            constant->setSingleStep(1e-2);
            constant->setDecimals(4);
            constant->setPrefix("10^");
            constant->setValue(m_model->Constants()[i]);
            connect(constant, SIGNAL(valueChanged(QString)), this, SLOT(recalulate()));
        QVBoxLayout *sign_layout = new QVBoxLayout;
            sign_layout->addWidget(constant);
        
        QLabel *pure_shift = new QLabel(tr("K11 Shift"));
        sign_layout->addWidget(pure_shift);
        
        
        QVector<QPointer < QDoubleSpinBox > > row;
        qDebug() << "Maxvars" << m_model->MaxVars();
        for(int j = 0; j < m_model->MaxVars(); ++j)
        {
            QDoubleSpinBox *signal = new QDoubleSpinBox;
            signal->setSingleStep(1e-2);
            signal->setDecimals(4);
            signal->setSuffix(" ppm");
            signal->setValue(m_model->Pair(i, j).second);
            connect(signal, SIGNAL(valueChanged(QString)), this, SLOT(recalulate()));
            sign_layout->addWidget(signal);
            row << signal;
            
            
            
        } 
         complex_layout->addLayout(sign_layout);
        m_complex_signals << row;
        
    }
    QVBoxLayout *error_layout = new QVBoxLayout;    
    QLabel *error_label = new QLabel(tr("Summ of Error:"));
            error_layout->addWidget(error_label);
         for(int j = 0; j < m_model->MaxVars(); ++j)
        {
          QPointer<QLineEdit > error = new QLineEdit;
                error->setReadOnly(true);
            error_layout->addWidget(error); 
                error->setText(QString::number(m_model->SumOfErrors(j)));
                m_errors << error;  
            
        }
    
    layout->addLayout(error_layout, 1, 2);
    m_minimize = new QPushButton("mini");
    layout->addLayout(sign_layout, 1, 0);
    layout->addLayout(complex_layout, 1, 1);
    layout->addWidget(m_minimize, 2, 0);
    connect(m_minimize, SIGNAL(clicked()), this, SLOT(Minimize()));
    
    m_sum_error = new QLineEdit;
    layout->addWidget(m_sum_error, 2, 1);
    m_sum_error->setReadOnly(true);
    setLayout(layout);
    QTimer::singleShot(1, this, SLOT(Repaint()));;
}

ModelWidget::~ModelWidget()
{
    
}
void ModelWidget::Repaint()
{
    qreal error = 0;
    for(int j = 0; j < m_errors.size(); ++j)
    {
        m_errors[j]->setText(QString::number(m_model->SumOfErrors(j)));
        error += m_model->SumOfErrors(j);
    }
    m_sum_error->setText(QString::number(error));
}



void ModelWidget::recalulate()
{
    if(m_pending)
        return;
    m_pending = true;
    QVector<qreal > pure_signals, constants;
    QVector<QVector <qreal > > complex_signals;
    complex_signals.resize(m_model->ConstantSize());
    for(int i = 0; i < m_pure_signals.size(); ++i)
    {
        pure_signals << m_pure_signals[i]->value();
        
        for(int j = 0; j < m_model->ConstantSize(); ++j)
        {
            complex_signals[j] << m_complex_signals[j][i]->value();
        }
    }
    for(int j = 0; j < m_model->ConstantSize(); ++j)
        m_model->setComplexSignals(complex_signals[j], j + 1);
    for(int i = 0; i < m_model->ConstantSize(); ++i)
        constants << m_constants[i]->value();
//     qDebug() << constants;
    m_model->setConstants(constants);
    m_model->setPureSignals(pure_signals);
    
    m_model->CalculateSignal();
    QTimer::singleShot(1, this, SLOT(Repaint()));;
    m_pending = false;
}

void ModelWidget::Minimize()
{
    
    QVector<int > v(10,0);
    m_model->Minimize(v);
    m_model->CalculateSignal();

    
    QVector<qreal > constants =  m_model->Constants();

        for(int j = 0; j < constants.size(); ++j)
            m_constants[j]->setValue(constants[j]);
            
        
 QTimer::singleShot(100, this, SLOT(Repaint()));
    
}



#include "modelwidget.moc"

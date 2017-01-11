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

#include <QtWidgets/QLineEdit>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QDialogButtonBox>
#include "chartconfig.h"

ChartConfigDialog::ChartConfigDialog()
{
    setModal(false);
    QGridLayout *layout = new QGridLayout;
    m_x_axis = new QLineEdit;
        connect(m_x_axis, SIGNAL(textChanged(QString)), this, SLOT(Changed()));
    m_y_axis = new QLineEdit;
        connect(m_y_axis, SIGNAL(textChanged(QString)), this, SLOT(Changed()));
    m_x_min = new QDoubleSpinBox;
        connect(m_x_min, SIGNAL(valueChanged(qreal)), this, SLOT(Changed()));
    m_x_max = new QDoubleSpinBox;
        connect(m_x_max, SIGNAL(valueChanged(qreal)), this, SLOT(Changed()));
    m_x_step = new QSpinBox;
        connect(m_x_step, SIGNAL(valueChanged(int)), this, SLOT(Changed()));
    m_y_min = new QDoubleSpinBox;
        m_y_min->setMinimum(-4);
        m_y_min->setSingleStep(0.0001);
        m_y_min->setDecimals(5);
        connect(m_y_min, SIGNAL(valueChanged(qreal)), this, SLOT(Changed()));
    m_y_max = new QDoubleSpinBox;
        m_y_max->setSingleStep(0.0001);
        m_y_max->setMinimum(-4);
        m_y_max->setDecimals(5);
        connect(m_y_max, SIGNAL(valueChanged(qreal)), this, SLOT(Changed()));
    m_y_step = new QSpinBox;
        connect(m_y_step, SIGNAL(valueChanged(int)), this, SLOT(Changed()));
    
    m_scaleaxis = new QPushButton(tr("Autoscale X"));
    connect(m_scaleaxis, SIGNAL(clicked()), this, SIGNAL(ScaleAxis()));
    
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
//     connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
/*    layout->addWidget(m_y_max, 0, 1);
    layout->addWidget(m_y_step, 1, 1);
    layout->addWidget(m_y_min, 2, 1);
    layout->addWidget(m_x_min, 3, 2);
    layout->addWidget(m_x_step , 3, 3);
    layout->addWidget(m_x_max, 3, 4);
    layout->addWidget(picture, 0, 2, 3, 3);
    layout->addWidget(m_y_axis, 2, 0);
    layout->addWidget(m_x_axis, 4, 2);
    layout->addWidget(m_buttons, 5, 0, 1, 3);
    */
    layout->addWidget(new QLabel(tr("x Label:")), 0, 0);
    layout->addWidget(m_x_axis, 0, 1);
    
    layout->addWidget(new QLabel(tr("Axis")), 1, 0);
    layout->addWidget(m_x_min, 1, 1);
    layout->addWidget(m_x_step, 1, 2);
    layout->addWidget(m_x_max, 1, 3);
    
    layout->addWidget(new QLabel(tr("y Label:")), 3, 0);
    layout->addWidget(m_y_axis, 3, 1);
    layout->addWidget(new QLabel(tr("Axis")), 4, 0);
    layout->addWidget(m_y_min, 4, 1);
    layout->addWidget(m_y_step, 4, 2);
    layout->addWidget(m_y_max, 4, 3);
    layout->addWidget(m_scaleaxis, 5, 0, 1, 3);
    layout->addWidget(m_buttons, 6, 0, 1, 3);
    setLayout(layout);
    
}


ChartConfigDialog::~ChartConfigDialog()
{
}

void ChartConfigDialog::setConfig(const ChartConfig& chartconfig)
{
    m_x_min->setValue(chartconfig.x_min);
    m_x_max->setValue(chartconfig.x_max);
    m_x_step->setValue(chartconfig.x_step); 
    m_y_min->setValue(chartconfig.y_min);
    m_y_max->setValue(chartconfig.y_max);
    m_y_step->setValue(chartconfig.y_step);
    
    m_x_axis->setText(chartconfig.x_axis);
    m_y_axis->setText(chartconfig.y_axis);
}

void ChartConfigDialog::Changed()
{
    m_chartconfig.x_axis = m_x_axis->text();
    m_chartconfig.y_axis = m_y_axis->text();
    m_chartconfig.x_min = m_x_min->value();
    m_chartconfig.x_max = m_x_max->value();
    m_chartconfig.x_step = m_x_step->value();  
     
    m_chartconfig.y_min = m_y_min->value();
    m_chartconfig.y_max = m_y_max->value();
    m_chartconfig.y_step = m_y_step->value();  
    
    emit ConfigChanged(Config());
}


#include "chartconfig.moc"

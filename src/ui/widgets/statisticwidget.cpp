/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/models.h"

#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>

#include "statisticwidget.h"

StatisticElement::StatisticElement(const QSharedPointer<AbstractTitrationModel> model, int no): m_model(model), m_no(no)
{
    connect(m_model.data(), SIGNAL(StatisticChanged(StatisticResult, int)), this, SLOT(UpdateStatistic(StatisticResult, int)));
    QGridLayout *layout = new QGridLayout;
    StatisticResult result;// = m_model->getStatisticResult(m_no);
    layout->addWidget(new QLabel(m_model->ConstantNames()[m_no]), 0, 0);
    m_value = new QLabel(QString::number(m_model->Constants()[m_no]));
    layout->addWidget(m_value, 0, 1);
    m_min = new QLabel(QString::number(result.min));
    layout->addWidget(m_min, 0, 2);
    m_max = new QLabel(QString::number(result.max));
    layout->addWidget(m_max, 0,3);
    m_range = new QLabel(QString::number(result.max - result.min));
    layout->addWidget(m_range, 0, 4);
    m_integ_1 = new QLabel(QString::number(result.integ_1));
    layout->addWidget(m_integ_1, 0, 5);
    m_integ_5 = new QLabel(QString::number(result.integ_5));
    layout->addWidget(m_integ_5, 0, 6);
    setLayout(layout);
}


StatisticElement::~StatisticElement()
{
    
}

void StatisticElement::UpdateStatistic(const StatisticResult& result, int i)
{
    if(m_no == i)
    {
        m_value->setText(QString::number(m_model->Constants()[m_no]));
        m_min->setText(QString::number(result.min));
        m_max->setText(QString::number(result.max));
        m_range->setText(QString::number(result.max-result.min));
        m_integ_1->setText(QString::number(result.integ_1));
        m_integ_5->setText(QString::number(result.integ_5));
    }
}


StatisticWidget::StatisticWidget(const QSharedPointer<AbstractTitrationModel> model, QWidget *parent) : m_model(model), QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;
    m_subwidget = new QWidget;
    m_subwidget->setLayout(layout);
    QVBoxLayout *m_layout = new QVBoxLayout;
    m_show = new QPushButton(tr("Toggle Statistic View"));
    m_show->setFlat(true);
    m_show->setMaximumHeight(25);
    connect(m_show, SIGNAL(clicked()), this, SLOT(toggleView()));
    m_layout->addWidget(m_show);
    for(int i = 0; i < m_model->ConstantSize(); ++i)
    {
        StatisticElement *element = new StatisticElement(m_model, i);
        layout->addWidget(element);
        m_elements << element;
    }
    
    m_layout->addWidget(m_subwidget); 
    setLayout(m_layout);
}

StatisticWidget::~StatisticWidget()
{
    
}

void StatisticWidget::toggleView()
{
    bool hidden = m_subwidget->isHidden();
    m_subwidget->setHidden(!hidden);
}



#include "statisticwidget.moc"

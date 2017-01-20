/*
 * <one line to give the library's name and an idea of what it does.>
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

#include "src/core/AbstractModel.h"

#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QPushButton>

#include <QDebug>

#include "advancedsearch.h"

ParameterWidget::ParameterWidget(const QString &name, QWidget* parent) : QGroupBox(name, parent)
{
    m_min = new QDoubleSpinBox;
    m_min->setValue(0.1);
    m_max = new QDoubleSpinBox;
    m_max->setValue(10);
    m_step = new QDoubleSpinBox;
    m_step->setValue(0.1);
    
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(new QLabel(tr("Min")), 0, 0);
    layout->addWidget(m_min, 0, 1);
    layout->addWidget(new QLabel(tr("Step")), 1, 0);
    layout->addWidget(m_step, 1, 1);
    layout->addWidget(new QLabel(tr("Max")), 2, 0);
    layout->addWidget(m_max, 2, 1);
    setLayout(layout);
}

double ParameterWidget::Max() const
{
    return m_max->text().toDouble();
}

double ParameterWidget::Min() const
{
    return m_min->text().toDouble();
}

double ParameterWidget::Step() const
{
    return m_step->text().toDouble();
}



AdvancedSearch::AdvancedSearch(QWidget *parent ) : QDialog(parent)
{
    setModal(false);
}


AdvancedSearch::~AdvancedSearch()
{
}

void AdvancedSearch::SetUi()
{
    if(m_model.isNull())
        return;
    
    QVBoxLayout *layout = new QVBoxLayout;
    
    for(int i = 0; i < m_model->ConstantSize(); ++i)
    {
        QPointer<ParameterWidget > widget = new ParameterWidget("name", this);
        layout->addWidget(widget);
        m_parameter_list << widget;
    }
    
    m_global = new QPushButton(tr("Global Search"));
    connect(m_global, SIGNAL(clicked()), this, SLOT(GlobalSearch()));
    m_optim = new QCheckBox(tr("Optimize"));
    
    QGridLayout *mlayout = new QGridLayout;
    mlayout->addLayout(layout, 0, 0, 1, 2);
    mlayout->addWidget(m_optim, 3, 0);
    mlayout->addWidget(m_global,3, 1);
    setLayout(mlayout);
}

void AdvancedSearch::GlobalSearch()
{
    
    QVector<double > list;
    for(int i = 0; i < m_parameter_list.size(); ++i)
    {
        double min = 0, max = 0, step = 0;
        min = m_parameter_list[i]->Min();
        max = m_parameter_list[i]->Max();
        step = m_parameter_list[i]->Step();
        for(double s = min; s <= max; s += step)
            list << s;
    }
    if(m_optim->isChecked())
    {
        
    }else
        Scan(list);
}


void AdvancedSearch::Scan(const QVector<qreal>& list)
{
    series.clear();
    for(int i = 0; i < list.size(); ++i)
    {
        m_model->setConstants(QVector<qreal>() << list[i]);
        m_model->CalculateSignal();
        qreal error = m_model->ModelError();
        series.append(QPointF(list[i], error));
    }
    emit finished(1);
}

#include "advancedsearch.moc"

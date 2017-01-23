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

#include <QtCore/QJsonObject>

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
    m_max->setValue(5);
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
    
    QVector< QVector<double > > full_list;
    for(int i = 0; i < m_parameter_list.size(); ++i)
    {
        QVector<double > list;
        double min = 0, max = 0, step = 0;
        min = m_parameter_list[i]->Min();
        max = m_parameter_list[i]->Max();
        step = m_parameter_list[i]->Step();
        for(double s = min; s <= max; s += step)
            list << s;
        full_list << list;
    }
    
    
    QVector<double > error;
    QVector< QVector<double > > input  = ConvertList(full_list, error);
    qDebug() << error;
    
    emit finished(m_optim->isChecked());
//     
}

QVector<QVector<double> > AdvancedSearch::ConvertList(const QVector<QVector<double> >& full_list, QVector<double > &error)
{
    QVector<int > position(full_list.size(), 0);
    QVector< QVector<double > > input;
    QtDataVisualization::QSurfaceDataRow *dataRow1 = new QtDataVisualization::QSurfaceDataRow;
    for(;;)
    {
        QVector<double > parameter(full_list.size(), 0);
        for(int j = 0; j < position.size(); ++j)
            parameter[j] = full_list[j][position[j]];
        
        bool allow_break = true;
        for(int i = 0; i < position.size(); ++i)
        {
            allow_break = allow_break && (position[i] == full_list[i].size() - 1);
        }
        m_model->setConstants(parameter);
        m_model->CalculateSignal();
        error << m_model->ModelError();
        last_result.m_error = error;
        last_result.m_input = full_list;

        *dataRow1 << QVector3D(parameter[0], m_model->ModelError(), parameter[1]);
        for(int k = position.size() - 1; k >= 0; --k)
        {
            if(position[k] == ( full_list[k].size() - 1) )
            {
                k--;
                if(k >= 0)
                {
                    position[k]++;
                    if(position[k] <= full_list[k].size() - 1)
                    {
                        m_3d_data << dataRow1;
                        dataRow1 = new QtDataVisualization::QSurfaceDataRow;
                        position[k + 1] = 0;
                    }
                    break;
                }
            }
            else
            {
                 position[k]++;
                 break;
            }
        }
                
        if(allow_break)
            return input;
    }
}


void AdvancedSearch::Scan(const QVector< QVector<double > >& list)
{

    QVector<double > error;
    
    for(int i = 0; i < list.size(); ++i)
    {
        m_model->setConstants(list[i]);
        m_model->CalculateSignal();
        error << m_model->ModelError();
    }
    
}

#include "advancedsearch.moc"

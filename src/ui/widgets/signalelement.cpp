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

#include "src/ui/guitools/chartwrapper.h"

#include <QApplication>
#include <QHeaderView>

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>

#include <QtCharts/QXYSeries>

#include <QStandardItemModel>

#include <QDebug>

#include "signalelement.h"

SignalElement::SignalElement(QWeakPointer<DataClass > data, QWeakPointer<ChartWrapper> wrapper, int no, QWidget *parent) : QGroupBox(parent), m_data(data), m_wrapper(wrapper), m_no(no)
{
    m_data_series = qobject_cast<ScatterSeries *>(m_wrapper.data()->Series(m_no)); //DataMapper(m_no)->series());
    QGridLayout *layout = new QGridLayout;
    
    m_show = new QCheckBox;
    m_show->setText(tr("Show"));
    m_show->setChecked(true);
    connect(m_show, SIGNAL(stateChanged(int)), this, SLOT(ShowLine(int)));
    
    m_name = new QLineEdit;
    QString name;
    name = data.data()->SignalModel()->headerData(m_no, Qt::Horizontal).toString();
    m_name->setText(name);
    m_data_series->setName(name);
    connect(m_name, SIGNAL(textChanged(QString)), this, SLOT(setName(QString)));
    
    m_choose = new QPushButton(tr("Color"));
    m_choose->setFlat(true);
    connect(m_choose, SIGNAL(clicked()), this, SLOT(chooseColor()));
    
    m_markerSize = new QDoubleSpinBox;
    m_markerSize->setMaximum(20);
    m_markerSize->setMinimum(0.1);
    m_markerSize->setSingleStep(1e-1);
    m_markerSize->setValue(10);
    setMarkerSize(10);
    connect(m_markerSize, SIGNAL(valueChanged(qreal)), this, SLOT(setMarkerSize(qreal)));
    
    m_rectangle = new QCheckBox(tr("Rectangle"));
    connect(m_rectangle, SIGNAL(stateChanged(int)), this, SLOT(setMarkerShape(int)));
    
    m_toggle = new QPushButton(tr("Single Plot"));
    m_toggle->setFlat(true);
    m_toggle->setCheckable(true);
    connect(m_toggle, SIGNAL(clicked()), this, SLOT(togglePlot()));
    
    layout->addWidget(m_name, 0, 0);
    layout->addWidget(m_show, 0, 1);
    layout->addWidget(m_choose, 0, 2);
    layout->addWidget(new QLabel(tr("Size")), 0, 3);
    layout->addWidget(m_markerSize, 0, 4);
    layout->addWidget(m_rectangle, 0, 5);
    layout->addWidget(m_toggle, 0, 6);
    setLayout(layout);
    ColorChanged(m_wrapper.data()->color(m_no));
}


SignalElement::~SignalElement()
{
    
    
}

void SignalElement::ColorChanged(const QColor &color)
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


void SignalElement::chooseColor()
{
    
    QColor color = QColorDialog::getColor(m_color, this, tr("Choose Color for Series"));
    if(!color.isValid())
        return;
    
    m_data_series->setColor(color);
    ColorChanged(color);
}

void SignalElement::ToggleSeries(int i)
{
    m_data_series->setVisible(i);
    m_show->setChecked(i);
}

void SignalElement::ShowLine(int i)
{
    m_data_series->ShowLine(i);
}

void SignalElement::setName(const QString &str)
{
    m_data_series->setName(str);
    emit m_data_series->NameChanged(str);
}

void SignalElement::setMarkerSize(qreal value)
{
    m_data_series->setMarkerSize(value);
}

void SignalElement::setMarkerShape(int shape)
{
    if(shape)
        m_data_series->setMarkerShape(ScatterSeries::MarkerShapeRectangle);
    else
        m_data_series->setMarkerShape(ScatterSeries::MarkerShapeCircle);
}

void SignalElement::togglePlot()
{
    if(m_toggle->isChecked())
        m_wrapper.data()->showSeries(m_no); 
    else
        m_wrapper.data()->showSeries(-1);
}

#include "signalelement.moc"

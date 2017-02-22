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

#include "src/ui/chartwrapper.h"

#include <QApplication>
#include <QHeaderView>

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTableView>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>

#include <QtCharts/QXYSeries>

#include <QStandardItemModel>

#include <QDebug>

#include "datawidget.h"

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
    
    layout->addWidget(m_name, 0, 0);
    layout->addWidget(m_show, 0, 1);
    layout->addWidget(m_choose, 0, 2);
    layout->addWidget(new QLabel(tr("Size")), 0, 3);
    layout->addWidget(m_markerSize, 0, 4);
    layout->addWidget(m_rectangle, 0, 5);
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

DataWidget::DataWidget() 
{
    layout = new QGridLayout;
    m_switch = new QPushButton(tr("Switch Host/Guest\nAssignment"));
    connect(m_switch, SIGNAL(clicked()), this, SLOT(switchHG()));
    m_name = new QLineEdit();
    connect(m_name, SIGNAL(textChanged(QString)), this, SLOT(SetProjectName()));
    m_concentrations = new QTableView;
    m_concentrations->setMaximumWidth(250);
    m_signals = new QTableView;
    m_signals->setMaximumWidth(750);
    
    QHBoxLayout *hlayout = new QHBoxLayout;
    
    hlayout->addWidget(new QLabel(tr("Project Name")));
    hlayout->addWidget(m_name);
    hlayout->addSpacing(4*width()/9);
    hlayout->addWidget(m_switch);
    
    m_datapoints = new QLabel;
    m_substances = new QLabel;
    m_const_subs = new QLabel;
    m_signals_count = new QLabel;
    m_tables = new QGroupBox(tr("Data Tables"));
    QHBoxLayout *group_layout = new QHBoxLayout;
    group_layout->addWidget(m_concentrations);
    group_layout->addWidget(m_signals);
    m_tables->setLayout(group_layout);
    
    
    
    layout->addLayout(hlayout, 0, 0, 1, 3);
    layout->addWidget(m_datapoints, 1, 0);
    layout->addWidget(m_substances, 2, 0);
    layout->addWidget(m_const_subs,3,0);
    
    layout->addWidget(m_signals_count, 5, 0);
    layout->addWidget(m_tables, 5, 0, 1, 4);
    
    setLayout(layout);
}

DataWidget::~DataWidget()
{
}

void DataWidget::setData(QWeakPointer<DataClass> dataclass, QWeakPointer<ChartWrapper> wrapper)
{  
    m_data = dataclass;
    m_wrapper = wrapper; 
    m_concentrations->setModel(m_data.data()->ConcentrationModel());
    
    m_signals->setModel(m_data.data()->SignalModel());
    connect(m_data.data()->SignalModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(HidePoint(QModelIndex,QModelIndex)));
    m_concentrations->resizeColumnsToContents();
    m_signals->resizeColumnsToContents();
    m_name->setText(qApp->instance()->property("projectname").toString());
    m_substances->setText(tr("Considered Substances: %1").arg(m_data.data()->ConcentrationModel()->columnCount()));
    m_datapoints->setText(tr("Data Points: %1").arg(m_data.data()->SignalModel()->rowCount()));
    m_signals_count->setText(tr("Signals: %1").arg(m_data.data()->SignalCount()));
    
    QVBoxLayout *vlayout = new QVBoxLayout;
    for(int i = 0; i < m_data.data()->SignalCount(); ++i)
    {
        QPointer<SignalElement > el = new SignalElement(m_data, m_wrapper, i, this);
        vlayout->addWidget(el);
        m_signal_elements << el;
    }
    layout->addLayout(vlayout, 1,1, 4, 3);
    
    QVBoxLayout *scaling_layout = new QVBoxLayout;
    for(int i = 0; i < m_data.data()->getScaling().size(); ++i)
    {
        
        QDoubleSpinBox *spin_box = new QDoubleSpinBox;
        spin_box->setMaximum(1);
        spin_box->setValue(m_data.data()->getScaling()[i]);
        spin_box->setSingleStep(1e-2);
        
        connect(spin_box, SIGNAL(valueChanged(double)), this, SLOT(setScaling()));
        m_scaling_boxes << spin_box;
        QHBoxLayout *lay = new QHBoxLayout;
        lay->addWidget(new QLabel(tr("scaling factor\n%1. concentration").arg(i + 1)));
        lay->addWidget(spin_box);
        scaling_layout->addLayout(lay);
    }
    layout->addLayout(scaling_layout,4,0);
    
}

void DataWidget::switchHG()
{
    m_data.data()->SwitchConentrations();
    m_wrapper.data()->UpdateModel();
    emit recalculate();
}

void DataWidget::RowAdded()
{
    QStandardItemModel *concentration = new QStandardItemModel;
    QStandardItemModel *signal = new QStandardItemModel;
    m_concentrations->setModel(concentration);
    m_signals->setModel(signal);
    m_concentrations->resizeColumnsToContents();
    m_signals->resizeColumnsToContents();
}

void DataWidget::SetProjectName()
{
    qApp->instance()->setProperty("projectname", m_name->text());
    emit NameChanged();
}

void DataWidget::setScaling()
{
    QVector<qreal> scaling;
    for(int i = 0; i < m_scaling_boxes.size(); ++i)
        scaling << m_scaling_boxes[i]->value();
    m_data.data()->setScaling(scaling);
    m_wrapper.data()->UpdateModel();
    emit recalculate();
}

void DataWidget::HidePoint(QModelIndex index, QModelIndex index2)
{
//     qDebug() << index.row() << index.column();
//      m_wrapper.data()->Series(index.column())->remove(index.row());
}

#include "datawidget.moc"

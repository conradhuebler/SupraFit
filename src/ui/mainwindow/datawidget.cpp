/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/ui/dialogs/regressionanalysisdialog.h"
#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/widgets/signalelement.h"
#include "src/ui/widgets/systemparameterwidget.h"

#include <QApplication>
#include <QDebug>
#include <QSettings>

#include <QtCore/QTimer>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTableView>

#include <QtCharts/QXYSeries>

#include "datawidget.h"


DataWidget::DataWidget() : m_system_parameter_loaded(false)
{
    m_widget = new QWidget;
    layout = new QGridLayout;
    m_switch = new QPushButton(tr("Switch H/G"));
    m_switch->setToolTip(tr("Switch Host/Guest\nAssignment"));
    m_switch->setStyleSheet("background-color: #77d740;");
    m_switch->setMaximumSize(100, 30);
    connect(m_switch, SIGNAL(clicked()), this, SLOT(switchHG()));
    
    m_linear = new QPushButton(tr("Regression"));
    m_linear->setToolTip(tr("Perform advanced linear regression"));
    m_linear->setStyleSheet("background-color: #77d740;");
    connect(m_linear, &QPushButton::clicked, this, &DataWidget::LinearAnalysis);
    
    m_name = new QLineEdit();
    connect(m_name, SIGNAL(textChanged(QString)), this, SLOT(SetProjectName()));
    m_concentrations = new QTableView;
    m_concentrations->setMaximumWidth(230);
    m_signals = new QTableView;
    m_signals->setMaximumWidth(750);
    m_signals->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_signals->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_signals, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowContextMenu(const QPoint&)));
    connect(m_concentrations->verticalScrollBar(), SIGNAL(valueChanged(int)), m_signals->verticalScrollBar(), SLOT(setValue(int)));
    QHBoxLayout *hlayout = new QHBoxLayout;
    
    hlayout->addWidget(new QLabel(tr("Project Name")), 0, Qt::AlignLeft);
    hlayout->addWidget(m_name, 0, Qt::AlignLeft);
    hlayout->addSpacerItem(new QSpacerItem(100,1));
    hlayout->addWidget(m_linear, 0, Qt::AlignLeft);
    hlayout->addWidget(m_switch, 0, Qt::AlignRight);
    
    m_datapoints = new QLabel;
    m_substances = new QLabel;
    m_const_subs = new QLabel;
    m_signals_count = new QLabel;
    m_tables = new QWidget; //(tr("Data Tables"));
    QHBoxLayout *group_layout = new QHBoxLayout;
    group_layout->addWidget(m_concentrations);
    group_layout->addWidget(m_signals);
    m_tables->setLayout(group_layout);
    
    layout->addLayout(hlayout, 0, 0, 1, 4);
    layout->addWidget(m_datapoints, 1, 0);
    layout->addWidget(m_substances, 1, 1);
    layout->addWidget(m_const_subs,1,2);
    layout->addWidget(m_signals_count, 1, 3);
    
    m_widget->setLayout(layout);
    QScrollArea *area = new QScrollArea;
    area->setWidgetResizable(true);
    area->setWidget(m_widget);
    m_splitter = new QSplitter;
    m_splitter->setOrientation(Qt::Vertical);
    m_splitter->addWidget(area);
    
    
    hlayout = new QHBoxLayout;
    hlayout->addWidget(m_splitter);
    setLayout(hlayout);
}

DataWidget::~DataWidget()
{
    QSettings settings;
    settings.beginGroup("overview");
    settings.setValue("splitterSizes", m_splitter->saveState());
}

void DataWidget::setData(QWeakPointer<DataClass> dataclass, QWeakPointer<ChartWrapper> wrapper)
{  
    m_data = dataclass;
    m_wrapper = wrapper; 
    dialog = new RegressionAnalysisDialog(m_data, m_wrapper, this);
    m_concentrations->setModel(m_data.data()->IndependentModel());
    m_signals->setModel(m_data.data()->DependentModel());
    connect(m_data.data()->DependentModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(HidePoint()));
    m_concentrations->resizeColumnsToContents();
    m_signals->resizeColumnsToContents();
    m_name->setText(qApp->instance()->property("projectname").toString());
    m_substances->setText(tr("Substances: %1").arg(m_data.data()->IndependentModel()->columnCount()));
    m_datapoints->setText(tr("Data Points: %1").arg(m_data.data()->DependentModel()->rowCount()));
    m_signals_count->setText(tr("Signals: %1").arg(m_data.data()->SeriesCount()));
    
    QVBoxLayout *vlayout = new QVBoxLayout;
    for(int i = 0; i < m_wrapper.data()->SeriesSize(); ++i)
    {
        QPointer<SignalElement > el = new SignalElement(m_data, m_wrapper, i, this);
        vlayout->addWidget(el);
        m_signal_elements << el;
    }
    layout->addLayout(vlayout, 2,0, 1, 4);
    
    QHBoxLayout *scaling_layout = new QHBoxLayout;
    scaling_layout->addWidget(new QLabel(tr("Scaling factors for input data:")));
    
    for(int i = 0; i < m_data.data()->getScaling().size(); ++i)
    {
        
        QDoubleSpinBox *spin_box = new QDoubleSpinBox;
        spin_box->setMaximum(1e8);
        spin_box->setMinimum(-1e8);
        spin_box->setValue(m_data.data()->getScaling()[i]);
        spin_box->setSingleStep(1e-2);
        spin_box->setDecimals(7);
        connect(spin_box, SIGNAL(valueChanged(double)), this, SLOT(setScaling()));
        m_scaling_boxes << spin_box;
        QHBoxLayout *lay = new QHBoxLayout;
        lay->addWidget(new QLabel(tr("%1. substance").arg(i + 1)));
        lay->addWidget(spin_box);
        scaling_layout->addLayout(lay);
    }
    layout->addLayout(scaling_layout,3,0,1,4);
    
    m_systemwidget = new QWidget;
    
    m_splitter->addWidget(m_tables);
    dialog = new RegressionAnalysisDialog(m_data, m_wrapper, this);
    dialog->UpdatePlots();
    
    QSettings settings;
    settings.beginGroup("overview");
    m_splitter->restoreState(settings.value("splitterSizes").toByteArray());
    m_switch->setVisible(m_data.data()->IndependentVariableSize() == 2);
    connect(m_data.data(), SIGNAL(SystemParameterLoaded()), this, SLOT(MakeSystemParameter()));
}

void DataWidget::switchHG()
{
    m_data.data()->SwitchConentrations();
    m_wrapper.data()->UpdateModel();
    emit recalculate();
}

void DataWidget::SetProjectName()
{
    qApp->instance()->setProperty("projectname", m_name->text());
    emit NameChanged();
}

void DataWidget::setEditable(bool editable)
{
    m_data.data()->DependentModel()->setEditable(editable);
}


void DataWidget::setScaling()
{
    QList<qreal> scaling;
    for(int i = 0; i < m_scaling_boxes.size(); ++i)
        scaling << m_scaling_boxes[i]->value();
    m_data.data()->setScaling(scaling);
    m_wrapper.data()->UpdateModel();
    emit recalculate();
}

void DataWidget::ShowContextMenu(const QPoint& pos)
{
    Q_UNUSED(pos)
    QModelIndex index = m_signals->currentIndex();
    int row = index.row();
    m_data.data()->DependentModel()->CheckRow(row);
    HidePoint();
    emit recalculate();
}

void DataWidget::HidePoint()
{
    m_wrapper.data()->stopAnimiation();
    m_wrapper.data()->UpdateModel();
    m_wrapper.data()->restartAnimation();
    dialog->UpdatePlots();
}

void DataWidget::MakeSystemParameter()
{
    if(m_system_parameter_loaded)
        return;
    m_splitter->addWidget(new SPOverview(m_data.data()));
    m_system_parameter_loaded = true;
}

void DataWidget::LinearAnalysis()
{
    dialog->show();
}


#include "datawidget.moc"

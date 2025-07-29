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

#include <charts.h>

#include "src/core/models/dataclass.h"

#include "src/ui/guitools/chartwrapper.h"

#include <QtCharts/QXYSeries>

#include <QtCore/QDebug>

#include <QtGui/QStandardItemModel>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

#include "signalelement.h"

SignalElement::SignalElement(QWeakPointer<DataClass> data, QWeakPointer<ChartWrapper> wrapper, int index, QWidget* parent)
    : QGroupBox(parent)
    , m_data(data)
    , m_wrapper(wrapper)
    , m_index(index)
{
    m_data_series = qobject_cast<ScatterSeries*>(m_wrapper.toStrongRef().data()->Series(m_index)); //DataMapper(m_index)->series());
    QGridLayout* layout = new QGridLayout;

    m_show = new QCheckBox;
    m_show->setText(tr("Show"));
    m_show->setChecked(true);
    connect(m_show, SIGNAL(stateChanged(int)), this, SLOT(ShowLine(int)));

    m_legend = new QCheckBox;
    m_legend->setText(tr("Show in Legend"));
    m_legend->setToolTip(tr("Show Line Series in legend"));
    m_legend->setChecked(false);
    connect(m_legend, &QCheckBox::stateChanged, this, [this](bool legend) {
        this->m_data_series->setShowInLegend(legend);
    });

    m_name = new QLineEdit;
    QString name;
    name = data.toStrongRef().data()->DependentModel()->headerData(m_index, Qt::Horizontal).toString();
    m_name->setPlaceholderText(name);
    m_data_series->setName(name);
    m_name->setMaximumWidth(300);
    connect(m_name, SIGNAL(textChanged(QString)), this, SLOT(setName(QString)));

    m_choose = new QPushButton(tr("Color"));
    m_choose->setFlat(true);
    m_choose->setMaximumSize(70, 30);
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
    connect(m_wrapper.toStrongRef().data(), SIGNAL(ShowSeries(int)), this, SLOT(UnCheckToggle(int)));

    layout->addWidget(m_name, 0, 0);
    layout->addWidget(m_show, 0, 1);
    layout->addWidget(m_choose, 0, 2);
    layout->addWidget(new QLabel(tr("Size")), 0, 3);
    layout->addWidget(m_markerSize, 0, 4);
    layout->addWidget(m_rectangle, 0, 5);
    layout->addWidget(m_toggle, 0, 6);
    layout->addWidget(m_legend, 0, 7);

    setLayout(layout);
    ColorChanged(m_wrapper.toStrongRef().data()->color(m_index));
    setMouseTracking(true);
}

SignalElement::~SignalElement()
{
}

void SignalElement::ColorChanged(const QColor& color)
{
    setStyleSheet("background-color:" + color.name() + ";");
    m_color = color;
}

void SignalElement::chooseColor()
{

    QColor color = QColorDialog::getColor(m_color, this, tr("Choose Color for Series"));
    if (!color.isValid())
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
    m_data_series->showLine(i);
}

void SignalElement::setName(const QString& str)
{
    m_data_series->setName(str);
    m_data.toStrongRef().data()->DependentModel()->setHeaderData(m_index, Qt::Horizontal, str, Qt::DisplayRole);
    emit m_data_series->nameChanged(str);
}

void SignalElement::HideSeries()
{
    if (m_series_hidden)
        m_data_series->setMarkerSize(m_markerSize->value());
    else
        m_data_series->setMarkerSize(0);
    m_series_hidden = !m_series_hidden;
}

void SignalElement::setMarkerSize(qreal value)
{
    m_data_series->setMarkerSize(value);
}

void SignalElement::setMarkerShape(int shape)
{
    if (shape)
        m_data_series->setMarkerShape(ScatterSeries::MarkerShapeRectangle);
    else
        m_data_series->setMarkerShape(ScatterSeries::MarkerShapeCircle);
}

void SignalElement::togglePlot()
{
    m_data_series->setVisible(true);
    if (m_toggle->isChecked())
        m_wrapper.toStrongRef().data()->showSeries(m_index);
    else
        m_wrapper.toStrongRef().data()->showSeries(-1);
}

void SignalElement::UnCheckToggle(int i)
{
    if (i != m_index)
        m_toggle->setChecked(false);
}

#pragma message("Restore or clean up before release")
/*
void SignalElement::enterEvent(QEnterEvent* event)
{
    m_data_series->setPointLabelsColor(m_data_series->color());
    for (int i = 0; i < m_data_series->points().size(); i++) {
        auto conf = m_data_series->pointConfiguration(i);
        conf[QXYSeries::PointConfiguration::Size] = m_markerSize->value() + qApp->instance()->property("MarkerPointFeedback").toDouble();
        if (qApp->instance()->property("ModuloPointFeedback").toInt())
            conf[QXYSeries::PointConfiguration::LabelVisibility] = (i % qApp->instance()->property("ModuloPointFeedback").toInt()) == 0 && qApp->instance()->property("PointFeedback").toBool();
        m_data_series->setPointConfiguration(i, conf);
    }
}

void SignalElement::leaveEvent(QEvent* event)
{
    QTimer::singleShot(500, this, [this]() {
        for (int i = 0; i < m_data_series->points().size(); i++) {
            auto conf = m_data_series->pointConfiguration(i);
            conf[QXYSeries::PointConfiguration::Size] = m_markerSize->value();
            conf[QXYSeries::PointConfiguration::LabelVisibility] = false;
            m_data_series->setPointConfiguration(i, conf);
        }
    });
}
*/
#include "signalelement.moc"

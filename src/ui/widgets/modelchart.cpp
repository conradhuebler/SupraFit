/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/AbstractModel.h"

#include "src/core/instance.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/guitools/guitools.h"

#include <QtCore/QWeakPointer>

#include <QtWidgets/QHBoxLayout>

#include <QtCharts/QXYSeries>

#include "modelchart.h"

ModelChartWidget::ModelChartWidget(const QWeakPointer<AbstractModel>& model, const QString& chart, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_chart(chart)
{
    setUI();
    connect(m_model.data(), &AbstractModel::ChartUpdated, this, [this](const QString& str) {
        if (str == m_chart)
            this->UpdateChart();
    });
}

ModelChartWidget::~ModelChartWidget()
{
    m_model.clear();
}

void ModelChartWidget::setUI()
{
    const ModelChart* chart = m_model.data()->Chart(m_chart);

    if (chart == NULL)
        return;

    QHBoxLayout* layout = new QHBoxLayout;
    setLayout(layout);

    view = new ListChart;
    connect(view, &ListChart::LastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });
    connect(Instance::GlobalInstance(), &Instance::ConfigurationChanged, view, &ListChart::ApplyConfigurationChange);

    view->setXAxis(chart->x_axis);
    view->setYAxis(chart->y_axis);
    view->setMinimumSize(300, 400);
    view->setName(m_chart);
    view->setTitle(QString("%1 for %2").arg(m_chart).arg(m_model.data()->Name()));
    layout->addWidget(view);

    for (int i = 0; i < chart->m_series.size(); ++i) {
        LineSeries* series = new LineSeries;
        series->setName(chart->m_series[i].name);
        series->append(chart->m_series[i].m_values);
        series->setColor(ChartWrapper::ColorCode(i));
        view->addSeries(series, i, ChartWrapper::ColorCode(i), chart->m_series[i].name);
        m_series << series;
    }
    UpdateChart();
}

void ModelChartWidget::UpdateChart()
{
    const ModelChart* chart = m_model.data()->Chart(m_chart);
    if (chart == NULL || view == NULL)
        return;

    view->setXAxis(chart->x_axis);
    view->setYAxis(chart->y_axis);

    if (m_series.size() != chart->m_series.size()) {
        view->Clear();
        m_series.clear();

        for (int i = 0; i < chart->m_series.size(); ++i) {
            LineSeries* series = new LineSeries;
            series->setName(chart->m_series[i].name);
            series->append(chart->m_series[i].m_values);
            view->addSeries(series, i, QColor("green"), chart->m_series[i].name);
            m_series << series;
        }
    } else {
        for (int i = 0; i < m_series.size(); ++i) {
            m_series[i]->clear();
            m_series[i]->append(chart->m_series[i].m_values);
        }
    }
    view->setTitle(QString("%1 for %2").arg(m_chart).arg(m_model.data()->Name()));
    view->Chart()->formatAxis();
}

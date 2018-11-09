/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/ui/guitools/chartwrapper.h"

#include "src/ui/widgets/listchart.h"

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
    QHBoxLayout* layout = new QHBoxLayout;
    setLayout(layout);

    view = new ListChart;
    view->setXAxis("parameter");
    view->setYAxis("relative rate");
    view->setMinimumSize(300, 400);

    layout->addWidget(view);

    const ModelChart* chart = m_model.data()->Chart(m_chart);
    if (chart == NULL)
        return;
    for (int i = 0; i < chart->m_series.size(); ++i) {
        LineSeries* series = new LineSeries;
        series->append(chart->m_series[i].m_values);
        view->addSeries(series, i, QColor("green"), chart->m_series[i].name);
        m_series << series;
    }
    UpdateChart();
}

void ModelChartWidget::UpdateChart()
{
    const ModelChart* chart = m_model.data()->Chart(m_chart);
    if (chart == NULL)
        return;

    for (int i = 0; i < m_series.size(); ++i) {
        m_series[i]->clear();
        m_series[i]->append(chart->m_series[i].m_values);
    }
}
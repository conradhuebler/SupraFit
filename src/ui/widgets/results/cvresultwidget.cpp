/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/capabilities/abstractsearchclass.h"
#include "src/capabilities/weakenedgridsearch.h"
#include "src/core/AbstractModel.h"
#include "src/core/toolset.h"

#include "src/ui/guitools/chartwrapper.h"

#include "src/ui/widgets/chartview.h"
#include "src/ui/widgets/statisticwidget.h"

#include <QtCore/QPointer>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include "cvresultwidget.h"


CVResultsWidget::CVResultsWidget(QSharedPointer<AbstractModel> model) //: m_model(model)
{
    m_model = model;
    setUi();
}

CVResultsWidget::~CVResultsWidget()
{

}

QWidget * CVResultsWidget::ChartWidget()
{
    QWidget *widget = new QWidget;
    QGridLayout *layout = new QGridLayout;

    widget->setLayout(layout);
    return widget;
}


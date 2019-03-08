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
 */

#include <QtCore/QDebug>
#include <QtCore/QPointer>

#include <QtCharts/QBoxPlotSeries>

#include <QtWidgets/QLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QWidget>

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/widgets/chartview.h"

#include "listchart.h"

ListChart::ListChart()
{
    m_chart = new QtCharts::QChart;

    m_chartview = new ChartView(m_chart, true);
    m_chartview->setYAxis("Y");
    m_chartview->setXAxis("X");

    m_list = new QListWidget;
    m_list->setMaximumWidth(200);

    m_names_list = new QListWidget;
    m_names_list->setMaximumWidth(200);

    QSplitter* list_splitter = new QSplitter(Qt::Vertical);
    list_splitter->addWidget(m_list);
    list_splitter->addWidget(m_names_list);
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(m_chartview);
    splitter->addWidget(list_splitter);
    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(splitter);
    setLayout(layout);

    connect(m_list, &QListWidget::itemDoubleClicked, this, &ListChart::SeriesListClicked);
    connect(m_names_list, &QListWidget::itemDoubleClicked, this, &ListChart::NamesListClicked);
}

ListChart::~ListChart()
{
}

void ListChart::setXAxis(const QString& str)
{
    m_chartview->setXAxis(str);
}

void ListChart::setYAxis(const QString& str)
{
    m_chartview->setYAxis(str);
}

void ListChart::addSeries(QtCharts::QAbstractSeries* series, int index, const QColor& color, const QString& name, bool callout)
{
    m_chartview->addSeries(series, callout);

    QListWidgetItem* item = NULL;
    if (index >= m_list->count()) {
        item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, index);
        if (qobject_cast<QtCharts::QXYSeries*>(series))
            item->setBackgroundColor(qobject_cast<QtCharts::QXYSeries*>(series)->color());
        else
            item->setBackgroundColor(color);
        m_list->addItem(item);
    }

    if (!m_names_list->findItems(name, Qt::MatchExactly).size()) {
        item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, name);
        m_names_list->addItem(item);
    }

    QtCharts::QXYSeries* s = qobject_cast<QtCharts::QXYSeries*>(series);
    if (s && item != NULL)
        connect(s, &QtCharts::QXYSeries::colorChanged, this, [item](const QColor& color) {
            item->setBackgroundColor(color);
        });

    m_list->setItemDelegate(new HTMLListItem(m_list));
    m_names_list->setItemDelegate(new HTMLListItem(m_names_list));
    m_hidden[index] = true;
    m_series.insert(index, series);
    m_chartview->formatAxis();
    if (m_list->count() == m_names_list->count())
        m_names_list->hide();
    else
        m_names_list->show();
}

QtCharts::QLineSeries* ListChart::addLinearSeries(qreal m, qreal n, qreal min, qreal max, int index)
{
    QtCharts::QLineSeries* serie = m_chartview->addLinearSeries(m, n, min, max);
    m_series.insert(index, serie);
    return serie;
}

void ListChart::setColor(int index, const QColor& color)
{
    if (index < m_list->count())
        m_list->item(index)->setBackgroundColor(color);
}

void ListChart::Clear()
{
    m_chartview->ClearChart();
    m_series.clear();
    m_list->clear();
    m_names_list->clear();
}

void ListChart::NamesListClicked(QListWidgetItem* item)
{
    QString str = item->data(Qt::UserRole).toString();
    QList<QListWidgetItem*> list = m_list->findItems(str, Qt::MatchExactly);
    for (int i = 0; i < list.size(); ++i)
        SeriesListClicked(list[i]);
}

void ListChart::HideSeries(int index)
{
    m_hidden[index] = !m_hidden[index];
    QList<QtCharts::QAbstractSeries*> series = m_series.values(index);
    for (int j = 0; j < series.size(); ++j) {
        if (qobject_cast<BoxPlotSeries*>(series[j])) // visibility doesnt work for boxplots ??
            qobject_cast<BoxPlotSeries*>(series[j])->setVisible(m_hidden[index]);
        else
            series[j]->setVisible(m_hidden[index]);
    }
}

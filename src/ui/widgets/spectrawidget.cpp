/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtCore/QVector>

#include <src/core/spectrahandler.h>

#include <src/ui/widgets/DropTable.h>

#include "spectrawidget.h"

SpectraWidget::SpectraWidget(QWidget* parent)
    : QWidget(parent)
{
    m_handler = new SpectraHandler;
    setUI();
}

void SpectraWidget::setUI()
{
    QGridLayout* layout = new QGridLayout;

    m_main_splitter = new QSplitter(Qt::Horizontal);
    m_list_splitter = new QSplitter(Qt::Vertical);

    m_files = new QListWidget;
    m_xvalues = new QListWidget;

    m_spectra_view = new ChartView;
    m_spectra_view->setAutoScaleStrategy(AutoScaleStrategy::QtNiceNumbers);
    m_spectra_view->setVerticalLineEnabled(true);

    m_datatable = new DropTable;

    m_views = new QTabWidget;
    m_views->addTab(m_spectra_view, tr("Spectra View"));
    m_views->addTab(m_datatable, tr("Compiled Input"));

    m_indep = new DropTable;
    m_indep->setMaximumWidth(300);
    m_list_splitter->addWidget(m_files);
    m_list_splitter->addWidget(m_xvalues);
    m_main_splitter->addWidget(m_list_splitter);
    m_files->setMaximumWidth(200);
    m_main_splitter->addWidget(m_views);
    m_main_splitter->addWidget(m_indep);

    layout->addWidget(m_main_splitter, 0, 0);

    setLayout(layout);

    connect(m_spectra_view, &ChartView::PointDoubleClicked, this, &SpectraWidget::PointDoubleClicked);
    connect(m_handler, &SpectraHandler::Updated, this, &SpectraWidget::UpdateData);
}

void SpectraWidget::addFile(const QString& file)
{
    m_handler->addSpectrum(file);
    UpdateSpectra();
}

void SpectraWidget::setDirectory(const QString& directry)
{
    m_handler->addDirectory(directry, "csv");
    UpdateSpectra();
}

void SpectraWidget::UpdateSpectra()
{
    m_spectra_view->ClearChart();
    m_files->clear();
    for (const auto& string : m_handler->getOrder()) {
        QList<QPointF> xy = m_handler->Data(string);
        LineSeries* series = new LineSeries;
        series->append(xy);
        m_spectra_view->addSeries(series, false);
        QPen pen = series->pen();
        pen.setWidth(1);
        series->setPen(pen);

        QListWidgetItem* item = new QListWidgetItem;
        item->setText(m_handler->Name(string));
        item->setData(Qt::UserRole, string);
        item->setData(Qt::UserRole + 1, m_handler->Path(string));
        item->setData(Qt::BackgroundRole, series->color());
        m_files->addItem(item);
    }
}

void SpectraWidget::PointDoubleClicked(const QPointF& point)
{
    m_xvalues->addItem(QString::number(point.x()));
    m_handler->addXValue(point.x());
    UpdateData();
}

void SpectraWidget::UpdateData()
{
    QPointer<DataTable> table = new DataTable(qobject_cast<DataTable*>(m_indep->model()));
    if (!table)
        return;
    if (!table->isValid())
        return;
    DataTable* data = qobject_cast<DataTable*>(m_handler->CompileSimpleTable());
    table->appendColumns(data);
    m_datatable->setModel(table);
    m_project = table->ExportTable(true);
    qDebug() << m_project;
}

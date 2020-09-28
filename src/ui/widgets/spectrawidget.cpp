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
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtCore/QVector>

#include <src/core/spectrahandler.h>

#include <src/ui/widgets/DropTable.h>

#include "src/ui/guitools/guitools.h"

#include "src/global.h"

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
    m_files->setContextMenuPolicy(Qt::ActionsContextMenu);

    QAction* action = new QAction("Remove spectra");
    action->setIcon(Icon("list-remove"));
    connect(action, &QAction::triggered, m_files, [this]() {
        auto avl = m_xvalues->currentItem();
        if (!avl)
            return;
        qDebug() << avl->data(Qt::DisplayRole);
        m_files->removeItemWidget(avl);
    });
    m_files->addAction(action);

    m_add_xvalue = new QLineEdit;
    m_add_xvalue->setPlaceholderText(tr("Enter x value!"));

    m_accept_x = new QPushButton(tr("Add value"));
    m_accept_x->setFlat(true);

    m_xvalues = new QListWidget;
    QGridLayout* xlayout = new QGridLayout;
    xlayout->addWidget(m_add_xvalue, 0, 0);
    xlayout->addWidget(m_accept_x, 0, 1);
    xlayout->addWidget(m_xvalues, 1, 0, 1, 2);
    QWidget* xwidget = new QWidget;
    xwidget->setLayout(xlayout);
    m_xvalues->setContextMenuPolicy(Qt::ActionsContextMenu);

    action = new QAction("Remove wavelength");
    action->setIcon(Icon("list-remove"));
    connect(action, &QAction::triggered, m_xvalues, [this]() {
        auto avl = m_xvalues->currentItem();
        if (!avl)
            return;
        qDebug() << avl->data(Qt::DisplayRole);
        m_xvalues->removeItemWidget(avl);
    });
    m_xvalues->addAction(action);

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
    m_list_splitter->addWidget(xwidget);
    m_main_splitter->addWidget(m_list_splitter);
    m_files->setMaximumWidth(200);
    m_main_splitter->addWidget(m_views);
    m_main_splitter->addWidget(m_indep);

    layout->addWidget(m_main_splitter, 0, 0);

    setLayout(layout);

    connect(m_spectra_view, &ChartView::PointDoubleClicked, this, &SpectraWidget::PointDoubleClicked);
    connect(m_handler, &SpectraHandler::Updated, this, &SpectraWidget::UpdateData);

    connect(m_accept_x, &QPushButton::clicked, this, [this]() {
        m_xvalues->addItem(m_add_xvalue->text());
        m_handler->addXValue(m_add_xvalue->text().toDouble());
        UpdateData();
        m_add_xvalue->clear();
    });
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
    // m_handler->PCA();
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
    QPointer<DataTable> table = qobject_cast<DataTable*>(m_indep->model());
    if (!table)
        return;
    if (!table->isValid())
        return;
    DataTable* result = new DataTable(table);
    DataTable* data = qobject_cast<DataTable*>(m_handler->CompileSimpleTable());
    result->appendColumns(data);
    m_datatable->setModel(result);
    m_input_table = result->ExportTable(true);
    m_project = m_handler->getSpectraData();
}

void SpectraWidget::setData(const QJsonObject& data)
{
    m_indep->setModel(new DataTable(data["independent"].toObject()));
    m_handler->LoadData(data["raw"].toObject());
    UpdateSpectra();
    for (auto d : m_handler->XValues())
        m_xvalues->addItem(QString::number(d));
    UpdateData();
}

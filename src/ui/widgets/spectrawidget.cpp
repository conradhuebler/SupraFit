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

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

#include <QtCore/QDir>
#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtCore/QVector>

#include <src/core/spectrahandler.h>

#include <src/ui/widgets/DropTable.h>

#include "src/ui/guitools/guitools.h"
#include "src/ui/guitools/waiter.h"

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
        auto avl = m_files->currentRow();
        if (avl < m_files->count() && avl >= 0) {
            m_files->takeItem(avl);
            m_handler->clearFiles();
            QStringList files;
            for (int i = 0; i < m_files->count(); ++i)
                files << m_files->item(i)->data(Qt::UserRole + 1).toString() + QDir::separator() + m_files->item(i)->data(Qt::DisplayRole).toString();

            for (const auto& file : files)
                addFile(file);
        }
    });
    m_files->addAction(action);

    m_add_xvalue = new QLineEdit;
    m_add_xvalue->setPlaceholderText(tr("Enter x value!"));

    m_accept_x = new QPushButton(tr("Add value"));
    m_accept_x->setFlat(true);

    m_xvalues = new QListWidget;

    m_cluster = new QCheckBox;
    m_cluster->setText(tr("Cluster Results"));
    m_cluster->setChecked(true);

    m_averaged = new QCheckBox;
    m_averaged->setText("Cluster Average");
    m_averaged->setChecked(false);

    m_values = new QSpinBox;
    m_values->setMinimum(1);
    m_values->setMaximum(100);
    m_values->setValue(5);
    m_varcovar = new QPushButton(tr("VarCovar"));

    QGridLayout* xlayout = new QGridLayout;
    xlayout->addWidget(m_add_xvalue, 0, 0);
    xlayout->addWidget(m_accept_x, 0, 1);
    xlayout->addWidget(m_xvalues, 1, 0, 1, 2);
    //xlayout->addWidget(m_values, 2, 0);
    //xlayout->addWidget(m_varcovar, 2, 1);
    QWidget* xwidget = new QWidget;
    xwidget->setLayout(xlayout);
    m_xvalues->setContextMenuPolicy(Qt::ActionsContextMenu);

    action = new QAction("Remove wavelength");
    action->setIcon(Icon("list-remove"));
    connect(action, &QAction::triggered, m_xvalues, [this]() {
        auto avl = m_xvalues->currentRow();
        if (avl < m_xvalues->count() && avl >= 0) {
            m_xvalues->takeItem(avl);
            UpdateXValues();
            UpdateVerticaLines();
        }
    });
    m_xvalues->addAction(action);

    action = new QAction("Clear List");
    action->setIcon(Icon("list-remove"));
    connect(action, &QAction::triggered, m_xvalues, [this]() {
        m_xvalues->clear();
        m_handler->setXValues(QVector<double>());
        UpdateVerticaLines();
    });
    m_xvalues->addAction(action);

    QWidget* chart_viewport = new QWidget;
    QGridLayout* chart_layout = new QGridLayout;
    chart_viewport->setLayout(chart_layout);

    m_x_start = new QDoubleSpinBox;
    m_x_end = new QDoubleSpinBox;
    chart_layout->addWidget(new QLabel(tr("Start")), 0, 0);
    chart_layout->addWidget(m_x_start, 0, 1);
    chart_layout->addWidget(new QLabel(tr("Ende")), 0, 2);
    chart_layout->addWidget(m_x_end, 0, 3);
    chart_layout->addWidget(new QLabel(tr("# X Values")), 0, 4);
    chart_layout->addWidget(m_values, 0, 5);
    chart_layout->addWidget(m_cluster, 0, 6);
    chart_layout->addWidget(m_averaged, 0, 7);
    chart_layout->addWidget(m_varcovar, 0, 8);
    m_spectra_view = new ChartView;
    m_spectra_view->setAutoScaleStrategy(AutoScaleStrategy::QtNiceNumbers);
    m_spectra_view->setVerticalLineEnabled(true);
    //m_spectra_view->setZoomStrategy(ZoomStrategy::Z_Horizontal);
    m_spectra_view->setSelectStrategy(SelectStrategy::S_Horizontal);

    m_spectra_view->PrivateView()->setVerticalLinePrec(0);
    m_spectra_view->PrivateView()->setVerticalLinesPrec(-1);

    chart_layout->addWidget(m_spectra_view, 1, 0, 1, 9);

    m_datatable = new DropTable;

    QWidget* table_viewport = new QWidget;
    QGridLayout* table_layout = new QGridLayout;

    m_export_table = new QPushButton(tr("Export table to file"));
    table_layout->addWidget(m_export_table, 0, 4);
    table_layout->addWidget(m_datatable, 1, 0, 1, 5);

    table_viewport->setLayout(table_layout);

    m_views = new QTabWidget;
    m_views->addTab(chart_viewport, tr("Spectra View"));
    m_views->addTab(table_viewport, tr("Data View"));

    m_indep = new DropTable;
    m_indep->setMaximumWidth(300);

    QWidget* indep = new QWidget;
    QVBoxLayout* vlayout = new QVBoxLayout;
    vlayout->addWidget(new QLabel("Drop table with independent data"));
    vlayout->addWidget(m_indep);
    indep->setLayout(vlayout);

    m_list_splitter->addWidget(m_files);
    m_list_splitter->addWidget(xwidget);
    m_main_splitter->addWidget(m_list_splitter);
    m_files->setMaximumWidth(200);
    m_main_splitter->addWidget(m_views);
    m_main_splitter->addWidget(indep);

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

    connect(m_varcovar, &QPushButton::clicked, this, [this]() {
        Waiter wait;
        m_handler->setXRange(m_x_start->value(), m_x_end->value());
        m_xvalues->clear();
        m_handler->VarCovarSelect(m_values->value(), m_cluster->isChecked(), m_averaged->isChecked());
        UpdateVerticaLines();
        for (auto d : m_handler->XValues())
            m_xvalues->addItem(QString::number(d));
        UpdateData();
    });

    connect(m_spectra_view, &ChartView::AddRect, this, &SpectraWidget::UpdateXRange);

    connect(m_export_table, &QPushButton::clicked, this, &SpectraWidget::SaveToFile);
}

void SpectraWidget::addFile(const QString& file)
{
    m_handler->addSpectrum(file);
    UpdateSpectra();
}

void SpectraWidget::setDirectory(const QString& directry, const QString& type)
{
    m_handler->addDirectory(directry, type);
    UpdateSpectra();
    //m_handler->PCA();
}

void SpectraWidget::clear()
{
    m_handler->clearFiles();
    m_spectra_view->ClearChart();
    m_files->clear();
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
    m_spectra_view->setXMin(m_handler->XMin());
    m_spectra_view->setXMax(m_handler->XMax());

    m_x_start->setMinimum(m_handler->XMin());
    m_x_start->setMaximum(m_handler->XMax());

    m_x_end->setMinimum(m_handler->XMin());
    m_x_end->setMaximum(m_handler->XMax());

    m_x_start->setValue(m_handler->XMin());
    m_x_end->setValue(m_handler->XMax());
}

void SpectraWidget::PointDoubleClicked(const QPointF& point)
{
    m_xvalues->addItem(QString::number(point.x()));
    m_handler->addXValue(point.x());
    UpdateData();
    UpdateVerticaLines();
}

void SpectraWidget::UpdateData()
{
    DataTable* data = qobject_cast<DataTable*>(m_handler->CompileSimpleTable());
    m_input_table = data->ExportTable(true);
    m_datatable->setModel(data);
    QPointer<DataTable> table = qobject_cast<DataTable*>(m_indep->model());
    if (!table)
        return;
    if (!table->isValid() || table->columnCount() == 0)
        return;
    DataTable* result = new DataTable(table);
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
    UpdateVerticaLines();
    for (auto d : m_handler->XValues())
        m_xvalues->addItem(QString::number(d));
    UpdateData();
}

void SpectraWidget::UpdateXValues()
{
    QVector<double> list;
    for (int i = 0; i < m_xvalues->count(); ++i)
        list << m_xvalues->item(i)->data(Qt::DisplayRole).toDouble();
    m_handler->setXValues(list);
}

void SpectraWidget::UpdateVerticaLines()
{
    m_spectra_view->PrivateView()->removeAllVerticalLines();
    for (auto d : m_handler->XValues())
        m_spectra_view->PrivateView()->addVerticalLine(d);
}

void SpectraWidget::UpdateXRange(const QPointF& point1, const QPointF& point2)
{
    m_x_start->setValue(point1.x());
    m_x_end->setValue(point2.x());
}

void SpectraWidget::SaveToFile()
{
    const QString content = qobject_cast<DataTable*>(m_datatable->model())->ExportAsString();
    const QString filename = QFileDialog::getSaveFileName(this, "Export Table", getDir());

    if (filename.isEmpty())
        return;

    setLastDir(filename);

    QFile file(filename);
    if (!file.open(QIODevice::ReadWrite))
        return;

    QTextStream stream(&file);
    stream << content;
}

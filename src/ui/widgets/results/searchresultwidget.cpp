/*
 * <one line to give the library's name and an idea of what it does.>
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

#include "src/capabilities/globalsearch.h"
#include "src/core/jsonhandler.h"
#include "src/core/models.h"
#include "src/core/toolset.h"

#include "src/ui/widgets/buttons/scientificbox.h"
#include "src/ui/widgets/chartview.h"
#include "src/ui/widgets/results/contourwidget.h"

#include <QtCharts/QChart>
#include <QtCharts/QScatterSeries>

#include <QtCore/QCollator>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QSortFilterProxyModel>

#include <QtGui/QStandardItemModel>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableView>
#include <QtWidgets/QWidget>

#include "searchresultwidget.h"

SearchResultWidget::SearchResultWidget(const QJsonObject& results, const QSharedPointer<AbstractModel> model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_results(results)
{
    m_central_widget = new QTabWidget;

    QGridLayout* layout = new QGridLayout;
    m_export = new QPushButton(tr("Export Models"));

    m_valid = new QCheckBox(tr("Only Valid Models"));
    m_valid->setChecked(true);
    connect(m_valid, &QCheckBox::stateChanged, this, &SearchResultWidget::ApplyFilter);

    m_converged = new QCheckBox(tr("Only Converged Models"));
    m_converged->setChecked(true);
    connect(m_converged, &QCheckBox::stateChanged, this, &SearchResultWidget::ApplyFilter);

    m_threshold = new ScientificBox;
    m_threshold->setValue(1);
    //connect(m_threshold, &ScientificBox::valueChanged, this, &SearchResultWidget::ApplyFilter);

    layout->addWidget(new QLabel(tr("Threshold SSE")), 0, 0);
    layout->addWidget(m_threshold, 0, 1);
    layout->addWidget(m_valid, 0, 2);
    layout->addWidget(m_converged, 0, 3);
    layout->addWidget(m_export, 0, 4);

    if (!m_model)
        throw 1;

    m_contour = BuildContour();
    connect(m_contour, &ContourWidget::ModelClicked, this, &SearchResultWidget::ModelClicked);
    m_table = BuildList();
    m_table->setSortingEnabled(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);

    m_contour->setData(m_models, m_model);

    connect(m_table, SIGNAL(clicked(QModelIndex)), this, SLOT(rowSelected(QModelIndex)));
    connect(m_table, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowContextMenu(const QPoint&)));
    connect(m_export, SIGNAL(clicked()), this, SLOT(ExportModels()));
    layout->addWidget(m_central_widget, 3, 0, 1, 5);

    m_central_widget->addTab(m_table, tr("Result List"));
    m_central_widget->addTab(m_contour, tr("Contour Plot"));

    setLayout(layout);
}

SearchResultWidget::~SearchResultWidget()
{
}

QTableView* SearchResultWidget::BuildList()
{
    QTableView* table = new QTableView(this);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    QStandardItemModel* model = new QStandardItemModel;

    QStringList header = QStringList() << "Sum of Squares";
    int size_optimised;
    int size = m_results["controller"].toObject()["size"].toInt();
    for (int i = 0; i < size; ++i) {
        QJsonObject local_model = m_results[QString::number(i)].toObject();
        double error = local_model["SSE"].toDouble();

        QStandardItem* item = new QStandardItem(QString::number(error));
        item->setData(i, Qt::UserRole);
        item->setData(error, Qt::UserRole + 1);
        item->setData(QString::number(local_model["converged"].toBool()), Qt::UserRole + 2);
        item->setData(QString::number(local_model["valid"].toBool()), Qt::UserRole + 3);
        item->setData(QString::number(local_model["valid"].toBool() && local_model["converged"].toBool()), Qt::UserRole + 4);
        item->setData(1, Qt::UserRole + 5);
        QBrush brush;
        if (local_model["valid"].toBool() && local_model["converged"].toBool()) {
            brush.setColor(Qt::green);
        }
        item->setData(brush, Qt::BackgroundRole);

        model->setItem(i, 0, item);
        int j = 1;
        QVector<qreal> initial = ToolSet::String2DoubleVec(local_model["initial"].toString());
        QVector<qreal> optimised = ToolSet::String2DoubleVec(local_model["optimised"].toString());
        size_optimised = optimised.size();
        for (int l = 0; l < m_model->GlobalParameterSize(); ++l) {
            QStandardItem* item = new QStandardItem(QString::number(initial[l]));
            item->setData(i, Qt::UserRole);
            item->setData(initial[l], Qt::UserRole + 1);
            item->setData(brush, Qt::BackgroundRole);
            model->setItem(i, j, item);
            j++;
        }
        if (!m_model->SupportSeries()) {
            for (int l = m_model->GlobalParameterSize(); l < initial.size(); ++l) {
                if (!m_model->LocalEnabled(l - m_model->GlobalParameterSize()))
                    continue;
                QStandardItem* item = new QStandardItem(QString::number(initial[l]));
                item->setData(i, Qt::UserRole);
                item->setData(initial[l], Qt::UserRole + 1);
                item->setData(brush, Qt::BackgroundRole);
                model->setItem(i, j, item);
                j++;
            }
        }
        for (int l = 0; l < m_model->GlobalParameterSize(); ++l) {
            QStandardItem* item = new QStandardItem(QString::number(optimised[l]));
            item->setData(i, Qt::UserRole);
            item->setData(optimised[l], Qt::UserRole + 1);
            item->setData(brush, Qt::BackgroundRole);
            model->setItem(i, j, item);
            j++;
        }
        if (!m_model->SupportSeries()) {
            for (int l = m_model->GlobalParameterSize(); l < optimised.size(); ++l) {
                if (!m_model->LocalEnabled(l - m_model->GlobalParameterSize()))
                    continue;
                QStandardItem* item = new QStandardItem(QString::number(optimised[l]));
                item->setData(i, Qt::UserRole);
                item->setData(optimised[l], Qt::UserRole + 1);
                item->setData(brush, Qt::BackgroundRole);
                model->setItem(i, j, item);
                j++;
            }
        }
        m_models << local_model["model"].toObject();
    }

    QStringList head;
    for (int i = 0; i < m_model.data()->GlobalParameterSize(); ++i)
        head << m_model.data()->GlobalParameterName(i);

    if (!m_model->SupportSeries()) {
        for (int l = m_model->GlobalParameterSize(); l < size_optimised; ++l) {
            if (m_model->LocalEnabled(l - m_model->GlobalParameterSize()))
                head << m_model->LocalParameterName(l - m_model->GlobalParameterSize());
        }
    }
    header << head << head;
    model->setHorizontalHeaderLabels(header);

    m_proxyModel = new QSortFilterProxyModel(this);
    table->setModel(m_proxyModel);
    table->resizeColumnsToContents();

    m_proxyModel->setSourceModel(model);
    m_proxyModel->setSortRole(Qt::UserRole + 1);

    ApplyFilter();

    resize(table->sizeHint());
    return table;
}

void SearchResultWidget::ModelClicked(int model)
{
    if (model < m_models.size())
        emit LoadModel(m_models[model]);
}

ContourWidget* SearchResultWidget::BuildContour()
{
    ContourWidget* widget = new ContourWidget();
    return widget;
}

void SearchResultWidget::rowSelected(const QModelIndex& index)
{
    int i = index.data(Qt::UserRole).toInt();
    QJsonObject model = m_results[QString::number(i)].toObject()["model"].toObject();
    emit LoadModel(model);
}

void SearchResultWidget::ShowContextMenu(const QPoint& pos)
{
    Q_UNUSED(pos)
    QModelIndex index = m_table->currentIndex();
    int i = index.data(Qt::UserRole).toInt();
    QJsonObject model = m_results[QString::number(i)].toObject()["model"].toObject();
    emit AddModel(model);
}

void SearchResultWidget::ExportModels()
{
    /*
    qreal threshold = m_threshold->value();
    bool allow_invalid = m_valid->isChecked();
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("Json File (*.json);;Binary (*.jdat);;All files (*.*)"));
    if (str.isEmpty())
        return;
    setLastDir(str);
    */
    //m_globalsearch->ExportResults(str, threshold, allow_invalid);
}

void SearchResultWidget::SwitchView()
{
    bool histogram = m_table->isHidden();
    m_table->setHidden(!histogram);
    m_contour->setHidden(histogram);
}

void SearchResultWidget::ApplyFilter()
{
    m_proxyModel->setFilterRegExp(QRegExp("1"));
    m_proxyModel->setFilterKeyColumn(0);

    if (m_valid->isChecked() && !m_converged->isChecked()) {
        m_proxyModel->setFilterRole(Qt::UserRole + 3);
    } else if (!m_valid->isChecked() && m_converged->isChecked()) {
        m_proxyModel->setFilterRole(Qt::UserRole + 2);
    } else if (m_valid->isChecked() && m_converged->isChecked()) {
        m_proxyModel->setFilterRole(Qt::UserRole + 4);
    } else {
        m_proxyModel->setFilterRole(Qt::UserRole + 5);
    }

    m_contour->setConverged(m_converged->isChecked());
    m_contour->setValid(m_valid->isChecked());
}

#include "searchresultwidget.h"

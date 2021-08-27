/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/capabilities/globalsearch.h"
#include "src/core/jsonhandler.h"
#include "src/core/toolset.h"

#include "src/core/models/models.h"

#include "src/ui/guitools/guitools.h"

#include "src/ui/widgets/buttons/scientificbox.h"
#include "src/ui/widgets/results/scatterwidget.h"

#include <QtCharts/QChart>
#include <QtCharts/QScatterSeries>

#include <QtCore/QCollator>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QSortFilterProxyModel>

#include <QtGui/QStandardItemModel>

#include <QtWidgets/QAction>
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

    //layout->addWidget(new QLabel(tr("Threshold SSE")), 0, 0);
    //layout->addWidget(m_threshold, 0, 1);
    layout->addWidget(m_valid, 0, 2);
    layout->addWidget(m_converged, 0, 3);
    //layout->addWidget(m_export, 0, 4);

    if (!m_model)
        throw 1;

    m_contour = BuildScatter();
    connect(m_contour, &ScatterWidget::ModelClicked, this, &SearchResultWidget::ModelClicked);
    m_table = BuildList();
    m_table->setSortingEnabled(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setContextMenuPolicy(Qt::ActionsContextMenu);

    m_contour->setData(m_models, m_model);

    connect(m_table, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(rowSelected(QModelIndex)));
    connect(m_export, SIGNAL(clicked()), this, SLOT(ExportModels()));
    layout->addWidget(m_central_widget, 3, 0, 1, 5);

    m_central_widget->addTab(m_table, tr("Result List"));
    m_central_widget->addTab(m_contour, tr("Scatter Plot"));

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
    table->setHorizontalHeader(new HeaderView(Qt::Horizontal, table));
    QAction* action;
    action = new QAction("Replace Parameter", table);
    action->setToolTip(tr("Current model parameter will be overwritten!"));
    action->setIcon(QIcon(":/icons/dialog-ok-red.png"));
    table->addAction(action);
    connect(action, &QAction::triggered, action, [this]() {
        this->rowSelected(m_table->currentIndex());
    });

    action = new QAction("Load Parameter", table);
    action->setToolTip(tr("Current model parameter will be untouched! A new model will be added to the project."));
    action->setIcon(Icon("list-add"));
    table->addAction(action);
    connect(action, &QAction::triggered, action, [this]() {
        QModelIndex index = m_table->currentIndex();
        int i = index.data(Qt::UserRole).toInt();
        QJsonObject model = this->m_results[QString::number(i)].toObject()["model"].toObject();
        emit this->AddModel(model);
    });

    QStandardItemModel* model = new QStandardItemModel;

    QStringList header = QStringList() << "SSE";
    int size = m_results["controller"].toObject()["size"].toInt();
    for (int i = 0; i < size; ++i) {

        QJsonObject local_model = m_results[QString::number(i)].toObject();

        QVector<qreal> initial = ToolSet::String2DoubleVec(local_model["initial"].toString());
        QVector<qreal> optimised = ToolSet::String2DoubleVec(local_model["optimised"].toString());
        local_model = m_results[QString::number(i)].toObject()["model"].toObject();

        double error = local_model["SSE"].toDouble();

        QStandardItem* item = new QStandardItem(QString::number(error));
        item->setData(i, Qt::UserRole);
        item->setData(error, Qt::UserRole + 1);
        item->setData(QString::number(local_model["converged"].toBool()), Qt::UserRole + 2);
        item->setData(QString::number(local_model["valid"].toBool()), Qt::UserRole + 3);
        item->setData(QString::number(local_model["valid"].toBool() && local_model["converged"].toBool()), Qt::UserRole + 4);
        item->setData(1, Qt::UserRole + 5);
        QColor color;
        if (local_model["valid"].toBool() && local_model["converged"].toBool())
            color = Qt::green;
        else if (local_model["valid"].toBool() && !local_model["converged"].toBool())
            color = Qt::yellow;
        else
            color = Qt::red;

        color = color.lighter();

        if (!qApp->instance()->property("ColorFullSearch").toBool())
            color = Qt::white;

        item->setData(color, Qt::BackgroundRole);

        model->setItem(i, 0, item);
        int j = 1;

        for (int l = 0; l < m_model.toStrongRef().data()->GlobalParameterSize(); ++l) {
            if (!m_model.toStrongRef().data()->GlobalTable()->isChecked(l, 0) || !m_model.toStrongRef().data()->GlobalEnabled(l))
                continue;
            QStandardItem* item = new QStandardItem(QString::number(initial[l]));
            item->setData(i, Qt::UserRole);
            item->setData(initial[l], Qt::UserRole + 1);
            item->setData(color, Qt::BackgroundRole);
            model->setItem(i, j, item);
            j++;
            item = new QStandardItem(QString::number(optimised[l]));
            item->setData(i, Qt::UserRole);
            item->setData(optimised[l], Qt::UserRole + 1);
            item->setBackground(color);
            model->setItem(i, j, item);
            j++;
        }

        int index = m_model.toStrongRef().data()->GlobalParameterSize();
        int idx = 0;
        for (int k = 0; k < m_model.toStrongRef().data()->SeriesCount(); ++k) {
            for (int l = 0; l < m_model.toStrongRef().data()->LocalParameterSize(); ++l) {
                if (!m_model.toStrongRef().data()->LocalTable()->isChecked(l, k) || !m_model.toStrongRef().data()->LocalEnabled(l)) {
                    index++;
                    idx++;
                    continue;
                }
                QStandardItem* item = new QStandardItem(QString::number(initial[index]));
                item->setData(i, Qt::UserRole);
                item->setData(initial[index], Qt::UserRole + 1);
                item->setBackground(color);
                model->setItem(i, j, item);

                j++;

                item = new QStandardItem(QString::number(optimised[index]));
                item->setData(i, Qt::UserRole);
                item->setData(optimised[index], Qt::UserRole + 1);
                item->setBackground(color);
                model->setItem(i, j, item);

                index++;
                idx++;
                j++;
            }
        }
        m_models << local_model; //["model"].toObject();
    }

    for (int i = 0; i < m_model.toStrongRef().data()->GlobalParameterSize(); ++i) {
        if (!m_model.toStrongRef().data()->GlobalTable()->isChecked(i, 0) || !m_model.toStrongRef().data()->GlobalEnabled(i))
            continue;
        header << tr("%1\n (before)").arg(m_model.toStrongRef().data()->GlobalParameterName(i));
        header << tr("%1\n (after)").arg(m_model.toStrongRef().data()->GlobalParameterName(i));
    }

    QString series;
    for (int k = 0; k < m_model.toStrongRef().data()->SeriesCount(); ++k) {
        if (m_model.toStrongRef().data()->SupportSeries())
            series = tr("Series %1, ").arg(k + 1);
        else
            series = QString();

        for (int l = 0; l < m_model.toStrongRef().data()->LocalParameterSize(); ++l) {
            if (!m_model.toStrongRef().data()->LocalTable()->isChecked(l, k) || !m_model.toStrongRef().data()->LocalEnabled(l))
                continue;
            header << tr("%1 %2 \n (before)").arg(series).arg(m_model.toStrongRef().data()->LocalParameterName(l));
            header << tr("%1 %2 \n (after)").arg(series).arg(m_model.toStrongRef().data()->LocalParameterName(l));
        }
    }

    model->setHorizontalHeaderLabels(header);

    m_proxyModel = new QSortFilterProxyModel(this);
    table->setModel(m_proxyModel);

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

ScatterWidget* SearchResultWidget::BuildScatter()
{
    ScatterWidget* widget = new ScatterWidget();
    return widget;
}

void SearchResultWidget::rowSelected(const QModelIndex& index)
{
    int i = index.data(Qt::UserRole).toInt();
    QJsonObject model = m_results[QString::number(i)].toObject()["model"].toObject();
    emit LoadModel(model);
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

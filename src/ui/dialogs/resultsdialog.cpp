/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QWeakPointer>

#include <QtGui/QStandardItemModel>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QListView>

#include <QtWidgets/QSplitter>

#include "src/global.h"

#include "src/core/models.h"

#include "src/ui/guitools/chartwrapper.h"

#include "src/ui/widgets/results/resultswidget.h"

#include "resultsdialog.h"

ResultsDialog::ResultsDialog(QSharedPointer<AbstractModel> model, ChartWrapper* wrapper, QWidget* parent)
    : QDialog(parent)
    , m_model(model)
    , m_wrapper(wrapper)
{
    m_layout = new QGridLayout;
    setModal(false);
    m_mainwidget = new QSplitter(Qt::Horizontal);
    m_layout->addWidget(m_mainwidget, 0, 0);

    m_results = new QListView;
    m_results->setMaximumWidth(200);
    m_results->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_results, &QListView::doubleClicked, this, &ResultsDialog::itemDoubleClicked);

    m_tabs = new QTabWidget;
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);

    m_mainwidget->addWidget(m_results);
    m_mainwidget->addWidget(m_tabs);

    setWindowTitle("Collected Results for " + m_model.data()->Name());
    setLayout(m_layout);
    resize(1024, 800);
    UpdateList();
    connect(m_model.data(), &AbstractModel::StatisticChanged, this, &ResultsDialog::UpdateList);
}

void ResultsDialog::Attention()
{
    show();
    raise();
    activateWindow();
}

void ResultsDialog::ShowResult(SupraFit::Statistic type, int index)
{
    int tab = m_tabs->addTab(new ResultsWidget(m_model.data()->getStatistic(type, index), m_model, m_wrapper), SupraFit::Statistic2Name(type));
    m_tabs->setCurrentIndex(tab);
    m_indices[Index(type, index)] = tab;
}

void ResultsDialog::UpdateList()
{
    auto makeItem = [this](QStandardItem* item) {
        const QString index = Index(item);
        if (m_indices.keys().contains(index))
            item->setData(m_indices[index], Qt::UserRole + 2);
        else {
            item->setData(-1, Qt::UserRole + 2);
            m_indices[index] = -1;
        }
    };

    QStandardItemModel* model = new QStandardItemModel;
    for (int i = 0; i < m_model.data()->getMCStatisticResult(); ++i) {
        QJsonObject controller = m_model.data()->getStatistic(SupraFit::Statistic::MonteCarlo, i)["controller"].toObject();

        QStandardItem* item = new QStandardItem(SupraFit::Statistic2Name(controller["method"].toInt()));
        item->setData(SupraFit::Statistic2Name(controller["method"].toInt()), Qt::DisplayRole);
        item->setData(controller["method"].toInt(), Qt::UserRole);
        item->setData(i, Qt::UserRole + 1);
        makeItem(item);
        model->appendRow(item);
    }

    for (int i = 0; i < m_model.data()->getWGStatisticResult(); ++i) {
        QStandardItem* item = new QStandardItem(tr("Weakend Grid Search"));
        item->setData(tr("Weakend Grid Search"), Qt::DisplayRole);
        item->setData(SupraFit::Statistic::WeakenedGridSearch, Qt::UserRole);
        item->setData(i, Qt::UserRole + 1);
        makeItem(item);
        model->appendRow(item);
    }

    for (int i = 0; i < m_model.data()->getMoCoStatisticResult(); ++i) {
        QStandardItem* item = new QStandardItem(tr("Model Comparison"));
        item->setData(tr("Model Comparison"), Qt::DisplayRole);
        item->setData(SupraFit::Statistic::ModelComparison, Qt::UserRole);
        item->setData(i, Qt::UserRole + 1);
        makeItem(item);
        model->appendRow(item);
    }

    QJsonObject statistic = m_model.data()->getReduction();
    if (!statistic.isEmpty()) {
        QStandardItem* item = new QStandardItem(tr("Reduction Analysis"));
        item->setData(tr("Reduction Analysis"), Qt::DisplayRole);
        item->setData(SupraFit::Statistic::Reduction, Qt::UserRole);
        item->setData(0, Qt::UserRole + 1);
        makeItem(item);
        model->appendRow(item);
    }
    m_itemmodel = model;
    m_results->setModel(m_itemmodel);
}

void ResultsDialog::itemDoubleClicked(const QModelIndex& index)
{
    QStandardItem* item = m_itemmodel->itemFromIndex(index);

    if (-1 == m_indices[Index(item)]) {
        SupraFit::Statistic type = SupraFit::Statistic(item->data(Qt::UserRole).toInt());
        int index = item->data(Qt::UserRole + 1).toInt();
        int tab = m_tabs->addTab(new ResultsWidget(m_model.data()->getStatistic(type, index), m_model, m_wrapper), SupraFit::Statistic2Name(type));
        item->setData(tab, Qt::UserRole + 2);
        m_indices[Index(item)] = tab;
    } else
        m_tabs->setCurrentIndex(m_indices[Index(item)]);
}

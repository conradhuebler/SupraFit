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

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QListWidget>
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
    m_results = new QListWidget;
    m_results->setMaximumWidth(200);
    connect(m_results, &QListWidget::itemDoubleClicked, this, &ResultsDialog::itemDoubleClicked);

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

void ResultsDialog::ShowResult(SupraFit::Statistic type)
{
}

void ResultsDialog::UpdateList()
{
    /* We load the MC statistcs from model
     */
    m_results->clear();

    for (int i = 0; i < m_model.data()->getMCStatisticResult(); ++i) {
        if (!m_model.data()->getMCStatisticResult(i).isEmpty()) {
            QListWidgetItem* item = new QListWidgetItem(tr("Monte Carlo"));
            item->setData(Qt::UserRole, SupraFit::Statistic::MonteCarlo);
            item->setData(Qt::UserRole + 1, i);
            m_results->addItem(item);
        }
    }

    QJsonObject statistic = m_model.data()->getWGStatisticResult();
    if (!statistic.isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem(tr("Weakend Grid Search"));
        item->setData(Qt::UserRole, SupraFit::Statistic::WeakenedGridSearch);
        item->setData(Qt::UserRole + 1, 0);
        m_results->addItem(item);
    }

    statistic = m_model.data()->getMoCoStatisticResult();
    if (!statistic.isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem(tr("Model Comparison"));
        item->setData(Qt::UserRole, SupraFit::Statistic::ModelComparison);
        item->setData(Qt::UserRole + 1, 0);
        m_results->addItem(item);
    }

    statistic = m_model.data()->getReduction();
    if (!statistic.isEmpty()) {
        //m_statistic_result->setWidget(new ResultsWidget(statistic, m_model, m_charts.signal_wrapper));
        QListWidgetItem* item = new QListWidgetItem(tr("Reduction Analysis"));
        item->setData(Qt::UserRole, SupraFit::Statistic::Reduction);
        item->setData(Qt::UserRole + 1, 0);
        m_results->addItem(item);
    }
}

void ResultsDialog::itemDoubleClicked(QListWidgetItem* item)
{
    SupraFit::Statistic type = SupraFit::Statistic(item->data(Qt::UserRole).toInt());
    QJsonObject statistic;
    if (type == SupraFit::Statistic::MonteCarlo) {
        int index = item->data(Qt::UserRole + 1).toInt();
        statistic = m_model.data()->getMCStatisticResult(index);
    }

    m_tabs->addTab(new ResultsWidget(statistic, m_model, m_wrapper), QString::number(type));
}

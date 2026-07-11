/*
 * SupraFit - scalable table view of an equilibrium model's parameters
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QTransposeProxyModel>

#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>

#include "src/core/models/AbstractModel.h"
#include "src/core/models/datatable.h"

#include "dynamicmodelwidget.h"

DynamicModelWidget::DynamicModelWidget(QSharedPointer<AbstractModel> model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* globalLabel = new QLabel(tr("<b>Global parameters</b> (one row per species)"));
    layout->addWidget(globalLabel);

    // The global table is 1 x nSpecies; transpose it so every species (stability constant) is a row,
    // which stays readable no matter how many species the reaction system defines.
    m_global_view = new QTableView;
    m_global_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_global_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    if (m_model && m_model->GlobalTable()) {
        QTransposeProxyModel* proxy = new QTransposeProxyModel(this);
        proxy->setSourceModel(m_model->GlobalTable());
        m_global_view->setModel(proxy);
    }
    layout->addWidget(m_global_view);

    QLabel* localLabel = new QLabel(tr("<b>Local parameters</b> (one row per series)"));
    layout->addWidget(localLabel);

    m_local_view = new QTableView;
    m_local_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_local_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    if (m_model && m_model->LocalTable())
        m_local_view->setModel(m_model->LocalTable());
    layout->addWidget(m_local_view);

    if (m_model)
        connect(m_model.data(), &AbstractModel::Recalculated, this, &DynamicModelWidget::refresh);
}

void DynamicModelWidget::refresh()
{
    // reset() re-reads every cell from the underlying DataTable(s) after a recalculation/fit, without
    // depending on fine-grained dataChanged signals.
    if (m_global_view)
        m_global_view->reset();
    if (m_local_view)
        m_local_view->reset();
}

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

#pragma once

#include <QtCore/QSharedPointer>

#include <QtWidgets/QWidget>

class AbstractModel;
class QTableView;

/**
 * @brief Compact, scalable table view of a model's global and local parameters (Claude Generated).
 *
 * The classic model view spawns one group box per parameter, which does not scale to N-component
 * models with many species. This widget instead binds two QTableView(s) to the model's existing
 * GlobalTable()/LocalTable() (both QAbstractTableModel), so an arbitrary number of species and series
 * stay browsable in a scroll area. The global table is transposed so each species is one row. It is a
 * read-only companion to the classic editor (opt-in via AbstractModel::UseDynamicParameterWidget);
 * the views refresh when the model recalculates.
 */
class DynamicModelWidget : public QWidget {
    Q_OBJECT

public:
    explicit DynamicModelWidget(QSharedPointer<AbstractModel> model, QWidget* parent = nullptr);

private:
    void refresh();

    QSharedPointer<AbstractModel> m_model;
    QTableView* m_global_view = nullptr;
    QTableView* m_local_view = nullptr;
};

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

#pragma once

#include <QtCore/QJsonObject>

#include "src/core/models/models.h"

namespace Statistic {

QString MonteCarlo2Thermo(int index, qreal T, const QJsonObject& object = QJsonObject(), bool heat = false);
/*! \brief Monte-Carlo BC50 for a reaction-defined system, dispatched on its stoichiometry.
 *
 * The `*_any` models carry their equilibrium system in a stoichiometry matrix rather than in the
 * model id, so they cannot pick a fixed BC50 formula at compile time. Returns an empty string when
 * the system has no implemented BC50, so the caller reports nothing instead of a wrong number.
 * \param lgBeta CUMULATIVE lg beta per species, in the column order of \p stoich.
 * Claude Generated (2026). */
QString MonteCarlo2BC50_Speciation(const Eigen::MatrixXi& stoich, const QVector<qreal>& lgBeta, const QJsonObject& object);

QString GridSearch2Thermo(int index, qreal T, const QJsonObject& object = QJsonObject(), bool heat = false);
QJsonObject PostGridSearch(const QList<QJsonObject>& models, qreal K, qreal T, int index, qreal H = 0);
/*! \brief Grid-search BC50 for a reaction-defined system. See MonteCarlo2BC50_Speciation. CG (2026). */
QString GridSearch2BC50_Speciation(const Eigen::MatrixXi& stoich, const QVector<qreal>& lgBeta, const QJsonObject& object);

QString PseudoANOVA(const QPointer<const AbstractModel>& model);
}

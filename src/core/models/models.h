/*
 * <one line to give the program's name and a brief idea of what it does.>
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

// Model factory. The concrete model headers and the factory registry live in models.cpp so that
// including this header no longer drags in every model definition (Claude Generated 2026, R5).

#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QWeakPointer>

#include "src/global.h"

#include "AbstractModel.h" // AbstractModel (return type) + DataClass (QPointer needs a complete type)

QSharedPointer<AbstractModel> CreateModel(int model, QPointer<DataClass> data);
QSharedPointer<AbstractModel> CreateModel(int model, QWeakPointer<DataClass> data);
QSharedPointer<AbstractModel> CreateModel(const QJsonObject& model, QWeakPointer<DataClass> data);

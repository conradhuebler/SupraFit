/*
 * SupraFit - fluorescence titration, 1:1 (preset of fl_any)
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
 * Rebuilt in 2026 (AI-implemented, Claude): the model is a preset of fl_any with the reactions of its stoichiometry, so that the signal is the
 * textbook sum of concentration times fluorescence coefficient for every point (the earlier version gave the first point a coefficient of its own
 * and, for 1:1, scaled the signal with the model signal of the first row). Stability constants are the cumulative lg beta of the species,
 * not the stepwise lg K of the earlier version. See docs/fluorescence_models.md.
 */

#include "fl_1_1_Model.h"

#include <QtCore/QJsonObject>

namespace {
const char* Reactions = "A + B <=> AB";
}

fl_ItoI_Model::fl_ItoI_Model(DataClass* data)
    : fl_any_Model(data)
{
    DefinePreset(QString::fromLatin1(Reactions)); // here SFModel() is this model: name, ids and the check of the data columns are its own
}

fl_ItoI_Model::fl_ItoI_Model(AbstractTitrationModel* other)
    : fl_any_Model(other)
{
}

fl_ItoI_Model::~fl_ItoI_Model()
{
}

QSharedPointer<AbstractModel> fl_ItoI_Model::Clone(bool statistics)
{
    QSharedPointer<AbstractModel> model = QSharedPointer<fl_ItoI_Model>(new fl_ItoI_Model(this), &QObject::deleteLater);
    finishClone(model, statistics);
    return model;
}

#include "fl_1_1_Model.moc"

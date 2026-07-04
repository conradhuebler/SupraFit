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

// Claude Generated (2026, R5): the model factory. Moved out of models.h (which every model TU
// pulled in, dragging all ~36 model headers with it) into this single translation unit, and the
// 36-arm switch replaced by a central factory registry. Adding a model = one registry line.

#include "models.h"

#include "meta_model.h"
#include "scriptmodel.h"

#include "titrations/models.h"

#include "kinetics/bimolecularmodel.h"
#include "kinetics/evap.h"
#include "kinetics/flexmolecularmodel.h"
#include "kinetics/mm_model.h"
#include "kinetics/monomolecularmodel.h"
#include "kinetics/tian.h"

#include "photophysics/decayrates.h"

#include "thermodynamics/models.h"

#include "misc/models.h"

#include <QtCore/QHash>

#include <functional>

namespace {

using ModelFactory = std::function<AbstractModel*(DataClass*)>;

// enum SupraFit::Model -> constructor. The QSharedPointer wrapping + deleteLater deleter and the
// throw(-2) guard live once, in CreateModel below.
const QHash<int, ModelFactory>& modelFactories()
{
    static const QHash<int, ModelFactory> factories = {
        { SupraFit::nmr_ItoI, [](DataClass* d) { return new nmr_ItoI_Model(d); } },
        { SupraFit::nmr_IItoI_ItoI, [](DataClass* d) { return new nmr_IItoI_ItoI_Model(d); } },
        { SupraFit::nmr_ItoI_ItoII, [](DataClass* d) { return new nmr_ItoI_ItoII_Model(d); } },
        { SupraFit::nmr_IItoI_ItoI_ItoII, [](DataClass* d) { return new nmr_IItoI_ItoI_ItoII_Model(d); } },
        { SupraFit::nmr_any, [](DataClass* d) { return new nmr_any_Model(d); } },
        { SupraFit::Michaelis_Menten, [](DataClass* d) { return new Michaelis_Menten_Model(d); } },
        { SupraFit::Indep_Quadrat, [](DataClass* d) { return new Indep_Quadrat(d); } },
        { SupraFit::Dep_Any, [](DataClass* d) { return new Dep_Any(d); } },
        { SupraFit::MonoMolecularModel, [](DataClass* d) { return new MonoMolecularModel(d); } },
        { SupraFit::BiMolecularModel, [](DataClass* d) { return new BiMolecularModel(d); } },
        { SupraFit::FlexMolecularModel, [](DataClass* d) { return new FlexMolecularModel(d); } },
        { SupraFit::itc_ItoI, [](DataClass* d) { return new itc_ItoI_Model(d); } },
        { SupraFit::itc_IItoI, [](DataClass* d) { return new itc_IItoI_Model(d); } },
        { SupraFit::itc_ItoII, [](DataClass* d) { return new itc_ItoII_Model(d); } },
        { SupraFit::itc_IItoII, [](DataClass* d) { return new itc_IItoII_Model(d); } },
        { SupraFit::itc_n_ItoI, [](DataClass* d) { return new itc_n_ItoI_Model(d); } },
        { SupraFit::itc_n_ItoII, [](DataClass* d) { return new itc_n_ItoII_Model(d); } },
        { SupraFit::itc_blank, [](DataClass* d) { return new Blank(d); } },
        { SupraFit::itc_any, [](DataClass* d) { return new itc_any_Model(d); } },
        { SupraFit::fl_ItoI, [](DataClass* d) { return new fl_ItoI_Model(d); } },
        { SupraFit::fl_ItoI_ItoII, [](DataClass* d) { return new fl_ItoI_ItoII_Model(d); } },
        { SupraFit::fl_IItoI_ItoI, [](DataClass* d) { return new fl_IItoI_ItoI_Model(d); } },
        { SupraFit::fl_IItoI_ItoI_ItoII, [](DataClass* d) { return new fl_IItoI_ItoI_ItoII_Model(d); } },
        { SupraFit::uv_vis_ItoI, [](DataClass* d) { return new uv_vis_ItoI_Model(d); } },
        { SupraFit::uv_vis_ItoI_ItoII, [](DataClass* d) { return new uv_vis_ItoI_ItoII_Model(d); } },
        { SupraFit::uv_vis_IItoI_ItoI, [](DataClass* d) { return new uv_vis_IItoI_ItoI_Model(d); } },
        { SupraFit::uv_vis_IItoI_ItoI_ItoII, [](DataClass* d) { return new uv_vis_IItoI_ItoI_ItoII_Model(d); } },
        { SupraFit::uvvis_any, [](DataClass* d) { return new uvvis_any_Model(d); } },
        { SupraFit::ScriptModel, [](DataClass* d) { return new ScriptModel(d); } },
        { SupraFit::DecayRates, [](DataClass* d) { return new DecayRates(d); } },
        { SupraFit::Arrhenius, [](DataClass* d) { return new ArrheniusFit(d); } },
        { SupraFit::Eyring, [](DataClass* d) { return new EyringFit(d); } },
        { SupraFit::TianModel, [](DataClass* d) { return new TIANModel(d); } },
        { SupraFit::EvapMModel, [](DataClass* d) { return new EvapMonoModel(d); } },
        { SupraFit::BETModel, [](DataClass* d) { return new BETModel(d); } },
        { SupraFit::MetaModel, [](DataClass* d) { return new MetaModel(d); } },
    };
    return factories;
}

} // namespace

QSharedPointer<AbstractModel> CreateModel(int model, QPointer<DataClass> data)
{
    QSharedPointer<AbstractModel> t;
    try {
        const auto it = modelFactories().constFind(model);
        if (it != modelFactories().constEnd())
            t = QSharedPointer<AbstractModel>(it.value()(data.data()), &QObject::deleteLater);
    } catch (const int value) {
        if (value == -2)
            t.clear();
    }
    return t;
}

QSharedPointer<AbstractModel> CreateModel(int model, QWeakPointer<DataClass> data)
{
    return CreateModel(model, data.toStrongRef().data());
}

QSharedPointer<AbstractModel> CreateModel(const QJsonObject& model, QWeakPointer<DataClass> data)
{
    return QSharedPointer<ScriptModel>(new ScriptModel(data.toStrongRef().data(), model), &QObject::deleteLater);
}

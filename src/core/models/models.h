#pragma once

#include <QtCore/QSharedPointer>

#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/global.h"

/* Pure Abstract Models */
#include "AbstractModel.h"

/* Meta Model - holds different models */
#include "meta_model.h"

/* Scripted Model - may be anything */
#include "scriptmodel.h"

/* Collection of titration models */
#include "titrations/models.h"

/* Kinetic Models */
#include "kinetics/bimolecularmodel.h"
#include "kinetics/evap.h"
#include "kinetics/flexmolecularmodel.h"
#include "kinetics/mm_model.h"
#include "kinetics/monomolecularmodel.h"
#include "kinetics/tian.h"

/* Photophysics stuff */
#include "photophysics/decayrates.h"

/* Thermodynamics  models */
#include "thermodynamics/models.h"

/* Any other models */
#include "misc/models.h"

inline QSharedPointer<AbstractModel> CreateModel(int model, QPointer<DataClass> data)
{
    QSharedPointer<AbstractModel> t;
    try {
        switch (model) {
        case SupraFit::nmr_ItoI:
            t = QSharedPointer<nmr_ItoI_Model>(new nmr_ItoI_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::nmr_IItoI_ItoI:
            t = QSharedPointer<nmr_IItoI_ItoI_Model>(new nmr_IItoI_ItoI_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::nmr_ItoI_ItoII:
            t = QSharedPointer<nmr_ItoI_ItoII_Model>(new nmr_ItoI_ItoII_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::nmr_IItoI_ItoI_ItoII:
            t = QSharedPointer<nmr_IItoI_ItoI_ItoII_Model>(new nmr_IItoI_ItoI_ItoII_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::nmr_any:
            t = QSharedPointer<nmr_any_Model>(new nmr_any_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::Michaelis_Menten:
            t = QSharedPointer<Michaelis_Menten_Model>(new Michaelis_Menten_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::Indep_Quadrat:
            t = QSharedPointer<Indep_Quadrat>(new Indep_Quadrat(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::Dep_Any:
            t = QSharedPointer<Dep_Any>(new Dep_Any(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::MonoMolecularModel:
            t = QSharedPointer<MonoMolecularModel>(new MonoMolecularModel(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::BiMolecularModel:
            t = QSharedPointer<BiMolecularModel>(new BiMolecularModel(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::FlexMolecularModel:
            t = QSharedPointer<FlexMolecularModel>(new FlexMolecularModel(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::itc_ItoI:
            t = QSharedPointer<itc_ItoI_Model>(new itc_ItoI_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::itc_IItoI:
            t = QSharedPointer<itc_IItoI_Model>(new itc_IItoI_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::itc_ItoII:
            t = QSharedPointer<itc_ItoII_Model>(new itc_ItoII_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::itc_IItoII:
            t = QSharedPointer<itc_IItoII_Model>(new itc_IItoII_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::itc_n_ItoI:
            t = QSharedPointer<itc_n_ItoI_Model>(new itc_n_ItoI_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::itc_n_ItoII:
            t = QSharedPointer<itc_n_ItoII_Model>(new itc_n_ItoII_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::itc_blank:
            t = QSharedPointer<Blank>(new Blank(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::itc_any:
            t = QSharedPointer<itc_any_Model>(new itc_any_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::fl_ItoI:
            t = QSharedPointer<fl_ItoI_Model>(new fl_ItoI_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::fl_ItoI_ItoII:
            t = QSharedPointer<fl_ItoI_ItoII_Model>(new fl_ItoI_ItoII_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::fl_IItoI_ItoI:
            t = QSharedPointer<fl_IItoI_ItoI_Model>(new fl_IItoI_ItoI_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::fl_IItoI_ItoI_ItoII:
            t = QSharedPointer<fl_IItoI_ItoI_ItoII_Model>(new fl_IItoI_ItoI_ItoII_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::uv_vis_ItoI:
            t = QSharedPointer<uv_vis_ItoI_Model>(new uv_vis_ItoI_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::uv_vis_ItoI_ItoII:
            t = QSharedPointer<uv_vis_ItoI_ItoII_Model>(new uv_vis_ItoI_ItoII_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::uv_vis_IItoI_ItoI:
            t = QSharedPointer<uv_vis_IItoI_ItoI_Model>(new uv_vis_IItoI_ItoI_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::uv_vis_IItoI_ItoI_ItoII:
            t = QSharedPointer<uv_vis_IItoI_ItoI_ItoII_Model>(new uv_vis_IItoI_ItoI_ItoII_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::uvvis_any:
            t = QSharedPointer<uvvis_any_Model>(new uvvis_any_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::ScriptModel:
            t = QSharedPointer<ScriptModel>(new ScriptModel(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::DecayRates:
            t = QSharedPointer<DecayRates>(new DecayRates(data.data()), &QObject::deleteLater);
            break;

        case SupraFit::Arrhenius:
            t = QSharedPointer<ArrheniusFit>(new ArrheniusFit(data.data()), &QObject::deleteLater);
            break;

        case SupraFit::Eyring:
            t = QSharedPointer<EyringFit>(new EyringFit(data.data()), &QObject::deleteLater);
            break;

        case SupraFit::TianModel:
            t = QSharedPointer<TIANModel>(new TIANModel(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::EvapMModel:
            t = QSharedPointer<EvapMonoModel>(new EvapMonoModel(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::BETModel:
            t = QSharedPointer<BETModel>(new BETModel(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::MetaModel:
            t = QSharedPointer<MetaModel>(new MetaModel(data.data()), &QObject::deleteLater);
            break;

        default:
            t.clear();
        };
    } catch (const int value) {
        if (value == -2)
            t.clear();
    }
    return t;
}

inline QSharedPointer<AbstractModel> CreateModel(int model, QWeakPointer<DataClass> data)
{
    return CreateModel(model, data.toStrongRef().data());
}

inline QSharedPointer<AbstractModel> CreateModel(const QJsonObject& model, QWeakPointer<DataClass> data)
{
    QSharedPointer<AbstractModel> t;

    return t = QSharedPointer<ScriptModel>(new ScriptModel(data.toStrongRef().data(), model), &QObject::deleteLater);
}

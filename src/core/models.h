#pragma once

#include <QtCore/QSharedPointer>

#include "src/core/dataclass.h"
#include "src/global.h"

/* Pure Abstract Models */
#include "AbstractModel.h"
#include "AbstractItcModel.h"
#include "AbstractTitrationModel.h"

/* ITC Models */
#include "models/itc/itc_1_1_Model.h"
#include "models/itc/itc_2_1_Model.h"
#include "models/itc/itc_1_2_Model.h"
#include "models/itc/itc_2_2_Model.h"

/* Fluorescence Models */
#include "models/fluorescence/fl_1_1_Model.h"
#include "models/fluorescence/fl_2_1_1_1_Model.h"
#include "models/fluorescence/fl_1_1_1_2_Model.h"
#include "models/fluorescence/fl_2_1_1_1_1_2_Model.h"

/* NMR and UV/VIS titration */
#include "models/titrations/1_1_Model.h"
#include "models/titrations/2_1_1_1_Model.h"
#include "models/titrations/1_1_1_2_Model.h"
#include "models/titrations/2_1_1_1_1_2_Model.h"

#include "models/titrations/ScriptModel.h"
/* Kinetic Models */
#include "models/kinetics/mm_model.h"
#include "models/kinetics/monomolecularmodel.h"


inline SupraFit::Model   Name2Model(const QString &str)
{
    if(str == "1:1-Model")
        return SupraFit::ItoI;
    else if(str == "2:1/1:1-Model")
        return SupraFit::IItoI_ItoI;
    else if(str == "1:1/1:2-Model")
       return SupraFit::ItoI_ItoII;
    else if(str == "2:1/1:1/1:2-Model")
        return SupraFit::IItoI_ItoI_ItoII;
    else if(str == "itc_1:1-Model")
        return SupraFit::itc_ItoI;
    else if(str == "itc_2:1/1:1-Model")
        return SupraFit::itc_IItoI;
    else if(str == "itc_1:1/1:2-Model")
        return SupraFit::itc_ItoII;
    else if(str == "itc_2:1/1:1/1:2-Model")
        return SupraFit::itc_IItoII;
    else if(str == "fl_1:1-Model")
         return SupraFit::fl_ItoI;
    else if(str == "fl_2:1/1:1-Model")
        return SupraFit::fl_IItoI_ItoI;
    else if(str == "fl_1:1/1:2-Model")
        return SupraFit::fl_ItoI_ItoII;
    else if(str == "fl_2:1/1:1/1:2-Model")
        return SupraFit::fl_IItoI_ItoI_ItoII;
    else if(str == "Monomolecular Kinetics")
         return SupraFit::MonoMolecularModel;
    else if(str == "Michaelis Menten")
        return SupraFit::Michaelis_Menten;
    else
        return SupraFit::Unknown;

}


inline QSharedPointer<AbstractModel > CreateModel(int model, QWeakPointer<DataClass> data)
{
    QSharedPointer<AbstractModel > t;

    switch(model){
        case SupraFit::ItoI:
            t =  QSharedPointer<ItoI_Model>(new ItoI_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::IItoI_ItoI:
            t = QSharedPointer<IItoI_ItoI_Model>(new IItoI_ItoI_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::ItoI_ItoII:
            t = QSharedPointer<ItoI_ItoII_Model>(new ItoI_ItoII_Model(data.data()),  &QObject::deleteLater);
            break;
        case SupraFit::IItoI_ItoI_ItoII:
            t = QSharedPointer<IItoI_ItoI_ItoII_Model>(new IItoI_ItoI_ItoII_Model(data.data()),  &QObject::deleteLater);
            break;
        case SupraFit::Michaelis_Menten:
            t = QSharedPointer<Michaelis_Menten_Model>(new Michaelis_Menten_Model(data.data()),  &QObject::deleteLater);
            break;
        case SupraFit::MonoMolecularModel:
            t = QSharedPointer<MonoMolecularModel>(new MonoMolecularModel(data.data()),  &QObject::deleteLater);
            break;
        case SupraFit::itc_ItoI:
            t = QSharedPointer<itc_ItoI_Model>(new itc_ItoI_Model(data.data()),  &QObject::deleteLater);
            break;
        case SupraFit::itc_IItoI:
            t = QSharedPointer<itc_IItoI_Model>(new itc_IItoI_Model(data.data()),  &QObject::deleteLater);
            break;
        case SupraFit::itc_ItoII:
            t = QSharedPointer<itc_ItoII_Model>(new itc_ItoII_Model(data.data()),  &QObject::deleteLater);
            break;
        case SupraFit::itc_IItoII:
            t = QSharedPointer<itc_IItoII_Model>(new itc_IItoII_Model(data.data()),  &QObject::deleteLater);
            break;
        case SupraFit::fl_ItoI:
            t =  QSharedPointer<fl_ItoI_Model>(new fl_ItoI_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::fl_ItoI_ItoII:
            t =  QSharedPointer<fl_ItoI_ItoII_Model>(new fl_ItoI_ItoII_Model(data.data()), &QObject::deleteLater);
            break;
        case SupraFit::fl_IItoI_ItoI:
             t = QSharedPointer<fl_IItoI_ItoI_Model>(new fl_IItoI_ItoI_Model(data.data()), &QObject::deleteLater);
            break;
         case SupraFit::fl_IItoI_ItoI_ItoII:
            t = QSharedPointer<fl_IItoI_ItoI_ItoII_Model>(new fl_IItoI_ItoI_ItoII_Model(data.data()), &QObject::deleteLater);
        break;
        default:
            t.clear();
    };
    return t;
}

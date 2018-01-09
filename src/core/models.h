#pragma once

#include "AbstractModel.h"
#include "AbstractTitrationModel.h"
#include "models/itc/itc_1_1_Model.h"
#include "models/fluorescence/fl_1_1_Model.h"
#include "models/fluorescence/fl_2_1_1_1_Model.h"
#include "models/fluorescence/fl_1_1_1_2_Model.h"
#include "models/titrations/1_1_Model.h"
#include "models/titrations/2_1_1_1_Model.h"
#include "models/titrations/1_1_1_2_Model.h"
#include "models/titrations/2_1_1_1_1_2_Model.h"
#include "models/titrations/ScriptModel.h"
#include "models/kinetics/mm_model.h"
#include "models/kinetics/first_order_model.h"

namespace SupraFit{
    
    enum {
        ItoI = 1,
        IItoI_ItoI = 2,
        ItoI_ItoII = 3,
        IItoI_ItoI_ItoII = 4,
        Michaelis_Menten = 5,
        First_Order_Kinetics = 6,
        ScriptedModel = 10,
        itc_ItoI = 11,
        fl_ItoI = 12,
        fl_IItoI_ItoI = 13,
        fl_ItoI_ItoII = 14
    };
    
}

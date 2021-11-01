/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "AbstractItcModel.h"
#include "AbstractNMRModel.h"
#include "AbstractTitrationModel.h"

/* ITC Models */
#include "itc/blank.h"
#include "itc/itc_1_1_Model.h"
#include "itc/itc_1_2_Model.h"
#include "itc/itc_2_1_Model.h"
#include "itc/itc_2_2_Model.h"
#include "itc/itc_n_1_1_Model.h"
#include "itc/itc_n_1_2_Model.h"

/* Fluorescence Models */
#include "fluorescence/fl_1_1_1_2_Model.h"
#include "fluorescence/fl_1_1_Model.h"
#include "fluorescence/fl_2_1_1_1_1_2_Model.h"
#include "fluorescence/fl_2_1_1_1_Model.h"

/* NMR */
#include "nmr/nmr_1_1_1_2_Model.h"
#include "nmr/nmr_1_1_Model.h"
#include "nmr/nmr_2_1_1_1_1_2_Model.h"
#include "nmr/nmr_2_1_1_1_Model.h"

/* UV/VIS */
#include "uv_vis/uv_vis_1_1_1_2_Model.h"
#include "uv_vis/uv_vis_1_1_Model.h"
#include "uv_vis/uv_vis_2_1_1_1_1_2_Model.h"
#include "uv_vis/uv_vis_2_1_1_1_Model.h"

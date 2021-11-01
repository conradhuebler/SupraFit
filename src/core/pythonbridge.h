/*
 * Python bridge to SupraFit
 * Copyright (C) 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/global_config.h"

#ifdef _Python

/*
 * Test for using python to access SupraFit using ctypes

from ctypes import cdll
hello_lib = cdll.LoadLibrary("./libpythonbridge.so")
hello_lib.LoadFile();


*/

struct CharPtrWrapperTag {
    int len;
    char* data;
};

typedef CharPtrWrapperTag CharPtrWrapper;

extern "C" {

CharPtrWrapper* LoadFile(const char* chars);
void Release(CharPtrWrapperTag* pWrap);
}

#endif

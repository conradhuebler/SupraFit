#!/usr/bin/env python3


# <python Test script for python wrapping functions.>
# Copyright (C) 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


# adopted from
# https://stackoverflow.com/questions/55103298/python-ctypes-read-pointerc-char-in-python

import sys
import ctypes
import json

CharPtr = ctypes.POINTER(ctypes.c_char)

class CharPtrWrapper(ctypes.Structure):
    _fields_ = [
        ("len", ctypes.c_int),
        ("data", CharPtr),
    ]


CharPtrWrapper = ctypes.POINTER(CharPtrWrapper)



def main():
    hello_lib = ctypes.cdll.LoadLibrary("libpythonbridge.so")
    input_string = input("SupraFit File Name to display: ")
    string = ctypes.create_string_buffer(input_string.encode('utf-8'))
    get = hello_lib.LoadFile
    get.restype = CharPtrWrapper
    release = hello_lib.Release
    release.argtypes = [CharPtrWrapper] 
    blob = get(string)
    wrap = blob.contents
    CharArr = ctypes.c_char * wrap.len
    char_arr = CharArr(*wrap.data[:wrap.len])
    release(blob)

    y = json.loads(char_arr.value)
    print(y)
    
if __name__ == "__main__":
    print("Python {:s} on {:s}\n".format(sys.version, sys.platform))
    main()

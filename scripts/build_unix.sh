#!/bin/bash
set -ex

if [ "$CXX" = "g++" ]; then export CXX="g++-6" CC="gcc-6"; fi # else export CXX="clang++" CC="clang"; fi
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make 

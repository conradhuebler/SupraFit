#!/bin/bash
set -ex

if [ "$CXX" = "g++" ]; then export CXX="g++-6" CC="gcc-6"; fi # else export CXX="clang++" CC="clang"; fi
git submodule init
git submodule update --recursive
git pull --all
mkdir -p release
cd release
cmake -DCMAKE_BUILD_TYPE=Release -Dnoto_font=true -D_Theme=false ..
make 

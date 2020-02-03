#!/bin/bash
set -ex

if [ "$CXX" = "g++" ]; then export CXX="g++-6" CC="gcc-6"; fi # else export CXX="clang++" CC="clang"; fi
git submodule init
git submodule update --recursive
# check submodules, seems not to work automatically
cd ../external/CuteChart/
git checkout master
git pull
cd ../..

cd external/libpeakpick/
git checkout master
git submodule init
git submodule update --recursive
git pull
cd ../..

cd external/fisher_dist/
git checkout master
git pull
cd ../..

mkdir -p release
cd release
cmake -DCMAKE_BUILD_TYPE=Release -Dnoto_font=true -D_Theme=false ..
make 

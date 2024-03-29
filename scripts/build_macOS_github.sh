#!/bin/bash
set -ex

git submodule init
git submodule update --recursive
# check submodules, seems not to work automatically

cd external
for i in $(ls -d */); do cd $i; git checkout master; git submodule init; git submodule update --recursive; cd ..; done
cd ..

mkdir -p release
cd release
#cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -Dnoto_font=true -D_Theme=false -DCMAKE_PREFIX_PATH=$HOME/SupraFit/Qt/5.15.1/gcc_64 ..
cmake -DCMAKE_BUILD_TYPE=Release -Dnoto_font=true -D_Theme=false  ..
make 
cd bin/macOS
macdeployqt  suprafit.app -dmg

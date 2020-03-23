echo on

SET project_dir="%cd%"
echo Building SupraFit using MinGW ...
git submodule update --init --recursive
git pull --recurse-submodules
mkdir build_windows
cd build_windows

cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -MT ..
cmake --build . --config Release

cd ..


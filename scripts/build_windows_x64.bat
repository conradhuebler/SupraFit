echo on

SET project_dir="%cd%"
SET VCInstallDir="C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/VC/Tools/MSVC/14.23.28105/bin/Hostx64/x64/"
echo Building SupraFit x64...
git submodule update --init --recursive
git pull --recurse-submodules
mkdir build_x64
cd build_x64
cmake -G "Visual Studio 16 2019" -DCMAKE_BUILD_TYPE=Release -MT ..
cmake --build . --config Release


echo Packaging...
cd %project_dir%
cd build_x64\bin\win\Release
mkdir SupraFit
copy suprafit.exe SupraFit
copy suprafit_cli.exe SupraFit

cd SupraFit
windeployqt --release --compiler-runtime  suprafit.exe

echo Copying project files for archival...
copy "%project_dir%\README.md" "%project_dir%\build_x64\bin\win\Release\SupraFit\README.md"
copy "%project_dir%\LICENSE.md" "%project_dir%\build_x64\bin\win\Release\SupraFit\LICENSE.md"

echo Packaging portable archive...

cd ..
7z a SupraFit.zip SupraFit

cd ..


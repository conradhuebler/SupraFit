echo on

SET project_dir="%cd%"
SET VCInstallDir="C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/VC/Tools/MSVC/14.23.28105/bin/Hostx32/x32/"
echo Building SupraFit x32...
git submodule update --init --recursive
git pull --recurse-submodules
mkdir build_x32
cd build_x32
cmake -G "Visual Studio 16 2019" -DCMAKE_BUILD_TYPE=Release -MT ..
cmake --build . --config Release


echo Packaging...
cd %project_dir%
cd build_x32\bin\win32\Release
mkdir SupraFit
copy suprafit.exe SupraFit
copy suprafit_cli.exe SupraFit

cd SupraFit
windeployqt --release suprafit.exe

echo Copying project files for archival...
copy "%project_dir%\README.md" "%project_dir%\build_x32\bin\win32\Release\SupraFit\README.md"
copy "%project_dir%\LICENSE.md" "%project_dir%\build_x32\bin\win32\Release\SupraFit\LICENSE.md"

echo Packaging portable archive...
cd ..
7z a SupraFit_latest_x32_windows.zip SupraFit

cd ..


echo on

SET project_dir="%cd%"

echo Building SupraFit...
git submodule update --init --recursive
git pull --recurse-submodules
mkdir build_x64
cd build_x64
cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=Release -Dnoto_font=true ..
cmake --build . --config Release


echo Packaging...
cd %project_dir%\build_x64\Release
mkdir SupraFit
copy suprafit.exe SupraFit
copy suprafit_cli.exe SupraFit

cd SupraFit
windeployqt --release suprafit.exe

echo Copying project files for archival...
copy "%project_dir%\README.md" "%project_dir%\build\Release\SupraFit\README.md"
copy "%project_dir%\LICENSE.md" "%project_dir%\build\Release\SupraFit\LICENSE.md"

echo Packaging portable archive...
cd ..
7z a SupraFit_nightly_%TAG_NAME%_x64_windows.zip SupraFit_x64

set QTDIR=C:\Qt\5.13.0\msvc2017
set PATH=C:\Qt\5.13.0\msvc2017\bin;%PATH%

cd %project_dir%
mkdir build_x32
cd build_x32
cmake -G "Visual Studio 15 2017" -DCMAKE_BUILD_TYPE=Release -Dnoto_font=true ..
cmake --build . --config Release


echo Packaging...
cd %project_dir%\build_x32\Release
mkdir SupraFit
copy suprafit.exe SupraFit
copy suprafit_cli.exe SupraFit

cd SupraFit
windeployqt --release suprafit.exe

echo Copying project files for archival...
copy "%project_dir%\README.md" "%project_dir%\build\Release\SupraFit\README.md"
copy "%project_dir%\LICENSE.md" "%project_dir%\build\Release\SupraFit\LICENSE.md"

echo Packaging portable archive...
cd ..
7z a SupraFit_nightly_%TAG_NAME%_x32_windows.zip SupraFit_x32

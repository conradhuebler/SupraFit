echo on

SET project_dir="%cd%"

echo Building SupraFit...
git submodule update --init --recursive
git pull --recurse-submodules
mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release


echo Packaging...
cd %project_dir%\build\Release
mkdir SupraFit
copy suprafit.exe SupraFit
copy suprafit_cli.exe SupraFit

cd SupraFit
windeployqt --release SupraFit.exe

echo Copying project files for archival...
copy "%project_dir%\README.md" "%project_dir%\build\Release\SupraFit\README.md"
copy "%project_dir%\LICENSE.md" "%project_dir%\build\Release\SupraFit\LICENSE.md"

echo Packaging portable archive...
cd ..
7z a SupraFit_nightly_%TAG_NAME%_windows.zip SupraFit

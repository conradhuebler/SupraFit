echo on

SET project_dir="%cd%"
echo Building SupraFit x64...
git submodule update --init --recursive
git pull --recurse-submodules
mkdir build_x64
cd build_x64
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release  ..
if errorlevel 1 exit /b 1
REM Build only the two deployed executables (like the macOS/Linux scripts), in parallel. Building
REM every target single-threaded - which -mbig-obj now lets run to completion - overran the 60 min CI
REM cap. suprafit + suprafit_cli pull in all needed libs but skip the test suite. Claude Generated.
cmake --build . --parallel 4 --target suprafit suprafit_cli
if errorlevel 1 exit /b 1

echo Packaging...
cd %project_dir%
cd build_x64\bin\win
mkdir SupraFit
copy suprafit.exe SupraFit
copy suprafit_cli.exe SupraFit
copy C:\ProgramData\chocolatey\lib\mingw\tools\install\mingw64\bin\libgomp-1.dll SupraFit

cd SupraFit
windeployqt --release --compiler-runtime  suprafit.exe

echo Copying project files for archival...
copy "%project_dir%\README.md" "%project_dir%\build_x64\bin\win\Release\SupraFit\README.md"
copy "%project_dir%\LICENSE.md" "%project_dir%\build_x64\bin\win\Release\SupraFit\LICENSE.md"

echo Packaging portable archive...

cd ..
7z a SupraFit.zip SupraFit

cd ..


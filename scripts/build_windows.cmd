echo on

SET project_dir="%cd%"

echo Building QBit...
git submodule update --init --recursive
git pull --recurse-submodules
mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release


echo Packaging...
cd %project_dir%\build
mkdir QBit
copy QBit.exe QBit
cd QBit
windeployqt --release QBit.exe
copy C:\Qt\Tools\mingw530_32\bin\libgomp-1.dll  "%project_dir%\build\QBit\libgomp-1.dll

echo Copying project files for archival...
copy "%project_dir%\README.md" "%project_dir%\build\QBit\README.md"
copy "%project_dir%\LICENSE.md" "%project_dir%\build\QBit\LICENSE.md"

echo Packaging portable archive...
cd ..
7z a QBit_nightly_%TAG_NAME%_windows.zip QBit

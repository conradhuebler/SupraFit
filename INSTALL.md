## Download and requirements
git clones automatically CuteCharts, libpeakpick and fisher-dist. Since libpeakpick needs Eigen, it will be cloned automatically.
## Compiling
To compile SupraFit you will need CMake 3 or newer, a C++11-capable compiler and Qt 5.10 or newer.

> SupraFit needs QtCharts, so please provide it

SupraFit has been successfully compilied with: 
- gcc 5.2 - 9.1
- clang 3.9 

on linux systems and 
 
- MinGW and MS Visual Studio on Windows Systems

> Windows 7 or higher is recommended if Qt is compilied without ICU support.

To obtain the most recent development version use
```sh
git clone --recursive https://github.com/conradhuebler/SupraFit.git
```

Compile it as follows on Unix Platform:
```sh
cd suprafit
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release  -Dnoto_font=true/false
make
```
or use the script in the subdirectory. It should automatically update the submodules.
```sh
sh scripts/build_unix.sh 
```
On Windows Systems use for example
```sh
cd suprafit
mkdir build
cd build
```

For Visual Studio use
```sh
cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=Release -Dnoto_font=true/false ..
```


> openMP is disabled when compiling with Visual Studio


or for MinGW (openMP is enabled, libgomp-1.dll is expected) use

```sh
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -Dnoto_font=true/false ..
```

```sh
cmake --build . --config Release
```

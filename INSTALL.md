## Download and requirements
git clones automatically Eigen and ChaiScript. Eigen is used as non-linear optimimization tool and ChaiScript is for now only for experimental stuff in use.

## Compiling
To compile SupraFit you will need CMake 3 or newer and a C++14-capable compiler.

SupraFit has been successfully compilied with: 
- gcc 5.2
- clang 3.9 
on linux systems and 
- mingw53 on windows systems (crosscompiled with wine). 

> Windows 7 or higher is recommended if Qt is compilied without ICU support.

```sh
git clone --recursive git@github.com:contra98/SupraFit.git
cd suprafit
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

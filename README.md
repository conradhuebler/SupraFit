[![Build Status](https://travis-ci.com/contra98/SupraFit.svg?token=kbszubggsBRpbhwmvtTL&branch=master)](https://travis-ci.com/contra98/SupraFit)

# SupraFit 

A Open Source Qt5 based fitting tool for supramolecular titration experiments and Michaelis Menten Kinetics (current development in master)

## Download and requirements
git clones automatically Eigen, ChaiScript and fisher_dist. Eigen is used as non-linear optimimization tool and ChaiScript is for now only for experimental stuff in use. Fisher_dist provides suprafit with a f-value for a given critical p-value.

## Compiling
To compile SupraFit you will need CMake 3 or newer and a C++14-capable compiler.

SupraFit has been successfully compilied with: 
- gcc 5.2 and gcc 6.3
- clang 3.9 
on linux systems and 
- mingw 5.3 on windows systems

> Windows 7 or higher is recommended if Qt is compilied without ICU support.

For SupraFit 1
```sh
git clone -b suprafit-v1 --recursive git@github.com:contra98/SupraFit.git
```
or for the development
```sh
git clone --recursive git@github.com:contra98/SupraFit.git
cd suprafit
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

> the master branch of SupraFit will probably fail to compile if the compiler handles #warning tags to strictly

## Running
### Titration experiments
Start suprafit executable from the build directory. Suprafit handles tables that are composed as follows:

| host | guest | signal1 | signal2 | signal3 |
|:-----:|:----|:----:|:----:|:----:|
| 0.1 | 0 | 1 | 2 | 3 |
| 0.1 | 0.01 | 1.1 | 2.1 | 3.3|

### Michaelis Menten Kinetics (not in SupraFit 1)
| S_0 | v | 
|:-----:|:----:|
| 0.1 | 0 |
| 0.1 | 0.01 |

Copy such a table from any spreadsheet application and paste it in the **New table** dialog or load such a table as `semicolon` or `tabulator seperated file` with **Open File**. 

Suprafit loads and saves tables and calculated models as `json files`.

# SupraFit 

A Open Source Qt5 based fitting tool for supramolecular titration experiments. 

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


## Running
Start suprafit executable from the build directory. Suprafit handles tables that are composed as follows:

| host | guest | signal1 | signal2 | signal3 |
|:-----:|:----|:----:|:----:|:----:|
| 0.1 | 0 | 1 | 2 | 3 |
| 0.1 | 0.01 | 1.1 | 2.1 | 3.3|

Copy such a table from any spreadsheet application and paste it in the **New table** dialog or load such a table as `semicolon` or `tabulator seperated file` with **Import Table**. 

Suprafit loads and saves tables and calculated models as `json files`.

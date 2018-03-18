[![Build Status](https://travis-ci.com/conradhuebler/SupraFit.svg?token=kbszubggsBRpbhwmvtTL&branch=master)](https://travis-ci.com/conradhuebler/SupraFit)

# SupraFit 

A Open Source Qt5 based fitting tool for supramolecular titration experiments (NMR, UV-VIS and Calorimetry) and Michaelis Menten Kinetics

## Download and requirements
git clones automatically Eigen, ChaiScript and fisher_dist. Eigen is used as non-linear optimimization tool and ChaiScript is for now only for experimental stuff in use. Fisher_dist provides suprafit with a f-value for a given critical p-value.

## Compiling
To compile SupraFit you will need CMake 3 or newer and a C++11-capable compiler.

SupraFit has been successfully compilied with: 
- gcc 5.2, gcc 6.3 and gcc 7.3
- clang 3.9 
on linux systems and 
- mingw 5.3 on windows systems

> Windows 7 or higher is recommended if Qt is compilied without ICU support.

For SupraFit 1
```sh
git clone -b suprafit-v1 --recursive git@github.com:conradhuebler/SupraFit.git
```
or for SupraFit 2 (devel version)
```sh
git clone --recursive git@github.com:conradhuebler/SupraFit.git
```
Compile it as follows:
```sh
cd suprafit
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

## Running
Start suprafit executable from the build directory. Suprafit handles tables that are composed as follows:

### Titration experiments (NMR, UV-VIS)
| host | guest | signal1 | signal2 | signal3 |
|:-----:|:----|:----:|:----:|:----:|
| 0.1 | 0 | 1 | 2 | 3 |
| 0.1 | 0.01 | 1.1 | 2.1 | 3.3|

The first two columns contain the concentrations of host and guest, the following columns should contain the NMR or UV/VIS signal.

### ITC data
| v | q |
|:-----:|:----:|
| 0.1 | -12 |
| 0.1 | -11 |

The first column contains the injected volume while the second columns has to contains the integrated heat response of the system.

### Michaelis Menten Kinetics
| S_0 | v | 
|:-----:|:----:|
| 0.1 | 0 |
| 0.1 | 0.01 |

The first column contains the substrat concentration while the second columns has to contains the rate of reaction.

Copy such a table from any spreadsheet application and paste it in the **New table** dialog or load such a table as `semicolon` or `tabulator seperated file` with **Open File**. 

Suprafit loads and saves tables and calculated models as `json files *.json` or compressed json files `*.suprafit`.

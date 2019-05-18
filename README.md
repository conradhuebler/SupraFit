[![Build Status](https://travis-ci.com/conradhuebler/SupraFit.svg?token=kbszubggsBRpbhwmvtTL&branch=master)](https://travis-ci.com/conradhuebler/SupraFit)

![SupraFit Logo](https://github.com/conradhuebler/SupraFit/raw/master/misc/logo_small.png)

# SupraFit 

A Open Source Qt5 based fitting tool for supramolecular titration experiments (NMR, UV-VIS and Calorimetry) and Michaelis Menten Kinetics

## Download and requirements
git clones automatically fisher_dist and libpeakpick.
- [fisher_dis](https://github.com/conradhuebler/fisher_dist) provides the finv-function like in octave to calculate the quantiles of the F distribution
- [libpeakpick](https://github.com/conradhuebler/libpeakpick) provides some basic peak picking, peak integration and regression tools. It retrives a copy of eigen from official git mirror, that is used by SupraFit as well.

## Compiling
To compile SupraFit you will need CMake 3 or newer, a C++11-capable compiler and a recent Qt version. Soon, Qt 5.12 LTS will be focused.

SupraFit has been successfully compilied with: 
- gcc 5.2, gcc 6.3, gcc 7.3 and gcc 8.3
- clang 3.9 
on linux systems and 
- mingw 5.3 on windows systems

> Windows 7 or higher is mandatory.
> MacOS has not been tested yet.

To obtain the most recent development version use
```sh
git clone --recursive git@github.com:conradhuebler/SupraFit.git
```

The deprecated, not recommended version SupraFit 1 can be obtained with
```sh
git clone -b suprafit-v1 --recursive git@github.com:conradhuebler/SupraFit.git
```

Compile it as follows:
```sh
cd suprafit
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

>Please note, that the current development version is still in alpha stage. Some features are not ready yet and some changes might break compatibility of the SupraFit file format.

## Running
Start suprafit executable from the build directory. SupraFit handles tables that are composed as follows:

### Titration experiments (NMR, UV-VIS)
| host | guest | signal1 | signal2 | signal3 |
|:-----:|:----|:----:|:----:|:----:|
| 0.1 | 0 | 1 | 2 | 3 |
| 0.1 | 0.01 | 1.1 | 2.1 | 3.3|

The first two columns contain the concentrations of host (fixed concentration) and guest (variable concentration, silent component), the following columns should contain the NMR or UV/VIS signal.

### ITC data
| v | q |
|:-----:|:----:|
| 0.1 | -12 |
| 0.1 | -11 |

The first column contains the injected volume while the second columns has to contains the integrated heat response of the system.

SupraFit supports import of *.itc files with peak integration and some basic base line corrections. Alternatively *.dH files from Origin can be loaded right away.

### Michaelis Menten Kinetics
| S_0 | v | 
|:-----:|:----:|
| 0.1 | 0 |
| 0.1 | 0.01 |

The first column contains the substrat concentration while the second columns has to contains the rate of reaction.

Copy such a table from any spreadsheet application and paste it in the **New table** dialog or load such a table as `semicolon` or `tabulator seperated file` with **Open File**. 

SupraFit loads and saves tables and calculated models as `json files *.json` or compressed json files `*.suprafit`.

## Statistics
SupraFit provides some statistical analysis, which will be described in an not yet finished article. Implemented methods are based on the following approaches:
- Monte Carlo simulation
- Error-Based analysis
- Resampling methods

A detailed handbook will be provided as soon as possbile.

## Citation
If you obtain results with SupraFit, please cite for now:

After release of the detailed articles describing the used methods, please refer to them as well.

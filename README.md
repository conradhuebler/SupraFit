Linux and Windows Build: [![Build Status](https://github.com/conradhuebler/SupraFit/workflows/AutomaticBuild/badge.svg)](https://github.com/conradhuebler/SupraFit)
[![DOI](https://zenodo.org/badge/55421012.svg)](https://zenodo.org/badge/latestdoi/55421012)

![SupraFit Logo](https://github.com/conradhuebler/SupraFit/raw/master/misc/logo_small.png)

# SupraFit 

A Open Source Qt5 based fitting tool for supramolecular titration experiments (NMR, UV-VIS and Calorimetry) and Michaelis Menten Kinetics.

A short introduction can be downloaded [here](https://github.com/conradhuebler/SupraFit/raw/master/docs/Quickstart.pdf). For question, comments, feedback etc. please use the email adress on page 18 in that Quickstart.

## Download and requirements
git clones automatically fisher_dist and libpeakpick.
- [fisher_dis](https://github.com/conradhuebler/fisher_dist) provides the finv-function like in octave to calculate the quantiles of the F distribution
- [libpeakpick](https://github.com/conradhuebler/libpeakpick) provides some basic peak picking, peak integration and regression tools. It retrives a copy of eigen from official git mirror, that is used by SupraFit as well.

SupraFit comes with the some selected [Google Noto Fonts](https://github.com/googlei18n/noto-fonts). They are optional and can be included into the binary during compile-time (set `-Dnoto_font=true\false` as cmake argument).

## Version
The current stable version is SupraFit 2.0.

- The current master branch contains a snapshot of an improved  thermogram handling, that is developed in the **thermogram** branch. The latest commit without the new branch is [dec3510](https://github.com/conradhuebler/SupraFit/commit/2211c62a327ea8a97c3960229837b44ee1c98511).
- The current master branch contains a snaphsot of an scripting interface, using ChaiScript to define own models. However, this is highly experimental and therefore disabled. The cmake options are ***_Python*** and ***_Models***. Additionally, some a concept for python support is added.

## Compiling
To compile SupraFit you will need [CMake](https://cmake.org/download/) 3 or newer, a C++14-capable compiler and a recent [Qt](https://www.qt.io/download) version. Soon, Qt 5.15 LTS will be focused.

> SupraFit needs QtCharts, so please provide it. It can/should be checked in the Installer Tools from Qt.

SupraFit has been successfully compilied with: 
- gcc 5.2, gcc 6.3, gcc 7.3 and gcc 8.3
- clang 3.9 

on linux systems, on windows systems using
- mingw 5.3
- MSVC 2015, MS 2019

and on macOS 10.12 and 10.13 with the latest [Qt (5.13.1)](https://www.qt.io/download). XCode was downloaded by the Qt Installer, [CMake](https://cmake.org/download/) downloaded and installed manually.

> Windows 7 or higher is mandatory.

Prebuild binaries for Windows (x64) can be downloaded on [SupraFit Releases](https://github.com/conradhuebler/SupraFit/releases).

To obtain SupraFit 2.0 , use
```sh
git clone --recursive -b 2.0  https://github.com/conradhuebler/SupraFit.git
```

To obtain the most recent development version, which is SupraFit 2.x , use
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

On macOS, an easy way to compile SupraFit is using QtCreator.

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

SupraFit supports import of *.itc and plain x-y files with peak integration and some basic base line corrections. Alternatively *.dH files from Origin can be loaded right away.
Plain x-y files for thermograms should look like:

|   |   |
|:-----:|:----:|
| 2 | -0.001 |
| 4 | -0.001 |
| 6 | -0.002 |
| 8 | -0.002 |
| 10 | -0.002 |
| 12 | -0.001 |

With the first row having the time and the second the observed heat.

### Michaelis Menten Kinetics
| S_0 | v | 
|:-----:|:----:|
| 0.1 | 0 |
| 0.1 | 0.01 |

The first column contains the substrat concentration while the second columns has to contains the rate of reaction.

Copy such a table from any spreadsheet application and paste it in the **New table** dialog or load such a table as `semicolon` or `tabulator seperated file` with **Open File**. 

SupraFit loads and saves tables and calculated models as `json files *.json` or compressed json files `*.suprafit`.

## Statistics
SupraFit provides some statistical analysis, which will be described in a not yet finished article. Implemented methods are based on the following approaches:
- Monte Carlo simulation (Percentile method based confidence calculation)
- F-Test based confidence calculation
- Resampling methods

A detailed handbook will be provided as soon as possbile.

## Acknowledgments
Special thanks to Prof. M. Mazik [Institut for organic Chemistry, TU Bergakademie Freiberg](https://tu-freiberg.de/fakultaet2/orgch) for her support.

Special thanks to Dr. Sebastian Förster and Dr. Stefan Kaiser for finding bugs and constructive feedback.

Special thanks to the [Centre of Advanced Study and Research - Freiberg (GraFA)](https://tu-freiberg.de/grafa) for funding.

## Citation and more
If you obtain results with SupraFit, I kindly ask to include in your citation:

Conrad Hübler, 2019, DOI [10.5281/zenodo.3364569](https://doi.org/10.5281/zenodo.3364569).

### Poster presentation at Physical-Organic Chemistry at its Best: The Art of Chemical Problem Solving (13.09 and 14.09 2018)
<img src="https://github.com/conradhuebler/SupraFit/raw/master/docs/SupraFit_Poster.png" width="300">

### SupraFit has been used in
Anthracene-Based Receptors with a Turn-on Fluorescence Response for Nitrate [Org. Lett. 2019, 21, 21, 8746-8750](https://pubs.acs.org/doi/abs/10.1021/acs.orglett.9b03361)

Purine Unit as a Building Block of Artificial Receptors Designed for the Recognition of Carbohydrates [Eur. J. Org. Chem., 2019: 7555-7562](https://onlinelibrary.wiley.com/doi/full/10.1002/ejoc.201901340)

## Some notes
- SupraFit prefers larger screens over smaller ones. 1600x1200 or 1680x1050 is the recommended size.
- SupraFit is being developed on a Linux platform, so some platform dependent errors or layout problems may have not been observed yet.

Have a lot of fun!

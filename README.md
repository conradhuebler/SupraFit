Linux and Windows Build: [![Build Status](https://github.com/conradhuebler/SupraFit/workflows/Build/badge.svg)](https://github.com/conradhuebler/SupraFit/actions?query=workflow%3ABuild)
[![DOI](https://zenodo.org/badge/55421012.svg)](https://zenodo.org/badge/latestdoi/55421012)
[![CodeFactor](https://www.codefactor.io/repository/github/conradhuebler/suprafit/badge)](https://www.codefactor.io/repository/github/conradhuebler/suprafit)

![SupraFit Logo](https://github.com/conradhuebler/SupraFit/raw/master/misc/logo_small.png)

# SupraFit 

A Open Source Qt6 based fitting tool for supramolecular titration experiments (NMR, UV-VIS and Calorimetry), Michaelis Menten Kinetics and indidual custom models. Custom models are work in progress and not yet well documented. For the start, please have a look at [here](https://github.com/conradhuebler/SupraFit/raw/master/docs/ScriptedModels.md)

A short introduction can be downloaded [here](https://github.com/conradhuebler/SupraFit/raw/master/docs/Quickstart.pdf). For question, comments, feedback etc. please use the email adress on page 18 in that Quickstart.

## Getting SupraFit
[SupraFit 2.0 binaries](https://github.com/conradhuebler/SupraFit/releases/tag/2.0.0) are available for Linux (as AppImage), for Windows and macOS. 

Windows users please note, that SupraFit requires the Microsoft Visual C++ Redistributable for Visual Studio 2015, 2017 and 2019 to be installed, which includes for example msvcp140.dll and msvcp140_1.dll.
It may be downloaded [from the official website.](https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads)

### History
- [SupraFit 2.0](https://github.com/conradhuebler/SupraFit/releases/tag/2.0.0) stable version, first public release
- [SupraFit 2.x](https://github.com/conradhuebler/SupraFit/releases) most recent development version. Automatic builds for Windows and Linux are not work currently.


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

## Download source code and requirements
git clones automatically fisher_dist and libpeakpick.
- [fisher_dis](https://github.com/conradhuebler/fisher_dist) provides the finv-function like in octave to calculate the quantiles of the F distribution
- [libpeakpick](https://github.com/conradhuebler/libpeakpick) provides some basic peak picking, peak integration and regression tools. It retrives a copy of eigen from official git mirror, that is used by SupraFit as well.
- [CuteCharts](https://github.com/conradhuebler/CuteChart) QtCharts adopted for SupraFit
- [ChaiScript](https://github.com/ChaiScript/ChaiScript) and [ChaiScriptExtra](https://github.com/ChaiScript/ChaiScript_Extras) The current development version (2.x) contains limited scripting implementation using ChaiScript.

SupraFit comes with the some selected [Google Noto Fonts](https://github.com/googlei18n/noto-fonts). They are optional and can be included into the binary during compile-time (set `-Dnoto_font=true\false` as cmake argument).

- The current master branch contains a snapshot of an improved  thermogram handling, that is developed in the **thermogram** branch. The latest commit without the new branch is [dec3510](https://github.com/conradhuebler/SupraFit/commit/2211c62a327ea8a97c3960229837b44ee1c98511).
- The current master branch contains a snaphsot of a scripting interface, using ChaiScript to define own models. However, this is highly experimental and therefore disabled. The cmake options are ***_Python*** and ***_Models***. Additionally, some a concept for python support is added.
- The current master branch contains a snaphsot of a spectra import interface.  The latest commit without the new branch is [b5af8cd](https://github.com/conradhuebler/SupraFit/commit/b5af8cd9e8c29792c15b893aee8bcffa8a19dd8d).

## Compiling
To compile SupraFit you will need [CMake](https://cmake.org/download/) 3 or newer, a C++14-capable compiler and [Qt 5.15](https://www.qt.io/download).

> SupraFit needs QtCharts, so please provide it. It can/should be checked in the Installer Tools from Qt.

SupraFit has been successfully compilied with: 
- gcc 5.2 and newer versions
- clang 3.9 

on linux systems, on windows systems using
- mingw 5.3
- MSVC 2015, MS 2019

and on macOS 10.12 and 10.13 with the latest [Qt (5.15.1)](https://www.qt.io/download). XCode was downloaded by the Qt Installer, [CMake](https://cmake.org/download/) downloaded and installed manually.

> Windows 7 or higher is mandatory.

To obtain SupraFit 2.0, use
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

## Acknowledgments
Special thanks to Prof. M. Mazik [Institut for organic Chemistry, TU Bergakademie Freiberg](https://tu-freiberg.de/fakultaet2/orgch) for her support.

Special thanks to Dr. Sebastian Förster and Dr. Stefan Kaiser for finding bugs and constructive feedback.

Special thanks to the [Centre of Advanced Study and Research - Freiberg (GraFA)](https://tu-freiberg.de/grafa) for funding.

## Citation and more
If you obtain results with SupraFit, I kindly ask to include in your citation:

Conrad Hübler, 2019, DOI [10.5281/zenodo.3364569](https://doi.org/10.5281/zenodo.3364569).

A preprint article has been published:
https://chemrxiv.org/engage/chemrxiv/article-details/61e0923080719d23820e97bf

C. Hübler, ChemRxiv 2022, DOI 10.26434/chemrxiv-2022-c1jwr. This content is a preprint and has not been peer-reviewed.

### Poster presentation at Physical-Organic Chemistry at its Best: The Art of Chemical Problem Solving (13.09 and 14.09 2018)
<img src="https://github.com/conradhuebler/SupraFit/raw/master/docs/SupraFit_Poster.png" width="300">

### SupraFit has been used in

- Anthracene-Based Receptors with a Turn-on Fluorescence Response for Nitrate [Org. Lett. 2019, 21, 21, 8746-8750](https://pubs.acs.org/doi/abs/10.1021/acs.orglett.9b03361)
- Purine Unit as a Building Block of Artificial Receptors Designed for the Recognition of Carbohydrates [Eur. J. Org. Chem., 2019: 7555-7562](https://onlinelibrary.wiley.com/doi/full/10.1002/ejoc.201901340)
- Cycloalkyl Groups as Subunits of Artificial Carbohydrate Receptors: Effect of Ring Size of the Cycloalkyl Unit on the Receptor Efficiency [Eur. J. Org. Chem. 2020, 4900–4915](https://doi.org/10.1002/ejoc.202000803)

## Some notes
- SupraFit prefers larger screens over smaller ones. 1600x1200 or 1680x1050 is the recommended size.
- SupraFit is being developed on a Linux platform, so some platform dependent errors or layout problems may have not been observed yet.

Have a lot of fun!

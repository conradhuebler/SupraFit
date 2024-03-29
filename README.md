Linux and Windows Build: [![Build Status](https://github.com/conradhuebler/SupraFit/workflows/Build/badge.svg)](https://github.com/conradhuebler/SupraFit/actions?query=workflow%3ABuild)
[![DOI](https://zenodo.org/badge/55421012.svg)](https://zenodo.org/badge/latestdoi/55421012)
[![CodeFactor](https://www.codefactor.io/repository/github/conradhuebler/suprafit/badge)](https://www.codefactor.io/repository/github/conradhuebler/suprafit)

![SupraFit Logo](https://github.com/conradhuebler/SupraFit/raw/master/misc/logo_small.png)

# SupraFit 

A Open Source Qt6 based fitting tool for supramolecular titration experiments (NMR, UV-VIS and Calorimetry), Michaelis Menten Kinetics and indidual custom models. 

A short introduction can be downloaded [here](https://github.com/conradhuebler/SupraFit/raw/master/docs/Quickstart.pdf). For question, comments, feedback etc. please use the email adress on page 18 in that Quickstart.

## Getting SupraFit
[SupraFit 2.0 binaries](https://github.com/conradhuebler/SupraFit/releases/tag/2.0.0) are available for Linux (as AppImage), for Windows and macOS. 

Windows users please note, that SupraFit 2.0 requires the Microsoft Visual C++ Redistributable for Visual Studio 2015, 2017 and 2019 to be installed, which includes for example msvcp140.dll and msvcp140_1.dll.
It may be downloaded [from the official website.](https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads) This holds not true for the most recent Nightly Build for Windows platforms. They are compilied with MinGW and all dlls are shipped with the zip archive.

### History
- [SupraFit 2.0](https://github.com/conradhuebler/SupraFit/releases/tag/2.0.0) stable version, first public release
- [SupraFit 2.x](https://github.com/conradhuebler/SupraFit/releases) most recent development version. Automatic builds for Windows and Linux are not work currently.


## Running
### Windows
Start SupraFit (**suprafit.exe**) from the build directory. The dlls have to be in place. Desktop shortcuts will work though.

### Linux
Run the AppImage. It may have to be marked as executable: **chmod +x SupraFit*.AppImage**

### macOS
The content of the dmg file has to be copied to the place where other programs are stored. Please not, that SupraFit on macOS has not been tested at. If there are any problems, please feel free to create an issue or contact me directly.

### Nightly Build
Latest snapshots (not more than 5) of the current development can be found via the preleases. There are some new features and bugs included.

- Custom models are work in progress and not yet well documented. For the start, please have a look at [here](https://github.com/conradhuebler/SupraFit/blob/master/docs/ScriptedModels.md) (Qt 6)
- The current master branch contains a snapshot of an improved  thermogram handling, that was developed in the **thermogram** branch. The latest commit without the new branch is [dec3510](https://github.com/conradhuebler/SupraFit/commit/2211c62a327ea8a97c3960229837b44ee1c98511). (Qt 5)
- The current master branch contains a snaphsot of a spectra import interface.  The latest commit without the new branch is [b5af8cd](https://github.com/conradhuebler/SupraFit/commit/b5af8cd9e8c29792c15b893aee8bcffa8a19dd8d). (Qt 5)

## Usage
SupraFit handles tables that are composed as follows:

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

## Constrained Optimisation
Global parameters (local will follow) can now be optimised with respect to boundary conditations. The limits can be set by clicking the three-dotted push button next to the parameter value. This is quite an experimental feature, since the correct math is somehow complicated. 
The boundary conditions in SupraFit are realised using a logfermi potential as penalty function (see https://xtb-docs.readthedocs.io/en/latest/xcontrol.html#different-potential-shapes). For best optimisation experience, make sure that the initial guess of the parameters lies within the choosen boundaries.


## Statistics
SupraFit provides some statistical analysis. Implemented methods are based on the following approaches:
- Monte Carlo simulation (Percentile method based confidence calculation)
- F-Test based confidence calculation
- Resampling methods

The Monte Carlo simulation and F-Test based approches are explained in C. Hübler, Chem. Methods 2022, e202200006. DOI [10.1002/cmtd.202200006](https://doi.org/10.1002/cmtd.202200006). Further analysis using Monte Carlo simulation and the Resampling methods are described in more detail in Hübler C. 2022. Analysing binding stoichiometries in NMR titration experiments using Monte Carlo simulation and resampling techniques. PeerJ Analytical Chemistry 4:e23 [https://doi.org/10.7717/peerj-achem.23](https://peerj.com/articles/achem-23/). 
A detailed handbook will be provided as soon as possbile.

## Download source code and requirements
git clones automatically fisher_dist and libpeakpick.
- [fisher_dis](https://github.com/conradhuebler/fisher_dist) provides the finv-function like in octave to calculate the quantiles of the F distribution
- [libpeakpick](https://github.com/conradhuebler/libpeakpick) provides some basic peak picking, peak integration and regression tools. It retrives a copy of eigen from official git mirror, that is used by SupraFit as well.
- [CuteCharts](https://github.com/conradhuebler/CuteChart) QtCharts adopted for SupraFit
- [ChaiScript](https://github.com/ChaiScript/ChaiScript) and [ChaiScriptExtra](https://github.com/ChaiScript/ChaiScript_Extras) The current development version (2.x) contains limited scripting implementation using ChaiScript.

SupraFit comes with the some selected [Google Noto Fonts](https://github.com/googlei18n/noto-fonts). They are optional and can be included into the binary during compile-time (set `-Dnoto_font=true\false` as cmake argument).


## Compiling
To compile SupraFit you will need [CMake](https://cmake.org/download/) 3.21 or newer, a C++14-capable compiler and [Qt 6.2](https://www.qt.io/download).

> SupraFit needs QtCharts, so please provide it. It can/should be checked in the Installer Tools from Qt.

SupraFit has been successfully compilied with: 
- gcc 5.2 and newer versions
- clang 3.9 

on linux systems, on windows systems using
- mingw 5.3 or newer

MSVC 2015, MS 2019 builds failed with Qt 6. Before the port, MSVC worked well.

Compiling works on macOS 10.15 with the latest [Qt (6.2.0)](https://www.qt.io/download). XCode was downloaded by the Qt Installer, [CMake](https://cmake.org/download/) downloaded and installed manually.

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

Special thanks to Dr. Sebastian Förster, Dr. Stefan Kaiser and Dr. Felix Amrhein for finding bugs and constructive feedback.

Special thanks to the [Centre of Advanced Study and Research - Freiberg (GraFA)](https://tu-freiberg.de/grafa) and Saxonian Ministry of Science, Culture and Tourism (SMWK) for funding.

## Citation and more
If you obtain results with SupraFit, I kindly ask to include in your citation:

- C. Hübler, Chem. Methods 2022, e202200006. DOI [10.1002/cmtd.202200006](https://doi.org/10.1002/cmtd.202200006)
- C. Hübler, 2019, DOI [10.5281/zenodo.3364569](https://doi.org/10.5281/zenodo.3364569).

If the Monte Carlo simulation and Resampling plans were helpfull:

- C. Hübler, PeerJ Analytical Chemistry 2022, 4:e23 [https://doi.org/10.7717/peerj-achem.23](https://peerj.com/articles/achem-23/)


### Poster presentation at Physical-Organic Chemistry at its Best: The Art of Chemical Problem Solving (13.09 and 14.09 2018)
<img src="https://github.com/conradhuebler/SupraFit/raw/master/docs/SupraFit_Poster.png" width="300">

### SupraFit has been used in
- Compounds Combining a Macrocyclic Building Block and Flexible Side-Arms as Carbohydrate Receptors: Syntheses and Structure-Binding Activity Relationship Studies [Eur. J. Org. Chem. 2021, 2021, 6282.](https://doi.org/10.1002/ejoc.202100758)
- Cycloalkyl Groups as Subunits of Artificial Carbohydrate Receptors: Effect of Ring Size of the Cycloalkyl Unit on the Receptor Efficiency [Eur. J. Org. Chem. 2020, 4900–4915](https://doi.org/10.1002/ejoc.202000803)
- Purine Unit as a Building Block of Artificial Receptors Designed for the Recognition of Carbohydrates [Eur. J. Org. Chem., 2019: 7555-7562](https://onlinelibrary.wiley.com/doi/full/10.1002/ejoc.201901340)
- Anthracene-Based Receptors with a Turn-on Fluorescence Response for Nitrate [Org. Lett. 2019, 21, 21, 8746-8750](https://pubs.acs.org/doi/abs/10.1021/acs.orglett.9b03361)

## Some notes
- SupraFit prefers larger screens over smaller ones. 1600x1200 or 1680x1050 is the recommended size.
- SupraFit is being developed on a Linux platform, so some platform dependent errors or layout problems may have not been observed yet.

Have a lot of fun!

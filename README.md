# SupraFit 

A Open Source Qt5 based fitting tool for supramolecular titration experiments. 

## Download and installation:
git clones automatically Eigen and ChaiScript. Eigen is used as non-linear optimimization tool and ChaiScript is for now only for experimental stuff in use.
SupraFit has been successfully compilied with gcc 5.2 and clang 3.9 on linux systems and mingw53 on windows systems (wine). Windows 7 or higher is recommended if Qt is compilied without ICU support.

```
git clone --recursive git@github.com:contra98/SupraFit.git
cd suprafit
mkdir build
cd build
cmake ..
make
```
## Running
Start suprafit executable from the build directory. Suprafit handles tables that are composed in the following:
| host | guest | signal1 | signal2 | signal3 |
| 0.1 | 0 | 1 | 2 | 3 |
| 0.1 | 0.01 | 1.1 | 2.1 | 3.3|



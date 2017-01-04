Download or clone source code:
```
git clone git@github.com:contra98/nmr2fit.git
cd nmr2fit
mkdir build
cd build
```
As external optimizer for nonlinear fitting you can use either levmar or eigen.

Eigen is less stable than levmar (default) and might lead to crashs but is easier to install.

Get levmar from this site: http://users.ics.forth.gr/~lourakis/levmar/
and
Eigen from: http://eigen.tuxfamily.org/index.php?title=Main_Page


```
cmake ..
make
```

#!/bin/bash
set -ex

cp misc/SupraFit.desktop bin/linux/
cp misc/SupraFit.png bin/linux/
cd release/bin/linux/

wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt-continuous-x86_64.AppImage

#./linuxdeployqt-continuous-x86_64.AppImage SupraFit.desktop -qmake=$HOME/5.15.1/gcc_64/bin/qmake -appimage
./linuxdeployqt-continuous-x86_64.AppImage SupraFit.desktop -appimage
cp SupraFit*.AppImage SupraFit-nightly-x86_64.AppImage

#!/bin/bash
set -ex

cp misc/SupraFit.desktop release/bin/linux/
cp misc/SupraFit.png release/bin/linux/
cd release/bin/

wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
wget -c -nv "https://github.com/AppImage/AppImageKit/releases/download/13/appimagetool-x86_64.AppImage"
chmod a+x linuxdeployqt-continuous-x86_64.AppImage
chmod a+x appimagetool-x86_64.AppImage

cd linux

#./linuxdeployqt-continuous-x86_64.AppImage SupraFit.desktop -qmake=$HOME/5.15.1/gcc_64/bin/qmake -appimage
mkdir -p lib
cd lib
for i in $(ldd ../suprafit |awk '{ print $3}' |grep Qt); do cp $i .; done
for i in $(ldd ../suprafit |awk '{ print $3}' |grep libic); do cp $i .; done

#cp /usr/lib/libQt6Widgets.so* .
#cp /usr/lib/libQt6OpenGLWidgets.so* .
#cp /usr/lib/libQt6OpenGL.so* .
#cp /usr/lib/libQt6Gui.so* .
#cp /usr/lib/libQt6DBus.so* .
#cp /usr/lib/libQt6Charts.so* .
#cp /usr/lib/libQt6Core.so* .

#cp /usr/lib/libicudata.so* .
#cp /usr/lib/libicui18n.so* .
#cp /usr/lib/libicuuc.so* .
cd ..
cp -r /usr/lib/qt6/plugins .

../linuxdeployqt-continuous-x86_64.AppImage SupraFit.desktop -unsupported-allow-new-glibc || true 
../appimagetool-x86_64.AppImage .

cp SupraFit*.AppImage SupraFit-nightly-x86_64.AppImage

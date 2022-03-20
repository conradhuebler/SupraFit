#!/bin/bash
set -ex
mkdir -p release/bundle

mkdir -p release/bundle/share
mkdir -p release/bundle/share/suprafit
mkdir -p release/bundle/lib
mkdir -p release/bundle/plugins
mkdir -p release/bundle/bin

cd release
cp -r bin/linux/* bundle/bin


cp ../misc/SupraFit.desktop bundle/share/suprafit
cp ../misc/SupraFit.png bundle/share/suprafit

cd bundle

cd plugins

cp -r  $(find $HOME -type f -name '*libqxcb-glx-integration*' -print 2>/dev/null |head -n1 |sed 's/xcbglintegrations\/libqxcb-glx-integration.so/iconengines/g') .
cp -r  $(find $HOME -type f -name '*libqxcb-glx-integration*' -print 2>/dev/null |head -n1 |sed 's/xcbglintegrations\/libqxcb-glx-integration.so/imageformats/g') .
cp -r  $(find $HOME -type f -name '*libqxcb-glx-integration*' -print 2>/dev/null |head -n1 |sed 's/xcbglintegrations\/libqxcb-glx-integration.so/platforms/g') .
cp -r  $(find $HOME -type f -name '*libqxcb-glx-integration*' -print 2>/dev/null |head -n1 |sed 's/xcbglintegrations\/libqxcb-glx-integration.so/platforminputcontexts/g') .
cp -r  $(find $HOME -type f -name '*libqxcb-glx-integration*' -print 2>/dev/null |head -n1 |sed 's/xcbglintegrations\/libqxcb-glx-integration.so/xcbglintegrations/g') .
cd ..

#cat <<ab>qt.conf
#[Paths]
#Prefix = ./
#Plugins = plugins
#Imports = qml
#Qml2Imports = qml
#ab

cd lib
cp -r  $(find $HOME -type f -name '*libicudata.so.56' -print 2>/dev/null |head -n1) .
cp -r  $(find $HOME -type f -name '*libicui18n.so.56' -print 2>/dev/null |head -n1) .
cp -r  $(find $HOME -type f -name '*libicuuc.so.56' -print 2>/dev/null |head -n1) .
cp -r  $(find $HOME -type f -name '*libQt6Charts.so.6' -print 2>/dev/null |head -n1) .
cp -r  $(find $HOME -type f -name '*libQt6Core.so.6' -print 2>/dev/null |head -n1) .
cp -r  $(find $HOME -type f -name '*libQt6DBus.so.6' -print 2>/dev/null |head -n1) .
cp -r  $(find $HOME -type f -name '*libQt6Gui.so.6' -print 2>/dev/null |head -n1) .
cp -r  $(find $HOME -type f -name '*libQt6OpenGL.so.6' -print 2>/dev/null |head -n1) .
cp -r  $(find $HOME -type f -name '*libQt6OpenGLWidgets.so.6' -print 2>/dev/null |head -n1) .
cp -r  $(find $HOME -type f -name '*libQt6Widgets.so.6' -print 2>/dev/null |head -n1) .
cp -r  $(find $HOME -type f -name '*libQt6XcbQpa.so.6' -print 2>/dev/null |head -n1) .

cd ..
cd bin
cat <<ab>suprafit.bash
#!/bin/bash
export LD_LIBRARY_PATH=../lib
# use the following for large screens
#  QT_USE_PHYSICAL_DPI=1 ./suprafit.bash 
./suprafit
ab
chmod +x suprafit.bash
cd ..
cd ..
mv bundle SupraFit
tar -czvf SupraFit.tar.gz SupraFit

#!/usr/bin/env bash

export APP_NAME="ResourceDragon"
export APP_VERSION=0.0.1
export GIT_VERSION=$(git rev-parse --short HEAD)

#export APP_NAME_LOWERCASE=${APP_NAME,,}  # lowercase
export APP_NAME_LOWERCASE=$APP_NAME       # not actually lowercase

echo "> $APP_NAME packager (Linux x86_64) [v$APP_VERSION]"

## CHECKS ######################################################################

if [ "$(id -u)" == "0" ]; then
  echo "This script MUST NOT be run as root" 1>&2
  exit 1
fi

if [ ${PWD##*/} != $APP_NAME ]; then
  echo "This script MUST be run from the $APP_NAME/ directory"
  exit 1
fi

## SETTINGS ####################################################################

use_contribs=false
ninja_install=false
create_package=false

while [[ $# -gt 0 ]]
do
case $1 in
  -c|--contribs)
  use_contribs=true
  ;;
  -i|--install)
  ninja_install=true
  ;;
  -p|--package)
  create_package=true
  ;;
  *)
  echo "> Unknown argument '$1'"
  ;;
esac
shift # skip argument or value
done

## linuxdeploy INSTALL #########################################################

if [[ $use_contribs = true ]] ; then
  export LD_LIBRARY_PATH=$(pwd)/contribs/src/env/linux_x86_64/usr/lib/:/usr/lib
else
  export LD_LIBRARY_PATH=/usr/lib/
fi

echo '---- Prepare linuxdeploy + plugins'

# linuxdeploy and plugins
if [ ! -x contribs/deploy/linuxdeploy-x86_64.AppImage ]; then
  wget -c -nv "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" -P contribs/deploy/
  wget -c -nv "https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage" -P contribs/deploy/
fi
chmod a+x contribs/deploy/linuxdeploy-x86_64.AppImage
chmod a+x contribs/deploy/linuxdeploy-plugin-appimage-x86_64.AppImage

## APP INSTALL #################################################################

if [[ $ninja_install = true ]] ; then
  echo '---- Running ninja install'
  ninja INSTALL_ROOT=bin/ install

  #echo '---- Installation directory content recap (after make install):'
  #find bin/
fi

## PACKAGE (AppImage) ##########################################################

if [[ $create_package = true ]] ; then
  echo '---- Format appdir'
  mkdir -p bin/usr/bin/
  mkdir -p bin/usr/share/appdata/
  mkdir -p bin/usr/share/applications/
  mkdir -p bin/usr/share/pixmaps/
  mkdir -p bin/usr/share/icons/hicolor/scalable/apps/
  mv build/$APP_NAME bin/usr/bin/$APP_NAME
  cp assets/linux/$APP_NAME_LOWERCASE.appdata.xml bin/usr/share/appdata/$APP_NAME_LOWERCASE.appdata.xml
  cp assets/linux/$APP_NAME_LOWERCASE.desktop bin/usr/share/applications/$APP_NAME_LOWERCASE.desktop
  cp assets/linux/$APP_NAME_LOWERCASE.png bin/usr/share/pixmaps/$APP_NAME_LOWERCASE.png
  cp assets/linux/$APP_NAME_LOWERCASE.png bin/usr/share/icons/hicolor/128x128/apps/$APP_NAME_LOWERCASE.png

  echo '---- Running AppImage packager'
  ./contribs/deploy/linuxdeploy-x86_64.AppImage --appdir bin --output appimage
  mv $APP_NAME-x86_64.AppImage $APP_NAME-$APP_VERSION-linux64.AppImage

  #echo '---- Installation directory content recap (after linuxdeploy):'
  #find bin/
fi

## PACKAGE (archive) ###########################################################

if [[ $create_package = true ]] ; then
  echo '---- Reorganize appdir into a regular directory'
  mkdir bin/$APP_NAME/
  mv bin/usr/bin/* bin/$APP_NAME/
  mv bin/usr/lib/* bin/$APP_NAME/
  mv bin/usr/share/appdata/$APP_NAME_LOWERCASE.appdata.xml bin/$APP_NAME/
  mv bin/usr/share/applications/$APP_NAME_LOWERCASE.desktop bin/$APP_NAME/
  mv bin/usr/share/pixmaps/$APP_NAME_LOWERCASE.png bin/$APP_NAME/
  printf '#!/bin/sh\nappname=`basename $0 | sed s,\.sh$,,`\ndirname=`dirname $0`\nexport LD_LIBRARY_PATH=$dirname\n$dirname/$appname' > bin/$APP_NAME/$APP_NAME_LOWERCASE.sh
  chmod +x bin/$APP_NAME/$APP_NAME_LOWERCASE.sh

  echo '---- Compressing package'
  cd bin
  tar zcvf ../$APP_NAME-$APP_VERSION-linux64.tar.gz $APP_NAME/
fi

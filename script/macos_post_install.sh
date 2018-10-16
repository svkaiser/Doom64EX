#!/bin/bash

mkdir -p ${MESON_INSTALL_PREFIX}/Contents/Frameworks

# Install SDL2 Framework
cp -R /Library/Frameworks/SDL2.framework ${CMAKE_INSTALL_PREFIX}/Contents/Frameworks
install_name_tool -change @rpath/SDL2.framework/Versions/A/SDL2 \
                  @executable_path/../Frameworks/SDL2.framework/Versions/A/SDL2 \
                  ${MESON_INSTALL_PREFIX}/Contents/MacOS/Doom64EX

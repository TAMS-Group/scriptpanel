#!/bin/bash
git submodule init
git submodule update
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make
make install
cd ..
cp script_panel.desktop.in $HOME/.local/share/applications/script_panel.desktop
cp scriptpanel.png $HOME/.local/share/icons/hicolor/128x128/apps/scriptpanel.png
cp scriptpanel.svg $HOME/.local/share/icons/hicolor/scalable/scriptpanel.svg

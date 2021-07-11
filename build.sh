#!/bin/bash
git submodule init
git submodule update
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/bin
make
make install
cd ..
sed -e "s,Icon=.*,Icon=$PWD/scriptpanel.svg,g" script_panel.desktop-template > script_panel.desktop
chmod +x script_panel.desktop
cp script_panel.desktop $HOME/Desktop

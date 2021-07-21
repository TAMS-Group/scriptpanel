#!/bin/sh
git submodule init &&
git submodule update &&
mkdir -p build &&
cd build &&
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local &&
make &&
make install

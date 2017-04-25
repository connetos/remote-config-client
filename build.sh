#!/bin/bash

#1) Compile libssh and install it in ./ 
rm -rf libssh-0.7.5/build
mkdir -p libssh-0.7.5/build
cd libssh-0.7.5/build
cmake -DCMAKE_INSTALL_PREFIX=../.. -DWITH_STATIC_LIB=ON -DCMAKE_BUILD_TYPE=Debug ..
make
make install
cd ../..

#2) Compile rcc
make


#!/bin/bash

set -e

MULTITHREADED_FLAG=""
if [[ "$1" == "--multithreaded" ]]; then
  MULTITHREADED_FLAG="-DMULTITHREADED=ON"
  echo "Compiling rix-cpp with MULTITHREADED=ON"
fi

echo "Installing Rix to $HOME/.rix"

echo "Installing rix-msg ..."
git clone https://github.com/rix-ros/rix-msg.git
cd rix-msg
bash install.bash
cd ..
rm -rf rix-msg
echo "rix-msg installed."

echo "Installing rix-py ..."
git clone https://github.com/rix-ros/rix-py.git
cd rix-py
bash install.bash
cd ..
rm -rf rix-py
echo "rix-py installed."

echo "Installing Eigen 3.4.0 ..."
wget https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
tar -xvf eigen-3.4.0.tar.gz
cd eigen-3.4.0
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=~/.rix/ ..
make install -j4
cd ../..
rm -rf eigen-3.4.0 eigen-3.4.0.tar.gz

echo "Installing nlohmann/json 3.11.3 ..."
wget https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz
tar -xvf v3.11.3.tar.gz
cd json-3.11.3
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=~/.rix/ ..
make install -j4
cd ../..
rm -rf json-3.11.3 v3.11.3.tar.gz

echo "Installing rix-cpp ..."
mkdir -p build
cd build
cmake -DCMAKE_PREFIX_PATH=$HOME/.rix/ -DCMAKE_INSTALL_PREFIX=$HOME/.rix/ $MULTITHREADED_FLAG ..
make -j4 $MULTITHREADED_FLAG ..
make -j4
make install
cd ..
echo "rix-cpp installed."

cp setup.bash $HOME/.rix/setup.bash

echo "Rix installed successfully."
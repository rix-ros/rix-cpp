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

echo "Installing rix-cpp ..."
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=$HOME/.rix/ -DCMAKE_INSTALL_PREFIX=$HOME/.rix/ $MULTITHREADED_FLAG ..
make -j4
make install
cd ..
echo "rix-cpp installed."

echo "Sourcing setup.bash ..."
cp setup.bash $HOME/.rix/setup.bash
source $HOME/.rix/setup.bash

echo "Rix installed successfully."
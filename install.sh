git clone https://github.com/rix-ros/rix-msg.git
cd rix-msg
bash install.sh
cd ..
rm -rf rix-msg

git clone https://github.com/rix-ros/rix-py.git
cd rix-py
bash install.sh
cd ..
rm -rf rix-py

mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=$HOME/.rix/ -DCMAKE_INSTALL_PREFIX=$HOME/.rix/ ..
make -j4
make install
cd ..
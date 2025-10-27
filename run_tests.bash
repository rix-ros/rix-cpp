#!/bin/bash

set -e

mkdir -p build
cd build
cmake -DMULTITHREADED=OFF -DBUILD_TESTS=ON ..
make -j install

echo "Running tests (multithreaded off)..."
./node_test
./mediator_test
./message_test

echo "Running example tests (multithreaded off)..."
cd ../examples/simple_publisher
mkdir -p build
cd build
cmake -DCMAKE_PREFIX_PATH=$HOME/.rix/ -DBUILD_TESTS=ON ..
make -j
./simple_publisher_test

cd ../../simple_subscriber
mkdir -p build
cd build
cmake -DCMAKE_PREFIX_PATH=$HOME/.rix/ -DBUILD_TESTS=ON ..
make -j
./simple_subscriber_test

cd ../../simple_service
mkdir -p build
cd build
cmake -DCMAKE_PREFIX_PATH=$HOME/.rix/ -DBUILD_TESTS=ON ..
make -j
./simple_service_test

cd ../../simple_service_client
mkdir -p build
cd build
cmake -DCMAKE_PREFIX_PATH=$HOME/.rix/ -DBUILD_TESTS=ON ..
make -j
./simple_service_client_test

cd ../../simple_action
mkdir -p build
cd build
cmake -DCMAKE_PREFIX_PATH=$HOME/.rix/ -DBUILD_TESTS=ON ..
make -j
./simple_action_test

cd ../../../build
cmake -DMULTITHREADED=ON -DBUILD_TESTS=ON ..
make -j install

echo "Running tests (multithreaded on)..."
./node_test
./mediator_test
./message_test

echo "Running example tests (multithreaded on)..."
cd ../examples/simple_publisher
mkdir -p build
cd build
cmake -DCMAKE_PREFIX_PATH=$HOME/.rix/ -DBUILD_TESTS=ON ..
make -j
./simple_publisher_test

cd ../../simple_subscriber
mkdir -p build
cd build
cmake -DCMAKE_PREFIX_PATH=$HOME/.rix/ -DBUILD_TESTS=ON ..
make -j
./simple_subscriber_test

cd ../../simple_service
mkdir -p build
cd build
cmake -DCMAKE_PREFIX_PATH=$HOME/.rix/ -DBUILD_TESTS=ON ..
make -j
./simple_service_test

cd ../../simple_service_client
mkdir -p build
cd build
cmake -DCMAKE_PREFIX_PATH=$HOME/.rix/ -DBUILD_TESTS=ON ..
make -j
./simple_service_client_test

cd ../../simple_action
mkdir -p build
cd build
cmake -DCMAKE_PREFIX_PATH=$HOME/.rix/ -DBUILD_TESTS=ON ..
make -j
./simple_action_test
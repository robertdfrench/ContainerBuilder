#!/usr/bin/env bash

set -e

#apt update
#apt -y upgrade

# Create non root queue user
useradd --create-home --home-dir /home/queue --shell /bin/bash queue

# Update cmake version
cd /
wget https://cmake.org/files/v3.9/cmake-3.9.5-Linux-x86_64.sh
chmod +x ./cmake-3.9.5-Linux-x86_64.sh
./cmake-3.9.5-Linux-x86_64.sh --skip-license
ln -s /cmake-3.9.5-Linux-x86_64/bin/* /usr/local/bin
rm -rf /cmake-3.9.5-Linux-x86_64

# Install a new version of boost
cd /
wget https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.gz
tar xf boost_1_65_1.tar.gz
cd boost_1_65_1
./bootstrap.sh --with-libraries=filesystem,regex,system,coroutine,serialization,log,thread
./b2 install || :
rm -rf /boost_1_65_1

# Install ContainerBuilder
cd /
git clone https://github.com/AdamSimpson/ContainerBuilder.git
cd ContainerBuilder
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTAL_PREFIX="/usr/local" ..
make
make install
rm -rf /ContainerBuilder

# TODO make this service more robust
su - queue -c "nohup /usr/local/bin/ResourceQueue > /dev/null 2>&1 < /dev/null &"
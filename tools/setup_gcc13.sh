#!/bin/bash

set -e

echo "Setting up GCC 13 alternatives..."

sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 11
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 13

sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 11
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 13

sudo update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 30
sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 30

sudo update-alternatives --set gcc /usr/bin/gcc-13
sudo update-alternatives --set g++ /usr/bin/g++-13

echo ""
echo "GCC version:"
gcc --version | head -1
echo ""
echo "G++ version:"
g++ --version | head -1
echo ""
echo "Now run: rm -rf build && make run"

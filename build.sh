#!/bin/bash
set -e

echo "=== Installing dependencies ==="
sudo apt update
sudo apt install -y \
  cmake build-essential \
  pkg-config curl \
  autoconf automake libtool libtool-bin m4 \
  ninja-build python3 git

echo "=== Cloning and building libOTe ==="
git clone https://github.com/osu-crypto/libOTe.git
cd libOTe
git checkout d0e499206d1d4d16c6b4ca6c0e712490e0632f80
python3 build.py --all --boost --sodium
cd ..

echo "=== Building FastUPSI ==="
mkdir -p build
cd build
cmake ..
make

echo "=== Build complete ==="

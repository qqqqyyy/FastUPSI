### VOLE

```bash
git clone -b vole --single-branch https://github.com/qqqqyyy/FastUPSI.git
cd FastUPSI
git clone https://github.com/osu-crypto/libOTe.git
cd libOTe
python build.py --all --boost --sodium
cd ..
mkdir -p build && cd build
cmake ..
cmake --build . -j
./test_rb_okvs

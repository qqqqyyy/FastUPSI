### VOLE

```bash
git clone -b vole --single-branch https://github.com/qqqqyyy/FastUPSI.git
cd FastUPSI
git clone https://github.com/osu-crypto/libOTe.git
cd libOTe
python build.py --all --boost --sodium
cd ..
mkdir build
cd build
cmake ..
make
./sender & ./receiver
``` 

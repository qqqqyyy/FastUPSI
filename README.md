This project can be built on ubuntu24.04-amd64

Instance type: c5.18xlarge

### setup

```bash
sudo apt update
sudo apt install -y cmake build-essential
sudo apt install -y \
  build-essential pkg-config git curl \
  autoconf automake libtool libtool-bin m4 \
  cmake ninja-build

git clone --single-branch --branch main --depth=10 https://github.com/qqqqyyy/FastUPSI.git
cd FastUPSI
git clone https://github.com/osu-crypto/libOTe.git
cd libOTe
python3 build.py --all --boost --sodium
cd ..
mkdir build
cd build
cmake ..
make
```

### run

Examples:

```bash
# generate datasets, 1024 days
./frontend/setup -start_size 1048576 -add_size 1024 -del_size 0 -days 1024
# tree, 1024 days, LAN
./frontend/main -party 1 -days 1024 -prot tree -LAN & ./frontend/main -party 0 -days 1024 -prot tree -LAN
# tree, 8 days, deletion, WAN 50Mbps
./frontend/main -party 1 -prot tree -del -WAN 50 & ./frontend/main -party 0 -prot tree -del -WAN 50
# okvs(adaptive), 1024 days, WAN 200Mbps
./frontend/main -party 1 -prot okvs -days 1024 -WAN 200 & ./frontend/main -party 0 -prot okvs -days 1024 -WAN 200
# cuckoo hashing(adaptive), 8 days, deletion, WAN 5Mbps
./frontend/main -party 1 -prot hash -WAN 5 & ./frontend/main -party 0 -prot hash -WAN 5
```

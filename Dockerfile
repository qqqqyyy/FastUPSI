ARG TARGETPLATFORM=linux/amd64
FROM --platform=$TARGETPLATFORM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    sudo iproute2\
    cmake build-essential\
    pkg-config curl \
    autoconf automake libtool libtool-bin m4 \
    ninja-build python3 git \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /home/fastupsi

# Copy project files
COPY . .

# Build libOTe
RUN git clone https://github.com/osu-crypto/libOTe.git && \
    cd libOTe && \
    git checkout d0e499206d1d4d16c6b4ca6c0e712490e0632f80 && \
    python3 build.py --all --boost --sodium -D ENABLE_MR_KYBER=OFF && \
    cd ..

# Build FastUPSI
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make

# Default command
CMD ["/bin/bash"]

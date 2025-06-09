FROM debian:buster

# LABEL org.opencontainers.image.source=https://github.com/ruidazeng/upsi-revisited

# set environment variables for tzdata
ARG TZ=America/New_York
ENV TZ=${TZ}
ENV LANG=en_US.UTF-8
ENV TERM=xterm-256color

# ensures project build can find shared libraries
ENV LD_LIBRARY_PATH=/usr/local/lib

# set higher timeouts to get through the build
RUN echo "Acquire::http::Timeout \"10\";" > /etc/apt/apt.conf.d/99timeout
RUN echo "Acquire::ftp::Timeout \"10\";" >> /etc/apt/apt.conf.d/99timeout
RUN echo "Acquire::Retries \"3\";" >> /etc/apt/apt.conf.d/99retry

ARG DEBIAN_FRONTEND=noninteractive

# install gcc-related packages
RUN apt-get update && \
    apt-get -y install \
      g++ \
      gdb \
      libblas-dev \
      liblapack-dev \
      make

# install clang-related packages
RUN apt-get -y install \
      clang \
      lldb \
      clang-format

# install cmake dependencies
RUN apt-get -y install \
      cmake \
      libssl-dev

# install interactive programs (emacs, vim, nano, man, sudo, etc.)
RUN apt-get -y install \
      bc \
      curl \
      dc \
      git \
      man \
      nano \
      psmisc \
      python3 \
      sudo \
      wget \
      zip \
      unzip \
      tar

# set up libraries
RUN apt-get -y install \
      libreadline-dev \
      locales \
      wamerican \
      libssl-dev \
      libgmp3-dev

# install programs used for networking
RUN apt-get -y install \
      dnsutils \
      inetutils-ping \
      iproute2 \
      net-tools \
      telnet \
      time \
      traceroute \
      libgmp3-dev

# configure our user environment
WORKDIR /home/fast-upsi

# install bazel
RUN apt-get install zlib1g-dev python3-distutils -y && \
    wget https://github.com/bazelbuild/bazel/releases/download/6.4.0/bazel_6.4.0-linux-x86_64.deb && \
    dpkg -i bazel_6.4.0-linux-x86_64.deb


# install emp-kit
RUN wget https://raw.githubusercontent.com/emp-toolkit/emp-readme/master/scripts/install.py && \
    python3 install.py --deps --tool --ot --sh2pc

# copy repository to image
COPY . .

# set up passwordless sudo for user
RUN useradd -m -s /bin/bash fast-upsi && \
    echo "fast-upsi ALL=(ALL:ALL) NOPASSWD: ALL" > /etc/sudoers.d/upsi-init && \
    chown fast-upsi:fast-upsi -R /home/fast-upsi
USER fast-upsi

# build all project endpoints
# RUN bazel build //upsi/original:all

# git build arguments
ARG USER=Fast\ UPSI
ARG EMAIL=nobody@example.com

RUN rm -f ~/.bash_logout
CMD ["/bin/bash", "-l"]
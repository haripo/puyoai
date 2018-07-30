FROM debian:stable

RUN apt-get update && \
    apt-get install -y git clang cmake ninja-build libcurl4-gnutls-dev \
    libgoogle-glog-dev libgflags-dev libprotobuf-dev \
    libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev \
    libmicrohttpd-dev libffms2-dev libusb-1.0-0-dev

VOLUME /work
WORKDIR /work

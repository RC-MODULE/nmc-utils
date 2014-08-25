#!/bin/bash


build_debs() { 
make clean
echo "===========BUILDING DEBS: $1 $2==========="
CROSS_COMPILE=/home/necromant/x-tools/$2/bin/$2- GNU_TARGET_NAME=$2 make deb ARCH=$1

mv *.deb /home/necromant/work/debian-repository-kitchen/repo/dists/stable/updates/binary-$1/
}


build_debs armel arm-module-linux-gnueabi
build_debs armhf arm-module-linux-gnueabihf

 
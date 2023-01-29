#!/bin/sh

set -xe

mkdir -p ../build
pushd ../build
gcc ~/Documents/code/c/uglykidjoe/code/sdl_uglykidjoe.c -o uglykidjoe `sdl2-config --cflags --libs`

./uglykidjoe

popd

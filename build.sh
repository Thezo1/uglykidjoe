#!/bin/sh

set -xe

mkdir -p ./build
pushd ./build
gcc -DUGLYKIDJOE_INTERNAL=1 -DUGLYKIDJOE_SLOW=1 -DUGLYKDJOE_SDL=1 ~/Documents/code/c/uglykidjoe/code/sdl_uglykidjoe.c -g -o  uglykidjoe `sdl2-config --cflags --libs` -lm
popd

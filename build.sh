#!/bin/sh

set -xe

mkdir -p ./build
pushd ./build
CommonFlags="-Wall -Werror -Wno-write-strings -Wno-unused -DUGLYKIDJOE_INTERNAL=1 -DUGLYKIDJOE_SLOW=1 -DUGLYKDJOE_SDL=1"
# Build for x86_64
gcc $CommonFlags ~/Documents/code/c/uglykidjoe/code/sdl_uglykidjoe.c -g -o  uglykidjoe.x86_64 `../code/sdl2-64/bin/sdl2-config --cflags --libs` -lm -Wl,-rpath,'$ORIGIN/x86_64'

# Build for x86
gcc -m32 $CommonFlags ~/Documents/code/c/uglykidjoe/code/sdl_uglykidjoe.c -g -o  uglykidjoe.x86 `../code/sdl2-32/bin/sdl2-config --cflags --libs` -lm -Wl,-rpath,'$ORIGIN/x86'

mkdir -p x86_64
cp ../code/sdl2-64/lib/libSDL2-2.0.so.0 x86_64/
mkdir -p x86
cp ../code/sdl2-32/lib/libSDL2-2.0.so.0 x86/
popd

@echo off

mkdir build
pushd build
cl -FC -Zi ../win_uglykidjoe.c user32.lib gdi32.lib
popd

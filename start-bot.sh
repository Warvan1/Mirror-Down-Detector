#!/bin/bash

if [ $# -eq 1 ]
then
cd build
cmake ..
cd -
cmake --build build/ -j$1
cd build
./discord-bot
else
echo "enter the number of cores to use for build as a command line argument"
fi
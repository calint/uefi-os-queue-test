#!/bin/sh
set -e

clang++ -std=c++26 -O3 -fsanitize=thread -o test1 src/test1.cpp
./test1 "$@"

#clang++ -std=c++26 -O3 -o test1 src/test1.cpp
#./test1 "$@"

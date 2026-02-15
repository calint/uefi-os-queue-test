#!/bin/sh
set -e

#clang++ -std=c++26 -O3 -fsanitize=thread -o test2 src/test2.cpp
clang++ -std=c++26 -O3 -o test2 src/test2.cpp
./test2 "$@"

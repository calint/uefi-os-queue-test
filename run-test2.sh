#!/bin/sh
set -e

#clang++ -fsanitize=thread -std=c++26 src/test1.cpp -o test1
clang++ -std=c++26 -O3 -o test2 src/test2.cpp

./test2 "$@"

#!/bin/sh
set -e

#clang++ -fsanitize=thread -std=c++26 src/test1.cpp -o test1
clang++ -std=c++26 src/test1.cpp -o test1

./test1 $1 $2

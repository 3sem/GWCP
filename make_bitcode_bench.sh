#!/bin/bash

dir_path="parser_bitcode"

if [ ! -d "$dir_path" ]; then
    mkdir -p "$dir_path"
    echo "Directory created: $dir_path"
else
    echo "Directory already exists: $dir_path"
fi


clang++ -DNAME=\"parser_bitcode\" -DVERSION=0.6  -c -emit-llvm src/inlined.cpp -o $dir_path/output.bc -std=c++14

# compiled once, you can make compiler gym benchmark from this .bc like:
# import compiler_gym
# from compiler_gym.datasets import Benchmark
#
#
# benchmark = Benchmark.from_file(
#    uri="benchmark://obparser",
#    path="..path/to/bc/file..",
# )

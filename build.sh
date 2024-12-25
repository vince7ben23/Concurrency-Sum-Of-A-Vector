#!/bin/bash

mkdir -p build && cd ./build;

# prepare cmake build system 
cmake ..;

# compile/link the project
cmake --build .;
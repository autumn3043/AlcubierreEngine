#!/bin/bash

SRC="https://github.com/KhronosGroup/SPIRV-Reflect.git"
ITEMS=("spirv_reflect.c" "spirv_reflect.cpp" "spirv_reflect.h" "include/spirv/unified1/spirv.h")
LICENSE="LICENSE"

git clone $SRC git_src

for ((i = 0; i < ${#ITEMS[@]}; i++)); do
    mkdir -p $(dirname ${ITEMS[i]})
    cp git_src/${ITEMS[i]} ${ITEMS[i]}
done

touch LICENSE
cp git_src/$LICENSE LICENSE

rm -rf git_src
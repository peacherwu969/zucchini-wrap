#!/usr/bin/env bash
#BUILD with clang!!!

rm -rf build
cmake -S . -B build -G "Unix Makefiles" \
  -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
  -DCMAKE_BUILD_TYPE=Release \
  	-DCMAKE_C_COMPILER=/usr/local/gcc-16/bin/gcc \
    -DCMAKE_CXX_COMPILER=/usr/local/gcc-16/bin/g++ \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
  -DZUCCHINI_NATIVE_OPTIMIZE=ON \
  -DZUCCHINI_SOURCE_DIR="$(readlink -f zucchini)"
make -C build -j"$(nproc)"

#time build/zucchini -gen $o $n d

# !/bin/sh

set -x

SOURCE_DIR=`pwd`

cmake -B build \
    && cd build \
    && cmake --build .
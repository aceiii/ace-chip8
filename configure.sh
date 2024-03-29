#!/bin/sh

cmake \
    -B=build \
    -S=. \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -G=Ninja

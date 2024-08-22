#!/bin/bash
# configuration on furore
export PATH=/home/lcarpent/approx_workspace/HPAC/build_compiler/bin/:$PATH
export LD_LIBRARY_PATH=/home/lcarpent/approx_workspace/HPAC/build_compiler/lib/:$LD_LIBRARY_PATH
export CC=clang
export CPP=clang++
export HPAC_GPU_ARCH=nvptx64
export HPAC_GPU_SM=sm_70
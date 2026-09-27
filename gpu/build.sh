#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
arch="${GPU_ARCH:-gfx1201}"
mkdir -p build
hipcc -O3 -std=c++17 --offload-arch="$arch" -Wall -c descriptor-backend.cpp -o build/backend.o
gcc -O3 -fopenmp -Wall -c descriptor-large.c -o build/descriptor-large.o
hipcc build/descriptor-large.o build/backend.o -fopenmp -pthread -lm -o build/descriptor-large
gcc -O3 -fopenmp -Wall baseline.c -lm -o build/baseline

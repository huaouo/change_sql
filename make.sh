#!/bin/bash

python sysinfo.py

cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build -j
strip build/change_sql

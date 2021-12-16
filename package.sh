#!/bin/bash

# Generate compile.sh
rm -rf build
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build -j -- -v | cut -d' ' -f2- | tee compile.sh
sed -i -e 's|/usr/bin/cmake -E||g' compile.sh
find build ! -type d -exec rm '{}' \;

# Zip the source code
rm -f tdsql.zip
zip -r tdsql.zip 3rd build src compile.sh make.sh start.sh
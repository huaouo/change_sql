#!/bin/bash

dir="$(dirname "$(readlink -f "$0")")"
sed -i -e "s|/mnt/c/Users/huaouo/Workspace/change_sql|$dir|g" compile.sh
pushd build > /dev/null || exit
../compile.sh
popd > /dev/null || exit
strip build/change_sql

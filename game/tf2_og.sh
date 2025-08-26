#!/usr/bin/env bash

script=$(readlink -f -- "$0")
pushd "$(dirname -- "$script")" > /dev/null

./tc2.sh -game $PWD/tf2_og

popd

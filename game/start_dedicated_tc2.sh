#!/usr/bin/env bash

script=$(readlink -f -- "$0")
pushd "$(dirname -- "$script")" > /dev/null

./tc2.sh -console -dedicated +sv_pure 1 "$@"

popd

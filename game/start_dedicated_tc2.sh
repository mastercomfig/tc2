#!/usr/bin/env bash

script=$(readlink -f -- "$0")
pushd "$(dirname -- "$script")" > /dev/null

./tc2.sh -console -dedicated +sv_pure 1 +ip 127.0.0.1 +maxplayers 100 "$@"

popd

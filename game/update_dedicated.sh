#!/usr/bin/env bash

script=$(readlink -f -- "$0")
pushd "$(dirname -- "$script")" > /dev/null

curl -Lo game.zip https://github.com/mastercomfig/tc2/releases/download/rel/game-linux.zip
7z x game.zip
rm game.zip

popd > /dev/null

#!/usr/bin/env bash

set -euo pipefail

script=$(readlink -f -- "$0")
pushd "$(dirname -- "$script")" > /dev/null

./steamcmd_update.sh
curl -Lo game.zip https://github.com/mastercomfig/tc2/releases/latest/download/game-linux.zip
7z x -aoa -y game.zip
rm game.zip

popd > /dev/null

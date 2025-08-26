#!/usr/bin/env bash
#
# Run script within the directory
BIN_DIR=$(dirname "$(readlink -fn "$0")")
cd "${BIN_DIR}" || exit 2

set -e

source ./shared.sh

if [ -f "${DEV_DIR}/tc2/pak1_dir.vpk" ]; then
  echo "pak already downloaded"
  exit 0
fi

gh release download -R "mastercomfig/tc2-pak" --clobber $PAK_VERSION -p "pak.zip" -O pak.zip

$CMD_7Z e pak.zip -y "-o${DEV_DIR}/tc2"

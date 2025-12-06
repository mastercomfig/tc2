#!/usr/bin/env bash
#
# Run script within the directory
BIN_DIR=$(dirname "$(readlink -fn "$0")")
cd "${BIN_DIR}" || exit 2

set -e

source ./shared.sh

PAK_VERSION_FILE="${DEV_DIR}/version.txt"

CUR_PAK_VERSION=$((cat $PAK_VERSION_FILE || echo "") | tr -d '\n')

if [[ -f "${DEV_DIR}/tc2/pak1_dir.vpk" && $CUR_PAK_VERSION == $PAK_VERSION ]]; then
  echo "pak already downloaded"
  exit 0
fi

rm -f pak.zip
curl -L "https://github.com/mastercomfig/tc2-pak/releases/download/$PAK_VERSION/pak.zip" -o pak.zip

$CMD_7Z e pak.zip -y "-o${DEV_DIR}/tc2"

echo $PAK_VERSION > $PAK_VERSION_FILE

#!/usr/bin/env bash
#
# Run script within the directory
BIN_DIR=$(dirname "$(readlink -fn "$0")")
cd "${BIN_DIR}" || exit 2

set -e

source ./shared.sh

if [ -z "$TF2_INSTALL_DIR" ]; then
  echo "You must define TF2_INSTALL_DIR!"
  exit 1
fi

if [ $PLATFORM = "win" ]; then
  PATH="${PATH}:${TF2_INSTALL_DIR}/bin/x64"
else
  echo "VPK only supported on Windows!"
  exit 1
fi

FULL_BASE_DIR=$(cd .. && pwd -W)

rm -f ../game/tc2/pak1*.vpk

# TODO: do we want to use Steam Pipe and VDF manifest?
vpk -M -k mcoms.publickey.vdf -K mcoms.privatekey.vdf "${FULL_BASE_DIR}/game_src/tc2/pak1"

# move instead of copy to regenerate multi-folder each time
mv ../game_src/tc2/pak1*.vpk ../game/tc2/

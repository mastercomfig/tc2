#!/bin/bash
#
# Run script within the directory
BIN_DIR=$(dirname "$(readlink -fn "$0")")
cd "${BIN_DIR}" || exit 2

set -e

./copy.sh
./push.sh
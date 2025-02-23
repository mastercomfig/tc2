#!/bin/bash
#
# Run script within the directory
BIN_DIR=$(dirname "$(readlink -fn "$0")")
cd "${BIN_DIR}" || exit 2

set -e

source ./shared.sh

PLATFORM_STR=win
SOURCE_MANIFEST=./.itch.${PLATFORM_STR}.toml
TARGET_MANIFEST=${CLEAN_DIR}/.itch.toml

cp -f ${SOURCE_MANIFEST} ${TARGET_MANIFEST}

PROJECT_STR="mastercoms/tc2"
BRANCH_STR=stable
CHANNEL_STR=${PLATFORM_STR}-${BRANCH_STR}
VERSION="0.11.0"

butler push "${CLEAN_DIR}" "${PROJECT_STR}:${CHANNEL_STR}" "--userversion" "${VERSION}"

rm -f ${TARGET_MANIFEST}

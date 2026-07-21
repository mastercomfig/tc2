#!/usr/bin/env bash
#
# Run script within the directory
BIN_DIR=$(dirname "$(readlink -fn "$0")")
ORIGINAL_DIR=$(pwd)
cd "${BIN_DIR}" || exit 2

set -e

source ./shared.sh

if [ -n "$1" ]; then
  CLEAN_DIR="${ORIGINAL_DIR}/$1"
fi

if [ -n "${RELEASE_VERSION}" ]; then
  VERSION="${RELEASE_VERSION}"
elif [ "${GITHUB_REF_TYPE:-}" = "tag" ] && [ -n "${GITHUB_REF_NAME:-}" ]; then
  VERSION="${GITHUB_REF_NAME}"
fi

BRANCH_STR=${BRANCH_STR:-stable}
SOURCE_MANIFEST=./.itch.${PLATFORM}.toml
TARGET_MANIFEST=${CLEAN_DIR}/.itch.toml

cp -f ${SOURCE_MANIFEST} ${TARGET_MANIFEST}

PROJECT_STR="mastercoms/tc2"
CHANNEL_STR=${PLATFORM}-${BRANCH_STR}

butler push "${CLEAN_DIR}" "${PROJECT_STR}:${CHANNEL_STR}" "--userversion" "${VERSION}"

rm -f ${TARGET_MANIFEST}

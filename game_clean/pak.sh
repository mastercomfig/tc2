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

# create release zip
rm -f pak.zip
$CMD_7Z a pak.zip ../game_src/tc2/*.vpk

# move instead of copy to regenerate multi-folder each time
mv ../game_src/tc2/pak1*.vpk ../game/tc2/

PAK_ZIP=$(realpath pak.zip)
PAK_REPO=../../tc2-pak

(
cd $PAK_REPO

git pull

PAK_VERSION_FILE="version.txt"

CUR_PAK_VERSION=$(cat $PAK_VERSION_FILE | tr -d '\n')
if [ $CUR_PAK_VERSION == $PAK_VERSION ]; then
  echo "Nothing to upload, PAK_VERSION up to date. If you don't expect this result, bump PAK_VERSION."
  exit 0
fi

echo $PAK_VERSION > $PAK_VERSION_FILE

git add $PAK_VERSION_FILE
git commit -m "pak version ${PAK_VERSION}"
git push -u origin HEAD

gh release create "$PAK_VERSION" -t "$PAK_VERSION" -n "TC2 pak file v$PAK_VERSION" $PAK_ZIP

git fetch --tags origin
)

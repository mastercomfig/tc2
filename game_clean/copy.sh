#!/bin/bash
#
# Run script within the directory
BIN_DIR=$(dirname "$(readlink -fn "$0")")
cd "${BIN_DIR}" || exit 2

set -e

source ./shared.sh

rm -rf ${CLEAN_DIR}
rm -rf ${CLEAN_DEBUG_DIR}
mkdir -p ${CLEAN_DIR}/{bin/x64,tc2/bin/x64,tc2/materials,tc2/cfg}
mkdir -p ${CLEAN_DEBUG_DIR}/{bin/x64,tc2/bin/x64,tc2/materials,tc2/cfg}

declare -a EXES=(
  tc2_win64
  bin/x64/captioncompiler
  bin/x64/glview
  bin/x64/height2normal
  bin/x64/motionmapper
  bin/x64/qc_eyes
  bin/x64/tgadiff
  bin/x64/vbsp
  bin/x64/vice
  bin/x64/vrad
  bin/x64/vtf2tga
  bin/x64/vtfdiff
  bin/x64/vvis
)

declare -a DLLS=(
  bin/x64/vrad_dll
  bin/x64/vvis_dll
  tc2/bin/x64/{client,server}
)

declare -a DLLS_LIB=(
  bin/x64/steam_api64
)

declare -a REP_FILES=(
  tc2/cfg/valve.rc
  tc2/cfg/default.cfg
  tc2/cfg/user_default.scr
  tc2/cfg/vscript_convar_allowlist.txt
  tc2/cfg/motd_default.txt
  tc2/cfg/motd_text_default.txt
  tc2/texture_preload_list.txt
  tc2/materials/logo
  tc2/resource
  tc2/gameinfo.txt
  tc2/steam.inf
)

declare -a FILES=(
  ../thirdpartylegalnotices.txt
  ../LICENSE
)

for F in "${EXES[@]}"; do
  cp -f ${DEV_DIR}/${F}.exe ${CLEAN_DIR}/${F}.exe
done

for F in "${DLLS[@]}"; do
  cp -f ${DEV_DIR}/${F}.dll ${CLEAN_DIR}/${F}.dll
  cp -f ${DEV_DIR}/${F,,}.pdb ${CLEAN_DEBUG_DIR}/${F,,}.pdb
done

for F in "${DLLS_LIB[@]}"; do
  cp -f ${DEV_DIR}/${F}.dll ${CLEAN_DIR}/${F}.dll
done

for F in "${REP_FILES[@]}"; do
  cp -rf ${DEV_DIR}/${F} ${CLEAN_DIR}/${F}
done

for F in "${FILES[@]}"; do
  ORIG=$(basename ${F})
  cp -f ${F} ${CLEAN_DIR}/${ORIG}
done
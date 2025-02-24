#!/usr/bin/env bash
#
# Run script within the directory
BIN_DIR=$(dirname "$(readlink -fn "$0")")
cd "${BIN_DIR}" || exit 2

set -e

source ./shared.sh

rm -rf ${CLEAN_DIR}
rm -rf ${CLEAN_DEBUG_DIR}
mkdir -p ${CLEAN_DIR}/{bin/$PLAT_DIR,tc2/bin/$PLAT_DIR,tc2/materials,tc2/cfg,tc2/scripts}
mkdir -p ${CLEAN_DEBUG_DIR}/{bin/$PLAT_DIR,tc2/bin/$PLAT_DIR}

declare -a EXES=(
  tc2_win64
  bin/$PLAT_DIR/captioncompiler
  bin/$PLAT_DIR/glview
  bin/$PLAT_DIR/height2normal
  bin/$PLAT_DIR/motionmapper
  bin/$PLAT_DIR/qc_eyes
  bin/$PLAT_DIR/tgadiff
  bin/$PLAT_DIR/vbsp
  bin/$PLAT_DIR/vice
  bin/$PLAT_DIR/vrad
  bin/$PLAT_DIR/vtf2tga
  bin/$PLAT_DIR/vtfdiff
  bin/$PLAT_DIR/vvis
)

declare -a DLLS=(
  bin/$PLAT_DIR/vrad_dll
  bin/$PLAT_DIR/vvis_dll
  tc2/$PLAT_DIR/{client,server}
)

declare -a DLLS_LIB=(
  bin/$PLAT_DIR/steam_api64
)

declare -a FILES_REP=(
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
  cp -f ${DEV_DIR}/${F}${EXE_EXT} ${CLEAN_DIR}/${F}${EXE_EXT}
done

for F in "${DLLS[@]}"; do
  DLL=${F}${DLL_EXT}
  cp -f ${DEV_DIR}/${DLL} ${CLEAN_DIR}/${DLL}
  cp -f ${DEV_DIR}/${F,,}.pdb ${CLEAN_DEBUG_DIR}/${F,,}.pdb
done

for F in "${DLLS_LIB[@]}"; do
  cp -f ${DEV_DIR}/${F}${DLL_EXT} ${CLEAN_DIR}/${F}${DLL_EXT}
done

for F in "${FILES_REP[@]}"; do
  cp -rf ${DEV_DIR}/${F} ${CLEAN_DIR}/${F}
done

for F in "${FILES[@]}"; do
  ORIG=$(basename ${F})
  cp -f ${F} ${CLEAN_DIR}/${ORIG}
done
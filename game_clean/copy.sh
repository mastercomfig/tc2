#!/usr/bin/env bash
#
# Run script within the directory
BIN_DIR=$(dirname "$(readlink -fn "$0")")
cd "${BIN_DIR}" || exit 2

set -e

source ./shared.sh

rm -rf ${CLEAN_DIR}
rm -rf ${CLEAN_DEBUG_DIR}
mkdir -p ${CLEAN_DIR}/{bin/$PLAT_DIR,tc2/bin/$PLAT_DIR,tf2_og/bin/$PLAT_DIR,tc2/cfg,tf2_og/cfg}
mkdir -p ${CLEAN_DEBUG_DIR}/{bin/$PLAT_DIR,tc2/bin/$PLAT_DIR,tf2_og/bin/$PLAT_DIR}

declare -a DLLS=(
  tc2/bin/$PLAT_DIR/{client,server}
  tf2_og/bin/$PLAT_DIR/{client,server}
)

declare -a FILES_REP=(
  #
  tc2/cfg/vscript_convar_allowlist.txt
  tf2_og/cfg/vscript_convar_allowlist.txt
  #
  tc2/pak1
  tf2_og/pak1
  #
  tc2/gameinfo.txt
  tf2_og/gameinfo.txt
  #
  tc2/steam.inf
  tf2_og/steam.inf
)

declare -a FILES=(
  ../thirdpartylegalnotices.txt
  ../LICENSE
)

if [ $PLATFORM = "win" ]; then
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

  DLLS+=(
    bin/$PLAT_DIR/vrad_dll
    bin/$PLAT_DIR/vvis_dll
  )

  declare -a DLLS_LIB=(
    bin/$PLAT_DIR/steam_api64
  )

  FILES_REP+=(
    tc2.bat
    tc2_vulkan.bat
    tf2_og.bat
    tf2_og_vulkan.bat
  )
elif [ $PLATFORM = "linux" ]; then
  declare -a EXES=(
    tc2_linux64
  )

  declare -a DLLS_LIB=(
    bin/$PLAT_DIR/libsteam_api
  )

  FILES_REP+=(
    tc2.sh
    tf2_og.sh
  )
fi

for F in "${EXES[@]}"; do
  cp -f ${DEV_DIR}/${F}${EXE_EXT} ${CLEAN_DIR}/${F}${EXE_EXT}
done

for F in "${DLLS[@]}"; do
  DLL=${F}${DLL_EXT}
  cp -f ${DEV_DIR}/${DLL} ${CLEAN_DIR}/${DLL}
  if [ $PLATFORM = "win" ]; then
    cp -f ${DEV_DIR}/${F,,}.pdb ${CLEAN_DEBUG_DIR}/${F,,}.pdb
  elif [ $PLATFORM = "linux" ]; then
    # Linux binaries aren't stripped by the build scripts, so separate the
    # debug info and strip them here.
    cp -f ${CLEAN_DIR}/${DLL} ${CLEAN_DEBUG_DIR}/${DLL}.dbg
    objcopy --add-gnu-debuglink=${CLEAN_DEBUG_DIR}/${DLL}.dbg ${CLEAN_DIR}/${DLL}
    strip ${CLEAN_DIR}/${DLL}
  fi
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

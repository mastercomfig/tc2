#!/usr/bin/env bash
#
# Run script within the directory
BIN_DIR=$(dirname "$(readlink -fn "$0")")
cd "${BIN_DIR}" || exit 2

set -e

source ./shared.sh

rm -rf ${CLEAN_DIR}
rm -rf ${CLEAN_DEBUG_DIR}
mkdir -p ${CLEAN_DIR}/{bin/$PLAT_DIR,tc2/bin/$PLAT_DIR,tc2/cfg,tc2/custom}
mkdir -p ${CLEAN_DEBUG_DIR}/{bin/$PLAT_DIR,tc2/bin/$PLAT_DIR}

./dlpak.sh

declare -a DLLS=(
  tc2/bin/$PLAT_DIR/{client,server,sentry,game_shader_generic_std}
)

declare -a FILES_REP=(
  #
  tc2/loose
  #
  tc2/gameinfo.txt
  tc2/gameinfo_server.txt
  #
  tc2/steam.inf
  #
  tc2/custom/readme.txt
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
    start_dedicated_tc2.bat
    tc2.bat
  )
elif [ $PLATFORM = "linux" ]; then
  declare -a EXES=(
    tc2_linux64
  )

  declare -a DLLS_LIB=(
    bin/$PLAT_DIR/libsteam_api
  )

  FILES_REP+=(
    start_dedicated_tc2.sh
    update_dedicated.sh
    srcds_run_64
    tc2.sh
  )
fi

for F in "${EXES[@]}"; do
  cp -f ${DEV_DIR}/${F}${EXE_EXT} ${CLEAN_DIR}/${F}${EXE_EXT}
done

for F in "${DLLS[@]}"; do
  DLL=${F}${DLL_EXT}
  cp -f ${DEV_DIR}/${DLL} ${CLEAN_DIR}/${DLL}
  if [ $PLATFORM = "win" ]; then
    if [ -f ${DEV_DIR}/${F,,}.pdb ]; then
      cp -f ${DEV_DIR}/${F,,}.pdb ${CLEAN_DEBUG_DIR}/${F,,}.pdb
    fi
  elif [ $PLATFORM = "linux" ]; then
    # Linux binaries aren't stripped by the build scripts, so separate the
    # debug info and strip them here.
    cp -f ${CLEAN_DIR}/${DLL} ${CLEAN_DEBUG_DIR}/${DLL}.dbg
    objcopy --add-gnu-debuglink=${CLEAN_DEBUG_DIR}/${DLL}.dbg ${CLEAN_DIR}/${DLL}
    strip ${CLEAN_DIR}/${DLL}
    # dedicated server DLL
    if [ -z ${DLL##*server.so} ]; then
      cp ${CLEAN_DIR}/${F}${DLL_EXT} ${CLEAN_DIR}/${F}_srv${DLL_EXT}
      patchelf --replace-needed libtier0.so libtier0_srv.so --replace-needed libvstdlib.so libvstdlib_srv.so ${CLEAN_DIR}/${F}_srv${DLL_EXT}
    fi
  fi
done

for F in "${DLLS_LIB[@]}"; do
  cp -f ${DEV_DIR}/${F}${DLL_EXT} ${CLEAN_DIR}/${F}${DLL_EXT}
done

for F in "${FILES_REP[@]}"; do
  cp -rf ${DEV_DIR}/${F} ${CLEAN_DIR}/${F}
done

# cfg files
for F in $(cd ${DEV_DIR}/tc2/cfg && git ls-files .); do
  cp -rf ${DEV_DIR}/tc2/cfg/${F} ${CLEAN_DIR}/tc2/cfg/${F}
done

# pak1.vpk
cp -rf ${DEV_DIR}/tc2/pak1*.vpk  ${CLEAN_DIR}/tc2

for F in "${FILES[@]}"; do
  ORIG=$(basename ${F})
  cp -f ${F} ${CLEAN_DIR}/${ORIG}
done

# make executable
if [ $PLATFORM = "linux" ]; then
  chmod +x ${CLEAN_DIR}/*.sh ${CLEAN_DIR}/tc2_linux64
fi

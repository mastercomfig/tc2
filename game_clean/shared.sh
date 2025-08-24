VERSION="0.50.0"
DEV_DIR=../game
CLEAN_DIR=../game_dist
CLEAN_DEBUG_DIR=${CLEAN_DIR}_debug
if [[ "$OSTYPE" == "linux"* ]]; then
  PLATFORM="linux"
  PLAT_DIR="linux64"
  DLL_EXT=".so"
  EXE_EXT=""
elif [[ "$OSTYPE" == "msys"* ]]; then
  PLATFORM="win"
  PLAT_DIR="x64"
  DLL_EXT=".dll"
  EXE_EXT=".exe"
else
  echo "OS is not supported! Exiting."
  exit 1
fi

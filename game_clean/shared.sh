VERSION="1.1.17"
PAK_VERSION="0.78.0"
DEV_DIR=../game
CLEAN_DIR=../game_dist
CLEAN_DEBUG_DIR=${CLEAN_DIR}_debug

if command -v 7zz >/dev/null 2>&1; then
  CMD_7Z=7zz
elif command -v 7z >/dev/null 2>&1; then
  CMD_7Z=7z
elif command -v 7za >/dev/null 2>&1; then
  CMD_7Z=7za
else
  echo "7zz/7z/7za command not found! Please install 7zip."
  exit 1
fi

BUILD_PLATFORM="${BUILD_PLATFORM:-${OSTYPE}}"

if [[ "$BUILD_PLATFORM" == "linux"* ]]; then
  PLATFORM="linux"
  PLAT_DIR="linux64"
  DLL_EXT=".so"
  EXE_EXT=""
elif [[ "$BUILD_PLATFORM" == "win"* || "$BUILD_PLATFORM" == "msys"* || "$BUILD_PLATFORM" == "cygwin"* || "$BUILD_PLATFORM" == "mingw"* ]]; then
  PLATFORM="win"
  PLAT_DIR="x64"
  DLL_EXT=".dll"
  EXE_EXT=".exe"
else
  echo "OS/Platform ${BUILD_PLATFORM} is not supported! Exiting."
  exit 1
fi

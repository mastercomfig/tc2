#!/usr/bin/env sh
set -e

source ../game_clean/shared.sh

(
cd "$(dirname "$0")"
git submodule update --init --recursive -- thirdparty/sentry
cd thirdparty/sentry
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DSENTRY_BUILD_SHARED_LIBS=1 -DSENTRY_BUILD_RUNTIMESTATIC=1 -DSENTRY_FOLDER=sentry
cmake --build build --parallel
cmake --install build --prefix ./install --config RelWithDebInfo
rm -rf ../sentry_native
cp -r ./install ../sentry_native
)

if [ $PLATFORM = "win" ]; then
  cp thirdparty/sentry_native/bin/sentry.dll ../game/tc2/bin/x64/sentry.dll
  cp thirdparty/sentry_native/bin/sentry.pdb ../game/tc2/bin/x64/sentry.pdb
  cp thirdparty/sentry_native/bin/crashpad_handler.exe ../game/bin/x64/crashpad_handler.exe
  cp thirdparty/sentry_native/bin/crashpad_wer.dll ../game/bin/x64/crashpad_wer.dll
else
  cp thirdparty/sentry_native/bin/libsentry.so ../game/tc2/bin/linux64/libsentry.so
  cp thirdparty/sentry_native/bin/libsentry.so.dbg ../game/tc2/bin/linux64/libsentry.so.dbg
  cp thirdparty/sentry_native/bin/crashpad_handler ../game/bin/linux64/crashpad_handler
fi

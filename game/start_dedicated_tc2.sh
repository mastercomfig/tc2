#!/usr/bin/env bash

script=$(readlink -f -- "$0")
pushd "$(dirname -- "$script")" > /dev/null

if [ ! -f tc2/gameinfo_client.txt ]; then
    cp tc2/gameinfo.txt tc2/gameinfo_client.txt
    cp tc2/gameinfo_server.txt tc2/gameinfo.txt
fi

if [ ! -f bin/linux64/libtinfo.so.5 ]; then
    ln -s /lib/x86_64-linux-gnu/libtinfo.so.6 bin/linux64/libtinfo.so.5
fi

./tc2.sh -console -dedicated -gatherdedi +sv_pure 1 "$@"

mv tc2/gameinfo_client.txt tc2/gameinfo.txt

popd > /dev/null

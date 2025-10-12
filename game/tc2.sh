#!/usr/bin/env bash

script=$(readlink -f -- "$0")
pushd "$(dirname -- "$script")" > /dev/null

# Launch the game under the steam runtime
if [ -z $SLR_SNIPER_PATH ]; then
  SLR_SNIPER_PATH="$HOME/.steam/steam/steamapps/common/SteamLinuxRuntime_sniper/run"
  if [ ! -f ${SLR_SNIPER_PATH} ]; then
    if [ -z $(command -v steam) ]; then
      SLR_SNIPER_PATH="$HOME/.local/share/Steam/steamcmd/SteamLinuxRuntime_sniper/run"
      if [ ! -f ${SLR_SNIPER_PATH} ]; then
        echo "Run steamcmd +force_install_dir ./SteamLinuxRuntime_sniper +login anonymous +app_update 1628350 validate +quit, or define \$SLR_SNIPER_PATH to the location of the Steam Linux Runtime 3.0 run path."
        exit 1
      fi
    else
      echo "Install Steam Linux Runtime (by running steam steam://install/1628350), or define \$SLR_SNIPER_PATH to the location of the Steam Linux Runtime 3.0 run file."
      exit 1
    fi
  fi
fi

${SLR_SNIPER_PATH} --devel -- ./tc2_linux64 -steam -particles 1 +ip 127.0.0.1 "$@"

popd > /dev/null

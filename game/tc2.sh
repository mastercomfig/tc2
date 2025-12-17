#!/usr/bin/env bash

script=$(readlink -f -- "$0")
pushd "$(dirname -- "$script")" > /dev/null

#ulimit -c unlimited
#sudo bash -c 'echo "core.%p" > /proc/sys/kernel/core_pattern 2>/dev/null || true'

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

#trap 'echo "Received SIGTERM, shutting down gracefully..." && kill -TERM $!' SIGTERM
#trap 'echo "Received SIGPIPE, shutting down gracefully..." && continue' SIGPIPE

${SLR_SNIPER_PATH} --devel -- ./tc2_linux64 -steam -gathermod -particles 1 -nobreakpad -nominidump "$@" +ip 127.0.0.1

popd > /dev/null

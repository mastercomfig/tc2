#!/usr/bin/env bash

set -euo pipefail

steamcmd +force_install_dir SteamLinuxRuntime_sniper +login anonymous +app_update 1628350 validate +quit
steamcmd +force_install_dir tf2 +login anonymous +app_update 232250 validate +quit

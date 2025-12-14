#!/bin/bash

# figure out the absolute path to the script being run a bit
# non-obvious, the ${0%/*} pulls the path out of $0, cd's into the
# specified directory, then uses $PWD to figure out where that
# directory lives - and all this in a subshell, so we don't affect
# $PWD

GAMEROOT=$(cd "${0%/*}" && echo $PWD)

BASEEXEC_NAME="tf"
BASEGAMEROOT="$1"
shift;

######
# Get the game name...

# Save the original argument list
original_args=("$@")

EXEC_NAME="tc2"
game_dir=$EXEC_NAME
# Loop through arguments to find -game
while [[ $# -gt 0 ]]; do
    case $1 in
        -game)
            # Check if there's a value after -game
            if [[ -n $2 && $2 != -* ]]; then
                game_dir=$2
                shift # Skip the value after -game
            else
                echo "Error: No argument provided after -game"
                break
            fi
            ;;
    esac
    shift # Move to the next argument
done

# Reset the argument list
set -- "${original_args[@]}"

server_64_dll="$game_dir/bin/linux64/server.so"

is_64_bit=0

if [[ -f $server_64_dll ]]; then
	is_64_bit=1
fi

architecture=$(uname -m)

if [[ "$architecture" == "i686" || "$architecture" == "i386" ]]; then
	is_64_bit=0
fi

plat_dir=
plat_suffix=_linux
if [[ "$is_64_bit" -eq 1 ]]; then
	plat_dir=linux64
	plat_suffix=_linux64
fi

######

# determine platform
UNAME=$(uname)
if [ "$UNAME" == "Linux" ]; then
	# TF2 requires the sniper container runtime
	. /etc/os-release

	# prepend our lib path to LD_LIBRARY_PATH
	export LD_LIBRARY_PATH="${GAMEROOT}/bin/$plat_dir":"${BASEGAMEROOT}/bin/$plat_dir":$LD_LIBRARY_PATH
fi

if [ -z $GAMEEXE ]; then
	GAMEEXE="${BASEEXEC_NAME}"$plat_suffix
fi

ulimit -n 2048

# enable nVidia threaded optimizations
export __GL_THREADED_OPTIMIZATIONS=1

# and launch the game
cd "$BASEGAMEROOT"

# Enable path match if we are running with loose files
if [ -f pathmatch.inf ]; then
	export ENABLE_PATHMATCH=1
fi

# Do the following for strace:
# 	GAME_DEBUGGER="strace -f -o strace.log"

STATUS=42
while [ $STATUS -eq 42 ]; do
	if [ "${GAME_DEBUGGER}" == "gdb" ] || [ "${GAME_DEBUGGER}" == "cgdb" ]; then
		ARGSFILE=$(mktemp $USER.$EXEC_NAME.gdb.XXXX)
		echo b main > "$ARGSFILE"

		# Set the LD_PRELOAD varname in the debugger, and unset the global version. This makes it so that
		#   gameoverlayrenderer.so and the other preload objects aren't loaded in our debugger's process.
		echo set env LD_PRELOAD=$LD_PRELOAD >> "$ARGSFILE"
		echo show env LD_PRELOAD >> "$ARGSFILE"
		unset LD_PRELOAD

		echo run $@ >> "$ARGSFILE"
		echo show args >> "$ARGSFILE"
		${GAME_DEBUGGER} "${BASEGAMEROOT}"/${GAMEEXE} -x "$ARGSFILE"
		rm "$ARGSFILE"
	else
		${GAME_DEBUGGER} "${BASEGAMEROOT}"/${GAMEEXE} "$@"
	fi
	STATUS=$?
done
exit $STATUS

@echo off
setlocal

set BUILD_SHADER=call buildshaders.bat

set SOURCE_DIR="..\..\"

rem Change me to your mod's name!
set GAME_DIR="..\..\..\game\tc2"

%BUILD_SHADER% stdshader_dx9_30         -game %GAME_DIR% -source %SOURCE_DIR% -force30

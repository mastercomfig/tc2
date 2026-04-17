@echo off
setlocal

set BUILD_SHADER=call buildshaders.bat

set SOURCE_DIR="..\..\"

rem Change me to your mod's name!
set GAME_DIR="..\..\..\game\tc2"

set SHADER_LIST=stdshader_dx9_30
if "%1"=="" goto build

set SHADER_LIST=%1

:build
%BUILD_SHADER% %SHADER_LIST% -game %GAME_DIR% -source %SOURCE_DIR% -force30

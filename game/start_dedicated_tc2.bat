@echo off
pushd %~dp0
start .\tc2_win64.exe -game tc2 -console -dedicated -nobreakpad -nominidumps %* +sv_pure 1 +ip 127.0.0.1
popd

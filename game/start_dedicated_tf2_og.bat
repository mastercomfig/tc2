@echo off
pushd %~dp0
start .\tc2_win64.exe -game tf2_og -console -dedicated +sv_pure 0 +ip 127.0.0.1 +maxplayers 100 %*
popd

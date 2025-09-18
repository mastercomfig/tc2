@echo off
pushd %~dp0
start .\tc2_win64.exe -steam -game tf2_og -particles 1 %*
popd

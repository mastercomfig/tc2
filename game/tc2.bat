@echo off
pushd %~dp0
start .\tc2_win64.exe -steam -particles 1 +ip 127.0.0.1 %*
popd

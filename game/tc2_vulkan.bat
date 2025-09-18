@echo off
pushd %~dp0
start .\tc2_win64.exe -steam -vulkan -particles 1 %*
popd

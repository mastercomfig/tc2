@echo off
pushd %~dp0
start .\tc2.bat -force_vendor_id 0X10DE -force_device_id 0xFFFE -disable_d3d9_hacks %* +mat_tonemapping_occlusion_use_stencil 1 +mat_disable_ps_patch 1
popd

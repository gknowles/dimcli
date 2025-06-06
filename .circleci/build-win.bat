:: Copyright Glen Knowles 2025
:: Distributed under the Boost Software License, Version 1.0.
@echo off
cd %~dp0
if "%p_generator%" equ "Visual Studio 16 2019" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
) else if "%p_generator%" equ "Visual Studio 17 2022" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
)
cd ..
md build
cd build
cmake .. -G "%p_generator%" -A %p_platform% -T %p_toolset%
cmake . -DCMAKE_BUILD_TYPE=%p_config% -DMAKE_INSTALL_PREFIX=publish ^
  -DINSTALL_LIBS:BOOL=ON
cmake --build . --target install --config %p_config%

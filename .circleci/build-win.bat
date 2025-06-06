:: Copyright Glen Knowles 2025
:: Distributed under the Boost Software License, Version 1.0.
@echo off
cd %~dp0
if "%p_generator%" equ "Visual Studio 16 2019" (
    call "c:\program files\microsoft visual studio\2019\community\vc\auxiliary\build\vcvarsall.bat" x64
) else if "%p_generator%" equ "Visual Studio 17 2022" (
    call vs17
)
cd ..
md build
cd build
cmake .. -G "%p_generator%" -A %p_platform% -T %p_toolset%
cmake . -DCMAKE_BUILD_TYPE=%p_config% -DMAKE_INSTALL_PREFIX=publish ^
  -DINSTALL_LIBS:BOOL=ON
cmake --build . --target install --config %p_config%

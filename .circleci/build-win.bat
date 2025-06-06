:: Copyright Glen Knowles 2025
:: Distributed under the Boost Software License, Version 1.0.
@echo off
set
cd %~dp0
call %p_init%
cd ..
md build
cd build
cmake .. -G "%p_generator%" -A %p_platform% -T %p_toolset%
cmake . -DCMAKE_BUILD_TYPE=%p_config% -DMAKE_INSTALL_PREFIX=publish ^
  -DINSTALL_LIBS:BOOL=ON
cmake --build . --target install --config %p_config%

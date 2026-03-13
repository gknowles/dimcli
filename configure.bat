:: Copyright Glen Knowles 2016 - 2026.
:: Distributed under the Boost Software License, Version 1.0.
@echo off
setlocal
set toolset=v145
set args=
set args2=
for %%t in (%*) do call :set_arg %%t
goto :build

:set_arg
if %1 == v140 (
    set toolset=%1
    set args=-DCMAKE_SYSTEM_VERSION="10.0.19041.0"
)
if %1 == v140_xp (
    set toolset=%1
    set args=-DCMAKE_SYSTEM_VERSION="10.0.19041.0"
)
if %1 == v141 (
    set toolset=%1
    set args=-DCMAKE_SYSTEM_VERSION="10.0.19041.0"
)
if %1 == v142 (
    set toolset=%1
)
if %1 == v143 (
    set toolset=%1
)
if %1 == v145 (
    set toolset=%1
)
if /i %1 == DLL (
    set args2=-DBUILD_SHARED_LIBS:BOOL=ON -DLINK_STATIC_RUNTIME:BOOL=OFF
)
goto :eof

:build
if exist build rd /s/q build
if exist build goto :eof
md build & cd build
set cmd=cmake .. -G "Visual Studio 18 2026" -A x64 -T %toolset% %args% %args2%
echo %cmd%
%cmd%
cd ..

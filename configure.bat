:: Copyright Glen Knowles 2016 - 2026.
:: Distributed under the Boost Software License, Version 1.0.
@echo off
setlocal
if "%1" equ "" (
    set toolset=v145
) else (
    set toolset=%1
)
if "%toolset%" == "v140" goto :sdk_10.19041
if "%toolset%" == "v140_xp" goto :sdk_10.19041
if "%toolset%" == "v141" goto :sdk_10.19041
goto :build
:sdk_8.1
set args=-DCMAKE_SYSTEM_VERSION="8.1"
goto :build
:sdk_10.19041
set args=-DCMAKE_SYSTEM_VERSION="10.0.19041.0"
goto :build

:build
if exist build rd /s/q build
if exist build goto :eof
md build & cd build
cmake .. -G "Visual Studio 18 2026" -A x64 -T %toolset% %args%
cd ..

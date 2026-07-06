:: Copyright Glen Knowles 2016 - 2026.
:: Distributed under the Boost Software License, Version 1.0.
@echo off
setlocal
set toolset=v145
set args=
set args2=
for %%t in (%*) do call :set_arg "%%t"
if errorlevel 1 goto :eof
if exist build rd /s/q build
if exist build goto :eof
md build & cd build
set cmd=cmake .. -G "Visual Studio 18 2026" -A x64 -T %toolset% %args% %args2%
echo %cmd%
%cmd%
cd ..
goto :eof

:set_arg
if %1 == "v140" (
    set toolset=v140
    set args=-DCMAKE_SYSTEM_VERSION="10.0.19041.0"
    goto :eof
)
if %1 == "v140_xp" (
    set toolset=v140_xp
    set args=-DCMAKE_SYSTEM_VERSION="10.0.19041.0"
    goto :eof
)
if %1 == "v141" (
    set toolset=v141
    set args=-DCMAKE_SYSTEM_VERSION="10.0.19041.0"
    goto :eof
)
if %1 == "v142" (
    set toolset=v142
    goto :eof
)
if %1 == "v143" (
    set toolset=v143
    goto :eof
)
if %1 == "v145" (
    set toolset=v145
    goto :eof
)
if /i %1 == "DLL" (
    set args2=%args2% -DBUILD_SHARED_LIBS:BOOL=ON
    set args2=%args2% -DLINK_STATIC_RUNTIME:BOOL=OFF
    goto :eof
)
if /i %1 == "COVERAGE" (
    set args2=%args2% -DBUILD_COVERAGE:BOOL=ON
    goto :eof
)
for %%s in (help -h --help) do (
    if /i %1 == "%%s" (
        goto :help
    )
)
echo Unknown option: %~1
echo Use 'help' for options.
exit /b 1

:help
echo Usage: configure [ARGS...]
echo.
echo Toolset (If multiple, last one wins).
echo    v140        Visual Studio 2015
echo    v140_xp     Visual Studio 2015 (Windows XP compatible).
echo    v141        Visual Studio 2017
echo    v142        Visual Studio 2019
echo    v143        Visual Studio 2022
echo    v145        Visual Studio 2026
echo.
echo Miscellaneous
echo    dll         Build Shared Libs (Otherwise everything static linked).
echo    coverage    Includes assert tests and, if gcc, code coverage report.
echo.
echo    help        Show this message and exit.
exit /b 1

:: Copyright Glen Knowles 2021.
:: Distributed under the Boost Software License, Version 1.0.
@echo off
call EnvReset restore
call EnvReset snapshot

setlocal
set vsbat_cd=%cd%
set vsbat_where=Microsoft Visual Studio\Installer\vswhere.exe
set vsbat_where=%ProgramFiles(x86)%\%vsbat_where%
set vsbat_wherecmd="%vsbat_where%" ^
  -property installationPath %2 %3 %4 %5 %6 %7 %8 %9
for /f "usebackq tokens=* delims=" %%i in (`%%vsbat_wherecmd%%`) do (
    set vsbat_install=%%i
)
set vsbat_devcmd=%vsbat_install%\Common7\Tools\VsDevCmd.bat

if not exist "%vsbat_devcmd%" (
    endlocal
    call EnvReset rollback
    echo Unable to find %1
    goto :eof
)
endlocal && call "%vsbat_devcmd%" -arch=x64 -host_arch=x64 && cd %vsbat_cd%
call EnvReset commit
goto :eof

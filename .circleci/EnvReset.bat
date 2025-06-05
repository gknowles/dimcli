:: Copyright Glen Knowles 2016 - 2021.
:: Distributed under the Boost Software License, Version 1.0.
@echo off
if /i "%1" equ "" goto :help
if /i "%1" equ "?" goto :help
if /i "%1" equ "/?" goto :help
if /i "%1" equ "help" goto :help
if /i "%1" equ "snapshot" goto :snapshot
if /i "%1" equ "commit" goto :commit
if /i "%1" equ "rollback" goto :rollback
if /i "%1" equ "restore" goto :restore
echo %~nx0: unknown operation '%1'
goto :eof

:help
echo EnvReset version 2019.1
echo.
echo Saves and restores the environment variables by applying (and deleting)
echo saved undo rules, if any. Undo rules are saved as EnvReset* environment
echo variables.
echo.
echo Additional commands exist to generate undo rules:
echo    restore         Apply and remove undo rules if defined.
echo    snapshot        Mark before state.
echo    commit          Saves diff from current to before as undo rules.
echo    rollback        Clear before state markers.
echo    ?, /?, or help  Show this message and exit.
echo.
echo Example usage:
echo    EnvReset snapshot
echo    VsDevCmd
echo    EnvReset commit
echo    ... do stuff
echo    EnvReset
echo.
echo In the above example, if VsDevCmd had failed you would call
echo "EnvReset rollback" to clean up and not save the undo rules.
goto :eof

:snapshot
call :rollback
for /f "tokens=1,2 delims==" %%v in ('set') do (
    call :add_snapshot_var "%%v" "%%w"
)
goto :eof

:: add name to EnvResetSnap_oldenv
:: %1   name to add
:add_snapshot_var
set "EnvResetSnap_%~1=%~2"
goto :eof

:commit
for /f "tokens=1,2 delims==" %%v in ('set') do (
    call :update_undo "%%v" "%%w"
)
call :rollback
goto :eof

:update_undo
set EnvResetName=%~1
if "%EnvResetName:~0,13%"=="EnvResetSnap_" goto :update_undo_snap
:: update_undo_current
set EnvResetSnapName=EnvResetSnap_%EnvResetName%
if not defined %EnvResetSnapName% (
    if defined EnvResetDeleteMe (
        set "EnvResetDeleteMe=%EnvResetDeleteMe% %EnvResetName%"
    ) else (
        set "EnvResetDeleteMe=%EnvResetName%"
    )
)
goto :update_undo_exit

:update_undo_snap:
set EnvResetSnapName=%EnvResetName%
set EnvResetName=%EnvResetSnapName:~13%
set EnvResetUndoName=EnvReset_%EnvResetName%
call :copy_if_diff "%EnvResetUndoName%" "%EnvResetName%" "%~2"

:update_undo_exit
set EnvResetName=
set EnvResetSnapName=
set EnvResetUndoName=
goto :eof

:copy_if_diff
:: Content of variable named by %2 is copied to variable named by %1 if it
:: is different from data in %3
:: %1   name of variable to receive copy
:: %2   name of variable whose content is checked
:: %3   value of expected content
setlocal EnableDelayedExpansion
set EnvResetValue=!%~2!
if "%EnvResetValue%" neq "%~3" (
    endlocal && set "%~1=%~3"
) else (
    endlocal
)
goto :eof

:rollback
set EnvResetSnap_EnvResetSnap_=;
for /f "tokens=1,2 delims==" %%v in ('set EnvResetSnap_') do (
    call :unset_env_name "%%v"
)
set EnvResetName=
set EnvResetValue=
goto :eof

:: Restore from the committed snapshot
::
:: - remove all variables listed in EnvResetDeleteMe
:: - remove EnvResetDeleteMe
:: - for every variable of the form EnvReset_<SOMENAME>
::      - set SOMENAME=%EnvReset_SOMENAME%
::      - set EnvReset_SOMENAME=
:restore
if defined EnvResetDeleteMe (
    for %%a in ("%EnvResetDeleteMe: =" "%") do (
        call :unset_env_name "%%~a"
    )
    set EnvResetDeleteMe=
)
set EnvReset_EnvReset_=;
for /f "tokens=1,2 delims==" %%v in ('set EnvReset_') do (
    call :reset_var "%%v" "%%w"
)
set EnvReset_=
set EnvResetName=
set EnvResetUndoName=
goto :eof

:: remove env name from environment
:: %1   name to unset
:unset_env_name
set %~1=
goto :eof

:: copies old value to current var and clears the old value
:: %1   name with EnvReset_ prefix
:: %2   value
:reset_var
set EnvResetUndoName=%~1
set EnvResetName=%EnvResetUndoName:~9%
set %EnvResetName%=%~2
set %EnvResetUndoName%=
goto :eof

:: Copyright Glen Knowles 2019.
:: Distributed under the Boost Software License, Version 1.0.
@echo off
setlocal
cd %~dp0
copy ..\CHANGELOG.md src /y >nul
mkdocs gh-deploy

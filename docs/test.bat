:: Copyright Glen Knowles 2020.
:: Distributed under the Boost Software License, Version 1.0.
@echo off
setlocal
cd %~dp0
shift
..\..\dimapp\bin\docgen test %*

:: Copyright Glen Knowles 2016 - 2017.
:: Distributed under the Boost Software License, Version 1.0.
@echo off
if exist build rd /s/q build
md build & cd build
cmake -G "Visual Studio 15 2017 Win64" ..
cd ..

:: Copyright Glen Knowles 2016 - 2024.
:: Distributed under the Boost Software License, Version 1.0.
@echo off
if exist build rd /s/q build
md build & cd build
cmake -G "Visual Studio 17 2022" -A x64 -T v143 ..
cd ..

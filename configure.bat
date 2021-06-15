:: Copyright Glen Knowles 2016 - 2021.
:: Distributed under the Boost Software License, Version 1.0.
@echo off
if exist build rd /s/q build
md build & cd build
cmake -G "Visual Studio 16 2019" -A x64 -T v142 ..
cd ..

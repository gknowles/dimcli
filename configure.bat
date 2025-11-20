:: Copyright Glen Knowles 2016 - 2025.
:: Distributed under the Boost Software License, Version 1.0.
@echo off
if exist build rd /s/q build
md build & cd build
cmake -G "Visual Studio 18 2026" -A x64 -T v145 ..
cd ..

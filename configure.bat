@echo off
rd /s/q build
md build & cd build
cmake -G "Visual Studio 15 2017 Win64" ..
cmake .

@echo off
rd /s/q build
md build & cd build
cmake -G "Visual Studio 14 2015 Win64" ..
cmake .

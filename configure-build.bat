@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

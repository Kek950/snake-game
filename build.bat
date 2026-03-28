@echo off
echo Compiling Resources...
windres resource.rc -o resource.o

echo Compiling Application...
g++ main.cpp resource.o -o SnakeGame.exe -mwindows -static -O2 -lcomctl32

echo Done!
pause

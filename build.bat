@ECHO off
gcc main.c .\glad\src\glad.c -o app.exe -I .\glad\include -I C:\Users\henry\dev\libs\glfw-3.4\include -L C:\Users\henry\dev\libs\glfw-3.4\lib-mingw-w64 -lglfw3 -luser32 -lgdi32 -lshell32
.\app.exe
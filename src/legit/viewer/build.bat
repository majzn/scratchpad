@echo off
gcc view.c -o view.exe -mwindows -lws2_32 -lwininet -lgdi32 -luser32 -lkernel32 -lcomdlg32 -lshell32 -lm -lopengl32 -lglu32

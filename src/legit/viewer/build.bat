@echo off
gcc view.c -o view -lws2_32 -lwininet -lgdi32 -lmsimg32

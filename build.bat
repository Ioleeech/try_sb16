@echo off

::: Variables
set PROJECT=try_sb16

set SRC=%PROJECT%.c
set OBJ=%PROJECT%.obj
set BIN=%PROJECT%.exe

::: Commands
if exist %BIN% del %BIN%
if exist %OBJ% del %OBJ%

tcc -1- -A- -C- -G- -K- -N- -O -Z -a -d -f- -g0 -j0 -ms -p- -r -u -v- -w %SRC%

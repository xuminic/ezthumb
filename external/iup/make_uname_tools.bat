@echo off
REM This builds all the tools for 1 uname

call make_uname_lib.bat srcledc %1 %2 %3 %4 %5 %6 %7 %8 %9

call make_uname_lib.bat srcview %1 %2 %3 %4 %5 %6 %7 %8 %9

call make_uname_lib.bat srcvled %1 %2 %3 %4 %5 %6 %7 %8 %9

REM call make_uname_lib.bat srcluaconsole %1 %2 %3 %4 %5 %6 %7 %8 %9
REM call make_uname_lib.bat srcluascripter %1 %2 %3 %4 %5 %6 %7 %8 %9

call make_uname_lib.bat srcluaconsole %1 "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8 %9
call make_uname_lib.bat srcluascripter %1 "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8 %9

call make_uname_lib.bat srcluaconsole %1 "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8 %9
call make_uname_lib.bat srcluascripter %1 "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8 %9

call make_uname_lib.bat srcluaconsole %1 "USE_LUA53=Yes" %2 %3 %4 %5 %6 %7 %8 %9
call make_uname_lib.bat srcluascripter %1 "USE_LUA53=Yes" %2 %3 %4 %5 %6 %7 %8 %9

call make_uname_lib.bat srcluaconsole %1 "USE_LUA54=Yes" %2 %3 %4 %5 %6 %7 %8 %9
call make_uname_lib.bat srcluascripter %1 "USE_LUA54=Yes" %2 %3 %4 %5 %6 %7 %8 %9

call make_uname_lib.bat html\examples\tests %1 %2 %3 %4 %5 %6 %7 %8 %9
cd ..\..

call make_uname_lib.bat html\examples\tutorial\simple_paint2 %1 %2 %3 %4 %5 %6 %7 %8 %9
cd ..\..\..

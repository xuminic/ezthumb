@echo off
REM This builds all the libraries for 1 uname

FOR %%u IN (src srccd srccontrols srcgl srcglcontrols srcplot srcmglplot srcscintilla srcim srcimglib srcole srcfiledlg srctuio srcweb) DO call make_uname_lib.bat %%u %1 %2 %3 %4 %5 %6 %7 %8 %9

REM call make_uname_lib.bat srclua5 %1 %2 %3 %4 %5 %6 %7 %8 %9
call make_uname_lib.bat srclua5 %1 "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8 %9
call make_uname_lib.bat srclua5 %1 "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8 %9
call make_uname_lib.bat srclua5 %1 "USE_LUA53=Yes" %2 %3 %4 %5 %6 %7 %8 %9
call make_uname_lib.bat srclua5 %1 "USE_LUA54=Yes" %2 %3 %4 %5 %6 %7 %8 %9

call make_uname_tools.bat %1 %2 %3 %4 %5 %6 %7 %8 %9

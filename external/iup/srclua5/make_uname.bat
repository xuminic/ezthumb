@echo off
REM This builds all the libraries of the folder for 1 uname

call tecmake %1 "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupcd" "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupcontrols" "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupmatrixex" "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupgl" "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iup_pplot" "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iup_mglplot" "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iup_scintilla" "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupim" "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupimglib" "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iuptuio" "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupole" "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupweb" "USE_LUA51=Yes" %2 %3 %4 %5 %6 %7 %8

call tecmake %1 "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupcd" "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupcontrols" "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupmatrixex" "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupgl" "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iup_pplot" "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iup_mglplot" "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iup_scintilla" "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupim" "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupimglib" "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iuptuio" "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupole" "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8
call tecmake %1 "MF=iupweb" "USE_LUA52=Yes" %2 %3 %4 %5 %6 %7 %8

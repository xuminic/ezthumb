@echo off  

if "%1"==""       goto iupexe32
if "%1"=="vc10"   goto iupexe32
if "%1"=="vc10_64" goto iupexe64
if "%1"=="cygw17"  goto iupexecygw
goto end

REM Must use dll* so USE_* will use the correct TEC_UNAME

:iupexe32
call tecmake dll10 "USE_LUA51=Yes" relink %2 %3 %4 %5 %6 %7
call tecmake dll10 "USE_LUA52=Yes" relink %2 %3 %4 %5 %6 %7
if not defined TECGRAF_INTERNAL goto end
call tecmake vc10 "MF=iuplua3" relink %2 %3 %4 %5 %6 %7
goto end

:iupexe64
call tecmake dll10_64 "USE_LUA51=Yes" relink %2 %3 %4 %5 %6 %7
call tecmake dll10_64 "USE_LUA52=Yes" relink %2 %3 %4 %5 %6 %7
if not defined TECGRAF_INTERNAL goto end
call tecmake vc10_64 "MF=iuplua3" relink %2 %3 %4 %5 %6 %7
goto end

:iupexecygw
call tecmake cygw17 "USE_LUA51=Yes" relink %2 %3 %4 %5 %6 %7
call tecmake cygw17 "USE_LUA52=Yes" relink %2 %3 %4 %5 %6 %7
if not defined TECGRAF_INTERNAL goto end
call tecmake cygw17 "MF=iuplua3" relink %2 %3 %4 %5 %6 %7
goto end

:end

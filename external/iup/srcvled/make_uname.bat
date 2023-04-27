@echo off  

if "%1"==""     goto iupexe32
if "%1"=="vc15"  goto iupexe32
if "%1"=="vc15_64"  goto iupexe64
if "%1"=="cygw17"  goto iupexecygw
if "%1"=="all"  goto iupexeall
goto end

:iupexe32
call tecmake vc15 %2 %3 %4 %5 %6 %7
REM call tecmake vc15 "USE_GTK=Yes" relink %2 %3 %4 %5 %6 %7
goto end

:iupexe64
call tecmake vc15_64 %2 %3 %4 %5 %6 %7
REM call tecmake vc15_64 "USE_GTK=Yes" relink %2 %3 %4 %5 %6 %7
goto end

:iupexecygw
call tecmake cygw17 relink %2 %3 %4 %5 %6 %7
REM call tecmake cygw17 "USE_GTK=Yes" relink %2 %3 %4 %5 %6 %7
goto end

:iupexeall
call make_uname vc15 %2 %3 %4 %5 %6 %7
call make_uname vc15_64 %2 %3 %4 %5 %6 %7
REM call make_uname cygw17 %2 %3 %4 %5 %6 %7
goto end

:end

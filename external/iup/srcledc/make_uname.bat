@echo off  

if "%1"==""     goto iupexe32
if "%1"=="vc10"  goto iupexe32
if "%1"=="vc10_64"  goto iupexe64
if "%1"=="all"  goto iupexe
goto end

:iupexe32
call tecmake vc10 relink %2 %3 %4 %5 %6 %7
goto end

:iupexe64
call tecmake vc10_64 relink %2 %3 %4 %5 %6 %7
goto end

:iupexe
call tecmake vc10 relink %2 %3 %4 %5 %6 %7
call tecmake vc10_64 relink %2 %3 %4 %5 %6 %7
goto end

:end

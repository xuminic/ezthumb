@echo off

echo English
FOR %%u IN (english english_matrix english_plot) DO ledc -f iup_load_lng_%%u -o iup_lng_%%u.h %%u.led
echo Portuguese
FOR %%u IN (portuguese portuguese_matrix portuguese_plot) DO ledc -f iup_load_lng_%%u -o iup_lng_%%u.h %%u.led
echo Spanish
FOR %%u IN (spanish spanish_matrix spanish_plot) DO ledc -f iup_load_lng_%%u -o iup_lng_%%u.h %%u.led

REM echo Others
REM FOR %%u IN (czech_utf8 russian_utf8) DO ledc -f iup_load_lng_%%u -o iup_lng_%%u.h %%u.led

REM Must edit the generated file to return the handle

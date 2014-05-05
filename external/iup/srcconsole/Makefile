ifeq ($(OS), Windows_NT)
  TECMAKE_CMD = $(MAKE) --no-print-directory -f ../tecmakewin.mak TEC_UNAME=$(TEC_UNAME)
else
  TECMAKE_CMD = $(MAKE) --no-print-directory -f ../tecmake.mak
endif

.PHONY: do_all iuplua5 iuplua5gtk iuplua3
do_all: iuplua5
 
iuplua5:
	@$(TECMAKE_CMD) USE_LUA51=Yes
	@$(TECMAKE_CMD) USE_LUA52=Yes

iuplua5gtk:
	@$(TECMAKE_CMD) USE_GTK=Yes

iuplua3:
	@$(TECMAKE_CMD) MF=iuplua3

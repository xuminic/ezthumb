ifeq ($(OS), Windows_NT)
  TECMAKE_CMD = $(MAKE) --no-print-directory -f ../tecmakewin.mak TEC_UNAME=$(TEC_UNAME)
  WINLIBS = iupole
else
  TECMAKE_CMD = $(MAKE) --no-print-directory -f ../tecmake.mak
endif

.PHONY: do_all iuplua iupluacd iupluacontrols iupluagl iuplua_pplot iuplua_mglplot iuplua_scintilla iupluaim iupluaimglib iupluatuio iupluaole iupluaweb
do_all: iuplua iupluacd iupluacontrols iupluagl iuplua_pplot iuplua_mglplot iuplua_scintilla iupluaim iupluaimglib iupluatuio $(WINLIBS)

iuplua:
	@$(TECMAKE_CMD) USE_LUA51=Yes 
	@$(TECMAKE_CMD) USE_LUA52=Yes 

iupluacd:
	@$(TECMAKE_CMD) MF=iupcd USE_LUA51=Yes
	@$(TECMAKE_CMD) MF=iupcd USE_LUA52=Yes

iupluacontrols:
	@$(TECMAKE_CMD) MF=iupcontrols USE_LUA51=Yes
	@$(TECMAKE_CMD) MF=iupcontrols USE_LUA52=Yes

iupluagl:
	@$(TECMAKE_CMD) MF=iupgl USE_LUA51=Yes
	@$(TECMAKE_CMD) MF=iupgl USE_LUA52=Yes

iuplua_pplot:
	@$(TECMAKE_CMD) MF=iup_pplot USE_LUA51=Yes
	@$(TECMAKE_CMD) MF=iup_pplot USE_LUA52=Yes

iuplua_mglplot:
	@$(TECMAKE_CMD) MF=iup_mglplot USE_LUA51=Yes
	@$(TECMAKE_CMD) MF=iup_mglplot USE_LUA52=Yes

iuplua_scintilla:
	@$(TECMAKE_CMD) MF=iup_scintilla USE_LUA51=Yes
	@$(TECMAKE_CMD) MF=iup_scintilla USE_LUA52=Yes

iupluaim:
	@$(TECMAKE_CMD) MF=iupim USE_LUA51=Yes
	@$(TECMAKE_CMD) MF=iupim USE_LUA52=Yes

iupluaimglib:
	@$(TECMAKE_CMD) MF=iupimglib USE_LUA51=Yes
	@$(TECMAKE_CMD) MF=iupimglib USE_LUA52=Yes
  
iupluatuio:
	@$(TECMAKE_CMD) MF=iuptuio USE_LUA51=Yes
	@$(TECMAKE_CMD) MF=iuptuio USE_LUA52=Yes
  
iupluaole:
	@$(TECMAKE_CMD) MF=iupole USE_LUA51=Yes
	@$(TECMAKE_CMD) MF=iupole USE_LUA52=Yes
  
iupluaweb:
	@$(TECMAKE_CMD) MF=iupweb USE_LUA51=Yes
	@$(TECMAKE_CMD) MF=iupweb USE_LUA52=Yes

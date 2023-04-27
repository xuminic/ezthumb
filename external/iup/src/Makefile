ifeq ($(OS), Windows_NT)
  TECMAKE_CMD = $(MAKE) --no-print-directory -f ../tecmakewin.mak TEC_UNAME=$(TEC_UNAME)
else
  TECMAKE_CMD = $(MAKE) --no-print-directory -f ../tecmake.mak
  TECMAKE_EMS_CMD = $(MAKE) --no-print-directory -f ../tecmake_emscripten.mak
endif

.PHONY: do_all iup iupgtk iupmot iuphaiku iupstub
do_all: iup

iup:
	@$(TECMAKE_CMD) 

iupgtk:
	@$(TECMAKE_CMD) USE_GTK=Yes

iupmot:
	@$(TECMAKE_CMD) USE_MOTIF=Yes

iuphaiku:
	@$(TECMAKE_CMD) USE_HAIKU=Yes

iupemscripten:
	@$(TECMAKE_EMS_CMD) USE_EMSCRIPTEN=Yes

iupstub:
	@$(TECMAKE_CMD) MF=iupstub

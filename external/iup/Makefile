ifeq ($(OS), Windows_NT)
  WINLIBS = iupole
  export MINGW4=/c/mingw
  export TEC_UNAME=mingw4
endif

# quick decision which GTK is available
ifndef	USE_GTK2
gtkver := $(shell pkg-config gtk+-3.0 --cflags)
ifeq ($(gtkver), )
  export USE_GTK2=1
endif
endif

.PHONY: do_all iup iupgtk iupmot iupcd iupcontrols iupmatrixex iupgl iupglcontrols iup_plot iup_mglplot iup_scintilla iupim iupimglib ledc iupview iuplua5 iupconsole iupole iupweb iuptuio
#do_all: iup iupcd iupcontrols iupgl iupglcontrols iupmatrixex iup_plot iup_mglplot iup_scintilla iupim iupimglib $(WINLIBS) iupweb iuptuio ledc iupview iuplua5 iupconsole
do_all: iup iupimglib $(WINLIBS)
ifdef	USE_GTK2
	echo 2 > VERSION.gtk
else
	echo 3 > VERSION.gtk
endif

clean:
	rm -rf ./lib/* ./obj/* VERSION.gtk

.PHONY: install
install:
	cp -f  ./lib/mingw4/*.a ../../libmingw/lib
	cp -af ./include/* ../../libmingw/include/iup

iup iupgtk iupmot:
	@$(MAKE) --no-print-directory -C ./src/ $@
iupcd:
	@$(MAKE) --no-print-directory -C ./srccd/
iupcontrols:
	@$(MAKE) --no-print-directory -C ./srccontrols/
iupmatrixex:
	@$(MAKE) --no-print-directory -C ./srcmatrixex/
iup_plot:
	@$(MAKE) --no-print-directory -C ./srcplot/
iup_mglplot:
	@$(MAKE) --no-print-directory -C ./srcmglplot/
iup_scintilla:
	@$(MAKE) --no-print-directory -C ./srcscintilla/
iupgl:
	@$(MAKE) --no-print-directory -C ./srcgl/
iupglcontrols:
	@$(MAKE) --no-print-directory -C ./srcglcontrols/
iupim:
	@$(MAKE) --no-print-directory -C ./srcim/
iupole:
	@$(MAKE) --no-print-directory -C ./srcole/
iupweb:
	@$(MAKE) --no-print-directory -C ./srcweb/
iuptuio:
	@$(MAKE) --no-print-directory -C ./srctuio/
iupimglib:
	@$(MAKE) USE_IUP_IMGLIB_LARGE=1 --no-print-directory -C ./srcimglib/
iuplua5:
	@$(MAKE) --no-print-directory -C ./srclua5/
iupconsole:
	@$(MAKE) --no-print-directory -C ./srcconsole/
ledc:
	@$(MAKE) --no-print-directory -C ./srcledc/
iupview: iupcontrols iup
	@$(MAKE) --no-print-directory -C ./srcview/

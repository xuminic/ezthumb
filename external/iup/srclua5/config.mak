PROJNAME = iup
LIBNAME  = iuplua

OPT = YES
NO_LUAOBJECT = Yes
NO_LUALINK = Yes
USE_BIN2C_LUA = Yes

DEF_FILE = iuplua.def
INCLUDES = ../include ../src
LDIR = ../lib/$(TEC_UNAME)  
LIBS = iup

ifdef USE_LUA52
  LUASFX = 52
  DEFINES += LUA_COMPAT_MODULE
else
  USE_LUA51 = Yes
  LUASFX = 51
endif

LIBNAME := $(LIBNAME)$(LUASFX)
ifdef NO_LUAOBJECT
  DEFINES += IUPLUA_USELH
  USE_LH_SUBDIR = Yes
  LHDIR = lh
else
  DEFINES += IUPLUA_USELOH
  USE_LOH_SUBDIR = Yes
  LOHDIR = loh$(LUASFX)
endif

CTRLUA = button.lua canvas.lua dialog.lua colordlg.lua clipboard.lua \
       filedlg.lua fill.lua frame.lua hbox.lua normalizer.lua gridbox.lua \
       item.lua image.lua imagergb.lua imagergba.lua label.lua expander.lua \
       link.lua menu.lua multiline.lua list.lua separator.lua user.lua \
       submenu.lua text.lua toggle.lua vbox.lua zbox.lua timer.lua detachbox.lua \
       sbox.lua scrollbox.lua split.lua spin.lua spinbox.lua cbox.lua \
       radio.lua val.lua tabs.lua fontdlg.lua tree.lua progressbar.lua \
       messagedlg.lua progressdlg.lua backgroundbox.lua

GC := $(addsuffix .c, $(basename $(CTRLUA)))
GC := $(addprefix il_, $(GC))

SRCLUA = iuplua.lua constants.lua $(CTRLUA)

$(GC) : il_%.c : %.lua generator.lua
	$(LUABIN) generator.lua $<

SRC = iuplua.c iuplua_api.c iuplua_tree_aux.c iuplua_scanf.c iuplua_getparam.c iuplua_getcolor.c $(GC)

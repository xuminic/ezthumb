PROJNAME = iup
LIBNAME = iupluafiledlg

IUP := ..

OPT = YES
# To not link with the Lua dynamic library in UNIX
NO_LUALINK = Yes
# To use a subfolder with the Lua version for binaries
LUAMOD_DIR = Yes
NO_LUAOBJECT = Yes
DEPENDDIR = dep

USE_IUPLUA = Yes

INCLUDES = ../srclua5
LIBS = iupfiledlg
DEF_FILE = ctrl/iupluafiledlg.def

ifdef USE_LUA_VERSION
  USE_LUA51:=
  USE_LUA52:=
  USE_LUA53:=
  USE_LUA54:=
  ifeq ($(USE_LUA_VERSION), 54)
    USE_LUA54:=Yes
  endif
  ifeq ($(USE_LUA_VERSION), 53)
    USE_LUA53:=Yes
  endif
  ifeq ($(USE_LUA_VERSION), 52)
    USE_LUA52:=Yes
  endif
  ifeq ($(USE_LUA_VERSION), 51)
    USE_LUA51:=Yes
  endif
endif

ifdef USE_LUA54
  LUASFX = 54
else
ifdef USE_LUA53
  LUASFX = 53
else
ifdef USE_LUA52
  LUASFX = 52
else
  USE_LUA51 = Yes
  LUASFX = 51
endif
endif
endif

LIBNAME := $(LIBNAME)$(LUASFX)

SRC	= ctrl/iuplua_filedlg.c

ifneq ($(findstring mingw, $(TEC_UNAME)), )
  $(error No support for NewFileDlg in MingW)
endif
ifneq ($(findstring dllw, $(TEC_UNAME)), )
  $(error No support for NewFileDlg in MingW)
endif

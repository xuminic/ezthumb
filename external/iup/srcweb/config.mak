PROJNAME = iup
LIBNAME = iupweb
OPT = YES

INCLUDES =  ../include ../src
LDIR = ../lib/$(TEC_UNAME)  
LIBS = iup
SRC = iup_webbrowser.c

ifeq ($(findstring Win, $(TEC_SYSNAME)), )
  DEPENDDIR = dep
endif

ifneq ($(findstring Win, $(TEC_SYSNAME)), )
  SRC += iupwin_webbrowser.cpp
  LIBS += iupole comsuppw
  USE_ATL = Yes
  DEFINES = _MBCS
else
  ifdef GTK_DEFAULT
    ifdef GTK_BASE
      GTK := $(GTK_BASE)
    else
      ifneq ($(findstring MacOS, $(TEC_UNAME)), )
        GTK = /sw
      else
        GTK = /usr
      endif
    endif
  
    SRC  += iupgtk_webbrowser.c
    USE_GTK = Yes
    INCLUDES += ../src/gtk
    STDINCS += $(GTK)/include/libsoup-2.4
    LINK_WEBKIT = Yes
    
    ifdef USE_GTK3
      ifneq ($(findstring Linux5, $(TEC_UNAME)), )
        DEFINES += USE_WEBKIT2
        STDINCS += $(GTK)/include/webkitgtk-4.0
      else
        ifneq ($(findstring Linux4, $(TEC_UNAME)), )
          DEFINES += USE_WEBKIT2
          STDINCS += $(GTK)/include/webkitgtk-4.0
        else
          STDINCS += $(GTK)/include/webkitgtk-3.0
        endif
      endif
    else 
      ifneq ($(findstring Linux3, $(TEC_UNAME)), )
        STDINCS += $(GTK)/include/webkitgtk-1.0
      else
        STDINCS += $(GTK)/include/webkit-1.0
      endif
    endif
  else
#    SRC = iupmot_webbrowser.c
#    LIBS += XmHTML
  endif
endif

ifneq ($(findstring MacOS, $(TEC_UNAME)), )
  ifneq ($(TEC_SYSMINOR), 4)
    BUILD_DYLIB=Yes
  endif
endif

ifneq ($(findstring mingw, $(TEC_UNAME)), )
  $(error No support for WebBrowser in MingW)
endif
ifneq ($(findstring dllw, $(TEC_UNAME)), )
  $(error No support for WebBrowser in MingW)
endif
ifneq ($(findstring owc, $(TEC_UNAME)), )
  $(error No support for WebBrowser in OpenWatcom)
endif
ifneq ($(findstring bc, $(TEC_UNAME)), )
  $(error No support for WebBrowser in BorlandC)
endif

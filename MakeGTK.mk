# Reload the previous GTK setting from IUP, if exists.
# so we don't need prefix USE_GTK2 every time to build renamex
GTKVER := $(shell cat ./external/iup/VERSION.gtk 2> /dev/null)
ifeq	($(GTKVER),2)
  USE_GTK2 = 1
endif

ifdef	USE_GTK2
  GTKINC := $(shell pkg-config gtk+-2.0 --cflags)
  GTKLIB := $(shell pkg-config gtk+-2.0 --libs)
  IUPCFG = USE_GTK2=$(USE_GTK2)
else	# auto-detect
  GTKINC := $(shell pkg-config gtk+-3.0 --cflags 2> /dev/null)
  GTKLIB := $(shell pkg-config gtk+-3.0 --libs 2> /dev/null)
  ifeq  ($(GTKINC), )
    GTKINC := $(shell pkg-config gtk+-2.0 --cflags)
    GTKLIB := $(shell pkg-config gtk+-2.0 --libs)
    IUPCFG = USE_GTK2=1	# No GTK3 installed
  endif
endif

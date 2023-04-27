PROJNAME = iup
LIBNAME  = iup_plot
OPT = YES

USE_CD = Yes
USE_OPENGL = Yes

ifdef DBG
  DEFINES += IUP_ASSERT
  ifndef DBG_DIR
    ifneq ($(findstring Win, $(TEC_SYSNAME)), )
      LIBNAME := $(LIBNAME)_debug
    endif
  endif  
endif  

DEF_FILE = iup_plot.def

INCLUDES = ../include ../src ../srccd
LDIR = ../lib/$(TEC_UNAME)  
LIBS = iup iupgl iupcd cdgl 

DEFINES = CD_NO_OLD_INTERFACE

USE_CONTEXTPLUS = Yes
ifdef USE_CONTEXTPLUS
  LIBS += cdcontextplus
  DEFINES += USE_CONTEXTPLUS
endif

SRC = iup_plot_ctrl.cpp iup_plot_attrib.cpp iupPlot.cpp iupPlotCalc.cpp iupPlotDraw.cpp iupPlotAxis.cpp iupPlotData.cpp iupPlotTick.cpp

ifneq ($(findstring MacOS, $(TEC_UNAME)), )
  INCLUDES += $(X11_INC)
  ifneq ($(TEC_SYSMINOR), 4)
    BUILD_DYLIB=Yes
  endif
endif

ifneq ($(findstring Linux, $(TEC_UNAME)), )
  CPPFLAGS = -Wno-reorder -Wno-write-strings
endif
ifneq ($(findstring cygw, $(TEC_UNAME)), )
  CPPFLAGS = -Wno-reorder -Wno-write-strings
endif

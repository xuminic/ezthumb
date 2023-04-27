PROJNAME = iup
LIBNAME = iupfiledlg
OPT = YES

INCLUDES =  ../include ../src ../src/win
LDIR = ../lib/$(TEC_UNAME)  
LIBS = iup shlwapi

SRC = iupwin_newfiledlg.cpp

# It does not compile under MingW
# Only Visual C++ is supported

ifneq ($(findstring mingw, $(TEC_UNAME)), )
  $(error No support for NewFileDlg in MingW)
endif
ifneq ($(findstring dllw, $(TEC_UNAME)), )
  $(error No support for NewFileDlg in MingW)
endif
ifneq ($(findstring owc, $(TEC_UNAME)), )
  $(error No support for NewFileDlg in OpenWatcom)
endif
ifneq ($(findstring bc, $(TEC_UNAME)), )
  $(error No support for NewFileDlg in BorlandC)
endif

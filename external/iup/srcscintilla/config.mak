PROJNAME = iup
LIBNAME = iup_scintilla
OPT = YES

# Supported only in Windows and GTK

ifdef DBG
  DEFINES += IUP_ASSERT
  ifndef DBG_DIR
    ifneq ($(findstring Win, $(TEC_SYSNAME)), )
      LIBNAME := $(LIBNAME)_debug
    endif
  endif
endif  

DEF_FILE = iup_scintilla.def

INCLUDES =  ../include ../src .
LDIR = ../lib/$(TEC_UNAME)
LIBS = iup

DEFINES += STATIC_BUILD SCI_LEXER SCI_NAMESPACE

# Used only in Linux
LINKER = $(CPPC)
LD = $(CPPC)

ifeq ($(findstring Win, $(TEC_SYSNAME)), )
  # Force definition if not in Windows
  USE_GTK = Yes
endif

ifeq ($(findstring Win, $(TEC_SYSNAME)), )
  DEPENDDIR = dep
  
  GCC_VERSION := $(shell $(CPPC) -dumpversion)  
  ifeq ($(shell expr $(GCC_VERSION) '>=' 4.8), 1)
    HAVE_CPP11 := Yes
  endif
else
  ifneq ($(findstring dll14, $(TEC_UNAME)), )
    HAVE_CPP11 := Yes
  endif
  ifneq ($(findstring dll15, $(TEC_UNAME)), )
    HAVE_CPP11 := Yes
  endif
  ifneq ($(findstring dllw6, $(TEC_UNAME)), )
    HAVE_CPP11 := Yes
  endif
  ifneq ($(findstring vc14, $(TEC_UNAME)), )
    HAVE_CPP11 := Yes
  endif
  ifneq ($(findstring vc15, $(TEC_UNAME)), )
    HAVE_CPP11 := Yes
  endif
  ifneq ($(findstring mingw6, $(TEC_UNAME)), )
    HAVE_CPP11 := Yes
  endif
endif

ifndef GTK_DEFAULT
  ifdef USE_GTK
    # Build GTK version in IRIX,SunOS,AIX,Win32
    LIBNAME := $(LIBNAME)gtk
  endif
endif

ifdef SCINTILLA_OLD
  # CentOS 5
  SCINTILLA_NUMBER := 353
  SCINTILLA_VERSION := 3.5.3
else
  ifdef HAVE_CPP11
    SCINTILLA_NEW = Yes
  endif
  
  ifdef SCINTILLA_NEW
    # minimum GCC 4.8 (Linux313_64) and MSVC 2015 (vc14)
    # Needs C++ 11 support
    USE_CPP11 = Yes
    
    # Notice: We are not using Scintilla 4.x because
    # it uses C++14 features, and requires Microsoft Visual C++ 2017 and GCC 7.
    
    # TODO: compile scintilla3112 in all CPP11 systems,
    #       if OK then remove scintilla375
    SCINTILLA3112 = Yes
    
    ifdef SCINTILLA3112
      SCINTILLA_NUMBER := 3112
      SCINTILLA_VERSION := 3.11.2
      
      ifdef LPEG_LEXER
        DEFINES += LPEG_LEXER
        
        # To not link with the Lua dynamic library in UNIX
        NO_LUALINK = Yes
        
        # Depends on Lua
        ifdef USE_LUA_VERSION
          USE_LUA51:=
          USE_LUA52:=
          USE_LUA53:=
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
        
        # Depends on LPEG
        ifdef LINK_LPEG
          DEFINES += LINK_LPEG
          # No need for includes
          LIBS += $(LPEG)
        endif
      endif
    else
      SCINTILLA_NUMBER := 375
      SCINTILLA_VERSION := 3.7.5
    endif
  else
    SCINTILLA_NUMBER := 366
    SCINTILLA_VERSION := 3.6.6
  endif
endif

INCLUDES += scintilla$(SCINTILLA_NUMBER)/lexlib scintilla$(SCINTILLA_NUMBER)/src scintilla$(SCINTILLA_NUMBER)/include
DEFINES += SCINTILLA_VERSION='"$(SCINTILLA_VERSION)"'

ifdef USE_GTK
  CHECK_GTK = Yes
  ifndef USE_CPP11
    DEFINES += NO_CXX11_REGEX
  endif
  DEFINES += GTK GTK_DISABLE_DEPRECATED 
  ifdef USE_GTK3
    DEFINES += GDK_DISABLE_DEPRECATED GSEAL_ENABLE G_HAVE_ISO_VARARGS
  endif
  INCLUDES += ../src/gtk scintilla$(SCINTILLA_NUMBER)/gtk
  ifneq ($(findstring cygw, $(TEC_UNAME)), )
    INCLUDES += $(GTK)/include/cairo
    LIBS += pangocairo-1.0 cairo
  endif
  ifdef SCINTILLA_NEW
    LIBS += atk-1.0
  endif
else
  INCLUDES += ../src/win scintilla$(SCINTILLA_NUMBER)/win32
  LIBS += imm32
  DEFINES += UNICODE
  
  ifdef SCINTILLA_NEW
    LIBS += msimg32
    DEFINES += _SCL_SECURE_NO_WARNINGS
  endif
  
  ifneq ($(findstring gcc, $(TEC_UNAME)), )
    DEFINES += _WIN32 DISABLE_D2D NO_CXX11_REGEX
  endif
  ifneq ($(findstring dllg, $(TEC_UNAME)), )
    DEFINES += _WIN32 DISABLE_D2D NO_CXX11_REGEX
  endif
  ifneq ($(findstring mingw, $(TEC_UNAME)), )
    DEFINES += _WIN32 DISABLE_D2D NO_CXX11_REGEX
  endif
  ifneq ($(findstring dllw, $(TEC_UNAME)), )
    DEFINES += _WIN32 DISABLE_D2D NO_CXX11_REGEX
  endif
endif

SCINTILLA_SRC = src/AutoComplete.cxx src/CallTip.cxx src/Catalogue.cxx src/CellBuffer.cxx src/CharClassify.cxx \
               src/ContractionState.cxx src/Decoration.cxx src/Document.cxx src/Editor.cxx src/ExternalLexer.cxx \
               src/Indicator.cxx src/KeyMap.cxx src/LineMarker.cxx src/PerLine.cxx src/PositionCache.cxx \
               src/RESearch.cxx src/RunStyles.cxx src/ScintillaBase.cxx src/Selection.cxx src/Style.cxx \
               src/UniConversion.cxx src/ViewStyle.cxx src/XPM.cxx src/CaseConvert.cxx src/CaseFolder.cxx \
               src/EditModel.cxx src/EditView.cxx src/MarginView.cxx
               
ifdef SCINTILLA3112
  SCINTILLA_SRC += src/DBCS.cxx src/UniqueString.cxx
endif

SCINTILLA_SRC += lexers/LexA68k.cxx lexers/LexAbaqus.cxx lexers/LexAda.cxx lexers/LexAPDL.cxx \
				lexers/LexAsn1.cxx lexers/LexASY.cxx lexers/LexAU3.cxx lexers/LexAVE.cxx lexers/LexAVS.cxx \
				lexers/LexBaan.cxx lexers/LexBash.cxx lexers/LexBasic.cxx lexers/LexBullant.cxx lexers/LexCaml.cxx \
				lexers/LexCLW.cxx lexers/LexCmake.cxx lexers/LexCOBOL.cxx lexers/LexCoffeeScript.cxx \
				lexers/LexConf.cxx lexers/LexCPP.cxx lexers/LexCrontab.cxx lexers/LexCsound.cxx lexers/LexCSS.cxx \
				lexers/LexD.cxx lexers/LexECL.cxx lexers/LexEiffel.cxx lexers/LexErlang.cxx lexers/LexEScript.cxx \
				lexers/LexFlagship.cxx lexers/LexForth.cxx lexers/LexFortran.cxx lexers/LexGAP.cxx \
				lexers/LexGui4Cli.cxx lexers/LexHaskell.cxx lexers/LexHTML.cxx lexers/LexInno.cxx \
				lexers/LexKix.cxx lexers/LexLisp.cxx lexers/LexLout.cxx lexers/LexLua.cxx lexers/LexMagik.cxx \
				lexers/LexMarkdown.cxx lexers/LexMatlab.cxx lexers/LexMetapost.cxx lexers/LexMMIXAL.cxx \
				lexers/LexModula.cxx lexers/LexMPT.cxx lexers/LexMSSQL.cxx lexers/LexMySQL.cxx \
				lexers/LexNimrod.cxx lexers/LexNsis.cxx lexers/LexOpal.cxx lexers/LexOScript.cxx \
				lexers/LexPascal.cxx lexers/LexPB.cxx lexers/LexPerl.cxx \
				lexers/LexPLM.cxx lexers/LexPO.cxx lexers/LexPOV.cxx lexers/LexPowerPro.cxx \
				lexers/LexPowerShell.cxx lexers/LexProgress.cxx lexers/LexPS.cxx lexers/LexPython.cxx \
				lexers/LexR.cxx lexers/LexRebol.cxx lexers/LexRuby.cxx lexers/LexScriptol.cxx \
				lexers/LexSmalltalk.cxx lexers/LexSML.cxx lexers/LexSorcus.cxx lexers/LexSpecman.cxx \
				lexers/LexSpice.cxx lexers/LexSQL.cxx lexers/LexTACL.cxx lexers/LexTADS3.cxx lexers/LexTAL.cxx \
				lexers/LexTCL.cxx lexers/LexTCMD.cxx lexers/LexTeX.cxx lexers/LexTxt2tags.cxx lexers/LexVB.cxx \
				lexers/LexVerilog.cxx lexers/LexVHDL.cxx lexers/LexVisualProlog.cxx lexers/LexYAML.cxx \
				lexers/LexKVIrc.cxx lexers/LexLaTeX.cxx lexers/LexSTTXT.cxx lexers/LexRust.cxx \
				lexers/LexDMAP.cxx lexers/LexDMIS.cxx lexers/LexBibTeX.cxx lexers/LexHex.cxx lexers/LexAsm.cxx \
				lexers/LexRegistry.cxx lexers/LexLed.cxx
        
ifdef SCINTILLA_OLD
  SCINTILLA_SRC += lexers/LexOthers.cxx
else
  SCINTILLA_SRC += lexers/LexBatch.cxx lexers/LexDiff.cxx lexers/LexErrorList.cxx \
                  lexers/LexMake.cxx lexers/LexNull.cxx lexers/LexProps.cxx lexers/LexJSON.cxx
endif

ifdef SCINTILLA_NEW
  SCINTILLA_SRC += lexers/LexEDIFACT.cxx lexers/LexIndent.cxx
  ifdef SCINTILLA3112
    SCINTILLA_SRC += lexers/LexCIL.cxx lexers/LexDataflex.cxx lexers/LexHollywood.cxx \
                    lexers/LexMaxima.cxx lexers/LexNim.cxx \
                    lexers/LexSAS.cxx lexers/LexStata.cxx lexers/LexX12.cxx \
                    lexers/LexLPeg.cxx 
  endif
endif

SCINTILLA_SRC += lexlib/Accessor.cxx lexlib/CharacterSet.cxx lexlib/LexerBase.cxx lexlib/LexerModule.cxx \
                lexlib/LexerNoExceptions.cxx lexlib/LexerSimple.cxx lexlib/PropSetSimple.cxx \
                lexlib/StyleContext.cxx lexlib/WordList.cxx lexlib/CharacterCategory.cxx
ifdef SCINTILLA3112
  SCINTILLA_SRC += lexlib/DefaultLexer.cxx
endif

ifdef USE_GTK
  SCINTILLA_SRC += gtk/PlatGTK.cxx gtk/ScintillaGTK.cxx gtk/scintilla-marshal.c
  ifdef SCINTILLA_NEW
    SCINTILLA_SRC += gtk/ScintillaGTKAccessible.cxx
  endif
else
  SCINTILLA_SRC += win32/PlatWin.cxx win32/ScintillaWin.cxx
  ifndef SCINTILLA_OLD
    SCINTILLA_SRC += win32/HanjaDic.cxx
  endif
  
  ifdef SCINTILLA3112
    ifneq ($(findstring dll, $(TEC_UNAME)), )
      SCINTILLA_SRC += win32/ScintillaDLL.cxx
    endif
  endif
  
endif

SCINTILLA_SRC := $(addprefix scintilla$(SCINTILLA_NUMBER)/, $(SCINTILLA_SRC))

SRC = $(SCINTILLA_SRC) iupsci_clipboard.c iupsci_folding.c iupsci_lexer.c iupsci_margin.c \
      iupsci_overtype.c iupsci_scrolling.c iupsci_selection.c iupsci_style.c iupsci_tab.c \
      iupsci_text.c iupsci_wordwrap.c iupsci_markers.c iupsci_bracelight.c iupsci_cursor.c \
      iupsci_whitespace.c iupsci_annotation.c iupsci_autocompletion.c iupsci_searching.c  \
      iupsci_print.c iupsci_indicator.c iup_scintilla.c iup_scintilladlg.c 
ifdef USE_GTK
  SRC += iup_scintilla_gtk.c 
else
  SRC += iup_scintilla_win.c 
endif

ifneq ($(findstring MacOS, $(TEC_UNAME)), )
  ifneq ($(TEC_SYSMINOR), 4)
    BUILD_DYLIB=Yes
  endif
endif

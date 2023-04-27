/** \file
 * \brief Scintilla control: Lexer
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#include <Scintilla.h>
#include <SciLexer.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"

#include "iupsci.h"

/***** LEXER *****
SCI_SETLEXER(int lexer)
SCI_GETLEXER
SCI_SETLEXERLANGUAGE(<unused>, const char *name)
SCI_GETLEXERLANGUAGE(<unused>, char *name)
SCI_LOADLEXERLIBRARY(<unused>, const char *path)
SCI_COLOURISE(int start, int end)
--SCI_CHANGELEXERSTATE(int start, int end)
SCI_PROPERTYNAMES(<unused>, char *names)
--SCI_PROPERTYTYPE(const char *name)
--SCI_DESCRIBEPROPERTY(const char *name, char *description)
SCI_SETPROPERTY(const char *key, const char *value)
SCI_GETPROPERTY(const char *key, char *value)
--SCI_GETPROPERTYEXPANDED(const char *key, char *value)
--SCI_GETPROPERTYINT(const char *key, int default)
SCI_DESCRIBEKEYWORDSETS(<unused>, char *descriptions)
SCI_SETKEYWORDS(int keyWordSet, const char *keyWordList)
--SCI_GETSTYLEBITSNEEDED
*/


static int iScintillaLoadLexerLibraryAttrib(Ihandle* ih, const char* value)
{
  if (value)
    IupScintillaSendMessage(ih, SCI_LOADLEXERLIBRARY, 0, (sptr_t)value);
  return 0;
}

static char* iScintillaGetLexerLanguageAttrib(Ihandle* ih)
{
  int len = (int)IupScintillaSendMessage(ih, SCI_GETLEXERLANGUAGE, 0, (sptr_t)NULL);
  char *str = iupStrGetMemory(len+1);
  len = (int)IupScintillaSendMessage(ih, SCI_GETLEXERLANGUAGE, 0, (sptr_t)str);
  if (len)
  {
    if (!iupStrEqual(str, "null"))
      return str;
  }
  return NULL;
}

static int iScintillaSetLexerLanguageAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    IupScintillaSendMessage(ih, SCI_SETLEXER, SCLEX_NULL, 0);
  else
    IupScintillaSendMessage(ih, SCI_SETLEXERLANGUAGE, 0, (sptr_t)value);
  return 0;
}

static int iScintillaSetKeywordsAttrib(Ihandle* ih, int keyWordSet, const char* value)
{
  /* Note: You can set up to 9 lists of keywords for use by the current lexer */
  if(keyWordSet >= 0 && keyWordSet < 9)
    IupScintillaSendMessage(ih, SCI_SETKEYWORDS, keyWordSet, (sptr_t)value);

  return 0;
}

static int iScintillaSetColoriseAttrib(Ihandle* ih, const char* value)
{
  int start = 0, end = -1;
  iupStrToIntInt(value, &start, &end, ':');
  IupScintillaSendMessage(ih, SCI_COLOURISE, start, end);
  return 0;
}

static char* iScintillaGetPropertyAttrib(Ihandle* ih)
{
  char* strKey = iupAttribGetStr(ih, "PROPERTYNAME");
  if (strKey)
  {
    int len = (int)IupScintillaSendMessage(ih, SCI_GETPROPERTY, (uptr_t)strKey, (sptr_t)NULL);
    char *str = iupStrGetMemory(len+1);

    len = (int)IupScintillaSendMessage(ih, SCI_GETPROPERTY, (uptr_t)strKey, (sptr_t)str);
    if (len)
      return str;
  }

  return NULL;
}

static int iScintillaSetPropertyAttrib(Ihandle* ih, const char* value)
{
  char strKey[50];
  char strVal[50];

  iupStrToStrStr(value, strKey, strVal, '=');

  IupScintillaSendMessage(ih, SCI_SETPROPERTY, (uptr_t)strKey, (sptr_t)strVal);

  return 0;
}

static char* iScintillaGetDescribeKeywordSetsAttrib(Ihandle* ih)
{
  int len = (int)IupScintillaSendMessage(ih, SCI_DESCRIBEKEYWORDSETS, 0, 0);
  char *str = iupStrGetMemory(len+1);
  IupScintillaSendMessage(ih, SCI_DESCRIBEKEYWORDSETS, 0, (sptr_t)str);
  return str;
}

static char* iScintillaGetPropertyNamessAttrib(Ihandle* ih)
{
  int len = (int)IupScintillaSendMessage(ih, SCI_PROPERTYNAMES, 0, 0);
  char *str = iupStrGetMemory(len+1);
  IupScintillaSendMessage(ih, SCI_PROPERTYNAMES, 0, (sptr_t)str);
  return str;
}

#if LPEG_LEXER

/* This requires that IupScitilla depends on Lua and on LPEG.
   But notice that the application does NOT need to use Lua. 
   It will by default to create a Lua state of its own,
   but an application state can be used.
   When compiling the lib LPEG_LEXER must be defined. 
   To link with LPEG must define also LINK_LPEG. If not linking with LPEG, must previously load lpeg in the Lua state.

"LPEGLEXERLANGUAGE" - the language name. A Lua file of the same name must exist (not including the .lua extension). 
                      It uses the following two attributes.
  "LPEGLEXERHOME" - the folder where the Lpeg Lua Lexers are installed. 
                    If not defined the environment variable IUP_LEXERLUA is consulted, 
                    if also not defined the LEXERLUA global attribute is consulted,
                    if also not defined the path "../etc" is used.
                    It is used only when LPEGLEXERLANGUAGE is set.
  "LPEGLEXERTHEME" - the Lexer theme defined by a Lua file of the same name (not including the .lua extension) 
                     located inside the LPEGLEXERHOME in the sub-folder "themes".
                     It can also be the full path of a theme file.
                     It is used only when LPEGLEXERLANGUAGE is set.
"LPEGLEXERSTATUS" - read-only attribute, returns the lexer error status.
"LPEGLEXERSTATE" - write-only attribute, sets the Lua state. Value is a lua_State*.
                   If IupLua is initialized, value can be set to NULL so the same Lua state used by IupLua will be used by the Lexer.
*/

static int iScintillaSetLpegLexerLanguageAttrib(Ihandle* ih, const char* value)
{
  const char* lexer_lang = "lpeg";
  const char* home = "lexer.lpeg.home";
  char* home_name;
  const char* theme = "lexer.lpeg.color.theme";
  char* theme_name;
  sptr_t fn, psci;

  home_name = iupAttribGet(ih, "LPEGLEXERHOME");  /* folder where Lexer Lua files are located */
  if (!home_name)
  {
    home_name = getenv("IUP_LEXERLUA");
    if (!home_name)
      home_name = IupGetGlobal("LEXERLUA");
    if (!home_name)
      home_name = "../etc";
  }

  theme_name = iupAttribGetStr(ih, "LPEGLEXERTHEME");
  if (!theme_name)
    theme_name = "dark";

  IupScintillaSendMessage(ih, SCI_SETLEXERLANGUAGE, 0, (sptr_t)lexer_lang);
  IupScintillaSendMessage(ih, SCI_SETPROPERTY, (uptr_t)home, (sptr_t)home_name);
  IupScintillaSendMessage(ih, SCI_SETPROPERTY, (uptr_t)theme, (sptr_t)theme_name);
  fn = IupScintillaSendMessage(ih, SCI_GETDIRECTFUNCTION, 0, 0);
  IupScintillaSendMessage(ih, SCI_PRIVATELEXERCALL, SCI_GETDIRECTFUNCTION, fn);
  psci = IupScintillaSendMessage(ih, SCI_GETDIRECTPOINTER, 0, 0);
  IupScintillaSendMessage(ih, SCI_PRIVATELEXERCALL, SCI_SETDOCPOINTER, psci);
  IupScintillaSendMessage(ih, SCI_PRIVATELEXERCALL, SCI_SETLEXERLANGUAGE, (sptr_t)value);

  return 1;
}

static char* iScintillaGetLpegLexerLanguageAttrib(Ihandle* ih)
{
  int len = (int)IupScintillaSendMessage(ih, SCI_PRIVATELEXERCALL, SCI_GETLEXERLANGUAGE, 0);
  char *str = iupStrGetMemory(len + 1);
  IupScintillaSendMessage(ih, SCI_PRIVATELEXERCALL, SCI_GETLEXERLANGUAGE, (sptr_t)str);
  return str;
}

static char* iScintillaGetLpegLexerStatusAttrib(Ihandle* ih)
{
  int len = (int)IupScintillaSendMessage(ih, SCI_PRIVATELEXERCALL, SCI_GETSTATUS, 0);
  char *str = iupStrGetMemory(len + 1);
  IupScintillaSendMessage(ih, SCI_PRIVATELEXERCALL, SCI_GETSTATUS, (sptr_t)str);
  return str;
}

static int iScintillaSetLpegLexerStateAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    value = IupGetAttribute(ih, "_IUPLUA_STATE_CONTEXT");

  IupScintillaSendMessage(ih, SCI_PRIVATELEXERCALL, SCI_CHANGELEXERSTATE, (sptr_t)value);
  return 0;
}

#endif  /* LPEG_LEXER */

void iupScintillaRegisterLexer(Iclass* ic)
{
  iupClassRegisterAttribute(ic,   "LOADLEXERLIBRARY", NULL, iScintillaLoadLexerLibraryAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic,   "LEXERLANGUAGE", iScintillaGetLexerLanguageAttrib, iScintillaSetLexerLanguageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic,   "PROPERTYNAME", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic,   "PROPERTY", iScintillaGetPropertyAttrib, iScintillaSetPropertyAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic,   "COLORISE", NULL, iScintillaSetColoriseAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "KEYWORDS", NULL, iScintillaSetKeywordsAttrib, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic,   "PROPERTYNAMES", iScintillaGetPropertyNamessAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic,   "KEYWORDSETS", iScintillaGetDescribeKeywordSetsAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
#if LPEG_LEXER
  iupClassRegisterAttribute(ic,   "LPEGLEXERLANGUAGE", iScintillaGetLpegLexerLanguageAttrib, iScintillaSetLpegLexerLanguageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic,   "LPEGLEXERHOME", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic,   "LPEGLEXERTHEME", NULL, NULL, IUPAF_SAMEASSYSTEM, "dark", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic,   "LPEGLEXERSTATUS", iScintillaGetLpegLexerStatusAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic,   "LPEGLEXERSTATE", NULL, iScintillaSetLpegLexerStateAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
#endif
}

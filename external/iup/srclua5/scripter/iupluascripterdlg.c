#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef WIN32
#include <unistd.h> /* for chdir */
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "iup.h"
#include "iup_scintilla.h"
#include "iup_config.h"

#include "iuplua.h"
#include "il.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_strmessage.h"
#include "iup_register.h"
#include "iup_childtree.h"
#include "iup_drvinfo.h"


#define BREAKPOINT_MARGIN "15"
#define FOLDING_MARGIN "20"

/* from _images.c */
void load_all_images_step_images(void);

/* from _debugger.c */
void iupLuaScripterDebuggerCreateTabs(Ihandle* panelTabs);
void iupLuaScripterDebuggerAddToolbarButtons(Ihandle *toolbar);
void iupLuaScripterDebuggerAddMenuItems(Ihandle *lua_menu);
void iupLuaScripterDebuggerAddHotKeys(Ihandle *ih);


/********************************** Utilities *****************************************/

static char* getLuaKeywords(void)
{
#ifdef OLD_LUA_LEXER
  return "and break do else elseif end for function if in local load nil not or repeat return then until while "
    "false true assert collectgarbage dofile error _G getmetatable ipairs loadfile next pairs "
    "pcall print rawequal rawget rawset setmetatable tonumber tostring type _VERSION select "
    "require xpcall "
    "string string.byte string.char string.dump string.find string.format string.gsub string.len string.lower "
    "string.rep string.sub string.upper string.gmatch string.match string.reverse "
    "table table.concat table.insert table.remove table.sort "
    "math math.abs math.acos math.asin math.atan math.ceil math.cos math.deg math.exp math.floor "
    "math.log math.max math.min math.pi math.rad math.random math.randomseed math.fmod "
    "math.sin math.sqrt math.tan math.huge math.modf "
    "coroutine coroutine.create coroutine.resume coroutine.status coroutine.wrap coroutine.yield coroutine.running "
    "io io.close io.flush io.input io.lines io.open io.output io.read io.tmpfile io.type io.write "
    "io.stdin io.stdout io.stderr io.popen "
    "os os.clock os.date os.difftime os.execute os.exit "
    "os.getenv os.remove os.rename os.setlocale os.time os.tmpname "
    "debug debug.getfenv debug.getmetatable debug.getregistry debug.setfenv debug.setmetatable debug.debug debug.gethook "
    "debug.setlocal debug.setupvalue debug.sethook debug.traceback debug.getinfo debug.getlocal debug.getupvalue "
    "package package.seeall package.cpath package.loaded "
    "package.loadlib package.path package.preload "
#if LUA_VERSION_NUM == 501  /* Lua 5.1 Only */
    "module setfenv getfenv math.log10 loadstring table.maxn unpack package.loaders "
    "math.atan2 math.cosh math.sinh math.tanh math.pow math.frexp math.ldexp "
#elif LUA_VERSION_NUM == 502  /* Lua 5.2 Only */
    "package.searchers goto rawlen table.pack table.unpack package.config debug.getuservalue debug.setuservalue debug.upvalueid debug.upvaluejoin _ENV "
    "bit32 bit32.arshift bit32.band bit32.bnot bit32.bor bit32.btest bit32.bxor bit32.extract bit32.replace bit32.lrotate bit32.lshift bit32.rrotate bit32.rshift "
    "package.searchpath "
    "math.atan2 math.cosh math.sinh math.tanh math.pow math.frexp math.ldexp "
#elif LUA_VERSION_NUM == 503  /* Lua 5.3 Only */
    "package.searchers goto rawlen table.pack table.unpack package.config debug.getuservalue debug.setuservalue debug.upvalueid debug.upvaluejoin _ENV "
    "table.move string.pack string.unpack string.packsize "
    "utf8 utf8.char utf8.charpattern utf8.codes utf8.codepoint utf8.len utf8.offset "
#endif  
#else
  return "and break do else elseif end for function if in local load nil not or repeat return then until while "
    "false true assert collectgarbage dofile error _G getmetatable ipairs loadfile next pairs "
    "pcall print rawequal rawget rawset setmetatable tonumber tostring type _VERSION select "
    "require xpcall "
    "string byte char dump find format gsub len lower "
    "rep sub upper gmatch match reverse "
    "table concat insert remove sort "
    "math abs acos asin atan ceil cos deg exp floor "
    "log max min pi rad random randomseed fmod "
    "sin sqrt tan huge modf "
    "coroutine create resume status wrap yield running "
    "io close flush input lines open output read tmpfile type write "
    "stdin stdout stderr popen "
    "os clock date difftime execute exit "
    "getenv remove rename setlocale time tmpname "
    "debug getfenv getmetatable getregistry setfenv setmetatable debug gethook "
    "setlocal setupvalue sethook traceback getinfo getlocal getupvalue "
    "package seeall cpath loaded "
    "loadlib path preload "
#if LUA_VERSION_NUM == 501  /* Lua 5.1 Only */
    "module setfenv getfenv log10 loadstring maxn unpack loaders "
    "atan2 cosh sinh tanh pow frexp ldexp "
#elif LUA_VERSION_NUM == 502  /* Lua 5.2 Only */
    "searchers goto rawlen pack unpack config getuservalue setuservalue upvalueid upvaluejoin _ENV "
    "bit32 arshift band bnot bor btest bxor extract replace lrotate lshift rrotate rshift "
    "searchpath "
    "atan2 cosh sinh tanh pow frexp ldexp "
#elif LUA_VERSION_NUM == 503  /* Lua 5.3 Only */
    "searchers goto rawlen pack unpack config getuservalue setuservalue upvalueid upvaluejoin _ENV "
    "move pack unpack packsize "
    "utf8 char charpattern codes codepoint len offset "
#endif  
#endif
    ;
}

static const char *getLastNonAlphaNumeric(const char *text)
{
  int len = (int)strlen(text);
  const char *c = text + len - 1;
  if (*c == '\n')
    c--;
  for (; c != text; c--)
  {
    if (*c < 48 || (*c > 57 && *c < 65) || (*c > 90 && *c < 97) || *c > 122)
      return c + 1;
  }
  return NULL;
}

static char *filterList(const char *text, const char *list)
{
  char *filteredList[1024];
  char *retList;
  int count = 0;

  int i, len;
  const char *lastValue = list;
  const char *nextValue = iupStrNextValue(list, (int)strlen(list), &len, ' ');
  while (len != 0)
  {
    if ((int)strlen(text) <= len && iupStrEqualPartial(lastValue, text))
    {
      char *value = malloc(80);

      strncpy(value, lastValue, len);
      value[len] = 0;
      filteredList[count++] = value;
    }
    lastValue = nextValue;
    nextValue = iupStrNextValue(nextValue, (int)strlen(nextValue), &len, ' ');
  }

  retList = malloc(1024);
  retList[0] = 0;
  for (i = 0; i < count; i++)
  {
    if (i == 0)
    {
      strcpy(retList, filteredList[i]);
      strcat(retList, " ");
    }
    else
    {
      strcat(retList, filteredList[i]);
      strcat(retList, " ");
    }
  }

  for (i = 0; i < count; i++)
    free(filteredList[i]);

  return retList;
}

static Ihandle* get_config(Ihandle* ih)
{
  Ihandle* config = (Ihandle*)IupGetAttribute(IupGetDialog(ih), "CONFIG_HANDLE");
  return config;
}

static void remove_breakpoint(Ihandle *listBreak, const char* m_filename, int m_line, lua_State* L)
{
  int i, line;
  char* filename;

  i = 1;
  filename = IupGetAttributeId(listBreak, "FILENAME", i);
  while (filename != NULL)
  {
    line = IupGetIntId(listBreak, "LINE", i);
    if (line == m_line && iupStrEqual(filename, m_filename))
    {
      iuplua_push_name(L, "DebuggerRemoveBreakpoint");
      iuplua_pushihandle(L, listBreak);
      lua_pushinteger(L, i);
      iuplua_call_raw(L, 2, 0);
      return;
    }

    i++;
    filename = IupGetAttributeId(listBreak, "FILENAME", i);
  }
}

static void add_breakpoint(Ihandle *listBreak, const char* filename, int line, lua_State* L)
{
  iuplua_push_name(L, "DebuggerAddBreakpoint");
  iuplua_pushihandle(L, listBreak);
  lua_pushstring(L, filename);
  lua_pushinteger(L, line);
  iuplua_call_raw(L, 3, 0);
}

static void save_globals(Ihandle *ih, Ihandle* config)
{
  Ihandle* treeGlobals = IupGetDialogChild(ih, "DEBUG_TREE_GLOBAL");
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  int i, count, id;
  const char *value;

  count = IupGetInt(treeGlobals, "ROOTCOUNT");

  id = 0;
  for (i = 0; i < count; i++)
  {
    iuplua_push_name(L, "DebuggerGetGlobalNameFromTree");
    iuplua_pushihandle(L, treeGlobals);
    lua_pushinteger(L, id);
    iuplua_call_raw(L, 2, 1);

    value = lua_tostring(L, -1);

    IupConfigSetVariableStrId(config, "LuaScripterGlobals", "Name", i+1, value);
    id = IupGetIntId(treeGlobals, "NEXT", id);
  }

  IupConfigSetVariableInt(config, "LuaScripterGlobals", "Count", count);

  /* make sure some older globals are not saved in the configuration file (at least 10) */
  for (i = count; i <= count + 10; i++)
    IupConfigSetVariableStrId(config, "LuaScripterGlobals", "Name", i+1, NULL);
}

static void save_breakpoints(Ihandle *ih, Ihandle* config)
{
  Ihandle* listBreak = IupGetDialogChild(ih, "DEBUG_LIST_BREAK");
  int i, line, count;
  char *filename;
  char data[10240];

  i = 1;
  filename = IupGetAttributeId(listBreak, "FILENAME", i);
  while (filename != NULL)
  {
    line = IupGetIntId(listBreak, "LINE", i);
    sprintf(data, "%s#%d", filename, line);
    IupConfigSetVariableStrId(config, "LuaScripterBreakpoints", "FileLine", i, data);

    i++;
    filename = IupGetAttributeId(listBreak, "FILENAME", i);
  }

  count = i - 1;
  IupConfigSetVariableInt(config, "LuaScripterBreakpoints", "Count", count);

  /* make sure some older breakpoints are not saved in the configuration file (at least 10) */
  for (i = count+1; i <= count+1 + 10; i++)
    IupConfigSetVariableStrId(config, "LuaScripterBreakpoints", "FileLine", i, NULL);
}

static void load_globals(Ihandle *ih, Ihandle* config)
{
  Ihandle* treeGlobals = IupGetDialogChild(ih, "DEBUG_TREE_GLOBAL");
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  int i;
  const char* value;

  /* this will work only if the tree is already mapped */

  IupSetAttribute(treeGlobals, "DELNODE", "ALL");

  i = 1;
  do
  {
    value = IupConfigGetVariableStrId(config, "LuaScripterGlobals", "Name", i);
    if (value)
    {
      iuplua_push_name(L, "DebuggerAddGlobal");
      iuplua_pushihandle(L, treeGlobals);
      lua_pushstring(L, value);
      iuplua_call_raw(L, 2, 0);
    }
    i++;
  } while (value != NULL);

  iuplua_push_name(L, "DebuggerInitGlobalsTree");
  iuplua_pushihandle(L, treeGlobals);
  iuplua_call_raw(L, 1, 0);
}

static void load_breakpoints(Ihandle *ih, Ihandle* config)
{
  Ihandle* listBreak = IupGetDialogChild(ih, "DEBUG_LIST_BREAK");
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  int i, line;
  const char* value;
  char filename[10240], line_str[30];

  i = 1;
  do
  {
    value = IupConfigGetVariableStrId(config, "LuaScripterBreakpoints", "FileLine", i);
    if (value)
    {
      iupStrToStrStr(value, filename, line_str, '#');
      iupStrToInt(line_str, &line);
      IupSetStrAttributeId(listBreak, "FILENAME", i, filename);
      IupSetIntId(listBreak, "LINE", i, line);
      i++;
    }
  } while (value != NULL);

  IupSetStrAttributeId(listBreak, "FILENAME", i, NULL);
  IupSetStrAttributeId(listBreak, "LINE", i, NULL);

  iuplua_push_name(L, "DebuggerInitBreakpointsList");
  iuplua_pushihandle(L, listBreak);
  iuplua_call_raw(L, 1, 0);
}

static char* changeTabsForSpaces(const char *text, int tabsize)
{
  static char newText[1024];
  int len = (int)strlen(text);
  int i, j, charcount = 0;

  for (i = 0; i < len; i++)
  {
    char c = text[i];

    if (c == '\t')
    {
      int nWhites = tabsize - (charcount % tabsize);

      for (j = 0; j < nWhites; j++)
      {
        newText[charcount] = ' ';
        charcount++;
      }
      continue;
    }

    newText[charcount] = c;
    charcount++;
  }

  newText[charcount] = '\0';

  return newText;
}

static Ihandle* get_current_multitext(Ihandle* ih)
{
  Ihandle* tabs = IupGetDialogChild(ih, "MULTITEXT_TABS");
  return (Ihandle*)IupGetAttribute(tabs, "VALUE_HANDLE");
}


/********************************** Callbacks *****************************************/


static int exit_cb(Ihandle *ih)
{
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");

  char* exit_loop = IupGetGlobal("EXITLOOP");
  if (exit_loop && !iupStrBoolean(exit_loop))
  {
    /* assume we are inside the iupluascripter application */
    IupSetGlobal("EXITLOOP", NULL);
    IupExitLoop();
  }

  iuplua_push_name(L, "DebuggerExit");
  iuplua_call_raw(L, 0, 0); /* this may trigger a Lua error which will abort the function with a goto, must sure you do nothing after that */
  return IUP_DEFAULT;
}

static int restoremarkers_cb(Ihandle *ih, Ihandle *multitext)
{
  int i, line;
  char *filename;

  /* called when a new multitext is created/loaded */

  char* m_filename = IupGetAttribute(multitext, "FILENAME");
  Ihandle* listBreak = IupGetDialogChild(ih, "DEBUG_LIST_BREAK");

  IupSetInt(multitext, "MARKERDELETEALL", 1);

  if (!m_filename)
    return IUP_DEFAULT;

  i = 1;
  filename = IupGetAttributeId(listBreak, "FILENAME", i);
  while (filename != NULL)
  {
    if (iupStrEqual(filename, m_filename))
    {
      line = IupGetIntId(listBreak, "LINE", i);
      IupSetIntId(multitext, "MARKERADD", line - 1, 1);
    }

    i++;
    filename = IupGetAttributeId(listBreak, "FILENAME", i);
  }

  return IUP_DEFAULT;
}

static int marker_changed_cb(Ihandle *ih, Ihandle *multitext, int lin, int margin)
{
  if (margin == 2)
  {
    lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
    unsigned int markerMask = (unsigned int)IupGetIntId(multitext, "MARKERGET", lin);
    int has_breakpoint = markerMask & 0x0002; /* 0010 - marker=1 */
    Ihandle* listBreak = IupGetDialogChild(ih, "DEBUG_LIST_BREAK");

    char* filename = IupGetAttribute(multitext, "FILENAME");
    if (!filename)
    {
      IupMessageError(IupGetDialog(ih), "Must have a filename to add a breakpoint.");
      return IUP_DEFAULT;
    }

    if (has_breakpoint)
      remove_breakpoint(listBreak, filename, lin + 1, L);
    else
      add_breakpoint(listBreak, filename, lin + 1, L);
  }
  else if (margin == 3)
    IupSetfAttribute(multitext, "FOLDTOGGLE", "%d", lin);

  (void)ih;
  return IUP_DEFAULT;
}

static int configsave_cb(Ihandle *ih, Ihandle* config)
{
  save_breakpoints(ih, config);

  save_globals(ih, config);

  return IUP_DEFAULT;
}

static int configload_cb(Ihandle *ih, Ihandle* config)
{
  const char* value;

  value = IupConfigGetVariableStr(config, "LuaScripter", "CurrentDirectory");
  if (value)
    IupSetStrAttribute(ih, "CURRENTDIRECTORY", value);

  value = IupConfigGetVariableStr(config, "LuaScripter", "Arguments");
  if (value)
    IupSetStrAttribute(ih, "ARGUMENTS", value);

  value = IupConfigGetVariableStr(config, "LuaScripter", "AutoCompletion");
  if (value)
  {
    Ihandle* ih_item = IupGetDialogChild(ih, "ITM_AUTOCOMPLETE");
    IupSetStrAttribute(ih_item, "VALUE", value);
  }

  value = IupConfigGetVariableStr(config, "LuaScripter", "Folding");
  if (value)
  {
    Ihandle* ih_item = IupGetDialogChild(ih, "ITM_FOLDING");
    IupSetStrAttribute(ih_item, "VALUE", value);
  }

  load_breakpoints(ih, config);

  if (ih->handle)
    load_globals(ih, config);

  return IUP_DEFAULT;
}

static int newfilename_cb(Ihandle *ih, char* old_filename, char* new_filename)
{
  Ihandle* listBreak = IupGetDialogChild(ih, "DEBUG_LIST_BREAK");
  int i, changed = 0;
  char* filename;

  i = 1;
  filename = IupGetAttributeId(listBreak, "FILENAME", i);
  while (filename != NULL)
  {
    if (iupStrEqual(filename, old_filename))
    {
      IupSetStrAttributeId(listBreak, "FILENAME", i, new_filename);
      changed = 1;
    }

    i++;
    filename = IupGetAttributeId(listBreak, "FILENAME", i);
  }

  if (changed)
  {
    lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
    iuplua_push_name(L, "DebuggerInitBreakpointsList");
    iuplua_pushihandle(L, listBreak);
    iuplua_call_raw(L, 1, 0);
  }
  return IUP_DEFAULT;
}

static int multitext_dwell_cb(Ihandle* multitext, int code, int pos, int x, int y)
{
  if (!IupGetInt(IupGetDialogChild(multitext, "DEBUG_ITM_DEBUG"), "ACTIVE")) /* can be called by the hot key in the dialog */
    return IUP_DEFAULT;

  if (code)
  {
    int start, end, lin, start_col, end_col;
    const char *value;
    char *text;
    char word[1024];
    int tabSize = IupGetInt(multitext, "TABSIZE");
    lua_State* L = (lua_State*)IupGetAttribute(multitext, "LUASTATE");
    char* wordpos = IupGetAttributeId(multitext, "WORDPOS", pos);
    if (wordpos == NULL)
      return IUP_DEFAULT;
    
    sscanf(wordpos, "%d:%d", &start, &end);
    IupTextConvertPosToLinCol(multitext, start, &lin, &start_col);
    IupTextConvertPosToLinCol(multitext, end, &lin, &end_col);
    text = IupGetAttributeId(multitext, "LINE", lin);
    text = changeTabsForSpaces(text, tabSize);
    text[end_col] = '\0';
    strcpy(word, text+start_col);
    
    iuplua_push_name(L, "DebuggerShowTip");
    lua_pushstring(L, word);
    lua_pushinteger(L, lin+1);
    iuplua_call_raw(L, 2, 1);
    
    value = lua_tostring(L, -1);
    if (value==NULL)
      return IUP_DEFAULT;

    IupSetStrf(multitext, "TIP", "%s = %s" , word, value);
    IupSetAttribute(multitext, "TIPVISIBLE", "Yes");
  }
  else
  {
    IupSetAttribute(multitext, "TIPVISIBLE", "No");
    IupSetAttribute(multitext, "TIP", NULL);
  }

  (void)x;
  (void)y;
  return IUP_DEFAULT;
}

static int multitext_kesc_cb(Ihandle *multitext)
{
  if (!IupGetInt(multitext, "AUTOCOMPLETION"))
    return IUP_CONTINUE;

  if (IupGetInt(multitext, "AUTOCACTIVE"))
    IupSetAttribute(multitext, "AUTOCCANCEL", "YES");

  return IUP_CONTINUE;
}

static int multitext_lineschanged_cb(Ihandle* multitext, int start, int len)
{
  lua_State* L = (lua_State*)IupGetAttribute(multitext, "LUASTATE");
  iuplua_push_name(L, "DebuggerBreakpointsChanged");
  iuplua_pushihandle(L, multitext);
  lua_pushinteger(L, start + 1);
  lua_pushinteger(L, len);
  iuplua_call_raw(L, 3, 0);
  return IUP_DEFAULT;
}

static int multitext_valuechanged_cb(Ihandle* multitext)
{
  if (IupGetInt(multitext, "AUTOCOMPLETION"))
  {
    const char *t;
    char *text = IupGetAttribute(multitext, "LINEVALUE");
    int col = IupGetInt2(multitext, "CARET");
    text[col + 1] = '\0';
    t = getLastNonAlphaNumeric(text);
    if (t != NULL && *t != '\n' && *t != 0)
    {
      char *fList = filterList(t, getLuaKeywords());
      if (strlen(fList) > 0)
        IupSetAttributeId(multitext, "AUTOCSHOW", (int)strlen(t) - 1, fList);
      free(fList);
    }

    return IUP_DEFAULT;
  }

  return IUP_CONTINUE;
}

static int item_autocomplete_action_cb(Ihandle* ih_item)
{
  Ihandle* tabs = IupGetDialogChild(ih_item, "MULTITEXT_TABS");
  Ihandle* multitext;
  Ihandle* config = get_config(ih_item);

  if (IupGetInt(ih_item, "VALUE"))
  {
    IupSetAttribute(ih_item, "VALUE", "OFF");

    for (multitext = tabs->firstchild; multitext; multitext = multitext->brother)
      IupSetAttribute(multitext, "AUTOCOMPLETION", "OFF");
  }
  else
  {
    IupSetAttribute(ih_item, "VALUE", "ON");

    for (multitext = tabs->firstchild; multitext; multitext = multitext->brother)
      IupSetAttribute(multitext, "AUTOCOMPLETION", "ON");
  }

  IupConfigSetVariableStr(config, "LuaScripter", "AutoCompletion", IupGetAttribute(ih_item, "VALUE"));

  return IUP_DEFAULT;
}

static int setparent_param_cb(Ihandle* param_dialog, int param_index, void* user_data)
{
  if (param_index == IUP_GETPARAM_MAP)
  {
    Ihandle* ih = (Ihandle*)user_data;
    IupSetAttributeHandle(param_dialog, "PARENTDIALOG", ih);
  }

  return 1;
}

static int item_style_config_action_cb(Ihandle* ih_item)
{
  Ihandle* tabs = IupGetDialogChild(ih_item, "MULTITEXT_TABS");
  Ihandle* multitext;
  Ihandle* config = get_config(ih_item);
  char commentColor[30], commentLineColor[30], numberColor[30], keywordColor[30],
    stringColor[30], characterColor[30], operatorColor[30];

  strcpy(commentColor, IupConfigGetVariableStr(config, "LuaScripter", "CommentColor"));
  strcpy(commentLineColor, IupConfigGetVariableStr(config, "LuaScripter", "CommentLineColor"));
  strcpy(numberColor, IupConfigGetVariableStr(config, "LuaScripter", "NumberColor"));
  strcpy(keywordColor, IupConfigGetVariableStr(config, "LuaScripter", "KeywordColor"));
  strcpy(stringColor, IupConfigGetVariableStr(config, "LuaScripter", "StringColor"));
  strcpy(characterColor, IupConfigGetVariableStr(config, "LuaScripter", "CharacterColor"));
  strcpy(operatorColor, IupConfigGetVariableStr(config, "LuaScripter", "OperatorColor"));

  if (!IupGetParam("Syntax Colors", setparent_param_cb, IupGetDialog(ih_item),
                   "Comment: %c\n"
                   "Comment Line: %c\n"
                   "Number: %c\n"
                   "Keyword: %c\n"
                   "String: %c\n"
                   "Character: %c\n"
                   "Operator: %c\n",
                   commentColor, commentLineColor, numberColor, keywordColor, stringColor, characterColor, operatorColor, NULL))
    return IUP_DEFAULT;

  IupConfigSetVariableStr(config, "LuaScripter", "CommentColor", commentColor);
  IupConfigSetVariableStr(config, "LuaScripter", "CommentLineColor", commentLineColor);
  IupConfigSetVariableStr(config, "LuaScripter", "NumberColor", numberColor);
  IupConfigSetVariableStr(config, "LuaScripter", "KeywordColor", keywordColor);
  IupConfigSetVariableStr(config, "LuaScripter", "StringColor", stringColor);
  IupConfigSetVariableStr(config, "LuaScripter", "CharacterColor", characterColor);
  IupConfigSetVariableStr(config, "LuaScripter", "OperatorColor", operatorColor);

  for (multitext = tabs->firstchild; multitext; multitext = multitext->brother)
  {
    IupSetStrAttribute(multitext, "STYLEFGCOLOR1", commentColor);
    IupSetStrAttribute(multitext, "STYLEFGCOLOR2", commentLineColor);
    IupSetStrAttribute(multitext, "STYLEFGCOLOR4", numberColor);
    IupSetStrAttribute(multitext, "STYLEFGCOLOR5", keywordColor);
    IupSetStrAttribute(multitext, "STYLEFGCOLOR6", stringColor);
    IupSetStrAttribute(multitext, "STYLEFGCOLOR7", characterColor);
    IupSetStrAttribute(multitext, "STYLEFGCOLOR10", operatorColor);
  }

  return IUP_DEFAULT;
}

static int item_folding_action_cb(Ihandle* ih)
{
  Ihandle* tabs = IupGetDialogChild(ih, "MULTITEXT_TABS");
  Ihandle* multitext;
  Ihandle* config = get_config(ih);

  if (IupGetInt(ih, "VALUE"))
  {
    IupSetAttribute(ih, "VALUE", "OFF");

    for (multitext = tabs->firstchild; multitext; multitext = multitext->brother)
    {
      IupSetAttribute(multitext, "PROPERTY", "fold=0");
      IupSetAttribute(multitext, "MARGINWIDTH3", "0");
      IupSetAttribute(multitext, "_IUP_FOLDDING", NULL);
    }
  }
  else
  {
    IupSetAttribute(ih, "VALUE", "ON");

    for (multitext = tabs->firstchild; multitext; multitext = multitext->brother)
    {
      IupSetAttribute(multitext, "PROPERTY", "fold=1");
      IupSetAttribute(multitext, "MARGINWIDTH3", FOLDING_MARGIN);
      IupSetAttribute(multitext, "_IUP_FOLDDING", "1");
      IupSetAttribute(multitext, "FOLDALL", "EXPAND");
    }
  }

  IupConfigSetVariableStr(config, "LuaScripter", "Folding", IupGetAttribute(ih, "VALUE"));

  return IUP_DEFAULT;
}

static int item_toggle_folding_action_cb(Ihandle* ih)
{
  Ihandle* multitext = get_current_multitext(ih);
  int pos = IupGetInt(multitext, "CARETPOS");
  int lin, col;

  /* must test again because it can be called by the hot key */
  if (!IupGetInt(multitext, "_IUP_FOLDDING"))
    return IUP_DEFAULT;

  IupTextConvertPosToLinCol(multitext, pos, &lin, &col);

  if (!IupGetIntId(multitext, "FOLDLEVELHEADER", lin))
  {
    lin = IupGetIntId(multitext, "FOLDPARENT", lin);
    if (lin < 0)
      return IUP_DEFAULT;
  }

  IupSetfAttribute(multitext, "FOLDTOGGLE", "%d", lin);
  IupSetfAttribute(multitext, "CARET", "%d:0", lin);

  return IUP_DEFAULT;
}

static int item_fold_collapse_action_cb(Ihandle* ih)
{
  Ihandle* multitext = get_current_multitext(ih);

  IupSetAttribute(multitext, "FOLDALL", "CONTRACT");

  return IUP_DEFAULT;
}

static int item_fold_expand_action_cb(Ihandle* ih)
{
  Ihandle* multitext = get_current_multitext(ih);

  IupSetAttribute(multitext, "FOLDALL", "EXPAND");

  return IUP_DEFAULT;
}

static int item_fold_toggle_action_cb(Ihandle* ih)
{
  Ihandle* multitext = get_current_multitext(ih);

  IupSetAttribute(multitext, "FOLDALL", "TOGGLE");

  return IUP_DEFAULT;
}

static int item_fold_level_action_cb(Ihandle* ih_item)
{
  int level=0, action=0;
  Ihandle* config = get_config(ih_item);

  const char* value = IupConfigGetVariableStr(config, "LuaScripter", "FoldAllLevel");
  if (value) iupStrToInt(value, &level);

  value = IupConfigGetVariableStr(config, "LuaScripter", "FoldAllLevelAction");
  if (value) iupStrToInt(value, &action);

  if (IupGetParam("Fold All by Level", setparent_param_cb, IupGetDialog(ih_item),
                  "Level: %i\n"
                  "Options: %o|Collapse|Expand|Toggle|\n",
                  &level, &action, NULL))
  {
    Ihandle* multitext = get_current_multitext(ih_item);
    int lin, count = IupGetInt(multitext, "LINECOUNT");

    IupConfigSetVariableInt(config, "LuaScripter", "FoldAllLevel", level);
    IupConfigSetVariableInt(config, "LuaScripter", "FoldAllLevelAction", action);

    for (lin = 0; lin < count; lin++)
    {
      if (IupGetIntId(multitext, "FOLDLEVELHEADER", lin))
      {
        int foldLevel = IupGetIntId(multitext, "FOLDLEVEL", lin);
        if (foldLevel + 1 == level) /* level at header is different from child */
        {
          switch (action)
          {
          case 0: /* Collapse */
            if (IupGetIntId(multitext, "FOLDEXPANDED", lin))
              IupSetfAttribute(multitext, "FOLDTOGGLE", "%d", lin);
            break;
          case 1: /* Expand */
            if (!IupGetIntId(multitext, "FOLDEXPANDED", lin))
              IupSetfAttribute(multitext, "FOLDTOGGLE", "%d", lin);
            break;
          case 2: /* Toggle */
            IupSetfAttribute(multitext, "FOLDTOGGLE", "%d", lin);
            break;
          }
        }
      }
    } 
  }
    
  return IUP_DEFAULT;
}

static int item_blockcomment_action_cb(Ihandle* ih_item)
{
  Ihandle* multitext = get_current_multitext(ih_item);
  char *selpos = IupGetAttribute(multitext, "SELECTIONPOS");
  int pos1 = 0, pos2 = 0;

  if (!selpos)
    return IUP_DEFAULT;

  sscanf(selpos, "%d:%d", &pos1, &pos2);

  IupSetAttribute(multitext, "UNDOACTION", "BEGIN");

  IupSetAttributeId(multitext, "INSERT", pos1, "--[[");
  IupSetAttributeId(multitext, "INSERT", pos2+4, "]]");

  IupSetAttribute(multitext, "UNDOACTION", "END");

  IupSetStrf(multitext, "SELECTIONPOS", "%d:%d", pos1, pos2 + 4 + 2);

  return IUP_DEFAULT;
}

static int item_blockuncomment_action_cb(Ihandle* ih_item)
{
  Ihandle* multitext = get_current_multitext(ih_item);
  char *sel = IupGetAttribute(multitext, "SELECTION");
  char *text_line;
  int lin1, lin2, col1, col2, pos1, pos2;

  if (!sel)
    return IUP_DEFAULT;

  sscanf(sel, "%d,%d:%d,%d", &lin1, &col1, &lin2, &col2);

  text_line = IupGetAttributeId(multitext, "LINE", lin1);
  if (text_line[col1] != '-' || text_line[col1 + 1] != '-' || text_line[col1 + 2] != '[' || text_line[col1 + 3] != '[')
    return IUP_DEFAULT;

  text_line = IupGetAttributeId(multitext, "LINE", lin2);
  if (text_line[col2 - 1] != ']' || text_line[col2 - 2] != ']')
    return IUP_DEFAULT;

  IupTextConvertLinColToPos(multitext, lin1, col1, &pos1);
  IupTextConvertLinColToPos(multitext, lin2, col2, &pos2);

  IupSetAttribute(multitext, "UNDOACTION", "BEGIN");

  IupSetStrf(multitext, "DELETERANGE", "%d,%d", pos1, 4);
  IupSetStrf(multitext, "DELETERANGE", "%d,%d", pos2 - 4 - 2, 2);

  IupSetAttribute(multitext, "UNDOACTION", "END");

  IupSetStrf(multitext, "SELECTIONPOS", "%d:%d", pos1, pos2 - 4 - 2);

  return IUP_DEFAULT;
}

static int item_linescomment_action_cb(Ihandle* ih_item)
{
  Ihandle* multitext = get_current_multitext(ih_item);
  char *sel = IupGetAttribute(multitext, "SELECTION");
  char *text_line;
  int lin, col, lin1, lin2, col1, col2;

  if (!sel)
    return IUP_DEFAULT;

  sscanf(sel, "%d,%d:%d,%d", &lin1, &col1, &lin2, &col2);

  IupSetAttribute(multitext, "UNDOACTION", "BEGIN");

  for (lin = lin1; lin <= lin2; lin++)
  {
    int len, pos;
    text_line = IupGetAttributeId(multitext, "LINE", lin);
    len = (int)strlen(text_line);

    for (col = 0; col < len; col++)
    {
      char c = text_line[col];
      if (c != ' ' && c != '\t')
        break;
    }

    IupTextConvertLinColToPos(multitext, lin, col, &pos);
    IupSetAttributeId(multitext, "INSERT", pos, "-- ");
  }

  IupSetAttribute(multitext, "UNDOACTION", "END");

  IupSetStrf(multitext, "SELECTION", "%d,0:%d,999", lin1, lin2);

  return IUP_DEFAULT;
}

static int item_linesuncomment_action_cb(Ihandle* ih_item)
{
  Ihandle* multitext = get_current_multitext(ih_item);
  char *sel = IupGetAttribute(multitext, "SELECTION");
  char *text_line;
  int lin, col, lin1, lin2, col1, col2;

  if (!sel)
    return IUP_DEFAULT;

  sscanf(sel, "%d,%d:%d,%d", &lin1, &col1, &lin2, &col2);

  IupSetAttribute(multitext, "UNDOACTION", "BEGIN");

  for (lin = lin1; lin <= lin2; lin++)
  {
    int len, pos, nChar;
    text_line = IupGetAttributeId(multitext, "LINE", lin);
    len = (int)strlen(text_line);
    nChar = 0;

    for (col = 0; col < len; col++)
    {
      if (text_line[col] == '-' && text_line[col+1] == '-')
      {
        nChar = 2;
        if (text_line[col+2] == ' ')
          nChar++;
        break;
      }
    }

    if (nChar == 0)
      continue;

    IupTextConvertLinColToPos(multitext, lin, col, &pos);
    IupSetStrf(multitext, "DELETERANGE", "%d,%d", pos, nChar);
  }

  IupSetAttribute(multitext, "UNDOACTION", "END");

  IupSetStrf(multitext, "SELECTION", "%d,0:%d,999", lin1, lin2);

  return IUP_DEFAULT;
}

static int item_options_action_cb(Ihandle* ih_item)
{
  Ihandle* ih = IupGetDialog(ih_item);
  char dir[10240] = "";
  char args[10240] = "";
  char* value = IupGetAttribute(ih, "CURRENTDIRECTORY");
  if (value)
    strcpy(dir, value);
  value = IupGetAttribute(ih, "ARGUMENTS");
  if (value)
    strcpy(args, value);

  if (IupGetParam("Options", setparent_param_cb, ih,
                  "Current Directory: %f[dir||||]\n"
                  "Arguments: %s{Sets the table \"arg\".}\n",
                  dir, args, NULL))
  {
    Ihandle* config = get_config(ih);

    IupSetStrAttribute(ih, "CURRENTDIRECTORY", dir);
    IupSetStrAttribute(ih, "ARGUMENTS", args);

    IupConfigSetVariableStr(config, "LuaScripter", "CurrentDirectory", dir);
    IupConfigSetVariableStr(config, "LuaScripter", "Arguments", args);
  }
  return IUP_DEFAULT;
}

static int lua_menu_open_cb(Ihandle *ih_menu)
{
  Ihandle* menu_foldall = IupGetDialogChild(ih_menu, "ITM_FOLD_ALL");
  Ihandle* item_toggle_folding = IupGetDialogChild(ih_menu, "ITM_TOGGLE_FOLDING");
  Ihandle* item_folding = IupGetDialogChild(ih_menu, "ITM_FOLDING");
  Ihandle* item_comments = IupGetDialogChild(ih_menu, "ITM_COMMENTS");
  Ihandle* multitext = get_current_multitext(ih_menu);
  char *selpos = IupGetAttribute(multitext, "SELECTIONPOS");

  if (IupGetInt(item_folding, "VALUE"))
  {
    IupSetAttribute(item_toggle_folding, "ACTIVE", "Yes");
    IupSetAttribute(menu_foldall, "ACTIVE", "Yes");
  }
  else
  {
    IupSetAttribute(item_toggle_folding, "ACTIVE", "NO");
    IupSetAttribute(menu_foldall, "ACTIVE", "NO");
  }

  if (selpos)
    IupSetAttribute(item_comments, "ACTIVE", "Yes");
  else
    IupSetAttribute(item_comments, "ACTIVE", "NO");

  return IUP_DEFAULT;
}

static int multitext_map_cb(Ihandle* multitext)
{
  Ihandle* config = get_config(multitext);
  const char *value;

  IupSetAttribute(multitext, "LEXERLANGUAGE", "lua");
  IupSetAttribute(multitext, "KEYWORDS0", getLuaKeywords());

  IupSetAttribute(multitext, "STYLEFGCOLOR1", "0 128 0");    /* 1-Lua comment */
  IupSetAttribute(multitext, "STYLEFGCOLOR2", "0 128 0");    /* 2-Lua comment line  */
  IupSetAttribute(multitext, "STYLEFGCOLOR4", "255 128 0");  /* 4-Number  */
  IupSetAttribute(multitext, "STYLEFGCOLOR5", "0 0 255");    /* 5-Keyword  */
  IupSetAttribute(multitext, "STYLEFGCOLOR6", "164 0 164");  /* 6-String  */
  IupSetAttribute(multitext, "STYLEFGCOLOR7", "164 0 164");  /* 7-Character  */
  IupSetAttribute(multitext, "STYLEFGCOLOR10", "164 0 0");   /* 10-Operator  */
  /* 3, 8 and 9 - are not used */
  IupSetAttribute(multitext, "STYLEBOLD10", "YES");

  IupSetAttribute(multitext, "MARKERHIGHLIGHT", "YES");

  IupSetAttributeId(multitext, "MARKERBGCOLOR", 25, "0 0 0");         /* BGCOLOR para SC_MARKNUM_FOLDEREND */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 25, "255 255 255");   /* FGCOLOR para SC_MARKNUM_FOLDEREND */
  IupSetAttributeId(multitext, "MARKERBGCOLOR", 26, "0 0 0");         /* BGCOLOR para SC_MARKNUM_FOLDEROPENMID */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 26, "255 255 255");   /* FGCOLOR para SC_MARKNUM_FOLDEROPENMID */
  IupSetAttributeId(multitext, "MARKERBGCOLOR", 27, "0 0 0");         /* BGCOLOR para SC_MARKNUM_FOLDERMIDTAIL */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 27, "255 255 255");   /* FGCOLOR para SC_MARKNUM_FOLDERMIDTAIL */
  IupSetAttributeId(multitext, "MARKERBGCOLOR", 28, "0 0 0");         /* BGCOLOR para SC_MARKNUM_FOLDERTAIL */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 28, "255 255 255");   /* FGCOLOR para SC_MARKNUM_FOLDERTAIL */
  IupSetAttributeId(multitext, "MARKERBGCOLOR", 29, "0 0 0");         /* BGCOLOR para SC_MARKNUM_FOLDERSUB */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 29, "255 255 255");   /* FGCOLOR para SC_MARKNUM_FOLDERSUB */
  IupSetAttributeId(multitext, "MARKERBGCOLOR", 30, "0 0 0");         /* BGCOLOR para SC_MARKNUM_FOLDER */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 30, "255 255 255");   /* FGCOLOR para SC_MARKNUM_FOLDER */
  IupSetAttributeId(multitext, "MARKERBGCOLOR", 31, "0 0 0");         /* BGCOLOR para SC_MARKNUM_FOLDEROPEN */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 31, "255 255 255");   /* FGCOLOR para SC_MARKNUM_FOLDEROPEN */

  IupSetAttribute(multitext, "PROPERTY", "fold=1");
  IupSetAttribute(multitext, "PROPERTY", "fold.compact=0"); /* avoid folding of blank lines */
  IupSetAttribute(multitext, "_IUP_FOLDDING", "1");

  /* Folding margin=3 */
  IupSetAttribute(multitext, "MARGINWIDTH3", FOLDING_MARGIN);
  IupSetAttribute(multitext, "MARGINMASKFOLDERS3", "Yes");
  IupSetAttribute(multitext, "MARGINSENSITIVE3", "YES");

  IupSetAttribute(multitext, "MARKERDEFINE", "FOLDER=BOXPLUS");
  IupSetAttribute(multitext, "MARKERDEFINE", "FOLDEROPEN=BOXMINUS");
  IupSetAttribute(multitext, "MARKERDEFINE", "FOLDEREND=BOXPLUSCONNECTED");
  IupSetAttribute(multitext, "MARKERDEFINE", "FOLDERMIDTAIL=TCORNER");
  IupSetAttribute(multitext, "MARKERDEFINE", "FOLDEROPENMID=BOXMINUSCONNECTED");
  IupSetAttribute(multitext, "MARKERDEFINE", "FOLDERSUB=VLINE");
  IupSetAttribute(multitext, "MARKERDEFINE", "FOLDERTAIL=LCORNER");

  IupSetAttribute(multitext, "FOLDFLAGS", "LINEAFTER_CONTRACTED");

  value = IupConfigGetVariableStr(config, "LuaScripter", "CommentColor");
  if (value)
    IupSetStrAttribute(multitext, "STYLEFGCOLOR1", value);
  else
    IupConfigSetVariableStr(config, "LuaScripter", "CommentColor", IupGetAttribute(multitext, "STYLEFGCOLOR1"));

  value = IupConfigGetVariableStr(config, "LuaScripter", "CommentLineColor");
  if (value)
    IupSetStrAttribute(multitext, "STYLEFGCOLOR2", value);
  else
    IupConfigSetVariableStr(config, "LuaScripter", "CommentLineColor", IupGetAttribute(multitext, "STYLEFGCOLOR2"));

  value = IupConfigGetVariableStr(config, "LuaScripter", "NumberColor");
  if (value)
    IupSetStrAttribute(multitext, "STYLEFGCOLOR4", value);
  else
    IupConfigSetVariableStr(config, "LuaScripter", "NumberColor", IupGetAttribute(multitext, "STYLEFGCOLOR4"));

  value = IupConfigGetVariableStr(config, "LuaScripter", "KeywordColor");
  if (value)
    IupSetStrAttribute(multitext, "STYLEFGCOLOR5", value);
  else
    IupConfigSetVariableStr(config, "LuaScripter", "KeywordColor", IupGetAttribute(multitext, "STYLEFGCOLOR5"));

  value = IupConfigGetVariableStr(config, "LuaScripter", "StringColor");
  if (value)
    IupSetStrAttribute(multitext, "STYLEFGCOLOR6", value);
  else
    IupConfigSetVariableStr(config, "LuaScripter", "StringColor", IupGetAttribute(multitext, "STYLEFGCOLOR6"));

  value = IupConfigGetVariableStr(config, "LuaScripter", "CharacterColor");
  if (value)
    IupSetStrAttribute(multitext, "STYLEFGCOLOR7", value);
  else
    IupConfigSetVariableStr(config, "LuaScripter", "CharacterColor", IupGetAttribute(multitext, "STYLEFGCOLOR7"));

  value = IupConfigGetVariableStr(config, "LuaScripter", "OperatorColor");
  if (value)
    IupSetStrAttribute(multitext, "STYLEFGCOLOR10", value);
  else
    IupConfigSetVariableStr(config, "LuaScripter", "OperatorColor", IupGetAttribute(multitext, "STYLEFGCOLOR10"));

  value = IupConfigGetVariableStr(config, "LuaScripter", "AutoCompletion");
  if (value)
    IupSetStrAttribute(multitext, "AUTOCOMPLETION", value);

  value = IupConfigGetVariableStr(config, "LuaScripter", "Folding");
  if (iupStrBoolean(value))
  {
    IupSetAttribute(multitext, "MARGINWIDTH3", FOLDING_MARGIN);
    IupSetAttribute(multitext, "PROPERTY", "fold=1");
    IupSetAttribute(multitext, "_IUP_FOLDDING", "1");
    IupSetAttribute(multitext, "FOLDALL", "EXPAND");
  }
  else
  {
    IupSetAttribute(multitext, "MARGINWIDTH3", "0");
    IupSetAttribute(multitext, "PROPERTY", "fold=0");
    IupSetAttribute(multitext, "_IUP_FOLDDING", NULL);
  }

  return IUP_DEFAULT;
}

static int newtext_cb(Ihandle* ih, Ihandle *multitext)
{
  (void)ih;
  /* this is called before the multitext is mapped */

  IupSetCallback(multitext, "VALUECHANGED_CB", (Icallback)multitext_valuechanged_cb);
  IupSetCallback(multitext, "LINESCHANGED_CB", (Icallback)multitext_lineschanged_cb);
  IupSetCallback(multitext, "K_ESC", (Icallback)multitext_kesc_cb);
  IupSetCallback(multitext, "MAP_CB", (Icallback)multitext_map_cb);
  IupSetCallback(multitext, "DWELL_CB", (Icallback)multitext_dwell_cb);

  IupSetAttribute(multitext, "AUTOCOMPLETION", "OFF");

  /* breakpoints margin=2 */
  IupSetAttribute(multitext, "MARGINWIDTH2", BREAKPOINT_MARGIN);
  IupSetAttribute(multitext, "MARGINTYPE2", "SYMBOL");
  IupSetAttribute(multitext, "MARGINSENSITIVE2", "YES");
  IupSetAttribute(multitext, "MARGINMASKFOLDERS2", "NO");  /* (disable folding) */
  IupSetAttributeId(multitext, "MARGINMASK", 2, "14");  /* 1110 - marker=1 and marker=2 ad marker=3 */

  /* breakpoints marker=1 */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 1, "255 0 0");
  IupSetAttributeId(multitext, "MARKERBGCOLOR", 1, "255 0 0");
  IupSetAttributeId(multitext, "MARKERSYMBOL", 1, "CIRCLE");

  /* current line marker=2 (not shown in a margin, but uses margin=2 mask) */
  IupSetAttributeId(multitext, "MARKERBGCOLOR", 2, "0 255 0");
  IupSetAttributeId(multitext, "MARKERALPHA", 2, "80");
  IupSetAttributeId(multitext, "MARKERSYMBOL", 2, "BACKGROUND");

  /* current line marker=3 */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 3, "0 0 0");
  IupSetAttributeId(multitext, "MARKERBGCOLOR", 3, "0 255 0");
  IupSetAttributeId(multitext, "MARKERALPHA", 3, "80");
  IupSetAttributeId(multitext, "MARKERSYMBOL", 3, "SHORTARROW");

  IupSetAttribute(multitext, "MOUSEDWELLTIME", "1000");

#ifdef WIN32
  IupSetAttribute(multitext, "FONT", "Consolas, 11");
#else
  IupSetAttribute(multitext, "FONT", "Monospace, 12");
  /* Other alternatives for "Consolas" in Linux: "DejaVu Sans Mono", "Liberation Mono", "Nimbus Mono L", "FreeMono" */
#endif

  return IUP_DEFAULT;
}

static int tree_globals_map(Ihandle* ih)
{
  Ihandle* config = get_config(ih);
  /* must load also here because we need the tree mapped */
  load_globals(ih, config);
  return IUP_NOERROR;
}


/********************************** Main *****************************************/


static Ihandle* buildLuaMenu(void)
{
  Ihandle *item_autocomplete, *item_style_config, *item_expand, *item_toggle, *item_level,
    *item_folding, *item_toggle_folding, *luaMenu, *item_options, *item_collapse,
    *item_blockcomment, *item_blockuncomment, *item_linescomment, *item_linesuncomment;

  item_autocomplete = IupItem("Auto Completion", NULL);
  IupSetAttribute(item_autocomplete, "NAME", "ITM_AUTOCOMPLETE");
  IupSetCallback(item_autocomplete, "ACTION", (Icallback)item_autocomplete_action_cb);

  item_style_config = IupItem("Syntax Colors...", NULL);
  IupSetAttribute(item_style_config, "NAME", "ITM_STYLE");
  IupSetCallback(item_style_config, "ACTION", (Icallback)item_style_config_action_cb);

  item_folding = IupItem("Folding", NULL);
  IupSetAttribute(item_folding, "NAME", "ITM_FOLDING");
  IupSetCallback(item_folding, "ACTION", (Icallback)item_folding_action_cb);

  item_toggle_folding = IupItem("Toggle Fold\tF8", NULL);
  IupSetAttribute(item_toggle_folding, "NAME", "ITM_TOGGLE_FOLDING");
  IupSetCallback(item_toggle_folding, "ACTION", (Icallback)item_toggle_folding_action_cb);

  item_collapse = IupItem("Collapse", NULL);
  IupSetAttribute(item_collapse, "NAME", "ITM_COLLAPSE");
  IupSetCallback(item_collapse, "ACTION", (Icallback)item_fold_collapse_action_cb);

  item_expand = IupItem("Expand", NULL);
  IupSetAttribute(item_expand, "NAME", "ITM_EXPAND");
  IupSetCallback(item_expand, "ACTION", (Icallback)item_fold_expand_action_cb);

  item_toggle = IupItem("Toggle", NULL);
  IupSetAttribute(item_toggle, "NAME", "ITM_TOGGLE");
  IupSetCallback(item_toggle, "ACTION", (Icallback)item_fold_toggle_action_cb);

  item_level = IupItem("by Level...", NULL);
  IupSetAttribute(item_level, "NAME", "ITM_LEVEL");
  IupSetCallback(item_level, "ACTION", (Icallback)item_fold_level_action_cb);

  item_blockcomment = IupItem("Block Comment", NULL);
  IupSetCallback(item_blockcomment, "ACTION", (Icallback)item_blockcomment_action_cb);

  item_blockuncomment = IupItem("Block Uncomment", NULL);
  IupSetCallback(item_blockuncomment, "ACTION", (Icallback)item_blockuncomment_action_cb);

  item_linescomment = IupItem("Lines Comment", NULL);
  IupSetCallback(item_linescomment, "ACTION", (Icallback)item_linescomment_action_cb);

  item_linesuncomment = IupItem("Lines Uncomment", NULL);
  IupSetCallback(item_linesuncomment, "ACTION", (Icallback)item_linesuncomment_action_cb);

  item_options = IupItem("Options...", NULL);
  IupSetCallback(item_options, "ACTION", (Icallback)item_options_action_cb);

  luaMenu = IupMenu(
    item_folding,
    item_toggle_folding,
    IupSubmenu("Fold All",
      IupSetAttributes(IupMenu(
        item_collapse,
        item_expand,
        item_toggle,
        item_level,
        NULL), "NAME=ITM_FOLD_ALL")),
    IupSeparator(),
    IupSubmenu("Comments",
      IupSetAttributes(IupMenu(
        item_blockcomment,
        item_blockuncomment,
        item_linescomment,
        item_linesuncomment,
        NULL), "NAME=ITM_COMMENTS")),
    IupSeparator(),
    item_autocomplete,
    IupSeparator(),
    item_style_config,
    item_options,
    NULL);

  iupLuaScripterDebuggerAddMenuItems(luaMenu);

  IupSetCallback(luaMenu, "OPEN_CB", (Icallback)lua_menu_open_cb);

  return IupSubmenu("&Lua", luaMenu);
}

static int iLuaScripterDlgCreateMethod(Ihandle* ih, void** params)
{
  lua_State *L;
  Ihandle *menu, *luaMenu;
  Ihandle *toolbar, *panelTabs, *multitextTabs;

  L = (lua_State*)IupGetGlobal("_IUP_LUA_DEFAULT_STATE");
  IupSetAttribute(ih, "LUASTATE", (char*)L);

  /* File Open/Save filters */
  IupSetAttribute(ih, "EXTRAFILTERS", "Lua Files|*.lua|");
  IupSetAttribute(ih, "DEFAULT_EXT", "lua");

  /* Additional hot keys */
  IupSetCallback(ih, "K_F8", (Icallback)item_toggle_folding_action_cb);

  iupLuaScripterDebuggerAddHotKeys(ih);

  /* configuring the IupScintillaDlg */
  IupSetCallback(ih, "EXIT_CB", (Icallback)exit_cb);
  IupSetCallback(ih, "MARKERCHANGED_CB", (Icallback)marker_changed_cb);
  IupSetCallback(ih, "RESTOREMARKERS_CB", (Icallback)restoremarkers_cb);
  IupSetCallback(ih, "CONFIGSAVE_CB", (Icallback)configsave_cb);
  IupSetCallback(ih, "CONFIGLOAD_CB", (Icallback)configload_cb);
  IupSetCallback(ih, "NEWTEXT_CB", (Icallback)newtext_cb);
  IupSetCallback(ih, "NEWFILENAME_CB", (Icallback)newfilename_cb);

  /* Call NEW_TEXT_CB because the first tab was already created */
  newtext_cb(ih, get_current_multitext(ih));

  /* Additional toolbar buttons */
  toolbar = IupGetDialogChild(ih, "TOOLBAR");
  iupLuaScripterDebuggerAddToolbarButtons(toolbar);

  luaMenu = buildLuaMenu();
  menu = IupGetAttributeHandle(ih, "MENU");
  IupInsert(menu, IupGetChild(menu, IupGetChildCount(menu) - 1), luaMenu);  /* penultimate submenu as reference */

  panelTabs = IupGetDialogChild(ih, "PANEL_TABS");
  multitextTabs = IupGetDialogChild(ih, "MULTITEXT_TABS");

  iuplua_push_name(L, "ConsoleCreate");
  iuplua_pushihandle(L, panelTabs);
  iuplua_call_raw(L, 1, 0);

  iupLuaScripterDebuggerCreateTabs(panelTabs);

  iuplua_push_name(L, "DebuggerInit");
  iuplua_pushihandle(L, panelTabs);
  iuplua_pushihandle(L, multitextTabs);
  iuplua_call_raw(L, 2, 0);

  {
    Ihandle* treeGlobals = IupGetDialogChild(ih, "DEBUG_TREE_GLOBAL");
    IupSetCallback(treeGlobals, "MAP_CB", (Icallback)tree_globals_map);
  }

  (void)params;
  return IUP_NOERROR;
}

static int iLuaScripterDebuggerCreate(lua_State *L)
{
  Ihandle* panelTabs = iuplua_checkihandle(L, 1);
  Ihandle* multitextTabs = iuplua_checkihandle(L, 2);

  IupSetAttribute(IupGetDialog(panelTabs), "LUASTATE", (char*)L);

  iupLuaScripterDebuggerCreateTabs(panelTabs);

  iuplua_push_name(L, "DebuggerInit");
  iuplua_pushihandle(L, panelTabs);
  iuplua_pushihandle(L, multitextTabs);
  iuplua_call_raw(L, 2, 0);

  return 0;
}

static int iLuaScripterDebuggerAddToolbarButtons(lua_State *L)
{
  Ihandle* toolbar = iuplua_checkihandle(L, 1);
  iupLuaScripterDebuggerAddToolbarButtons(toolbar);
  return 0;
}

static int iLuaScripterDebuggerAddMenuItems(lua_State *L)
{
  Ihandle* lua_menu = iuplua_checkihandle(L, 1);
  iupLuaScripterDebuggerAddMenuItems(lua_menu);
  return 0;
}

static int iLuaScripterDebuggerAddHotKeys(lua_State *L)
{
  Ihandle* ih = iuplua_checkihandle(L, 1);
  iupLuaScripterDebuggerAddHotKeys(ih);
  return 0;
}

static Iclass* iupLuaScripterDlgNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("scintilladlg"));

  ic->New = iupLuaScripterDlgNewClass;
  ic->Create = iLuaScripterDlgCreateMethod;

  ic->name = "luascripterdlg";
  ic->cons = "LuaScripterDlg";
  ic->nativetype = IUP_TYPEDIALOG;
  ic->is_interactive = 1;
  ic->childtype = IUP_CHILDNONE;
  ic->has_attrib_id = 1;   /* has attributes with IDs that must be parsed */

  iupClassRegisterReplaceAttribDef(ic, "SUBTITLE", IUPAF_SAMEASSYSTEM, "Scripter");

  if (!IupGetHandle("IUP_stepinto"))
    load_all_images_step_images();

  return ic;
}

Ihandle* IupLuaScripterDlg(void)
{
  return IupCreate("luascripterdlg");
}

void IupLuaScripterDlgOpen(lua_State *L)
{
  if (!IupGetGlobal("_IUP_LUASCRIPTERDLG_OPEN"))
  {
    IupScintillaOpen();
    IupImageLibOpen();

#if LUA_VERSION_NUM < 502
    lua_pushliteral(L, LUA_RELEASE "  " LUA_COPYRIGHT);
#else
    lua_pushliteral(L, LUA_COPYRIGHT);
#endif
    lua_setglobal(L, "_COPYRIGHT");  /* set global _COPYRIGHT */

#ifdef IUPLUA_USELOH
#include "debugger.loh"
#include "console.loh"
#else
#ifdef IUPLUA_USELH
#include "debugger.lh"
#include "console.lh"
#else
    iuplua_dofile(L, "console.lua");
    iuplua_dofile(L, "debugger.lua");
#endif
#endif

    iuplua_register(L, iLuaScripterDebuggerCreate, "DebuggerCreate");
    iuplua_register(L, iLuaScripterDebuggerAddToolbarButtons, "DebuggerAddToolbarButtons");
    iuplua_register(L, iLuaScripterDebuggerAddMenuItems, "DebuggerAddMenuItems");
    iuplua_register(L, iLuaScripterDebuggerAddHotKeys, "DebuggerAddHotKeys");

    iupRegisterClass(iupLuaScripterDlgNewClass());
    IupSetGlobal("_IUP_LUASCRIPTERDLG_OPEN", "1");
  }
}

/* TODO:
- ** condicional Breakpoints, Hit Count, When Hit
- sub-folder level for Projects
- multi-language (Portuguese, Spanish)
- detachable Console, Debug, Breakpoints (problem with IupGetDialogChild(NAME))?
- detachable Multitext?
*/

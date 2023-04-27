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


/********************************** Utilities *****************************************/


static Ihandle* get_current_multitext(lua_State *L)
{
  iuplua_push_name(L, "DebuggerGetCurrentMultitext");
  iuplua_call_raw(L, 0, 1);
  return iuplua_checkihandle(L, -1);
}

static void debug_set_state(lua_State *L, const char* state)
{
  iuplua_push_name(L, "DebuggerSetStateString");
  lua_pushstring(L, state);
  iuplua_call_raw(L, 1, 0);
}

static int debug_save_check(Ihandle* multitext, const char* filename, lua_State *L)
{
  if (!filename || IupGetInt(multitext, "MODIFIED"))
  {
    if (IupMessageAlarm(IupGetDialog(multitext), "Attention!", "File must be saved for debugging.\n  Save it now? (No will cancel debug)", "YESNO") == 1)
    {
      iuplua_push_name(L, "DebuggerSaveFile");
      iuplua_call_raw(L, 0, 0);
    }
    else
      return 0;
  }
  return 1;
}

static int debug_is_active(lua_State* L)
{
  int ret;
  iuplua_push_name(L, "DebuggerIsActive");
  iuplua_call_raw(L, 0, 1);
  ret = lua_toboolean(L, -1);
  lua_pop(L, 1);  /* remove the result from the stack */
  return ret;
}


/********************************** Callbacks *****************************************/


static int but_printlocal_cb(Ihandle *ih)
{
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");

  if (IupGetInt(NULL, "SHIFTKEY"))
    iuplua_push_name(L, "DebuggerPrintAllLocalVariables");
  else
    iuplua_push_name(L, "DebuggerPrintLocalVariable");

  iuplua_call_raw(L, 0, 0);
  return IUP_DEFAULT;
}

static int but_setlocal_cb(Ihandle *ih)
{
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  iuplua_push_name(L, "DebuggerSetLocalVariable");
  iuplua_call_raw(L, 0, 0);
  return IUP_DEFAULT;
}

static int tree_locals_action_cb(Ihandle *ih, int id, int v)
{
  lua_State* L;

  if (v == 0)
    return IUP_DEFAULT;

  L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  iuplua_push_name(L, "DebuggerLocalVariablesTreeAction");
  iuplua_pushihandle(L, ih);
  lua_pushinteger(L, id);
  iuplua_call_raw(L, 2, 0);
  return IUP_DEFAULT;
}

static int tree_locals_branchopen_cb(Ihandle *ih, int id)
{
  lua_State* L;

  L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  iuplua_push_name(L, "DebuggerLocalVariablesBranchOpenAction");
  iuplua_pushihandle(L, ih);
  lua_pushinteger(L, id);
  iuplua_call_raw(L, 2, 0);
  return IUP_DEFAULT;
}

static int tree_globals_action_cb(Ihandle *ih, int index, int v)
{
  lua_State* L;

  if (v == 0)
    return IUP_DEFAULT;

  L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  iuplua_push_name(L, "DebuggerGlobalsTreeAction");
  iuplua_pushihandle(L, ih);
  lua_pushinteger(L, index);
  iuplua_call_raw(L, 2, 0);
  return IUP_DEFAULT;
}

static int tree_globals_branchopen_cb(Ihandle *ih, int id)
{
  lua_State* L;

  L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  iuplua_push_name(L, "DebuggerGlobalVariablesBranchOpenAction");
  iuplua_pushihandle(L, ih);
  lua_pushinteger(L, id);
  iuplua_call_raw(L, 2, 0);
  return IUP_DEFAULT;
}

static int but_printglobal_cb(Ihandle *ih)
{
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");

  if (IupGetInt(NULL, "SHIFTKEY"))
    iuplua_push_name(L, "DebuggerPrintAllGlobalVariables");
  else
    iuplua_push_name(L, "DebuggerPrintGlobalVariable");

  iuplua_call_raw(L, 0, 0);
  return IUP_DEFAULT;
}

static int but_setglobal_cb(Ihandle *ih)
{
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  iuplua_push_name(L, "DebuggerSetGlobalVariable");
  iuplua_call_raw(L, 0, 0);
  return IUP_DEFAULT;
}

static int but_addglobal_cb(Ihandle *ih)
{
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  iuplua_push_name(L, "DebuggerAddGlobalVariable");
  iuplua_call_raw(L, 0, 0);
  return IUP_DEFAULT;
}

static int but_removeglobal_cb(Ihandle *ih)
{
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");

  if (IupGetInt(NULL, "SHIFTKEY"))
    iuplua_push_name(L, "DebuggerRemoveAllGlobalVariable");
  else
    iuplua_push_name(L, "DebuggerRemoveGlobalVariable");

  iuplua_call_raw(L, 0, 0);
  return IUP_DEFAULT;
}

static int list_stack_action_cb(Ihandle *ih, char *t, int index, int v)
{
  lua_State* L;
  (void)t;

  if (v == 0)
    return IUP_DEFAULT;

  L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  iuplua_push_name(L, "DebuggerStackListAction");
  iuplua_pushihandle(L, ih);
  lua_pushinteger(L, index);
  iuplua_call_raw(L, 2, 0);
  return IUP_DEFAULT;
}

static int list_stack_dblclick_cb(Ihandle *ih, int index, char *t)
{
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  iuplua_push_name(L, "DebuggerStackListActivate");
  iuplua_pushihandle(L, ih);
  lua_pushinteger(L, index);
  iuplua_call_raw(L, 2, 0);
  (void)t;
  return IUP_DEFAULT;
}

static int but_printlevel_cb(Ihandle *ih)
{
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");

  if (IupGetInt(NULL, "SHIFTKEY"))
    iuplua_push_name(L, "DebuggerPrintAllStackLevel");
  else
    iuplua_push_name(L, "DebuggerPrintStackLevel");

  iuplua_call_raw(L, 0, 0);

  return IUP_DEFAULT;
}

static int list_breaks_dblclick_cb(Ihandle *ih, int index, char *t)
{
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  iuplua_push_name(L, "DebuggerBreaksListActivate");
  iuplua_pushihandle(L, ih);
  lua_pushinteger(L, index);
  iuplua_call_raw(L, 2, 0);
  (void)t;
  return IUP_DEFAULT;
}

static int list_breaks_action_cb(Ihandle *ih, char *t, int index, int v)
{
  lua_State* L;
  (void)t;

  if (v == 0)
    return IUP_DEFAULT;

  L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  iuplua_push_name(L, "DebuggerBreaksListAction");
  iuplua_pushihandle(L, ih);
  lua_pushinteger(L, index);
  iuplua_call_raw(L, 2, 0);
  return IUP_DEFAULT;
}

static int but_togglebreak_cb(Ihandle *ih)
{
  int lin, col;
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  Ihandle* multitext = get_current_multitext(L);
  int pos = IupGetInt(multitext, "CARETPOS");
  IupTextConvertPosToLinCol(multitext, pos, &lin, &col);

  iuplua_push_name(L, "DebuggerToggleMarker");    /* this will trigger the MARKERCHANGED_CB callback too */
  lua_pushinteger(L, lin);
  iuplua_call_raw(L, 1, 0);

  return IUP_DEFAULT;
}

static int but_addbreak_cb(Ihandle* ih)
{
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  iuplua_push_name(L, "DebuggerAddBreakpointList");
  iuplua_call_raw(L, 0, 0);
  return IUP_DEFAULT;
}

static int item_removeallbreaks_cb(Ihandle *ih)
{
  lua_State* L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  iuplua_push_name(L, "DebuggerRemoveAllBreakpoints");
  iuplua_call_raw(L, 0, 0);
  return IUP_DEFAULT;
}

static int but_removebreak_cb(Ihandle *ih)
{
  if (IupGetInt(NULL, "SHIFTKEY"))
    item_removeallbreaks_cb(ih);
  else
  {
    lua_State* L;
    Ihandle* listBreak = IupGetDialogChild(ih, "DEBUG_LIST_BREAK");
    int index = IupGetInt(listBreak, "VALUE");

    if (index == 0)
    {
      IupMessageError(IupGetDialog(ih), "Must select a breakpoint on the list.");
      return IUP_DEFAULT;
    }

    L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
    iuplua_push_name(L, "DebuggerRemoveBreakpoint");
    iuplua_pushihandle(L, listBreak);
    lua_pushinteger(L, index);
    iuplua_call_raw(L, 2, 0);
  }

  return IUP_DEFAULT;
}

static void set_arguments(lua_State* L, const char* data)
{
  int i = 1, len = (int)strlen(data), value_len;
  char value[100];

  /* only positive indices will be set (non-zero) */

  lua_createtable(L, 0, 0);
  while (len > 0)
  {
    const char* next_value = iupStrNextValue(data, len, &value_len, ' ');

    if (value_len)
    {
      if (data[0] == '\"' && data[value_len - 1] == '\"')
      {
        data++;
        value_len -= 2;
      }

      memcpy(value, data, value_len);
      value[value_len] = 0;

      lua_pushstring(L, value);
      lua_rawseti(L, -2, i);
      i++;
    }

    data = next_value;
    len -= value_len + 1;
  }
  lua_setglobal(L, "arg");
}

static int item_debug_action_cb(Ihandle* ih_item)
{
  char* filename, *value;
  Ihandle* multitext;
  lua_State* L;
  int end_debug = 1;
  Ihandle* ih = IupGetDialog(ih_item);

  if (!IupGetInt(IupGetDialogChild(ih, "DEBUG_ITM_DEBUG"), "ACTIVE")) /* can be called by the hot key in the dialog */
    return IUP_DEFAULT;

  L = (lua_State*)IupGetAttribute(ih, "LUASTATE");

  if (debug_is_active(L)) /* already active, just continue */
  {
    debug_set_state(L, "DEBUG_ACTIVE");
    return IUP_DEFAULT;
  }

  if (IupGetInt(NULL, "SHIFTKEY"))
    end_debug = 0;

  multitext = get_current_multitext(L);
  filename = IupGetAttribute(multitext, "FILENAME");

  if (!debug_save_check(multitext, filename, L))
    return IUP_DEFAULT;

  iuplua_push_name(L, "DebuggerStartDebug");
  if (end_debug)
    lua_pushstring(L, filename);
  else
    lua_pushnil(L);
  iuplua_call_raw(L, 1, 0);

  value = IupGetAttribute(ih, "CURRENTDIRECTORY");
  if (value && value[0] != 0) iupdrvSetCurrentDirectory(value);
  value = IupGetAttribute(ih, "ARGUMENTS");
  if (value && value[0] != 0) set_arguments(L, value);

  iuplua_dofile(L, filename);

  if (end_debug)
  {
    iuplua_push_name(L, "DebuggerEndDebug");
    lua_pushboolean(L, 0);
    iuplua_call_raw(L, 1, 0);
  }

  return IUP_DEFAULT;
}

static int item_run_action_cb(Ihandle *ih_item)
{
  Ihandle* multitext;
  lua_State* L;
  char* filename, *value;
  Ihandle* ih = IupGetDialog(ih_item);

  if (!IupGetInt(IupGetDialogChild(ih, "DEBUG_ITM_RUN"), "ACTIVE")) /* can be called by the hot key in the dialog */
    return IUP_DEFAULT;

  L = (lua_State*)IupGetAttribute(ih, "LUASTATE");
  multitext = get_current_multitext(L);
  filename = IupGetAttribute(multitext, "FILENAME");

  value = IupGetAttribute(ih, "CURRENTDIRECTORY");
  if (value) iupdrvSetCurrentDirectory(value);
  value = IupGetAttribute(ih, "ARGUMENTS");
  if (value && value[0] != 0) set_arguments(L, value);

  if (filename && !IupGetInt(multitext, "MODIFIED"))
    iuplua_dofile(L, filename);
  else
  {
    char chunk_name[1024];
    char* title = IupGetAttribute(ih, "TITLE");
    if (!title)
      title = "iup.dostring";
    sprintf(chunk_name, "=%s", title);
    value = IupGetAttribute(multitext, "VALUE");
    iuplua_dostring(L, value, chunk_name);
  }

  return IUP_DEFAULT;
}

static int item_stop_action_cb(Ihandle *ih_item)
{
  lua_State* L;

  if (!IupGetInt(IupGetDialogChild(ih_item, "DEBUG_ITM_STOP"), "ACTIVE")) /* can be called by the hot key in the dialog */
  {
    L = (lua_State*)IupGetAttribute(ih_item, "LUASTATE");

    if (!debug_is_active(L))
      item_debug_action_cb(ih_item);
    return IUP_DEFAULT;
  }

  L = (lua_State*)IupGetAttribute(ih_item, "LUASTATE");
  debug_set_state(L, "DEBUG_STOPPED");
  return IUP_DEFAULT;
}

static int item_pause_action_cb(Ihandle *ih_item)
{
  lua_State* L;

  if (!IupGetInt(IupGetDialogChild(ih_item, "DEBUG_ITM_PAUSE"), "ACTIVE")) /* can be called by the hot key in the dialog */
    return IUP_IGNORE;  /* to avoid garbage in Scintilla when pressing the hot key */

  L = (lua_State*)IupGetAttribute(ih_item, "LUASTATE");
  debug_set_state(L, "DEBUG_PAUSED");
  return IUP_DEFAULT;
}

static int item_stepinto_action_cb(Ihandle *ih_item)
{
  lua_State* L;

  if (!IupGetInt(IupGetDialogChild(ih_item, "DEBUG_ITM_STEPINTO"), "ACTIVE")) /* can be called by the hot key in the dialog */
    return IUP_DEFAULT;

  L = (lua_State*)IupGetAttribute(ih_item, "LUASTATE");
  debug_set_state(L, "DEBUG_STEP_INTO");
  return IUP_DEFAULT;
}

static int item_stepover_action_cb(Ihandle *ih_item)
{
  lua_State* L;

  if (!IupGetInt(IupGetDialogChild(ih_item, "DEBUG_ITM_STEPOVER"), "ACTIVE")) /* can be called by the hot key in the dialog */
    return IUP_DEFAULT;

  L = (lua_State*)IupGetAttribute(ih_item, "LUASTATE");
  debug_set_state(L, "DEBUG_STEP_OVER");
  return IUP_IGNORE; /* avoid system default behavior for F10 key */
}

static int item_stepout_action_cb(Ihandle *ih_item)
{
  lua_State* L;

  if (!IupGetInt(IupGetDialogChild(ih_item, "DEBUG_ITM_STEPOUT"), "ACTIVE")) /* can be called by the hot key in the dialog */
    return IUP_DEFAULT;

  L = (lua_State*)IupGetAttribute(ih_item, "LUASTATE");
  debug_set_state(L, "DEBUG_STEP_OUT");
  return IUP_DEFAULT;
}

static int item_currentline_cb(Ihandle *ih_item)
{
  lua_State* L = (lua_State*)IupGetAttribute(ih_item, "LUASTATE");
  iuplua_push_name(L, "DebuggerShowCurrentLine");
  iuplua_call_raw(L, 0, 0);
  return IUP_DEFAULT;
}


/********************************** Main *****************************************/


void iupLuaScripterDebuggerAddMenuItems(Ihandle *lua_menu)
{
  Ihandle *item_debug, *item_run, *item_stop, *item_pause, *item_stepinto,
    *item_stepover, *item_stepout, *item_currentline, *item_togglebreakpoint,
    *item_addbreakpoint, *item_removeallbreakpoints;

  item_run = IupItem("&Run\tCtrl+F5", NULL);
  IupSetAttribute(item_run, "NAME", "DEBUG_ITM_RUN");
  IupSetCallback(item_run, "ACTION", (Icallback)item_run_action_cb);
  IupSetAttribute(item_run, "IMAGE", "IUP_MediaPlay");

  item_debug = IupItem("&Debug/Continue\tF5", NULL);
  IupSetAttribute(item_debug, "NAME", "DEBUG_ITM_DEBUG");
  IupSetCallback(item_debug, "ACTION", (Icallback)item_debug_action_cb);
  IupSetAttribute(item_debug, "IMAGE", "IUP_MediaGoToEnd");

  item_stop = IupItem("&Stop\tShift+F5", NULL);
  IupSetAttribute(item_stop, "NAME", "DEBUG_ITM_STOP");
  IupSetCallback(item_stop, "ACTION", (Icallback)item_stop_action_cb);
  IupSetAttribute(item_stop, "ACTIVE", "NO");
  IupSetAttribute(item_stop, "IMAGE", "IUP_MediaStop");

  item_pause = IupItem("&Pause\tCtrl+Break", NULL);
  IupSetAttribute(item_pause, "NAME", "DEBUG_ITM_PAUSE");
  IupSetCallback(item_pause, "ACTION", (Icallback)item_pause_action_cb);
  IupSetAttribute(item_pause, "ACTIVE", "NO");
  IupSetAttribute(item_pause, "IMAGE", "IUP_MediaPause");

  item_stepover = IupItem("Step &Over\tF10", NULL);
  IupSetAttribute(item_stepover, "NAME", "DEBUG_ITM_STEPOVER");
  IupSetCallback(item_stepover, "ACTION", (Icallback)item_stepover_action_cb);
  IupSetAttribute(item_stepover, "ACTIVE", "NO");
  IupSetAttribute(item_stepover, "IMAGE", "IUP_stepover");

  item_stepinto = IupItem("Step &Into\tF11", NULL);
  IupSetAttribute(item_stepinto, "NAME", "DEBUG_ITM_STEPINTO");
  IupSetCallback(item_stepinto, "ACTION", (Icallback)item_stepinto_action_cb);
  IupSetAttribute(item_stepinto, "ACTIVE", "NO");
  IupSetAttribute(item_stepinto, "IMAGE", "IUP_stepinto");

  item_stepout = IupItem("Step Ou&t\tShift+F11", NULL);
  IupSetAttribute(item_stepout, "NAME", "DEBUG_ITM_STEPOUT");
  IupSetCallback(item_stepout, "ACTION", (Icallback)item_stepout_action_cb);
  IupSetAttribute(item_stepout, "ACTIVE", "NO");
  IupSetAttribute(item_stepout, "IMAGE", "IUP_stepout");

  item_currentline = IupItem("Show Current Line", NULL);
  IupSetAttribute(item_currentline, "NAME", "DEBUG_ITM_CURRENTLINE");
  IupSetCallback(item_currentline, "ACTION", (Icallback)item_currentline_cb);
  IupSetAttribute(item_currentline, "ACTIVE", "NO");
  IupSetAttribute(item_currentline, "IMAGE", "IUP_ArrowRight");

  item_togglebreakpoint = IupItem("Toggle Breakpoint\tF9", NULL);
  IupSetCallback(item_togglebreakpoint, "ACTION", (Icallback)but_togglebreak_cb);

  item_addbreakpoint = IupItem("Add Breakpoint...", NULL);
  IupSetCallback(item_addbreakpoint, "ACTION", (Icallback)but_addbreak_cb);

  item_removeallbreakpoints = IupItem("Remove All Breakpoints", NULL);
  IupSetCallback(item_removeallbreakpoints, "ACTION", (Icallback)item_removeallbreaks_cb);

  IupInsert(lua_menu, NULL, IupSeparator());
  IupInsert(lua_menu, NULL, item_removeallbreakpoints);
  IupInsert(lua_menu, NULL, item_addbreakpoint);
  IupInsert(lua_menu, NULL, item_togglebreakpoint);
  IupInsert(lua_menu, NULL, IupSeparator());
  IupInsert(lua_menu, NULL, item_currentline);
  IupInsert(lua_menu, NULL, item_stepout);
  IupInsert(lua_menu, NULL, item_stepinto);
  IupInsert(lua_menu, NULL, item_stepover);
  IupInsert(lua_menu, NULL, item_pause);
  IupInsert(lua_menu, NULL, item_stop);
  IupInsert(lua_menu, NULL, item_debug);
  IupInsert(lua_menu, NULL, IupSeparator());
  IupInsert(lua_menu, NULL, item_run);
}

void iupLuaScripterDebuggerAddHotKeys(Ihandle *ih)
{
  IupSetCallback(ih, "K_F5", (Icallback)item_debug_action_cb);
  IupSetCallback(ih, "K_cF5", (Icallback)item_run_action_cb);
  IupSetCallback(ih, "K_sF5", (Icallback)item_stop_action_cb);
  IupSetCallback(ih, "K_cPAUSE", (Icallback)item_pause_action_cb);
  IupSetCallback(ih, "K_F10", (Icallback)item_stepover_action_cb);
  IupSetCallback(ih, "K_F11", (Icallback)item_stepinto_action_cb);
  IupSetCallback(ih, "K_sF11", (Icallback)item_stepout_action_cb);
  IupSetCallback(ih, "K_F9", (Icallback)but_togglebreak_cb);
}

void iupLuaScripterDebuggerAddToolbarButtons(Ihandle *toolbar)
{
  Ihandle *btn_debug, *btn_run, *btn_stop, *btn_pause, *btn_currentline;
  Ihandle *btn_stepinto, *btn_stepover, *btn_stepout;

  btn_debug = IupButton(NULL, NULL);
  IupSetAttribute(btn_debug, "NAME", "DEBUG_BTN_DEBUG");
  IupSetAttribute(btn_debug, "IMAGE", "IUP_MediaGoToEnd");
  IupSetAttribute(btn_debug, "FLAT", "Yes");
  IupSetCallback(btn_debug, "ACTION", (Icallback)item_debug_action_cb);
  IupSetAttribute(btn_debug, "TIP", "Debug/Continue (F5)\nPress <Shift> to keep debug active after script finishes.");
  IupSetAttribute(btn_debug, "CANFOCUS", "No");

  btn_run = IupButton(NULL, NULL);
  IupSetAttribute(btn_run, "NAME", "DEBUG_BTN_RUN");
  IupSetAttribute(btn_run, "IMAGE", "IUP_MediaPlay");
  IupSetAttribute(btn_run, "FLAT", "Yes");
  IupSetCallback(btn_run, "ACTION", (Icallback)item_run_action_cb);
  IupSetAttribute(btn_run, "TIP", "Run (Ctrl+F5)");
  IupSetAttribute(btn_run, "CANFOCUS", "No");

  btn_stop = IupButton(NULL, NULL);
  IupSetAttribute(btn_stop, "NAME", "DEBUG_BTN_STOP");
  IupSetAttribute(btn_stop, "ACTIVE", "NO");
  IupSetAttribute(btn_stop, "IMAGE", "IUP_MediaStop");
  IupSetAttribute(btn_stop, "FLAT", "Yes");
  IupSetCallback(btn_stop, "ACTION", (Icallback)item_stop_action_cb);
  IupSetAttribute(btn_stop, "TIP", "Stop (Shift+F5)");
  IupSetAttribute(btn_stop, "CANFOCUS", "No");

  btn_pause = IupButton(NULL, NULL);
  IupSetAttribute(btn_pause, "NAME", "DEBUG_BTN_PAUSE");
  IupSetAttribute(btn_pause, "ACTIVE", "NO");
  IupSetAttribute(btn_pause, "IMAGE", "IUP_MediaPause");
  IupSetAttribute(btn_pause, "FLAT", "Yes");
  IupSetCallback(btn_pause, "ACTION", (Icallback)item_pause_action_cb);
  IupSetAttribute(btn_pause, "TIP", "Pause (Ctrl+Break)");
  IupSetAttribute(btn_pause, "CANFOCUS", "No");

  btn_stepover = IupButton(NULL, NULL);
  IupSetAttribute(btn_stepover, "NAME", "DEBUG_BTN_STEPOVER");
  IupSetAttribute(btn_stepover, "ACTIVE", "NO");
  IupSetAttribute(btn_stepover, "IMAGE", "IUP_stepover");
  IupSetAttribute(btn_stepover, "FLAT", "Yes");
  IupSetCallback(btn_stepover, "ACTION", (Icallback)item_stepover_action_cb);
  IupSetAttribute(btn_stepover, "TIP", "Executes one step over the execution (F10).");
  IupSetAttribute(btn_stepover, "CANFOCUS", "No");

  btn_stepinto = IupButton(NULL, NULL);
  IupSetAttribute(btn_stepinto, "NAME", "DEBUG_BTN_STEPINTO");
  IupSetAttribute(btn_stepinto, "ACTIVE", "NO");
  IupSetAttribute(btn_stepinto, "IMAGE", "IUP_stepinto");
  IupSetAttribute(btn_stepinto, "FLAT", "Yes");
  IupSetCallback(btn_stepinto, "ACTION", (Icallback)item_stepinto_action_cb);
  IupSetAttribute(btn_stepinto, "TIP", "Executes one step into the execution (F11).");
  IupSetAttribute(btn_stepinto, "CANFOCUS", "No");

  btn_stepout = IupButton(NULL, NULL);
  IupSetAttribute(btn_stepout, "NAME", "DEBUG_BTN_STEPOUT");
  IupSetAttribute(btn_stepout, "ACTIVE", "NO");
  IupSetAttribute(btn_stepout, "IMAGE", "IUP_stepout");
  IupSetAttribute(btn_stepout, "FLAT", "Yes");
  IupSetCallback(btn_stepout, "ACTION", (Icallback)item_stepout_action_cb);
  IupSetAttribute(btn_stepout, "TIP", "Executes one step out of the execution (Shift+F11).");
  IupSetAttribute(btn_stepout, "CANFOCUS", "No");

  btn_currentline = IupButton(NULL, NULL);
  IupSetAttribute(btn_currentline, "NAME", "DEBUG_BTN_CURRENTLINE");
  IupSetAttribute(btn_currentline, "ACTIVE", "NO");
  IupSetAttribute(btn_currentline, "IMAGE", "IUP_ArrowRight");
  IupSetAttribute(btn_currentline, "FLAT", "Yes");
  IupSetCallback(btn_currentline, "ACTION", (Icallback)item_currentline_cb);
  IupSetAttribute(btn_currentline, "TIP", "Shows the debugger current line.");
  IupSetAttribute(btn_currentline, "CANFOCUS", "No");

  IupAppend(toolbar, IupSetAttributes(IupLabel(NULL), "SEPARATOR=VERTICAL"));
  IupAppend(toolbar, btn_run);
  IupAppend(toolbar, IupSetAttributes(IupLabel(NULL), "SEPARATOR=VERTICAL"));
  IupAppend(toolbar, btn_debug);
  IupAppend(toolbar, btn_stop);
  IupAppend(toolbar, btn_pause);
  IupAppend(toolbar, btn_stepover);
  IupAppend(toolbar, btn_stepinto);
  IupAppend(toolbar, btn_stepout);
  IupAppend(toolbar, btn_currentline);
}

static Ihandle *buildTabDebug(void)
{
  Ihandle *tree_local, *button_printLocal, *button_setLocal, *vbox_local, *frame_local;
  Ihandle *list_stack, *button_printLevel, *vbox_stack, *frame_stack, *debug;

  tree_local = IupTree();
  IupSetAttribute(tree_local, "EXPAND", "YES");
  IupSetAttribute(tree_local, "NAME", "DEBUG_TREE_LOCAL");
  IupSetAttribute(tree_local, "TIP", "List of local variables at selected stack level (ordered by pos)");
  IupSetAttribute(tree_local, "ADDROOT", "NO");
  IupSetAttribute(tree_local, "HIDELINES", "YES");
  IupSetAttribute(tree_local, "HIDEBUTTONS", "YES");
  IupSetAttribute(tree_local, "IMAGELEAF", "IUP_TreeEmpty");
  IupSetAttribute(tree_local, "IMAGEBRANCHCOLLAPSED", "IUP_TreePlus");
  IupSetAttribute(tree_local, "IMAGEBRANCHEXPANDED", "IUP_TreeMinus");
  IupSetAttribute(tree_local, "ADDEXPANDED", "NO");
  IupSetCallback(tree_local, "SELECTION_CB", (Icallback)tree_locals_action_cb);
  IupSetCallback(tree_local, "BRANCHOPEN_CB", (Icallback)tree_locals_branchopen_cb);

  button_printLocal = IupButton(NULL, NULL);
  IupSetAttribute(button_printLocal, "ACTIVE", "NO");
  IupSetAttribute(button_printLocal, "FLAT", "Yes");
  IupSetAttribute(button_printLocal, "IMAGE", "IUP_Print");
  IupSetAttribute(button_printLocal, "TIP", "Prints in the console debug information about the selected local variable.\nPress <Shift> to print all variables.");
  IupSetAttribute(button_printLocal, "NAME", "DEBUG_PRINT_LOCAL");
  IupSetCallback(button_printLocal, "ACTION", (Icallback)but_printlocal_cb);

  button_setLocal = IupButton(NULL, NULL);
  IupSetAttribute(button_setLocal, "IMAGE", "IUP_FileProperties");
  IupSetAttribute(button_setLocal, "FLAT", "Yes");
  IupSetAttribute(button_setLocal, "ACTIVE", "NO");
  IupSetAttribute(button_setLocal, "TIP", "Changes the value of the selected local variable.\nOnly strings, numbers and booleans can be changed.");
  IupSetAttribute(button_setLocal, "NAME", "DEBUG_SET_LOCAL");
  IupSetCallback(button_setLocal, "ACTION", (Icallback)but_setlocal_cb);

  vbox_local = IupVbox(button_printLocal, button_setLocal, NULL);
  IupSetAttribute(vbox_local, "MARGIN", "0x0");
  IupSetAttribute(vbox_local, "GAP", "4");
  IupSetAttribute(vbox_local, "NORMALIZESIZE", "HORIZONTAL");

  frame_local = IupFrame(IupHbox(tree_local, vbox_local, NULL));
  IupSetAttribute(frame_local, "MARGIN", "4x4");
  IupSetAttribute(frame_local, "GAP", "4");
  IupSetAttribute(frame_local, "TITLE", "Locals:");

  list_stack = IupList(NULL);
  IupSetAttribute(list_stack, "EXPAND", "YES");
  IupSetAttribute(list_stack, "NAME", "DEBUG_LIST_STACK");
  IupSetAttribute(list_stack, "TIP", "List of call stack (ordered by level)");
  IupSetCallback(list_stack, "ACTION", (Icallback)list_stack_action_cb);
  IupSetCallback(list_stack, "DBLCLICK_CB", (Icallback)list_stack_dblclick_cb);
  IupSetAttribute(list_stack, "VISIBLELINES", "3");

  button_printLevel = IupButton(NULL, NULL);
  IupSetAttribute(button_printLevel, "FLAT", "Yes");
  IupSetAttribute(button_printLevel, "IMAGE", "IUP_Print");
  IupSetAttribute(button_printLevel, "TIP", "Prints in the console debug information about the selected call stack level.\nPress <Shift> to print all levels.");
  IupSetAttribute(button_printLevel, "ACTIVE", "NO");
  IupSetAttribute(button_printLevel, "NAME", "DEBUG_PRINT_LEVEL");
  IupSetCallback(button_printLevel, "ACTION", (Icallback)but_printlevel_cb);

  vbox_stack = IupVbox(button_printLevel, NULL);
  IupSetAttribute(vbox_stack, "MARGIN", "0x0");
  IupSetAttribute(vbox_stack, "GAP", "4");
  IupSetAttribute(vbox_stack, "NORMALIZESIZE", "HORIZONTAL");

  frame_stack = IupFrame(IupHbox(list_stack, vbox_stack, NULL));
  IupSetAttribute(frame_stack, "MARGIN", "4x4");
  IupSetAttribute(frame_stack, "GAP", "4");
  IupSetAttribute(frame_stack, "TITLE", "Call Stack:");

  debug = IupHbox(frame_local, frame_stack, NULL);
  IupSetAttribute(debug, "MARGIN", "0x0");
  IupSetAttribute(debug, "GAP", "4");
  IupSetAttribute(debug, "TABTITLE", "Debug");

  return debug;
}

static Ihandle *buildTabWatch(void)
{
  Ihandle *tree_global, *button_printGlobal, *button_addGlobal, 
    *button_removeGlobal, *button_setGlobal, *vbox_global, *frame_global, *watch;

  tree_global = IupTree();
  IupSetAttribute(tree_global, "EXPAND", "YES");
  IupSetAttribute(tree_global, "NAME", "DEBUG_TREE_GLOBAL");
  IupSetAttribute(tree_global, "TIP", "List of globals variables or expressions");
  IupSetAttribute(tree_global, "ADDROOT", "NO");
  IupSetAttribute(tree_global, "HIDELINES", "YES");
  IupSetAttribute(tree_global, "HIDEBUTTONS", "YES");
  IupSetAttribute(tree_global, "IMAGELEAF", "IUP_TreeEmpty");
  IupSetAttribute(tree_global, "IMAGEBRANCHCOLLAPSED", "IUP_TreeEmpty");
  IupSetAttribute(tree_global, "ADDEXPANDED", "NO");
  IupSetAttribute(tree_global, "IMAGEBRANCHEXPANDED", "IUP_TreeEmpty");
  IupSetCallback(tree_global, "ACTION", (Icallback)tree_globals_action_cb);
  IupSetCallback(tree_global, "BRANCHOPEN_CB", (Icallback)tree_globals_branchopen_cb);

  button_printGlobal = IupButton(NULL, NULL);
  IupSetAttribute(button_printGlobal, "FLAT", "Yes");
  IupSetAttribute(button_printGlobal, "IMAGE", "IUP_Print");
  IupSetAttribute(button_printGlobal, "ACTIVE", "NO");
  IupSetAttribute(button_printGlobal, "TIP", "Prints in the console debug information about the selected global variable or expression.\nPress <Shift> to print all items.");
  IupSetAttribute(button_printGlobal, "NAME", "DEBUG_PRINT_GLOBAL");
  IupSetCallback(button_printGlobal, "ACTION", (Icallback)but_printglobal_cb);

  button_setGlobal = IupButton(NULL, NULL);
  IupSetAttribute(button_setGlobal, "IMAGE", "IUP_FileProperties");
  IupSetAttribute(button_setGlobal, "FLAT", "Yes");
  IupSetAttribute(button_setGlobal, "ACTIVE", "NO");
  IupSetAttribute(button_setGlobal, "TIP", "Changes the value of the selected global variable.\nOnly strings, numbers and booleans can be changed.");
  IupSetAttribute(button_setGlobal, "NAME", "DEBUG_SET_GLOBAL");
  IupSetCallback(button_setGlobal, "ACTION", (Icallback)but_setglobal_cb);

  button_addGlobal = IupButton(NULL, NULL);
  IupSetAttribute(button_addGlobal, "FLAT", "Yes");
  IupSetAttribute(button_addGlobal, "IMAGE", "IUP_plus");
  IupSetAttribute(button_addGlobal, "TIP", "Add a global variable given its name.");
  IupSetCallback(button_addGlobal, "ACTION", (Icallback)but_addglobal_cb);

  button_removeGlobal = IupButton(NULL, NULL);
  IupSetAttribute(button_removeGlobal, "FLAT", "Yes");
  IupSetAttribute(button_removeGlobal, "IMAGE", "IUP_EditErase");
  IupSetAttribute(button_removeGlobal, "NAME", "DEBUG_REMOVE_GLOBAL");
  IupSetAttribute(button_removeGlobal, "TIP", "Removes the selected global variable or expression.\nPress <Shift> to remove all items.");
  IupSetCallback(button_removeGlobal, "ACTION", (Icallback)but_removeglobal_cb);

  vbox_global = IupVbox(button_printGlobal, button_setGlobal, button_addGlobal, button_removeGlobal, NULL);
  IupSetAttribute(vbox_global, "MARGIN", "0x0");
  IupSetAttribute(vbox_global, "GAP", "4");
  IupSetAttribute(vbox_global, "NORMALIZESIZE", "HORIZONTAL");

  frame_global = IupFrame(IupHbox(tree_global, vbox_global, NULL));
  IupSetAttribute(frame_global, "MARGIN", "4x4");
  IupSetAttribute(frame_global, "GAP", "4");
  IupSetAttribute(frame_global, "TITLE", "Globals:");

  watch = IupHbox(frame_global, NULL);
  IupSetAttribute(watch, "MARGIN", "0x0");
  IupSetAttribute(watch, "GAP", "4");
  IupSetAttribute(watch, "TABTITLE", "Watch");

  return watch;
}

static Ihandle *buildTabBreaks(void)
{
  Ihandle *button_addbreak, *button_removebreak, *hbox, *list, *vbox, *frame, *button_togglebreak;

  button_togglebreak = IupButton(NULL, NULL);
  IupSetAttribute(button_togglebreak, "IMAGE", "IUP_MediaRecord");
  IupSetAttribute(button_togglebreak, "FLAT", "Yes");
  IupSetAttribute(button_togglebreak, "TIP", "Toggle a breakpoint at current line. (F9)");
  IupSetCallback(button_togglebreak, "ACTION", (Icallback)but_togglebreak_cb);

  button_addbreak = IupButton(NULL, NULL);
  IupSetAttribute(button_addbreak, "TIP", "Adds a breakpoint at given function and line.");
  IupSetAttribute(button_addbreak, "FLAT", "Yes");
  IupSetAttribute(button_addbreak, "IMAGE", "IUP_plus");
  IupSetCallback(button_addbreak, "ACTION", (Icallback)but_addbreak_cb);

  button_removebreak = IupButton(NULL, NULL);
  IupSetAttribute(button_removebreak, "FLAT", "Yes");
  IupSetAttribute(button_removebreak, "IMAGE", "IUP_EditErase");
  IupSetAttribute(button_removebreak, "TIP", "Removes the selected breakpoint.\nPress <Shift> to remove all breakpoints.");
  IupSetCallback(button_removebreak, "ACTION", (Icallback)but_removebreak_cb);
  IupSetAttribute(button_removebreak, "NAME", "DEBUG_REMOVE_BREAK");
  IupSetAttribute(button_removebreak, "ACTIVE", "NO");

  vbox = IupVbox(button_togglebreak, button_addbreak, button_removebreak, NULL);
  IupSetAttribute(vbox, "MARGIN", "0x0");
  IupSetAttribute(vbox, "GAP", "4");
  IupSetAttribute(vbox, "NORMALIZESIZE", "HORIZONTAL");

  list = IupList(NULL);
  IupSetAttribute(list, "EXPAND", "YES");
  IupSetAttribute(list, "NAME", "DEBUG_LIST_BREAK");
  IupSetCallback(list, "ACTION", (Icallback)list_breaks_action_cb);
  IupSetCallback(list, "DBLCLICK_CB", (Icallback)list_breaks_dblclick_cb);
  IupSetAttribute(list, "VISIBLELINES", "3");

  frame = IupFrame(IupHbox(list, vbox, NULL));
  IupSetAttribute(frame, "MARGIN", "4x4");
  IupSetAttribute(frame, "GAP", "4");
  IupSetAttribute(frame, "TITLE", "Breakpoints:");

  hbox = IupVbox(frame, NULL);
  IupSetAttribute(hbox, "MARGIN", "0x0");
  IupSetAttribute(hbox, "GAP", "4");
  IupSetAttribute(hbox, "TABTITLE", "Breaks");

  return hbox;
}

void iupLuaScripterDebuggerCreateTabs(Ihandle* panelTabs)
{
  Ihandle *tabDebug, *tabBreaks, *tabWatch;

  tabBreaks = buildTabBreaks();

  tabDebug = buildTabDebug();

  tabWatch = buildTabWatch();

  IupAppend(panelTabs, tabBreaks);
  IupAppend(panelTabs, tabDebug);
  IupAppend(panelTabs, tabWatch);
}

/******************************************************************************
 * Automatically generated file. Please don't change anything.                *
 *****************************************************************************/

#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "iup.h"
#include "iuplua.h"
#include "il.h"


static int flatlist_flat_action(Ihandle *self, char * p0, int p1, int p2)
{
  lua_State *L = iuplua_call_start(self, "flat_action");
  lua_pushstring(L, p0);
  lua_pushinteger(L, p1);
  lua_pushinteger(L, p2);
  return iuplua_call(L, 3);
}

static int flatlist_flat_motion_cb(Ihandle *self, int p0, int p1, char * p2)
{
  lua_State *L = iuplua_call_start(self, "flat_motion_cb");
  lua_pushinteger(L, p0);
  lua_pushinteger(L, p1);
  lua_pushstring(L, p2);
  return iuplua_call(L, 3);
}

static int FlatList(lua_State *L)
{
  Ihandle *ih = IupFlatList();
  iuplua_plugstate(L, ih);
  iuplua_pushihandle_raw(L, ih);
  return 1;
}

int iupflatlistlua_open(lua_State * L)
{
  iuplua_register(L, FlatList, "FlatList");

  iuplua_register_cb(L, "FLAT_ACTION", (lua_CFunction)flatlist_flat_action, "flatlist");
  iuplua_register_cb(L, "FLAT_MOTION_CB", (lua_CFunction)flatlist_flat_motion_cb, NULL);

#ifdef IUPLUA_USELOH
#include "flatlist.loh"
#else
#ifdef IUPLUA_USELH
#include "flatlist.lh"
#else
  iuplua_dofile(L, "flatlist.lua");
#endif
#endif

  return 0;
}


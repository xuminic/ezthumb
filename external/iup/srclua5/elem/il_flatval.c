/******************************************************************************
 * Automatically generated file. Please don't change anything.                *
 *****************************************************************************/

#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "iup.h"
#include "iuplua.h"
#include "il.h"


static int flatval_flat_wheel_cb(Ihandle *self, float p0, int p1, int p2, char * p3)
{
  lua_State *L = iuplua_call_start(self, "flat_wheel_cb");
  lua_pushnumber(L, p0);
  lua_pushinteger(L, p1);
  lua_pushinteger(L, p2);
  lua_pushstring(L, p3);
  return iuplua_call(L, 4);
}

static int flatval_valuechanging_cb(Ihandle *self, int p0)
{
  lua_State *L = iuplua_call_start(self, "valuechanging_cb");
  lua_pushinteger(L, p0);
  return iuplua_call(L, 1);
}

static int FlatVal(lua_State *L)
{
  Ihandle *ih = IupFlatVal((char *)luaL_optstring(L, 1, NULL));
  iuplua_plugstate(L, ih);
  iuplua_pushihandle_raw(L, ih);
  return 1;
}

int iupflatvallua_open(lua_State * L)
{
  iuplua_register(L, FlatVal, "FlatVal");

  iuplua_register_cb(L, "FLAT_WHEEL_CB", (lua_CFunction)flatval_flat_wheel_cb, NULL);
  iuplua_register_cb(L, "VALUECHANGING_CB", (lua_CFunction)flatval_valuechanging_cb, NULL);

#ifdef IUPLUA_USELOH
#include "flatval.loh"
#else
#ifdef IUPLUA_USELH
#include "flatval.lh"
#else
  iuplua_dofile(L, "flatval.lua");
#endif
#endif

  return 0;
}


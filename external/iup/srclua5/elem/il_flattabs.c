/******************************************************************************
 * Automatically generated file. Please don't change anything.                *
 *****************************************************************************/

#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "iup.h"
#include "iuplua.h"
#include "il.h"


static int flattabs_flat_getfocus_cb(Ihandle *self)
{
  lua_State *L = iuplua_call_start(self, "flat_getfocus_cb");
  return iuplua_call(L, 0);
}

static int flattabs_flat_killfocus_cb(Ihandle *self)
{
  lua_State *L = iuplua_call_start(self, "flat_killfocus_cb");
  return iuplua_call(L, 0);
}

static int FlatTabs(lua_State *L)
{
  Ihandle *ih = IupFlatTabs(NULL);
  iuplua_plugstate(L, ih);
  iuplua_pushihandle_raw(L, ih);
  return 1;
}

int iupflattabslua_open(lua_State * L)
{
  iuplua_register(L, FlatTabs, "FlatTabs");

  iuplua_register_cb(L, "FLAT_GETFOCUS_CB", (lua_CFunction)flattabs_flat_getfocus_cb, NULL);
  iuplua_register_cb(L, "FLAT_KILLFOCUS_CB", (lua_CFunction)flattabs_flat_killfocus_cb, NULL);

#ifdef IUPLUA_USELOH
#include "flattabs.loh"
#else
#ifdef IUPLUA_USELH
#include "flattabs.lh"
#else
  iuplua_dofile(L, "flattabs.lua");
#endif
#endif

  return 0;
}


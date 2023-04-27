/******************************************************************************
 * Automatically generated file. Please don't change anything.                *
 *****************************************************************************/

#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "iup.h"
#include "iuplua.h"
#include "il.h"


static int Space(lua_State *L)
{
  Ihandle *ih = IupSpace();
  iuplua_plugstate(L, ih);
  iuplua_pushihandle_raw(L, ih);
  return 1;
}

int iupspacelua_open(lua_State * L)
{
  iuplua_register(L, Space, "Space");


#ifdef IUPLUA_USELOH
#include "space.loh"
#else
#ifdef IUPLUA_USELH
#include "space.lh"
#else
  iuplua_dofile(L, "space.lua");
#endif
#endif

  return 0;
}


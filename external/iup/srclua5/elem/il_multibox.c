/******************************************************************************
 * Automatically generated file. Please don't change anything.                *
 *****************************************************************************/

#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "iup.h"
#include "iuplua.h"
#include "il.h"


static int MultiBox(lua_State *L)
{
  Ihandle *ih = IupMultiBox(NULL);
  iuplua_plugstate(L, ih);
  iuplua_pushihandle_raw(L, ih);
  return 1;
}

int iupmultiboxlua_open(lua_State * L)
{
  iuplua_register(L, MultiBox, "MultiBox");


#ifdef IUPLUA_USELOH
#include "multibox.loh"
#else
#ifdef IUPLUA_USELH
#include "multibox.lh"
#else
  iuplua_dofile(L, "multibox.lua");
#endif
#endif

  return 0;
}


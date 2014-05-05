/******************************************************************************
 * Automatically generated file. Please don't change anything.                *
 *****************************************************************************/

#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "iup.h"
#include "iuplua.h"
#include "il.h"


static int BackgroundBox(lua_State *L)
{
  Ihandle *ih = IupBackgroundBox(iuplua_checkihandleornil(L, 1));
  iuplua_plugstate(L, ih);
  iuplua_pushihandle_raw(L, ih);
  return 1;
}

int iupbackgroundboxlua_open(lua_State * L)
{
  iuplua_register(L, BackgroundBox, "BackgroundBox");


#ifdef IUPLUA_USELOH
#include "backgroundbox.loh"
#else
#ifdef IUPLUA_USELH
#include "backgroundbox.lh"
#else
  iuplua_dofile(L, "backgroundbox.lua");
#endif
#endif

  return 0;
}


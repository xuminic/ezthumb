/******************************************************************************
 * Automatically generated file. Please don't change anything.                *
 *****************************************************************************/

#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "iup.h"
#include "iuplua.h"
#include "il.h"


static int FlatTree(lua_State *L)
{
  Ihandle *ih = IupFlatTree();
  iuplua_plugstate(L, ih);
  iuplua_pushihandle_raw(L, ih);
  return 1;
}

int iupflattreelua_open(lua_State * L)
{
  iuplua_register(L, FlatTree, "FlatTree");


#ifdef IUPLUA_USELOH
#include "flattree.loh"
#else
#ifdef IUPLUA_USELH
#include "flattree.lh"
#else
  iuplua_dofile(L, "flattree.lua");
#endif
#endif

  return 0;
}


/** \file
 * \brief IupGetColor binding to Lua 5.
 *
 * See Copyright Notice in "iup.h"
 */
 
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "iup.h"

#include "iuplua.h"
#include "il.h"

 
static int GetColor(lua_State *L)
{
  int x = (int)luaL_checkinteger(L,1);
  int y = (int)luaL_checkinteger(L, 2);
  unsigned char r = (unsigned char)luaL_optinteger(L, 3, 0);
  unsigned char g = (unsigned char)luaL_optinteger(L, 4, 0);
  unsigned char b = (unsigned char)luaL_optinteger(L, 5, 0);
  int ret = IupGetColor(x,y,&r,&g,&b);
  if (ret) 
  {
    lua_pushinteger(L, r);
    lua_pushinteger(L, g);
    lua_pushinteger(L, b);
    return 3;
  } 
  else 
  {
    lua_pushnil(L);
    return 1;
  }
}

int iupgclua_open(lua_State * L)
{
  iuplua_register(L, GetColor, "GetColor");
  return 0;
}


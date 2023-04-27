/** \file
 * \brief IupGetParam binding to Lua 5.
 *
 * See Copyright Notice in "iup.h"
 */
 
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "iup.h"

#include "iuplua.h"
#include "il.h"


/* Used only by the Lua binding */
IUP_SDK_API int iupGetParamCount(const char *format, int *param_extra);
IUP_SDK_API char iupGetParamType(const char* format, int *line_size);


typedef struct _getparam_data
{
  lua_State *L;
  int has_func;
  int func_ref;
}getparam_data;

static int param_action(Ihandle* dialog, int param_index, void* user_data)
{
  int ret = 1;
  getparam_data* gp = (getparam_data*)user_data;
  if (gp->has_func)
  {
    lua_State *L = gp->L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, gp->func_ref);
    iuplua_pushihandle(L, dialog);
    lua_pushinteger(L, param_index);
    if (iuplua_call_raw(L, 2, 1) == LUA_OK)    /* 2 args, 1 return */
    {
      ret = (int)lua_tointeger(L, -1);
      lua_pop(L, 1);
    }
  }
  return ret;
}

#define LUA_MAX_PARAM 100

static int GetParam(lua_State *L)
{
  getparam_data gp;
  const char* title = luaL_checkstring(L, 1);
  void* user_data = (void*)&gp;
  const char* format = luaL_checkstring(L, 3);
  size_t size, max_str;
  int param_count, param_extra, i, ret,
      line_size = 0, lua_param_start = 4;
  const char* f = format;
  const char* s;
  void* param_data[LUA_MAX_PARAM];
  char param_type[LUA_MAX_PARAM];

  gp.L = L;
  gp.has_func = 0;
  gp.func_ref = 0;

  memset(param_data, 0, sizeof(void*)*LUA_MAX_PARAM);
  memset(param_type, 0, sizeof(char)*LUA_MAX_PARAM);

  param_count = iupGetParamCount(format, &param_extra);

  if (param_count > LUA_MAX_PARAM)
    return 0;

  for (i = 0; i < param_count; i++)
  {
    char t = iupGetParamType(f, &line_size);

    switch(t)
    {
    case 'x':
    case 'u':
    case 't':
      f += line_size;
      i--; /* compensate next increment */
      continue; /* notice this will go to the next i */
    case 'b':
      /*  TODO: add this code some day:
      if (lua_isboolean(L, lua_param_start))
      {
        param_data[i] = malloc(sizeof(int));
        *(int*)(param_data[i]) = lua_toboolean(L, lua_param_start); lua_param_start++;
        break;
      }  
      else continue and get an integer  */
    case 'i':
    case 'o':
    case 'l':
      param_data[i] = malloc(sizeof(int));
      *(int*)(param_data[i]) = (int)luaL_checkinteger(L, lua_param_start); lua_param_start++;
      break;
    case 'a':
    case 'r':
      param_data[i] = malloc(sizeof(float));
      *(float*)(param_data[i]) = (float)luaL_checknumber(L, lua_param_start); lua_param_start++;
      break;
    case 'A':
    case 'R':
      param_data[i] = malloc(sizeof(double));
      *(double*)(param_data[i]) = (double)luaL_checknumber(L, lua_param_start); lua_param_start++;
      break;
    case 'h':
      param_data[i] = iuplua_checkihandle(L, lua_param_start); lua_param_start++;  /* no malloc here */
      break;
    case 'd':
    case 'f':
    case 's':
    case 'm':
    case 'n':
    case 'c':
      max_str = 512;
      if (t == 'f')
        max_str = 4096;
      else if (t == 'm')
        max_str = 10240;
      s = luaL_checklstring(L, lua_param_start, &size); lua_param_start++;
      if (size < max_str)
        param_data[i] = malloc(max_str);
      else
        param_data[i] = malloc(2*size);
      memcpy(param_data[i], s, size+1);
      break;
    }

    param_type[i] = t;
    f += line_size;
  }

  if (lua_isfunction(L, 2))
  {
    lua_pushvalue(L, 2);
    gp.func_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    gp.has_func = 1;
  }

  ret = IupGetParamv(title, param_action, user_data, format, param_count, param_extra, param_data);

  lua_pushboolean(L, ret);

  if (ret)
  {
    for (i = 0; i < param_count; i++)
    {
      switch(param_type[i])
      {
      case 'b':
      case 'i':
      case 'o':
      case 'l':
        lua_pushinteger(L, *(int*)(param_data[i]));
        break;
      case 'A':
      case 'R':
        lua_pushnumber(L, *(double*)(param_data[i]));
        break;
      case 'a':
      case 'r':
        lua_pushnumber(L, *(float*)(param_data[i]));
        break;
      case 'd':
      case 'f':
      case 'n':
      case 'c':
      case 's':
      case 'm':
        lua_pushstring(L, (char*)(param_data[i]));
        break;
      }
    }
  }

  for (i = 0; i < param_count; i++)
  {
    if (param_type[i] != 'h')
      free(param_data[i]);
  }

  if (gp.has_func)
    luaL_unref(L, LUA_REGISTRYINDEX, gp.func_ref);

  if (ret)
    return param_count+1;
  else
    return 1;
}

static int GetParamParam(lua_State *L)
{
  Ihandle *dialog = iuplua_checkihandle(L, 1);
  int param_index = (int)luaL_checkinteger(L, 2);
  Ihandle* param = (Ihandle*)IupGetAttributeId(dialog, "PARAM", param_index);
  iuplua_pushihandle(L, param);
  return 1;
}

static int GetParamHandle(lua_State *L)
{
  Ihandle *param = iuplua_checkihandle(L, 1);
  const char* name = luaL_checkstring(L, 2);
  Ihandle* control = (Ihandle*)IupGetAttribute(param, name);
  iuplua_pushihandle(L, control);
  return 1;
}

void iupgetparamlua_open(lua_State * L)
{
  iuplua_register(L, GetParam, "GetParam");
  iuplua_register(L, GetParamParam, "GetParamParam");
  iuplua_register(L, GetParamHandle, "GetParamHandle");

  iupparamlua_open(L);
  iupparamboxlua_open(L);
}

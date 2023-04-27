/******************************************************************************
 * Automatically generated file. Please don't change anything.                *
 *****************************************************************************/

#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "iup.h"
#include "iuplua.h"
#include "il.h"


static int thread_thread_cb(Ihandle *self)
{
  lua_State *L = iuplua_call_start(self, "thread_cb");
  return iuplua_call(L, 0);
}

static int Thread(lua_State *L)
{
  Ihandle *ih = IupThread();
  iuplua_plugstate(L, ih);
  iuplua_pushihandle_raw(L, ih);
  return 1;
}

int iupthreadlua_open(lua_State * L)
{
  iuplua_register(L, Thread, "Thread");

  iuplua_register_cb(L, "THREAD_CB", (lua_CFunction)thread_thread_cb, NULL);

#ifdef IUPLUA_USELOH
#include "thread.loh"
#else
#ifdef IUPLUA_USELH
#include "thread.lh"
#else
  iuplua_dofile(L, "thread.lua");
#endif
#endif

  return 0;
}


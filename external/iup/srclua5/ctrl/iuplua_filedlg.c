
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "iup.h"
#include "iuplua.h"
#include "iupfiledlg.h"
#include "il.h"

 
int iupfiledlglua_open(lua_State* L)
{
  if (iuplua_opencall_internal(L))
    IupNewFileDlgOpen();
    
  return 0;
}

/* obligatory to use require"iupluafiledlg" */
int luaopen_iupluafiledlg(lua_State* L)
{
  return iupfiledlglua_open(L);
}

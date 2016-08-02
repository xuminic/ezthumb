/** \file
 * \brief Windows Driver IupHelp
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <shellapi.h>

#include <stdlib.h>
#include <stdio.h>

#include "iup.h"

int IupExecute(const char *filename, const char* parameters)
{
  /* no need to convert from UTF8 here */
  int err = (int)ShellExecuteA(GetDesktopWindow(), "open", filename, parameters, NULL, SW_SHOWNORMAL);
  if (err <= 32)
  {
    switch (err)
    {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
      return -2; /* File not found */
      break;
    default:
      return -1; /* Generic error */
      break;
    }
  }
  return 1;
}

int IupHelp(const char* url)
{
  return IupExecute(url, NULL);
}

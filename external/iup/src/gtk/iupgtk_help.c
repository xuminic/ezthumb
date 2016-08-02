/** \file
 * \brief GTK Driver IupHelp for non Windows systems
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "iup.h"

#include "iup_str.h"

int IupExecute(const char *filename, const char* parameters)
{
  GError *error = NULL;
  gchar *argv[3];
  int ret;
  
  argv[0] = (gchar*)filename;
  argv[1] = (gchar*)parameters;
  argv[2] = NULL;

  ret = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);

  if (error)
    g_error_free(error);

  if (!ret)
    return -1;
  return 1;
}

int IupHelp(const char* url)
{
  char *browser = getenv("IUP_HELPAPP");
  if (!browser)
    browser = IupGetGlobal("HELPAPP");

  if (!browser)
  {
    char* system = IupGetGlobal("SYSTEM");
    if (iupStrEqualNoCase(system, "Linux") ||
        iupStrEqualNoCase(system, "FreeBSD"))
        browser = "firefox";
    else if (iupStrEqualNoCase(system, "MacOS"))
      browser = "safari";
    else if (iupStrEqualPartial(system, "CYGWIN"))
      browser = "iexplore";
    else
      browser = "netscape";
  }

  return IupExecute(browser, url);
}

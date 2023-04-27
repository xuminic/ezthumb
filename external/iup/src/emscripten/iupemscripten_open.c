/** \file
 * \brief Emscripten Driver Core
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_object.h"
#include "iup_globalattrib.h"

#include "iupemscripten_drv.h"


#if 0
char* iupEmscriptenGetNativeWindowHandle(Ihandle* ih)
{
  id window = ih->handle->window;
  if (window)
    return (char*)window;
  else
    return NULL;
}
#endif

void* iupdrvGetDisplay(void)
{
  return NULL;
}


void iupEmscriptenUpdateGlobalColors(void)
{
  iupGlobalSetDefaultColorAttrib("DLGBGCOLOR", 237,237,237);

  iupGlobalSetDefaultColorAttrib("DLGFGCOLOR", 0,0,0);

  iupGlobalSetDefaultColorAttrib("TXTBGCOLOR", 255,255,255);

  iupGlobalSetDefaultColorAttrib("TXTFGCOLOR", 0,0,0);

  iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", 183,183,183);

  iupGlobalSetDefaultColorAttrib("MENUFGCOLOR", 0,0,0);
}

int iupdrvOpen(int *argc, char ***argv)
{
  (void)argc; /* unused in the Emscripten driver */
  (void)argv;

  // Emscripten doesn't enable persistent file storage by default. But we can opt-into a persistent local file storage called IDBFS.
  // We must create a mount point directory, mount the file system there, and call syncfs (with two params).
  // Subsequent writes need to call syncfs (with one param).
  // Make sure to write to the correct mount location.
  // IMPORTANT: If you change the path (/IupIDBFS), make sure to change it also in iupdrvGetPreferencePath().
  // WARNING: I'm seeing race conditions in the web browser (Chrome). I think we're using this correctly, but sometimes my read tests fail. Break points seem to give the system time to catch up. I'm leaving console.log() messages to try to help.
  EM_ASM(
    FS.mkdir('/IupIDBFS');
    FS.mount(IDBFS, {}, '/IupIDBFS');
	console.log('Mounted /IupIDBFS');
	FS.syncfs(true, function(err) {
        if(err) { console.log('Error: FS.mount and FS.syncfs failed', err); }
		else { console.log('Mounting /IupIDBFS and syncing for mount'); }
	  }
	);
  );

	// Assuming we're always on the main thread.

  // TODO: Use this to reinitialize static/global variables???
	iupEmscripten_InitializeInternalGlobals();

  IupSetGlobal("DRIVER", "Emscripten");

//  IupSetGlobal("SYSTEMLANGUAGE", iupEmscriptenGetSystemLanguage());

  iupEmscriptenUpdateGlobalColors();

  IupSetGlobal("_IUP_RESET_GLOBALCOLORS", "YES");  /* will update the global colors when the first dialog is mapped */


  return IUP_NOERROR;
}

// WARNING: We don't have a way to ever call IupClose(), so this may never run.
void iupdrvClose(void)
{

  iupEmscripten_DestroyInternalGlobals();

  EM_ASM(
	FS.syncfs(function(err) {
        if(err) console.log('Error: FS.syncfs failed', err);
	  }
	);
    FS.unmount('/IupIDBFS');
  );
}

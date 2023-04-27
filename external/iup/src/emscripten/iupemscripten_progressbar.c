/** \file
* \brief Progress bar Control
*
* See Copyright Notice in "iup.h"
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_progressbar.h"
#include "iup_drv.h"

#include "iupemscripten_drv.h"

// TODO: API: I think we're going to need a separate start/stop key.
// emscripten Indeterminate is for progresses you don't know the range for, but are still animated when in progress.

// TODO: FEATURE: emscripten provides spinner style


extern int emjsProgressBar_Create(void);

extern int emjsProgressBar_SetValueAttrib(int handle_id, double value);

static int emscriptenProgressBarMapMethod(Ihandle* ih)
{

  int progressBar_id = 0;
  InativeHandle* new_handle = NULL;

  progressBar_id = emjsProgressBar_Create();
  new_handle = (InativeHandle*)calloc(1, sizeof(InativeHandle));

  new_handle->handleID = progressBar_id;
  ih->handle = new_handle;

  iupEmscripten_SetIntKeyForIhandleValue(progressBar_id, ih);

  iupEmscripten_AddWidgetToParent(ih);

  return IUP_NOERROR;
}

static int emscriptenProgressBarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->marquee) return 0; /* Hypothesis: If user wanted a marquee, just give up setting value because that will be default behavior in browser */

  if (!value) { /* If no value, put 0 (to avoid default marquee state) */
    iupStrToDouble("0", &(ih->data->value));
    return 0; }
  else { 
    iupStrToDouble(value, &(ih->data->value)); /*idk what I'm doing just copying GTK don't mind me */
    emjsProgressBar_SetValueAttrib(ih->handle->handleID, ih->data->value);
    return 0;
  }
}

static void emscriptenProgressBarUnMapMethod(Ihandle* ih)
{
}

void iupdrvProgressBarInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
	ic->Map = emscriptenProgressBarMapMethod;
	ic->UnMap = emscriptenProgressBarUnMapMethod;

  /* Driver Dependent Attribute functions */
  
  /* Visual */
//  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  
  /* Special */
//  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
/* #if 0 */

  /* IupProgressBar only */
  iupClassRegisterAttribute(ic, "VALUE",  iProgressBarGetValueAttrib,  emscriptenProgressBarSetValueAttrib,  NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

	
/*   iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NO_INHERIT); */
/*   iupClassRegisterAttribute(ic, "MARQUEE",     NULL, emscriptenProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT); */
/*   iupClassRegisterAttribute(ic, "DASHED",      NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT); */
/* #endif */
	
}


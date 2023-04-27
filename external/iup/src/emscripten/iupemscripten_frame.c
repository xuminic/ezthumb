/** \file
 * \brief Frame Control
 *
 * See Copyright Notice in "iup.h"
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_frame.h"

#include "iup_mask.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_list.h"

#include "iupemscripten_drv.h"

extern int emjsFrame_CreateFrame(void);

static int emscriptenFrameMapMethod(Ihandle* ih)
{
  int frame_id = 0;
  InativeHandle* new_handle = NULL;

  frame_id = emjsFrame_CreateFrame();
  new_handle = (InativeHandle*)calloc(1, sizeof(InativeHandle));

  new_handle->handleID = frame_id;
  ih->handle = new_handle;

  iupEmscripten_SetIntKeyForIhandleValue(frame_id, ih);

  // If title is set, add to frame
  char* attribute_title = iupAttribGet(ih, "TITLE");
  if (attribute_title && *attribute_title != 0)
  {
    // why do we always have this in there?
    /* ih->data->type != IUP_LABEL_TEXT; */
    /* emjsFrame_SetTitle(frame_id, attribute_title); */
  }

	iupEmscripten_AddWidgetToParent(ih);

	return IUP_NOERROR;
}


static void emscriptenFrameUnMapMethod(Ihandle* ih)
{
#if 0
	id the_frame = ih->handle;
	[the_frame release];
	ih->handle = nil;
#endif

}



void iupdrvFrameGetDecorOffset(Ihandle* ih, int *x, int *y)
{
	*x = 2;
	*y = 2;
}

int iupdrvFrameHasClientOffset(Ihandle* ih)
{
	return 0;
}

int iupdrvFrameGetTitleHeight(Ihandle* ih, int *h)
{
  (void)ih;
  (void)h;
  return 0;
}

int iupdrvFrameGetDecorSize(Ihandle* ih, int *w, int *h)
{
  (void)ih;
  (void)w;
  (void)h;
  return 0;
}


void iupdrvFrameInitClass(Iclass* ic)
{
	/* Driver Dependent Class functions */
	ic->Map = emscriptenFrameMapMethod;
	ic->UnMap = emscriptenFrameUnMapMethod;
#if 0
	ic->GetInnerNativeContainerHandle = gtkFrameGetInnerNativeContainerHandleMethod;

	/* Driver Dependent Attribute functions */

	/* Overwrite Common */
	iupClassRegisterAttribute(ic, "STANDARDFONT", NULL, gtkFrameSetStandardFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED);

	/* Visual */
	iupClassRegisterAttribute(ic, "BGCOLOR", iupFrameGetBgColorAttrib, gtkFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "SUNKEN", NULL, gtkFrameSetSunkenAttrib, NULL, NULL, IUPAF_NO_INHERIT);

	/* Special */
	iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtkFrameSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "TITLE", NULL, gtkFrameSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
#endif
}

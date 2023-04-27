/** \file
 * \brief Button Control
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
#include "iup_image.h"
#include "iup_button.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_key.h"

#include "iupemscripten_drv.h"

#include <emscripten.h>



//EMSCRIPTEN_KEEPALIVE void emscriptenButtonCallbackTrampoline(int handle_id, Ihandle* ih)
//EMSCRIPTEN_KEEPALIVE void emscriptenButtonCallbackTrampoline(int handle_id, intptr_t ih_ptr)
//EMSCRIPTEN_KEEPALIVE void emscriptenButtonCallbackTrampoline(int handle_id, int ih_ptr)

// Generate IupButton ACTION callback to feed to js
EMSCRIPTEN_KEEPALIVE void emscriptenButtonCallbackTrampoline(int handle_id)
{
  Ihandle* ih = iupEmscripten_GetIhandleValueForKey(handle_id);
  Icallback action_callback = IupGetCallback(ih, "ACTION");
  if (action_callback) 
    {
      action_callback(ih);

      /* if(action_callback(ih) == IUP_CLOSE) */
      /* { */
      /* 	IupExitLoop(); */
      /* } */
    }

}

EMSCRIPTEN_KEEPALIVE void emscriptenButtonCallbackTrampoline_Cb(int handle_id, int but, int pressed, int x, int y, char* status) 
{
  Ihandle* ih = iupEmscripten_GetIhandleValueForKey(handle_id);

  IFniiiis button_callback = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (button_callback)
    {
      button_callback(ih, but, pressed, x, y, status);
    }

#if 0
  if(button_callback)
    {
      if(button_callback(handle_id, but, pressed, x, y, status) == IUP_CLOSE)
	{
	  IupExitLoop();
	}
    }
#endif
}

void iupdrvButtonAddBorders(Ihandle* ih, int *x, int *y)
{
  // have to add beause system is making button too small in some select cases; look into why? TODO
  *x += 1;
  /* *y += 0; */
}

extern int emjsButton_CreateButton(void);
extern void emjsButton_SetTitle(int handle_id, const char* title);
//extern void emjsButton_SetCallback(int handle_id, Ihandle* ih);
//extern void emjsButton_SetCallback(int handle_id, intptr_t ih);
extern void emjsButton_SetCallback(int handle_id);

static int emscriptenButtonMapMethod(Ihandle* ih)
{
#if 1
  int button_id = 0;
  InativeHandle* new_handle = NULL;
  char* attribute_value;
  // TODO: Image button
  // emscripten.widget.Button
  // emscripten.widget.ImageButton
  attribute_value = iupAttribGet(ih, "IMAGE");

  if (attribute_value && *attribute_value != 0)
    {
      ih->data->type |= IUP_BUTTON_IMAGE;
    }
  else
    {
      button_id = emjsButton_CreateButton();
      new_handle = (InativeHandle*)calloc(1, sizeof(InativeHandle));
      new_handle->handleID = button_id;
      ih->handle = new_handle;
      /* emjsCommon_Alert(ih->handle); */
    }

  // Does ImageButton support title text?
  attribute_value = iupAttribGet(ih, "TITLE");
  if(attribute_value && *attribute_value!=0)  //Is the derefrenced value going to be 0 if null? 
    {
      //TODO ask eric
      ih->data->type |= IUP_BUTTON_TEXT;
      /*
	if(ih->data->type & IUP_BUTTON_IMAGE)
	{
	}
	else
	{
	}
      */

      emjsButton_SetTitle(button_id, attribute_value);
    }

  //emjsButton_SetCallback(button_id, (intptr_t)ih);
  emjsButton_SetCallback(button_id);
  iupEmscripten_SetIntKeyForIhandleValue(button_id, ih);


  iupEmscripten_AddWidgetToParent(ih);
#endif
  return IUP_NOERROR;
}

extern void emjsButton_DestroyButton(int handle_id);
static void emscriptenButtonUnMapMethod(Ihandle* ih)
{
  if(ih && ih->handle) {
    iupEmscripten_RemoveIntKeyFromIhandleMap(ih->handle->handleID);
    emjsButton_DestroyButton(ih->handle->handleID);
    free(ih->handle);
    ih->handle = NULL;
  }
}

void iupdrvButtonInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = emscriptenButtonMapMethod;
  ic->UnMap = emscriptenButtonUnMapMethod;

#if 0

  ic->LayoutUpdate = gtkButtonLayoutUpdateMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Common */
  iupClassRegisterAttribute(ic, "STANDARDFONT", NULL, gtkButtonSetStandardFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED);

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, gtkButtonSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtkButtonSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtkButtonSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, gtkButtonSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupButton only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, gtkButtonSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);  /* force new default value */
  iupClassRegisterAttribute(ic, "IMAGE", NULL, gtkButtonSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, gtkButtonSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, gtkButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
#endif

}

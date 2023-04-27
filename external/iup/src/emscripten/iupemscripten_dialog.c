/** \file
 * \brief IupDialog class
 *
 * See Copyright Notice in "iup.h"
 */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>
#include <stdint.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_class.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_dlglist.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_focus.h"
#include "iup_str.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"
#include "iup_image.h"
#include "iup_assert.h"

#include "iupemscripten_drv.h"
#include <emscripten.h>


/*
1. compute size of various elements
2. section that tells javascript to draw elements given a size
(iupdrvBaseLayoutUpdateMethod - where everything for this should start)
3. padding and sizing stuff - one func that computes the size of string for things like labels and buttons, and need to fill in iup functs that compute padding

1. get size of widget
2. get size of string
3. draw widget precisely based on given params
*/


extern void emjsDialog_GetSize(int handle_id, int* out_ptr_outer_width, int* out_ptr_outer_height, int* out_ptr_inner_width, int* out_ptr_inner_height);

		
// FIXME: The innerWidth/innerHeight is what we need for layout computations. 
// outerWidth/outerHeight is (I think) what we need if the user asks to make a window a certain size or wants the window size.
// Since the internals use ih->currentwidth and ih->currentheight for layout computation, I think we should set these to inner.
// That means we need to correctly override anyplace that tries to get the outer window dimensions.
// We may want to create new fields in ih->outerWidth, etc. 
EMSCRIPTEN_KEEPALIVE void emscriptenDialogResizeCallbackTrampoline(int handle_id, int outer_width, int outer_height, int inner_width, int inner_height) 
{
	Ihandle* ih = iupEmscripten_GetIhandleValueForKey(handle_id);
//	iupEmscripten_Log("emscriptenDialogResizeCallbackTrampoline is ih:%p, id:%d, <outer_w:%d, outer_h:%d>, <inner_width:%d, inner_h:%d>", ih, handle_id, outer_width, outer_height, inner_width, inner_height);
	IFnii cb;
	cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
	// FIXME: Are the parameters supposed to be the contentView or the entire window. The Windows code comments make me think contentView, but the actual code makes me think entire window. The latter is way easier to do.
	if(!cb || cb(ih, inner_width, inner_height)!=IUP_IGNORE)
	{
		ih->currentwidth = inner_width;
		ih->currentheight = inner_height;
		
//		ih->data->ignore_resize = 1;
		IupRefresh(ih);
//		ih->data->ignore_resize = 0;
	}
	else
	{
		// FIXME: How do we prevent a resize? Is this even possible, e.g. mobile web browser, or in a tab among multiple tabs?

		// For now, do a resize.
		ih->currentwidth = inner_width;
		ih->currentheight = inner_height;
		IupRefresh(ih);
	}

}


/****************************************************************
 Utilities
 ****************************************************************/

int iupdrvDialogIsVisible(Ihandle* ih)
{
//	return iupdrvIsVisible(ih);
	return 1;
}


void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int *w, int *h)
{
  // first grab dialog element
  // then find width and height
  // then set appropriately below

	int out_outer_width = 0;
	int out_outer_height = 0;
	int out_inner_width = 0;
	int out_inner_height = 0;

	emjsDialog_GetSize(ih->handle->handleID, &out_outer_width, &out_outer_height, &out_inner_width, &out_inner_height);

	// FIXME: Do we use outer or inner?
	if (w) *w = out_outer_width;
	if (h) *h = out_outer_height;
}

void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{

	if(visible)
	{

	}
	else
	{

	}
}

void iupdrvDialogGetPosition(Ihandle *ih, InativeHandle* handle, int *x, int *y)
{

	if (x) *x = 0;
	//if (y) *y = iupEmscriptenComputeIupScreenHeightFromCartesian(the_rect.origin.y);
	if (y) *y = 0;
}

void iupdrvDialogSetPosition(Ihandle *ih, int x, int y)
{
}


void iupdrvDialogGetDecoration(Ihandle* ih, int *border, int *caption, int *menu)
{
	// TODO: these are placeholder values
	if(border) *border = 0;
	if(caption) *caption = 0;
	if(menu) *menu = 0;
}

int iupdrvDialogSetPlacement(Ihandle* ih)
{
	
#if 0
	char* placement;
	
	NSWindow* the_window = (NSWindow*)ih->handle;
	NSRect the_rect = [the_window frame];
	
	
	int old_state = ih->data->show_state;
	ih->data->show_state = IUP_SHOW;
	
	if (iupAttribGetBoolean(ih, "FULLSCREEN"))
	{

		NSUInteger masks = [the_window styleMask];
		if ( masks & NSFullScreenWindowMask)
		{
			// Do something
		}
		else
		{
			[the_window toggleFullScreen:nil];
		}
		
		
		return 1;
	}
	
	placement = iupAttribGet(ih, "PLACEMENT");
	if (!placement)
	{
		if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
			ih->data->show_state = IUP_RESTORE;
		
//		gtk_window_unmaximize((GtkWindow*)ih->handle);
//		gtk_window_deiconify((GtkWindow*)ih->handle);
		return 0;
	}
	
	if (iupStrEqualNoCase(placement, "MINIMIZED"))
	{
//		ih->data->show_state = IUP_MINIMIZE;
//		gtk_window_iconify((GtkWindow*)ih->handle);
	}
	else if (iupStrEqualNoCase(placement, "MAXIMIZED"))
	{
//		ih->data->show_state = IUP_MAXIMIZE;
//		gtk_window_maximize((GtkWindow*)ih->handle);
	}
	else if (iupStrEqualNoCase(placement, "FULL"))
	{
#if 0
		int width, height, x, y;
		int border, caption, menu;
		iupdrvDialogGetDecoration(ih, &border, &caption, &menu);
		
		/* position the decoration outside the screen */
		x = -(border);
		y = -(border+caption+menu);
		
		/* the dialog client area will cover the task bar */
		iupdrvGetFullSize(&width, &height);
		
		height += menu; /* menu is inside the client area. */
		
		/* set the new size and position */
		/* The resize evt will update the layout */
		gtk_window_move((GtkWindow*)ih->handle, x, y);
		gtk_window_resize((GtkWindow*)ih->handle, width, height);
		
		if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
			ih->data->show_state = IUP_RESTORE;
#endif
	}
	
	iupAttribSet(ih, "PLACEMENT", NULL); /* reset to NORMAL */
	
#endif


	return 1;
}

void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* parent)
{

}



/****************************************************************
 Callbacks and Events
 ****************************************************************/

static int emscriptenDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
	
	return 1;
}

extern int emjsDialog_CreateDialog(char* window_name, int width, int height);
static int emscriptenDialogMapMethod(Ihandle* ih)
{

	char* window_title = NULL;
	int width = 0;
	int height = 0;
	
	window_title = iupAttribGet(ih, "TITLE");

	int dialog_id = emjsDialog_CreateDialog(window_title, width, height);
	InativeHandle* new_handle = (InativeHandle*)calloc(1, sizeof(InativeHandle));
	new_handle->handleID = dialog_id;
	ih->handle = new_handle;
	iupEmscripten_SetIntKeyForIhandleValue(dialog_id, ih);
	
	int out_outer_width = 0;
	int out_outer_height = 0;
	int out_inner_width = 0;
	int out_inner_height = 0;
	emjsDialog_GetSize(ih->handle->handleID, &out_outer_width, &out_outer_height, &out_inner_width, &out_inner_height);
	// FIXME: Do we use outer or inner? I am using inner for now because we need it for layout.
	ih->currentwidth = out_inner_width;
	ih->currentheight = out_inner_height;
	
//	iupEmscripten_Log("emscriptenDialogMapMethod is ih:%p, id:%d", ih, dialog_id);
//	iupAttribSet(ih, "RASTERSIZE", "500x400");
	

	return IUP_NOERROR;

}

extern void emjsDialog_DestroyDialog(int handle_id);
static void emscriptenDialogUnMapMethod(Ihandle* ih)
{
	if(ih && ih->handle)
	{
		emjsDialog_DestroyDialog(ih->handle->handleID);
		free(ih->handle);
		ih->handle = NULL;
	}
}

#if 1
static void emscriptenDialogLayoutUpdateMethod(Ihandle* ih)
{

}
#endif


void iupdrvDialogInitClass(Iclass* ic)
{
	/* Driver Dependent Class methods */
	ic->Map = emscriptenDialogMapMethod;
	ic->UnMap = emscriptenDialogUnMapMethod;
	ic->LayoutUpdate = emscriptenDialogLayoutUpdateMethod;
//	ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

#if 0
	ic->LayoutUpdate = gtkDialogLayoutUpdateMethod;
	ic->GetInnerNativeContainerHandle = gtkDialogGetInnerNativeContainerHandleMethod;
	ic->SetChildrenPosition = gtkDialogSetChildrenPositionMethod;
	
	/* Callback Windows and GTK Only */
	iupClassRegisterCallback(ic, "TRAYCLICK_CB", "iii");
	
	/* Driver Dependent Attribute functions */
#ifndef GTK_MAC
#ifdef WIN32
	iupClassRegisterAttribute(ic, "HWND", iupgtkGetNativeWindowHandle, NULL, NULL, NULL, IUPAF_NO_STRING|IUPAF_NO_INHERIT);
#else
	iupClassRegisterAttribute(ic, "XWINDOW", iupgtkGetNativeWindowHandle, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_NO_STRING);
#endif
#endif
	
	/* Visual */
	iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, "DLGBGCOLOR", NULL, IUPAF_DEFAULT);  /* force new default value */
	
	/* Base Container */
	iupClassRegisterAttribute(ic, "CLIENTSIZE", gtkDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);  /* dialog is the only not read-only */
	iupClassRegisterAttribute(ic, "CLIENTOFFSET", gtkDialogGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
#endif
	
	
	/* Special */
	iupClassRegisterAttribute(ic, "TITLE", NULL, emscriptenDialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	
	
#if 0
	/* IupDialog only */
	iupClassRegisterAttribute(ic, "BACKGROUND", NULL, gtkDialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ICON", NULL, gtkDialogSetIconAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "FULLSCREEN", NULL, gtkDialogSetFullScreenAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MINSIZE", NULL, gtkDialogSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MAXSIZE", NULL, gtkDialogSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SAVEUNDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);  /* saveunder not supported in GTK */
	
	/* IupDialog Windows and GTK Only */
	iupClassRegisterAttribute(ic, "ACTIVEWINDOW", gtkDialogGetActiveWindowAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TOPMOST", NULL, gtkDialogSetTopMostAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "DIALOGHINT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
#if GTK_CHECK_VERSION(2, 12, 0)
	iupClassRegisterAttribute(ic, "OPACITY", NULL, gtkDialogSetOpacityAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "OPACITYIMAGE", NULL, gtkDialogSetOpacityImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
#endif
#if GTK_CHECK_VERSION(2, 10, 0)
	iupClassRegisterAttribute(ic, "TRAY", NULL, gtkDialogSetTrayAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TRAYIMAGE", NULL, gtkDialogSetTrayImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TRAYTIP", NULL, gtkDialogSetTrayTipAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TRAYTIPMARKUP", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_MAPPED);
#endif
	
	/* Not Supported */
	iupClassRegisterAttribute(ic, "BRINGFRONT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "COMPOSITED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NOT_MAPPED);
	iupClassRegisterAttribute(ic, "CONTROL", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "HELPBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TOOLBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MDIFRAME", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MDICLIENT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MDIMENU", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MDICHILD", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
#endif
	
}

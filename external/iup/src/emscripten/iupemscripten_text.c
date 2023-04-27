/** \file
 * \brief Text Control
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
#include "iup_mask.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_key.h"
#include "iup_array.h"
#include "iup_text.h"

#include "iupemscripten_drv.h"

typedef enum
{
  IUPEMSCRIPTENTEXTSUBTYPE_UNKNOWN = -1,
  IUPEMSCRIPTENTEXTSUBTYPE_FIELD,
  IUPEMSCRIPTENTEXTSUBTYPE_TEXTAREA,
  IUPEMSCRIPTENTEXTSUBTYPE_STEPPER,
} IupEmscriptenTextSubType;

extern int emjsText_CreateText(IupEmscriptenTextSubType subtype);
extern char* emjsText_GetText(int handle_id);
extern void emjsText_SetText(int handle_id, const char* text);
extern void emjsText_DestroyText(int handle_id);
extern _Bool emjsText_GetReadOnly(int handle_id);
extern void emjsText_SetReadOnly(int handle_id, _Bool is_readonly);

static IupEmscriptenTextSubType emscriptenTextGetSubType(Ihandle* ih)
{
  if(ih->data->is_multiline)
    {
      iupEmscripten_Log("*** Type is textarea ***");
      return IUPEMSCRIPTENTEXTSUBTYPE_TEXTAREA;
    }
	else if(iupAttribGetBoolean(ih, "SPIN"))
    {
      iupEmscripten_Log("*** Type is spin ***");
      return IUPEMSCRIPTENTEXTSUBTYPE_STEPPER;
    }
	else
    {
      iupEmscripten_Log("*** Type is input ***");
      return IUPEMSCRIPTENTEXTSUBTYPE_FIELD;
    }
  iupEmscripten_Log("*** Type is UNKNOWN ***");
	return IUPEMSCRIPTENTEXTSUBTYPE_UNKNOWN;
}

void iupdrvTextAddSpin(Ihandle* ih, int *w, int h)
{

}

void iupdrvTextAddBorders(Ihandle* ih, int *x, int *y)
{

}


void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int *pos)
{

}

void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int *lin, int *col)
{

}



void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
  return NULL;
}

void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state)
{

}

void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
{

}

static char* emscriptenTextGetValueAttrib(Ihandle* ih)
{
	char* value;

	IupEmscriptenTextSubType sub_type = emscriptenTextGetSubType(ih);

	switch(sub_type)
  {
		case IUPEMSCRIPTENTEXTSUBTYPE_TEXTAREA:
    case IUPEMSCRIPTENTEXTSUBTYPE_FIELD:
    {
      // single or multi-line text box
      // call into js, and get the user input from the text widget
      // iupStrReturnStr makes a copy of string and puts into memory that IUP manages
      char* c_str = emjsText_GetText(ih->handle->handleID);
      value = iupStrReturnStr(c_str);
      free(c_str);
      break;
    }
		case IUPEMSCRIPTENTEXTSUBTYPE_STEPPER:
    {
      break;
    }
		default:
    {
      break;
    }
  }

	if (value == NULL)
    {
      value = "";
    }

	return value;
}


static int emscriptenTextSetValueAttrib(Ihandle* ih, const char* value)
{
  if (value == NULL) {
    value = "";
  }

  emjsText_SetText(ih->handle->handleID, value);
  return 0;
}

static char* emscriptenTextGetReadOnlyAttrib(Ihandle* ih)
{
	_Bool is_readonly;

	IupEmscriptenTextSubType sub_type = emscriptenTextGetSubType(ih);
	switch(sub_type)
  {
		case IUPEMSCRIPTENTEXTSUBTYPE_TEXTAREA:
    case IUPEMSCRIPTENTEXTSUBTYPE_FIELD:
    {
      is_readonly = emjsText_GetReadOnly(ih->handle->handleID);
      break;
    }
		case IUPEMSCRIPTENTEXTSUBTYPE_STEPPER:
    {
      break;
    }
		default:
    {
      break;
    }
  }

	return iupStrReturnBoolean(is_readonly);
}

static int emscriptenTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
	_Bool is_readonly = (_Bool)iupStrBoolean(value);

	IupEmscriptenTextSubType sub_type = emscriptenTextGetSubType(ih);
	switch(sub_type)
  {
		case IUPEMSCRIPTENTEXTSUBTYPE_TEXTAREA:
		case IUPEMSCRIPTENTEXTSUBTYPE_FIELD:
    {
      emjsText_SetReadOnly(ih->handle->handleID, is_readonly);
      break;
    }
		case IUPEMSCRIPTENTEXTSUBTYPE_STEPPER:
    {
      break;
    }
		default:
    {
      break;
    }
  }

	return 0;
}

// TODO: just need to do CUEBANNER to complete necessary attributes 

static int emscriptenTextMapMethod(Ihandle* ih)
{
  int text_id = 0;
  InativeHandle* new_handle = NULL;

  IupEmscriptenTextSubType sub_type = emscriptenTextGetSubType(ih);
  text_id = emjsText_CreateText(sub_type);
  new_handle = (InativeHandle*)calloc(1, sizeof(InativeHandle));

  new_handle->handleID = text_id;
  ih->handle = new_handle;

  iupEmscripten_SetIntKeyForIhandleValue(text_id, ih);

  iupEmscripten_AddWidgetToParent(ih);

#if 0
	NSView* the_view;



	if (ih->data->is_multiline)
	{
//		NSTextView* text_view = [[NSTextView alloc] initWithFrame:NSZeroRect];
		NSTextView* text_view = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, 400, 400)];
		the_view = text_view;

		int wordwrap = 0;


		/* formatting is always supported when MULTILINE=YES */
		ih->data->has_formatting = 1;

		if (iupAttribGetBoolean(ih, "WORDWRAP"))
		{
			wordwrap = 1;
			ih->data->sb &= ~IUP_SB_HORIZ;  /* must remove the horizontal scroolbar */
		}
		else
		{
			NSSize layout_size = [text_view maxSize];
			layout_size.width = layout_size.height;
			[text_view setMaxSize:layout_size];
			[[text_view textContainer] setWidthTracksTextView:NO];
			[[text_view textContainer] setContainerSize:layout_size];
		}
	}
	else
	{
		NSTextField* text_field;
		// IMPORTANT: Secure text fields are not togglable in emscripten
		// It might be fakeable, however, since this is security related, mucking with it is ill-advised.
		// Also Mac App Store may reject ill-advised things.
		if(iupAttribGetBoolean(ih, "PASSWORD"))
		{
			//text_field = [[NSSecureTextField alloc] initWithFrame:NSZeroRect];
			text_field = [[NSSecureTextField alloc] initWithFrame:NSMakeRect(0, 0, 140, 40)];
		}
		else
		{
			//text_field = [[NSTextField alloc] initWithFrame:NSZeroRect];
			text_field = [[NSTextField alloc] initWithFrame:NSMakeRect(50, 50, 140, 40)];
		}
		the_view = text_field;


		[text_field setPlaceholderString:@"Placeholder Text"];

		if(iupAttribGetBoolean(ih, "SPIN"))
		{
			// TODO: NSStepper
			/*
			gtk_spin_button_set_numeric((GtkSpinButton*)ih->handle, FALSE);
			gtk_spin_button_set_digits((GtkSpinButton*)ih->handle, 0);

			gtk_spin_button_set_wrap((GtkSpinButton*)ih->handle, iupAttribGetBoolean(ih, "SPINWRAP"));

			g_signal_connect(G_OBJECT(ih->handle), "value-changed", G_CALLBACK(gtkTextSpinValueChanged), ih);
			g_signal_connect(G_OBJECT(ih->handle), "output", G_CALLBACK(gtkTextSpinOutput), ih);

			if (!iupAttribGetBoolean(ih, "SPINAUTO"))
			{
				g_signal_connect(G_OBJECT(ih->handle), "input", G_CALLBACK(gtkTextSpinInput), ih);
				iupAttribSet(ih, "_IUPGTK_SPIN_NOAUTO", "1");
			}
			 */
		}
		else
		{
		}

		/* formatting is never supported when MULTILINE=NO */
		ih->data->has_formatting = 0;
//		[text_field sizeToFit];


	}

	ih->handle = the_view;

#if 0
	// I'm using objc_setAssociatedObject/objc_getAssociatedObject because it allows me to avoid making subclasses just to hold ivars.
	objc_setAssociatedObject(the_toggle, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
	// I also need to track the memory of the buttion action receiver.
	// I prefer to keep the Ihandle the actual NSView instead of the receiver because it makes the rest of the implementation easier if the handle is always an NSView (or very small set of things, e.g. NSWindow, NSView, CALayer).
	// So with only one pointer to deal with, this means we need our Toggle to hold a reference to the receiver object.
	// This is generally not good emscripten as Toggles don't retain their receivers, but this seems like the best option.
	// Be careful of retain cycles.
	IupemscriptenToggleReceiver* toggle_receiver = [[IupemscriptenToggleReceiver alloc] init];
	[the_toggle setTarget:toggle_receiver];
	[the_toggle setAction:@selector(myToggleClickAction:)];
	// I *think* is we use RETAIN, the object will be released automatically when the Toggle is freed.
	// However, the fact that this is tricky and I had to look up the rules (not to mention worrying about retain cycles)
	// makes me think I should just explicitly manage the memory so everybody is aware of what's going on.
	objc_setAssociatedObject(the_toggle, IUP_emscripten_TOGGLE_RECEIVER_OBJ_KEY, (id)toggle_receiver, OBJC_ASSOCIATION_ASSIGN);

#endif
	// All emscripten views shoud call this to add the new view to the parent view.
	iupemscriptenAddToParent(ih);

#if 0
	/* configure for DRAG&DROP */
	if (IupGetCallback(ih, "DROPFILES_CB"))
		iupAttribSet(ih, "DROPFILESTARGET", "YES");

	/* update a mnemonic in a label if necessary */
	iupgtkUpdateMnemonic(ih);

	if (ih->data->formattags)
		iupTextUpdateFormatTags(ih);
#endif

#endif

	return IUP_NOERROR;
}


static void emscriptenTextUnMapMethod(Ihandle* ih)
{

	if (ih && ih->handle) {
    iupEmscripten_RemoveIntKeyFromIhandleMap(ih->handle->handleID);
    emjsText_DestroyText(ih->handle->handleID);
    free(ih->handle);
    ih->handle = NULL;
  }

#if 0
	id the_view = ih->handle;
	/*
	id text_receiver = objc_getAssociatedObject(the_view, IUP_emscripten_TOGGLE_RECEIVER_OBJ_KEY);
	objc_setAssociatedObject(the_view, IUP_emscripten_TOGGLE_RECEIVER_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
	[text_receiver release];
	*/
	[the_view release];
	ih->handle = NULL;
#endif

}


void iupdrvTextInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = emscriptenTextMapMethod;
  ic->UnMap = emscriptenTextUnMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", emscriptenTextGetValueAttrib, emscriptenTextSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", emscriptenTextGetReadOnlyAttrib, emscriptenTextSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  // still need to implement CUEBANNER
  /* iupClassRegisterAttribute(ic, "CUEBANNER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT); */

#if 0
  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtkTextSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  /* IupText only */
  iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, gtkTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "LINEVALUE", gtkTextGetLineValueAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", gtkTextGetSelectedTextAttrib, gtkTextSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", gtkTextGetSelectionAttrib, gtkTextSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", gtkTextGetSelectionPosAttrib, gtkTextSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", gtkTextGetCaretAttrib, gtkTextSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", gtkTextGetCaretPosAttrib, gtkTextSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, gtkTextSetInsertAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, gtkTextSetAppendAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, gtkTextSetNCAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, gtkTextSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, gtkTextSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, gtkTextSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMIN", NULL, gtkTextSetSpinMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, gtkTextSetSpinMaxAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, gtkTextSetSpinIncAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", gtkTextGetSpinValueAttrib, gtkTextSetSpinValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", gtkTextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINECOUNT", gtkTextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* IupText Windows and GTK only */
  iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iupTextSetAddFormatTagAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iupTextSetAddFormatTagHandleAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, gtkTextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", iupTextGetFormattingAttrib, iupTextSetFormattingAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", gtkTextGetOverwriteAttrib, gtkTextSetOverwriteAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REMOVEFORMATTING", NULL, gtkTextSetRemoveFormattingAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSIZE", NULL, gtkTextSetTabSizeAttrib, "8", NULL, IUPAF_DEFAULT);  /* force new default value */
  iupClassRegisterAttribute(ic, "PASSWORD", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* Not Supported */
  iupClassRegisterAttribute(ic, "CUEBANNER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
#endif

}

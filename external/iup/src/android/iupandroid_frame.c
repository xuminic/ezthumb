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

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_frame.h"

#include "iupandroid_drv.h"



static int androidFrameMapMethod(Ihandle* ih)
{

#if 0
//	NSBox* the_frame = [[NSBox alloc] initWithFrame:NSZeroRect];
	NSBox* the_frame = [[NSBox alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];

	{
		char* title;
		title = iupAttribGet(ih, "TITLE");
		if(title && *title!=0)
		{
			NSString* ns_string = [NSString stringWithUTF8String:title];
			[the_frame setTitle:ns_string];
		}
	}
	
	
	
	
	ih->handle = the_frame;
	
	
	
	iupandroidAddToParent(ih);
	
#endif

	
	return IUP_NOERROR;
}


static void androidFrameUnMapMethod(Ihandle* ih)
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
	ic->Map = androidFrameMapMethod;
	ic->UnMap = androidFrameUnMapMethod;
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

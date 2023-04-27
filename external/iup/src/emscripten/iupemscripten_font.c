/** \file
 * \brief MAC Font mapping
 *
 * See Copyright Notice in "iup.h"
 */


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


#include "iup.h"

#include "iup_str.h"
#include "iup_array.h"
#include "iup_attrib.h"
#include "iup_object.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_assert.h"

#include "iupemscripten_drv.h"

//#include "iupmac_info.h"

extern int emjsFont_GetStringWidth(Ihandle* ih, int handleID, const char* str);
extern void emjsFont_GetMultiLineStringSize(Ihandle* ih, int handleID, const char* str, int32_t* out_ptr_width, int32_t* out_ptr_height);
extern void emjsFont_GetTextSize(const char* font_name, int point_size, int is_bold, int is_italic, int is_underline, int is_strikeout, const char* str, int32_t* out_ptr_width, int32_t* out_ptr_height);
extern void emjsFont_GetCharSize(Ihandle* ih, int handleID, int32_t* out_ptr_width, int32_t* out_ptr_height);

IUP_SDK_API char* iupdrvGetSystemFont(void)
{
  static char systemfont[200] = "";
#if 0
  NSFont *font = [NSFont systemFontOfSize:0];
	NSLog(@"systemfont: %@", font);
  char *name = [[font familyName] UTF8String];
  if(*name)
    strcpy(systemfont,name);
  else
    strcpy(systemfont, "Tahoma, 10");
#endif
  return systemfont;
}


IUP_SDK_API int iupdrvSetStandardFontAttrib(Ihandle* ih, const char* value)
{
#if 0 // iupBaseUpdateSizeFromFont missing
  ImacFont* macfont = macFontCreateNativeFont(ih, value);
  if (!macfont)
    return 1;

	/* If FONT is changed, must update the SIZE attribute */
	iupBaseUpdateAttribFromFont(ih);

  /* FONT attribute must be able to be set before mapping, 
      so the font is enable for size calculation. */
  if (ih->handle && (ih->iclass->nativetype != IUP_TYPEVOID)) {
	
  }
#endif
  return 1;
}

// three functions below are used to compute string size:

IUP_SDK_API void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int *w, int *h)
{
	int32_t tmp_width = 0;
	int32_t tmp_height = 0;
	int handle_id = 0;

	if(ih && ih->handle)
	{
		handle_id = ih->handle->handleID;
	}


	emjsFont_GetMultiLineStringSize(ih, handle_id, str, &tmp_width, &tmp_height);
	if(w)
	{
		*w = (int)tmp_width;
	}
	if(h)
	{ 
		*h = (int)tmp_height;
	}

	iupEmscripten_Log("iupdrvFontGetMultiLineStringSize being called.  Here's an arg: %s", str);
	return;
}

// FIXME: This is a quick-and-dirty copy-and-paste from emjsFont_GetMultiLineStringSize to get things working due to Iup internal API changes.
IUP_SDK_API void iupdrvFontGetTextSize(const char* font, const char* str, int len, int *w, int *h)
{
	int32_t tmp_width = 0;
	int32_t tmp_height = 0;

	char typeface[50] = "";
	int point_size = 8;
	int is_bold = 0;
	int is_italic = 0;
	int is_underline = 0;
	int is_strikeout = 0;

	iupEmscripten_Log("iupdrvFontGetTextSize being called: font: %s, str: %s", font, str);

	if (!iupGetFontInfo(font, typeface, &point_size, &is_bold, &is_italic, &is_underline, &is_strikeout))
	{
		// I'm told this function does nothing if the font doesn't exist.
		return;
	}


	emjsFont_GetTextSize(typeface, point_size, is_bold, is_italic, is_underline, is_strikeout, str, &tmp_width, &tmp_height);
	if(w)
	{
		*w = (int)tmp_width;
	}
	if(h)
	{ 
		*h = (int)tmp_height;
	}
	iupEmscripten_Log("iupdrvFontGetTextSize w:%d, h:%d", tmp_width, tmp_height);

	return;

}

IUP_SDK_API int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
	int handle_id = 0;
	if(ih && ih->handle)
	{
		handle_id = ih->handle->handleID;
	}
	return emjsFont_GetStringWidth(ih, handle_id, str);
}

IUP_SDK_API void iupdrvFontGetCharSize(Ihandle* ih, int *charwidth, int *charheight)
{
	int32_t tmp_width = 0;
	int32_t tmp_height = 0;
	int handle_id = 0;
	if(ih && ih->handle)
	{
		handle_id = ih->handle->handleID;
	}

	emjsFont_GetCharSize(ih, handle_id, &tmp_width, &tmp_height);

	if(charwidth)
	{
		*charwidth = (int)tmp_width;
	}
	if(charheight)
	{ 
		*charheight = (int)tmp_height;
	}
}

void iupdrvFontInit(void)
{
}

void iupdrvFontFinish(void)
{
}

IUP_SDK_API int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
  return 1;
}

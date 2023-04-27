/** \file
 * \brief MAC Driver iupdrvSetGlobal
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_strmessage.h"

#include "iupemscripten_drv.h"



IUP_SDK_API int iupdrvSetGlobal(const char *name, const char *value)
{
  if (iupStrEqual(name, "LANGUAGE"))
  {
    iupStrMessageUpdateLanguage(value);
    return 1;
  }
  if (iupStrEqual(name, "CURSORPOS"))
  {
/*	  
    int x, y;
    if (iupStrToIntInt(value, &x, &y, 'x') == 2) {
	  CGEventRef event=CGEventCreateMouseEvent(NULL,kCGEventMouseMoved,CGPointMake(x,y),0);
	  CGEventPost(kCGSessionEventTap, event);
	  CFRelease(event);
	}
*/	
    return 0;
  }
  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
/*
    if (!value || iupStrBoolean(value))
      iupmac_utf8autoconvert = 1;
    else
      iupmac_utf8autoconvert = 0;
*/
  	  return 0;
  }
  if (iupStrEqual(name, "KEYPRESS"))
  {
/*	  
    int key;
    if (iupStrToInt(value, &key))
      macGlobalSendKey(key, 0x01);
*/
    return 0;
  }
  if (iupStrEqual(name, "KEYRELEASE"))
  {
/*
    int key;
    if (iupStrToInt(value, &key))
      macGlobalSendKey(key, 0x02);
*/
    return 0;
  }
  if (iupStrEqual(name, "KEY"))
  {
/*
    int key;
    if (iupStrToInt(value, &key))
      macGlobalSendKey(key, 0x03);
*/
    return 0;
  }
  return 1;
}

IUP_SDK_API char *iupdrvGetGlobal(const char *name)
{
#if 0
  if (iupStrEqual(name, "CURSORPOS"))
  {
    char *str = iupStrGetMemory(50);
    int x, y;
    iupdrvGetCursorPos(&x, &y);
    sprintf(str, "%dx%d", (int)x, (int)y);
    return str;
  }
  if (iupStrEqual(name, "SHIFTKEY"))
  {
    char key[5];
    iupdrvGetKeyState(key);
    if (key[0] == 'S')
      return "ON";
    else
      return "OFF";
  }
  if (iupStrEqual(name, "CONTROLKEY"))
  {
    char key[5];
    iupdrvGetKeyState(key);
    if (key[1] == 'C')
      return "ON";
    else
      return "OFF";
  }
  if (iupStrEqual(name, "MODKEYSTATE"))
  {
    char *str = iupStrGetMemory(5);
    iupdrvGetKeyState(str);
    return str;
  }
  if (iupStrEqual(name, "SCREENSIZE"))
  {
    char *str = iupStrGetMemory(50);
    int w, h;
    iupdrvGetScreenSize(&w, &h);
    sprintf(str, "%dx%d", w, h);
    return str;
  }
  if (iupStrEqual(name, "FULLSIZE"))
  {
    char *str = iupStrGetMemory(50);
    int w, h;
    iupdrvGetFullSize(&w, &h);
    sprintf(str, "%dx%d", w, h);
    return str;
  }
  if (iupStrEqual(name, "SCREENDEPTH"))
  {
    char *str = iupStrGetMemory(50);
    int bpp = iupdrvGetScreenDepth();
    sprintf(str, "%d", bpp);
    return str;
  }
  if (iupStrEqual(name, "VIRTUALSCREEN"))
  {
    char *str = iupStrGetMemory(50);
	int x=0,y=0,w,h;
    iupdrvGetFullSize(&w, &h);
    sprintf(str, "%d %d %d %d", x, y, w, h);
    return str;
  }
  if (iupStrEqual(name, "MONITORSINFO"))
  {
    int i;
	NSArray* arr = [NSScreen screens];
	int monitors_count = [arr count];
    char *str = iupStrGetMemory(monitors_count*50);
    char* pstr = str;
	NSRect frame;

	  for(NSScreen* screen in arr)
	  {
      frame = [screen frame];
      pstr += sprintf(pstr, "%d %d %d %d\n", frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
    }

    return str;
  }
  if (iupStrEqual(name, "TRUECOLORCANVAS"))
  {
    if (iupdrvGetScreenDepth() > 8)
      return "YES";
    else
      return "NO";
  }
  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
    if (iupmac_utf8autoconvert)
      return "YES";
    else
      return "NO";
  }
#endif
  return NULL;
}

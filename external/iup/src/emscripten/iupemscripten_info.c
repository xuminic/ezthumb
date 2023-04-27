/** \file
 * \brief MAC OS System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <unistd.h>
#include <limits.h>
#include <errno.h>

//#include <gtk/gtk.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"


IUP_SDK_API void iupdrvAddScreenOffset(int *x, int *y, int add)
{
	/* ?????? */
}

// How is this different than iupdrvGetFullSize? Is this supposed to subtract the menu and dock?
IUP_SDK_API void iupdrvGetScreenSize(int *width, int *height)
{

//	NSRect screen_rect = [[NSScreen mainScreen] visibleFrame];
	
	// dp and sp in Emscripten

	//  int w_size = CGDisplayPixelsWide(kCGDirectMainDisplay);
	//  int h_size = CGDisplayPixelsHigh(kCGDirectMainDisplay);
//	if (width) *width = screen_rect.size.width;
//	if (height) *height = screen_rect.size.height;
	
}

IUP_SDK_API void iupdrvGetFullSize(int *width, int *height)
{
#if 0
	NSRect screen_rect = [[NSScreen mainScreen] frame];
	
	// Points vs. Pixels in emscripten
	//  int w_size = CGDisplayPixelsWide(kCGDirectMainDisplay);
	//  int h_size = CGDisplayPixelsHigh(kCGDirectMainDisplay);
	if (width) *width = screen_rect.size.width;
	if (height) *height = screen_rect.size.height;
#endif
}

IUP_SDK_API int iupdrvGetScreenDepth(void)
{
//	return CGDisplayBitsPerPixel(kCGDirectMainDisplay);  /* Deprecated in Mac OS X v10.6 */
	return 32;
}

IUP_SDK_API double iupdrvGetScreenDpi(void)
{
#if 0
	CGRect rect = CGDisplayBounds(kCGDirectMainDisplay);
	int height = (int)CGRectGetHeight(rect);   /* pixels */
	CGSize size = CGDisplayScreenSize(kCGDirectMainDisplay);  /* millimeters */
	return ((float)height / size.height) * 25.4f;  /* mm to inch */
#endif
}

IUP_SDK_API void iupdrvGetCursorPos(int *x, int *y)
{
#if 0
	CGPoint point;
#ifdef OLD_MAC_INFO
	Point pnt;
	GetMouse(&pnt);
	point = CGPointMake(pnt.h, pnt.v);
#else
	HIGetMousePosition(kHICoordSpaceScreenPixel, NULL, &point);
#endif
	
	*x = (int)point.x;
	*y = (int)point.y;
#endif
}

IUP_SDK_API void iupdrvGetKeyState(char* key)
{
#if 0
	if (GetCurrentEventKeyModifiers() & shiftKey)
		key[0] = 'S';
	else
		key[0] = ' ';
	if (GetCurrentEventKeyModifiers() & controlKey)
		key[1] = 'C';
	else
		key[1] = ' ';
	if (GetCurrentEventKeyModifiers() & optionKey)
		key[2] = 'A';
	else
		key[2] = ' ';
	if (GetCurrentEventKeyModifiers() & cmdKey)
		key[3] = 'Y';
	else
		key[3] = ' ';
	
	key[4] = 0;
#endif
	
}

IUP_SDK_API char *iupdrvGetSystemName(void)
{

	return "Emscripten";
}

IUP_SDK_API char *iupdrvGetSystemVersion(void)
{
	
#if 0
	char* str = iupStrGetMemory(100);
	SInt32 systemVersion, versionMajor, versionMinor, versionBugFix, systemArchitecture;
	
	if (Gestalt(gestaltSystemVersion, &systemVersion) != noErr)
		return NULL;
	
	if (systemVersion < 0x1040)
	{
		/* Major, Minor, Bug fix */
		sprintf(str, "%ld.%ld.%ld", (((long)systemVersion & 0xF000) >> 12) * 10 + (((long)systemVersion & 0x0F00) >> 8),
				(((long)systemVersion & 0x00F0) >> 4), ((long)systemVersion & 0x000F));
	}
	else  /* MAC_OS_X_VERSION_10_4 or later */
	{
		Gestalt(gestaltSystemVersionMajor,  &versionMajor);
		Gestalt(gestaltSystemVersionMinor,  &versionMinor);
		Gestalt(gestaltSystemVersionBugFix, &versionBugFix);
		
		sprintf(str, "%ld.%ld.%ld", (long)versionMajor, (long)versionMinor, (long)versionBugFix);
	}
	
	if (Gestalt(gestaltSysArchitecture, &systemArchitecture) == noErr)
	{
		if (systemArchitecture == gestalt68k)
			strcat(str, " (Motorola 68k)");
		else if (systemArchitecture == gestaltPowerPC)
			strcat(str, " (Power PC)");
		else /* gestaltIntel */
			strcat(str, " (Intel)");
	}
	
	return str;
#else
	
/*
	NSString* version_string = nil;
	version_string = [[NSProcessInfo processInfo] operatingSystemVersionString];
	
	const char* c_str = [version_string UTF8String];
	// don't use [version_string length]...counts characters, not bytes
	size_t str_len = strlen(c_str);
	
	char* iup_str = iupStrGetMemory((int)str_len);
	strlcpy(iup_str, c_str, str_len+1);
	
	return iup_str;
*/
	return NULL;
#endif

}

IUP_SDK_API char *iupdrvGetComputerName(void)
{
	// Emscripten doesn't give a computer name. This is also a problem for protocols like Zeroconf.
	// TODO: Use my solution for Zeroconf here.
	
	return NULL;
}

IUP_SDK_API char *iupdrvGetUserName(void)
{

	return NULL;
}

IUP_SDK_API int iupdrvGetPreferencePath(char *filename, int use_system)
{
  /*
  Everything is supposed to be sandboxed.
  getenv is virtualized and getenv("HOME") isn't guaranteed to be defined.
  But we should be able to write anywhere in our sandbox.
  */
  /* Emscripten virtualizes everything which means this data is not persistent.
  A browser reload will lose all data.
  There is a thing called IDBFS which writes to a persistent IndexedDB.
  We mount an IDBFS in iupdrvOpen(). Make sure the /IupIDBFS directory is consistent in both parts (if you change the path).
  */

  /* We put this in a unique Iup directory to try to not conflict with anything the end user might want to setup for their own program. */
  size_t str_len = strlen(filename);
  size_t num = strlcpy(filename, "/IupIDBFS/", str_len);
  if (num >= str_len)
  {
    filename[0] = '\0';
    return 0;
  }
  return 1;
}


IUP_SDK_API char* iupdrvLocaleInfo(void)
{
	//return iupStrReturnStr(nl_langinfo(CODESET));
	return NULL;
}

IUP_SDK_API char* iupdrvGetCurrentDirectory(void)
{
	return NULL;
}

IUP_SDK_API int iupdrvSetCurrentDirectory(const char* dir)
{
	return 0;
}

IUP_SDK_API int iupdrvMakeDirectory(const char* name)
{
	return 0;
}

IUP_SDK_API int iupdrvIsFile(const char* name)
{
	return 0;
}

IUP_SDK_API int iupdrvIsDirectory(const char* name)
{
	return 0;
}

IUP_SDK_API int iupdrvGetWindowDecor(void* wnd, int *border, int *caption)
{
	*border = 0;
	*caption = 0;
	return 0;
}


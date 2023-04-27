/** \file
 * \brief Android System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include <jni.h>
#include <android/log.h>
#include <stdarg.h>

#include "iup.h"
#include "iup_varg.h"

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iupandroid_drv.h"
#include <android/log.h>


IUP_SDK_API void iupdrvAddScreenOffset(int *x, int *y, int add)
{
	/* ?????? */
}

// How is this different than iupdrvGetFullSize? Is this supposed to subtract the menu and dock?
IUP_SDK_API void iupdrvGetScreenSize(int *width, int *height)
{
	__android_log_print(ANDROID_LOG_ERROR, "iupdrvGetScreenSize", "FIXME: Not properly implemented");

//	NSRect screen_rect = [[NSScreen mainScreen] visibleFrame];
	
	// dp and sp in Android

	//  int w_size = CGDisplayPixelsWide(kCGDirectMainDisplay);
	//  int h_size = CGDisplayPixelsHigh(kCGDirectMainDisplay);
//	if (width) *width = screen_rect.size.width;
//	if (height) *height = screen_rect.size.height;


	// What do we do about device orientation?

	if (width) *width = 1024;
	if (height) *height = 1920;
}

IUP_SDK_API void iupdrvGetFullSize(int *width, int *height)
{
	__android_log_print(ANDROID_LOG_ERROR, "iupdrvGetFullSize", "FIXME: Not properly implemented");

	#if 0
	NSRect screen_rect = [[NSScreen mainScreen] frame];
	
	// Points vs. Pixels in android
	//  int w_size = CGDisplayPixelsWide(kCGDirectMainDisplay);
	//  int h_size = CGDisplayPixelsHigh(kCGDirectMainDisplay);
	if (width) *width = screen_rect.size.width;
	if (height) *height = screen_rect.size.height;
	#endif


	// What do we do about device orientation?

	if (width) *width = 1024;
	if (height) *height = 1920;
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

	return "Android";
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
	// Android doesn't give a computer name. This is also a problem for protocols like Zeroconf.
	// TODO: Use my solution for Zeroconf here.
	
	return NULL;
}

IUP_SDK_API char *iupdrvGetUserName(void)
{

	return NULL;

	
}

IUP_SDK_API int iupdrvGetPreferencePath(char *filename, int use_system)
{
	JNIEnv* jni_env;
    jmethodID method_id;
	jclass java_class;
	/* aka context object */
	jobject context_object;
	jobject file_object;
	jstring j_path_string;
	const char* c_path_string = NULL;
	size_t num;
	char path_separator[] = "/";

	filename[0] = '\0';

	jni_env = iupAndroid_GetEnvThreadSafe();

	/* the Application object is a context */
	context_object = iupAndroid_GetApplication(jni_env);

	java_class = (*jni_env)->GetObjectClass(jni_env, context_object);

	/* file_object = context.getFilesDir(); */
	method_id = (*jni_env)->GetMethodID(jni_env, java_class, "getFilesDir", "()Ljava/io/File;");
	file_object = (*jni_env)->CallObjectMethod(jni_env, context_object, method_id);
	if(NULL == file_object)
	{
		__android_log_print(ANDROID_LOG_ERROR, "iupAndroid", "iupdrvGetPreferencePath context.getFilesDir() failed"); 
		(*jni_env)->DeleteLocalRef(jni_env, file_object);
		(*jni_env)->DeleteLocalRef(jni_env, java_class);
		(*jni_env)->DeleteLocalRef(jni_env, context_object);

		return 0;
	}

	(*jni_env)->DeleteLocalRef(jni_env, java_class);
	java_class = (*jni_env)->GetObjectClass(jni_env, file_object);
	/* path = file_object.getAbsolutePath(); */
	method_id = (*jni_env)->GetMethodID(jni_env, java_class, "getAbsolutePath", "()Ljava/lang/String;");
	j_path_string = (jstring)(*jni_env)->CallObjectMethod(jni_env, file_object, method_id);

	c_path_string = (*jni_env)->GetStringUTFChars(jni_env, j_path_string, NULL);

	size_t str_len = strlen(filename);
	num = strlcpy(filename, c_path_string, str_len);

	(*jni_env)->ReleaseStringUTFChars(jni_env, j_path_string, c_path_string);
	(*jni_env)->DeleteLocalRef(jni_env, file_object);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);
	(*jni_env)->DeleteLocalRef(jni_env, context_object);

    if(num >= str_len)
    {
      filename[0] = '\0';
      return 0;
	}
	/* Don't forget to end in trailing '\' */
    num = strlcat(filename, path_separator, str_len);
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


IUP_API void IupLogV(const char* type, const char* format, va_list arglist)
{
	int priority = ANDROID_LOG_DEFAULT;
	/*
	typedef enum android_LogPriority {
		ANDROID_LOG_UNKNOWN = 0,
		ANDROID_LOG_DEFAULT,    // only for SetMinPriority()
		ANDROID_LOG_VERBOSE,
		ANDROID_LOG_DEBUG,
		ANDROID_LOG_INFO,
		ANDROID_LOG_WARN,
		ANDROID_LOG_ERROR,
		ANDROID_LOG_FATAL,
		ANDROID_LOG_SILENT,     // only for SetMinPriority(); must be last
	} android_LogPriority;
	*/

	if (iupStrEqualNoCase(type, "DEBUG"))
	{
		priority = ANDROID_LOG_DEBUG;
	}
	else if (iupStrEqualNoCase(type, "ERROR"))
	{
		priority = ANDROID_LOG_ERROR;
	}
	else if (iupStrEqualNoCase(type, "WARNING"))
	{
		priority = ANDROID_LOG_WARN;
	}
	else if (iupStrEqualNoCase(type, "INFO"))
	{
		priority = ANDROID_LOG_INFO;
	}

	// Extras: (not officially documented)
	else if (iupStrEqualNoCase(type, "FATAL"))
	{
		priority = ANDROID_LOG_FATAL;
	}
	else if (iupStrEqualNoCase(type, "SILENT"))
	{
		priority = ANDROID_LOG_SILENT;
	}
	else if (iupStrEqualNoCase(type, "UNKNOWN"))
	{
		priority = ANDROID_LOG_UNKNOWN;
	}
	else if (iupStrEqualNoCase(type, "DEFAULT"))
	{
		priority = ANDROID_LOG_DEFAULT;
	}
	else if (iupStrEqualNoCase(type, "VERBOSE"))
	{
		priority = ANDROID_LOG_VERBOSE;
	}

	__android_log_vprint(priority, "IupLog", format, arglist);
}

IUP_API void IupLog(const char* type, const char* format, ...)
{
	va_list arglist;
	va_start(arglist, format);
	IupLogV(type, format, arglist);
	va_end(arglist);
}



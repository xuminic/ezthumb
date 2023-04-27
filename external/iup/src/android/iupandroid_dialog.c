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

#include "iupandroid_drv.h"
#include <jni.h>
#include <android/log.h>
#include "iupandroid_jnimacros.h"
#include "iupandroid_jnicacheglobals.h"

IUPJNI_DECLARE_CLASS_STATIC(IupActivity);


// REMOVE these
#define TEMP_HARDCODED_WIDTH 1024
#define TEMP_HARDCODED_HEIGHT 1920

#if 0
/*
@interface NSWindow () 
@property(readwrite, unsafe_unretained) Ihandle* iupIhandle;
@end

@implementation NSWindow
@synthesize iupIhandle = _iupIhandle;
@end
 */
@interface IupAndroidWindowDelegate : NSObject <NSWindowDelegate>
- (BOOL) windowShouldClose:(id)the_sender;
- (NSSize) windowWillResize:(NSWindow*)the_sender toSize:(NSSize)frame_size;
@end

static void androidCleanUpWindow(Ihandle* ih)
{
	NSWindow* the_window = (__bridge NSWindow*)ih->handle;
	[the_window close];
	
	IupAndroidWindowDelegate* window_delegate = [the_window delegate];
	[the_window setDelegate:nil];
	[window_delegate release];
	
	[the_window release];
}



@implementation IupAndroidWindowDelegate

- (BOOL) windowShouldClose:(id)the_sender
{
	// I'm using objc_setAssociatedObject/objc_getAssociatedObject because it allows me to avoid making subclasses just to hold ivars. And category extension isn't working for some reason...NSWindow might be too big/complicated and is expecting me to define Apple stuff.
	
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_sender, IHANDLE_ASSOCIATED_OBJ_KEY);
	
	/* even when ACTIVE=NO the dialog gets this evt */
#if 0
	if (!iupdrvIsActive(ih)) // not implemented yet
	{
		return YES;
	}
#endif
	
	Icallback callback_function = IupGetCallback(ih, "CLOSE_CB");
	if(callback_function)
	{
		int ret = callback_function(ih);
		if (ret == IUP_IGNORE)
		{
			return NO;
		}
		if (ret == IUP_CLOSE)
		{
			IupExitLoop();
		}
	}

	// I think??? we need to hide and not destroy because the user is supposed to call IupDestroy explicitly
	IupHide(ih); /* default: close the window */

//	IupDestroy(ih);
	
	return YES; /* do not propagate */
	
}

- (NSSize) windowWillResize:(NSWindow*)the_sender toSize:(NSSize)frame_size
{
	// I'm using objc_setAssociatedObject/objc_getAssociatedObject because it allows me to avoid making subclasses just to hold ivars. And category extension isn't working for some reason...NSWindow might be too big/complicated and is expecting me to define Apple stuff.
	
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_sender, IHANDLE_ASSOCIATED_OBJ_KEY);
	
	/* even when ACTIVE=NO the dialog gets this evt */
#if 0
	if (!iupdrvIsActive(ih)) // not implemented yet
	{
		return YES;
	}
#endif
	
//	iupdrvDialogGetSize(ih, NULL, &(ih->currentwidth), &(ih->currentheight));

	ih->currentwidth = frame_size.width;
	ih->currentheight = frame_size.height;
	
	return frame_size;
	
}


@end
#endif

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
	__android_log_print(ANDROID_LOG_ERROR, "iupdrvDialogGetSize", "iupdrvDialogGetSize is hardcoded");

	if (w) *w = TEMP_HARDCODED_WIDTH;
	if (h) *h = TEMP_HARDCODED_HEIGHT;
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
	//if (y) *y = iupAndroidComputeIupScreenHeightFromCartesian(the_rect.origin.y);
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

static char* androidDialogGetClientSizeAttrib(Ihandle *ih)
{
//	int width, height;

	__android_log_print(ANDROID_LOG_ERROR, "androidDialogGetClientSizeAttrib", "FIXME: size is hardcoded");

	return iupStrReturnIntInt(TEMP_HARDCODED_WIDTH, TEMP_HARDCODED_HEIGHT, 'x');
}

static int androidDialogSetMinSizeAttrib(Ihandle* ih, const char* value)
{
	__android_log_print(ANDROID_LOG_ERROR, "androidDialogSetMinSizeAttrib", "FIXME: size is hardcoded");

	int min_w = TEMP_HARDCODED_WIDTH, min_h = TEMP_HARDCODED_HEIGHT;          /* MINSIZE default value */
	iupStrToIntInt(value, &min_w, &min_h, 'x');

	return iupBaseSetMinSizeAttrib(ih, value);
}

static int androidDialogSetMaxSizeAttrib(Ihandle* ih, const char* value)
{
	__android_log_print(ANDROID_LOG_ERROR, "androidDialogSetMaxSizeAttrib", "FIXME: size is hardcoded");

	int max_w = TEMP_HARDCODED_WIDTH, max_h = TEMP_HARDCODED_HEIGHT;  /* MAXSIZE default value */
	iupStrToIntInt(value, &max_w, &max_h, 'x');


	return iupBaseSetMaxSizeAttrib(ih, value);
}


/****************************************************************
 Callbacks and Events
 ****************************************************************/

static int androidDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
	
	return 1;
}

static int androidDialogMapMethod(Ihandle* ih)
{
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupActivity_createActivity);
    JNIEnv* jni_env;
	jclass java_class;
    jmethodID method_id;
	char* result_string = NULL;
	jstring new_activity;

	jobject current_activity;

	jobject view_group;
		__android_log_print(ANDROID_LOG_INFO, "androidDialogMapMethod", "entered"); 

	
	jni_env = iupAndroid_GetEnvThreadSafe();

	current_activity = iupAndroid_GetCurrentActivity(jni_env);
	if(NULL == current_activity)
	{
		__android_log_print(ANDROID_LOG_ERROR, "androidDialogMapMethod", "FAILURE: current_activity is NULL. Skipping call. No dialog will be created."); 
		return IUP_ERROR;
	}
		__android_log_print(ANDROID_LOG_INFO, "androidDialogMapMethod", "current_activity: %p", current_activity); 

	java_class = IUPJNI_FindClass(IupActivity, jni_env, "br/pucrio/tecgraf/iup/IupActivity");
	method_id = IUPJNI_GetStaticMethodID(IupActivity_createActivity, jni_env, java_class, "createActivity", "(Landroid/app/Activity;J)Landroid/view/ViewGroup;");
	view_group = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, current_activity, (jlong)(intptr_t)ih);
		__android_log_print(ANDROID_LOG_INFO, "androidDialogMapMethod", "view_group: %p", view_group); 

	// Unforuntately, Android doesn't give us back the Activity object immediately.
	// We can only get the object after the onCreate() method is invoked for the Activity.
	// So we need to set the ih->handle in that callback event.
	// See Java_br_pucrio_tecgraf_iup_IupActivity_SetIhandle.
	// But unfortunately, Iup isn't designed to handle this possibility.
	// If I do not provide something for the handle now, 
	// Iup will skip the map process for all the widgets that go in the Dialog.
	// So I have a overly clever workaround.
	// I create a detached ViewGroup and use it in the Activity's place.
	// The ViewGroup will still allow me to add widgets to it.
	// Then when the onCreate() finally gets invoked, I can swap out the pointers.
	// I will attach the ViewGroup as the contentView for the activity and all will be as expected.
	// I also get to do something extra slippery here and pass a non-serialable object
	// from this activity to the new one. 
	// Normally, you use Intents (which I am using to pass the Ihandle pointer).
	// But ViewGroup is not serializable.
	// But using C/JNI allows me to circumvent this restriction and pass the object.

	// Keep a strong reference to the ViewGroup to keep it alive until our Activity is ready.
	// (Remember to release this reference once we attach it to the new Activity).
	ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, view_group));
	
	// Optional: Free up the temporaries.
	(*jni_env)->DeleteLocalRef(jni_env, view_group);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);
	(*jni_env)->DeleteLocalRef(jni_env, current_activity);


//	iupAttribSet(ih, "RASTERSIZE", "100x100");
	//	iupAttribSet(ih, "RASTERSIZE", "500x400");
	iupAttribSet(ih, "RASTERSIZE", "1024x1920");


	// This should be scrutinized:
	// I don't know if I want GlobalRef the Android Activity here.
	// The end user controls the life-cycle (e.g. back button).
	// If I keep a strong reference, this might prevent the activity from being cleaned up?
	// Maybe the close callback would allow me to free it.
	// I also don't know what IupDestroy(dialog) means for an Activity.
	// Update1: onDestroy gets called correctly even with GlobalRef, so I can call DeleteRef safely. This pattern works.
	__android_log_print(ANDROID_LOG_ERROR, "androidDialogMapMethod", "hardcoding size (width, height)");

	ih->currentwidth = TEMP_HARDCODED_WIDTH;
	ih->currentheight = TEMP_HARDCODED_HEIGHT;
	__android_log_print(ANDROID_LOG_INFO, "androidDialogMapMethod", "end");

	return IUP_NOERROR;

}

static void androidDialogUnMapMethod(Ihandle* ih)
{
	__android_log_print(ANDROID_LOG_ERROR, "Iup", "androidDialogUnMapMethod");
	if(ih && ih->handle)
	{
		IUPJNI_DECLARE_METHOD_ID_STATIC(IupActivity_unMapActivity);
		jclass java_class;
		jmethodID method_id;
		JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();

		// Any extra cleanup I need to do in Java is in unMapActivity.
		// I'm not sure if I need to explicitly call finish() for the case where 
		// the user explictly calls IupDestroy() before the Activity is popped.
		// Also, because of the ViewGroup dance I do in Map, I need to check the type.
		java_class = IUPJNI_FindClass(IupActivity, jni_env, "br/pucrio/tecgraf/iup/IupActivity");
		method_id = IUPJNI_GetStaticMethodID(IupActivity_unMapActivity, jni_env, java_class, "unMapActivity", "(Ljava/lang/Object;J)V");
		(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jlong)(intptr_t)ih);

		// Optional: Free up the temporaries.
		(*jni_env)->DeleteLocalRef(jni_env, java_class);

	
		iupAndroid_ReleaseIhandle(jni_env, ih);
	}
	
}

static void androidDialogLayoutUpdateMethod(Ihandle* ih)
{

	ih->currentwidth = TEMP_HARDCODED_WIDTH;
	ih->currentheight = TEMP_HARDCODED_HEIGHT;
#if 0
	if (ih->data->ignore_resize)
		return;
	
	ih->data->ignore_resize = 1;
	
	/* for dialogs the position is not updated here */
	SetWindowPos(ih->handle, 0, 0, 0, ih->currentwidth, ih->currentheight,
				 SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSENDCHANGING);
	
	ih->data->ignore_resize = 0;
#endif
	

}



void iupdrvDialogInitClass(Iclass* ic)
{
	/* Driver Dependent Class methods */
	ic->Map = androidDialogMapMethod;
	ic->UnMap = androidDialogUnMapMethod;
//	ic->LayoutUpdate = androidDialogLayoutUpdateMethod;
		__android_log_print(ANDROID_LOG_INFO, "iupdrvDialogInitClass", "entered"); 

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
	#endif

	/* Base Container */
	iupClassRegisterAttribute(ic, "CLIENTSIZE", androidDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);  /* dialog is the only not read-only */
#if 0
	iupClassRegisterAttribute(ic, "CLIENTOFFSET", gtkDialogGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
#endif
	
	/* Special */
	iupClassRegisterAttribute(ic, "TITLE", NULL, androidDialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	
	
#if 0
	/* IupDialog only */
	iupClassRegisterAttribute(ic, "BACKGROUND", NULL, gtkDialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ICON", NULL, gtkDialogSetIconAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "FULLSCREEN", NULL, gtkDialogSetFullScreenAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
#endif
	iupClassRegisterAttribute(ic, "MINSIZE", NULL, androidDialogSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MAXSIZE", NULL, androidDialogSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NO_INHERIT);
#if 0
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

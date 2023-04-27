/** \file
* \brief Progress bar Control
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
#include "iup_progressbar.h"
#include "iup_drv.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"
#include "iupandroid_jnicacheglobals.h"

// TODO: API: I think we're going to need a separate start/stop key.
// android Indeterminate is for progresses you don't know the range for, but are still animated when in progress.

// TODO: FEATURE: android provides spinner style

IUPJNI_DECLARE_CLASS_STATIC(IupProgressBarHelper);


#if 1



static int androidProgressBarSetValueAttrib(Ihandle* ih, const char* value)
{
	/*
	if (ih->data->marquee)
	{
		return 0;
	}
	*/
	if(!value)
	{
		ih->data->value = 0;
	}
	else
	{
		iupStrToDouble(value, &(ih->data->value));
	}

	iProgressBarCropValue(ih);


	IUPJNI_DECLARE_METHOD_ID_STATIC(IupProgressBarHelper_setProgressBarValues);
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = IUPJNI_FindClass(IupProgressBarHelper, jni_env, "br/pucrio/tecgraf/iup/IupProgressBarHelper");

	jmethodID method_id = IUPJNI_GetStaticMethodID(IupProgressBarHelper_setProgressBarValues, jni_env, java_class, "setProgressBarValues", "(JLandroid/widget/ProgressBar;DDD)V");
	(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id,
		(jlong)(intptr_t)ih,
		(jobject)ih->handle,
		ih->data->vmin,
		ih->data->vmax,
		ih->data->value
	);

	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	
	return 0;
}

static int androidProgressBarSetMarqueeAttrib(Ihandle* ih, const char* value)
{
	/*
	if (!ih->data->marquee)
	{
		return 0;
	}
	*/
	
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupProgressBarHelper_setIndeterminate);
	jboolean enable_marquee = iupStrBoolean(value);


	IUPJNI_DECLARE_METHOD_ID_STATIC(IupProgressBarHelper_setProgressBarValues);
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = IUPJNI_FindClass(IupProgressBarHelper, jni_env, "br/pucrio/tecgraf/iup/IupProgressBarHelper");

	jmethodID method_id = IUPJNI_GetStaticMethodID(IupProgressBarHelper_setIndeterminate, jni_env, java_class, "setIndeterminate", "(JLandroid/widget/ProgressBar;Z)V");
	(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id,
		(jlong)(intptr_t)ih,
		(jobject)ih->handle,
		enable_marquee
	);

	(*jni_env)->DeleteLocalRef(jni_env, java_class);


	return 1;
}



static int androidProgressBarMapMethod(Ihandle* ih)
{
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupProgressBarHelper_createProgressBar);
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = IUPJNI_FindClass(IupProgressBarHelper, jni_env, "br/pucrio/tecgraf/iup/IupProgressBarHelper");


	int initial_width = 200;
	int initial_height = 30;
	
//	woffset += 60;
//	hoffset += 10;
	//	ih->data->type = 0;
	
	// Due to an Apple bug, we can't just use setFrameCenterRotation: to create a vertical progress bar.
	// The rendering is really screwed up under certain conditions. It seems that Layer Backed Views must be enabled, and should be enabled immediately when created (and not enabled later) or the rendering is glitched.
	// As a workaround, we can create a container NSView which we rotate instead. And this doesn't even require layer backed views to be enabled.

	// IUP doc says 200x30 is the default
	// However, Mac always draws the bar 6 pixels thick. It seems to pad with empty space if you make it bigger. And Interface Builder says the height is always 20 (presuming padding around 6 pixels)
	
	IupGetIntInt(ih, "RASTERSIZE", &initial_width, &initial_height);
	if(0 == initial_width)
	{
		initial_width = 200;
	}
	if(0 == initial_height)
	{
		initial_height = 30;
	}
	
	jmethodID method_id = IUPJNI_GetStaticMethodID(IupProgressBarHelper_createProgressBar, jni_env, java_class, "createProgressBar", "(JIIZ)Landroid/widget/ProgressBar;");

	jboolean is_vertical = JNI_FALSE;

	// Vertical mode is completely broken. This appears to be a Mac bug.
	if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
	{
		is_vertical = JNI_TRUE;
	}
	else
	{

		is_vertical = JNI_FALSE;



	
		
		// We must not allow IUP to EXPAND the height of the NSProgressIndicator so unset the bit flag if it is set.
		// Mac fixes the thickness to 6 pixels. Expanding causes the progress bar to become uncentered in the container view.
		// TODO: Maybe we should remove the container view for just the horizontal.
//		ih->expand = ih->expand & ~IUP_EXPAND_HEIGHT;

		
	}
	
	jobject java_widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, 
			(jlong)(intptr_t)ih, 
			initial_width,
			initial_height,
			is_vertical
	);

	// In the cached case, this will clean up a NewLocalRef copy that the was created so it is consistent with the non-cached case.
	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	if(!java_widget)
	{
		return IUP_ERROR;
	}



	/*
	// indeterminate mode
	if (iupAttribGetBoolean(ih, "MARQUEE"))
	{

		ih->data->marquee = 1;
		
		
	}
	else
	{
		ih->data->marquee = 0;

	}
	*/

	

	ih->handle = (jobject) ((*jni_env)->NewGlobalRef(jni_env, java_widget));
	(*jni_env)->DeleteLocalRef(jni_env, java_widget);


	// add to the parent, all Android controls must call this.
	iupAndroid_AddWidgetToParent(jni_env, ih);	


	return IUP_NOERROR;
}

static void androidProgressBarUnMapMethod(Ihandle* ih)
{
	if(ih && ih->handle)
	{
		JNIEnv* jni_env;
		jclass java_class;
		jmethodID method_id;
		jni_env = iupAndroid_GetEnvThreadSafe();

		java_class = IUPJNI_FindClass(IupCommon, jni_env, "br/pucrio/tecgraf/iup/IupCommon");
		method_id = IUPJNI_GetStaticMethodID(IupCommon_removeWidgetFromParent, jni_env, java_class, "removeWidgetFromParent", "(J)V");
		(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih);
		(*jni_env)->DeleteLocalRef(jni_env, java_class);


		(*jni_env)->DeleteGlobalRef(jni_env, ih->handle);
		ih->handle = NULL;
	}
	
}
#endif
void iupdrvProgressBarInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
	ic->Map = androidProgressBarMapMethod;
	ic->UnMap = androidProgressBarUnMapMethod;

  /* Driver Dependent Attribute functions */
  
  /* Visual */
//  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  
  /* Special */
//  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
#if 1

  /* IupProgressBar only */
  iupClassRegisterAttribute(ic, "VALUE",  iProgressBarGetValueAttrib,  androidProgressBarSetValueAttrib,  NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

	
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARQUEE",     NULL, androidProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DASHED",      NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
#endif
	
}

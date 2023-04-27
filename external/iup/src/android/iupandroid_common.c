/** \file
 * \brief Android Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>              
#include <stdlib.h>
#include <string.h>             
#include <limits.h>             

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_focus.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_drv.h"

#include "iupandroid_drv.h"

#include <android/log.h>
#include <jni.h>
#include <pthread.h>
#include <stdbool.h>

#include "iupandroid_jnimacros.h"
#include "iupandroid_jnicacheglobals.h"


static int s_isInitialized = 0;

static JavaVM* s_javaVM = NULL;

// This is intended to be the God Context object you need for a lot of Android APIs (e.g. AssetManager).
// It uses the Application context because that supposedly will be available for the life of the program.
static jobject s_applicationContextObject = NULL;
static pthread_key_t s_attachThreadKey;
// We must cache the IupApplication class. See notes for iupAndroid_GetApplication() to understand why.
static jclass s_classIupApplication = NULL;


/*
*******
FAQ: Why didn't FindClass find my class?
Excerpt: 
If the class name looks right, you could be running into a class loader issue. FindClass wants to start the class search in the class loader associated with your code. It examines the call stack, which will look something like:

    Foo.myfunc(Native Method)
    Foo.main(Foo.java:10)
The topmost method is Foo.myfunc. FindClass finds the ClassLoader object associated with the Foo class and uses that.

This usually does what you want. You can get into trouble if you create a thread yourself (perhaps by calling pthread_create and then attaching it with AttachCurrentThread). Now there are no stack frames from your application. If you call FindClass from this thread, the JavaVM will start in the "system" class loader instead of the one associated with your application, so attempts to find app-specific classes will fail.

There are a few ways to work around this:

Do your FindClass lookups once, in JNI_OnLoad, and cache the class references for later use. Any FindClass calls made as part of executing JNI_OnLoad will use the class loader associated with the function that called System.loadLibrary (this is a special rule, provided to make library initialization more convenient). If your app code is loading the library, FindClass will use the correct class loader.
Pass an instance of the class into the functions that need it, by declaring your native method to take a Class argument and then passing Foo.class in.
Cache a reference to the ClassLoader object somewhere handy, and issue loadClass calls directly. This requires some effort.
*******

Problem: Because we are a C-based cross-platform framework, most of our users will probably create a pthread themselves when they need a thread (and AttachCurrentThread).
This causes the problem in the FAQ.
Ironically, we are trying to solve our threading problems by calling MainThreadRedirect to get back on the main thread.
But we can't FindClass from a background thread.

Solution: Since this class is special, we will cache it and avoid the FindClass issue that way.
So we need to cache it in JNI_OnLoad.
*/
static jclass s_classIupMainThreadRedirect = NULL;



void iupAndroid_ThreadDestroyed(void* user_data)
{
	/* The thread is being destroyed, detach it from the Java VM and set the s_attachThreadKey value to NULL as required */
	JNIEnv* jni_env = (JNIEnv*)user_data;
	if(NULL != jni_env)
	{
		(*s_javaVM)->DetachCurrentThread(s_javaVM);
		pthread_setspecific(s_attachThreadKey, NULL);
	}
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* java_vm, void* reserved)
{
    s_javaVM = java_vm;
	/* SDL notes:
	 * Create s_attachThreadKey so we can keep track of the JNIEnv assigned to each thread
     * Refer to http://developer.android.com/guide/practices/design/jni.html for the rationale behind this
     */
    if(pthread_key_create(&s_attachThreadKey, iupAndroid_ThreadDestroyed) != 0)
	{
		__android_log_print(ANDROID_LOG_ERROR, "Iup", "Error initializing pthread key");
    }

	// In order to cache our IupApplication class (see notes in iupAndroid_GetApplication)
	// we need the jni_env. The thread should already be attached (I think) for the OnLoad case,
	// but just in case, we'll call our iupAndroid_GetEnvThreadSafe().
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass the_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iup/IupApplication");
	s_classIupApplication = (jobject)((*jni_env)->NewGlobalRef(jni_env, the_class));
	(*jni_env)->DeleteLocalRef(jni_env, the_class);



	// See comments at the top of the file about why we need to cache IupMainThreadRedirect.
	// This code must be run on the main thread.
	the_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iup/IupMainThreadRedirect");
	s_classIupMainThreadRedirect = (jobject)((*jni_env)->NewGlobalRef(jni_env, the_class));
	(*jni_env)->DeleteLocalRef(jni_env, the_class);

    return JNI_VERSION_1_6;
}

void iupAndroid_DetachThreadIfAttached()
{
	JNIEnv* jni_env = NULL;
	int get_env_stat = (*s_javaVM)->GetEnv(s_javaVM, (void**)&jni_env, JNI_VERSION_1_6);
	if(get_env_stat == JNI_EDETACHED)
	{
		// Not attached, don't need to do anything.
	}
	else if(JNI_OK == get_env_stat)
	{
		jint detach_status = (*s_javaVM)->DetachCurrentThread(s_javaVM);
		if(0 != detach_status)
		{
			__android_log_print(ANDROID_LOG_ERROR, "Iup", "DetachCurrentThread failed");
		}
	}
	else if (get_env_stat == JNI_EVERSION)
	{
		__android_log_print(ANDROID_LOG_ERROR, "Iup", "GetEnv version not supported");
	}
}

void iupAndroid_DetachThreadUnchecked()
{
	jint detach_status = (*s_javaVM)->DetachCurrentThread(s_javaVM);
	if(0 != detach_status)
	{
		__android_log_print(ANDROID_LOG_ERROR, "Iup", "DetachCurrentThread failed");
	}
}

JNIEnv* iupAndroid_GetEnvThreadSafeWithAttachStatus(_Bool* out_was_detached)
{
	JNIEnv* jni_env = NULL;
	
	/* ALmixer Notes: */
	/* Careful: If ALmixer is compiled with threads, make sure any calls back into Java are thread safe. */
	/* Unfortunately, JNI is making thread handling even more complicated than usual.
	 * If ALmixer is compiled with threads, it invokes callbacks on a ALmixer private background thread.
	 * In this case, we are required to call AttachCurrentThread for Java.
	 * However, there is a case in ALmixer where the callback doesn't happen on the background thread, but the calling thread.
	 * Calling ALmixer_HaltChannel() will trigger the callback on immediately on the thread you called the function on.
	 * (In this program, it is the main thread.)
	 * But JNI will break and crash if you try calling AttachCurrentThread in this case.
	 * So we need to know what thread we are on. If we are on the background thread, we must call AttachCurrentThread.
	 * Otherwise, we need to avoid calling it and use the current "env".
	 */

	_Bool was_detached = false;

	/* There is a little JNI dance you can do to deal with this situation which is shown here.
	*/
	int get_env_stat = (*s_javaVM)->GetEnv(s_javaVM, (void**)&jni_env, JNI_VERSION_1_6);
	if(get_env_stat == JNI_EDETACHED)
	{
		was_detached = true;
		jint attach_status = (*s_javaVM)->AttachCurrentThread(s_javaVM, &jni_env, NULL);
		if(0 != attach_status)
		{
			__android_log_print(ANDROID_LOG_ERROR, "Iup", "AttachCurrentThread failed"); 
		}

		/* SDL notes: */
		/* From http://developer.android.com/guide/practices/jni.html
		 * Threads attached through JNI must call DetachCurrentThread before they exit. If coding this directly is awkward,
		 * in Android 2.0 (Eclair) and higher you can use pthread_key_create to define a destructor function that will be
		 * called before the thread exits, and call DetachCurrentThread from there. (Use that key with pthread_setspecific
		 * to store the JNIEnv in thread-local-storage; that way it'll be passed into your destructor as the argument.)
		 * Note: The destructor is not called unless the stored value is != NULL
		 * Note: You can call this function any number of times for the same thread, there's no harm in it
		 *       (except for some lost CPU cycles)
		 */
		pthread_setspecific(s_attachThreadKey, (void*)jni_env);

	}
	else if(JNI_OK == get_env_stat)
	{
		// don't need to do anything
		was_detached = false;
	}
	else if (get_env_stat == JNI_EVERSION)
	{
		__android_log_print(ANDROID_LOG_ERROR, "Iup", "GetEnv version not supported");
		was_detached = false;
	}

	if(NULL != out_was_detached)
	{
		*out_was_detached = was_detached;
	}

	return jni_env;
}

// External callers may want to call iupAndroid_GetEnvThreadSafeWithAttachStatus
// so you can decide if you want to detach immediately after you are done.
JNIEnv* iupAndroid_GetEnvThreadSafe()
{
	return iupAndroid_GetEnvThreadSafeWithAttachStatus(NULL);
}

void iupAndroid_RetainIhandle(JNIEnv* jni_env, jobject native_widget, Ihandle* ih)
{
	if(ih)
	{
		ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, native_widget));
		__android_log_print(ANDROID_LOG_INFO, "Iup", "NewGlobalRef on ih->handle: %p", ih->handle);
	}
}

void iupAndroid_ReleaseIhandle(JNIEnv* jni_env, Ihandle* ih)
{
	if(ih && ih->handle)
	{
		__android_log_print(ANDROID_LOG_INFO, "Iup", "DeleteGlobalRef on ih->handle: %p", ih->handle);
		(*jni_env)->DeleteGlobalRef(jni_env, ih->handle);
		ih->handle = NULL;
	}
}


/*
*******
FAQ: Why didn't FindClass find my class?
Excerpt: 
If the class name looks right, you could be running into a class loader issue. FindClass wants to start the class search in the class loader associated with your code. It examines the call stack, which will look something like:

    Foo.myfunc(Native Method)
    Foo.main(Foo.java:10)
The topmost method is Foo.myfunc. FindClass finds the ClassLoader object associated with the Foo class and uses that.

This usually does what you want. You can get into trouble if you create a thread yourself (perhaps by calling pthread_create and then attaching it with AttachCurrentThread). Now there are no stack frames from your application. If you call FindClass from this thread, the JavaVM will start in the "system" class loader instead of the one associated with your application, so attempts to find app-specific classes will fail.

There are a few ways to work around this:

Do your FindClass lookups once, in JNI_OnLoad, and cache the class references for later use. Any FindClass calls made as part of executing JNI_OnLoad will use the class loader associated with the function that called System.loadLibrary (this is a special rule, provided to make library initialization more convenient). If your app code is loading the library, FindClass will use the correct class loader.
Pass an instance of the class into the functions that need it, by declaring your native method to take a Class argument and then passing Foo.class in.
Cache a reference to the ClassLoader object somewhere handy, and issue loadClass calls directly. This requires some effort.
*******

Problem: iupAndroid_GetApplication is a very important function that may be used to get a Context from another thread.
And that thread may have been created via pthread_create, 
and then using our iupAndroid_GetEnvThreadSafe() function 
which uses AttachCurrentThread.

So we are vulnerable to the problem described in the FAQ.

Solution: Since this class is special, we will cache it and avoid the FindClass issue that way.
So we need to cache it in JNI_OnLoad.
*/
jobject iupAndroid_GetApplication(JNIEnv* jni_env)
{
	static jobject s_applicationObject = NULL;
	jobject ret_object = NULL;


	if(NULL == s_applicationObject)
	{
		IUPJNI_DECLARE_METHOD_ID_STATIC(IupApplication_getIupApplication);
		jclass java_class;
		jmethodID method_id;

		//	java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iup/IupApplication");
		java_class = (*jni_env)->NewLocalRef(jni_env, s_classIupApplication);
		method_id = IUPJNI_GetStaticMethodID(IupApplication_getIupApplication, jni_env, java_class, "getIupApplication", "()Lbr/pucrio/tecgraf/iup/IupApplication;");
		ret_object = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id);

		s_applicationObject = (*jni_env)->NewGlobalRef(jni_env, ret_object);
		(*jni_env)->DeleteLocalRef(jni_env, java_class);

	}
	else
	{
		ret_object = (*jni_env)->NewLocalRef(jni_env, s_applicationObject);

	}

	// Note: This is a Local Ref. Caller is expected to call DeleteLocalRef when done.
	return ret_object;

}

// Note: Because the "Current" activity can change, we don't want to cache it.
jobject iupAndroid_GetCurrentActivity(JNIEnv* jni_env)
{
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupApplication_getCurrentActivity);
	jclass java_class;
    jmethodID method_id;
	jobject ret_object = NULL;

	jobject application_object = iupAndroid_GetApplication(jni_env);

	// We actually know the class has to be IupApplication, which is already cached in s_classIupApplication
//    java_class = (*jni_env)->GetObjectClass(jni_env, application_object);
	java_class = (*jni_env)->NewLocalRef(jni_env, s_classIupApplication);
	method_id = IUPJNI_GetMethodID(IupApplication_getCurrentActivity, jni_env, java_class, "getCurrentActivity", "()Landroid/app/Activity;");
	ret_object = (*jni_env)->CallObjectMethod(jni_env, application_object, method_id);

	(*jni_env)->DeleteLocalRef(jni_env, java_class);
	(*jni_env)->DeleteLocalRef(jni_env, application_object);

	return ret_object;
}

void iupAndroid_AddWidgetToParent(JNIEnv* jni_env, Ihandle* ih)
{
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupCommon_addWidgetToParent);


	jclass java_class;
    jmethodID method_id;

	jobject parent_native_handle = iupChildTreeGetNativeParentHandle(ih);
	jobject child_handle = ih->handle;
	
		__android_log_print(ANDROID_LOG_INFO, "iupAndroidAddWidgetToParent", "parent_native_handle:%p, ih->handle: %p", parent_native_handle, ih->handle);


	java_class = IUPJNI_FindClass(IupCommon, jni_env, "br/pucrio/tecgraf/iup/IupCommon");
	method_id = IUPJNI_GetStaticMethodID(IupCommon_addWidgetToParent, jni_env, java_class, "addWidgetToParent", "(Ljava/lang/Object;Ljava/lang/Object;)V");
	(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, parent_native_handle, child_handle);

	(*jni_env)->DeleteLocalRef(jni_env, java_class);


}



void iupdrvActivate(Ihandle* ih)
{

}

void iupdrvReparent(Ihandle* ih)
{

	
}


void iupdrvBaseLayoutUpdateMethod(Ihandle *ih)
{
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupCommon_setWidgetPosition);


	jobject parent_native_handle = iupChildTreeGetNativeParentHandle(ih);
	jobject child_handle = ih->handle;

	__android_log_print(ANDROID_LOG_INFO, "iupdrvBaseLayoutUpdateMethod", "parent_native_handle:%p, ih->handle: %p", parent_native_handle, ih->handle);
	__android_log_print(ANDROID_LOG_INFO, "iupdrvBaseLayoutUpdateMethod", "x:%d, y:%d, w:%d, h:%d", ih->x, ih->y, ih->currentwidth, ih->currentheight);

	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "br/pucrio/tecgraf/iup/IupCommon");
	jmethodID method_id = IUPJNI_GetStaticMethodID(IupCommon_setWidgetPosition, jni_env, java_class, "setWidgetPosition", "(Ljava/lang/Object;IIII)V");
	(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, child_handle, ih->x, ih->y, ih->currentwidth, ih->currentheight);

	(*jni_env)->DeleteLocalRef(jni_env, java_class);

}

void iupdrvBaseUnMapMethod(Ihandle* ih)
{
	// Why do I need this when everything else has its own UnMap method?
	//NSLog(@"iupdrvBaseUnMapMethod not implemented. Might be leaking");
}

void iupdrvDisplayUpdate(Ihandle *ih)
{
	jobject the_handle = ih->handle;
	
	// call ViewGroup.invalidate()

}

void iupdrvDisplayRedraw(Ihandle *ih)
{
	iupdrvDisplayUpdate(ih);
}

void iupdrvScreenToClient(Ihandle* ih, int *x, int *y)
{
}



int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  return 0;
}

void iupdrvSetVisible(Ihandle* ih, int visible)
{
	jobject the_object = ih->handle;
}

int iupdrvIsVisible(Ihandle* ih)
{
	return 1;
}

int iupdrvIsActive(Ihandle *ih)
{
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupCommon_isActive);
	jclass java_class;
	jmethodID method_id;

	jobject widget_object = ih->handle;

	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();

	java_class = IUPJNI_FindClass(IupCommon, jni_env, "br/pucrio/tecgraf/iup/IupCommon");
	method_id = IUPJNI_GetStaticMethodID(IupCommon_isActive, jni_env, java_class, "isActive", "(Ljava/lang/Object;)Z");
	jboolean ret_val = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, method_id, widget_object);

	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	return ret_val;
}

void iupdrvSetActive(Ihandle* ih, int enable)
{
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupCommon_setActive);
	jclass java_class;
	jmethodID method_id;

	jobject widget_object = ih->handle;

	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();

	java_class = IUPJNI_FindClass(IupCommon, jni_env, "br/pucrio/tecgraf/iup/IupCommon");
	method_id = IUPJNI_GetStaticMethodID(IupCommon_setActive, jni_env, java_class, "setActive", "(Ljava/lang/Object;Z)V");
	(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, widget_object, (jboolean)enable);

	(*jni_env)->DeleteLocalRef(jni_env, java_class);

}

char* iupdrvBaseGetXAttrib(Ihandle *ih)
{
  return NULL;
}

char* iupdrvBaseGetYAttrib(Ihandle *ih)
{

  return NULL;
}

/*
char* iupdrvBaseGetClientSizeAttrib(Ihandle *ih)
{

    return NULL;

}
 */

int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{

	

  /* DO NOT NEED TO UPDATE GTK IMAGES SINCE THEY DO NOT DEPEND ON BGCOLOR */

  return 1;
}

int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{

  return 0;
}


int iupdrvGetScrollbarSize(void)
{

  return 0;
}

void iupdrvSetAccessibleTitle(Ihandle *ih, const char* title)
{

}

void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
	/*
#ifndef GTK_MAC
  #ifdef WIN32                                 
    iupClassRegisterAttribute(ic, "HFONT", iupgtkGetFontIdAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  #else
    iupClassRegisterAttribute(ic, "XFONTID", iupgtkGetFontIdAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  #endif
#endif
  iupClassRegisterAttribute(ic, "PANGOFONTDESC", iupgtkGetPangoFontDescAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
*/
}

void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
	
}

void iupdrvClientToScreen(Ihandle* ih, int *x, int *y)
{
	
}

void iupdrvPostRedraw(Ihandle *ih)
{

}

void iupdrvRedrawNow(Ihandle *ih)
{

}
void iupdrvSendKey(int key, int press)
{
	
}
void iupdrvSendMouse(int x, int y, int bt, int status)
{
	
}
void iupdrvSleep(int time)
{
	
}
void iupdrvWarpPointer(int x, int y)
{
	
}

// Even though other platforms implement this in iup*_loop.*,
// it was more convenient to implement this here so we can keep 
// s_classIupMainThreadRedirect as a static variable, instead of a global variable.
void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
	// We need to call into Java and instruct it to run a block of code 
	// on the UI Thread that will callback into C and invoke the user's callback function.

	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jobject app_context = (jobject)iupAndroid_GetApplication(jni_env);

	jclass java_class = (*jni_env)->NewLocalRef(jni_env, s_classIupMainThreadRedirect);

	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "postMessage", "(Landroid/content/Context;JJLjava/lang/String;JD)V");

	jstring j_string = (*jni_env)->NewStringUTF(jni_env, s);

    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, app_context, (jlong)(intptr_t)ih, (jlong)(intptr_t)p, j_string, (jlong)i, (jdouble)d);

	(*jni_env)->DeleteLocalRef(jni_env, j_string);

	(*jni_env)->DeleteLocalRef(jni_env, java_class);
}

#include <stddef.h>
#include <jni.h>
#include <android/log.h>
#include "iupandroid_drv.h"
#include "iup.h"
#include "iupcbs.h"

#include <dlfcn.h>




JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupLaunchActivity_IupEntry(JNIEnv* jni_env, jobject thiz, jobject current_activity, jstring j_entry_function_name, jstring j_library_name)
{
	__android_log_print(ANDROID_LOG_INFO,  "IupLaunchActivity", "Calling IupEntry");

	// Because of the way the other platforms work, I don't think any of them should call IupOpen inside the IupEntry callback.
	// That means we should call IupOpen ourselves for Android.
	IupOpen(0, NULL);

//	IupEntry();
	// Invoke the IupEntry callback function to start the user code.
	// I don't see how the user will be able to set ENTRY_POINT in normal startup conditions,
	// but if they did, let's respect it.
	IFentry entry_callback = (IFentry)IupGetFunction("ENTRY_POINT");
	__android_log_print(ANDROID_LOG_INFO,  "IupLaunchActivity", "passed IupGetFunction('ENTRY_POINT') with result %p", entry_callback);
#if 1


	// The string parameter passed in may contain the function name we want to call.
	if(NULL == entry_callback)
	{
		if(NULL != j_entry_function_name)
		{
			const char* c_entry_function_name = (*jni_env)->GetStringUTFChars(jni_env, j_entry_function_name, NULL);
			__android_log_print(ANDROID_LOG_INFO,  "IupLaunchActivity", "getting entry point from AndroidManifest: %s", c_entry_function_name);
			entry_callback = (IFentry)dlsym(RTLD_DEFAULT, c_entry_function_name);
			(*jni_env)->ReleaseStringUTFChars(jni_env, j_entry_function_name, c_entry_function_name);
			__android_log_print(ANDROID_LOG_INFO,  "IupLaunchActivity", "entry_callback result from Manifest %p", entry_callback);
        }
	}
#endif
	// If no entry point has been defined, we can try to fallback and use dsym to look up a hardcoded function name.
	if(NULL == entry_callback)
	{
		__android_log_print(ANDROID_LOG_INFO,  "IupLaunchActivity", "final fallback. Looking for IupEntryPoint");

		entry_callback = (IFentry)dlsym(RTLD_DEFAULT, "IupEntryPoint");
		__android_log_print(ANDROID_LOG_INFO,  "IupLaunchActivity", "final fallback result %p", entry_callback);

		// I think Android 7.0 intentionally broke RTLD_DEFAULT behavior as their way of trying to block access to private libs.
		// We don't use private libs, but in some cases, I'm getting back NULL in some cases on Android 7.0 devices.
		// I'm unclear why, because in isolated cases built directly into the Iup Android Studio project, this still works.
		// But taking a built .aar and .so and using it in another project on 64-bit Android 7.0 devices seems to break.
		// Maybe it could be that test builds are treated differently than product builds.
		// So as a fallback/workaround, if the pointer is still NULL by this point, we will try to directly dlopen the library to get the handle.
		// Assumption: System.loadLibrary() has already loaded our library.
		// I presume since it is loaded by loadLibrary, dlopen() might not need to be as rigorous with absolute paths
		// or other possible side-effects which could break loadLibrary if we reversed the order.
		// (which would require apriori information to where these things are since there is no official API for this).
        if(NULL == entry_callback)
        {
			const char* c_entry_library_name = (*jni_env)->GetStringUTFChars(jni_env, j_library_name, NULL);
			__android_log_print(ANDROID_LOG_INFO,  "IupLaunchActivity", "getting entry point library name: %s", c_entry_library_name);

			// Get the handle to the library containing our entry point symbol.
			void* dl_handle = dlopen(c_entry_library_name, RTLD_LOCAL|RTLD_NOW);

			__android_log_print(ANDROID_LOG_INFO,  "IupLaunchActivity", "dlopen result %p", dl_handle);

			entry_callback = (IFentry)dlsym(dl_handle, "IupEntryPoint");
			(*jni_env)->ReleaseStringUTFChars(jni_env, j_library_name, c_entry_library_name);
			__android_log_print(ANDROID_LOG_INFO,  "IupLaunchActivity", "final fallback result from dlopen %p", entry_callback);
        }
	}

	__android_log_print(ANDROID_LOG_INFO,  "IupLaunchActivity", "checking to finally call entry_callback %p", entry_callback);

	if(NULL != entry_callback)
	{
		__android_log_print(ANDROID_LOG_INFO,  "IupLaunchActivity", "about to finally call entry_callback %p", entry_callback);
		entry_callback();
		__android_log_print(ANDROID_LOG_INFO,  "IupLaunchActivity", "finished entry_callback");
	}
	
	__android_log_print(ANDROID_LOG_INFO,  "IupLaunchActivity", "Returned from IupEntry");
  //  return JNI_TRUE;
}




JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupLaunchActivity_doPause(JNIEnv* env, jobject thiz)
{
	//LOGD("Java_br_pucrio_tecgraf_iup_IupLaunchActivity_doPause");
}

JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupLaunchActivity_doResume(JNIEnv* env, jobject thiz)
{
	//LOGD("Java_br_pucrio_tecgraf_iup_IupLaunchActivity_doResume");
}

JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupLaunchActivity_doDestroy(JNIEnv* env, jobject thiz)
{
	//LOGD("Java_br_pucrio_tecgraf_iup_IupLaunchActivity_doDestroy");

	
	/* Release the proxy object. */
//	(*env)->DeleteGlobalRef(env, s_proxyObject);
//	s_proxyObject = NULL;

//	LOGD("Java_br_pucrio_tecgraf_iup_IupLaunchActivity_doDestroy end");
	
}


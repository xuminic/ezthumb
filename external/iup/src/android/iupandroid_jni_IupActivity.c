#include <jni.h>
#include <android/log.h>
#include <stdint.h>

#include "iup.h"
#include "iupcbs.h"
//#include "iup_object.h"
#include "iupandroid_drv.h"


/*
JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupActivity_swapAndAttachViewGroup(JNIEnv* jni_env, jobject thiz, jobject current_activity, jlong ihandle_ptr)
{
	Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;

	if(ih)
	{
		if(ih->handle)
		{
			
		}
	}


}
*/


JNIEXPORT jboolean JNICALL Java_br_pucrio_tecgraf_iup_IupActivity_doInit(JNIEnv* jni_env, jobject thiz, jobject current_activity)
{

	__android_log_print(ANDROID_LOG_INFO,  "IupActivity", "in doInit for IupActivity");


	
    return JNI_TRUE;
}


JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupActivity_doPause(JNIEnv* jni_env, jobject thiz)
{
	//LOGD("Java_br_pucrio_tecgraf_iup_IupActivity_doPause");
}

JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupActivity_doResume(JNIEnv* jni_env, jobject thiz)
{
	//LOGD("Java_br_pucrio_tecgraf_iup_IupActivity_doResume");
}

JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupActivity_doDestroy(JNIEnv* jni_env, jobject thiz)
{
	//LOGD("Java_br_pucrio_tecgraf_iup_IupActivity_doDestroy");

	
	/* Release the proxy object. */
//	(*env)->DeleteGlobalRef(env, s_proxyObject);
//	s_proxyObject = NULL;

//	LOGD("Java_br_pucrio_tecgraf_iup_IupActivity_doDestroy end");
	
}


JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupActivity_OnActivityResult(JNIEnv* jni_env, jobject thiz, jlong ihandle_ptr, jint request_code, jint result_code, jobject intent_data)
{
	Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
	if(ih)
	{
		__android_log_print(ANDROID_LOG_INFO, "Java_br_pucrio_tecgraf_iup_IupActivity_onActivityResult", "Java_br_pucrio_tecgraf_iup_IupActivity_onActivityResult");

		IFniiv callback_function = (IFniiv)IupGetCallback(ih, "ONACTIVITYRESULT_CB");
		if(callback_function)
		{
			callback_function(ih, request_code, result_code, intent_data);
		}
	}
}



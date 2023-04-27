#include <jni.h>
#include <android/log.h>
#include <stdint.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iupandroid_drv.h"

JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupCommon_RetainIhandle(JNIEnv* jni_env, jclass cls, jobject the_widget, jlong ihandle_ptr)
{
	iupAndroid_RetainIhandle(jni_env, the_widget, (Ihandle*)(intptr_t)ihandle_ptr);
}

JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupCommon_ReleaseIhandle(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
	iupAndroid_ReleaseIhandle(jni_env, (Ihandle*)(intptr_t)ihandle_ptr);
}


JNIEXPORT jobject JNICALL Java_br_pucrio_tecgraf_iup_IupCommon_GetObjectFromIhandle(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
	Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
	if(ih && ih->handle)
	{
		return ih->handle;
	}
	return NULL;
}


JNIEXPORT jstring JNICALL Java_br_pucrio_tecgraf_iup_IupCommon_nativeIupAttribGet(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_key_string)
{
	Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
	if(ih)
	{
		if(NULL != j_key_string)
		{
			const char* key_string = (*jni_env)->GetStringUTFChars(jni_env, j_key_string, NULL);
		__android_log_print(ANDROID_LOG_INFO, "Java_br_pucrio_tecgraf_iup_IupCommon_nativeIupAttribGet", "key_string: %s", key_string); 
			
			char* value_string = iupAttribGet(ih, key_string);
			(*jni_env)->ReleaseStringUTFChars(jni_env, j_key_string, key_string);
		__android_log_print(ANDROID_LOG_INFO, "Java_br_pucrio_tecgraf_iup_IupCommon_nativeIupAttribGet", "value_string: %s", value_string); 

			if((NULL != value_string) && (*value_string != 0))
			{
				jstring j_value_string = (*jni_env)->NewStringUTF(jni_env, value_string);
				return j_value_string;
				
			}
			else
			{
				return NULL;
			}
			
		}	
	}
	return NULL;
}

JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupCommon_nativeIupAttribSet(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_key_string, jstring j_value_string)
{
	Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
	if(ih)
	{
		if(NULL != j_key_string)
		{
			const char* key_string = (*jni_env)->GetStringUTFChars(jni_env, j_key_string, NULL);
			const char* value_string = (*jni_env)->GetStringUTFChars(jni_env, j_value_string, NULL);
		
			iupAttribSet(ih, key_string, value_string);
			
			(*jni_env)->ReleaseStringUTFChars(jni_env, j_value_string, value_string);
			(*jni_env)->ReleaseStringUTFChars(jni_env, j_key_string, key_string);
		}	
	}
}



JNIEXPORT jint JNICALL Java_br_pucrio_tecgraf_iup_IupCommon_nativeIupAttribGetInt(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_key_string)
{
	Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
	if(ih)
	{
		if(NULL != j_key_string)
		{
			const char* key_string = (*jni_env)->GetStringUTFChars(jni_env, j_key_string, NULL);
			
			int value_int = iupAttribGetInt(ih, key_string);
			(*jni_env)->ReleaseStringUTFChars(jni_env, j_key_string, key_string);

			return (jint)value_int;
		}	
	}
	return 0;
}

JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupCommon_nativeIupAttribSetInt(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_key_string, jint j_value_int)
{
	Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
	if(ih)
	{
		if(NULL != j_key_string)
		{
			const char* key_string = (*jni_env)->GetStringUTFChars(jni_env, j_key_string, NULL);
		
			iupAttribSetInt(ih, key_string, (int)j_value_int);
			
			(*jni_env)->ReleaseStringUTFChars(jni_env, j_key_string, key_string);
		}	
	}
}



/* IUP returns -1 through -4 for callbacks. I also return -15 if no callback is registered. I picked -15 because I wanted to leave >=0 for users and wanted to leave IUP space to expand. */
JNIEXPORT int JNICALL Java_br_pucrio_tecgraf_iup_IupCommon_HandleIupCallback(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_key_string)
{

	int ret_val = -15;
	Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
	if(ih)
	{
		if(NULL != j_key_string)
		{
			const char* key_string = (*jni_env)->GetStringUTFChars(jni_env, j_key_string, NULL);
		__android_log_print(ANDROID_LOG_INFO, "Java_br_pucrio_tecgraf_iup_IupCommon_HandleIupCallback", "key_string: %s", key_string); 

			Icallback callback_function = IupGetCallback(ih, key_string);
			(*jni_env)->ReleaseStringUTFChars(jni_env, j_key_string, key_string);

			if(callback_function)
			{
				ret_val = callback_function(ih);
				if(IUP_CLOSE == ret_val)
				{
					IupExitLoop();
				}
			}
		}	
	}
	return ret_val;
}

JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupCommon_DoResize(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint x, jint y, jint width, jint height)
{
	Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
	IFnii cb;
	cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
	// FIXME: Are the parameters supposed to be the contentView or the entire window. The Windows code comments make me think contentView, but the actual code makes me think entire window. The latter is way easier to do.
	if(!cb || cb(ih, width, height)!=IUP_IGNORE)
	{
		ih->currentwidth = width;
		ih->currentheight = height;
		
//		ih->data->ignore_resize = 1;
		IupRefresh(ih);
//		ih->data->ignore_resize = 0;
	}
	else
	{
		// WARNING: It is impossible to refuse a resize on Android.
		__android_log_print(ANDROID_LOG_INFO, "Java_br_pucrio_tecgraf_iup_IupCommon_HandleIupCallback", "IUP_IGNORE not supported on iOS for RESIZE_CB");
		ih->currentwidth = width;
		ih->currentheight = height;
		
//		ih->data->ignore_resize = 1;
		IupRefresh(ih);
//		ih->data->ignore_resize = 0;
	}
	
}



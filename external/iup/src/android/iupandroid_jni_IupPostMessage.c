#include <jni.h>
//#include <android/log.h>

#include "iup.h"

JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupPostMessage_OnMainThreadCallback(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jlong message_data, jstring j_usr_str, jlong j_usr_int, jdouble j_usr_double)
{
	Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
	IFnsid user_post_message_callback = (IFnsid)IupGetCallback(ih, "POSTMESSAGE_CB");
	if(user_post_message_callback)
	{
		const char* c_usr_str = (*jni_env)->GetStringUTFChars(jni_env, j_usr_str, NULL);

		//user_post_message_callback(ih, (void*)(intptr_t)message_data);
		user_post_message_callback(ih, c_usr_str, (int)j_usr_int, j_usr_double);

		(*jni_env)->ReleaseStringUTFChars(jni_env, j_usr_str, c_usr_str);
	}
}


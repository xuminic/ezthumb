#include <jni.h>
#include <android/log.h>

#include "iup.h"
#include "iup_loop.h"


JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iup_IupApplication_IupExitCallback(JNIEnv* jni_env, jobject thiz)
{


	__android_log_print(ANDROID_LOG_INFO,  "IupApplication", "Doing IupExitCallback");

	iupLoopCallExitCb();


  //  return JNI_TRUE;
}


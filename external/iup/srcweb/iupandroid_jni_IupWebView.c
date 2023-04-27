#include <jni.h>
#include <android/log.h>
#include <stdint.h>

#include "iup.h"
#include "iupcbs.h"

//#include "iup_object.h"
//#include "iup_attrib.h"
//#include "iupandroid_drv.h"

// NEWWINDOW_CB not supported

/*
 ~/Source/GIT/IupMac/iupAndroid/Android/iupweb/build/intermediates/classes/release]$ javah -classpath /Library/Frameworks/Android/android-sdk/platforms/android-25/android.jar:. br.pucrio.tecgraf.iupweb.IupWebView$IupWebViewClient
*/

/*
 * Class:     br_pucrio_tecgraf_iupweb_IupWebView_IupWebViewClient
 * Method:    OnReceivedError
 * Signature: (JLandroid/webkit/WebView;ILjava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_br_pucrio_tecgraf_iupweb_IupWebView_00024IupWebViewClient_OnReceivedError(JNIEnv* jni_env, jobject thiz, jlong ihandle_ptr, jobject web_view, jint error_code, jstring j_error_description, jstring j_failing_url)
{
	Ihandle* ih = (Ihandle*)ihandle_ptr;

	IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
	if (cb)
	{
		// TODO: Introduce API to pass error code and error string.
	
		const char* c_failing_url = (*jni_env)->GetStringUTFChars(jni_env, j_failing_url, NULL);
		cb(ih, (char*)c_failing_url);
		(*jni_env)->ReleaseStringUTFChars(jni_env, j_failing_url, c_failing_url);
	}
}

/*
 * Class:     br_pucrio_tecgraf_iupweb_IupWebView_IupWebViewClient
 * Method:    ShouldOverrideUrlLoading
 * Signature: (JLandroid/webkit/WebView;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_br_pucrio_tecgraf_iupweb_IupWebView_00024IupWebViewClient_ShouldOverrideUrlLoading(JNIEnv* jni_env, jobject thiz, jlong ihandle_ptr, jobject web_view, jstring j_url)
{
	Ihandle* ih = (Ihandle*)ihandle_ptr;
	jboolean ret_flag = JNI_FALSE;
	
	IFns cb = (IFns)IupGetCallback(ih, "NAVIGATE_CB");
	if (cb)
	{
		
		const char* c_url = (*jni_env)->GetStringUTFChars(jni_env, j_url, NULL);
		
		if(cb(ih, (char*)c_url) == IUP_IGNORE)
		{
			// tells the web view that the application will handle the link instead
			ret_flag = JNI_TRUE;
		}
		else
		{
			// let's the webview follow the link
			ret_flag = JNI_FALSE;

		}
		(*jni_env)->ReleaseStringUTFChars(jni_env, j_url, c_url);
		
	}

	return ret_flag;
	
}



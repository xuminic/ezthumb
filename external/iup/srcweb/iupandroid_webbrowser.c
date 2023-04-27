/** \file
 * \brief Web Browser Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stddef.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_webbrowser.h"
#include "iup_drv.h"
#include "iup_drvfont.h"

#include "iupandroid_drv.h"

#include <jni.h>
#include <android/log.h>


/*********************************************************************************************/

static char* androidWebBrowserGetItemHistoryAttrib(Ihandle* ih, int index)
{
	char* return_str = NULL;
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getItemHistoryAtIndex", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;I)Ljava/lang/String;");

	jobject web_view = (jobject)ih->handle;
	jint j_index = (jint)index;

	jstring j_url = (jstring) ((*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, web_view, j_index));
	if(NULL != j_url)
	{
		const char* c_url;
		c_url = (*jni_env)->GetStringUTFChars(jni_env, j_url, NULL);
		return_str = iupStrReturnStr(c_url);
		(*jni_env)->ReleaseStringUTFChars(jni_env, j_url, c_url);
		(*jni_env)->DeleteLocalRef(jni_env, j_url);
	}
	(*jni_env)->DeleteLocalRef(jni_env, java_class);
	return return_str;
}

static char* androidWebBrowserGetForwardCountAttrib(Ihandle* ih)
{
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getForwardCount", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;)I");
	jobject web_view = (jobject)ih->handle;
	jint j_item_count = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id, web_view);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);
	return iupStrReturnInt((int)j_item_count);
}

static char* androidWebBrowserGetBackCountAttrib(Ihandle* ih)
{
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getBackCount", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;)I");
	jobject web_view = (jobject)ih->handle;
	jint j_item_count = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id, web_view);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);
	return iupStrReturnInt((int)j_item_count);
}

static int androidWebBrowserSetHTMLAttrib(Ihandle* ih, const char* value)
{
	if(value)
	{
		JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
		jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
		jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "loadHtmlString", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;Ljava/lang/String;)V");

		jstring java_string = (*jni_env)->NewStringUTF(jni_env, value);
		jobject web_view = (jobject)ih->handle;
		(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, web_view, java_string);

		(*jni_env)->DeleteLocalRef(jni_env, java_string);
		(*jni_env)->DeleteLocalRef(jni_env, java_class);
	}
	return 0; /* do not store value in hash table */
}

#if 1
// Not really sure if this can work or how reliable it is:
// https://stackoverflow.com/questions/6058843/android-how-to-select-texts-from-webview
// https://stackoverflow.com/questions/41477548/how-to-copy-the-selected-text-to-clipboard-dynamically-using-android-webview
// https://github.com/btate/BTAndroidWebViewSelection
// https://stackoverflow.com/questions/34804100/how-to-get-the-selected-text-of-webview-in-actionmode-override
static int androidWebBrowserSetCopyAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	jobject web_view = ih->handle;
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "copySelectionToClipboard", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;)V");
	(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, web_view);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);
	return 0;
}
#endif

#if 0
// Not sure if this can work:
// https://stackoverflow.com/questions/11881824/android-programatically-trigger-text-selection-mode-in-a-webview-on-jelly-bean?rq=1
// http://devemat-androidprogramming.blogspot.com/2011/06/selecting-text-with-webview.html
static int androidWebBrowserSetSelectAllAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	return 0;
}
#endif

static int androidWebBrowserSetPrintAttrib(Ihandle* ih, const char* value)
{
	(void)value;

	jobject web_view = ih->handle;
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "print", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;)V");
	(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, web_view);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	return 0;
}

#if 0
static int androidWebBrowserSetZoomAttrib(Ihandle* ih, const char* value)
{
  int zoom;
  if (iupStrToInt(value, &zoom))
    webkit_web_view_set_zoom_level((WebKitWebView*)ih->handle, (float)zoom/100.0f);
  return 0;
}

static char* androidWebBrowserGetZoomAttrib(Ihandle* ih)
{
  int zoom = (int)(webkit_web_view_get_zoom_level((WebKitWebView*)ih->handle) * 100);
  return iupStrReturnInt(zoom);
}
#endif

/*
 Besides implementing the webView:didStartProvisionalLoadForFrame: method to display the page title, you can also use it to display the status, for example, “Loading.” Remember that at this point the content has only been requested, not loaded; therefore, the data source is provisional.
 
 Similarly, implement the webView:didFinishLoadForFrame:, webView:didFailProvisionalLoadWithError:forFrame: and webView:didFailLoadWithError:forFrame: delegate methods to receive notification when a page has been loaded successfully or unsuccessfully. You might want to display a message if an error occurred.
*/
static char* androidWebBrowserGetStatusAttrib(Ihandle* ih)
{
	char* return_str = NULL;
	jobject web_view = ih->handle;
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getLoadStatus", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;)I");

	jint j_int_status = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id, web_view);

	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	/*
	private static final int LoadStatusFinished = 0;
	private static final int LoadStatusFailed = 1;
	private static final int LoadStatusLoading = 2;
	*/
	switch(j_int_status)
	{
		case 0:
		{
			return_str = "COMPLETED";
			break;
		}
		case 2:
		{
			return_str = "LOADING";
			break;
		}
		case 1:
		default:
		{
			return_str = "FAILED";
			break;
		}
	}

	return return_str;
}

static int androidWebBrowserSetReloadAttrib(Ihandle* ih, const char* value)
{
	(void)value;

	jobject web_view = ih->handle;
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "reload", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;)V");
	(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, web_view);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	return 0; /* do not store value in hash table */
}

static int androidWebBrowserSetStopAttrib(Ihandle* ih, const char* value)
{
	(void)value;

	jobject web_view = ih->handle;
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "stopLoading", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;)V");
	(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, web_view);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	return 0; /* do not store value in hash table */
}

static int androidWebBrowserSetBackForwardAttrib(Ihandle* ih, const char* value)
{
	int val;
	if(iupStrToInt(value, &val))
	{
		jint j_val = (jint)val;
		jobject web_view = ih->handle;
		JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
		jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
		jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "goBackOrForward", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;I)V");
		(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, web_view, j_val);
		(*jni_env)->DeleteLocalRef(jni_env, java_class);
	}
	return 0; /* do not store value in hash table */
}

static int androidWebBrowserSetValueAttrib(Ihandle* ih, const char* value)
{
	if(value)
	{
		JNIEnv* jni_env;
		jclass java_class;
		jmethodID method_id;
		char* attribute_value;

		jni_env = iupAndroid_GetEnvThreadSafe();

		{

			java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
			method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "loadUrl", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;Ljava/lang/String;)V");

			jstring java_string = (*jni_env)->NewStringUTF(jni_env, value);
			jobject web_view = (jobject)ih->handle;
			(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, web_view, java_string);

			(*jni_env)->DeleteLocalRef(jni_env, java_string);
			(*jni_env)->DeleteLocalRef(jni_env, java_class);

		}                                          



	}
		__android_log_print(ANDROID_LOG_INFO, "androidWebBrowserSetValueAttrib", "str:%s", value);
	
	return 0; /* do not store value in hash table */
}

static char* androidWebBrowserGetValueAttrib(Ihandle* ih)
{
	char* return_str = NULL;
	jobject web_view = ih->handle;
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getUrl", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;)Ljava/lang/String;");

	jstring j_url = (jstring) ((*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, web_view));
	if(NULL != j_url)
	{
		const char *c_url;
		c_url = (*jni_env)->GetStringUTFChars(jni_env, j_url, NULL);
		return_str = iupStrReturnStr(c_url);
		(*jni_env)->ReleaseStringUTFChars(jni_env, j_url, c_url);
		(*jni_env)->DeleteLocalRef(jni_env, j_url);
	}

	(*jni_env)->DeleteLocalRef(jni_env, java_class);
		__android_log_print(ANDROID_LOG_INFO, "androidWebBrowserGetValueAttrib", "str:%s", return_str);

	return return_str;

}



static char* androidWebBrowserCanGoBackAttrib(Ihandle* ih)
{
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "canGoBack", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;)Z");
	jobject web_view = (jobject)ih->handle;
	jboolean is_true = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, method_id, web_view);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);
	return iupStrReturnBoolean((int)is_true);
}

static char* androidWebBrowserCanGoForwardAttrib(Ihandle* ih)
{
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "canGoForward", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;)Z");
	jobject web_view = (jobject)ih->handle;
	jboolean is_true = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, method_id, web_view);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);
	return iupStrReturnBoolean((int)is_true);
}

static int androidWebBrowserSetBackAttrib(Ihandle* ih, const char* value)
{
	(void)value;

	jobject web_view = ih->handle;
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "goBack", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;)V");
	(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, web_view);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	return 0; /* do not store value in hash table */
}

static int androidWebBrowserSetForwardAttrib(Ihandle* ih, const char* value)
{
	(void)value;

	jobject web_view = ih->handle;
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "goForward", "(Lbr/pucrio/tecgraf/iupweb/IupWebView;)V");
	(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, web_view);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	return 0; /* do not store value in hash table */
}

/*********************************************************************************************/


static int androidWebBrowserMapMethod(Ihandle* ih)
{
    JNIEnv* jni_env;
	jclass java_class;
    jmethodID method_id;
	jobject java_widget;
	char* attribute_value;
   
		__android_log_print(ANDROID_LOG_INFO, "androidWebBrowserMapMethod", "starting");
	jni_env = iupAndroid_GetEnvThreadSafe();



	{

		java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iupweb/IupWebViewHelper");
		method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "createWebView", "(J)Lbr/pucrio/tecgraf/iupweb/IupWebView;");
		java_widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih);

		ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, java_widget));

		(*jni_env)->DeleteLocalRef(jni_env, java_widget);
		(*jni_env)->DeleteLocalRef(jni_env, java_class);

	}
	iupAndroid_AddWidgetToParent(jni_env, ih);

	return IUP_NOERROR;
}

static void androidWebBrowserUnMapMethod(Ihandle* ih)
{
		__android_log_print(ANDROID_LOG_INFO, "androidWebBrowserUnMapMethod", "starting");


	if(ih && ih->handle)
	{
		JNIEnv *jni_env;
		jclass java_class;
		jmethodID method_id;
		jni_env = iupAndroid_GetEnvThreadSafe();

		java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iup/IupCommon");
		method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "removeWidgetFromParent",
												  "(J)V");
		(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong) (intptr_t) ih);
		(*jni_env)->DeleteLocalRef(jni_env, java_class);


		(*jni_env)->DeleteGlobalRef(jni_env, ih->handle);
		ih->handle = NULL;
	}
}






static void androidWebBrowserComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0, natural_h = 0;
  (void)children_expand; /* unset if not a container */

  /* natural size is 1 character */
  iupdrvFontGetCharSize(ih, &natural_w, &natural_h);

  *w = natural_w;
  *h = natural_h;

  // hack for testing
  *w = 480;
  *h = 640;
}

static int androidWebBrowserCreateMethod(Ihandle* ih, void **params)
{
	ih->expand = IUP_EXPAND_BOTH;

#if 0
  (void)params;

  ih->data = iupALLOCCTRLDATA();

  /* default EXPAND is YES */
  ih->expand = IUP_EXPAND_BOTH;
  ih->data->sb = IUP_SB_HORIZ | IUP_SB_VERT;  /* default is YES */

#endif

  return IUP_NOERROR;
}

Iclass* iupWebBrowserNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "webbrowser";
  ic->format = NULL; /* no parameters */
  ic->nativetype  = IUP_TYPECONTROL;
  ic->childtype   = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;   /* has attributes with IDs that must be parsed */

  /* Class functions */
  ic->New = iupWebBrowserNewClass;
  ic->Create = androidWebBrowserCreateMethod;
  ic->Map = androidWebBrowserMapMethod;
  ic->UnMap = androidWebBrowserUnMapMethod;
  ic->ComputeNaturalSize = androidWebBrowserComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "NEWWINDOW_CB", "s");
  iupClassRegisterCallback(ic, "NAVIGATE_CB", "s");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT); 

  /* IupWebBrowser only */
  iupClassRegisterAttribute(ic, "VALUE", androidWebBrowserGetValueAttrib, androidWebBrowserSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKFORWARD", NULL, androidWebBrowserSetBackForwardAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", NULL, androidWebBrowserSetStopAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RELOAD", NULL, androidWebBrowserSetReloadAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HTML", NULL, androidWebBrowserSetHTMLAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATUS", androidWebBrowserGetStatusAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPY", NULL, androidWebBrowserSetCopyAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
//  iupClassRegisterAttribute(ic, "SELECTALL", NULL, androidWebBrowserSetSelectAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
#if 0
  iupClassRegisterAttribute(ic, "ZOOM", androidWebBrowserGetZoomAttrib, androidWebBrowserSetZoomAttrib, NULL, NULL, IUPAF_NO_INHERIT);
#endif
  iupClassRegisterAttribute(ic, "PRINT", NULL, androidWebBrowserSetPrintAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKCOUNT", androidWebBrowserGetBackCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORWARDCOUNT", androidWebBrowserGetForwardCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMHISTORY",  androidWebBrowserGetItemHistoryAttrib,  NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CANGOBACK", androidWebBrowserCanGoBackAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOFORWARD", androidWebBrowserCanGoForwardAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "GOBACK", NULL, androidWebBrowserSetBackAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOFORWARD", NULL, androidWebBrowserSetForwardAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	return ic;
}

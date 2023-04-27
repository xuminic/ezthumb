/** \file
 * \brief MAC Font mapping
 *
 * See Copyright Notice in "iup.h"
 */


#include <stdlib.h>
#include <stdio.h>


#include "iup.h"

#include "iup_str.h"
#include "iup_array.h"
#include "iup_attrib.h"
#include "iup_object.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_assert.h"

#include "iupandroid_drv.h"

#include <jni.h>
#include <android/log.h>



IUP_SDK_API char* iupdrvGetSystemFont(void)
{
  static char systemfont[200] = "";
#if 0
  NSFont *font = [NSFont systemFontOfSize:0];
	NSLog(@"systemfont: %@", font);
  char *name = [[font familyName] UTF8String];
  if(*name)
    strcpy(systemfont,name);
  else
    strcpy(systemfont, "Tahoma, 10");
#endif
  return systemfont;
}


IUP_SDK_API int iupdrvSetStandardFontAttrib(Ihandle* ih, const char* value)
{
#if 0 // iupBaseUpdateSizeFromFont missing
  ImacFont* macfont = macFontCreateNativeFont(ih, value);
  if (!macfont)
    return 1;

	/* If FONT is changed, must update the SIZE attribute */
	iupBaseUpdateAttribFromFont(ih);

  /* FONT attribute must be able to be set before mapping, 
      so the font is enable for size calculation. */
  if (ih->handle && (ih->iclass->nativetype != IUP_TYPEVOID)) {
	
  }
#endif
  return 1;
}

IUP_SDK_API void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int *w, int *h)
{
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iup/IupFontHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getMultiLineStringSize", "(JLjava/lang/Object;ILjava/lang/String;)Landroid/graphics/Rect;");
	jobject native_object = (jobject)ih->handle;
	jstring java_string = (*jni_env)->NewStringUTF(jni_env, str);
	jobject j_rect = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, native_object, 0, java_string);
	(*jni_env)->DeleteLocalRef(jni_env, java_string);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	java_class = (*jni_env)->GetObjectClass(jni_env, j_rect);
	method_id = (*jni_env)->GetMethodID(jni_env, java_class, "width", "()I");
	jint j_width = (*jni_env)->CallIntMethod(jni_env, j_rect, method_id);
	method_id = (*jni_env)->GetMethodID(jni_env, java_class, "height", "()I");
	jint j_height = (*jni_env)->CallIntMethod(jni_env, j_rect, method_id);
	(*jni_env)->DeleteLocalRef(jni_env, j_rect);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);
	__android_log_print(ANDROID_LOG_INFO, "iupdrvFontGetMultiLineStringSize", "the_width:%d j_height:%d, for str=%s", (int)j_width, (int)j_height, str);

	if (w) *w = (int)j_width;
	if (h) *h = (int)j_height;
}


IUP_SDK_API void iupdrvFontGetTextSize(const char* font, const char* str, int len, int *w, int *h)
{
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iup/IupFontHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getTextSize", "(JLjava/lang/String;ILjava/lang/String;)Landroid/graphics/Rect;");
	jstring java_font_name = (*jni_env)->NewStringUTF(jni_env, font);
	jstring java_string = (*jni_env)->NewStringUTF(jni_env, str);
	jobject j_rect = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, java_font_name, java_string);
	(*jni_env)->DeleteLocalRef(jni_env, java_font_name);
	(*jni_env)->DeleteLocalRef(jni_env, java_string);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	java_class = (*jni_env)->GetObjectClass(jni_env, j_rect);
	method_id = (*jni_env)->GetMethodID(jni_env, java_class, "width", "()I");
	jint j_width = (*jni_env)->CallIntMethod(jni_env, j_rect, method_id);
	method_id = (*jni_env)->GetMethodID(jni_env, java_class, "height", "()I");
	jint j_height = (*jni_env)->CallIntMethod(jni_env, j_rect, method_id);
	(*jni_env)->DeleteLocalRef(jni_env, j_rect);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);
	__android_log_print(ANDROID_LOG_INFO, "iupdrvFontGetTextSize", "the_width:%d j_height:%d, for str=%s", (int)j_width, (int)j_height, str);

	if (w) *w = (int)j_width;
	if (h) *h = (int)j_height;
}

IUP_SDK_API int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iup/IupFontHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getStringWidth", "(JLjava/lang/Object;ILjava/lang/String;)I");
	jobject native_object = (jobject)ih->handle;
	jstring java_string = (*jni_env)->NewStringUTF(jni_env, str);
	jint j_width = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, native_object, 0, java_string);
	(*jni_env)->DeleteLocalRef(jni_env, java_string);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);
	__android_log_print(ANDROID_LOG_INFO, "iupdrvFontGetStringWidth", "the_width:%d for str:%s", (int)j_width, str);

	return ((int)j_width);
}

IUP_SDK_API void iupdrvFontGetCharSize(Ihandle* ih, int *charwidth, int *charheight)
{
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = (*jni_env)->FindClass(jni_env, "br/pucrio/tecgraf/iup/IupFontHelper");
	jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getCharSize", "(JLjava/lang/Object;I)Landroid/graphics/Rect;");
	jobject native_object = (jobject)ih->handle;
	jobject j_rect = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, native_object, 0);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	java_class = (*jni_env)->GetObjectClass(jni_env, j_rect);
	method_id = (*jni_env)->GetMethodID(jni_env, java_class, "width", "()I");
	jint j_width = (*jni_env)->CallIntMethod(jni_env, j_rect, method_id);
	method_id = (*jni_env)->GetMethodID(jni_env, java_class, "height", "()I");
	jint j_height = (*jni_env)->CallIntMethod(jni_env, j_rect, method_id);
	(*jni_env)->DeleteLocalRef(jni_env, j_rect);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);
	__android_log_print(ANDROID_LOG_INFO, "iupdrvFontGetCharSize", "the_width:%d j_height:%d", (int)j_width, (int)j_height);

	if (charwidth)  *charwidth = (int)j_width;
	if (charheight) *charheight = (int)j_height;
}

void iupdrvFontInit(void)
{
}

void iupdrvFontFinish(void)
{
}


IUP_SDK_API int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
	return 1;
}

IUP_SDK_API const char* iupdrvGetFontAttrib(Ihandle* ih)
{
	return NULL;
}




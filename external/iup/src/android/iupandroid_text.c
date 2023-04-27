/** \file
 * \brief Text Control
 *
 * See Copyright Notice in "iup.h"
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <stdbool.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_mask.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_key.h"
#include "iup_array.h"
#include "iup_text.h"

#include "iupandroid_drv.h"

#include <android/log.h>
#include "iupandroid_jnimacros.h"
#include "iupandroid_jnicacheglobals.h"

IUPJNI_DECLARE_CLASS_STATIC(IupTextHelper);


typedef enum
{
	IUPANDROIDTEXTSUBTYPE_UNKNOWN = -1,
	IUPANDROIDTEXTSUBTYPE_FIELD,
	IUPANDROIDTEXTSUBTYPE_VIEW,
	IUPANDROIDTEXTSUBTYPE_STEPPER,
} IupAndroidTextSubType;

/*
 Each IUP list subtype requires a completely different Cocoa native widget.
 This function provides a consistent and centralized way to distinguish which subtype we need.
 */
static IupAndroidTextSubType androidTextGetSubType(Ihandle* ih)
{
	if(ih->data->is_multiline)
	{
		return IUPANDROIDTEXTSUBTYPE_VIEW;
	}
	else if(iupAttribGetBoolean(ih, "SPIN"))
	{
		return IUPANDROIDTEXTSUBTYPE_STEPPER;
	}
	else
	{
		return IUPANDROIDTEXTSUBTYPE_FIELD;
	}
	return IUPANDROIDTEXTSUBTYPE_UNKNOWN;
}


void iupdrvTextAddSpin(Ihandle* ih, int *w, int h)
{

	
}

void iupdrvTextAddBorders(Ihandle* ih, int *x, int *y)
{

	if(x)
	{
		*x += 10;
	}
	if(y)
	{
		*y += 10;
	}

}


void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int *pos)
{
 
}

void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int *lin, int *col)
{

	
}



void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
	
	
  return NULL;
}

void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state)
{

	
}

void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
{

	
}


static int androidTextSetValueAttrib(Ihandle* ih, const char* value)
{
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setText);
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "br/pucrio/tecgraf/iup/IupTextHelper");
	jmethodID method_id = NULL;
	char* attribute_value = NULL;
	jobject java_widget = NULL;

	if(NULL == value)
	{
		value = "";
	}

	__android_log_print(ANDROID_LOG_INFO, "androidTextSetValueAttrib", "str: %s", value);
	IupAndroidTextSubType sub_type = androidTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPANDROIDTEXTSUBTYPE_VIEW:
		case IUPANDROIDTEXTSUBTYPE_FIELD:
		{
			method_id = IUPJNI_GetStaticMethodID(IupTextHelper_setText, jni_env, java_class, "setText", "(JLandroid/widget/EditText;Ljava/lang/String;)V");

			jstring j_string = (*jni_env)->NewStringUTF(jni_env, value);

			(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id,
				(jlong)(intptr_t) ih,
					(jobject)ih->handle,
					j_string

			);

			(*jni_env)->DeleteLocalRef(jni_env, j_string);


			break;
		}

		case IUPANDROIDTEXTSUBTYPE_STEPPER:
		{
			break;
		}
		default:
		{
			break;
		}
	}

	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	return 0;
}

static char* androidTextGetValueAttrib(Ihandle* ih)
{
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getText);
	char* value = NULL;

	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "br/pucrio/tecgraf/iup/IupTextHelper");
	jmethodID method_id = NULL;


	IupAndroidTextSubType sub_type = androidTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPANDROIDTEXTSUBTYPE_VIEW:
		case IUPANDROIDTEXTSUBTYPE_FIELD:
		{
			method_id = IUPJNI_GetStaticMethodID(IupTextHelper_getText, jni_env, java_class, "getText",
													  "(JLandroid/widget/EditText;)Ljava/lang/String;");
			jstring j_string = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id,
					(jlong)(intptr_t)ih,
					(jobject)ih->handle
			);


			const char* c_string = (*jni_env)->GetStringUTFChars(jni_env, j_string, NULL);
			if(NULL != c_string)
			{
				value = iupStrReturnStr(c_string);
			}
			else
			{
				value = NULL;
			}

			(*jni_env)->ReleaseStringUTFChars(jni_env, j_string, c_string);
			(*jni_env)->DeleteLocalRef(jni_env, j_string);


			break;
		}
		case IUPANDROIDTEXTSUBTYPE_STEPPER:
		{
			break;
		}
		default:
		{
			break;
		}
	}

	(*jni_env)->DeleteLocalRef(jni_env, java_class);


	return value;
}


static int androidTextSetCueBannerAttrib(Ihandle *ih, const char *value)
{
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setCueBanner);
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "br/pucrio/tecgraf/iup/IupTextHelper");
	jmethodID method_id = NULL;
	char* attribute_value = NULL;
	jobject java_widget = NULL;

	if(NULL == value)
	{
		value = "";
	}

	IupAndroidTextSubType sub_type = androidTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPANDROIDTEXTSUBTYPE_VIEW:
		case IUPANDROIDTEXTSUBTYPE_FIELD:
		{
			method_id = IUPJNI_GetStaticMethodID(IupTextHelper_setCueBanner, jni_env, java_class, "setCueBanner", "(JLandroid/widget/EditText;Ljava/lang/String;)V");

			jstring j_string = (*jni_env)->NewStringUTF(jni_env, value);

			(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id,
					(jlong)(intptr_t) ih,
					(jobject)ih->handle,
					j_string

			);

			(*jni_env)->DeleteLocalRef(jni_env, j_string);


			break;
		}

		case IUPANDROIDTEXTSUBTYPE_STEPPER:
		{
			break;
		}
		default:
		{
			break;
		}
	}

	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	return 0;
}

static int androidTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setReadOnlyMultiLine);
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setReadOnlySingleLine);
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "br/pucrio/tecgraf/iup/IupTextHelper");
	jmethodID method_id = NULL;

	bool is_read_only = (bool)iupStrBoolean(value);

	IupAndroidTextSubType sub_type = androidTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPANDROIDTEXTSUBTYPE_VIEW:
		{
			method_id = IUPJNI_GetStaticMethodID(IupTextHelper_setReadOnlyMultiLine, jni_env, java_class, "setReadOnlyMultiLine", "(JLandroid/widget/EditText;Z)V");

			(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id,
					(jlong)(intptr_t) ih,
					(jobject)ih->handle,
					is_read_only

			);

			break;
		}
		case IUPANDROIDTEXTSUBTYPE_FIELD:
		{
			method_id = IUPJNI_GetStaticMethodID(IupTextHelper_setReadOnlySingleLine, jni_env, java_class, "setReadOnlySingleLine", "(JLandroid/widget/EditText;Z)V");

			(*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id,
					(jlong)(intptr_t) ih,
					(jobject)ih->handle,
					is_read_only

			);



			break;
		}
		case IUPANDROIDTEXTSUBTYPE_STEPPER:
		{
			break;
		}
		default:
		{
			break;
		}
	}
	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	return 0;
}

static char* androidTextGetReadOnlyAttrib(Ihandle* ih)
{
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getReadOnlyMultiLine);
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getReadOnlySingleLine);
	bool is_read_only = 0;

	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "br/pucrio/tecgraf/iup/IupTextHelper");
	jmethodID method_id = NULL;


	IupAndroidTextSubType sub_type = androidTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPANDROIDTEXTSUBTYPE_VIEW:
		{
			method_id = IUPJNI_GetStaticMethodID(IupTextHelper_getReadOnlyMultiLine, jni_env, java_class, "getReadOnlyMultiLine", "(JLandroid/widget/EditText;)Z");

			is_read_only = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, method_id,
					(jlong)(intptr_t) ih,
					(jobject)ih->handle

			);


			break;
		}
		case IUPANDROIDTEXTSUBTYPE_FIELD:
		{
			method_id = IUPJNI_GetStaticMethodID(IupTextHelper_getReadOnlySingleLine, jni_env, java_class, "getReadOnlySingleLine", "(JLandroid/widget/EditText;)Z");

			is_read_only = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, method_id,
					(jlong)(intptr_t) ih,
					(jobject)ih->handle

			);

			break;
		}
		case IUPANDROIDTEXTSUBTYPE_STEPPER:
		{
			break;
		}
		default:
		{
			break;
		}
	}
	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	return iupStrReturnBoolean(is_read_only);
}




static int androidTextMapMethod(Ihandle* ih)
{
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_createMultiLineText);
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_createSpinnerText);
	IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_createSingleLineText);

	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "br/pucrio/tecgraf/iup/IupTextHelper");
	jmethodID method_id = NULL;
	char* attribute_value = NULL;
	jobject java_widget = NULL;



	
	if(ih->data->is_multiline)
	{

		method_id = IUPJNI_GetStaticMethodID(IupTextHelper_createMultiLineText, jni_env, java_class, "createMultiLineText",
												  "(J)Landroid/widget/EditText;");
		java_widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id,
														 (jlong) (intptr_t) ih);

		ih->handle = (jobject) ((*jni_env)->NewGlobalRef(jni_env, java_widget));
		__android_log_print(ANDROID_LOG_INFO, "androidTextMapMethod", "got multiline text: %p",
							ih->handle);

	}
	else if(iupAttribGetBoolean(ih, "SPIN"))
	{
		// FIXME: This is just a single line text view. May need to change return type.
		method_id = IUPJNI_GetStaticMethodID(IupTextHelper_createSpinnerText, jni_env, java_class, "createSpinnerText",
												  "(J)Landroid/widget/EditText;");
		java_widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id,
														 (jlong) (intptr_t) ih);

		ih->handle = (jobject) ((*jni_env)->NewGlobalRef(jni_env, java_widget));
	}
	else
	{
		method_id = IUPJNI_GetStaticMethodID(IupTextHelper_createSingleLineText, jni_env, java_class, "createSingleLineText",
												  "(J)Landroid/widget/EditText;");
		java_widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id,
														 (jlong) (intptr_t) ih);

		ih->handle = (jobject) ((*jni_env)->NewGlobalRef(jni_env, java_widget));
		__android_log_print(ANDROID_LOG_INFO, "androidTextMapMethod", "got single line text: %p",
							ih->handle);
	}

	(*jni_env)->DeleteLocalRef(jni_env, java_widget);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);







	iupAndroid_AddWidgetToParent(jni_env, ih);


	return IUP_NOERROR;
}


static void androidTextUnMapMethod(Ihandle* ih)
{
	if(ih && ih->handle)
	{
		IUPJNI_DECLARE_METHOD_ID_STATIC(IupCommon_removeWidgetFromParent);
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


void iupdrvTextInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = androidTextMapMethod;
	ic->UnMap = androidTextUnMapMethod;

#if 0

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtkTextSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  /* IupText only */
  iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, gtkTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
#endif
  iupClassRegisterAttribute(ic, "VALUE", androidTextGetValueAttrib, androidTextSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
#if 0
  iupClassRegisterAttribute(ic, "LINEVALUE", gtkTextGetLineValueAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", gtkTextGetSelectedTextAttrib, gtkTextSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", gtkTextGetSelectionAttrib, gtkTextSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", gtkTextGetSelectionPosAttrib, gtkTextSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", gtkTextGetCaretAttrib, gtkTextSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", gtkTextGetCaretPosAttrib, gtkTextSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, gtkTextSetInsertAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, gtkTextSetAppendAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
#endif
  iupClassRegisterAttribute(ic, "READONLY", androidTextGetReadOnlyAttrib, androidTextSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
#if 0
  iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, gtkTextSetNCAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, gtkTextSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, gtkTextSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, gtkTextSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMIN", NULL, gtkTextSetSpinMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, gtkTextSetSpinMaxAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, gtkTextSetSpinIncAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", gtkTextGetSpinValueAttrib, gtkTextSetSpinValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", gtkTextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINECOUNT", gtkTextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* IupText Windows and GTK only */
  iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iupTextSetAddFormatTagAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iupTextSetAddFormatTagHandleAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, gtkTextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", iupTextGetFormattingAttrib, iupTextSetFormattingAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", gtkTextGetOverwriteAttrib, gtkTextSetOverwriteAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REMOVEFORMATTING", NULL, gtkTextSetRemoveFormattingAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSIZE", NULL, gtkTextSetTabSizeAttrib, "8", NULL, IUPAF_DEFAULT);  /* force new default value */
  iupClassRegisterAttribute(ic, "PASSWORD", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
#endif

	iupClassRegisterAttribute(ic, "CUEBANNER", NULL, androidTextSetCueBannerAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

#if 0
  /* Not Supported */
  iupClassRegisterAttribute(ic, "FILTER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
#endif
	
}

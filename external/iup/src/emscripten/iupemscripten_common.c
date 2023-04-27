/** \file
 * \brief Emscripten Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>              
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdarg.h>

#include "iup.h"
#include "iup_varg.h"
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

#include "iupemscripten_drv.h"


static Itable* s_integerIdToIhandleMap = NULL;

void iupEmscripten_InitializeInternalGlobals()
{
	if(!s_integerIdToIhandleMap)
	{
		// FIXME: Is there a place to free the memory?
		s_integerIdToIhandleMap = iupTableCreate(IUPTABLE_POINTERINDEXED);
	}
} 
void iupEmscripten_DestroyInternalGlobals()
{

	if(s_integerIdToIhandleMap)
	{
		iupTableDestroy(s_integerIdToIhandleMap);
		s_integerIdToIhandleMap = NULL;
	}
}
void iupEmscripten_SetIntKeyForIhandleValue(int handle_id, Ihandle* ih)
{
	iupTableSet(s_integerIdToIhandleMap, (const char*)((intptr_t)handle_id), ih, IUPTABLE_POINTER);
}
void iupEmscripten_RemoveIntKeyFromIhandleMap(int handle_id)
{
	iupTableRemove(s_integerIdToIhandleMap, (const char*)((intptr_t)handle_id));
}
Ihandle* iupEmscripten_GetIhandleValueForKey(int handle_id)
{
	Ihandle* ih = (Ihandle*)((intptr_t)iupTableGet(s_integerIdToIhandleMap, (const char*)((intptr_t)handle_id)));
	return ih;
}


extern void emjsCommon_Log(char* message);
void iupEmscripten_Log(const char* restrict format, ...) {
  va_list argList;

  va_start(argList, format);
  char *my_string;

  vasprintf (&my_string, format, argList); // this mallocs under the hood - need to free later
  emjsCommon_Log(my_string);
  /* printf(format, argList); */
  va_end(argList);
  free(my_string);
}

extern void emjsCommon_IupLog(int priority, char* message);
IUP_API void IupLogV(const char* type, const char* format, va_list arglist)
{
	enum IUPLOG_LEVEL
	{
		IUPLOG_LEVEL_LOG = 0,
		IUPLOG_LEVEL_DEBUG,
		IUPLOG_LEVEL_INFO,
		IUPLOG_LEVEL_WARN,
		IUPLOG_LEVEL_ERROR
	};
	enum IUPLOG_LEVEL priority = IUPLOG_LEVEL_LOG;

	if (iupStrEqualNoCase(type, "DEBUG"))
	{
		priority = IUPLOG_LEVEL_DEBUG;
	}
	else if (iupStrEqualNoCase(type, "ERROR"))
	{
		priority = IUPLOG_LEVEL_ERROR;
	}
	else if (iupStrEqualNoCase(type, "WARNING"))
	{
		priority = IUPLOG_LEVEL_WARN;
	}
	else if (iupStrEqualNoCase(type, "INFO"))
	{
		priority = IUPLOG_LEVEL_INFO;
	}

	char *my_string;
	vasprintf(&my_string, format, arglist); // this mallocs under the hood - need to free later

	emjsCommon_IupLog((int)priority, my_string);

	free(my_string);
	
}

IUP_API void IupLog(const char* type, const char* format, ...)
{
  va_list arglist;
  va_start(arglist, format);
  IupLogV(type, format, arglist);
  va_end(arglist);
}

extern void emjsCommon_AddWidgetToDialog(int parent_id, int child_id);
extern void emjsCommon_AddCompoundToDialog(int parent_id, int32_t elem_array[], size_t num_elems);
extern void emjsCommon_AddWidgetToWidget(int parent_id, int child_id);
extern void emjsCommon_AddCompoundToWidget(int parent_id, int32_t elem_array[], size_t num_elems);
extern void emjsCommon_SetPosition(int handle_id, int x, int y, int height, int width);
void iupEmscripten_AddWidgetToParent(Ihandle* ih)
{
	Ihandle* parent_ih = iupChildTreeGetNativeParent(ih);
	//InativeHandle* parent_native_handle = iupChildTreeGetNativeParentHandle(ih);
	// No parent? Probably need to assert here.
	if(!parent_ih)
	{
		return;
	}

	InativeHandle* parent_native_handle = parent_ih->handle;
	InativeHandle* child_handle = ih->handle;

	int32_t parent_id = 0;
	int32_t child_id = 0;
	_Bool parent_is_dialog = false;
	if(parent_native_handle)
	{
		parent_id = parent_native_handle->handleID;
		if(parent_ih->iclass->nativetype == IUP_TYPEDIALOG)
		{
			parent_is_dialog = true;
		}
	}
	if(child_handle)
	{
		child_id = child_handle->handleID;
	}
	if(parent_is_dialog)
	{

    /* emjsCommon_Alert("is compound"); */
    /* emjsCommon_Alert(child_handle->isCompound); */
		if (child_handle->isCompound) {
      /* emjsCommon_Alert("compound dialog"); */
      emjsCommon_AddCompoundToDialog(parent_id, child_handle->compoundHandleIDArray, child_handle->numElemsIfCompound);
    }
    else {
      /* emjsCommon_Alert("single dialog"); */
      emjsCommon_AddWidgetToDialog(parent_id, child_id);
    }
	}
	else
	{
		if (child_handle->isCompound) {
      /* emjsCommon_Alert("compound widget"); */
      emjsCommon_AddCompoundToWidget(parent_id, child_handle->compoundHandleIDArray, child_handle->numElemsIfCompound);
    }
    else {
      /* emjsCommon_Alert("single widget"); */
      emjsCommon_AddWidgetToWidget(parent_id, child_id);
    }
	}

}

extern void emjsCommon_SetFgColor(int handle_id, unsigned char r, unsigned char g, unsigned char b);
  // matzy: send to JS?
  

#if 0
  GdkRGBA rgba;

  iupgdkRGBASet(&rgba, r, g, b);

  gtk_widget_override_color(handle, GTK_STATE_FLAG_NORMAL, &rgba);
  gtk_widget_override_color(handle, GTK_STATE_ACTIVE, &rgba);
  gtk_widget_override_color(handle, GTK_STATE_PRELIGHT, &rgba);
  
  GtkRcStyle *rc_style;  
  GdkColor color;

  iupgdkColorSet(&color, r, g, b);

  rc_style = gtk_widget_get_modifier_style(handle);  

  rc_style->fg[GTK_STATE_ACTIVE] = rc_style->fg[GTK_STATE_NORMAL] = rc_style->fg[GTK_STATE_PRELIGHT] = color;
  rc_style->text[GTK_STATE_ACTIVE] = rc_style->text[GTK_STATE_NORMAL] = rc_style->text[GTK_STATE_PRELIGHT] = color;

  rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_TEXT | GTK_RC_FG;
  rc_style->color_flags[GTK_STATE_ACTIVE] |= GTK_RC_TEXT | GTK_RC_FG;
  rc_style->color_flags[GTK_STATE_PRELIGHT] |= GTK_RC_TEXT | GTK_RC_FG;

  /* do not set at CHILD_CONTAINER */
  gtk_widget_modify_style(handle, rc_style);
#endif
//}

IUP_SDK_API void iupdrvActivate(Ihandle* ih)
{

}

IUP_SDK_API void iupdrvReparent(Ihandle* ih)
{

	
}


IUP_SDK_API void iupdrvBaseLayoutUpdateMethod(Ihandle *ih)
{
//	iupEmscripten_Log("x:%d, y:%d, w:%d, h:%d, id: %d", ih->x,ih->y,ih->currentwidth,ih->currentheight,ih->handle->handleID);

	if (ih->handle->isCompound)
	{
		for (int i = 0; i < ih->handle->numElemsIfCompound; i++)
		{
			// Our JS implementation uses elem which doesn't exist for Windows (Dialogs). Avoid for safety.
			if(ih->iclass->nativetype != IUP_TYPEDIALOG)
			{
				// Set element's position on screen
				emjsCommon_SetPosition(ih->handle->compoundHandleIDArray[i],ih->x,ih->y,ih->currentwidth,ih->currentheight);
			}
		}
	}
	else
	{
		// Our JS implementation uses elem which doesn't exist for Windows (Dialogs). Avoid for safety.
		if(ih->iclass->nativetype != IUP_TYPEDIALOG)
		{
			// Set element's position on screen
			emjsCommon_SetPosition(ih->handle->handleID,ih->x,ih->y,ih->currentwidth,ih->currentheight);
		}
	}


  //TODO Calculate size and return to ih
  


  /*******************BEGIN IF 0 ***/
	/* id parent_native_handle = iupChildTreeGetNativeParentHandle(ih); */
#if 0
	NSView* parent_view = nil;
	if([parent_native_handle isKindOfClass:[NSWindow class]])
	{
		NSWindow* parent_window = (NSWindow*)parent_native_handle;
		parent_view = [parent_window contentView];
	}
	else if([parent_native_handle isKindOfClass:[NSView class]])
	{
		parent_view = (NSView*)parent_native_handle;
	}
	else
	{
		NSCAssert(1, @"Unexpected type for parent widget");
		@throw @"Unexpected type for parent widget";
	}



	id child_handle = ih->handle;
	NSView* the_view = nil;
	if([child_handle isKindOfClass:[NSView class]])
	{
		the_view = (NSView*)child_handle;
	}
	else if([child_handle isKindOfClass:[CALayer class]])
	{
		NSCAssert(1, @"CALayer not implemented");
		@throw @"CALayer not implemented";
	}
	else
	{
		NSCAssert(1, @"Unexpected type for parent widget");
		@throw @"Unexpected type for parent widget";
	}
	
	
//	iupgtkNativeContainerMove((GtkWidget*)parent, widget, x, y);

//	iupgtkSetPosSize(GTK_CONTAINER(parent), widget, ih->x, ih->y, ih->currentwidth, ih->currentheight);

	/*
	CGSize fitting_size = [the_view fittingSize];
	ih->currentwidth = fitting_size.width;
	ih->currentheight = fitting_size.height;
*/
	
	NSRect parent_rect = [parent_view frame];

	NSRect the_rect = NSMakeRect(
		ih->x,
		// Need to invert y-axis, and also need to shift/account for height of widget because that is also lower-left instead of upper-left.
		parent_rect.size.height - ih->y - ih->currentheight,
		ih->currentwidth,
		ih->currentheight
	);
	[the_view setFrame:the_rect];
//	[the_view setBounds:the_rect];
	
	
#endif
  /***** END IF ***********************************/




}

IUP_SDK_API void iupdrvBaseUnMapMethod(Ihandle* ih)
{
	// Why do I need this when everything else has its own UnMap method?
	//NSLog(@"iupdrvBaseUnMapMethod not implemented. Might be leaking");
}

IUP_SDK_API void iupdrvDisplayUpdate(Ihandle *ih)
{
	// call ViewGroup.invalidate()

}

IUP_SDK_API void iupdrvDisplayRedraw(Ihandle *ih)
{
	iupdrvDisplayUpdate(ih);
}

IUP_SDK_API void iupdrvScreenToClient(Ihandle* ih, int *x, int *y)
{
}



IUP_SDK_API int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  return 0;
}

IUP_SDK_API void iupdrvSetVisible(Ihandle* ih, int visible)
{
}

IUP_SDK_API int iupdrvIsVisible(Ihandle* ih)
{
	return 1;
}

IUP_SDK_API int iupdrvIsActive(Ihandle *ih)
{
  return 1;
}

IUP_SDK_API void iupdrvSetActive(Ihandle* ih, int enable)
{
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

IUP_SDK_API int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{

	

  /* DO NOT NEED TO UPDATE GTK IMAGES SINCE THEY DO NOT DEPEND ON BGCOLOR */

  return 1;
}


IUP_SDK_API int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;
  emjsCommon_SetFgColor(ih->handle->handleID, r, g, b);

  return 1;
}

IUP_SDK_API int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{

  return 0;
}


IUP_SDK_API int iupdrvGetScrollbarSize(void)
{

  return 0;
}

IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle *ih, const char* title)
{
}


IUP_SDK_API void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
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

IUP_SDK_API void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
	
}

IUP_SDK_API void iupdrvClientToScreen(Ihandle* ih, int *x, int *y)
{
	
}

IUP_SDK_API void iupdrvPostRedraw(Ihandle *ih)
{

}

IUP_SDK_API void iupdrvRedrawNow(Ihandle *ih)
{

}
IUP_SDK_API void iupdrvSendKey(int key, int press)
{
	
}
IUP_SDK_API void iupdrvSendMouse(int x, int y, int bt, int status)
{
	
}
IUP_SDK_API void iupdrvSleep(int time)
{
	
}
IUP_SDK_API void iupdrvWarpPointer(int x, int y)
{
	
}

// pthreads are planned for Emscripten, but Spectre/Meltdown have delayed things.
// But it is likely this will eventually come.
// Right now, it is not obvious to me how we should implement this.
// I see other people need this exact same functionality, so optimistically, I expect Emscripten to support this functionality if it doesn't already.
// My naive solution is simply to convert a pointer address to intptr_t and pass it through, hoping that the value is legitimate across threads, 
// hoping the implementation already put the value in SharedBufferArray and everything just falls into place.
// If this does not work, we may need to look for Emscripten APIs such as
// emscripten_async_run_in_main_runtime_thread_
// https://github.com/emscripten-core/emscripten/issues/8212
// https://games.natestuff.com/node_modules/emcc/emsdk/emscripten/1.37.40/tests/pthread/test_pthread_run_on_main_thread.cpp
// That seems to do almost exactly what we need, except I don't know know how to express new signatures.
// The implementation supports parameters for EM_FUNC_SIG_PARAM_I64,
// so it appears there should be a way to construct other signtures.
// Like above, we could try to convert pointers to intptr_t and hope everything just falls into place.
// Another idea is we can try to do the SharedBufferArray stuff manually in WebAssembly.
// https://dzone.com/articles/webassembly-web-workers
// There may also be other ideas which we can collect here.
// https://groups.google.com/forum/#!topic/emscripten-discuss/atlPQCtAJFc

// For now, since pthreads are not really usable due to all browsers disabling SharedBufferArrays due to Spectre/Meltdown,
// we just need this API to do something sensible for the single-threaded case.
// 
// use -s USE_PTHREADS=1 to compile this path
#ifdef __EMSCRIPTEN_PTHREADS__
#include <emscripten/threading.h>

#warning "IupPostMessage using __EMSCRIPTEN_PTHREADS__ is not fully tested (main thread case, but not worker thread case), and the Emscripten APIs we are using to implement this are still in flux."

	// Idea 1: Can we call just call postMessage
	/*
		intptr_t ihandle_intptr = (intptr)ih;
		intptr_t message_data_intptr = (intptr_t)message_data;
		EM_ASM_({
			self.postMessage([$0, UTF8ToString($1), $2, $3, $4]);
		}, ihandle_intptr, s, i, d, message_data_intptr);

	*/
	// The problem is we need to implement a handler on the main thread.
    // But I think we need a handle to the worker thread on the main thread so we can implement onmessage for it.
	// I'm not sure how to do that. Maybe there is a way to convert "this" current thread to get the web worker handle?


	// Idea 2: Use emscripten_async_run_in_main_runtime_thread_
	// https://git.sprintf.io/emscripten-ports/emscripten/commit/cdf7bc5149e5462e255d39cd05f5c4cd7022be92

	// Attempt 1 failed because Emscripten changed the API. It appears in the older version, you could construct your own signatures. But now, it looks like you must use the pre-built ones.

/* // Idea 2: Attempt 1: Craft my own signature
#define IUP_POST_MESSAGE_EM_FUNC_SIG (EM_FUNC_SIG_RETURN_VALUE_V | EM_FUNC_SIG_WITH_N_PARAMETERS(5) | EM_FUNC_SIG_SET_PARAM(0, EM_FUNC_SIG_PARAM_I64) | EM_FUNC_SIG_SET_PARAM(1, EM_FUNC_SIG_PARAM_I64) | EM_FUNC_SIG_SET_PARAM(2, EM_FUNC_SIG_PARAM_I) | EM_FUNC_SIG_SET_PARAM(3, EM_FUNC_SIG_PARAM_D) | EM_FUNC_SIG_SET_PARAM(4, EM_FUNC_SIG_PARAM_I64))
//#include <assert.h>
static void postMessageCallback(int64_t ihandle_intptr, int64_t string_intptr, int i, double d, int64_t message_data_intptr)
{
//	assert(emscripten_is_main_runtime_thread());

	Ihandle* ih = (Ihandle*)((intptr_t)ihandle_intptr);
	char* s = (char*)((intptr_t)string_intptr);
	void* message_data = (void*)((intptr_t)message_data_intptr);
	IFnsid post_message_callback = (IFnsid)IupGetCallback(ih, "POSTMESSAGE_CB");
	if(post_message_callback)
	{
		post_message_callback(ih, s, i, d);
	}
	// We made our own copy of the string in IupPostMessage, so we need to free it now that we are done with it.
	free(s);
}

IUP_SDK_API void IupPostMessage(Ihandle* ih, const char* s, int i, double d)
{
	void* message_data = NULL; // TODO: Restore message_data as public APIs



	// There is no parameter for strings. So we need to shove a pointer as an integer. But the string may get freed before the callback, so we need to duplicate it.
	char* s_copy = strdup(s);
	intptr_t ihandle_intptr = (intptr_t)ih;
	intptr_t string_intptr = (intptr_t)s_copy;
	intptr_t message_data_intptr = (intptr_t)message_data;
	emscripten_async_run_in_main_runtime_thread(IUP_POST_MESSAGE_EM_FUNC_SIG, postMessageCallback, (int64_t)ihandle_intptr, (int64_t)string_intptr, i, d, (int64_t)message_data_intptr);
}
*/

// Idea 2: Attempt 2: Use an existing signature. 
// Because I don't have pointer/64-bit types, and not enough parameters, 
// I will split a 64-bit number into two 32-bit numbers.
// NOTE: I think I may have over-did this. In the web, I don't think we will ever get 64-bit pointers, only 32-bit. So all the 64-bit/32-bit unpacking/packing was probably overkill.

struct IupEmscriptenPostMessagePayload
{
	Ihandle* iHandle;
	void* messageData;
	double usrDouble;
	char* usrStr;
	int usrInt;
  void* usrVoidP;
};

static void postMessageCallback(int high_bits, int low_bits)
{
//	assert(emscripten_is_main_runtime_thread());

	// Join two 32-bit numbers into 64-bit number
	int64_t payload_64 = ((int64_t)high_bits) << 32 | (int32_t)low_bits;
	intptr_t payload_intptr = (intptr_t)payload_64;
	struct IupEmscriptenPostMessagePayload* payload = (struct IupEmscriptenPostMessagePayload*)payload_intptr;

	Ihandle* ih = payload->iHandle;
	char* s = payload->usrStr;
	int i = payload->usrInt;
	double d = payload->usrDouble;
  void* p = payload->usrVoidP;

	IFnsidv cb = (IFnsidv)IupGetCallback(ih, "POSTMESSAGE_CB");
  if(cb)
	{
    cb(ih, s, i, d, p);
	}
	// We made our own copy of the string in IupPostMessage, so we need to free it now that we are done with it.
  if (payload->usrStr) free(payload->usrStr);
	free(payload);
}

IUP_SDK_API void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
	struct IupEmscriptenPostMessagePayload* payload = (struct IupEmscriptenPostMessagePayload*)calloc(1, sizeof(struct IupEmscriptenPostMessagePayload));

	// There is no parameter for strings. So we need to shove a pointer as an integer. But the string may get freed before the callback, so we need to duplicate it.

	payload->iHandle = ih;
  payload->usrStr = iupStrDup(s);    //s can be NULL
	payload->usrInt = i;
	payload->usrDouble = d;	
  payload->usrVoidP = p;

	// Split 64-bit pointer into two 32-bit pointers.
	intptr_t payload_intptr = (intptr_t)payload;
	int64_t payload_64 = (int64_t)payload_intptr;

	int32_t upper_bits = (int32_t)(payload_64>>32);
	int32_t lower_bits = (int32_t)payload_64;

	// TODO: Looking at the emscripten_async_run_in_main_runtime_thread implementation, if we are on the main thread, the code immediately invokes the callback.
	// While this is perfectly okay, we *might* want to instead check for this case ourselves and invoke setTimeout like our non-pthread solution for more consistency.
	// Presumably, setTimeout will invoke on the next loop (though I could be wrong and maybe (some) browsers also invoke immediately, in which case this is moot).
	emscripten_async_run_in_main_runtime_thread(EM_FUNC_SIG_VII, postMessageCallback, upper_bits, lower_bits);
}


#else


extern void emjsCommon_IupPostMessageNoThreads(Ihandle* ih, const char* s, int i, double d, void* p);

// In this case, we don't have to worry about threads.
// We just want to queue up the message to be processed in the future.
// setTimeout with an interval of 0, should work fine for this.
IUP_SDK_API void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
	emjsCommon_IupPostMessageNoThreads(ih, s, i, d, p);
}

#endif
// I think I need to compile this even if not used, because I don't have any conditional compilation in the .js files.
// WARNING: There is some kind of build system dependency bug with EMSCRIPTEN_KEEPALIVE. When I change this function, I need to clean/rebuild, otherwise the changes don't seem to work, even though the test app rebuilds/relinks.
EMSCRIPTEN_KEEPALIVE void emscriptenIupPostMessageNoThreadsCallbackTrampoline(Ihandle* ih, char* s, int i, double d, void* p)
{
	IFnsidv cb = (IFnsidv)IupGetCallback(ih, "POSTMESSAGE_CB");
  if (cb)
	{
    cb(ih, s, i, d, p);
	}
}





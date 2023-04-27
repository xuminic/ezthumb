/** \file
 * \brief Timer for the Emscripten backend.
 *
 * See Copyright Notice in "iup.h"
 */
#include <stdio.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_timer.h"

#include "iupemscripten_drv.h"

#include <emscripten.h>

extern int emjsTimer_CreateTimer(int time_ms);
extern void emjsTimer_DestroyTimer(int handle_id);
extern int emjsTimer_GetTime();

#if 0
- (void) onTimerCallback:(NSTimer*)theTimer
{
  Icallback callback_function;
  Ihandle* ih = (Ihandle*)[[[self theTimer] userInfo] pointerValue];
  callback_function = IupGetCallback(ih, "ACTION_CB");
  
  if(callback_function)
    {
      CFTimeInterval start_time = [self startTime];
      double current_time = CACurrentMediaTime();
      NSUInteger elapsed_time = (NSUInteger)(((current_time - start_time) * 1000.0) + 0.5);
      iupAttribSetInt(ih, "ELAPSEDTIME", (int)elapsed_time);
    
      if(callback_function(ih) == IUP_CLOSE)
        {
          IupExitLoop();
        }
    }
}
#endif

EMSCRIPTEN_KEEPALIVE void emscriptenTimerCallbackTrampoline(int handle_id, int elapsed_time)
{
  iupEmscripten_Log("Hello world");
	Ihandle* ih = iupEmscripten_GetIhandleValueForKey(handle_id);
  Icallback action_callback = IupGetCallback(ih, "ACTION_CB");
  if (action_callback)
  {
    // need to implement this - store value in global dictionary
	  iupAttribSetInt(ih, "ELAPSEDTIME", elapsed_time);

    action_callback(ih);
		/* if(action_callback(ih) == IUP_CLOSE) */
		/* { */
		/* 	IupExitLoop(); */
		/* } */
  }

}

void iupdrvTimerRun(Ihandle* ih)
{
  unsigned int time_ms;
  int timer_id;
	InativeHandle* new_handle = NULL;
  
  if (ih->handle != NULL) { /* timer already started */
    return;
  }
  
  time_ms = iupAttribGetInt(ih, "TIME");

  if (time_ms > 0)
    {
      timer_id = emjsTimer_CreateTimer(time_ms);

      new_handle = (InativeHandle*)calloc(1, sizeof(InativeHandle));
      new_handle->handleID = timer_id;
      ih->handle = new_handle;

      iupEmscripten_SetIntKeyForIhandleValue(timer_id, ih);
    }
}

static void emscriptenTimerUnMapMethod(Ihandle* ih)
{
  if (ih->handle != NULL) {
    emjsTimer_DestroyTimer(ih->handle->handleID);
    ih->handle = NULL;
  }
}

void iupdrvTimerStop(Ihandle* ih)
{
  emscriptenTimerUnMapMethod(ih);
}

void iupdrvTimerInitClass(Iclass* ic)
{
	(void)ic;
	// This must be UnMap and not Destroy because we're using the ih->handle and UnMap will clear the pointer to NULL before we reach Destroy.
	ic->UnMap = emscriptenTimerUnMapMethod;
}



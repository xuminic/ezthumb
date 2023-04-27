/** \file
 * \brief Emscripten Message Loop
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>    
#include <string.h>    

#include "iup.h"
#include "iupcbs.h"
#include "iup_loop.h"


static IFidle emscripten_idle_cb = NULL;


void iupdrvSetIdleFunction(Icallback f)
{
  emscripten_idle_cb = (IFidle)f;
}

void IupExitLoop(void)
{
}


int IupMainLoopLevel(void)
{
  return 0;
}

/* I don't see any possible way of supporting this. 
   Emscripten/Java controls the main loop and users don't have access to it.
*/
int IupMainLoop(void)
{
  static int has_done_entry = 0;

  if (0 == has_done_entry)
  {
    has_done_entry = 1;
    iupLoopCallEntryCb();
  }

  
  return IUP_NOERROR;

}

int IupLoopStepWait(void)
{
  return IUP_DEFAULT;
}

int IupLoopStep(void)
{
  return IUP_DEFAULT;
}

void IupFlush(void)
{
}


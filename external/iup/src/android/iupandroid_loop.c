/** \file
 * \brief Android Message Loop
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>    
#include <string.h>    

#include "iup.h"
#include "iupcbs.h"


static IFidle android_idle_cb = NULL;


void iupdrvSetIdleFunction(Icallback f)
{
  android_idle_cb = (IFidle)f;
}

void IupExitLoop(void)
{
}


int IupMainLoopLevel(void)
{
  return 0;
}

/* I don't see any possible way of supporting this. 
   Android/Java controls the main loop and users don't have access to it.
*/
int IupMainLoop(void)
{
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


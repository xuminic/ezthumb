/** \file
 * \brief IupBackgroundBox control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_register.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_drv.h"


/*****************************************************************************\
|* Methods                                                                   *|
\*****************************************************************************/

static void iBackgroundBoxComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  if (ih->firstchild)
  {
    iupBaseComputeNaturalSize(ih->firstchild);

    *children_expand = ih->firstchild->expand;
    *w = ih->firstchild->naturalwidth;
    *h = ih->firstchild->naturalheight;
  }
}

static void iBackgroundBoxSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  if (ih->firstchild)
  {
    int width = (ih->currentwidth  > 0 ? ih->currentwidth : 0);
    int height = (ih->currentheight > 0 ? ih->currentheight : 0);
    iupBaseSetCurrentSize(ih->firstchild, width, height, shrink);
  }
}

static void iBackgroundBoxSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (ih->firstchild)
  {
    iupBaseSetPosition(ih->firstchild, 0, 0);
    (void)x;
    (void)y;
  }
}

static int iBackgroundBoxCreateMethod(Ihandle* ih, void** params)
{
  IupSetAttribute(ih, "CANFOCUS", "NO");

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    if (iparams[0]) IupAppend(ih, iparams[0]);
  }

  return IUP_NOERROR;
}

Iclass* iupBackgroundBoxNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name   = "backgroundbox";
  ic->format = "h";   /* one ihandle */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype  = IUP_CHILDMANY+1;  /* 1 child */
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupBackgroundBoxNewClass;
  ic->Create  = iBackgroundBoxCreateMethod;

  ic->ComputeNaturalSize = iBackgroundBoxComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iBackgroundBoxSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iBackgroundBoxSetChildrenPositionMethod;

  /* Base Container */
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iupBaseGetRasterSizeAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* replace IupCanvas behavior */
  iupClassRegisterReplaceAttribFunc (ic, "BGCOLOR", iupBaseNativeParentGetBgColorAttrib, NULL);
  iupClassRegisterReplaceAttribDef  (ic, "BGCOLOR", "DLGBGCOLOR", NULL);
  iupClassRegisterReplaceAttribDef  (ic, "BORDER", "NO", NULL);
  iupClassRegisterReplaceAttribFlags(ic, "BORDER", IUPAF_NO_INHERIT);
  iupClassRegisterReplaceAttribDef  (ic, "SCROLLBAR", "NO", NULL);

  return ic;
}

Ihandle* IupBackgroundBox(Ihandle* child)
{
  void *children[2];
  children[0] = (void*)child;
  children[1] = NULL;
  return IupCreatev("backgroundbox", children);
}

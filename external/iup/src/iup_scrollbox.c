/** \file
 * \brief iupscrollbox control
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
|* Canvas Callbacks                                                          *|
\*****************************************************************************/

static void iScrollBoxUpdatePosition(Ihandle* ih, float posx, float posy)
{
  iupBaseSetPosition(ih->firstchild, -(int)posx, -(int)posy);
}

static int iScrollBoxScroll_CB(Ihandle *ih, int op, float posx, float posy)
{
  if (ih->firstchild)
  {
    if (IupGetInt(ih, "DX") > IupGetInt(ih, "XMAX")-iupdrvGetScrollbarSize())
      posx = 0;
    if (IupGetInt(ih, "DY") > IupGetInt(ih, "YMAX")-iupdrvGetScrollbarSize())
      posy = 0;

    iScrollBoxUpdatePosition(ih, posx, posy);
    iupLayoutUpdate(ih->firstchild);
  }
  (void)op;
  return IUP_DEFAULT;
}

static int iScrollBoxButton_CB(Ihandle *ih, int but, int pressed, int x, int y, char* status)
{
  if (but==IUP_BUTTON1 && pressed)
  {
    iupAttribSetInt(ih, "_IUP_START_X", x);
    iupAttribSetInt(ih, "_IUP_START_Y", y);
    iupAttribSetInt(ih, "_IUP_START_POSX", (int)IupGetFloat(ih, "POSX"));
    iupAttribSetInt(ih, "_IUP_START_POSY", (int)IupGetFloat(ih, "POSY"));
    iupAttribSet(ih, "_IUP_DRAG_SB", "1");
  }
  if (but==IUP_BUTTON1 && !pressed)
    iupAttribSet(ih, "_IUP_DRAG_SB", NULL);
  (void)status;
  return IUP_DEFAULT;
}

static int iScrollBoxMotion_CB(Ihandle *ih, int x, int y, char* status)
{
  if (iup_isbutton1(status) &&
      iupAttribGet(ih, "_IUP_DRAG_SB"))
  {
    int start_x = iupAttribGetInt(ih, "_IUP_START_X");
    int start_y = iupAttribGetInt(ih, "_IUP_START_Y");
    int dx = x - start_x;
    int dy = y - start_y;
    int posx = iupAttribGetInt(ih, "_IUP_START_POSX");
    int posy = iupAttribGetInt(ih, "_IUP_START_POSY");
    IupSetInt(ih, "POSX", posx-dx);  /* drag direction is oposite to scrollbar */
    IupSetInt(ih, "POSY", posy-dy);
    iScrollBoxScroll_CB(ih, 0, IupGetFloat(ih, "POSX"), IupGetFloat(ih, "POSY"));
  }
  return IUP_DEFAULT;
}


/*****************************************************************************\
|* Methods                                                                   *|
\*****************************************************************************/


static void iScrollBoxComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  if (ih->firstchild)
  {
    /* update child natural size first */
    iupBaseComputeNaturalSize(ih->firstchild);
  }

  /* ScrollBox size does not depends on the child size,
     its natural size must be 0 to be free of restrictions. */
  (void)w;
  (void)h;

  /* Also set expand to its own expand so it will not depend on children */
  *children_expand = ih->expand;
}

static void iScrollBoxSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  if (ih->firstchild)
  {
    int w, h, has_sb_horiz=0, has_sb_vert=0;

    /* If child is greater than scrollbox area, use child natural size,
       else use current scrollbox size;
       So this will let the child be gratter than the scrollbox,
       or let the child expand to the scrollbox. */

    if (ih->firstchild->naturalwidth > ih->currentwidth)
    {
      w = ih->firstchild->naturalwidth;
      has_sb_horiz = 1;
    }
    else
      w = ih->currentwidth;  /* expand space */

    if (ih->firstchild->naturalheight > ih->currentheight)
    {
      h = ih->firstchild->naturalheight;
      has_sb_vert = 1;
    }
    else
      h = ih->currentheight; /* expand space */

    if (!has_sb_horiz && has_sb_vert)
      w -= iupdrvGetScrollbarSize();  /* reduce expand space */
    if (has_sb_horiz && !has_sb_vert)
      h -= iupdrvGetScrollbarSize();  /* reduce expand space */

    iupBaseSetCurrentSize(ih->firstchild, w, h, shrink);

    IupSetInt(ih, "XMAX", ih->firstchild->currentwidth);
    IupSetInt(ih, "YMAX", ih->firstchild->currentheight);
  }
  else
  {
    IupSetAttribute(ih, "XMAX", "0");
    IupSetAttribute(ih, "YMAX", "0");
  }
}

static void iScrollBoxSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (ih->firstchild)
  {
    iScrollBoxUpdatePosition(ih, IupGetFloat(ih, "POSX"), IupGetFloat(ih, "POSY"));

    /* because ScrollBox is a native container, 
       child position is restarted at (0,0) */
    (void)x;
    (void)y;
  }
}

static void iScrollBoxLayoutUpdate(Ihandle* ih)
{
  int dx = ih->currentwidth, 
      dy = ih->currentheight;

  /* already updated the canvas layout, 
     so just have to update the scrollbars and child. */

  /* if child is greater than scrollbox, has scrollbars
     but this affects the oposite direction */

  if (ih->firstchild->currentwidth > ih->currentwidth)
    dy -= iupdrvGetScrollbarSize();

  if (ih->firstchild->currentheight > ih->currentheight)
    dx -= iupdrvGetScrollbarSize();

  IupSetInt(ih, "DX", dx);
  IupSetInt(ih, "DY", dy);

  if (ih->firstchild)
  {
    iScrollBoxUpdatePosition(ih, IupGetFloat(ih, "POSX"), IupGetFloat(ih, "POSY"));
    iupLayoutUpdate(ih->firstchild);
  }
}

static int iScrollBoxCreateMethod(Ihandle* ih, void** params)
{
  /* Setting callbacks */
  IupSetCallback(ih, "SCROLL_CB",    (Icallback)iScrollBoxScroll_CB);
  IupSetCallback(ih, "BUTTON_CB",    (Icallback)iScrollBoxButton_CB);
  IupSetCallback(ih, "MOTION_CB",    (Icallback)iScrollBoxMotion_CB);

  IupSetAttribute(ih, "CANFOCUS", "NO");

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    if (iparams[0]) IupAppend(ih, iparams[0]);
  }

  return IUP_NOERROR;
}

Iclass* iupScrollBoxNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name   = "scrollbox";
  ic->format = "h";   /* one ihandle */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype  = IUP_CHILDMANY+1;  /* 1 child */
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupScrollBoxNewClass;
  ic->Create  = iScrollBoxCreateMethod;

  ic->ComputeNaturalSize = iScrollBoxComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iScrollBoxSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iScrollBoxSetChildrenPositionMethod;
  ic->LayoutUpdate = iScrollBoxLayoutUpdate;

  /* Base Container */
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
  {
    IattribGetFunc drawsize_get = NULL;
    iupClassRegisterGetAttribute(ic, "DRAWSIZE", &drawsize_get, NULL, NULL, NULL, NULL);
    iupClassRegisterAttribute(ic, "CLIENTSIZE", drawsize_get, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
  }

  /* replace IupCanvas behavior */
  iupClassRegisterReplaceAttribFunc(ic, "BGCOLOR", iupBaseNativeParentGetBgColorAttrib, NULL);
  iupClassRegisterReplaceAttribDef(ic, "BGCOLOR", "DLGBGCOLOR", NULL);
  iupClassRegisterReplaceAttribDef(ic, "BORDER", "NO", NULL);
  iupClassRegisterReplaceAttribFlags(ic, "BORDER", IUPAF_NO_INHERIT);
  iupClassRegisterReplaceAttribDef(ic, "SCROLLBAR", "YES", NULL);

  return ic;
}

Ihandle* IupScrollBox(Ihandle* child)
{
  void *children[2];
  children[0] = (void*)child;
  children[1] = NULL;
  return IupCreatev("scrollbox", children);
}

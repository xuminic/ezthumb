/** \file
 * \brief iupexpander control
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
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_childtree.h"
#include "iup_draw.h"


#define IEXPAND_HANDLE_SIZE 20
#define IEXPAND_HANDLE_SPC   3
#define IEXPAND_BACK_MARGIN  2

enum { IEXPANDER_LEFT, IEXPANDER_RIGHT, IEXPANDER_TOP, IEXPANDER_BOTTOM };
enum { IEXPANDER_CLOSE, IEXPANDER_OPEN, IEXPANDER_OPEN_FLOAT };

struct _IcontrolData
{
  /* attributes */
  int position;
  int state;
  int barSize;

  int highlight,
      auto_show;
  Ihandle* timer;
};


static void iExpanderOpenCloseChild(Ihandle* ih, int refresh, int callcb)
{
  Ihandle *child = ih->firstchild->brother;

  IupUpdate(ih->firstchild);

  if (!child)
    return;

  if (ih->data->state == IEXPANDER_CLOSE)
    IupSetAttribute(child, "VISIBLE", "NO");
  else 
    IupSetAttribute(child, "VISIBLE", "YES");

  if (refresh)
    IupRefresh(child); /* this will recompute the layout of the hole dialog */

  if (callcb)
  {
    IFn cb = IupGetCallback(ih, "ACTION");
    if (cb)
      cb(ih);
  }
}

static int iExpanderGetBarSize(Ihandle* ih)
{
  int bar_size;
  if (ih->data->barSize == -1)
  {
    iupdrvFontGetCharSize(ih, NULL, &bar_size); 

    if (bar_size < IEXPAND_HANDLE_SIZE)
      bar_size = IEXPAND_HANDLE_SIZE;

    if (ih->data->position == IEXPANDER_TOP && iupAttribGetStr(ih, "TITLE"))
      bar_size += 2*IEXPAND_BACK_MARGIN;
  }
  else
    bar_size = ih->data->barSize;

  return bar_size;
}

/*****************************************************************************\
|* Callbacks of canvas bar                                                   *|
\*****************************************************************************/

static void iExpanderDrawTriangle(IdrawCanvas *dc, int x, int y, unsigned char r, unsigned char g, unsigned char b, int dir)
{
  int points[6];

  /* fix for smooth triangle */
  int delta = (IEXPAND_HANDLE_SIZE - 2*IEXPAND_HANDLE_SPC)/2;

  switch(dir)
  {
  case IEXPANDER_LEFT:  /* arrow points left */
    x += IEXPAND_HANDLE_SPC;  /* fix center */
    points[0] = x + IEXPAND_HANDLE_SIZE - IEXPAND_HANDLE_SPC - delta;
    points[1] = y + IEXPAND_HANDLE_SPC;
    points[2] = x + IEXPAND_HANDLE_SIZE - IEXPAND_HANDLE_SPC - delta;
    points[3] = y + IEXPAND_HANDLE_SIZE - IEXPAND_HANDLE_SPC;
    points[4] = x + IEXPAND_HANDLE_SPC;
    points[5] = y + IEXPAND_HANDLE_SIZE/2;
    break;
  case IEXPANDER_TOP:    /* arrow points top */
    y += IEXPAND_HANDLE_SPC;  /* fix center */
    points[0] = x + IEXPAND_HANDLE_SPC;
    points[1] = y + IEXPAND_HANDLE_SIZE - IEXPAND_HANDLE_SPC - (delta-1);
    points[2] = x + IEXPAND_HANDLE_SIZE - IEXPAND_HANDLE_SPC;
    points[3] = y + IEXPAND_HANDLE_SIZE - IEXPAND_HANDLE_SPC - (delta-1);
    points[4] = x + IEXPAND_HANDLE_SIZE/2;
    points[5] = y + IEXPAND_HANDLE_SPC;
    break;
  case IEXPANDER_RIGHT:  /* arrow points right */
    x += IEXPAND_HANDLE_SPC;  /* fix center */
    points[0] = x + IEXPAND_HANDLE_SPC;
    points[1] = y + IEXPAND_HANDLE_SPC;
    points[2] = x + IEXPAND_HANDLE_SPC;
    points[3] = y + IEXPAND_HANDLE_SIZE - IEXPAND_HANDLE_SPC;
    points[4] = x + IEXPAND_HANDLE_SIZE - IEXPAND_HANDLE_SPC - delta;
    points[5] = y + IEXPAND_HANDLE_SIZE/2;
    break;
  case IEXPANDER_BOTTOM:  /* arrow points bottom */
    y += IEXPAND_HANDLE_SPC;  /* fix center */
    points[0] = x + IEXPAND_HANDLE_SPC;
    points[1] = y + IEXPAND_HANDLE_SPC;
    points[2] = x + IEXPAND_HANDLE_SIZE - IEXPAND_HANDLE_SPC;
    points[3] = y + IEXPAND_HANDLE_SPC;
    points[4] = x + IEXPAND_HANDLE_SIZE/2;
    points[5] = y + IEXPAND_HANDLE_SIZE - IEXPAND_HANDLE_SPC - (delta-1);

    /* fix for simmetry */
    iupDrawLine(dc, x+IEXPAND_HANDLE_SPC, y+IEXPAND_HANDLE_SPC, x+IEXPAND_HANDLE_SIZE-IEXPAND_HANDLE_SPC, y+IEXPAND_HANDLE_SPC, r, g, b, IUP_DRAW_STROKE);
    break;
  }

  iupDrawPolygon(dc, points, 3, r, g, b, IUP_DRAW_FILL);
}

static void iExpanderDrawSmallTriangle(IdrawCanvas *dc, int x, int y, unsigned char r, unsigned char g, unsigned char b, int dir)
{
  int points[6];
  int size = IEXPAND_HANDLE_SIZE-2;
  int space = IEXPAND_HANDLE_SPC+1;

  /* fix for smooth triangle */
  int delta = (size - 2*space)/2;

  switch(dir)
  {
  case IEXPANDER_RIGHT:  /* arrow points right */
    x += space-1;  /* fix center */
    y += 1;
    points[0] = x + space;
    points[1] = y + space;
    points[2] = x + space;
    points[3] = y + size - space;
    points[4] = x + size - space - delta;
    points[5] = y + size/2;
    break;
  case IEXPANDER_BOTTOM:  /* arrow points bottom */
    y += space;  /* fix center */
    points[0] = x + space;
    points[1] = y + space;
    points[2] = x + size - space;
    points[3] = y + space;
    points[4] = x + size/2;
    points[5] = y + size - space - (delta-1);

    /* fix for simmetry */
    iupDrawLine(dc, x+space, y+space, x+size-space, y+space, r, g, b, IUP_DRAW_STROKE);
    break;
  }

  iupDrawPolygon(dc, points, 3, r, g, b, IUP_DRAW_FILL);
}

static void iExpanderDrawArrow(IdrawCanvas *dc, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char bg_r, unsigned char bg_g, unsigned char bg_b, int dir)
{
  unsigned char sr, sg, sb;

  sr = (r+bg_r)/2;
  sg = (g+bg_g)/2;
  sb = (b+bg_b)/2;

  /* to smooth the arrow border */
  switch(dir)
  {
  case IEXPANDER_LEFT:  /* arrow points left */
    iExpanderDrawTriangle(dc, x-1, y, sr, sg, sb, dir);
    break;
  case IEXPANDER_TOP:    /* arrow points top */
    iExpanderDrawTriangle(dc, x, y-1, sr, sg, sb, dir);
    break;
  case IEXPANDER_RIGHT:  /* arrow points right */
    iExpanderDrawTriangle(dc, x+1, y, sr, sg, sb, dir);
    break;
  case IEXPANDER_BOTTOM:  /* arrow points bottom */
    iExpanderDrawTriangle(dc, x, y+1, sr, sg, sb, dir);
    break;
  }

  iExpanderDrawTriangle(dc, x, y, r, g, b, dir);
}

static void iExpanderDrawSmallArrow(IdrawCanvas *dc, unsigned char r, unsigned char g, unsigned char b, unsigned char bg_r, unsigned char bg_g, unsigned char bg_b, int dir)
{
  unsigned char sr, sg, sb;

  sr = (r+bg_r)/2;
  sg = (g+bg_g)/2;
  sb = (b+bg_b)/2;

  /* to smooth the arrow border */
  switch(dir)
  {
  case IEXPANDER_RIGHT:  /* arrow points right */
    iExpanderDrawSmallTriangle(dc, 2+IEXPAND_BACK_MARGIN, 0+IEXPAND_BACK_MARGIN, sr, sg, sb, dir);
    iExpanderDrawSmallTriangle(dc, 1+IEXPAND_BACK_MARGIN, 0+IEXPAND_BACK_MARGIN, r, g, b, dir);
    break;
  case IEXPANDER_BOTTOM:  /* arrow points bottom */
    iExpanderDrawSmallTriangle(dc, 0+IEXPAND_BACK_MARGIN, 1+IEXPAND_BACK_MARGIN, sr, sg, sb, dir);
    iExpanderDrawSmallTriangle(dc, 0+IEXPAND_BACK_MARGIN, 0+IEXPAND_BACK_MARGIN, r, g, b, dir);
    break;
  }
}

static void iExpanderHighlight(unsigned char *r, unsigned char *g, unsigned char *b)
{
  int i = (*r+*g+*b)/3;
  if (i < 128)
  {
    *r = (*r+255)/2;
    *g = (*g+255)/2;
    *b = (*b+255)/2;
  }
  else
  {
    *r = (*r+0)/2;
    *g = (*g+0)/2;
    *b = (*b+0)/2;
  }
}

static int iExpanderAction_CB(Ihandle* bar)
{
  Ihandle *ih = bar->parent;
  IdrawCanvas *dc = iupDrawCreateCanvas(bar);
  unsigned char r=0, g=0, b=0;
  unsigned char bg_r=0, bg_g=0, bg_b=0;
  int draw_bgcolor = 1;
  char* title = iupAttribGetStr(ih, "TITLE");
  char* bgcolor = iupAttribGetStr(ih, "BACKCOLOR");
  if (!bgcolor)
  {
    bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
    draw_bgcolor = 0;
  }
  
  iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);
  iupStrToRGB(IupGetAttribute(ih, "FORECOLOR"), &r, &g, &b);

  iupDrawParentBackground(dc);

  if (draw_bgcolor)
    iupDrawRectangle(dc, IEXPAND_BACK_MARGIN, IEXPAND_BACK_MARGIN, bar->currentwidth - IEXPAND_BACK_MARGIN, bar->currentheight - IEXPAND_BACK_MARGIN, bg_r, bg_g, bg_b, IUP_DRAW_FILL);

  if (ih->data->position == IEXPANDER_TOP && title)
  {
    /* left align everything */
    int len, charheight;
    iupStrNextLine(title, &len);  /* get the length of the first line */
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    iupDrawText(dc, title, len, IEXPAND_HANDLE_SIZE+IEXPAND_HANDLE_SPC, (bar->currentheight-charheight)/2, r, g, b, IupGetAttribute(ih, "FONT"));

    if (ih->data->highlight)
      iExpanderHighlight(&r, &g, &b);

    if (ih->data->state == IEXPANDER_CLOSE)
      iExpanderDrawSmallArrow(dc, r, g, b, bg_r, bg_g, bg_b, IEXPANDER_RIGHT);
    else
      iExpanderDrawSmallArrow(dc, r, g, b, bg_r, bg_g, bg_b, IEXPANDER_BOTTOM);
  }
  else
  {
    /* center align the arrow */
    int x, y;

    if (ih->data->highlight)
      iExpanderHighlight(&r, &g, &b);

    switch(ih->data->position)
    {
    case IEXPANDER_LEFT:
      x = 0;
      y = (bar->currentheight - IEXPAND_HANDLE_SIZE)/2;
      if (ih->data->state == IEXPANDER_CLOSE)
        iExpanderDrawArrow(dc, x, y, r, g, b, bg_r, bg_g, bg_b, IEXPANDER_RIGHT);
      else
        iExpanderDrawArrow(dc, x, y, r, g, b, bg_r, bg_g, bg_b, IEXPANDER_LEFT);
      break;
    case IEXPANDER_TOP:
      x = (bar->currentwidth - IEXPAND_HANDLE_SIZE)/2;
      y = 0;
      if (ih->data->state == IEXPANDER_CLOSE)
        iExpanderDrawArrow(dc, x, y, r, g, b, bg_r, bg_g, bg_b, IEXPANDER_BOTTOM);
      else
        iExpanderDrawArrow(dc, x, y, r, g, b, bg_r, bg_g, bg_b, IEXPANDER_TOP);
      break;
    case IEXPANDER_RIGHT:
      x = 0;
      y = (bar->currentheight - IEXPAND_HANDLE_SIZE)/2;
      if (ih->data->state == IEXPANDER_CLOSE)
        iExpanderDrawArrow(dc, x, y, r, g, b, bg_r, bg_g, bg_b, IEXPANDER_LEFT);
      else
        iExpanderDrawArrow(dc, x, y, r, g, b, bg_r, bg_g, bg_b, IEXPANDER_RIGHT);
      break;
    case IEXPANDER_BOTTOM:
      x = (bar->currentwidth - IEXPAND_HANDLE_SIZE)/2;
      y = 0;
      if (ih->data->state == IEXPANDER_CLOSE)
        iExpanderDrawArrow(dc, x, y, r, g, b, bg_r, bg_g, bg_b, IEXPANDER_TOP);
      else
        iExpanderDrawArrow(dc, x, y, r, g, b, bg_r, bg_g, bg_b, IEXPANDER_BOTTOM);
      break;
    }
  }

  iupDrawFlush(dc);

  iupDrawKillCanvas(dc);

  return IUP_DEFAULT;
}

static int iExpanderGlobalMotion_cb(int x, int y)
{
  int child_x, child_y;
  Ihandle* ih = (Ihandle*)IupGetGlobal("_IUP_EXPANDER_GLOBAL");
  Ihandle *child = ih->firstchild->brother;

  if (ih->data->state != IEXPANDER_OPEN_FLOAT)
  {
    IupSetGlobal("_IUP_EXPANDER_GLOBAL", NULL);
    IupSetFunction("GLOBALMOTION_CB", IupGetFunction("_IUP_OLD_GLOBALMOTION_CB"));
    IupSetFunction("_IUP_OLD_GLOBALMOTION_CB", NULL);
    IupSetGlobal("INPUTCALLBACKS", "No");
    return IUP_DEFAULT;
  }

  child_x = 0, child_y = 0;
  iupdrvClientToScreen(ih->firstchild, &child_x, &child_y);
  if (x > child_x && x < child_x+ih->firstchild->currentwidth &&
      y > child_y && y < child_y+ih->firstchild->currentheight)
    return IUP_DEFAULT;  /* ignore if inside the bar */

  child_x = 0, child_y = 0;
  iupdrvClientToScreen(child, &child_x, &child_y);
  if (x < child_x || x > child_x+child->currentwidth ||
      y < child_y || y > child_y+child->currentheight)
  {
    ih->data->state = IEXPANDER_CLOSE;
    iExpanderOpenCloseChild(ih, 0, 1);
    IupSetGlobal("_IUP_EXPANDER_GLOBAL", NULL);
    IupSetFunction("GLOBALMOTION_CB", IupGetFunction("_IUP_OLD_GLOBALMOTION_CB"));
    IupSetFunction("_IUP_OLD_GLOBALMOTION_CB", NULL);
    IupSetGlobal("INPUTCALLBACKS", "No");
  }

  return IUP_DEFAULT;
}

static int iExpanderTimer_cb(Ihandle* timer)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(timer, "_IUP_EXPANDER");
  Ihandle *child = ih->firstchild->brother;

  /* run timer just once each time */
  IupSetAttribute(timer, "RUN", "No");

  /* just show child on top,
     that's why child must be a native container when using autoshow. */
  ih->data->state = IEXPANDER_OPEN_FLOAT;
  iExpanderOpenCloseChild(ih, 0, 1);
  IupRefreshChildren(ih);
  IupSetAttribute(child, "ZORDER", "TOP"); 

  /* now monitor mouse move */
  IupSetGlobal("INPUTCALLBACKS", "Yes");
  IupSetFunction("_IUP_OLD_GLOBALMOTION_CB", IupGetFunction("GLOBALMOTION_CB"));
  IupSetGlobal("_IUP_EXPANDER_GLOBAL", (char*)ih);
  IupSetFunction("GLOBALMOTION_CB", (Icallback)iExpanderGlobalMotion_cb);
  return IUP_DEFAULT;
}

static int iExpanderLeaveWindow_cb(Ihandle* bar)
{
  Ihandle* ih = bar->parent;
  if (ih->data->highlight)
  {
    ih->data->highlight = 0;
    IupUpdate(ih->firstchild);

    if (ih->data->auto_show)
    {
      if (IupGetInt(ih->data->timer, "RUN"))
        IupSetAttribute(ih->data->timer, "RUN", "No");
    }
  }
  return IUP_DEFAULT;
}

static int iExpanderEnterWindow_cb(Ihandle* bar)
{
  Ihandle* ih = bar->parent;
  if (!ih->data->highlight)
  {
    ih->data->highlight = 1;
    IupUpdate(ih->firstchild);

    if (ih->data->auto_show && 
        ih->firstchild->brother &&
        ih->data->state==IEXPANDER_CLOSE)
      IupSetAttribute(ih->data->timer, "RUN", "Yes");
  }
  return IUP_DEFAULT;
}

static int iExpanderButton_CB(Ihandle* bar, int button, int pressed, int x, int y, char* status)
{
  Ihandle* ih = bar->parent;

  if (ih->data->auto_show)
  {
    if (IupGetInt(ih->data->timer, "RUN"))
      IupSetAttribute(ih->data->timer, "RUN", "No");
  }

  if (button==IUP_BUTTON1 && pressed)
  {
    /* Update the state: OPEN ==> collapsed, CLOSE ==> expanded */
     ih->data->state = (ih->data->state == IEXPANDER_OPEN? IEXPANDER_CLOSE: IEXPANDER_OPEN);

     iExpanderOpenCloseChild(ih, 1, 1);
  }

  (void)x;
  (void)y;
  (void)status;
  return IUP_DEFAULT;
}


/*****************************************************************************\
|* Attributes                                                                *|
\*****************************************************************************/


static char* iExpanderGetClientSizeAttrib(Ihandle* ih)
{
  int width = ih->currentwidth;
  int height = ih->currentheight;
  int bar_size = iExpanderGetBarSize(ih);

  if (ih->data->position == IEXPANDER_LEFT || ih->data->position == IEXPANDER_RIGHT)
    width -= bar_size;
  else
    height -= bar_size;

  if (width < 0) width = 0;
  if (height < 0) height = 0;
  return iupStrReturnIntInt(width, height, 'x');
}

static char* iExpanderGetClientOffsetAttrib(Ihandle* ih)
{
  int dx = 0, dy = 0;
  int bar_size = iExpanderGetBarSize(ih);

  if (ih->data->position == IEXPANDER_LEFT)
    dx += bar_size;
  else if (ih->data->position == IEXPANDER_TOP)
    dy += bar_size;

  return iupStrReturnIntInt(dx, dy, 'x');
}

static int iExpanderSetPositionAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "LEFT"))
    ih->data->position = IEXPANDER_LEFT;
  else if (iupStrEqualNoCase(value, "RIGHT"))
    ih->data->position = IEXPANDER_RIGHT;
  else if (iupStrEqualNoCase(value, "BOTTOM"))
    ih->data->position = IEXPANDER_BOTTOM;
  else  /* Default = TOP */
    ih->data->position = IEXPANDER_TOP;

  return 0;  /* do not store value in hash table */
}

static int iExpanderSetBarSizeAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    ih->data->barSize = -1;
  else
    iupStrToInt(value, &ih->data->barSize);  /* must manually update layout */
  return 0; /* do not store value in hash table */
}

static char* iExpanderGetBarSizeAttrib(Ihandle* ih)
{
  int bar_size = iExpanderGetBarSize(ih);
  return iupStrReturnInt(bar_size);
}

static int iExpanderPostRedrawSetAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  IupUpdate(ih->firstchild);
  return 1;  /* store value in hash table */
}

static int iExpanderSetStateAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "OPEN"))
    ih->data->state = IEXPANDER_OPEN;
  else
    ih->data->state = IEXPANDER_CLOSE;

  iExpanderOpenCloseChild(ih, 1, 0);

  return 0; /* do not store value in hash table */
}

static char* iExpanderGetStateAttrib(Ihandle* ih)
{
  if (ih->data->state)
    return "OPEN";
  else
    return "CLOSE";
}

static int iExpanderSetAutoShowAttrib(Ihandle* ih, const char* value)
{
  ih->data->auto_show = iupStrBoolean(value);
  if (ih->data->auto_show)
  {
    if (!ih->data->timer)
    {
      ih->data->timer = IupTimer();
      IupSetAttribute(ih->data->timer, "TIME", "1000");  /* 1 second */
      IupSetCallback(ih->data->timer, "ACTION_CB", iExpanderTimer_cb);
      iupAttribSet(ih->data->timer, "_IUP_EXPANDER", (char*)ih);  /* 1 second */
    }
  }
  else
  {
    if (ih->data->timer)
      IupSetAttribute(ih->data->timer, "RUN", "NO");
  }
  return 0; /* do not store value in hash table */
}

static char* iExpanderGetAutoShowAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->auto_show); 
}


/*****************************************************************************\
|* Methods                                                                   *|
\*****************************************************************************/


static void iExpanderComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int child_expand = 0,
      natural_w, natural_h;
  Ihandle *child = ih->firstchild->brother;
  int bar_size = iExpanderGetBarSize(ih);

  /* bar */
  if (ih->data->position == IEXPANDER_LEFT || ih->data->position == IEXPANDER_RIGHT)
  {
    natural_w = bar_size;
    natural_h = IEXPAND_HANDLE_SIZE;
  }
  else
  {
    natural_w = IEXPAND_HANDLE_SIZE;
    natural_h = bar_size;

    if (ih->data->position == IEXPANDER_TOP)
    {
      char* title = iupAttribGetStr(ih, "TITLE");
      if (title)
      {
        int title_size = 0;
        iupdrvFontGetMultiLineStringSize(ih, title, &title_size, NULL);
        natural_w += title_size + 2*IEXPAND_BACK_MARGIN + IEXPAND_HANDLE_SPC;
      }
    }
  }

  if (child)
  {
    /* update child natural bar_size first */
    iupBaseComputeNaturalSize(child);

    if (ih->data->position == IEXPANDER_LEFT || ih->data->position == IEXPANDER_RIGHT)
    {
      if (ih->data->state == IEXPANDER_OPEN)  /* only open, not float */
        natural_w += child->naturalwidth;
      natural_h = iupMAX(natural_h, child->naturalheight);
    }
    else
    {
      natural_w = iupMAX(natural_w, child->naturalwidth);
      if (ih->data->state == IEXPANDER_OPEN)  /* only open, not float */
        natural_h += child->naturalheight;
    }

    if (ih->data->state == IEXPANDER_OPEN)
      child_expand = child->expand;
  }

  *children_expand = child_expand;
  *w = natural_w;
  *h = natural_h;
}

static void iExpanderSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  Ihandle *child = ih->firstchild->brother;
  int width = ih->currentwidth;
  int height = ih->currentheight;
  int bar_size = iExpanderGetBarSize(ih);

  if (ih->data->position == IEXPANDER_LEFT || ih->data->position == IEXPANDER_RIGHT)
  {
    /* bar */
    ih->firstchild->currentwidth  = bar_size;
    ih->firstchild->currentheight = ih->currentheight;

    if (ih->currentwidth < bar_size)
      ih->currentwidth = bar_size;

    width = ih->currentwidth - bar_size;
  }
  else /* IEXPANDER_TOP OR IEXPANDER_BOTTOM */
  {
    /* bar */
    ih->firstchild->currentwidth  = ih->currentwidth;
    ih->firstchild->currentheight = bar_size;

    if (ih->currentheight < bar_size)
      ih->currentheight = bar_size;

    height = ih->currentheight - bar_size;
  }

  if (child)
  {
    if (ih->data->state == IEXPANDER_OPEN)
      iupBaseSetCurrentSize(child, width, height, shrink);
    else if (ih->data->state == IEXPANDER_OPEN_FLOAT)  /* simply set to the natural size */
      iupBaseSetCurrentSize(child, child->currentwidth, child->currentheight, shrink);
  }
}

static void iExpanderSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  Ihandle *child = ih->firstchild->brother;
  int bar_size = iExpanderGetBarSize(ih);

  /* always position bar */
  if (ih->data->position == IEXPANDER_LEFT)
  {
    iupBaseSetPosition(ih->firstchild, x, y);
    x += bar_size;
  }
  else if (ih->data->position == IEXPANDER_RIGHT)
    iupBaseSetPosition(ih->firstchild, x + ih->currentwidth - bar_size, y);
  else if (ih->data->position == IEXPANDER_BOTTOM)
    iupBaseSetPosition(ih->firstchild, x, y + ih->currentheight - bar_size);
  else /* IEXPANDER_TOP */
  {
    iupBaseSetPosition(ih->firstchild, x, y);
    y += bar_size;
  }

  if (child)
  {
    if (ih->data->state == IEXPANDER_OPEN)
      iupBaseSetPosition(child, x, y);
    else if (ih->data->state == IEXPANDER_OPEN_FLOAT)
    {
      if (ih->data->position == IEXPANDER_RIGHT)
        x -= child->currentwidth;
      else if (ih->data->position == IEXPANDER_BOTTOM)
        y -= child->currentheight;

      iupBaseSetPosition(child, x, y);
    }
  }
}

static void iExpanderChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  iExpanderOpenCloseChild(ih, 0, 0);
  (void)child;
}

static int iExpanderCreateMethod(Ihandle* ih, void** params)
{
  Ihandle* bar;

  ih->data = iupALLOCCTRLDATA();

  ih->data->position = IEXPANDER_TOP;
  ih->data->state = IEXPANDER_OPEN;
  ih->data->barSize = -1;

  bar = IupCanvas(NULL);
  iupChildTreeAppend(ih, bar);  /* bar will always be the firstchild */
  bar->flags |= IUP_INTERNAL;

  IupSetAttribute(bar, "CANFOCUS", "NO");
  IupSetAttribute(bar, "BORDER", "NO");
  IupSetAttribute(bar, "EXPAND", "NO");

  /* Setting callbacks */
  IupSetCallback(bar, "BUTTON_CB", (Icallback) iExpanderButton_CB);
  IupSetCallback(bar, "ACTION",    (Icallback) iExpanderAction_CB);
  IupSetCallback(bar, "ENTERWINDOW_CB", (Icallback)iExpanderEnterWindow_cb);
  IupSetCallback(bar, "LEAVEWINDOW_CB", (Icallback)iExpanderLeaveWindow_cb);

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    if (*iparams)
      IupAppend(ih, *iparams);
  }

  return IUP_NOERROR;
}

static void iExpanderDestroyMethod(Ihandle* ih)
{
  if (ih->data->timer)
    IupDestroy(ih->data->timer);
}

Iclass* iupExpanderNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name   = "expander";
  ic->format = "h";   /* one ihandle */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype  = IUP_CHILDMANY+2;  /* canvas+child */
  ic->is_interactive = 0;

  /* Class functions */
  ic->New     = iupExpanderNewClass;
  ic->Create  = iExpanderCreateMethod;
  ic->Destroy = iExpanderDestroyMethod;
  ic->Map     = iupBaseTypeVoidMapMethod;
  ic->ChildAdded = iExpanderChildAddedMethod;

  ic->ComputeNaturalSize     = iExpanderComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iExpanderSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition    = iExpanderSetChildrenPositionMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "ACTION", "");

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Base Container */
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iExpanderGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iExpanderGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* IupExpander only */
  iupClassRegisterAttribute(ic, "BARPOSITION", NULL, iExpanderSetPositionAttrib, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BARSIZE", iExpanderGetBarSizeAttrib, iExpanderSetBarSizeAttrib, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATE", iExpanderGetStateAttrib, iExpanderSetStateAttrib, IUPAF_SAMEASSYSTEM, "OPEN", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORECOLOR", NULL, iExpanderPostRedrawSetAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", NULL, iExpanderPostRedrawSetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, iExpanderPostRedrawSetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOSHOW", iExpanderGetAutoShowAttrib, iExpanderSetAutoShowAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  return ic;
}

Ihandle* IupExpander(Ihandle* child)
{
  void *children[2];
  children[0] = (void*)child;
  children[1] = NULL;
  return IupCreatev("expander", children);
}

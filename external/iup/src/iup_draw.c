/** \file
 * \brief Canvas Control.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"
#include "iupdraw.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_drvdraw.h"
#include "iup_assert.h"
#include "iup_image.h"



void IupDrawBegin(Ihandle* ih)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = iupdrvDrawCreateCanvas(ih);
  iupAttribSet(ih, "_IUP_DRAW_DC", (char*)dc);
}

void IupDrawEnd(Ihandle* ih)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawFlush(dc);
  iupdrvDrawKillCanvas(dc);
  iupAttribSet(ih, "_IUP_DRAW_DC", NULL);
}

void IupDrawGetSize(Ihandle* ih, int *w, int *h)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawGetSize(dc, w, h);
}

void IupDrawParentBackground(Ihandle* ih)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawParentBackground(dc);
}

static int iDrawGetStyle(Ihandle* ih)
{
  char* style = IupGetAttribute(ih, "DRAWSTYLE");
  if (iupStrEqualNoCase(style, "FILL"))
    return IUP_DRAW_FILL;
  else if (iupStrEqualNoCase(style, "STROKE_DASH"))
    return IUP_DRAW_STROKE_DASH;
  else if (iupStrEqualNoCase(style, "STROKE_DOT"))
    return IUP_DRAW_STROKE_DOT;
  else 
    return IUP_DRAW_STROKE;
}

void IupDrawLine(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  IdrawCanvas* dc;
  unsigned char r = 0, g = 0, b = 0;
  int style;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  IupGetRGB(ih, "DRAWCOLOR", &r, &g, &b);

  style = iDrawGetStyle(ih);

  iupdrvDrawLine(dc, x1, y1, x2, y2, r, g, b, style);
}

void IupDrawRectangle(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  IdrawCanvas* dc;
  unsigned char r = 0, g = 0, b = 0;
  int style;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  IupGetRGB(ih, "DRAWCOLOR", &r, &g, &b);

  style = iDrawGetStyle(ih);

  iupdrvDrawRectangle(dc, x1, y1, x2, y2, r, g, b, style);
}

void IupDrawArc(Ihandle* ih, int x1, int y1, int x2, int y2, double a1, double a2)
{
  IdrawCanvas* dc;
  unsigned char r = 0, g = 0, b = 0;
  int style;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  IupGetRGB(ih, "DRAWCOLOR", &r, &g, &b);

  style = iDrawGetStyle(ih);

  iupdrvDrawArc(dc, x1, y1, x2, y2, a1, a2, r, g, b, style);
}

void IupDrawPolygon(Ihandle* ih, int* points, int count)
{
  IdrawCanvas* dc;
  unsigned char r = 0, g = 0, b = 0;
  int style;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  IupGetRGB(ih, "DRAWCOLOR", &r, &g, &b);

  style = iDrawGetStyle(ih);

  iupdrvDrawPolygon(dc, points, count, r, g, b, style);
}

void IupDrawText(Ihandle* ih, const char* text, int len, int x, int y)
{
  IdrawCanvas* dc;
  unsigned char r = 0, g = 0, b = 0;
  char* font;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  font = IupGetAttribute(ih, "FONT");

  IupGetRGB(ih, "DRAWCOLOR", &r, &g, &b);

  iupdrvDrawText(dc, text, len, x, y, r, g, b, font);
}

void IupDrawGetTextSize(Ihandle* ih, const char* str, int *w, int *h)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  iupdrvFontGetMultiLineStringSize(ih, str, w, h);
}

void IupDrawGetImageInfo(const char* name, int *w, int *h, int *bpp)
{
  iupImageGetInfo(name, w, h, bpp);
}

void IupDrawImage(Ihandle* ih, const char* name, int make_inactive, int x, int y)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawImage(dc, name, make_inactive, x, y);
}

void IupDrawSetClipRect(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawSetClipRect(dc, x1, y1, x2, y2);
}

void IupDrawResetClip(Ihandle* ih)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawResetClip(dc);
}

void IupDrawSelectRect(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawSelectRect(dc, x1, y1, x2, y2);
}

void IupDrawFocusRect(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawFocusRect(dc, x1, y1, x2, y2);
}

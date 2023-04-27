/** \file
 * \brief IupDraw driver
 *
 * See Copyright Notice in cd.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <cd.h>
#include <cd_private.h>

#include "iup.h"
#include "iupdraw_cd.h"

#include "iup_drvdraw.h"
#include "iup_drvfont.h"


struct _cdCtxCanvas
{
  cdCanvas* canvas;
  Ihandle* ih;
  IdrawCanvas* dc;
};

static void cdkillcanvas(cdCtxCanvas *ctxcanvas)
{
  if (ctxcanvas->dc)
    iupdrvDrawKillCanvas(ctxcanvas->dc);

  memset(ctxcanvas, 0, sizeof(cdCtxCanvas));
  free(ctxcanvas);
}

static int cdactivate(cdCtxCanvas *ctxcanvas)
{
  int w, h;

  if (ctxcanvas->dc)
    iupdrvDrawKillCanvas(ctxcanvas->dc);

  ctxcanvas->dc = iupdrvDrawCreateCanvas(ctxcanvas->ih);

  iupdrvDrawGetSize(ctxcanvas->dc, &w, &h);

  ctxcanvas->canvas->bpp = IupGetInt(NULL, "SCREENDEPTH");
  ctxcanvas->canvas->xres = IupGetDouble(NULL, "SCREENDPI") / 25.4;
  ctxcanvas->canvas->yres = ctxcanvas->canvas->xres;

  ctxcanvas->canvas->w = w;
  ctxcanvas->canvas->h = h;
  ctxcanvas->canvas->w_mm = ((double)w) / ctxcanvas->canvas->xres;
  ctxcanvas->canvas->h_mm = ((double)h) / ctxcanvas->canvas->yres;
  ctxcanvas->canvas->invert_yaxis = 1;

  return CD_OK;
}

static void cdflush(cdCtxCanvas *ctxcanvas)
{
  if (ctxcanvas->dc)
    iupdrvDrawFlush(ctxcanvas->dc);
}

static void cdclear(cdCtxCanvas* ctxcanvas)
{
  if (ctxcanvas->dc)
    iupdrvDrawRectangle(ctxcanvas->dc, 0, 0, ctxcanvas->canvas->w - 1, ctxcanvas->canvas->h - 1, cdCanvasBackground(ctxcanvas->canvas, CD_QUERY), CD_FILL, 1);
}

static int cdfont(cdCtxCanvas *ctxcanvas, const char *type_face, int style, int size)
{
  int is_italic = 0, is_bold = 0;   /* default is CD_PLAIN */
  int is_strikeout = 0, is_underline = 0;
  char font[1024];

  if (style & CD_BOLD)
    is_bold = 1;

  if (style & CD_ITALIC)
    is_italic = 1;

  if (style & CD_UNDERLINE)
    is_underline = 1;

  if (style & CD_STRIKEOUT)
    is_strikeout = 1;

  sprintf(font, "%s, %s%s%s%s%d", type_face, is_bold ? "Bold " : "", is_italic ? "Italic " : "", is_underline ? "Underline " : "", is_strikeout ? "Strikeout " : "", size);

  /* store in native font and manually save font parameters */
  strcpy(ctxcanvas->canvas->native_font, font);
  strcpy(ctxcanvas->canvas->font_type_face, type_face);
  ctxcanvas->canvas->font_style = style;
  ctxcanvas->canvas->font_size = size;

  return 0;
}

static void cdgetfontdim(cdCtxCanvas* ctxcanvas, int *max_width, int *line_height, int *ascent, int *descent)
{
  iupdrvFontGetFontDim(ctxcanvas->canvas->native_font, max_width, line_height, ascent, descent);
}

static void cdgettextsize(cdCtxCanvas* ctxcanvas, const char *s, int len, int *width, int *height)
{
  iupdrvFontGetTextSize(ctxcanvas->canvas->native_font, s, len, width, height);
}

static void cdcliparea(cdCtxCanvas *ctxcanvas, int xmin, int xmax, int ymin, int ymax)
{
  if (ctxcanvas->canvas->clip_mode == CD_CLIPAREA)
    iupdrvDrawSetClipRect(ctxcanvas->dc, xmin, ymin, xmax, ymax);
}

static int cdclip(cdCtxCanvas *ctxcanvas, int mode)
{
  if (mode == CD_CLIPAREA)
    iupdrvDrawSetClipRect(ctxcanvas->dc, ctxcanvas->canvas->clip_rect.xmin, ctxcanvas->canvas->clip_rect.ymin, ctxcanvas->canvas->clip_rect.xmax, ctxcanvas->canvas->clip_rect.ymax);
  else if (mode == CD_CLIPOFF)
    iupdrvDrawResetClip(ctxcanvas->dc);
  else if (mode == CD_CLIPPOLYGON || mode == CD_CLIPREGION)
    return ctxcanvas->canvas->clip_mode;
  return mode;
}


/******************************************************/
/* primitives                                         */
/******************************************************/

static int cd2iup_linestyle(cdCtxCanvas *ctxcanvas)
{
  if (ctxcanvas->canvas->line_style == CD_DASHED)
    return IUP_DRAW_STROKE_DASH;
  else if (ctxcanvas->canvas->line_style == CD_DOTTED)
    return IUP_DRAW_STROKE_DOT;
  else if (ctxcanvas->canvas->line_style == CD_DASH_DOT)
    return IUP_DRAW_STROKE_DASH_DOT;
  else if (ctxcanvas->canvas->line_style == CD_DASH_DOT_DOT)
    return IUP_DRAW_STROKE_DASH_DOT_DOT;
  else
    return IUP_DRAW_STROKE;
}

static void cdline(cdCtxCanvas *ctxcanvas, int px1, int py1, int px2, int py2)
{
  if (ctxcanvas->dc)
    iupdrvDrawLine(ctxcanvas->dc, px1, py1, px2, py2, ctxcanvas->canvas->foreground, cd2iup_linestyle(ctxcanvas), ctxcanvas->canvas->line_width);
}

static void cdrect(cdCtxCanvas *ctxcanvas, int xmin, int xmax, int ymin, int ymax)
{
  if (ctxcanvas->dc)
    iupdrvDrawRectangle(ctxcanvas->dc, xmin, ymin, xmax, ymax, ctxcanvas->canvas->foreground, cd2iup_linestyle(ctxcanvas), ctxcanvas->canvas->line_width);
}

static void cdbox(cdCtxCanvas *ctxcanvas, int xmin, int xmax, int ymin, int ymax)
{
  if (ctxcanvas->dc)
    iupdrvDrawRectangle(ctxcanvas->dc, xmin, ymin, xmax, ymax, ctxcanvas->canvas->foreground, IUP_DRAW_FILL, ctxcanvas->canvas->line_width);
}

static void cdarc(cdCtxCanvas *ctxcanvas, int xc, int yc, int w, int h, double a1, double a2)
{
  int rx = w / 2;
  int ry = h / 2;
  int x1 = xc - rx;
  int y1 = yc - ry;
  int x2 = xc + rx;
  int y2 = yc + ry;
  if (ctxcanvas->dc)
    iupdrvDrawArc(ctxcanvas->dc, x1, y1, x2, y2, a1, a2, ctxcanvas->canvas->foreground, cd2iup_linestyle(ctxcanvas), ctxcanvas->canvas->line_width);
}

static void cdsector(cdCtxCanvas *ctxcanvas, int xc, int yc, int w, int h, double a1, double a2)
{
  int rx = w / 2;
  int ry = h / 2;
  int x1 = xc - rx;
  int y1 = yc - ry;
  int x2 = xc + rx;
  int y2 = yc + ry;
  if (ctxcanvas->dc)
    iupdrvDrawArc(ctxcanvas->dc, x1, y1, x2, y2, a1, a2, ctxcanvas->canvas->foreground, IUP_DRAW_FILL, ctxcanvas->canvas->line_width);
}

/* TODO: not exported in CD dll yet */
static void x_cdRotatePoint(cdCanvas* canvas, int x, int y, int cx, int cy, int *rx, int *ry, double sin_theta, double cos_theta)
{
  double t;

  /* translate to (cx,cy) */
  x = x - cx;
  y = y - cy;

  /* rotate */
  if (canvas->invert_yaxis)
  {
    t = (x * cos_theta) + (y * sin_theta); *rx = _cdRound(t);
    t = -(x * sin_theta) + (y * cos_theta); *ry = _cdRound(t);
  }
  else
  {
    t = (x * cos_theta) - (y * sin_theta); *rx = _cdRound(t);
    t = (x * sin_theta) + (y * cos_theta); *ry = _cdRound(t);
  }

  /* translate back */
  *rx = *rx + cx;
  *ry = *ry + cy;
}

static void cdtext(cdCtxCanvas *ctxcanvas, int x, int y, const char *s, int len)
{
  int w, h, desc;
  int ox = x, oy = y;

  /* cxText method is called for single lines only */

  iupdrvFontGetFontDim(ctxcanvas->canvas->native_font, NULL, NULL, NULL, &desc);
  iupdrvFontGetTextSize(ctxcanvas->canvas->native_font, s, len, &w, &h);

  /* move to top-left corner of the text */
  switch (ctxcanvas->canvas->text_alignment)
  {
  case CD_BASE_RIGHT:
  case CD_NORTH_EAST:
  case CD_EAST:
  case CD_SOUTH_EAST:
    x = x - w;
    break;
  case CD_BASE_CENTER:
  case CD_CENTER:
  case CD_NORTH:
  case CD_SOUTH:
    x = x - w / 2;
    break;
  case CD_BASE_LEFT:
  case CD_NORTH_WEST:
  case CD_WEST:
  case CD_SOUTH_WEST:
    x = x;
    break;
  }

  switch (ctxcanvas->canvas->text_alignment)
  {
  case CD_BASE_LEFT:
  case CD_BASE_CENTER:
  case CD_BASE_RIGHT:
    y = y - h + desc;
    break;
  case CD_SOUTH_EAST:
  case CD_SOUTH_WEST:
  case CD_SOUTH:
    y = y - h;
    break;
  case CD_NORTH_EAST:
  case CD_NORTH:
  case CD_NORTH_WEST:
    y = y;
    break;
  case CD_CENTER:
  case CD_EAST:
  case CD_WEST:
    y = y - h / 2;
    break;
  }

  if (ctxcanvas->canvas->text_orientation)
  {
    double cos_angle = cos(ctxcanvas->canvas->text_orientation*CD_DEG2RAD);
    double sin_angle = sin(ctxcanvas->canvas->text_orientation*CD_DEG2RAD);
    x_cdRotatePoint(ctxcanvas->canvas, x, y, ox, oy, &x, &y, sin_angle, cos_angle);
  }

  if (ctxcanvas->dc)
    iupdrvDrawText(ctxcanvas->dc, s, len, x, y, w, h, ctxcanvas->canvas->foreground, ctxcanvas->canvas->native_font, IUP_DRAW_LEFT, ctxcanvas->canvas->text_orientation);  /* left alignment - unused, multiline alignment is done by CD, layout is NOT centered */
}

static void cdpoly(cdCtxCanvas *ctxcanvas, int mode, cdPoint* poly, int n)
{
  int style;

  if (mode == CD_BEZIER)
  {
    /* cdSimPolyBezier(ctxcanvas->canvas, poly, n); TODO: not exported in CD dll yet */
    return;
  }

  if (mode == CD_PATH)
  {
    cdSimPolyPath(ctxcanvas->canvas, poly, n);
    return;
  }

  if (mode == CD_CLIP || mode == CD_REGION)
    return;

  if (mode == CD_CLOSED_LINES || mode == CD_FILL)
  {
    poly[n].x = poly[0].x;
    poly[n].y = poly[0].y;
    n++;
  }

  if (mode == CD_FILL && ctxcanvas->canvas->interior_style == CD_HOLLOW)
    mode = CD_CLOSED_LINES;

  if (mode == CD_FILL)
    style = IUP_DRAW_STROKE;
  else
    style = cd2iup_linestyle(ctxcanvas);

  if (ctxcanvas->dc)
    iupdrvDrawPolygon(ctxcanvas->dc, (int*)poly, n, ctxcanvas->canvas->foreground, style, ctxcanvas->canvas->line_width);
}


/******************************************************/
/* client images                                      */
/******************************************************/


static void sFixImageY(int *y, int h)
{
  *y -= (h - 1);  /* move Y to top-left corner, since it was at the bottom of the image */
}

static void cdputimagerectrgba(cdCtxCanvas* ctxcanvas, int width, int height, const unsigned char *red,
                               const unsigned char *green, const unsigned char *blue, const unsigned char *alpha, int x, int y, int w, int h, int xmin, int xmax, int ymin, int ymax)
{
  int i, j, count;
  Ihandle *image;
  int pos;
  unsigned char *rgba;
  int dy = y;
  int nw, nh;

  sFixImageY(&dy, height);

  nw = xmax - xmin + 1;
  nh = ymax - ymin + 1;

  image = IupImageRGBA(nw, nh, NULL);
  IupSetHandle("_IUPDRAW_CD_IMAGE", image);
  rgba = (unsigned char*)IupGetAttribute(image, "WID");

  count = 0;
  for (i = ymin; i <= ymax; i++)
  {
    for (j = xmin; j <= xmax; j++)
    {
      pos = (ymax + ymin - i)*width + j;

      rgba[count++] = red[pos];
      rgba[count++] = green[pos];
      rgba[count++] = blue[pos];
      rgba[count++] = alpha[pos];
    }
  }

  if (ctxcanvas->dc)
    iupdrvDrawImage(ctxcanvas->dc, "_IUPDRAW_CD_IMAGE", 0, NULL, x, dy, w, h);

  IupDestroy(image);
  IupSetHandle("_IUPDRAW_CD_IMAGE", NULL);
}

static void cdputimagerectrgb(cdCtxCanvas* ctxcanvas, int width, int height, const unsigned char *red,
                              const unsigned char *green, const unsigned char *blue, int x, int y, int w, int h, int xmin, int xmax, int ymin, int ymax)
{
  int i, j, count;
  Ihandle *image;
  int pos;
  unsigned char *rgb;
  int dy = y;
  int nw, nh;

  sFixImageY(&dy, height);

  nw = xmax - xmin + 1;
  nh = ymax - ymin + 1;

  image = IupImageRGB(nw, nh, NULL);
  IupSetHandle("_IUPDRAW_CD_IMAGE", image);
  rgb = (unsigned char*)IupGetAttribute(image, "WID");

  count = 0;
  for (i = ymin; i <= ymax; i++)
  {
    for (j = xmin; j <= xmax; j++)
    {
      pos = (ymax + ymin - i)*width + j;

      rgb[count++] = red[pos];
      rgb[count++] = green[pos];
      rgb[count++] = blue[pos];
    }
  }

  if (ctxcanvas->dc)
    iupdrvDrawImage(ctxcanvas->dc, "_IUPDRAW_CD_IMAGE", 0, NULL, x, dy, w, h);

  IupDestroy(image);
  IupSetHandle("_IUPDRAW_CD_IMAGE", NULL);
}

static void cdputimagerectmap(cdCtxCanvas *ctxcanvas, int iw, int ih, const unsigned char *index, const long *colors,
                              int x, int y, int w, int h, int xmin, int xmax, int ymin, int ymax)
{
  int i, j, count;
  int pos;
  Ihandle *image;
  unsigned char *map;
  int dy = y;
  char name[60];
  int nh, nw;

  sFixImageY(&dy, ih);

  nw = xmax - xmin + 1;
  nh = ymax - ymin + 1;

  image = IupImage(nw, nh, NULL);
  IupSetHandle("_IUPDRAW_CD_IMAGE", image);
  map = (unsigned char*)IupGetAttribute(image, "WID");

  for (i = 0; i < 256; i++)
  {
    sprintf(name, "%d", i);
    IupSetRGB(image, name, cdRed(colors[i]), cdGreen(colors[i]), cdBlue(colors[i]));
  }

  count = 0;
  for (i = ymin; i <= ymax; i++)
  {
    for (j = xmin; j <= xmax; j++)
    {
      pos = (ymax + ymin - i)*iw + j;

      map[count++] = index[pos];
    }
  }

  if (ctxcanvas->dc)
    iupdrvDrawImage(ctxcanvas->dc, "_IUPDRAW_CD_IMAGE", 0, NULL, x, dy, w, h);

  IupDestroy(image);
  IupSetHandle("_IUPDRAW_CD_IMAGE", NULL);
}

/******************************************************/
/* server images                                      */
/******************************************************/


static void cdpixel(cdCtxCanvas *ctxcanvas, int x, int y, long color)
{
  if (ctxcanvas->dc)
    iupdrvDrawArc(ctxcanvas->dc, x, y, x, y, 0.0, 360.0, color, CD_FILL, 1);
}

static void cdcreatecanvas(cdCanvas* canvas, Ihandle *ih)
{
  cdCtxCanvas *ctxcanvas;

  ctxcanvas = (cdCtxCanvas *)malloc(sizeof(cdCtxCanvas));
  memset(ctxcanvas, 0, sizeof(cdCtxCanvas));

  ctxcanvas->canvas = canvas;
  ctxcanvas->ih = ih;
  canvas->ctxcanvas = ctxcanvas;

  ctxcanvas->canvas->bpp = IupGetInt(NULL, "SCREENDEPTH");
  ctxcanvas->canvas->xres = IupGetDouble(NULL, "SCREENDPI") / 25.4;
  ctxcanvas->canvas->yres = ctxcanvas->canvas->xres;

  IupSetAttribute(ih, "_CD_CANVAS", (char*)canvas);
}

static void cdinittable(cdCanvas* canvas)
{
  /* initialize function table*/
  canvas->cxFlush = cdflush;
  canvas->cxClear = cdclear;
  canvas->cxPixel = cdpixel;
  canvas->cxLine = cdline;
  canvas->cxPoly = cdpoly;
  canvas->cxRect = cdrect;
  canvas->cxBox = cdbox;
  canvas->cxArc = cdarc;
  canvas->cxSector = cdsector;
  canvas->cxChord = cdSimChord;
  canvas->cxText = cdtext;
  canvas->cxPutImageRectRGBA = cdputimagerectrgba;
  canvas->cxPutImageRectRGB = cdputimagerectrgb;
  canvas->cxPutImageRectMap = cdputimagerectmap;

  canvas->cxClip = cdclip;
  canvas->cxClipArea = cdcliparea;
  canvas->cxFont = cdfont;
  canvas->cxGetFontDim = cdgetfontdim;
  canvas->cxGetTextSize = cdgettextsize;

  canvas->cxKillCanvas = cdkillcanvas;
  canvas->cxActivate = cdactivate;
}

/******************************************************/

static cdContext cdIupDrawContext =
{
  CD_CAP_ALL & ~(CD_CAP_PLAY | CD_CAP_YAXIS | CD_CAP_FPRIMTIVES | CD_CAP_GETIMAGERGB |
                 CD_CAP_IMAGESRV | CD_CAP_BACKOPACITY | CD_CAP_WRITEMODE | CD_CAP_HATCH | CD_CAP_STIPPLE | CD_CAP_PATTERN |
                 CD_CAP_LINECAP | CD_CAP_LINEJOIN | CD_CAP_PATH | CD_CAP_BEZIER |
                 CD_CAP_PALETTE | CD_CAP_CLIPPOLY | CD_CAP_REGION | CD_CAP_CHORD),
  CD_CTX_WINDOW,
  cdcreatecanvas,
  cdinittable,
  NULL,
  NULL,
};

cdContext* cdContextIupDraw(void)
{
  return &cdIupDrawContext;
}

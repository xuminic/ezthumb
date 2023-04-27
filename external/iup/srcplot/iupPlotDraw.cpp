
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "iupPlot.h"
#include "iup_cdutil.h"
#include "iup_image.h"


static inline void iPlotDrawRectI(cdCanvas* canvas, int inX, int inY, int inW, int inH)
{
  cdCanvasRect(canvas, inX, inX + inW - 1, inY, inY + inH - 1);
}

static inline void iPlotDrawSector(cdCanvas* canvas, double inX, double inY, double inW, double inH, double inStartAngle, double inEndAngle)
{
  cdfCanvasSector(canvas, inX, inY, inW, inH, inStartAngle, inEndAngle);
}


/************************************************************************************/


void iupPlotBox::Draw(const iupPlotRect &inRect, cdCanvas* canvas) const
{
  cdCanvasSetForeground(canvas, mColor);
  iupPlotDrawSetLineStyle(canvas, mLineStyle, mLineWidth);

  iPlotDrawRectI(canvas, inRect.mX, inRect.mY, inRect.mWidth, inRect.mHeight);
}

bool iupPlotGrid::DrawX(iupPlotTickIter* inTickIter, iupPlotTrafo* inTrafo, const iupPlotRect &inRect, cdCanvas* canvas) const
{
  if (mShowX)
  {
    if (!inTickIter->Init())
      return false;

    double theX;
    bool theIsMajorTick;

    cdCanvasSetForeground(canvas, mColor);
    iupPlotDrawSetLineStyle(canvas, mLineStyle, mLineWidth);

    while (inTickIter->GetNextTick(theX, theIsMajorTick, NULL))
    {
      if ((theIsMajorTick && mMajor) || (!theIsMajorTick && !mMajor))
      {
        double theScreenX = inTrafo->Transform(theX);
        cdfCanvasLine(canvas, theScreenX, inRect.mY, theScreenX, inRect.mY + inRect.mHeight - 1);
      }
    }
  }

  return true;
}

bool iupPlotGrid::DrawY(iupPlotTickIter* inTickIter, iupPlotTrafo* inTrafo, const iupPlotRect &inRect, cdCanvas* canvas) const
{
  if (mShowY)
  {
    if (!inTickIter->Init())
      return false;

    double theY;
    bool theIsMajorTick;
    iupPlotRect theTickRect;

    cdCanvasSetForeground(canvas, mColor);
    iupPlotDrawSetLineStyle(canvas, mLineStyle, mLineWidth);

    while (inTickIter->GetNextTick(theY, theIsMajorTick, NULL))
    {
      if ((theIsMajorTick && mMajor) || (!theIsMajorTick && !mMajor))
      {
        double theScreenY = inTrafo->Transform(theY);
        cdfCanvasLine(canvas, inRect.mX, theScreenY, inRect.mX + inRect.mWidth - 1, theScreenY);
      }
    }
  }

  return true;
}


/************************************************************************************************/


void iupPlot::DrawCrossSamplesH(const iupPlotRect &inRect, const iupPlotData *inXData, const iupPlotData *inYData, cdCanvas* canvas) const
{
  int theCount = inXData->GetCount();
  if (theCount == 0)
    return;

  double theXTarget = mAxisX.mTrafo->TransformBack((double)mCrossHairX);
  bool theFirstIsLess = inXData->GetSample(0) < theXTarget;

  for (int i = 0; i < theCount; i++)
  {
    double theX = inXData->GetSample(i);
    bool theCurrentIsLess = theX < theXTarget;

    if (theCurrentIsLess != theFirstIsLess)
    {
      double theY = inYData->GetSample(i);
      int theScreenY = iupPlotRound(mAxisY.mTrafo->Transform(theY)); // transform to pixels
      // Draw a horizontal line at data Y coordinate
      cdfCanvasLine(canvas, inRect.mX, theScreenY, inRect.mX + inRect.mWidth - 1, theScreenY);

      theFirstIsLess = theCurrentIsLess;
    }
  }
}

void iupPlot::DrawCrossHairH(const iupPlotRect &inRect, cdCanvas* canvas) const
{
  cdCanvasSetForeground(canvas, mAxisY.mColor);
  iupPlotDrawSetLineStyle(canvas, CD_CONTINUOUS, 1);

  // Draw a vertical line at cursor X coordinate
  cdCanvasLine(canvas, mCrossHairX, inRect.mY, mCrossHairX, inRect.mY + inRect.mHeight - 1);

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];

    const iupPlotData *theXData = dataset->GetDataX();
    const iupPlotData *theYData = dataset->GetDataY();

    cdCanvasSetForeground(canvas, dataset->mColor);

    DrawCrossSamplesH(inRect, theXData, theYData, canvas);
  }
}

void iupPlot::DrawCrossSamplesV(const iupPlotRect &inRect, const iupPlotData *inXData, const iupPlotData *inYData, cdCanvas* canvas) const
{
  int theCount = inXData->GetCount();
  if (theCount == 0)
    return;

  double theYTarget = mAxisY.mTrafo->TransformBack((double)mCrossHairY);
  bool theFirstIsLess = inYData->GetSample(0) < theYTarget;

  for (int i = 0; i < theCount; i++)
  {
    double theY = inYData->GetSample(i);
    bool theCurrentIsLess = theY < theYTarget;

    if (theCurrentIsLess != theFirstIsLess)
    {
      double theX = inXData->GetSample(i);
      int theScreenX = iupPlotRound(mAxisX.mTrafo->Transform(theX)); // transform to pixels
      // Draw a vertical line at data X coordinate
      cdfCanvasLine(canvas, theScreenX, inRect.mY, theScreenX, inRect.mY + inRect.mHeight - 1);

      theFirstIsLess = theCurrentIsLess;
    }
  }
}

void iupPlot::DrawCrossHairV(const iupPlotRect &inRect, cdCanvas* canvas) const
{
  cdCanvasSetForeground(canvas, mAxisX.mColor);
  iupPlotDrawSetLineStyle(canvas, CD_CONTINUOUS, 1);

  // Draw an horizontal line at cursor Y coordinate
  cdCanvasLine(canvas, inRect.mX, mCrossHairY, inRect.mX + inRect.mWidth - 1, mCrossHairY);

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];

    const iupPlotData *theXData = dataset->GetDataX();
    const iupPlotData *theYData = dataset->GetDataY();

    cdCanvasSetForeground(canvas, dataset->mColor);

    DrawCrossSamplesV(inRect, theXData, theYData, canvas);
  }
}

void iupPlot::SetTitleFont(cdCanvas* canvas) const
{
  int theFontSize = mTitle.mFontSize;
  if (theFontSize == 0)
  {
    int size = IupGetInt(ih, "FONTSIZE");
    if (size > 0) size += 6;
    else size -= 8;

    theFontSize = size;
  }

  SetFont(canvas, mTitle.mFontStyle, theFontSize);
}

void iupPlot::DrawTitle(cdCanvas* canvas) const
{
  if (mTitle.GetText())
  {
    cdCanvasSetForeground(canvas, mTitle.mColor);

    SetTitleFont(canvas);

    cdCanvasTextAlignment(canvas, CD_NORTH);
    cdCanvasText(canvas, mTitle.mPosX, mTitle.mPosY, mTitle.GetText());
  }
}

void iupPlot::DrawBackground(cdCanvas* canvas) const
{
  if (!mBack.mTransparent)
  {
    cdCanvasOrigin(canvas, 0, 0);
    cdCanvasClip(canvas, CD_CLIPOFF);
    cdCanvasSetForeground(canvas, mBack.mColor);
    cdCanvasBox(canvas, mViewportBack.mX, mViewportBack.mX + mViewportBack.mWidth - 1, mViewportBack.mY, mViewportBack.mY + mViewportBack.mHeight - 1);
  }
}

void iupPlot::DrawInactive(cdCanvas* canvas) const
{
  cdCanvasOrigin(canvas, 0, 0);
  cdCanvasClip(canvas, CD_CLIPOFF);
  long inactive_color = cdEncodeAlpha(CD_GRAY, 96);
  cdCanvasSetForeground(canvas, inactive_color);
  cdCanvasBox(canvas, mViewportBack.mX, mViewportBack.mX + mViewportBack.mWidth - 1, mViewportBack.mY, mViewportBack.mY + mViewportBack.mHeight - 1);
}

void iupPlot::DrawBackgroundImage(cdCanvas* canvas) const
{
  Ihandle* image = IupImageGetHandle(mBack.GetImage());
  if (image)
  {
    double theScreenMinX = mAxisX.mTrafo->Transform(mBack.mImageMinX);
    double theScreenMinY = mAxisY.mTrafo->Transform(mBack.mImageMinY);
    double theScreenMaxX = mAxisX.mTrafo->Transform(mBack.mImageMaxX);
    double theScreenMaxY = mAxisY.mTrafo->Transform(mBack.mImageMaxY);

    double theScreenW = theScreenMaxX - theScreenMinX + 1;
    double theScreenH = theScreenMaxY - theScreenMinY + 1;

    int theX = iupPlotRound(theScreenMinX);
    int theY = iupPlotRound(theScreenMinY);
    int theW = iupPlotRound(theScreenW);
    int theH = iupPlotRound(theScreenH);

    cdIupDrawImage(canvas, image, theX, theY, theW, theH, 0, mBack.mColor);
  }
}

bool iupPlot::DrawLegend(const iupPlotRect &inRect, cdCanvas* canvas, iupPlotRect &ioPos) const
{
  if (mLegend.mShow)
  {
    int ds;
    int theFontHeight;

    SetFont(canvas, mLegend.mFontStyle, mLegend.mFontSize);
    cdCanvasGetFontDim(canvas, NULL, &theFontHeight, NULL, NULL);

    int theMargin = theFontHeight / 2;
    if (mLegend.mPosition == IUP_PLOT_BOTTOMCENTER)
      theMargin = 0;
    int theTotalHeight = mDataSetListCount*theFontHeight + 2 * theMargin;
    int theLineSpace = 20;

    int theWidth, theMaxWidth = 0;
    for (ds = 0; ds < mDataSetListCount; ds++)
    {
      iupPlotDataSet* dataset = mDataSetList[ds];

      cdCanvasGetTextSize(canvas, dataset->GetName(), &theWidth, NULL);

      if (dataset->mMode == IUP_PLOT_MARK || dataset->mMode == IUP_PLOT_MARKLINE)
      {
        if (dataset->mMarkSize + 6 > theLineSpace)
          theLineSpace = dataset->mMarkSize + 6;
      }

      theWidth += theLineSpace;

      if (theWidth > theMaxWidth)
        theMaxWidth = theWidth;
    }

    if (theMaxWidth == 0)
      return false;

    theMaxWidth += 2 * theMargin;

    if (mLegend.mPosition != IUP_PLOT_XY)
    {
      int theScreenX = inRect.mX;
      int theScreenY = inRect.mY;

      switch (mLegend.mPosition)
      {
      case IUP_PLOT_TOPLEFT:
        theScreenX += 2;
        theScreenY += inRect.mHeight - theTotalHeight - 2;
        break;
      case IUP_PLOT_BOTTOMLEFT:
        theScreenX += 2;
        theScreenY += 2;
        break;
      case IUP_PLOT_BOTTOMRIGHT:
        theScreenX += inRect.mWidth - theMaxWidth - 2;
        theScreenY += 2;
        break;
      case IUP_PLOT_BOTTOMCENTER:
        theScreenX += (inRect.mWidth - theMaxWidth) / 2;
        theScreenY = theFontHeight / 4;
        break;
      default: // IUP_PLOT_TOPRIGHT
        theScreenX += inRect.mWidth - theMaxWidth - 2;
        theScreenY += inRect.mHeight - theTotalHeight - 2;
        break;
      }

      ioPos.mX = theScreenX;
      ioPos.mY = theScreenY;
    }

    ioPos.mWidth = theMaxWidth;
    ioPos.mHeight = theTotalHeight;

    // Clip to the legend box
    cdCanvasClipArea(canvas, ioPos.mX, ioPos.mX + ioPos.mWidth - 1,
                             ioPos.mY, ioPos.mY + ioPos.mHeight - 1);

    if (mLegend.mBoxShow)
    {
      cdCanvasSetForeground(canvas, mLegend.mBoxBackColor);
      iupPlotDrawBox(canvas, ioPos.mX + 1, ioPos.mY + 1, ioPos.mWidth - 2, ioPos.mHeight - 2);

      cdCanvasSetForeground(canvas, mLegend.mBoxColor);
      iupPlotDrawSetLineStyle(canvas, mLegend.mBoxLineStyle, mLegend.mBoxLineWidth);
      iPlotDrawRectI(canvas, ioPos.mX, ioPos.mY, ioPos.mWidth, ioPos.mHeight);
    }

    for (ds = 0; ds < mDataSetListCount; ds++)
    {
      iupPlotDataSet* dataset = mDataSetList[ds];

      cdCanvasSetForeground(canvas, dataset->mColor);

      int theLegendX = ioPos.mX + theMargin;
      int theLegendY = ioPos.mY + (mDataSetListCount - 1 - ds)*theFontHeight + theMargin;

      theLegendY += theFontHeight / 2;

      if (dataset->mMode == IUP_PLOT_MARK || dataset->mMode == IUP_PLOT_MARKLINE)
      {
        iPlotSetMark(canvas, dataset->mMarkStyle, dataset->mMarkSize);
        cdCanvasMark(canvas, theLegendX + (theLineSpace - 3) / 2, theLegendY - theFontHeight / 8);
      }
      if (dataset->mMode != IUP_PLOT_MARK)
      {
        iupPlotDrawSetLineStyle(canvas, dataset->mLineStyle, dataset->mLineWidth);
        cdCanvasLine(canvas, theLegendX, theLegendY - theFontHeight / 8,
                             theLegendX + theLineSpace - 3, theLegendY - theFontHeight / 8);
      }

      iupPlotDrawText(canvas, theLegendX + theLineSpace, theLegendY, CD_WEST, dataset->GetName());
    }
  }

  return true;
}

int iupStrToColor(const char* str, long *color);

long iupPlotDrawGetSampleColorTable(Ihandle* ih, int index)
{
  char* value = IupGetAttributeId(ih, "SAMPLECOLOR", index);
  long color;
  if (iupStrToColor(value, &color))
    return color;

  switch (index % 12)
  {
  case  0: return cdEncodeColor(220, 60, 20);
  case  1: return cdEncodeColor(0, 128, 0);
  case  2: return cdEncodeColor(20, 100, 220);

  case  3: return cdEncodeColor(220, 128, 0);
  case  4: return cdEncodeColor(128, 0, 128);
  case  5: return cdEncodeColor(0, 128, 220);

  case  6: return cdEncodeColor(220, 60, 128);
  case  7: return cdEncodeColor(128, 220, 0);
  case  8: return cdEncodeColor(192, 60, 60);

  case  9: return cdEncodeColor(60, 60, 128);
  case 10: return cdEncodeColor(220, 60, 220);
  case 11: return cdEncodeColor(60, 128, 128);
  }

  return 0;
}

bool iupPlot::DrawSampleColorLegend(iupPlotDataSet *dataset, const iupPlotRect &inRect, cdCanvas* canvas, iupPlotRect &ioPos) const
{
  if (mLegend.mShow)
  {
    int theFontHeight;

    SetFont(canvas, mLegend.mFontStyle, mLegend.mFontSize);
    cdCanvasGetFontDim(canvas, NULL, &theFontHeight, NULL, NULL);

    int theMargin = theFontHeight / 2;
    if (mLegend.mPosition == IUP_PLOT_BOTTOMCENTER)
      theMargin = 0;
    int theCount = dataset->GetCount();
    int theTotalHeight = theCount*theFontHeight + 2 * theMargin;
    int theLineSpace = theFontHeight / 2 + 3;

    int theWidth, theMaxWidth = 0;
    for (int i = 0; i < theCount; i++)
    {
      cdCanvasGetTextSize(canvas, ((iupPlotDataString *)dataset->GetDataX())->GetSampleString(i), &theWidth, NULL);

      theWidth += theLineSpace;

      if (theWidth > theMaxWidth)
        theMaxWidth = theWidth;
    }

    if (theMaxWidth == 0)
      return false;

    theMaxWidth += 2 * theMargin;

    if (mLegend.mPosition != IUP_PLOT_XY)
    {
      int theScreenX = inRect.mX;
      int theScreenY = inRect.mY;

      switch (mLegend.mPosition)
      {
      case IUP_PLOT_TOPLEFT:
        theScreenX += 2;
        theScreenY += inRect.mHeight - theTotalHeight - 2;
        break;
      case IUP_PLOT_BOTTOMLEFT:
        theScreenX += 2;
        theScreenY += 2;
        break;
      case IUP_PLOT_BOTTOMRIGHT:
        theScreenX += inRect.mWidth - theMaxWidth - 2;
        theScreenY += 2;
        break;
      case IUP_PLOT_BOTTOMCENTER:
        theScreenX += (inRect.mWidth - theMaxWidth) / 2;
        theScreenY = theFontHeight / 4;
        break;
      default: // IUP_PLOT_TOPRIGHT
        theScreenX += inRect.mWidth - theMaxWidth - 2;
        theScreenY += inRect.mHeight - theTotalHeight - 2;
        break;
      }

      ioPos.mX = theScreenX;
      ioPos.mY = theScreenY;
    }

    ioPos.mWidth = theMaxWidth;
    ioPos.mHeight = theTotalHeight;

    // Clip to the legend box
    cdCanvasClipArea(canvas, ioPos.mX, ioPos.mX + ioPos.mWidth - 1,
                             ioPos.mY, ioPos.mY + ioPos.mHeight - 1);

    if (mLegend.mBoxShow)
    {
      cdCanvasSetForeground(canvas, mLegend.mBoxBackColor);
      iupPlotDrawBox(canvas, ioPos.mX + 1, ioPos.mY + 1, ioPos.mWidth - 2, ioPos.mHeight - 2);

      cdCanvasSetForeground(canvas, mLegend.mBoxColor);
      iupPlotDrawSetLineStyle(canvas, mLegend.mBoxLineStyle, mLegend.mBoxLineWidth);
      iPlotDrawRectI(canvas, ioPos.mX, ioPos.mY, ioPos.mWidth, ioPos.mHeight);
    }

    for (int i = 0; i < theCount; i++)
    {
      cdCanvasSetForeground(canvas, iupPlotDrawGetSampleColorTable(ih, i));

      int theLegendX = ioPos.mX + theMargin;
      int theLegendY = ioPos.mY + (theCount - 1 - i)*theFontHeight + theMargin;

      int boxSize = theLineSpace - 3;

      cdCanvasBox(canvas, theLegendX, theLegendX + boxSize, theLegendY, theLegendY + boxSize);

      iupPlotDrawText(canvas, theLegendX + theLineSpace, theLegendY + boxSize / 2, CD_WEST, ((iupPlotDataString *)dataset->GetDataX())->GetSampleString(i));
    }
  }

  return true;
}



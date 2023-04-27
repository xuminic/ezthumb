
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "iupPlot.h"



const double kMajorTickXInitialFac = 2.0;
const double kMajorTickYInitialFac = 3.0;
const double kRangeVerySmall = (double)1.0e-3;


void iupPlot::CalculateTitlePos()
{
  // it does not depend on theMargin
  if (mTitle.mAutoPos)
  {
    mTitle.mPosX = mViewport.mWidth / 2;
    mTitle.mPosY = mViewport.mHeight - 1 - mBack.mVertPadding;
  }
}

bool iupPlot::CheckInsideTitle(cdCanvas* canvas, int x, int y) const
{
  // it does not depend on theMargin
  if (mTitle.GetText())
  {
    SetTitleFont(canvas);

    cdCanvasTextAlignment(canvas, CD_NORTH);

    int xmin, xmax, ymin, ymax;
    cdCanvasGetTextBox(canvas, mTitle.mPosX, mTitle.mPosY, mTitle.GetText(), &xmin, &xmax, &ymin, &ymax);

    if (x >= xmin && x <= xmax && 
        y >= ymin && y <= ymax)
      return true;
  }

  return false;
}

bool iupPlot::CheckInsideLegend(int x, int y) const
{
  if (mLegend.mShow)
  {
    if (x >= mLegend.mPos.mX && x < mLegend.mPos.mX + mLegend.mPos.mWidth &&
        y >= mLegend.mPos.mY && y < mLegend.mPos.mY + mLegend.mPos.mHeight)
      return true;
  }

  return false;
}

int iupPlot::CalcTitleVerticalMargin(cdCanvas* canvas) const
{
  SetTitleFont(canvas);

  int theTextHeight;
  cdCanvasGetTextSize(canvas, mTitle.GetText(), NULL, &theTextHeight);
  return theTextHeight + 5 + theTextHeight / 2;
}

int iupPlot::CalcXTickHorizontalMargin(cdCanvas* canvas, bool start) const
{
  int theXTickHorizontalMargin = 0;

  if (mAxisX.mShowArrow)
  {
    if ((!start && !mAxisX.mReverse) ||
        ( start &&  mAxisX.mReverse))
      theXTickHorizontalMargin += mAxisX.GetArrowSize();
  }

  if (mAxisX.mTick.mShow && mAxisX.mTick.mShowNumber)  // Leave space for half number
    theXTickHorizontalMargin = iupPlotMax(theXTickHorizontalMargin, mAxisX.GetTickNumberWidth(canvas) / 2);

  return theXTickHorizontalMargin;
}

int iupPlot::CalcYTickVerticalMargin(cdCanvas* canvas, bool start) const
{
  int theYTickVerticalMargin = 0;

  if (mAxisY.mShowArrow)
  {
    if ((!start && !mAxisY.mReverse) ||
        ( start &&  mAxisY.mReverse))
        theYTickVerticalMargin += mAxisY.GetArrowSize();
  }

  if (mAxisY.mTick.mShow && mAxisY.mTick.mShowNumber) // Leave space for half number
    theYTickVerticalMargin = iupPlotMax(theYTickVerticalMargin, mAxisY.GetTickNumberHeight(canvas) / 2);

  return theYTickVerticalMargin;
}

int iupPlot::CalcXTickVerticalMargin(cdCanvas* canvas) const
{
  int theXTickVerticalMargin = 0;

  if (mAxisX.mTick.mShow)
  {
    theXTickVerticalMargin += mAxisX.mTick.mMajorSize;

    if (mAxisX.mTick.mShowNumber)
      theXTickVerticalMargin += mAxisX.GetTickNumberHeight(canvas) + mAxisX.mTick.mMinorSize;  // Use minor size as spacing
  }

  if (mAxisX.GetLabel())
  {
    int theXFontHeight;
    SetFont(canvas, mAxisX.mFontStyle, mAxisX.mFontSize);
    cdCanvasGetFontDim(canvas, NULL, &theXFontHeight, NULL, NULL);

    theXTickVerticalMargin += theXFontHeight + theXFontHeight / 10;
  }

  return theXTickVerticalMargin;
}

int iupPlot::CalcYTickHorizontalMargin(cdCanvas* canvas) const
{
  int theYTickHorizontalMargin = 0;

  if (mAxisY.mTick.mShow)
  {
    theYTickHorizontalMargin += mAxisY.mTick.mMajorSize;

    if (mAxisY.mTick.mShowNumber)
      theYTickHorizontalMargin += mAxisY.GetTickNumberWidth(canvas) + mAxisY.mTick.mMinorSize;  // Use minor size as spacing
  }

  if (mAxisY.GetLabel())
  {
    int theYFontHeight;
    SetFont(canvas, mAxisY.mFontStyle, mAxisY.mFontSize);
    cdCanvasGetFontDim(canvas, NULL, &theYFontHeight, NULL, NULL);

    theYTickHorizontalMargin += theYFontHeight + theYFontHeight / 10;
  }

  return theYTickHorizontalMargin;
}

void iupPlot::CalculateMargins(cdCanvas* canvas)
{
  if (mBack.mMarginAuto.mTop)
  {
    mBack.mMargin.mTop = 0;

    if (mTitle.GetText() && mTitle.mAutoPos)  // Affects only top margin
      mBack.mMargin.mTop += CalcTitleVerticalMargin(canvas);

    if (mAxisX.mShow && mAxisX.mPosition == IUP_PLOT_END)
      mBack.mMargin.mTop += CalcXTickVerticalMargin(canvas);

    if (mAxisY.mShow)
      mBack.mMargin.mTop = iupPlotMax(mBack.mMargin.mTop, CalcYTickVerticalMargin(canvas, false));
  }

  if (mBack.mMarginAuto.mBottom)
  {
    mBack.mMargin.mBottom = 0;

    if (mAxisX.mShow && mAxisX.mPosition == IUP_PLOT_START)
      mBack.mMargin.mBottom += CalcXTickVerticalMargin(canvas);

    if (mAxisY.mShow)
      mBack.mMargin.mBottom = iupPlotMax(mBack.mMargin.mBottom, CalcYTickVerticalMargin(canvas, true));
  }

  if (mBack.mMarginAuto.mLeft)
  {
    mBack.mMargin.mLeft = 0;

    if (mAxisY.mShow && mAxisY.mPosition == IUP_PLOT_START)
      mBack.mMargin.mLeft += CalcYTickHorizontalMargin(canvas);

    if (mAxisX.mShow)
      mBack.mMargin.mLeft = iupPlotMax(mBack.mMargin.mLeft, CalcXTickHorizontalMargin(canvas, true));
  }

  if (mBack.mMarginAuto.mRight)
  {
    mBack.mMargin.mRight = 0;

    if (mAxisY.mShow && mAxisY.mPosition == IUP_PLOT_END)
      mBack.mMargin.mRight += CalcYTickHorizontalMargin(canvas);

    if (mAxisX.mShow)
      mBack.mMargin.mRight = iupPlotMax(mBack.mMargin.mRight, CalcXTickHorizontalMargin(canvas, false));
  }
}

void iupPlot::CalculateXRange(double &outXMin, double &outXMax) const
{
  bool theFirst = true;
  outXMin = 0;
  outXMax = 0;

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    const iupPlotData *theXData = mDataSetList[ds]->GetDataX();

    if (theXData->GetCount() == 0)
      continue;

    double theXMin;
    double theXMax;
    if (mDataSetList[ds]->mMode == IUP_PLOT_PIE)
    {
      theXMin = -1;
      theXMax = 1;
    }
    else if (!theXData->CalculateRange(theXMin, theXMax))
      return;
    
    if (theFirst) 
    {
      outXMin = theXMin;
      outXMax = theXMax;
      theFirst = false;
    }
    if (theXMax>outXMax)
      outXMax = theXMax;
    if (theXMin<outXMin)
      outXMin = theXMin;
  }
}

void iupPlot::CalculateYRange(double &outYMin, double &outYMax) const
{
  bool theFirst = true;
  outYMin = 0;
  outYMax = 0;

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    const iupPlotData *theYData = mDataSetList[ds]->GetDataY();

    double theYMin;
    double theYMax;
    if (mDataSetList[ds]->mMode == IUP_PLOT_PIE)
    {
      theYMin = -1;
      theYMax = 1;
    }
    else if (!theYData->CalculateRange(theYMin, theYMax))
      return;
    
    if (theFirst) 
    {
      outYMin = theYMin;
      outYMax = theYMax;
      theFirst = false;
    }
    if (theYMin<outYMin) 
      outYMin = theYMin;
    if (theYMax>outYMax)
      outYMax = theYMax;
  }
}

bool iupPlot::CalculateAxisRange() 
{
  if (mAxisX.mAutoScaleMin || mAxisX.mAutoScaleMax) 
  {
    double theXMin = 0.0;
    double theXMax = 1.0;

    CalculateXRange(theXMin, theXMax);

    if (mAxisX.mAutoScaleMin)
    {
      mAxisX.mMin = theXMin;
      if (mAxisX.mLogScale && (theXMin < kLogMinClipValue)) 
        mAxisX.mMin = kLogMinClipValue;
    }

    if (mAxisX.mAutoScaleMax) 
      mAxisX.mMax = theXMax;

    if (!mAxisX.mTickIter->AdjustRange(mAxisX.mMin, mAxisX.mMax)) 
      return false;
  }

  if (mAxisY.mAutoScaleMin || mAxisY.mAutoScaleMax) 
  {
    double theYMin = 0.0;
    double theYMax = 1.0;

    CalculateYRange(theYMin, theYMax);

    if (mAxisY.mAutoScaleMin) 
    {
      mAxisY.mMin = theYMin;
      if (mAxisY.mLogScale && (theYMin < kLogMinClipValue)) 
        mAxisY.mMin = kLogMinClipValue;
    }
    if (mAxisY.mAutoScaleMax)
      mAxisY.mMax = theYMax;

    if (!mAxisY.mTickIter->AdjustRange(mAxisY.mMin, mAxisY.mMax))
      return false;
  }

  if (mScaleEqual)
  {
    if (mAxisY.HasZoom() || mAxisX.HasZoom())
    {
      if (mAxisY.mMax - mAxisY.mMin != mAxisX.mMax - mAxisX.mMin)
      {
        double theLength;

        if (mAxisY.mMax - mAxisY.mMin > mAxisX.mMax - mAxisX.mMin)
        {
          theLength = mAxisY.mMax - mAxisY.mMin;
          mAxisX.mMax = mAxisX.mMin + theLength;
        }
        else
        {
          theLength = mAxisX.mMax - mAxisX.mMin;
          mAxisY.mMax = mAxisY.mMin + theLength;
        }
      }
    }
    else
    {
      double theMin = mAxisY.mMin;
      if (mAxisX.mMin < theMin)
        theMin = mAxisX.mMin;

      double theMax = mAxisY.mMax;
      if (mAxisX.mMax > theMax)
        theMax = mAxisX.mMax;

      mAxisX.mMin = theMin;
      mAxisY.mMin = theMin;
      mAxisX.mMax = theMax;
      mAxisY.mMax = theMax;
    }
  }

  return true;
}

bool iupPlot::CheckRange(const iupPlotAxis &inAxis) const
{
  if (inAxis.mLogScale) 
  {
    if (inAxis.mMin < kLogMinClipValue)
      return false;
  }
  return true;
}


bool iupPlot::CalculateXTransformation(const iupPlotRect &inRect)
{
  return mAxisX.mTrafo->Calculate(inRect.mX, inRect.mX + inRect.mWidth, mAxisX);
}

bool iupPlot::CalculateYTransformation(const iupPlotRect &inRect)
{
  return mAxisY.mTrafo->Calculate(inRect.mY, inRect.mY + inRect.mHeight, mAxisY);
}

void iupPlot::CalculateTickSize(cdCanvas* canvas, iupPlotTick &ioTick)
{
  if (ioTick.mAutoSize)
  {
    int theFontHeight;
    SetFont(canvas, ioTick.mFontStyle, ioTick.mFontSize);
    cdCanvasGetFontDim(canvas, NULL, &theFontHeight, NULL, NULL);

    ioTick.mMajorSize = theFontHeight / 2;
    ioTick.mMinorSize = theFontHeight / 4;
  }
}

bool iupPlot::CalculateTickSpacing(const iupPlotRect &inRect, cdCanvas* canvas)
{
  double theXRange = mAxisX.mMax - mAxisX.mMin;
  double theYRange = mAxisY.mMax - mAxisY.mMin;

  if (theXRange <= 0 || theYRange < 0)
    return false;

  // YRange can be 0, must adjust
  if (mAxisY.mAutoScaleMax && ((mAxisY.mMax != 0 && fabs(theYRange / mAxisY.mMax) < kRangeVerySmall) || theYRange == 0))
  {
    double delta = 0.1;
    if (mAxisY.mMax != 0)
      delta *= fabs(mAxisY.mMax);

    mAxisY.mMax += delta;
    theYRange = mAxisY.mMax - mAxisY.mMin;
  }

  if (mAxisX.mTick.mAutoSpacing)
  {
    int theXFontHeight;
    SetFont(canvas, mAxisX.mTick.mFontStyle, mAxisX.mTick.mFontSize);
    cdCanvasGetFontDim(canvas, NULL, &theXFontHeight, NULL, NULL);

    int theTextWidth;
    cdCanvasGetTextSize(canvas, "12345", &theTextWidth, NULL);

    double theDivGuess = inRect.mWidth / (kMajorTickXInitialFac*theTextWidth);
    if (!mAxisX.mTickIter->CalculateSpacing(theXRange, theDivGuess, mAxisX.mTick))
      return false;
  }

  if (mAxisY.mTick.mAutoSpacing)
  {
    int theYFontHeight;
    SetFont(canvas, mAxisY.mTick.mFontStyle, mAxisY.mTick.mFontSize);
    cdCanvasGetFontDim(canvas, NULL, &theYFontHeight, NULL, NULL);

    double theDivGuess = inRect.mHeight / (kMajorTickYInitialFac*theYFontHeight);
    if (!mAxisY.mTickIter->CalculateSpacing(theYRange, theDivGuess, mAxisY.mTick))
      return false;
  }

  return true;
}


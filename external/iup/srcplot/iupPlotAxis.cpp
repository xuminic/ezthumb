
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "iupPlot.h"


static inline void iPlotCheckMinMax(double &inoutMin, double &inoutMax)
{
  if (inoutMin > inoutMax)
  {
    double theTmp = inoutMin;
    inoutMin = inoutMax;
    inoutMax = theTmp;
  }
}

static int iPlotCountDigit(int inNum)
{
  int theCount = 0;
  while (inNum != 0)
  {
    inNum = inNum / 10;
    theCount++;
  }
  if (theCount == 0) theCount = 1;
  return theCount;
}

static int iPlotEstimateNumberCharCount(bool inFormatAuto, const char* inFormatString, double inMin, double inMax)
{
  int thePrecision = 0;

  while (*inFormatString)
  {
    if (*inFormatString == '.')
      break;
    inFormatString++;
  }

  if (*inFormatString == '.')
  {
    inFormatString++;
    iupStrToInt(inFormatString, &thePrecision);
  }

  if (inFormatAuto)
  {
    int theMinPrecision = iupPlotCalcPrecision(inMin);
    int theMaxPrecision = iupPlotCalcPrecision(inMax);
    if (theMinPrecision > theMaxPrecision)
      thePrecision = iupPlotMax(thePrecision, theMinPrecision);
    else
      thePrecision = iupPlotMax(thePrecision, theMaxPrecision);
  }

  int theMin = iupPlotRound(inMin);
  int theMax = iupPlotRound(inMax);
  int theNumDigitMin = iPlotCountDigit(theMin);
  int theNumDigitMax = iPlotCountDigit(theMax);
  if (theNumDigitMin > theNumDigitMax)
    thePrecision += theNumDigitMin;
  else
    thePrecision += theNumDigitMax;

  thePrecision += 3;  // sign, decimal symbol, exp 

  return thePrecision;
}

static bool iPlotGetTickFormat(Ihandle* ih, IFnssds formatticknumber_cb, char* inBuf, const char *inFormatString, double inValue)
{
  char* decimal_symbol = IupGetGlobal("DEFAULTDECIMALSYMBOL");

  if (formatticknumber_cb)
  {
    int ret = formatticknumber_cb(ih, inBuf, (char*)inFormatString, inValue, decimal_symbol);
    if (ret == IUP_IGNORE)
      return false;
    else if (ret == IUP_CONTINUE)
      iupStrPrintfDoubleLocale(inBuf, inFormatString, inValue, decimal_symbol);
  }
  else
    iupStrPrintfDoubleLocale(inBuf, inFormatString, inValue, decimal_symbol);

  return true;
}


/************************************************************************************************/


double iupPlotTrafoLinear::Transform(double inValue) const
{
  return inValue * mSlope + mOffset;
}

double iupPlotTrafoLinear::TransformBack(double inValue) const
{
  if (mSlope != 0)
    return (inValue - mOffset) / mSlope;
  else
    return 0;
}

bool iupPlotTrafoLinear::Calculate(int inBegin, int inEnd, const iupPlotAxis& inAxis)
{
  double theDataRange = inAxis.mMax - inAxis.mMin;
  if (theDataRange < kFloatSmall)
    return false;

  double theMin = inAxis.mMin;
  if (inAxis.mDiscrete)
  {
    theDataRange++;
    theMin -= 0.5;
  }

  double theTargetRange = inEnd - inBegin;
  double theScale = theTargetRange / theDataRange;

  if (!inAxis.mReverse)
    mOffset = inBegin - theMin * theScale;
  else
    mOffset = inEnd + theMin * theScale;

  mSlope = theScale;
  if (inAxis.mReverse)
    mSlope *= -1;

  return true;
}


/************************************************************************************************/


double iupPlotTrafoLog::Transform(double inValue) const
{
  if (inValue < kLogMinClipValue) inValue = kLogMinClipValue;
  return iupPlotLog(inValue, mBase)*mSlope + mOffset;
}

double iupPlotTrafoLog::TransformBack(double inValue) const
{
  if (mSlope != 0)
    return iupPlotExp((inValue - mOffset) / mSlope, mBase);
  else
    return 0;
}

bool iupPlotTrafoLog::Calculate(int inBegin, int inEnd, const iupPlotAxis& inAxis)
{
  double theBase = inAxis.mLogBase;
  double theDataRange = iupPlotLog(inAxis.mMax, theBase) - iupPlotLog(inAxis.mMin, theBase);
  if (theDataRange < kFloatSmall)
    return false;

  double theTargetRange = inEnd - inBegin;
  double theScale = theTargetRange / theDataRange;

  if (!inAxis.mReverse)
    mOffset = inBegin - iupPlotLog(inAxis.mMin, theBase) * theScale;
  else
    mOffset = inEnd + iupPlotLog(inAxis.mMin, theBase) * theScale;

  mBase = theBase;

  mSlope = theScale;
  if (inAxis.mReverse)
    mSlope *= -1;

  return true;
}


/************************************************************************************************/


void iupPlotAxis::Init()
{
  if (mLogScale)
  {
    mTickIter = &mLogTickIter;
    mTrafo = &mLogTrafo;
  }
  else
  {
    mTickIter = &mLinTickIter;
    mTrafo = &mLinTrafo;
  }

  mTickIter->SetAxis(this);

  if (mPosition == IUP_PLOT_START)
    mReverseTicksLabel = false;
  if (mPosition == IUP_PLOT_END)
    mReverseTicksLabel = true;
}

void iupPlotAxis::GetTickNumberSize(cdCanvas* canvas, int *outWitdh, int *outHeight) const
{
  int theTickFontWidth, theTickFontHeight;
  SetFont(canvas, mTick.mFontStyle, mTick.mFontSize);
  cdCanvasGetTextSize(canvas, "1234567890.", &theTickFontWidth, &theTickFontHeight);
  theTickFontWidth /= 11;
  if (outHeight) *outHeight = theTickFontHeight;
  if (outWitdh)  *outWitdh  = theTickFontWidth * iPlotEstimateNumberCharCount(mTick.mFormatAuto, mTick.mFormatString, mHasZoom? mNoZoomMin: mMin, mHasZoom? mNoZoomMax: mMax);
}

void iupPlotAxis::SetNamedTickIter(const iupPlotDataString *inStringData)
{
  mTickIter = &mNamedTickIter;
  mTickIter->SetAxis(this);
  mNamedTickIter.SetStringList(inStringData);
}

void iupPlotAxis::CheckZoomOutLimit(double inRange)
{
  if (mMin < mNoZoomMin)
  {
    mMin = mNoZoomMin;
    mMax = mMin + inRange;
    if (mMax > mNoZoomMax)
      mMax = mNoZoomMax;
  }

  if (mMax > mNoZoomMax)
  {
    mMax = mNoZoomMax;
    mMin = mMax - inRange;
    if (mMin < mNoZoomMin)
      mMin = mNoZoomMin;
  }
}

void iupPlotAxis::InitZoom()
{
  if (!mHasZoom)
  {
    // Save NoZoom state values
    mNoZoomMin = mMin;
    mNoZoomMax = mMax;
    mNoZoomAutoScaleMin = mAutoScaleMin;
    mNoZoomAutoScaleMax = mAutoScaleMax;

    mHasZoom = true;

    // disable AutoScale
    mAutoScaleMin = false;
    mAutoScaleMax = false;
  }
}

bool iupPlotAxis::ResetZoom()
{
  if (mHasZoom)
  {
    mHasZoom = false;

    // Restore NoZoom state values
    mMin = mNoZoomMin;
    mMax = mNoZoomMax;
    mAutoScaleMin = mNoZoomAutoScaleMin;
    mAutoScaleMax = mNoZoomAutoScaleMax;
    return true;
  }

  return false;
}

bool iupPlotAxis::ZoomOut(double inCenter)
{
  if (inCenter < mMin || inCenter > mMax)
    return false;

  if (!mHasZoom)
    return false;

  double theRange = mMax - mMin;
  double theNewRange = theRange * 1.1; // 10%
  double theFactor = (inCenter - mMin) / theRange;
  double theOffset = (theNewRange - theRange);

  mMin -= theOffset*theFactor;
  mMax += theOffset*(1.0 - theFactor);

  CheckZoomOutLimit(theNewRange);

  if (mMin == mNoZoomMin && mMax == mNoZoomMax)
    ResetZoom();

  return true;
}

bool iupPlotAxis::ZoomIn(double inCenter)
{
  if (inCenter < mMin || inCenter > mMax)
    return false;

  InitZoom();

  double theRange = mMax - mMin;
  double theNewRange = theRange * 0.9; // 10%
  double theFactor = (inCenter - mMin) / theRange;
  double theOffset = (theRange - theNewRange);

  mMin += theOffset*theFactor;
  mMax -= theOffset*(1.0 - theFactor);

  // Check limits only in ZoomOut and Pan
  return true;
}

bool iupPlotAxis::ZoomTo(double inMin, double inMax)
{
  InitZoom();

  iPlotCheckMinMax(inMin, inMax);

  if (inMin > mNoZoomMax || inMax < mNoZoomMin)
    return false;

  if (inMin < mNoZoomMin) inMin = mNoZoomMin;
  if (inMax > mNoZoomMax) inMax = mNoZoomMax;

  mMin = inMin;
  mMax = inMax;

  if (mMin == mNoZoomMin && mMax == mNoZoomMax)
    ResetZoom();

  return true;
}

bool iupPlotAxis::Pan(double inOffset)
{
  if (!mHasZoom)
    return false;

  double theRange = mMax - mMin;

  mMin = mPanMin - inOffset;
  mMax = mMin + theRange;

  CheckZoomOutLimit(theRange);
  return true;
}

bool iupPlotAxis::Scroll(double inDelta, bool inFullPage)
{
  if (!mHasZoom)
    return false;

  double theRange = mMax - mMin;

  double thePage;
  if (inFullPage)
    thePage = theRange;
  else
    thePage = theRange / 10.0;

  double theOffset = thePage * inDelta;

  mMin += theOffset;
  mMax += theOffset;

  CheckZoomOutLimit(theRange);
  return true;
}

bool iupPlotAxis::ScrollTo(double inMin)
{
  if (inMin < mNoZoomMin || inMin > mNoZoomMax)
    return false;

  if (!mHasZoom)
    return false;

  double theRange = mMax - mMin;

  mMin = inMin;
  mMax = mMin + theRange;

  CheckZoomOutLimit(theRange);
  return true;
}


/************************************************************************************************/


static inline void iPlotDrawRotatedText(cdCanvas* canvas, double inX, double inY, double inDegrees, int inAlignment, const char *inString)
{
  cdCanvasTextAlignment(canvas, inAlignment);
  double theOldOrientation = cdCanvasTextOrientation(canvas, inDegrees);
  cdfCanvasText(canvas, inX, inY, inString);
  cdCanvasTextOrientation(canvas, theOldOrientation);
}

static void iPlotFillArrowI(cdCanvas* canvas, int inX1, int inY1, int inX2, int inY2, int inX3, int inY3)
{
  cdCanvasBegin(canvas, CD_FILL);
  cdCanvasVertex(canvas, inX1, inY1);
  cdCanvasVertex(canvas, inX2, inY2);
  cdCanvasVertex(canvas, inX3, inY3);
  cdCanvasEnd(canvas);
}

static void iPlotDrawArrow(cdCanvas* canvas, double inX, double inY, int inVertical, int inDirection, int inSize)
{
  int theX = iupPlotRound(inX);
  int theY = iupPlotRound(inY);

  int theSizeDir = iupPlotRound(inSize * 0.7);
  if (inVertical)
  {
    cdfCanvasLine(canvas, inX, inY, inX, inY + inDirection*inSize);  // complement the axis

    int theY1 = iupPlotRound(inY + inDirection*inSize);
    int theY2 = theY1 - inDirection*theSizeDir;
    iPlotFillArrowI(canvas, theX, theY1,
                            theX - theSizeDir, theY2,
                            theX + theSizeDir, theY2);
  }
  else
  {
    cdfCanvasLine(canvas, inX, inY, inX + inDirection*inSize, inY);

    int theX1 = iupPlotRound(inX + inDirection*inSize);
    int theX2 = theX1 - inDirection*theSizeDir;
    iPlotFillArrowI(canvas, theX1, theY,
                            theX2, theY - theSizeDir,
                            theX2, theY + theSizeDir);
  }
}

void iupPlotAxis::SetFont(cdCanvas* canvas, int inFontStyle, int inFontSize) const
{
  if (inFontStyle == -1) inFontStyle = mDefaultFontStyle;
  if (inFontSize == 0) inFontSize = mDefaultFontSize;
  cdCanvasFont(canvas, NULL, inFontStyle, inFontSize);
}

int iupPlotAxis::GetTickNumberHeight(cdCanvas* canvas) const
{
  int height;
  if (mTick.mRotateNumber)
  {
    int theXTickNumberWidth;
    GetTickNumberSize(canvas, &theXTickNumberWidth, NULL);
    height = theXTickNumberWidth;
  }
  else
  {
    int theXTickNumberHeight;
    GetTickNumberSize(canvas, NULL, &theXTickNumberHeight);
    height = theXTickNumberHeight;
  }
  return height;
}

int iupPlotAxis::GetTickNumberWidth(cdCanvas* canvas) const
{
  int width;
  if (mTick.mRotateNumber)
  {
    int theYTickNumberHeight;
    GetTickNumberSize(canvas, NULL, &theYTickNumberHeight);
    width = theYTickNumberHeight;
  }
  else
  {
    int theYTickNumberWidth;
    GetTickNumberSize(canvas, &theYTickNumberWidth, NULL);
    width = theYTickNumberWidth;
  }
  return width;
}

int iupPlotAxis::GetArrowSize() const
{
  return mTick.mMinorSize + 2;  // to avoid too small sizes
}


/*****************************************************************************/


double iupPlotAxisX::GetScreenYOriginX(const iupPlotAxis& inAxisY) const
{
  double theTargetY = 0;
  if (mPosition != IUP_PLOT_CROSSORIGIN)
  {
    if (mPosition == IUP_PLOT_START)
    {
      if (inAxisY.mReverse)
        theTargetY = inAxisY.mMax;
      else
        theTargetY = inAxisY.mMin;
    }
    else
    {
      if (inAxisY.mReverse)
        theTargetY = inAxisY.mMin;
      else
        theTargetY = inAxisY.mMax;
    }
  }
  if (inAxisY.mDiscrete)
    theTargetY -= 0.5;

  return inAxisY.mTrafo->Transform(theTargetY);
}

bool iupPlotAxisX::DrawX(const iupPlotRect &inRect, cdCanvas* canvas, const iupPlotAxis& inAxisY, Ihandle* ih) const
{
  if (!mShow)
    return true;

  cdCanvasSetForeground(canvas, mColor);
  iupPlotDrawSetLineStyle(canvas, CD_CONTINUOUS, mLineWidth);

  double theScreenY = GetScreenYOriginX(inAxisY);
  double theScreenX1 = inRect.mX;
  double theScreenX2 = theScreenX1 + inRect.mWidth;

  cdfCanvasLine(canvas, theScreenX1, theScreenY, theScreenX2, theScreenY);

  if (mShowArrow)
  {
    if (!mReverse)
      iPlotDrawArrow(canvas, theScreenX2, theScreenY, 0, 1, GetArrowSize());
    else
      iPlotDrawArrow(canvas, theScreenX1, theScreenY, 0, -1, GetArrowSize());
  }

  if (mTick.mShow)
  {
    if (!mTickIter->Init())
      return false;

    double theX;
    bool theIsMajorTick;
    char theFormatString[30];
    strcpy(theFormatString, mTick.mFormatString);

    IFnssds formatticknumber_cb = (IFnssds)IupGetCallback(ih, "XTICKFORMATNUMBER_CB");

    if (mTick.mShowNumber)
      SetFont(canvas, mTick.mFontStyle, mTick.mFontSize);

    while (mTickIter->GetNextTick(theX, theIsMajorTick, theFormatString))
      DrawXTick(theX, theScreenY, theIsMajorTick, theFormatString, canvas, ih, formatticknumber_cb);

    int theTickSpace = mTick.mMajorSize;  // skip major tick
    if (mTick.mShowNumber)
      theTickSpace += GetTickNumberHeight(canvas) + mTick.mMinorSize;  // Use minor size as spacing

    if (mReverseTicksLabel)
      theScreenY += theTickSpace;
    else
      theScreenY -= theTickSpace;
  }

  if (GetLabel())
  {
    SetFont(canvas, mFontStyle, mFontSize);

    int theLabelSpacing = mLabelSpacing;
    if (mLabelSpacing == -1)
    {
      int theXFontHeight;
      cdCanvasGetFontDim(canvas, NULL, &theXFontHeight, NULL, NULL);
      theLabelSpacing = theXFontHeight / 10;  // default spacing
    }

    if (mReverseTicksLabel)
      theScreenY += theLabelSpacing;
    else
      theScreenY -= theLabelSpacing;

    if (mLabelCentered)
    {
      double theScreenX = theScreenX1 + inRect.mWidth / 2;
      iupPlotDrawText(canvas, theScreenX, theScreenY, mReverseTicksLabel? CD_SOUTH: CD_NORTH, GetLabel());
    }
    else
    {
      double theScreenX = theScreenX2;
      iupPlotDrawText(canvas, theScreenX, theScreenY, mReverseTicksLabel? CD_SOUTH_EAST: CD_NORTH_EAST, GetLabel());
    }
  }

  return true;
}

void iupPlotAxisX::DrawXTick(double inX, double inScreenY, bool inMajor, const char*inFormatString, cdCanvas* canvas, Ihandle* ih, IFnssds formatticknumber_cb) const
{
  int theTickSize;
  double theScreenX = mTrafo->Transform(inX);
  if (inMajor)
  {
    theTickSize = mTick.mMajorSize;

    if (mTick.mShowNumber)
    {
      char theBuf[128];
      if (iPlotGetTickFormat(ih, formatticknumber_cb, theBuf, inFormatString, inX))
      {
        double theScreenY;
        if (mReverseTicksLabel)
          theScreenY = inScreenY + theTickSize + mTick.mMinorSize;  // Use minor size as spacing
        else
          theScreenY = inScreenY - theTickSize - mTick.mMinorSize;  // Use minor size as spacing

        // SetFont called in DrawX
        if (mTick.mRotateNumber)
          iPlotDrawRotatedText(canvas, theScreenX, theScreenY, mTick.mRotateNumberAngle, mReverseTicksLabel ? CD_WEST : CD_EAST, theBuf);
        else
          iupPlotDrawText(canvas, theScreenX, theScreenY, mReverseTicksLabel ? CD_SOUTH : CD_NORTH, theBuf);
      }
    }
  }
  else
    theTickSize = mTick.mMinorSize;

  if (mReverseTicksLabel)
    cdfCanvasLine(canvas, theScreenX, inScreenY, theScreenX, inScreenY + theTickSize);
  else
    cdfCanvasLine(canvas, theScreenX, inScreenY, theScreenX, inScreenY - theTickSize);
}


/*****************************************************************************/


double iupPlotAxisY::GetScreenXOriginY(const iupPlotAxis& inAxisX) const
{
  double theTargetX = 0;
  if (mPosition != IUP_PLOT_CROSSORIGIN)
  {
    if (mPosition == IUP_PLOT_START)
    {
      if (inAxisX.mReverse)
        theTargetX = inAxisX.mMax;
      else
        theTargetX = inAxisX.mMin;
    }
    else
    {
      if (inAxisX.mReverse)
        theTargetX = inAxisX.mMin;
      else
        theTargetX = inAxisX.mMax;
    }
  }
  if (inAxisX.mDiscrete)
    theTargetX -= 0.5;

  return inAxisX.mTrafo->Transform(theTargetX);
}

bool iupPlotAxisY::DrawY(const iupPlotRect &inRect, cdCanvas* canvas, const iupPlotAxis& inAxisX, Ihandle* ih) const
{
  if (!mShow)
    return true;

  cdCanvasSetForeground(canvas, mColor);
  iupPlotDrawSetLineStyle(canvas, CD_CONTINUOUS, mLineWidth);

  double theScreenX = GetScreenXOriginY(inAxisX);
  double theScreenY1 = inRect.mY;
  double theScreenY2 = theScreenY1 + inRect.mHeight;

  cdfCanvasLine(canvas, theScreenX, theScreenY1, theScreenX, theScreenY2);

  if (mShowArrow)
  {
    if (!mReverse)
      iPlotDrawArrow(canvas, theScreenX, theScreenY2, 1, 1, GetArrowSize());
    else
      iPlotDrawArrow(canvas, theScreenX, theScreenY1, 1, -1, GetArrowSize());
  }

  if (mTick.mShow)
  {
    if (!mTickIter->Init())
      return false;

    double theY;
    bool theIsMajorTick;
    char theFormatString[30];
    strcpy(theFormatString, mTick.mFormatString);

    IFnssds formatticknumber_cb = (IFnssds)IupGetCallback(ih, "YTICKFORMATNUMBER_CB");

    if (mTick.mShowNumber)
      SetFont(canvas, mTick.mFontStyle, mTick.mFontSize);

    while (mTickIter->GetNextTick(theY, theIsMajorTick, theFormatString))
      DrawYTick(theY, theScreenX, theIsMajorTick, theFormatString, canvas, ih, formatticknumber_cb);

    int theTickSpace = mTick.mMajorSize;  // skip major tick
    if (mTick.mShowNumber)
      theTickSpace += GetTickNumberWidth(canvas) + mTick.mMinorSize;  // Use minor size as spacing

    if (mReverseTicksLabel)
      theScreenX += theTickSpace;
    else
      theScreenX -= theTickSpace;
  }

  if (GetLabel())
  {
    SetFont(canvas, mFontStyle, mFontSize);

    int theLabelSpacing = mLabelSpacing;
    if (mLabelSpacing == -1)
    {
      int theYFontHeight;
      cdCanvasGetFontDim(canvas, NULL, &theYFontHeight, NULL, NULL);
      theLabelSpacing = theYFontHeight / 10;  // default spacing
    }

    if (mReverseTicksLabel)
      theScreenX += theLabelSpacing;
    else
      theScreenX -= theLabelSpacing;

    if (mLabelCentered)
    {
      double theScreenY = theScreenY1 + inRect.mHeight / 2;
      iPlotDrawRotatedText(canvas, theScreenX, theScreenY, 90, mReverseTicksLabel? CD_NORTH: CD_SOUTH, GetLabel());
    }
    else
    {
      double theScreenY = theScreenY2;
      iPlotDrawRotatedText(canvas, theScreenX, theScreenY, 90, mReverseTicksLabel? CD_NORTH_EAST: CD_SOUTH_EAST, GetLabel());
    }
  }

  return true;
}

void iupPlotAxisY::DrawYTick(double inY, double inScreenX, bool inMajor, const char* inFormatString, cdCanvas* canvas, Ihandle* ih, IFnssds formatticknumber_cb) const
{
  int theTickSize;
  double theScreenY = mTrafo->Transform(inY);
  if (inMajor)
  {
    theTickSize = mTick.mMajorSize;

    if (mTick.mShowNumber)
    {
      char theBuf[128];
      if (iPlotGetTickFormat(ih, formatticknumber_cb, theBuf, inFormatString, inY))
      {
        double theScreenX;
        if (mReverseTicksLabel)
          theScreenX = inScreenX + theTickSize + mTick.mMinorSize;  // Use minor size as spacing
        else
          theScreenX = inScreenX - theTickSize - mTick.mMinorSize;  // Use minor size as spacing

        // SetFont called in DrawX
        if (mTick.mRotateNumber)
          iPlotDrawRotatedText(canvas, theScreenX, theScreenY, mTick.mRotateNumberAngle, mReverseTicksLabel ? CD_NORTH : CD_SOUTH, theBuf);
        else
          iupPlotDrawText(canvas, theScreenX, theScreenY, mReverseTicksLabel ? CD_WEST : CD_EAST, theBuf);
      }
    }
  }
  else
    theTickSize = mTick.mMinorSize;

  if (mReverseTicksLabel)
    cdfCanvasLine(canvas, inScreenX, theScreenY, inScreenX + theTickSize, theScreenY);
  else
    cdfCanvasLine(canvas, inScreenX, theScreenY, inScreenX - theTickSize, theScreenY);
}

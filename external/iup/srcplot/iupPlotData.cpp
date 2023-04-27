
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "iupPlot.h"


static inline bool iPlotCheckInsideBox(double x, double y, double boxMinX, double boxMaxX, double boxMinY, double boxMaxY)
{
  if (x > boxMaxX || x < boxMinX || y > boxMaxY || y < boxMinY)
    return false;

  return true;
}


/************************************************************************************************/


bool iupPlotDataReal::CalculateRange(double &outMin, double &outMax) const
{
  int theCount = iupArrayCount(mArray);
  if (theCount > 0)
  {
    double* theData = (double*)iupArrayGetData(mArray);
    outMax = outMin = theData[0];
    for (int i = 1; i < theCount; i++)
    {
      if (theData[i] > outMax)
        outMax = theData[i];
      if (theData[i] < outMin)
        outMin = theData[i];
    }
    return true;
  }

  return false;
}

iupPlotDataString::~iupPlotDataString()
{
  for (int i = 0; i < mCount; i++)
    free(mData[i]);
}

bool iupPlotDataString::CalculateRange(double &outMin, double &outMax) const
{
  if (mCount > 0)
  {
    outMin = 0;
    outMax = mCount - 1;
    return true;
  }

  return false;
}

bool iupPlotDataBool::CalculateRange(double &outMin, double &outMax) const
{
  if (mCount > 0)
  {
    outMin = 0;
    outMax = mCount - 1;
    return true;
  }

  return false;
}


/************************************************************************************************/


iupPlotDataSet::iupPlotDataSet(bool strXdata)
: mColor(CD_BLACK), mLineStyle(CD_CONTINUOUS), mLineWidth(1), mAreaTransparency(255), mMarkStyle(CD_X), mMarkSize(7),
  mMultibarIndex(-1), mMultibarCount(0), mBarOutlineColor(0), mBarShowOutline(false), mBarSpacingPercent(10),
  mPieStartAngle(0), mPieRadius(0.95), mPieContour(false), mPieHole(0), mPieSliceLabelPos(0.95),
  mHighlightedSample(-1), mHighlightedCurve(false), mBarMulticolor(false), mOrderedX(false), mSelectedCurve(false),
  mPieSliceLabel(IUP_PLOT_NONE), mMode(IUP_PLOT_LINE), mName(NULL), mHasSelected(false), mUserData(0)
{
  if (strXdata)
    mDataX = (iupPlotData*)(new iupPlotDataString());
  else
    mDataX = (iupPlotData*)(new iupPlotDataReal());

  mDataY = (iupPlotData*)new iupPlotDataReal();

  mSelection = new iupPlotDataBool();
  mSegment = NULL;
  mExtra = NULL;
}

iupPlotDataSet::~iupPlotDataSet()
{
  SetName(NULL);

  delete mDataX;
  delete mDataY;
  delete mSelection;
  if (mSegment)
    delete mSegment;
  if (mExtra)
    delete mExtra;
}

bool iupPlotDataSet::FindSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY, double inScreenTolerance,
                                int &outSampleIndex, double &outX, double &outY) const
{
  switch (mMode)
  {
  case IUP_PLOT_MULTIBAR:
    return this->FindMultipleBarSample(inTrafoX, inTrafoY, inScreenX, inScreenY, outSampleIndex, outX, outY);
  case IUP_PLOT_BAR:
    return this->FindBarSample(inTrafoX, inTrafoY, inScreenX, inScreenY, outSampleIndex, outX, outY);
  case IUP_PLOT_HORIZONTALBAR:
    return this->FindHorizontalBarSample(inTrafoX, inTrafoY, inScreenX, inScreenY, outSampleIndex, outX, outY);
  case IUP_PLOT_PIE:
    return this->FindPieSample(inTrafoX, inTrafoY, inScreenX, inScreenY, outSampleIndex, outX, outY);
  default:
    return this->FindPointSample(inTrafoX, inTrafoY, inScreenX, inScreenY, inScreenTolerance, outSampleIndex, outX, outY);
  }
}

bool iupPlotDataSet::FindPointSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY, double inScreenTolerance,
                                     int &outSampleIndex, double &outX, double &outY) const
{
  double theX, theY, theScreenX, theScreenY;
  double thePrevScreenX = 0;

  int theCount = mDataX->GetCount();

  for (int i = 0; i < theCount; i++)
  {
    theX = mDataX->GetSample(i);
    theY = mDataY->GetSample(i);
    theScreenX = inTrafoX->Transform(theX);
    theScreenY = inTrafoY->Transform(theY);

    // optimization when X values are ordered
    if (mOrderedX && i > 0 && (inScreenX < thePrevScreenX - inScreenTolerance || inScreenX > theScreenX + inScreenTolerance))
    {
      if (inScreenX < thePrevScreenX - inScreenTolerance)
        break;

      thePrevScreenX = theScreenX;
      continue;
    }

    if (fabs(theScreenX - inScreenX) < inScreenTolerance &&
        fabs(theScreenY - inScreenY) < inScreenTolerance)
    {
      outX = theX;
      outY = theY;
      outSampleIndex = i;
      return true;
    }

    thePrevScreenX = theScreenX;
  }

  return false;
}

bool iupPlotDataSet::FindMultipleBarSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY,
                                           int &outSampleIndex, double &outX, double &outY) const
{
  double theX, theY, theScreenX, theScreenY;

  int theCount = mDataX->GetCount();

  double theScreenY0 = inTrafoY->Transform(0);

  double theMinX = mDataX->GetSample(0);
  double theScreenMinX = inTrafoX->Transform(theMinX);
  double theMaxX = mDataX->GetSample(theCount - 1);
  double theScreenMaxX = inTrafoX->Transform(theMaxX);

  double theTotalBarWidth = (theScreenMaxX - theScreenMinX) / (theCount - 1);
  theTotalBarWidth *= 1 - (double)mBarSpacingPercent / 100.0;
  double theBarWidth = theTotalBarWidth / mMultibarCount;

  for (int i = 0; i < theCount; i++)
  {
    theX = mDataX->GetSample(i);
    theY = mDataY->GetSample(i);
    theScreenX = inTrafoX->Transform(theX);
    theScreenY = inTrafoY->Transform(theY);

    double theBarX = (theScreenX - theTotalBarWidth / 2) + (mMultibarIndex*theBarWidth);
    double theBarHeight = theScreenY - theScreenY0;

    if (iPlotCheckInsideBox(inScreenX, inScreenY, theBarX, theBarX + theBarWidth, theScreenY0, theScreenY0 + theBarHeight))
    {
      outX = theX;
      outY = theY;
      outSampleIndex = i;
      return true;
    }
  }

  return false;
}

bool iupPlotDataSet::FindBarSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY,
                                   int &outSampleIndex, double &outX, double &outY) const
{
  double theX, theY, theScreenX, theScreenY;

  int theCount = mDataX->GetCount();

  double theScreenY0 = inTrafoY->Transform(0);

  double theMinX = mDataX->GetSample(0);
  double theScreenMinX = inTrafoX->Transform(theMinX);
  double theMaxX = mDataX->GetSample(theCount - 1);
  double theScreenMaxX = inTrafoX->Transform(theMaxX);

  double theBarWidth = (theScreenMaxX - theScreenMinX) / (theCount - 1);
  theBarWidth *= 1 - (double)mBarSpacingPercent / 100.0;

  for (int i = 0; i < theCount; i++)
  {
    theX = mDataX->GetSample(i);
    theY = mDataY->GetSample(i);
    theScreenX = inTrafoX->Transform(theX);
    theScreenY = inTrafoY->Transform(theY);

    double theBarX = theScreenX - theBarWidth / 2;
    double theBarHeight = theScreenY - theScreenY0;

    if (iPlotCheckInsideBox(inScreenX, inScreenY, theBarX, theBarX + theBarWidth, theScreenY0, theScreenY0 + theBarHeight))
    {
      outX = theX;
      outY = theY;
      outSampleIndex = i;
      return true;
    }
  }

  return false;
}

bool iupPlotDataSet::FindHorizontalBarSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY,
                                             int &outSampleIndex, double &outX, double &outY) const
{
  double theX, theY, theScreenX, theScreenY;

  int theCount = mDataX->GetCount();

  double theScreenX0 = inTrafoX->Transform(0);

  double theMinY = mDataY->GetSample(0);
  double theScreenMinY = inTrafoY->Transform(theMinY);
  double theMaxY = mDataY->GetSample(theCount - 1);
  double theScreenMaxY = inTrafoY->Transform(theMaxY);

  double theBarHeight = (theScreenMaxY - theScreenMinY) / (theCount - 1);
  theBarHeight *= 1 - (double)mBarSpacingPercent / 100.0;

  for (int i = 0; i < theCount; i++)
  {
    theX = mDataX->GetSample(i);
    theY = mDataY->GetSample(i);
    theScreenX = inTrafoX->Transform(theX);
    theScreenY = inTrafoY->Transform(theY);

    double theBarY = theScreenY - theBarHeight / 2;
    double theBarWidth = theScreenX - theScreenX0;

    if (iPlotCheckInsideBox(inScreenX, inScreenY, theScreenX0, theScreenX0 + theBarWidth, theBarY, theBarY + theBarHeight))
    {
      outX = theX;
      outY = theY;
      outSampleIndex = i;
      return true;
    }
  }

  return false;
}

bool iupPlotDataSet::FindPieSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY, int &outSampleIndex, double &outX, double &outY) const
{
  double theX, theY;

  int theCount = mDataX->GetCount();
  double sum = 0;

  for (int i = 0; i < theCount; i++)
  {
    theY = mDataY->GetSample(i);

    if (theY <= 0)
      continue;

    sum += theY;
  }

  double inX = inTrafoX->TransformBack(inScreenX);
  double inY = inTrafoY->TransformBack(inScreenY);

  double inRadius = sqrt(inX*inX + inY*inY);

  double holeRadius = mPieHole * mPieRadius;

  double inAngle = atan2(inY, inX);

  inAngle = CD_RAD2DEG*inAngle;

  if (inAngle < 0)
    inAngle += 360.;

  if (inRadius < holeRadius || inRadius > mPieRadius)
    return false;

  double startAngle = mPieStartAngle;

  for (int i = 0; i < theCount; i++)
  {
    theX = mDataX->GetSample(i);
    theY = mDataY->GetSample(i);

    if (theY <= 0)
      continue;

    double angle = (theY*360.) / sum;

    if (inAngle > startAngle  &&
        inAngle < startAngle + angle)
    {
      outX = theX;
      outY = theY;
      outSampleIndex = i;
      return true;
    }

    startAngle += angle;
  }

  return false;
}

static bool iPlotCheckInsideBoxTolerance(double x1, double y1, double x2, double y2, double inX, double inY, double inTolerance)
{
  if (x1 > x2)
  {
    double tmp = x1;
    x1 = x2;
    x2 = tmp;
  }

  if (y1 > y2)
  {
    double tmp = y1;
    y1 = y2;
    y2 = tmp;
  }

  x1 -= inTolerance;
  x2 += inTolerance;

  y1 -= inTolerance;
  y2 += inTolerance;

  if (inX < x1 || inX > x2)
    return false;

  if (inY < y1 || inY > y2)
    return false;

  return true;
}

bool iupPlotDataSet::FindSegment(iupPlotTrafo *mTrafoX, iupPlotTrafo *mTrafoY, double inScreenX, double inScreenY, double inScreenTolerance,
                                 int &outSampleIndex1, int &outSampleIndex2, double &outX1, double &outY1, double &outX2, double &outY2) const
{
  if (!mTrafoX || !mTrafoY)
    return false;

  double lowestDist = 0;
  int found_Id = -1;
  double found_x1 = 0, found_y1 = 0, found_x2 = 0, found_y2 = 0;
  bool found = false;

  double theX1 = mDataX->GetSample(0);
  double theY1 = mDataY->GetSample(0);
  double theScreenX1 = mTrafoX->Transform(theX1);
  double theScreenY1 = mTrafoY->Transform(theY1);

  int theCount = mDataX->GetCount();
  for (int i = 0; i < theCount - 1; i++)
  {
    double theX2 = mDataX->GetSample(i + 1);
    double theY2 = mDataY->GetSample(i + 1);
    double theScreenX2 = mTrafoX->Transform(theX2);
    double theScreenY2 = mTrafoY->Transform(theY2);

    // optimization when X values are ordered
    if (mOrderedX && (inScreenX < theScreenX1 || inScreenX > theScreenX2))
    {
      if (inScreenX < theScreenX1)
        break;

      theX1 = theX2;
      theY1 = theY2;
      theScreenX1 = theScreenX2;
      theScreenY1 = theScreenY2;
      continue;
    }

    // inX,inY must be inside box theScreenX1,theScreenY1 - theScreenX2,theScreenY2
    if (!iPlotCheckInsideBoxTolerance(theScreenX1, theScreenY1, theScreenX2, theScreenY2, inScreenX, inScreenY, inScreenTolerance))
    {
      theX1 = theX2;
      theY1 = theY2;
      theScreenX1 = theScreenX2;
      theScreenY1 = theScreenY2;
      continue;
    }

    double v1x = theScreenX2 - theScreenX1;
    double v1y = theScreenY2 - theScreenY1;

    double v1 = v1x*v1x + v1y*v1y;

    double v2x = inScreenX - theScreenX1;
    double v2y = inScreenY - theScreenY1;

    double prod = v1x*v2x + v1y*v2y;

    if (v1 == 0.)
    {
      theX1 = theX2;
      theY1 = theY2;
      theScreenX1 = theScreenX2;
      theScreenY1 = theScreenY2;
      continue;
    }

    double p1 = prod / v1;

    if (p1<0. || p1>1.)
    {
      theX1 = theX2;
      theY1 = theY2;
      theScreenX1 = theScreenX2;
      theScreenY1 = theScreenY2;
      continue;
    }

    double px = theScreenX1 + (theScreenX2 - theScreenX1)*p1;
    double py = theScreenY1 + (theScreenY2 - theScreenY1)*p1;

    double d = sqrt((inScreenX - px)*(inScreenX - px) + (inScreenY - py)*(inScreenY - py));

    if (!found || fabs(d) < lowestDist)
    {
      lowestDist = fabs(d);
      found_Id = i;
      found_x1 = theX1;
      found_x2 = theX2;
      found_y1 = theY1;
      found_y2 = theY2;
      found = true;
    }

    theX1 = theX2;
    theY1 = theY2;
    theScreenX1 = theScreenX2;
    theScreenY1 = theScreenY2;
  }

  if (found && lowestDist < inScreenTolerance)
  {
    outSampleIndex1 = found_Id;
    outSampleIndex2 = found_Id + 1;
    outX1 = found_x1;
    outY1 = found_y1;
    outX2 = found_x2;
    outY2 = found_y2;
    return true;
  }

  return false;
}

bool iupPlotDataSet::SelectSamples(double inMinX, double inMaxX, double inMinY, double inMaxY, const iupPlotSampleNotify* inNotify)
{
  bool theChanged = false;
  mHasSelected = false;

  int theCount = mDataX->GetCount();
  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    bool theSelected = mSelection->GetSampleBool(i);

    if (theX >= inMinX && theX <= inMaxX &&
        theY >= inMinY && theY <= inMaxY)
    {
      mHasSelected = true;

      if (!theSelected)
      {
        if (inNotify->cb)
        {
          int ret = inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)theSelected);
          if (ret == IUP_IGNORE)
            continue;
        }

        theChanged = true;
        mSelection->SetSampleBool(i, true);
      }
    }
    else
    {
      if (theSelected)
      {
        if (inNotify->cb)
        {
          int ret = inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)theSelected);
          if (ret == IUP_IGNORE)
            continue;
        }

        theChanged = true;
        mSelection->SetSampleBool(i, false);
      }
    }
  }

  return theChanged;
}

bool iupPlotDataSet::ClearSelection(const iupPlotSampleNotify* inNotify)
{
  bool theChanged = false;

  if (!mHasSelected)
    return theChanged;

  mHasSelected = false;

  int theCount = mDataX->GetCount();
  for (int i = 0; i < theCount; i++)
  {
    bool theSelected = mSelection->GetSampleBool(i);
    if (theSelected)
    {
      if (inNotify->cb)
      {
        double theX = mDataX->GetSample(i);
        double theY = mDataY->GetSample(i);
        int ret = inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)theSelected);
        if (ret == IUP_IGNORE)
          continue;
      }

      theChanged = true;
      mSelection->SetSampleBool(i, false);
    }
  }

  return theChanged;
}

bool iupPlotDataSet::DeleteSelectedSamples(const iupPlotSampleNotify* inNotify)
{
  bool theChanged = false;

  if (!mHasSelected)
    return theChanged;

  mHasSelected = false;

  int theCount = mDataX->GetCount();
  for (int i = theCount - 1; i >= 0; i--)
  {
    bool theSelected = mSelection->GetSampleBool(i);
    if (theSelected)
    {
      if (inNotify->cb)
      {
        double theX = mDataX->GetSample(i);
        double theY = mDataY->GetSample(i);
        int ret = inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)theSelected);
        if (ret == IUP_IGNORE)
          continue;
      }

      theChanged = true;
      RemoveSample(i);
    }
  }

  return theChanged;
}

int iupPlotDataSet::GetCount()
{
  return mDataX->GetCount();
}

void iupPlotDataSet::AddSample(double inX, double inY)
{
  iupPlotDataReal *theXData = (iupPlotDataReal*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (theXData->IsString())
    return;

  theXData->AddSample(inX);
  theYData->AddSample(inY);
  mSelection->AddSample(false);
  if (mSegment)
    mSegment->AddSample(false);
  if (mExtra)
    mExtra->AddSample(0);
}

void iupPlotDataSet::InsertSample(int inSampleIndex, double inX, double inY)
{
  iupPlotDataReal *theXData = (iupPlotDataReal*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (theXData->IsString())
    return;

  theXData->InsertSample(inSampleIndex, inX);
  theYData->InsertSample(inSampleIndex, inY);
  mSelection->InsertSample(inSampleIndex, false);
  if (mSegment)
    mSegment->InsertSample(inSampleIndex, false);
  if (mExtra)
    mExtra->InsertSample(inSampleIndex, 0);
}

void iupPlotDataSet::InitSegment()
{
  mSegment = new iupPlotDataBool();

  int theCount = mDataX->GetCount();
  for (int i = 0; i < theCount; i++)
    mSegment->AddSample(false);
}

void iupPlotDataSet::InitExtra()
{
  mExtra = new iupPlotDataReal();

  int theCount = mDataX->GetCount();
  for (int i = 0; i < theCount; i++)
    mExtra->AddSample(0);
}

void iupPlotDataSet::AddSampleSegment(double inX, double inY, bool inSegment)
{
  iupPlotDataReal *theXData = (iupPlotDataReal*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (theXData->IsString())
    return;

  if (!mSegment)
    InitSegment();

  theXData->AddSample(inX);
  theYData->AddSample(inY);
  mSelection->AddSample(false);
  mSegment->AddSample(inSegment);
  if (mExtra)
    mExtra->AddSample(0);
}

void iupPlotDataSet::InsertSampleSegment(int inSampleIndex, double inX, double inY, bool inSegment)
{
  iupPlotDataReal *theXData = (iupPlotDataReal*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (theXData->IsString())
    return;

  if (!mSegment)
    InitSegment();

  theXData->InsertSample(inSampleIndex, inX);
  theYData->InsertSample(inSampleIndex, inY);
  mSelection->InsertSample(inSampleIndex, false);
  mSegment->InsertSample(inSampleIndex, inSegment);
  if (mExtra)
    mExtra->InsertSample(inSampleIndex, 0);
}

void iupPlotDataSet::AddSample(const char* inX, double inY)
{
  iupPlotDataString *theXData = (iupPlotDataString*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (!theXData->IsString())
    return;

  theXData->AddSample(inX);
  theYData->AddSample(inY);
  mSelection->AddSample(false);
  if (mSegment)
    mSegment->AddSample(false);
  if (mExtra)
    mExtra->AddSample(0);
}

void iupPlotDataSet::InsertSample(int inSampleIndex, const char* inX, double inY)
{
  iupPlotDataString *theXData = (iupPlotDataString*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (!theXData->IsString())
    return;

  theXData->InsertSample(inSampleIndex, inX);
  theYData->InsertSample(inSampleIndex, inY);
  mSelection->InsertSample(inSampleIndex, false);
  if (mSegment)
    mSegment->InsertSample(inSampleIndex, false);
  if (mExtra)
    mExtra->InsertSample(inSampleIndex, 0);
}

void iupPlotDataSet::RemoveSample(int inSampleIndex)
{
  mDataX->RemoveSample(inSampleIndex);
  mDataY->RemoveSample(inSampleIndex);
  mSelection->RemoveSample(inSampleIndex);
  if (mSegment)
    mSegment->RemoveSample(inSampleIndex);
  if (mExtra)
    mExtra->RemoveSample(inSampleIndex);
}

void iupPlotDataSet::GetSample(int inSampleIndex, double *inX, double *inY)
{
  iupPlotDataReal *theXData = (iupPlotDataReal*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (theXData->IsString())
    return;

  int theCount = theXData->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return;

  if (inX) *inX = theXData->GetSample(inSampleIndex);
  if (inY) *inY = theYData->GetSample(inSampleIndex);
}

void iupPlotDataSet::GetSample(int inSampleIndex, const char* *inX, double *inY)
{
  iupPlotDataString *theXData = (iupPlotDataString*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (!theXData->IsString())
    return;

  int theCount = theXData->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return;

  if (inX) *inX = theXData->GetSampleString(inSampleIndex);
  if (inY) *inY = theYData->GetSample(inSampleIndex);
}

bool iupPlotDataSet::GetSampleSelection(int inSampleIndex)
{
  int theCount = mDataX->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return false;

  return mSelection->GetSampleBool(inSampleIndex);
}

double iupPlotDataSet::GetSampleExtra(int inSampleIndex)
{
  int theCount = mDataX->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount || !mExtra)
    return 0;

  return mExtra->GetSample(inSampleIndex);
}

void iupPlotDataSet::SetSample(int inSampleIndex, double inX, double inY)
{
  iupPlotDataReal *theXData = (iupPlotDataReal*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (theXData->IsString())
    return;

  int theCount = theXData->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return;

  theXData->SetSample(inSampleIndex, inX);
  theYData->SetSample(inSampleIndex, inY);
}

void iupPlotDataSet::SetSample(int inSampleIndex, const char* inX, double inY)
{
  iupPlotDataString *theXData = (iupPlotDataString*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (!theXData->IsString())
    return;

  int theCount = theXData->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return;

  theXData->SetSampleString(inSampleIndex, inX);
  theYData->SetSample(inSampleIndex, inY);
}

void iupPlotDataSet::SetSampleSelection(int inSampleIndex, bool inSelected)
{
  int theCount = mDataX->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return;

  mSelection->SetSampleBool(inSampleIndex, inSelected);
}

void iupPlotDataSet::SetSampleExtra(int inSampleIndex, double inExtra)
{
  int theCount = mDataX->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return;

  if (!mExtra)
    InitExtra();

  mExtra->SetSample(inSampleIndex, inExtra);
}


/************************************************************************************/


#define HIGHLIGHT_ALPHA 64
#define HIGHLIGHT_OFFSET 12
#define SELECT_ALPHA 128
#define SELECT_OFFSET 8


static void iPlotDrawHighlightedBar(cdCanvas *canvas, double x, double y, double barWidth, double barHeight)
{
  int foreground = cdCanvasForeground(canvas, CD_QUERY);
  long color = cdEncodeAlpha(foreground, HIGHLIGHT_ALPHA);
  int width = cdCanvasLineWidth(canvas, CD_QUERY);
  int style = cdCanvasLineStyle(canvas, CD_QUERY);

  cdCanvasLineStyle(canvas, CD_CONTINUOUS);
  cdCanvasLineWidth(canvas, width + HIGHLIGHT_OFFSET);
  cdCanvasSetForeground(canvas, color);

  iupPlotDrawRect(canvas, x, y, barWidth, barHeight);

  cdCanvasLineStyle(canvas, style);
  cdCanvasLineWidth(canvas, width);
  cdCanvasSetForeground(canvas, foreground);
}

static void iPlotDrawHighlightedStem(cdCanvas *canvas, double x1, double y1, double x2, double y2)
{
  int foreground = cdCanvasForeground(canvas, CD_QUERY);
  long color = cdEncodeAlpha(foreground, HIGHLIGHT_ALPHA);
  int width = cdCanvasLineWidth(canvas, CD_QUERY);
  int style = cdCanvasLineStyle(canvas, CD_QUERY);
  int size = cdCanvasMarkSize(canvas, CD_QUERY);
  int type = cdCanvasMarkType(canvas, CD_QUERY);

  cdCanvasLineStyle(canvas, CD_CONTINUOUS);
  cdCanvasLineWidth(canvas, width + HIGHLIGHT_OFFSET);
  cdCanvasMarkSize(canvas, size + HIGHLIGHT_OFFSET);
  cdCanvasMarkType(canvas, CD_CIRCLE);
  cdCanvasSetForeground(canvas, color);

  cdfCanvasLine(canvas, x1, y1, x2, y2);
  cdfCanvasMark(canvas, x2, y2);

  cdCanvasSetForeground(canvas, foreground);
  cdCanvasLineStyle(canvas, style);
  cdCanvasLineWidth(canvas, width);
  cdCanvasMarkSize(canvas, size);
  cdCanvasMarkType(canvas, type);
}

static void iPlotDrawHighlightedMark(cdCanvas *canvas, double x, double y)
{
  int foreground = cdCanvasForeground(canvas, CD_QUERY);
  long color = cdEncodeAlpha(foreground, HIGHLIGHT_ALPHA);
  int size = cdCanvasMarkSize(canvas, CD_QUERY);
  int type = cdCanvasMarkType(canvas, CD_QUERY);

  cdCanvasMarkSize(canvas, size + HIGHLIGHT_OFFSET);
  cdCanvasMarkType(canvas, CD_CIRCLE);
  cdCanvasSetForeground(canvas, color);

  cdfCanvasMark(canvas, x, y);

  cdCanvasSetForeground(canvas, foreground);
  cdCanvasMarkSize(canvas, size);
  cdCanvasMarkType(canvas, type);
}

static void iPlotDrawHighlightedArc(cdCanvas *canvas, double xc, double yc, double w, double h, double startAngle, double endAngle)
{
  int foreground = cdCanvasForeground(canvas, CD_QUERY);
  long color = cdEncodeAlpha(foreground, HIGHLIGHT_ALPHA);
  int width = cdCanvasLineWidth(canvas, CD_QUERY);
  int style = cdCanvasLineStyle(canvas, CD_QUERY);

  cdCanvasLineStyle(canvas, CD_CONTINUOUS);
  cdCanvasLineWidth(canvas, width + HIGHLIGHT_OFFSET);
  cdCanvasSetForeground(canvas, color);

  cdfCanvasArc(canvas, xc, yc, w, h, startAngle, endAngle);

  cdCanvasLineStyle(canvas, style);
  cdCanvasLineWidth(canvas, width);
  cdCanvasSetForeground(canvas, foreground);
}

static void iPlotDrawHighlightedCurve(cdCanvas *canvas, int inCount, const iupPlotData* inDataX, const iupPlotData* inDataY, const iupPlotDataBool* inSegment, 
                                      const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, bool inConnectPreviousX, bool inSelected = false)
{
  int foreground = cdCanvasForeground(canvas, CD_QUERY);
  long color = cdEncodeAlpha(foreground, inSelected? SELECT_ALPHA: HIGHLIGHT_ALPHA);
  int width = cdCanvasLineWidth(canvas, CD_QUERY);
  int style = cdCanvasLineStyle(canvas, CD_QUERY);
  double thePreviousScreenX = 0.;

  cdCanvasLineStyle(canvas, CD_CONTINUOUS);
  cdCanvasLineWidth(canvas, width + (inSelected ? SELECT_OFFSET: HIGHLIGHT_OFFSET));
  cdCanvasSetForeground(canvas, color);

  cdCanvasBegin(canvas, CD_OPEN_LINES);

  for (int i = 0; i < inCount; i++)
  {
    double theX = inDataX->GetSample(i);
    double theY = inDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);

    if (i > 0 && inSegment && inSegment->GetSampleBool(i))
    {
      cdCanvasEnd(canvas);
      cdCanvasBegin(canvas, CD_OPEN_LINES);
    }

    if (inConnectPreviousX && i > 0)
      cdfCanvasVertex(canvas, thePreviousScreenX, theScreenY);

    cdfCanvasVertex(canvas, theScreenX, theScreenY);

    thePreviousScreenX = theScreenX;
  }

  cdCanvasEnd(canvas);

  cdCanvasSetForeground(canvas, foreground);
  cdCanvasLineStyle(canvas, style);
  cdCanvasLineWidth(canvas, width);
}

void iupPlotDataSet::DrawDataLine(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, cdCanvas* canvas, const iupPlotSampleNotify* inNotify, bool inShowMark, bool inErrorBar) const
{
  int theCount = mDataX->GetCount();
  cdCanvasBegin(canvas, CD_OPEN_LINES);

  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    if (inShowMark)
    {
      if (mExtra)
      {
        if (inErrorBar)
          DrawErrorBar(inTrafoY, canvas, i, theY, theScreenX);
        else
          SetSampleExtraMarkSize(inTrafoY, canvas, i);
      }

      // No problem that will be drawn before the polygon, they both should have the same color
      cdfCanvasMark(canvas, theScreenX, theScreenY);
    }

    if (i == mHighlightedSample)
      iPlotDrawHighlightedMark(canvas, theScreenX, theScreenY);

    if (i > 0 && mSegment && mSegment->GetSampleBool(i))
    {
      cdCanvasEnd(canvas);
      cdCanvasBegin(canvas, CD_OPEN_LINES);
    }

    cdfCanvasVertex(canvas, theScreenX, theScreenY);
  }

  cdCanvasEnd(canvas);

  if (mHighlightedCurve)
    iPlotDrawHighlightedCurve(canvas, theCount, mDataX, mDataY, mSegment, inTrafoX, inTrafoY, false);
  else if (mSelectedCurve)
    iPlotDrawHighlightedCurve(canvas, theCount, mDataX, mDataY, mSegment, inTrafoX, inTrafoY, false, true);
}

void iupPlotDataSet::DrawErrorBar(const iupPlotTrafo *inTrafoY, cdCanvas* canvas, int index, double theY, double theScreenX) const
{
  double theError = mExtra->GetSample(index);
  double theScreenErrorY1 = inTrafoY->Transform(theY - theError);
  double theScreenErrorY2 = inTrafoY->Transform(theY + theError);

  double theBarWidth = (double)mMarkSize;  /* fixed size in screen coordinates */

  cdfCanvasLine(canvas, theScreenX, theScreenErrorY1, theScreenX, theScreenErrorY2);
  cdfCanvasLine(canvas, theScreenX - theBarWidth, theScreenErrorY1, theScreenX + theBarWidth, theScreenErrorY1);
  cdfCanvasLine(canvas, theScreenX - theBarWidth, theScreenErrorY2, theScreenX + theBarWidth, theScreenErrorY2);
}

void iupPlotDataSet::SetSampleExtraMarkSize(const iupPlotTrafo *inTrafoY, cdCanvas* canvas, int inSampleIndex) const
{
  double theMarkSize = mExtra->GetSample(inSampleIndex);
  int theScreenSize = 1;
  if (theMarkSize != 0)
    theScreenSize = iupPlotRound(inTrafoY->Transform(theMarkSize));
  if (theScreenSize < 1) theScreenSize = 1;

  cdCanvasMarkSize(canvas, theScreenSize);
}

void iupPlotDataSet::DrawDataMark(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, cdCanvas* canvas, const iupPlotSampleNotify* inNotify) const
{
  int theCount = mDataX->GetCount();
  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    if (mExtra)
      SetSampleExtraMarkSize(inTrafoY, canvas, i);

    cdfCanvasMark(canvas, theScreenX, theScreenY);

    if (i == mHighlightedSample)
      iPlotDrawHighlightedMark(canvas, theScreenX, theScreenY);
  }
}

void iupPlotDataSet::DrawDataStem(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, cdCanvas* canvas, const iupPlotSampleNotify* inNotify, bool inShowMark) const
{
  double theScreenY0 = inTrafoY->Transform(0);

  int theCount = mDataX->GetCount();
  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    if (inShowMark)
    {
      if (mExtra)
        SetSampleExtraMarkSize(inTrafoY, canvas, i);

      cdfCanvasMark(canvas, theScreenX, theScreenY);
    }

    cdfCanvasLine(canvas, theScreenX, theScreenY0, theScreenX, theScreenY);

    if (i == mHighlightedSample)
      iPlotDrawHighlightedStem(canvas, theScreenX, theScreenY0, theScreenX, theScreenY);
  }
}

void iupPlotDataSet::DrawDataArea(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, cdCanvas* canvas, const iupPlotSampleNotify* inNotify) const
{
  int theCount = mDataX->GetCount();
  cdCanvasBegin(canvas, CD_FILL);

  double theScreenY0 = inTrafoY->Transform(0);
  double theLastScreenX = 0;

  if (mAreaTransparency != 255)
    cdCanvasSetForeground(canvas, cdEncodeAlpha(mColor, mAreaTransparency));

  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    if (i == 0)
      cdfCanvasVertex(canvas, theScreenX, theScreenY0);

    if (i > 0 && mSegment && mSegment->GetSampleBool(i))
    {
      cdfCanvasVertex(canvas, theLastScreenX, theScreenY0);
      cdfCanvasVertex(canvas, theScreenX, theScreenY0);
    }

    cdfCanvasVertex(canvas, theScreenX, theScreenY);

    if (i == mHighlightedSample)
      iPlotDrawHighlightedMark(canvas, theScreenX, theScreenY);

    if (i == theCount - 1)
      cdfCanvasVertex(canvas, theScreenX, theScreenY0);

    theLastScreenX = theScreenX;
  }

  cdCanvasEnd(canvas);

  if (mAreaTransparency != 255)
  {
    cdCanvasSetForeground(canvas, mColor);

    cdCanvasBegin(canvas, CD_OPEN_LINES);

    for (int i = 0; i < theCount; i++)
    {
      double theX = mDataX->GetSample(i);
      double theY = mDataY->GetSample(i);
      double theScreenX = inTrafoX->Transform(theX);
      double theScreenY = inTrafoY->Transform(theY);

      if (i > 0 && mSegment && mSegment->GetSampleBool(i))
      {
        cdCanvasEnd(canvas);
        cdCanvasBegin(canvas, CD_OPEN_LINES);
      }

      cdfCanvasVertex(canvas, theScreenX, theScreenY);
    }

    cdCanvasEnd(canvas);
  }

  if (mHighlightedCurve)
    iPlotDrawHighlightedCurve(canvas, theCount, mDataX, mDataY, mSegment, inTrafoX, inTrafoY, false);
  else if (mSelectedCurve)
    iPlotDrawHighlightedCurve(canvas, theCount, mDataX, mDataY, mSegment, inTrafoX, inTrafoY, false, true);
}

void iupPlotDataSet::DrawDataBar(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, cdCanvas* canvas, const iupPlotSampleNotify* inNotify) const
{
  int theCount = mDataX->GetCount();
  double theScreenY0 = inTrafoY->Transform(0);

  double theMinX = mDataX->GetSample(0);
  double theScreenMinX = inTrafoX->Transform(theMinX);
  double theMaxX = mDataX->GetSample(theCount - 1);
  double theScreenMaxX = inTrafoX->Transform(theMaxX);

  double theBarWidth = (theScreenMaxX - theScreenMinX) / (theCount - 1);
  theBarWidth *= 1 - (double)mBarSpacingPercent / 100.0;

  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);

    double theBarX = theScreenX - theBarWidth / 2;
    double theBarHeight = theScreenY - theScreenY0;

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    if (theBarHeight == 0)
      continue;

    if (mBarMulticolor)
      cdCanvasSetForeground(canvas, iupPlotDrawGetSampleColorTable(inNotify->ih, i));

    iupPlotDrawBox(canvas, theBarX, theScreenY0, theBarWidth, theBarHeight);

    if (mBarShowOutline)
    {
      cdCanvasSetForeground(canvas, mBarOutlineColor);
      iupPlotDrawRect(canvas, theBarX, theScreenY0, theBarWidth, theBarHeight);
    }

    if (i == mHighlightedSample)
      iPlotDrawHighlightedBar(canvas, theBarX, theScreenY0, theBarWidth, theBarHeight);

    if (mBarShowOutline && !mBarMulticolor)
      cdCanvasSetForeground(canvas, mColor); // restore curve color
  }
}

void iupPlotDataSet::DrawDataHorizontalBar(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, cdCanvas* canvas, const iupPlotSampleNotify* inNotify) const
{
  int theCount = mDataX->GetCount();
  double theScreenX0 = inTrafoX->Transform(0);


  double theMinY = mDataY->GetSample(0);
  double theScreenMinY = inTrafoY->Transform(theMinY);
  double theMaxY = mDataY->GetSample(theCount - 1);
  double theScreenMaxY = inTrafoY->Transform(theMaxY);

  double theBarHeight = (theScreenMaxY - theScreenMinY) / (theCount - 1);
  theBarHeight *= 1 - (double)mBarSpacingPercent / 100.0;

  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);

    double theBarY = theScreenY - theBarHeight / 2;
    double theBarWidth = theScreenX - theScreenX0;

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    if (theBarWidth == 0)
      continue;

    if (mBarMulticolor)
      cdCanvasSetForeground(canvas, iupPlotDrawGetSampleColorTable(inNotify->ih, i));

    iupPlotDrawBox(canvas, theScreenX0, theBarY, theBarWidth, theBarHeight);

    if (mBarShowOutline)
    {
      cdCanvasSetForeground(canvas, mBarOutlineColor);
      iupPlotDrawRect(canvas, theScreenX0, theBarY, theBarWidth, theBarHeight);
    }

    if (i == mHighlightedSample)
      iPlotDrawHighlightedBar(canvas, theScreenX0, theBarY, theBarWidth, theBarHeight);

    if (mBarShowOutline && !mBarMulticolor)
      cdCanvasSetForeground(canvas, mColor); // restore curve color
  }
}

void iupPlotDataSet::DrawDataMultiBar(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, cdCanvas* canvas, const iupPlotSampleNotify* inNotify) const
{
  int theCount = mDataX->GetCount();
  double theScreenY0 = inTrafoY->Transform(0);

  double theMinX = mDataX->GetSample(0);
  double theScreenMinX = inTrafoX->Transform(theMinX);
  double theMaxX = mDataX->GetSample(theCount - 1);
  double theScreenMaxX = inTrafoX->Transform(theMaxX);

  double theTotalBarWidth = (theScreenMaxX - theScreenMinX) / (theCount - 1);
  theTotalBarWidth *= 1 - (double)mBarSpacingPercent / 100.0;
  double theBarWidth = theTotalBarWidth / mMultibarCount;

  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);

    double theBarX = (theScreenX - theTotalBarWidth / 2) + (mMultibarIndex*theBarWidth);
    double theBarHeight = theScreenY - theScreenY0;

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    if (theBarHeight == 0)
      continue;

    iupPlotDrawBox(canvas, theBarX, theScreenY0, theBarWidth, theBarHeight);

    if (mBarShowOutline)
    {
      cdCanvasSetForeground(canvas, mBarOutlineColor);
      iupPlotDrawRect(canvas, theBarX, theScreenY0, theBarWidth, theBarHeight);
    }

    if (i == mHighlightedSample)
      iPlotDrawHighlightedBar(canvas, theBarX, theScreenY0, theBarWidth, theBarHeight);

    if (mBarShowOutline)
      cdCanvasSetForeground(canvas, mColor); // restore curve color
  }
}

void iupPlotDataSet::DrawDataStep(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, cdCanvas* canvas, const iupPlotSampleNotify* inNotify) const
{
  int theCount = mDataX->GetCount();
  cdCanvasBegin(canvas, CD_OPEN_LINES);
  double theLastScreenX = 0.;

  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    if (i > 0 && mSegment && mSegment->GetSampleBool(i))
    {
      cdCanvasEnd(canvas);
      cdCanvasBegin(canvas, CD_OPEN_LINES);
    }

    if (i > 0)
      cdfCanvasVertex(canvas, theLastScreenX, theScreenY);

    cdfCanvasVertex(canvas, theScreenX, theScreenY);

    if (i == mHighlightedSample)
      iPlotDrawHighlightedMark(canvas, theScreenX, theScreenY);

    theLastScreenX = theScreenX;
  }

  cdCanvasEnd(canvas);


  if (mHighlightedCurve)
    iPlotDrawHighlightedCurve(canvas, theCount, mDataX, mDataY, mSegment, inTrafoX, inTrafoY, true);
  else if (mSelectedCurve)
    iPlotDrawHighlightedCurve(canvas, theCount, mDataX, mDataY, mSegment, inTrafoX, inTrafoY, true, true);
}

static int iPlotGetPieTextAligment(double bisectrix, double inPieSliceLabelPos)
{
  if (inPieSliceLabelPos < 0)
    bisectrix += 180;

  bisectrix = fmod(bisectrix, 360);

  if (bisectrix < 22.5)
    return CD_EAST;
  else if (bisectrix < 67.5)
    return  CD_NORTH_EAST;
  else if (bisectrix < 112.5)
    return CD_NORTH;
  else if (bisectrix < 157.5)
    return CD_NORTH_WEST;
  else if (bisectrix < 202.5)
    return CD_WEST;
  else if (bisectrix < 247.5)
    return CD_SOUTH_WEST;
  else if (bisectrix < 292.5)
    return CD_SOUTH;
  else if (bisectrix < 337.5)
    return CD_SOUTH_EAST;
  else
    return CD_EAST;
}

void iupPlotDataSet::DrawDataPie(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, cdCanvas* canvas, const iupPlotSampleNotify* inNotify, const iupPlotAxis& inAxisY, long inBackColor) const
{
  int theXCount = mDataX->GetCount();
  int theYCount = mDataY->GetCount();

  if ((theXCount == 0) || (theYCount == 0))
    return;

  if (theXCount != theYCount)
    return;

  double xc, yc, w, h;

  int theCount = mDataX->GetCount();
  double sum = 0;

  for (int i = 0; i < theCount; i++)
  {
    double theY = mDataY->GetSample(i);

    if (theY <= 0)
      continue;

    sum += theY;
  }

  xc = 0;
  yc = 0;
  xc = inTrafoX->Transform(xc);
  yc = inTrafoY->Transform(yc);

  w = 2.0 * mPieRadius;
  h = 2.0 * mPieRadius;
  w *= ((iupPlotTrafoLinear *)inTrafoX)->mSlope;
  h *= ((iupPlotTrafoLinear *)inTrafoY)->mSlope;

  double w1 = 2.0 * (mPieRadius*1.01);
  double h1 = 2.0 * (mPieRadius*1.01);
  w1 *= ((iupPlotTrafoLinear *)inTrafoX)->mSlope;
  h1 *= ((iupPlotTrafoLinear *)inTrafoY)->mSlope;

  double startAngle = mPieStartAngle;

  if (mPieContour)
    iupPlotDrawSetLineStyle(canvas, mLineStyle, mLineWidth);

  if (mPieSliceLabel != IUP_PLOT_NONE)
    inAxisY.SetFont(canvas, inAxisY.mFontStyle, inAxisY.mFontSize);

  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);

    if (theY <= 0)
      continue;

    double angle = (theY*360.) / sum;

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    cdCanvasSetForeground(canvas, iupPlotDrawGetSampleColorTable(inNotify->ih, i));

    cdfCanvasSector(canvas, xc, yc, w, h, startAngle, startAngle + angle);

    if (mPieContour)
    {
      int foreground = cdCanvasForeground(canvas, mColor);

      cdCanvasInteriorStyle(canvas, CD_HOLLOW);
      cdfCanvasSector(canvas, xc, yc, w, h, startAngle, startAngle + angle);
      cdCanvasInteriorStyle(canvas, CD_SOLID);

      cdCanvasForeground(canvas, foreground);
    }

    if (i == mHighlightedSample)
      iPlotDrawHighlightedArc(canvas, xc, yc, w1, h1, startAngle, startAngle + angle);

    if (mPieSliceLabel != IUP_PLOT_NONE)
    {
      double bisectrix = (startAngle + startAngle + angle) / 2;

      int text_alignment = iPlotGetPieTextAligment(bisectrix, mPieSliceLabelPos);

      double px = xc + (((w / 2.)*fabs(mPieSliceLabelPos)) * cos(bisectrix * CD_DEG2RAD));
      double py = yc + (((h / 2.)*fabs(mPieSliceLabelPos)) * sin(bisectrix * CD_DEG2RAD));

      cdCanvasSetForeground(canvas, inAxisY.mColor);

      char theBuf[128];
      switch (mPieSliceLabel)
      {
      case IUP_PLOT_X:
        if (mDataX->IsString())
          iupPlotDrawText(canvas, px, py, text_alignment, ((iupPlotDataString *)mDataX)->GetSampleString(i));
        else
        {
          sprintf(theBuf, "%d", i);
          iupPlotDrawText(canvas, px, py, text_alignment, theBuf);
        }
        break;
      case IUP_PLOT_Y:
        iupStrPrintfDoubleLocale(theBuf, inAxisY.mTick.mFormatString, theY, IupGetGlobal("DEFAULTDECIMALSYMBOL"));
        iupPlotDrawText(canvas, px, py, text_alignment, theBuf);
        break;
      case IUP_PLOT_PERCENT:
      {
        double percent = (theY*100.) / sum;
        iupStrPrintfDoubleLocale(theBuf, inAxisY.mTick.mFormatString, percent, IupGetGlobal("DEFAULTDECIMALSYMBOL"));
        strcat(theBuf, " %");
        iupPlotDrawText(canvas, px, py, text_alignment, theBuf);
        break;
      }
      default:  /* IUP_PLOT_NONE */
        break;
      }
    }

    startAngle += angle;
  }

  if (mPieHole > 0)
  {
    double hw = mPieHole * 2.0 * mPieRadius;
    double hh = mPieHole * 2.0 * mPieRadius;
    hw *= ((iupPlotTrafoLinear *)inTrafoX)->mSlope;
    hh *= ((iupPlotTrafoLinear *)inTrafoY)->mSlope;

    cdCanvasSetForeground(canvas, inBackColor);

    cdfCanvasSector(canvas, xc, yc, hw, hh, 0., 360.);

    if (mPieContour)
    {
      cdCanvasSetForeground(canvas, mColor);

      cdCanvasInteriorStyle(canvas, CD_HOLLOW);
      cdfCanvasSector(canvas, xc, yc, hw, hh, 0., 360.);
      cdCanvasInteriorStyle(canvas, CD_SOLID);
    }
  }
}

void iupPlotDataSet::DrawSelection(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, cdCanvas* canvas, const iupPlotSampleNotify* inNotify) const
{
  int theCount = mDataX->GetCount();

  cdCanvasMarkSize(canvas, 7);

  for (int i = 0; i < theCount; i++)
  {
    if (mSelection->GetSampleBool(i))
    {
      double theX = mDataX->GetSample(i);
      double theY = mDataY->GetSample(i);
      double theScreenX = inTrafoX->Transform(theX);
      double theScreenY = inTrafoY->Transform(theY);

      if (inNotify->cb)
      {
        int ret = inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));
        if (ret == IUP_IGNORE)
          continue;
      }

      cdCanvasMarkType(canvas, CD_BOX);
      cdCanvasSetForeground(canvas, cdEncodeAlpha(CD_GRAY, 128));
      cdfCanvasMark(canvas, theScreenX, theScreenY);

      cdCanvasMarkType(canvas, CD_HOLLOW_BOX);
      cdCanvasSetForeground(canvas, cdEncodeAlpha(CD_BLACK, 128));
      cdfCanvasMark(canvas, theScreenX, theScreenY);
    }
  }
}

void iupPlotDataSet::DrawData(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, cdCanvas* canvas, const iupPlotSampleNotify* inNotify) const
{
  int theXCount = mDataX->GetCount();
  int theYCount = mDataY->GetCount();

  if ((theXCount == 0) || (theYCount == 0))
    return;

  if (theXCount != theYCount)
    return;

  cdCanvasSetForeground(canvas, mColor);
  iupPlotDrawSetLineStyle(canvas, mLineStyle, mLineWidth);
  iPlotSetMark(canvas, mMarkStyle, mMarkSize);

  switch (mMode)
  {
  case IUP_PLOT_LINE:
    DrawDataLine(inTrafoX, inTrafoY, canvas, inNotify, false, false);
    break;
  case IUP_PLOT_MARK:
    DrawDataMark(inTrafoX, inTrafoY, canvas, inNotify);
    break;
  case IUP_PLOT_STEM:
    DrawDataStem(inTrafoX, inTrafoY, canvas, inNotify, false);
    break;
  case IUP_PLOT_MARKSTEM:
    DrawDataStem(inTrafoX, inTrafoY, canvas, inNotify, true);
    break;
  case IUP_PLOT_MARKLINE:
    DrawDataLine(inTrafoX, inTrafoY, canvas, inNotify, true, false);
    break;
  case IUP_PLOT_AREA:
    DrawDataArea(inTrafoX, inTrafoY, canvas, inNotify);
    break;
  case IUP_PLOT_BAR:
    DrawDataBar(inTrafoX, inTrafoY, canvas, inNotify);
    break;
  case IUP_PLOT_PIE: /* handled outside DrawData */
    break;
  case IUP_PLOT_HORIZONTALBAR:
    DrawDataHorizontalBar(inTrafoX, inTrafoY, canvas, inNotify);
    break;
  case IUP_PLOT_MULTIBAR:
    DrawDataMultiBar(inTrafoX, inTrafoY, canvas, inNotify);
    break;
  case IUP_PLOT_ERRORBAR:
    DrawDataLine(inTrafoX, inTrafoY, canvas, inNotify, true, true);
    break;
  case IUP_PLOT_STEP:
    DrawDataStep(inTrafoX, inTrafoY, canvas, inNotify);
    break;
  }

  if (mHasSelected)
    DrawSelection(inTrafoX, inTrafoY, canvas, inNotify);
}


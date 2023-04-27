
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


/************************************************************************************************/


iupPlot::iupPlot(Ihandle* _ih, int inDefaultFontStyle, int inDefaultFontSize)
  :ih(_ih), mCurrentDataSet(-1), mRedraw(true), mDataSetListCount(0), mCrossHairH(false), mCrossHairV(false),
   mGrid(true), mGridMinor(false), mViewportSquare(false), mScaleEqual(false), mHighlightMode(IUP_PLOT_HIGHLIGHT_NONE),
   mDefaultFontSize(inDefaultFontSize), mDefaultFontStyle(inDefaultFontStyle), mScreenTolerance(5),
   mAxisX(inDefaultFontStyle, inDefaultFontSize), mAxisY(inDefaultFontStyle, inDefaultFontSize),
   mCrossHairX(0), mCrossHairY(0), mShowSelectionBand(false), mDataSetListMax(20), mDataSetClipping(IUP_PLOT_CLIPAREA)
{
  mDataSetList = (iupPlotDataSet**)malloc(sizeof(iupPlotDataSet*)* mDataSetListMax); /* use malloc because we will use realloc */
  memset(mDataSetList, 0, sizeof(iupPlotDataSet*)* mDataSetListMax);
}

iupPlot::~iupPlot()
{
  RemoveAllDataSets();
  free(mDataSetList);  /* use free because we used malloc */
}

void iupPlot::SetViewport(int x, int y, int w, int h)
{
  mViewportBack.mX = x;
  mViewportBack.mY = y;
  mViewportBack.mWidth = w;
  mViewportBack.mHeight = h;

  if (mViewportSquare && w != h)
  {
    /* take the smallest length */
    if (w > h)
    {
      mViewport.mX = x + (w - h) / 2;
      mViewport.mY = y;
      mViewport.mWidth = h;
      mViewport.mHeight = h;
    }
    else
    {
      mViewport.mX = x;
      mViewport.mY = y + (h - w) / 2;
      mViewport.mWidth = w;
      mViewport.mHeight = w;
    }
  }
  else
  {
    mViewport.mX = x;
    mViewport.mY = y;
    mViewport.mWidth = w;
    mViewport.mHeight = h;
  }

  mRedraw = true;
}

void iupPlot::SetFont(cdCanvas* canvas, int inFontStyle, int inFontSize) const
{
  if (inFontStyle == -1) inFontStyle = mDefaultFontStyle;
  if (inFontSize == 0) inFontSize = mDefaultFontSize;
  cdCanvasFont(canvas, NULL, inFontStyle, inFontSize);
}

void iupPlot::UpdateMultibarCount()
{
  int i, count = 0, index = 0;

  for (i = 0; i < mDataSetListCount; i++)
  {
    if (mDataSetList[i]->mMode == IUP_PLOT_MULTIBAR)
      count++;
  }

  for (i = 0; i < mDataSetListCount; i++)
  {
    if (mDataSetList[i]->mMode == IUP_PLOT_MULTIBAR)
    {
      mDataSetList[i]->mMultibarCount = count;
      mDataSetList[i]->mMultibarIndex = index;
      index++;
    }
    else
    {
      mDataSetList[i]->mMultibarCount = 0;
      mDataSetList[i]->mMultibarIndex = 0;
    }
  }
}

static long iPlotGetDefaultColor(int index)
{
  switch (index)
  {
  case 0: return cdEncodeColor(255, 0, 0);
  case 1: return cdEncodeColor(0, 255, 0);
  case 2: return cdEncodeColor(0, 0, 255);
  case 3: return cdEncodeColor(0, 255, 255);
  case 4: return cdEncodeColor(255, 0, 255);
  case 5: return cdEncodeColor(255, 255, 0);
  case 6: return cdEncodeColor(128, 0, 0);
  case 7: return cdEncodeColor(0, 128, 0);
  case 8: return cdEncodeColor(0, 0, 128);
  case 9: return cdEncodeColor(0, 128, 128);
  case 10: return cdEncodeColor(128, 0, 128);
  case 11: return cdEncodeColor(128, 128, 0);
  default: return cdEncodeColor(0, 0, 0);  // the last must be always black
  }
}

long iupPlot::GetNextDataSetColor() const
{
  int def_color = 0, i = 0;
  long theColor;

  do
  {
    theColor = iPlotGetDefaultColor(def_color);

    for (i = 0; i < mDataSetListCount; i++)
    {
      // already used, get another
      long theDataSetColor = cdEncodeAlpha(mDataSetList[i]->mColor, 255);
      if (theDataSetColor == theColor)
        break;
    }

    // not found, use it
    if (i == mDataSetListCount)
      break;

    def_color++;
  } while (def_color < 12);

  return theColor;
}

void iupPlot::AddDataSet(iupPlotDataSet* inDataSet)
{
  if (mDataSetListCount >= mDataSetListMax)
  {
    int old_max = mDataSetListMax;
    mDataSetListMax += 20;
    mDataSetList = (iupPlotDataSet**)realloc(mDataSetList, sizeof(iupPlotDataSet*)* mDataSetListMax);
    memset(mDataSetList + old_max, 0, sizeof(iupPlotDataSet*)* (mDataSetListMax - old_max));
  }

  if (mDataSetListCount < mDataSetListMax)
  {
    long theColor = GetNextDataSetColor();

    mCurrentDataSet = mDataSetListCount;
    mDataSetListCount++;

    char theLegend[30];
    sprintf(theLegend, "plot %d", mCurrentDataSet);

    mDataSetList[mCurrentDataSet] = inDataSet;

    inDataSet->SetName(theLegend);
    inDataSet->mColor = theColor;
  }
}

void iupPlot::RemoveDataSet(int inIndex)
{
  if (mCurrentDataSet == mDataSetListCount - 1)
    mCurrentDataSet--;

  delete mDataSetList[inIndex];

  for (int i = inIndex; i < mDataSetListCount; i++)
    mDataSetList[i] = mDataSetList[i + 1];

  mDataSetList[mDataSetListCount - 1] = NULL;

  mDataSetListCount--;
}

int iupPlot::FindDataSet(const char* inName) const
{
  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    if (iupStrEqualNoCase(mDataSetList[ds]->GetName(), inName))
      return ds;
  }
  return -1;
}

void iupPlot::RemoveAllDataSets()
{
  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    delete mDataSetList[ds];
  }
  mDataSetListCount = 0;
}

void iupPlot::ClearHighlight()
{
  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];
    dataset->mHighlightedCurve = false;
    dataset->mHighlightedSample = -1;
  }
}

bool iupPlot::FindDataSetSample(double inScreenX, double inScreenY, int &outIndex, const char* &outName, int &outSampleIndex, double &outX, double &outY, const char* &outStrX) const
{
  if (!mAxisX.mTrafo || !mAxisY.mTrafo)
    return false;

  /* search for datasets in the inverse order they are drawn */
  for (int ds = mDataSetListCount - 1; ds >= 0; ds--)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];

    if (dataset->FindSample(mAxisX.mTrafo, mAxisY.mTrafo, inScreenX, inScreenY, mScreenTolerance, outSampleIndex, outX, outY))
    {
      const iupPlotData *theXData = dataset->GetDataX();
      if (theXData->IsString())
      {
        const iupPlotDataString *theStringXData = (const iupPlotDataString *)(theXData);
        outStrX = theStringXData->GetSampleString(outSampleIndex);
      }
      else
        outStrX = NULL;

      outIndex = ds;
      outName = dataset->GetName();

      return true;
    }
  }
  return false;
}

bool iupPlot::FindDataSetSegment(double inScreenX, double inScreenY, int &outIndex, const char* &outName, int &outSampleIndex1, double &outX1, double &outY1, int &outSampleIndex2, double &outX2, double &outY2) const
{
  if (!mAxisX.mTrafo || !mAxisY.mTrafo)
    return false;

  /* search for datasets in the inverse order they are drawn */
  for (int ds = mDataSetListCount - 1; ds >= 0; ds--)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];

    // only for modes that have lines connecting the samples.
    if (dataset->mMode != IUP_PLOT_LINE &&
        dataset->mMode != IUP_PLOT_MARKLINE &&
        dataset->mMode != IUP_PLOT_AREA &&
        dataset->mMode != IUP_PLOT_ERRORBAR)
        continue;

    if (dataset->FindSegment(mAxisX.mTrafo, mAxisY.mTrafo, inScreenX, inScreenY, mScreenTolerance, outSampleIndex1, outSampleIndex2, outX1, outY1, outX2, outY2))
    {
      outIndex = ds;
      outName = dataset->GetName();
      return true;
    }
  }
  return false;
}

void iupPlot::SelectDataSetSamples(double inMinX, double inMaxX, double inMinY, double inMaxY)
{
  bool theChanged = false;

  iPlotCheckMinMax(inMinX, inMaxX);
  iPlotCheckMinMax(inMinY, inMaxY);

  IFniiddi select_cb = (IFniiddi)IupGetCallback(ih, "SELECT_CB");
  if (select_cb)
  {
    Icallback cb = IupGetCallback(ih, "SELECTBEGIN_CB");
    if (cb && cb(ih) == IUP_IGNORE)
      return;
  }

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];
    iupPlotSampleNotify theNotify = { ih, ds, select_cb };
    if (dataset->SelectSamples(inMinX, inMaxX, inMinY, inMaxY, &theNotify))
      theChanged = true;
  }

  if (select_cb)
  {
    Icallback cb = IupGetCallback(ih, "SELECTEND_CB");
    if (cb)
      return;
  }

  if (theChanged)
    mRedraw = true;
}

void iupPlot::ClearDataSetSelection()
{
  bool theChanged = false;

  IFniiddi select_cb = (IFniiddi)IupGetCallback(ih, "SELECT_CB");
  if (select_cb)
  {
    Icallback cb = IupGetCallback(ih, "SELECTBEGIN_CB");
    if (cb && cb(ih) == IUP_IGNORE)
      return;
  }

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];
    iupPlotSampleNotify theNotify = { ih, ds, select_cb };
    if (dataset->ClearSelection(&theNotify))
      theChanged = true;
  }

  if (select_cb)
  {
    Icallback cb = IupGetCallback(ih, "SELECTEND_CB");
    if (cb)
      return;
  }

  if (theChanged)
    mRedraw = true;
}

void iupPlot::DeleteSelectedDataSetSamples()
{
  bool theChanged = false;

  IFniiddi delete_cb = (IFniiddi)IupGetCallback(ih, "DELETE_CB");
  if (delete_cb)
  {
    Icallback cb = IupGetCallback(ih, "DELETEBEGIN_CB");
    if (cb && cb(ih) == IUP_IGNORE)
      return;
  }

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];
    iupPlotSampleNotify theNotify = { ih, ds, delete_cb };
    if (dataset->DeleteSelectedSamples(&theNotify))
      theChanged = true;
  }

  if (delete_cb)
  {
    Icallback cb = IupGetCallback(ih, "DELETEEND_CB");
    if (cb)
      return;
  }

  if (theChanged)
    mRedraw = true;
}

void iupPlot::ConfigureAxis()
{
  mAxisX.Init();
  mAxisY.Init();

  if (mAxisX.mLogScale)
    mAxisY.mPosition = IUP_PLOT_START;  // change at the other axis
  else
  {
    if (mDataSetListCount > 0)
    {
      const iupPlotData *theXData = mDataSetList[0]->GetDataX();   // The first dataset will define the named tick usage
      if (theXData->IsString())
      {
        const iupPlotDataString *theStringXData = (const iupPlotDataString *)(theXData);
        mAxisX.SetNamedTickIter(theStringXData);
      }
    }
  }

  if (mAxisY.mLogScale)
    mAxisX.mPosition = IUP_PLOT_START;   // change at the other axis
}

iupPlotDataSet* iupPlot::HasPie() const
{
  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];
    if (dataset->mMode == IUP_PLOT_PIE)
      return dataset;
  }
  return NULL;
}

void iupPlot::DataSetClipArea(cdCanvas* canvas, int xmin, int xmax, int ymin, int ymax) const
{
  if (mDataSetClipping == IUP_PLOT_CLIPAREAOFFSET)
  {
    if (!mAxisY.HasZoom())
    {
      int yoff = (ymax - ymin) / 50; // 2%
      if (yoff < 10) yoff = 10;

      ymin -= yoff;
      ymax += yoff;
    }

    if (!mAxisX.HasZoom())
    {
      int xoff = (xmax - xmin) / 50; // 2%
      if (xoff < 10) xoff = 10;

      xmin -= xoff;
      xmax += xoff;
    }
  }

  if (mDataSetClipping != IUP_PLOT_CLIPNONE)
    cdCanvasClipArea(canvas, xmin, xmax, ymin, ymax);
}

bool iupPlot::PrepareRender(cdCanvas* canvas)
{
  cdCanvasNativeFont(canvas, IupGetAttribute(ih, "FONT"));

  ConfigureAxis();

  if (!CalculateAxisRange())
    return false;

  if (!CheckRange(mAxisX))
    return false;

  if (!CheckRange(mAxisY))
    return false;

  CalculateTitlePos();

  // Must be before calculate margins
  CalculateTickSize(canvas, mAxisX.mTick);
  CalculateTickSize(canvas, mAxisY.mTick);

  CalculateMargins(canvas);

  return true;
}

bool iupPlot::Render(cdCanvas* canvas)
{
  if (!mRedraw)
    return true;

  // draw entire plot viewport
  DrawBackground(canvas);

  // Shift the drawing area to the plot viewport
  cdCanvasOrigin(canvas, mViewport.mX, mViewport.mY);

  // There are no additional transformations set in the CD canvas,
  // all transformations are done here.

  cdCanvasClip(canvas, CD_CLIPAREA);

  // Draw axis and grid restricted only by the viewport
  cdCanvasClipArea(canvas, 0, mViewport.mWidth - 1, 0, mViewport.mHeight - 1);

  if (!mDataSetListCount)
    return true;

  cdCanvasNativeFont(canvas, IupGetAttribute(ih, "FONT"));

  iupPlotRect theDataSetArea;  /* Viewport - Margin (size only, no need for viewport offset) */
  theDataSetArea.mX = mBack.mMargin.mLeft + mBack.mHorizPadding;
  theDataSetArea.mY = mBack.mMargin.mBottom + mBack.mVertPadding;
  theDataSetArea.mWidth = mViewport.mWidth - mBack.mMargin.mLeft - mBack.mMargin.mRight - 2 * mBack.mHorizPadding;
  theDataSetArea.mHeight = mViewport.mHeight - mBack.mMargin.mTop - mBack.mMargin.mBottom - 2 * mBack.mVertPadding;

  if (!CalculateTickSpacing(theDataSetArea, canvas))
    return false;

  if (!CalculateXTransformation(theDataSetArea))
    return false;

  if (!CalculateYTransformation(theDataSetArea))
    return false;

  IFnC pre_cb = (IFnC)IupGetCallback(ih, "PREDRAW_CB");
  if (pre_cb)
    pre_cb(ih, canvas);

  if (mBack.GetImage())
    DrawBackgroundImage(canvas);

  if (!mGrid.DrawX(mAxisX.mTickIter, mAxisX.mTrafo, theDataSetArea, canvas))
    return false;

  if (mGrid.mShowX)
    mGridMinor.DrawX(mAxisX.mTickIter, mAxisX.mTrafo, theDataSetArea, canvas);

  if (!mGrid.DrawY(mAxisY.mTickIter, mAxisY.mTrafo, theDataSetArea, canvas))
    return false;

  if (mGrid.mShowY)
    mGridMinor.DrawY(mAxisY.mTickIter, mAxisY.mTrafo, theDataSetArea, canvas);

  if (!mAxisX.DrawX(theDataSetArea, canvas, mAxisY, ih))
    return false;

  if (!mAxisY.DrawY(theDataSetArea, canvas, mAxisX, ih))
    return false;

  if (mBox.mShow)
    mBox.Draw(theDataSetArea, canvas);

  // draw the datasets restricted to the dataset area with options
  DataSetClipArea(canvas, theDataSetArea.mX, theDataSetArea.mX + theDataSetArea.mWidth - 1, theDataSetArea.mY, theDataSetArea.mY + theDataSetArea.mHeight - 1);

  IFniiddi drawsample_cb = (IFniiddi)IupGetCallback(ih, "DRAWSAMPLE_CB");

  iupPlotDataSet* pie_dataset = HasPie();

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];
    iupPlotSampleNotify theNotify = { ih, ds, drawsample_cb };

    if (pie_dataset)
    {
      if (dataset != pie_dataset)
        continue;
      else
        dataset->DrawDataPie(mAxisX.mTrafo, mAxisY.mTrafo, canvas, &theNotify, mAxisY, mBack.mColor);
    }

    dataset->DrawData(mAxisX.mTrafo, mAxisY.mTrafo, canvas, &theNotify);
  }

  // draw the legend, crosshair and selection restricted to the dataset area
  cdCanvasClipArea(canvas, theDataSetArea.mX, theDataSetArea.mX + theDataSetArea.mWidth - 1, theDataSetArea.mY, theDataSetArea.mY + theDataSetArea.mHeight - 1);

  if (mCrossHairH)
    DrawCrossHairH(theDataSetArea, canvas);
  else if (mCrossHairV)
    DrawCrossHairV(theDataSetArea, canvas);

  if (mShowSelectionBand)
  {
    if (mSelectionBand.mX < theDataSetArea.mX)
    {
      mSelectionBand.mWidth = mSelectionBand.mX + mSelectionBand.mWidth - theDataSetArea.mX;
      mSelectionBand.mX = theDataSetArea.mX;
    }
    if (mSelectionBand.mY < theDataSetArea.mY)
    {
      mSelectionBand.mHeight = mSelectionBand.mY + mSelectionBand.mHeight - theDataSetArea.mY;
      mSelectionBand.mY = theDataSetArea.mY;
    }
    if (mSelectionBand.mX + mSelectionBand.mWidth > theDataSetArea.mX + theDataSetArea.mWidth)
      mSelectionBand.mWidth = theDataSetArea.mX + theDataSetArea.mWidth - mSelectionBand.mX;
    if (mSelectionBand.mY + mSelectionBand.mHeight > theDataSetArea.mY + theDataSetArea.mHeight)
      mSelectionBand.mHeight = theDataSetArea.mY + theDataSetArea.mHeight - mSelectionBand.mY;

    mBox.Draw(mSelectionBand, canvas);
  }

  IFnC post_cb = (IFnC)IupGetCallback(ih, "POSTDRAW_CB");
  if (post_cb)
    post_cb(ih, canvas);

  if (pie_dataset)
    DrawSampleColorLegend(pie_dataset, theDataSetArea, canvas, mLegend.mPos);
  else if (!DrawLegend(theDataSetArea, canvas, mLegend.mPos))
    return false;

  // Draw title restricted only by the viewport
  cdCanvasClipArea(canvas, 0, mViewport.mWidth - 1, 0, mViewport.mHeight - 1);

  DrawTitle(canvas);

  if (!IupGetInt(ih, "ACTIVE"))
    DrawInactive(canvas);

  mRedraw = false;
  return true;
}


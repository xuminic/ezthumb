/** \file
 * \brief IupImgLib
 *
 * See Copyright Notice in iup.h
 */

#include <stdlib.h>

#include "iup.h"
#include "iup_image.h"

#include "iup_imglib.h"

/* source code, included only here */
#include "iup_imglib_basewin32x32.h"

void iupImglibBaseLibWin32x32Open(void)
{
  iupImageStockSet("IUP_ActionCancel", load_image_ActionCancel, NULL);
  iupImageStockSet("IUP_ActionOk", load_image_ActionOk, NULL);
  iupImageStockSet("IUP_ArrowDown", load_image_ArrowDown, NULL);
  iupImageStockSet("IUP_ArrowLeft", load_image_ArrowLeft, NULL);
  iupImageStockSet("IUP_ArrowRight", load_image_ArrowRight, NULL);
  iupImageStockSet("IUP_ArrowUp", load_image_ArrowUp, NULL);
  iupImageStockSet("IUP_EditCopy", load_image_EditCopy, NULL);
  iupImageStockSet("IUP_EditCut", load_image_EditCut, NULL);
  iupImageStockSet("IUP_EditErase", load_image_EditErase, NULL);
  iupImageStockSet("IUP_EditFind", load_image_EditFind, NULL);
  iupImageStockSet("IUP_EditPaste", load_image_EditPaste, NULL);
  iupImageStockSet("IUP_EditRedo", load_image_EditRedo, NULL);
  iupImageStockSet("IUP_EditUndo", load_image_EditUndo, NULL);
  iupImageStockSet("IUP_FileClose", load_image_FileClose, NULL);
  iupImageStockSet("IUP_FileNew", load_image_FileNew, NULL);
  iupImageStockSet("IUP_FileOpen", load_image_FileOpen, NULL);
  iupImageStockSet("IUP_FileProperties", load_image_FileProperties, NULL);
  iupImageStockSet("IUP_FileSave", load_image_FileSave, NULL);
  iupImageStockSet("IUP_MediaForward", load_image_MediaForward, NULL);
  iupImageStockSet("IUP_MediaGoToBegin", load_image_MediaGoToBegin, NULL);
  iupImageStockSet("IUP_MediaGoToEnd", load_image_MediaGoToEnd, NULL);
  iupImageStockSet("IUP_MediaPause", load_image_MediaPause, NULL);
  iupImageStockSet("IUP_MediaPlay", load_image_MediaPlay, NULL);
  iupImageStockSet("IUP_MediaRecord", load_image_MediaRecord, NULL);
  iupImageStockSet("IUP_MediaReverse", load_image_MediaReverse, NULL);
  iupImageStockSet("IUP_MediaRewind", load_image_MediaRewind, NULL);
  iupImageStockSet("IUP_MediaStop", load_image_MediaStop, NULL);
  iupImageStockSet("IUP_MessageError", load_image_MessageError, NULL);
  iupImageStockSet("IUP_MessageHelp", load_image_MessageHelp, NULL);
  iupImageStockSet("IUP_MessageInfo", load_image_MessageInfo, NULL);
  iupImageStockSet("IUP_NavigateHome", load_image_NavigateHome, NULL);
  iupImageStockSet("IUP_NavigateRefresh", load_image_NavigateRefresh, NULL);
  iupImageStockSet("IUP_Print", load_image_Print, NULL);
  iupImageStockSet("IUP_PrintPreview", load_image_PrintPreview, NULL);
  iupImageStockSet("IUP_ToolsColor", load_image_ToolsColor, NULL);
  iupImageStockSet("IUP_ToolsSettings", load_image_ToolsSettings, NULL);
  iupImageStockSet("IUP_ToolsSortAscend", load_image_ToolsSortAscend, NULL);
  iupImageStockSet("IUP_ToolsSortDescend", load_image_ToolsSortDescend, NULL);
  iupImageStockSet("IUP_ViewFullScreen", load_image_ViewFullScreen, NULL);
  iupImageStockSet("IUP_ZoomActualSize", load_image_ZoomActualSize, NULL);
  iupImageStockSet("IUP_ZoomIn", load_image_ZoomIn, NULL);
  iupImageStockSet("IUP_ZoomOut", load_image_ZoomOut, NULL);
  iupImageStockSet("IUP_ZoomSelection", load_image_ZoomSelection, NULL);
  
  iupImageStockSet("IUP_Webcam", load_image_Webcam, NULL);

#ifdef IUP_IMGLIB_OLD
  iupImageStockSet("IUP_FileCloseAll", load_image_FileCloseAll, NULL);
  iupImageStockSet("IUP_FileSaveAll", load_image_FileSaveAll, NULL);
  iupImageStockSet("IUP_FileText", load_image_FileText, NULL);
  iupImageStockSet("IUP_FontBold", load_image_FontBold, NULL);
  iupImageStockSet("IUP_FontDialog", load_image_FontDialog, NULL);
  iupImageStockSet("IUP_FontItalic", load_image_FontItalic, NULL);
  iupImageStockSet("IUP_WindowsCascade", load_image_WindowsCascade, NULL);
  iupImageStockSet("IUP_WindowsTile", load_image_WindowsTile, NULL);
  iupImageStockSet("IUP_Zoom", load_image_Zoom, NULL);
#endif
}


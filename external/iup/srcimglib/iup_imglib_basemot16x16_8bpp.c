/** \file
 * \brief IupImgLib
 *
 * See Copyright Notice in iup.h
 */

#include <stdlib.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_image.h"

#include "iup_imglib.h"

/* source code, included only here */
#include "iup_imglib_basemot16x16_8bpp.h"

void iupImglibBaseLibMot16x16Open(void)
{
  iupImageStockSet("IUP_ActionCancel", load_image_ActionCancel8, NULL);
  iupImageStockSet("IUP_ActionOk", load_image_ActionOk8, NULL);
  iupImageStockSet("IUP_ArrowDown", load_image_ArrowDown8, NULL);
  iupImageStockSet("IUP_ArrowLeft", load_image_ArrowLeft8, NULL);
  iupImageStockSet("IUP_ArrowRight", load_image_ArrowRight8, NULL);
  iupImageStockSet("IUP_ArrowUp", load_image_ArrowUp8, NULL);
  iupImageStockSet("IUP_EditCopy", load_image_EditCopy8, NULL);
  iupImageStockSet("IUP_EditCut", load_image_EditCut8, NULL);
  iupImageStockSet("IUP_EditErase", load_image_EditErase8, NULL);
  iupImageStockSet("IUP_EditFind", load_image_EditFind8, NULL);
  iupImageStockSet("IUP_EditPaste", load_image_EditPaste8, NULL);
  iupImageStockSet("IUP_EditRedo", load_image_EditRedo8, NULL);
  iupImageStockSet("IUP_EditUndo", load_image_EditUndo8, NULL);
  iupImageStockSet("IUP_FileClose", load_image_FileClose8, NULL);
  iupImageStockSet("IUP_FileNew", load_image_FileNew8, NULL);
  iupImageStockSet("IUP_FileOpen", load_image_FileOpen8, NULL);
  iupImageStockSet("IUP_FileProperties", load_image_FileProperties8, NULL);
  iupImageStockSet("IUP_FileSave", load_image_FileSave8, NULL);
  iupImageStockSet("IUP_MediaForward", load_image_MediaForward8, NULL);
  iupImageStockSet("IUP_MediaGoToBegin", load_image_MediaGoToBegin8, NULL);
  iupImageStockSet("IUP_MediaGoToEnd", load_image_MediaGoToEnd8, NULL);
  iupImageStockSet("IUP_MediaPause", load_image_MediaPause8, NULL);
  iupImageStockSet("IUP_MediaPlay", load_image_MediaPlay8, NULL);
  iupImageStockSet("IUP_MediaRecord", load_image_MediaRecord8, NULL);
  iupImageStockSet("IUP_MediaReverse", load_image_MediaReverse8, NULL);
  iupImageStockSet("IUP_MediaRewind", load_image_MediaRewind8, NULL);
  iupImageStockSet("IUP_MediaStop", load_image_MediaStop8, NULL);
  iupImageStockSet("IUP_MessageError", load_image_MessageError8, NULL);
  iupImageStockSet("IUP_MessageHelp", load_image_MessageHelp8, NULL);
  iupImageStockSet("IUP_MessageInfo", load_image_MessageInfo8, NULL);
  iupImageStockSet("IUP_NavigateHome", load_image_NavigateHome8, NULL);
  iupImageStockSet("IUP_NavigateRefresh", load_image_NavigateRefresh8, NULL);
  iupImageStockSet("IUP_Print", load_image_Print8, NULL);
  iupImageStockSet("IUP_PrintPreview", load_image_PrintPreview8, NULL);
  iupImageStockSet("IUP_ToolsColor", load_image_ToolsColor8, NULL);
  iupImageStockSet("IUP_ToolsSettings", load_image_ToolsSettings8, NULL);
  iupImageStockSet("IUP_ToolsSortAscend", load_image_ToolsSortAscend8, NULL);
  iupImageStockSet("IUP_ToolsSortDescend", load_image_ToolsSortDescend8, NULL);
  iupImageStockSet("IUP_ViewFullScreen", load_image_ViewFullScreen8, NULL);
  iupImageStockSet("IUP_ZoomActualSize", load_image_ZoomActualSize8, NULL);
  iupImageStockSet("IUP_ZoomIn", load_image_ZoomIn8, NULL);
  iupImageStockSet("IUP_ZoomOut", load_image_ZoomOut8, NULL);
  iupImageStockSet("IUP_ZoomSelection", load_image_ZoomSelection8, NULL);

  iupImageStockSet("IUP_Webcam", load_image_Webcam8, NULL);

#ifdef IUP_IMGLIB_OLD
  iupImageStockSet("IUP_FileCloseAll", load_image_FileCloseAll8, NULL);
  iupImageStockSet("IUP_FileSaveAll", load_image_FileSaveAll8, NULL);
  iupImageStockSet("IUP_FileText", load_image_FileText8, NULL);
  iupImageStockSet("IUP_FontBold", load_image_FontBold8, NULL);
  iupImageStockSet("IUP_FontDialog", load_image_FontDialog8, NULL);
  iupImageStockSet("IUP_FontItalic", load_image_FontItalic8, NULL);
  iupImageStockSet("IUP_WindowsCascade", load_image_WindowsCascade8, NULL);
  iupImageStockSet("IUP_WindowsTile", load_image_WindowsTile8, NULL);
  iupImageStockSet("IUP_Zoom", load_image_Zoom8, NULL);
#endif
}

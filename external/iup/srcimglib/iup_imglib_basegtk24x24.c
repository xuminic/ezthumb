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
#include "iup_imglib_basegtk16x16.h"

void iupImglibBaseLibGtk24x24Open(void)
{
  iupImageStockSet("IUP_ActionCancel", NULL, "gtk-cancel");
  iupImageStockSet("IUP_ActionOk", NULL, "gtk-apply");
  iupImageStockSet("IUP_ArrowDown", NULL, "gtk-go-down");
  iupImageStockSet("IUP_ArrowLeft", NULL, "gtk-go-back-ltr");
  iupImageStockSet("IUP_ArrowRight", NULL, "gtk-go-forward-ltr");
  iupImageStockSet("IUP_ArrowUp", NULL, "gtk-go-up");
  iupImageStockSet("IUP_EditCopy", NULL, "gtk-copy");
  iupImageStockSet("IUP_EditCut", NULL, "gtk-cut");
  iupImageStockSet("IUP_EditErase", NULL, "gtk-delete");
  iupImageStockSet("IUP_EditFind", NULL, "gtk-find");
  iupImageStockSet("IUP_EditPaste", NULL, "gtk-paste");
  iupImageStockSet("IUP_EditRedo", NULL, "gtk-redo-ltr");
  iupImageStockSet("IUP_EditUndo", NULL, "gtk-undo-ltr");
  iupImageStockSet("IUP_FileClose", NULL, "gtk-directory");
  iupImageStockSet("IUP_FileNew", NULL, "gtk-new");
  iupImageStockSet("IUP_FileOpen", NULL, "gtk-open");
  iupImageStockSet("IUP_FileProperties", NULL, "gtk-properties");
  iupImageStockSet("IUP_FileSave", NULL, "gtk-save");
  iupImageStockSet("IUP_MediaForward", NULL, "gtk-media-forward-ltr");
  iupImageStockSet("IUP_MediaGoToEnd", NULL, "gtk-media-next-ltr");
  iupImageStockSet("IUP_MediaGoToBegin", NULL, "gtk-media-previous-ltr");
  iupImageStockSet("IUP_MediaPause", NULL, "gtk-media-pause");
  iupImageStockSet("IUP_MediaPlay", NULL, "gtk-media-play-ltr");
  iupImageStockSet("IUP_MediaRecord", NULL, "gtk-media-record");
  iupImageStockSet("IUP_MediaReverse", NULL, "gtk-media-play-rtl");
  iupImageStockSet("IUP_MediaRewind", NULL, "gtk-media-rewind-ltr");
  iupImageStockSet("IUP_MediaStop", NULL, "gtk-media-stop");
  iupImageStockSet("IUP_MessageError", NULL, "gtk-stop");
  iupImageStockSet("IUP_MessageHelp", NULL, "gtk-help");
  iupImageStockSet("IUP_MessageInfo", NULL, "gtk-info");
  iupImageStockSet("IUP_NavigateHome", NULL, "gtk-home");
  iupImageStockSet("IUP_NavigateRefresh", NULL, "gtk-refresh");
  iupImageStockSet("IUP_Print", NULL, "gtk-print");
  iupImageStockSet("IUP_PrintPreview", NULL, "gtk-print-preview");
  iupImageStockSet("IUP_ToolsColor", NULL, "gtk-select-color");
  iupImageStockSet("IUP_ToolsSettings", NULL, "gtk-preferences");
  iupImageStockSet("IUP_ToolsSortAscend", NULL, "gtk-sort-ascending");
  iupImageStockSet("IUP_ToolsSortDescend", NULL, "gtk-sort-descending");
  iupImageStockSet("IUP_ViewFullScreen", NULL, "gtk-fullscreen");
  iupImageStockSet("IUP_ZoomActualSize", NULL, "gtk-zoom-100");
  iupImageStockSet("IUP_ZoomIn", NULL, "gtk-zoom-in");
  iupImageStockSet("IUP_ZoomOut", NULL, "gtk-zoom-out");
  iupImageStockSet("IUP_ZoomSelection", NULL, "gtk-zoom-fit");
  
  iupImageStockSet("IUP_Webcam", load_image_iupgtk_webcam, NULL);

#ifdef IUP_IMGLIB_OLD
  iupImageStockSet("IUP_FileCloseAll", load_image_iupgtk_close_all, NULL);
  iupImageStockSet("IUP_FileSaveAll", load_image_iupgtk_save_all, NULL);
  iupImageStockSet("IUP_FileText", load_image_iupgtk_text, NULL);
  iupImageStockSet("IUP_FontBold", NULL, "gtk-bold");
  iupImageStockSet("IUP_FontDialog", NULL, "gtk-font");
  iupImageStockSet("IUP_FontItalic", NULL, "gtk-italic");
  iupImageStockSet("IUP_WindowsCascade", load_image_iupgtk_cascade, NULL);
  iupImageStockSet("IUP_WindowsTile", load_image_iupgtk_tile, NULL);
  iupImageStockSet("IUP_Zoom", load_image_iupgtk_zoom, NULL);
#endif
}

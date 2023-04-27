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
#include "iup_imglib_basegtk324x24.h"

void iupImglibBaseLibGtk324x24Open(void)
{
  iupImageStockSet("IUP_ActionCancel", load_image_gtk_cancel, NULL);
  iupImageStockSet("IUP_ActionOk", load_image_gtk_apply, NULL);
  iupImageStockSet("IUP_ArrowDown", load_image_go_down, NULL);
  iupImageStockSet("IUP_ArrowLeft", load_image_go_previous_ltr, NULL);
  iupImageStockSet("IUP_ArrowRight", load_image_go_next_ltr, NULL);
  iupImageStockSet("IUP_ArrowUp", load_image_go_up, NULL);
  iupImageStockSet("IUP_EditCopy", load_image_edit_copy, NULL);
  iupImageStockSet("IUP_EditCut", load_image_edit_cut, NULL);
  iupImageStockSet("IUP_EditErase", load_image_edit_delete, NULL);
  iupImageStockSet("IUP_EditFind", load_image_edit_find, NULL);
  iupImageStockSet("IUP_EditPaste", load_image_edit_paste, NULL);
  iupImageStockSet("IUP_EditRedo", load_image_edit_redo_ltr, NULL);
  iupImageStockSet("IUP_EditUndo", load_image_edit_undo_ltr, NULL);
  iupImageStockSet("IUP_FileClose", load_image_folder, NULL);
  iupImageStockSet("IUP_FileNew", load_image_document_new, NULL);
  iupImageStockSet("IUP_FileOpen", load_image_document_open, NULL);
  iupImageStockSet("IUP_FileProperties", load_image_document_properties, NULL);
  iupImageStockSet("IUP_FileSave", load_image_document_save, NULL);
  iupImageStockSet("IUP_MediaForward", load_image_media_seek_forward_ltr, NULL);
  iupImageStockSet("IUP_MediaGoToEnd", load_image_media_skip_forward_ltr, NULL);
  iupImageStockSet("IUP_MediaGoToBegin", load_image_media_skip_backward_ltr, NULL);
  iupImageStockSet("IUP_MediaPause", load_image_media_playback_pause, NULL);
  iupImageStockSet("IUP_MediaPlay", load_image_media_playback_start_ltr, NULL);
  iupImageStockSet("IUP_MediaRecord", load_image_media_record, NULL);
  iupImageStockSet("IUP_MediaReverse", load_image_media_playback_start_rtl, NULL);
  iupImageStockSet("IUP_MediaRewind", load_image_media_seek_backward_ltr, NULL);
  iupImageStockSet("IUP_MediaStop", load_image_media_playback_stop, NULL);
  iupImageStockSet("IUP_MessageError", load_image_process_stop, NULL);
  iupImageStockSet("IUP_MessageHelp", load_image_help_contents, NULL);
  iupImageStockSet("IUP_MessageInfo", load_image_dialog_information, NULL);
  iupImageStockSet("IUP_NavigateHome", load_image_go_home, NULL);
  iupImageStockSet("IUP_NavigateRefresh", load_image_view_refresh, NULL);
  iupImageStockSet("IUP_Print", load_image_document_print, NULL);
  iupImageStockSet("IUP_PrintPreview", load_image_document_print_preview, NULL);
  iupImageStockSet("IUP_ToolsColor", load_image_gtk_select_color, NULL);
  iupImageStockSet("IUP_ToolsSettings", load_image_gtk_preferences, NULL);
  iupImageStockSet("IUP_ToolsSortAscend", load_image_view_sort_ascending, NULL);
  iupImageStockSet("IUP_ToolsSortDescend", load_image_view_sort_descending, NULL);
  iupImageStockSet("IUP_ViewFullScreen", load_image_view_fullscreen, NULL);
  iupImageStockSet("IUP_ZoomActualSize", load_image_zoom_original, NULL);
  iupImageStockSet("IUP_ZoomIn", load_image_zoom_in, NULL);
  iupImageStockSet("IUP_ZoomOut", load_image_zoom_out, NULL);
  iupImageStockSet("IUP_ZoomSelection", load_image_zoom_fit_best, NULL);

  iupImageStockSet("IUP_Webcam", load_image_webcam, NULL);
}

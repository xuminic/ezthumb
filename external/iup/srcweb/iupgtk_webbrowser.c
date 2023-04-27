/** \file
 * \brief Web Browser Control
 *
 * See Copyright Notice in "iup.h"
 */


#ifdef IUPWEB_USE_DLOPEN
#include "iupwebgtk_dlopen.h"
#else
#include <gtk/gtk.h>
#ifdef USE_WEBKIT2
#include <webkit2/webkit2.h>
#else
#include <webkit/webkit.h>
#endif  
#include <JavaScriptCore/JavaScript.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_webbrowser.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
#include "iup_register.h"

#include "iupgtk_drv.h"


#ifndef WEBKIT_LOAD_FAILED
#define WEBKIT_LOAD_FAILED 4
#endif

#ifdef IUPWEB_USE_DLOPEN
#include <dlfcn.h>

/* In my past experience with GTK, I don't think it is possible to unload the library.
	 I don't know if WebKit is any better, but for now I am assuming it is not.
	 So load it once and and never close for now.
*/
static void* s_webKitLibrary = NULL;

/*
	 Returns IUP_NOERROR on successful dlopen of libwebkitgtk.
	 IUP_OPENED if already opened.
	 IUP_ERROR on failure to load. Sets IupSetGlobal with key _IUP_WEBBROWSER_MISSING_DLL with a name of a missing libwekkitgtk.so.
*/
int IupGtkWebBrowserDLOpen()
{
  size_t i;
  /* TODO: RTLD_LAZY or RTLD_NOW? 
  const mode_flags = RTLD_NOW | RTLD_LOCAL;	*/
  const mode_flags = RTLD_LAZY | RTLD_LOCAL;	
  static const char* listOfWebKitNames[] =
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    "libwebkitgtk-3.0.so",
    "libwebkitgtk-3.0.so.0"
#else
    "libwebkitgtk-1.0.so",
    "libwebkitgtk-1.0.so.0"
#endif
  };

  if(NULL != s_webKitLibrary)
  {
    return IUP_OPENED;
  }

#define WEBKIT_NAMES_ARRAY_LENGTH (sizeof(listOfWebKitNames)/sizeof(*listOfWebKitNames))
  for(i=0; i<WEBKIT_NAMES_ARRAY_LENGTH; i++)
  {
    s_webKitLibrary = dlopen(listOfWebKitNames[i], mode_flags);
    if(NULL != s_webKitLibrary)
    {
      break;
    }
  }

  if(NULL == s_webKitLibrary)
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    IupSetGlobal("_IUP_WEBBROWSER_MISSING_DLL", "libwebkitgtk-3.0.so");
#else
    IupSetGlobal("_IUP_WEBBROWSER_MISSING_DLL", "libwebkitgtk-1.0.so");
#endif
    iupgtkWebBrowser_ClearDLSymbols();
    return IUP_ERROR;
  }
  else
  {
    iupgtkWebBrowser_SetDLSymbols(s_webKitLibrary);
    return IUP_NOERROR;
  }
}

/*
// Unused for now.
static void Internal_UnloadLibrary()
{
  if(NULL != s_webKitLibrary)
  {
    dlclose(s_webKitLibrary);
    s_webKitLibrary = NULL;
  }
}
*/
#endif /* IUPWEB_USE_DLOPEN */

struct _IcontrolData 
{
  int sb;    /* scrollbar configuration, valid only after map, use iupBaseGetScrollbar before map */
};

static char* gtkWebBrowserGetItemHistoryAttrib(Ihandle* ih, int id)
{
#ifdef USE_WEBKIT2
  WebKitBackForwardList *back_forward_list = webkit_web_view_get_back_forward_list((WebKitWebView*)ih->handle);
  WebKitBackForwardListItem* item = webkit_back_forward_list_get_nth_item(back_forward_list, id);
#else
  WebKitWebBackForwardList *back_forward_list = webkit_web_view_get_back_forward_list((WebKitWebView*)ih->handle);
  WebKitWebHistoryItem* item = webkit_web_back_forward_list_get_nth_item(back_forward_list, id);
#endif
  if (item)
#ifdef USE_WEBKIT2
    return iupStrReturnStr(webkit_back_forward_list_item_get_uri(item));
#else
    return iupStrReturnStr(webkit_web_history_item_get_uri(item));
#endif
  else
    return NULL;
}

static char* gtkWebBrowserGetForwardCountAttrib(Ihandle* ih)
{
#ifdef USE_WEBKIT2
  WebKitBackForwardList *back_forward_list = webkit_web_view_get_back_forward_list ((WebKitWebView*)ih->handle);
  return iupStrReturnInt(g_list_length(webkit_back_forward_list_get_back_list(back_forward_list)));
#else
  WebKitWebBackForwardList *back_forward_list = webkit_web_view_get_back_forward_list((WebKitWebView*)ih->handle);
  return iupStrReturnInt(webkit_web_back_forward_list_get_forward_length(back_forward_list));
#endif
}

static char* gtkWebBrowserGetBackCountAttrib(Ihandle* ih)
{
#ifdef USE_WEBKIT2
  WebKitBackForwardList *back_forward_list = webkit_web_view_get_back_forward_list((WebKitWebView*)ih->handle);
  return iupStrReturnInt(g_list_length(webkit_back_forward_list_get_forward_list(back_forward_list)));
#else
  WebKitWebBackForwardList *back_forward_list = webkit_web_view_get_back_forward_list((WebKitWebView*)ih->handle);
  return iupStrReturnInt(webkit_web_back_forward_list_get_back_length(back_forward_list));
#endif
}

static int gtkWebBrowserSetHTMLAttrib(Ihandle* ih, const char* value)
{
  if (value)
#ifdef USE_WEBKIT2
    webkit_web_view_load_html((WebKitWebView*)ih->handle, iupgtkStrConvertToSystem(value), NULL);
#else
    webkit_web_view_load_string((WebKitWebView*)ih->handle, iupgtkStrConvertToSystem(value), "text/html", "UTF-8", "");
#endif
  return 0; /* do not store value in hash table */
}

#ifdef USE_WEBKIT2
static void gtkWebBrowserGetResourceData(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  Ihandle* ih = (Ihandle*)user_data;
  WebKitWebResource *resource = (WebKitWebResource*)source_object;
  GError *error = NULL;
  gsize len = 0;

  char* data = (char*)webkit_web_resource_get_data_finish(resource, res, &len, &error);
  if (!data)
  {
    if (error)
    {
      iupAttribSetStrf(ih, "HTML", "ERROR: %s", error->message);
      g_error_free(error);
    }
    else
      iupAttribSet(ih, "HTML", "ERROR: UNKNOWN");
  }
  else
  {
    if (data[len] != 0)
      data[len] = 0;
    iupAttribSetStr(ih, "HTML", data);
  }
}
#endif

static char* gtkWebBrowserGetHTMLAttrib(Ihandle* ih)
{
#ifdef USE_WEBKIT2
  char* value = NULL;
  WebKitWebResource* resource = webkit_web_view_get_main_resource((WebKitWebView*)ih->handle);
  webkit_web_resource_get_data(resource, NULL, gtkWebBrowserGetResourceData, ih);

  int i = 0;
  while (!value && i < 1000)
  {
    IupLoopStep();
    value = iupAttribGet(ih, "HTML");
    i++;
  }

  return NULL;
#else
  WebKitWebFrame* frame = webkit_web_view_get_main_frame((WebKitWebView*)ih->handle);
#if 1
  WebKitWebDataSource* data_source = webkit_web_frame_get_data_source(frame);
  GString* string = webkit_web_data_source_get_data(data_source);
#else  /*  ???????  */
  WebKitWebResource* resource = webkit_web_data_source_get_main_resource(data_source);
  GString* string = webkit_web_resource_get_data(resource);
#endif
  return iupStrReturnStr(string->str);
#endif
}

#ifndef USE_WEBKIT2
static int write_file(const char* filename, const char* str, int count)
{
  FILE* file = fopen(filename, "wb");
  if (!file)
    return 0;

  fwrite(str, 1, count, file);

  fclose(file);
  return 1;
}
#endif

static int gtkWebBrowserSetSaveAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
#ifdef USE_WEBKIT2
    GFile* file = g_file_new_for_path(iupgtkStrConvertToFilename(value));
    if (file)
      webkit_web_view_save_to_file((WebKitWebView*)ih->handle, file, WEBKIT_SAVE_MODE_MHTML, NULL, NULL, NULL);
#else
    WebKitWebFrame* frame = webkit_web_view_get_main_frame((WebKitWebView*)ih->handle);
    WebKitWebDataSource* data_source = webkit_web_frame_get_data_source(frame);
    GString* string = webkit_web_data_source_get_data(data_source);
    write_file(iupgtkStrConvertToFilename(value), string->str, string->len);
#endif
  }
  return 0;
}

static int gtkWebBrowserSetValueAttrib(Ihandle* ih, const char* value);
static int gtkWebBrowserSetEditableAttrib(Ihandle* ih, const char* value);

static int gtkWebBrowserSetOpenAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    char* url = iupStrFileMakeURL(value);
    gtkWebBrowserSetValueAttrib(ih, url);
    gtkWebBrowserSetEditableAttrib(ih, "Yes");
    free(url);
  }
  return 0;
}

static int gtkWebBrowserSetNewAttrib(Ihandle* ih, const char* value)
{
  gtkWebBrowserSetHTMLAttrib(ih, "<HTML><BODY><P></P></BODY></HTML>");
  gtkWebBrowserSetEditableAttrib(ih, "Yes");
  (void)value;
  return 0;
}

static int gtkWebBrowserSetCutAttrib(Ihandle* ih, const char* value)
{
#ifdef USE_WEBKIT2
  webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_CUT);
#else
  webkit_web_view_cut_clipboard((WebKitWebView*)ih->handle);
#endif
  (void)value;
  return 0;
}

static int gtkWebBrowserSetCopyAttrib(Ihandle* ih, const char* value)
{
#ifdef USE_WEBKIT2
  webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_COPY);
#else
  webkit_web_view_copy_clipboard((WebKitWebView*)ih->handle);
#endif
  (void)value;
  return 0;
}

static int gtkWebBrowserSetPasteAttrib(Ihandle* ih, const char* value)
{
#ifdef USE_WEBKIT2
  webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_PASTE);
#else
  webkit_web_view_paste_clipboard((WebKitWebView*)ih->handle);
#endif
  (void)value;
  return 0;
}

static int gtkWebBrowserSetUndoAttrib(Ihandle* ih, const char* value)
{
#ifdef USE_WEBKIT2
  webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_UNDO);
#else
  webkit_web_view_undo((WebKitWebView*)ih->handle);
#endif
  (void)value;
  return 0;
}

static int gtkWebBrowserSetRedoAttrib(Ihandle* ih, const char* value)
{
#ifdef USE_WEBKIT2
  webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_REDO);
#else
  webkit_web_view_redo((WebKitWebView*)ih->handle);
#endif
  (void)value;
  return 0;
}

static char* gtkWebBrowserGetEditableAttrib(Ihandle* ih)
{
#ifdef USE_WEBKIT2
  return iupStrReturnBoolean(webkit_web_view_is_editable((WebKitWebView*)ih->handle));
#else
  return iupStrReturnBoolean(webkit_web_view_can_paste_clipboard((WebKitWebView*)ih->handle));
#endif
}

static int gtkWebBrowserSetSelectAllAttrib(Ihandle* ih, const char* value)
{
#ifdef USE_WEBKIT2
  webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_SELECT_ALL);
#else
 webkit_web_view_select_all((WebKitWebView*)ih->handle);
#endif
  (void)value;
  return 0;
}

static int gtkWebBrowserSetPrintAttrib(Ihandle* ih, const char* value)
{
#ifdef USE_WEBKIT2
  WebKitPrintOperation *print_operation = webkit_print_operation_new((WebKitWebView*)ih->handle);
  if (iupStrBoolean(value))
  {
    Ihandle* dlg = IupGetDialog(ih);
    GtkWindow* parent = NULL;

    if (dlg && dlg->handle)
      parent = (GtkWindow*)dlg->handle;

    webkit_print_operation_run_dialog(print_operation, parent);
  }
  else
    webkit_print_operation_print(print_operation);
#else
  WebKitWebFrame* frame = webkit_web_view_get_main_frame((WebKitWebView*)ih->handle);
  webkit_web_frame_print(frame);
#endif
  (void)value;
  return 0;
}

static int gtkWebBrowserSetZoomAttrib(Ihandle* ih, const char* value)
{
  int zoom;
  if (iupStrToInt(value, &zoom))
    webkit_web_view_set_zoom_level((WebKitWebView*)ih->handle, (float)zoom/100.0f);
  return 0;
}

static char* gtkWebBrowserGetZoomAttrib(Ihandle* ih)
{
  int zoom = (int)(webkit_web_view_get_zoom_level((WebKitWebView*)ih->handle) * 100);
  return iupStrReturnInt(zoom);
}

static int gtkWebBrowserSetEditableAttrib(Ihandle* ih, const char* value)
{
  webkit_web_view_set_editable((WebKitWebView*)ih->handle, iupStrBoolean(value));
  return 0;
}

static void gtkWebBrowserRunJavascript(Ihandle* ih, const char* format, ...)
{
  char js[1024];
  va_list arglist;
  va_start(arglist, format);
  vsnprintf(js, 1024, format, arglist);
  va_end(arglist);

#ifdef USE_WEBKIT2
  webkit_web_view_run_javascript((WebKitWebView*)ih->handle, js, NULL, NULL, NULL);
#else
  webkit_web_view_execute_script((WebKitWebView*)ih->handle, js);
#endif
}

static int gtkWebBrowserExecCommandAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
#ifdef USE_WEBKIT2
    webkit_web_view_execute_editing_command((WebKitWebView*)ih->handle, value);
#else
    gtkWebBrowserRunJavascript(ih, "document.execCommand('%s', false, null)", value);
#endif
  }
  return 0;
}

static int gtkWebBrowserSetInsertImageAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
#ifdef USE_WEBKIT2
    webkit_web_view_execute_editing_command_with_argument((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_INSERT_IMAGE, value);
#else
    gtkWebBrowserRunJavascript(ih, "document.execCommand('insertimage', false, '%s')", value);
#endif
  }
  return 0;
}

static int gtkWebBrowserSetCreateLinkAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
#ifdef USE_WEBKIT2
    webkit_web_view_execute_editing_command_with_argument((WebKitWebView*)ih->handle, WEBKIT_EDITING_COMMAND_CREATE_LINK, value);
#else
    gtkWebBrowserRunJavascript(ih, "document.execCommand('createLink', false, '%s')", value);
#endif
  }
  return 0;
}

static void gtkWebBrowserExecCommandParam(Ihandle* ih, const char* cmd, const char* param)
{
#ifdef USE_WEBKIT2
  webkit_web_view_execute_editing_command_with_argument((WebKitWebView*)ih->handle, cmd, param);
#else
  gtkWebBrowserRunJavascript(ih, "document.execCommand('%s', false, '%s')", cmd, param);
#endif
}

static int gtkWebBrowserSetInsertTextAttrib(Ihandle* ih, const char* value)
{
  if (value)
    gtkWebBrowserExecCommandParam(ih, "insertText", value);
  return 0;
}

static int gtkWebBrowserSetInsertHtmlAttrib(Ihandle* ih, const char* value)
{
  if (value)
    gtkWebBrowserExecCommandParam(ih, "insertHTML", value);
  return 0;
}

static int gtkWebBrowserSetFontNameAttrib(Ihandle* ih, const char* value)
{
  if (value)
    gtkWebBrowserExecCommandParam(ih, "fontName", value);
  return 0;
}

static int gtkWebBrowserSetFontSizeAttrib(Ihandle* ih, const char* value)
{
  int param = 0;
  if (iupStrToInt(value, &param) && param > 0 && param < 8)
    gtkWebBrowserExecCommandParam(ih, "fontSize", value);
  return 0;
}

static int gtkWebBrowserSetFormatBlockAttrib(Ihandle* ih, const char* value)
{
  if (value)
    gtkWebBrowserExecCommandParam(ih, "formatBlock", value);
  return 0;
}

static int gtkWebBrowserSetForeColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
    gtkWebBrowserExecCommandParam(ih, "forecolor", value);
  return 0;
}

static int gtkWebBrowserSetBackColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
    gtkWebBrowserExecCommandParam(ih, "backcolor", value);
  return 0;
}

static int gtkWebBrowserSetInsertImageFileAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    char* url = iupStrFileMakeURL(value);
    gtkWebBrowserSetInsertImageAttrib(ih, url);
    free(url);
  }
  return 0;
}

static char* gtkWebBrowserGetStatusAttrib(Ihandle* ih)
{
#ifdef USE_WEBKIT2
  if (webkit_web_view_is_loading((WebKitWebView*)ih->handle))
    return "LOADING";
  else
    return "COMPLETED";
#else
  WebKitLoadStatus status = webkit_web_view_get_load_status((WebKitWebView*)ih->handle);
  if (status == WEBKIT_LOAD_FAILED)
    return "FAILED";
  else if (status == WEBKIT_LOAD_FINISHED)
    return "COMPLETED";
  else
    return "LOADING";
#endif
}

static int gtkWebBrowserSetReloadAttrib(Ihandle* ih, const char* value)
{
  webkit_web_view_reload((WebKitWebView*)ih->handle);
  (void)value;
  return 0; /* do not store value in hash table */
}

static int gtkWebBrowserSetStopAttrib(Ihandle* ih, const char* value)
{
  webkit_web_view_stop_loading((WebKitWebView*)ih->handle);
  (void)value;
  return 0; /* do not store value in hash table */
}

static int gtkWebBrowserSetBackForwardAttrib(Ihandle* ih, const char* value)
{
  int val;
  if (iupStrToInt(value, &val))
  {
    /* Negative values represent steps backward while positive values represent steps forward. */
#ifdef USE_WEBKIT2
    if (val > 0)
    {
      int i;
      for ( i = 0; i < val && webkit_web_view_can_go_forward((WebKitWebView*)ih->handle); i++)
        webkit_web_view_go_forward((WebKitWebView*)ih->handle);
    }
    else if (val < 0)
    {
      int i;
      for (i = 0; i < abs(val) && webkit_web_view_can_go_back((WebKitWebView*)ih->handle); i++)
        webkit_web_view_go_back((WebKitWebView*)ih->handle);
    }
#else
    webkit_web_view_go_back_or_forward((WebKitWebView*)ih->handle, val);
#endif
  }
  return 0; /* do not store value in hash table */
}

static int gtkWebBrowserSetGoBackAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  webkit_web_view_go_back((WebKitWebView*)ih->handle);
  return 0; /* do not store value in hash table */
}

static int gtkWebBrowserSetGoForwardAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  webkit_web_view_go_forward((WebKitWebView*)ih->handle);
  return 0; /* do not store value in hash table */
}

static char* gtkWebBrowserGetCanGoBackAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(webkit_web_view_can_go_back((WebKitWebView*)ih->handle));
}

static char* gtkWebBrowserGetCanGoForwardAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(webkit_web_view_can_go_forward((WebKitWebView*)ih->handle));
}

static int gtkWebBrowserSetValueAttrib(Ihandle* ih, const char* value)
{
  if (value)
    webkit_web_view_load_uri((WebKitWebView*)ih->handle, value);
  return 0; /* do not store value in hash table */
}

static char* gtkWebBrowserGetValueAttrib(Ihandle* ih)
{
  const gchar* value = webkit_web_view_get_uri((WebKitWebView*)ih->handle);
  return iupStrReturnStr(value);
}

/*********************************************************************************************/

#ifdef USE_WEBKIT2
static void gtkWebBrowserDocumentLoadFinished(WebKitWebView *web_view, WebKitLoadEvent load_event, Ihandle *ih)
{
  IFns cb = (IFns)IupGetCallback(ih, "COMPLETED_CB");

  if (load_event != WEBKIT_LOAD_FINISHED)
    return;

  if (cb)
    cb(ih, (char*)webkit_web_view_get_uri(web_view));
}
#else
static void gtkWebBrowserDocumentLoadFinished(WebKitWebView *web_view, WebKitWebFrame *frame, Ihandle *ih)
{
  IFns cb = (IFns)IupGetCallback(ih, "COMPLETED_CB");
  if (cb)
    cb(ih, (char*)webkit_web_frame_get_uri(frame));
}
#endif

#ifdef USE_WEBKIT2
static gboolean gtkWebBrowserLoadError(WebKitWebView *web_view, WebKitLoadEvent load_event,
                                       gchar *failing_uri, GError *error, Ihandle *ih)
{
  IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
  if (cb)
    cb(ih, failing_uri);

  return FALSE;
}
#else
static gboolean gtkWebBrowserLoadError(WebKitWebView *web_view, WebKitWebFrame *frame,
                                       gchar *uri, gpointer web_error, Ihandle *ih)
{
  IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
  if (cb)
    cb(ih, uri);

  return FALSE;
}
#endif

#ifdef USE_WEBKIT2
static int gtkWebBrowserNavigate(WebKitWebView *web_view, WebKitPolicyDecision *decision,
                                 WebKitPolicyDecisionType decision_type, Ihandle *ih)
{
  if (decision_type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
    return FALSE;

  IFns cb = (IFns)IupGetCallback(ih, "NAVIGATE_CB");
  if (cb)
  {
    if (cb(ih, (char*)webkit_web_view_get_uri(web_view)) == IUP_IGNORE)
      return FALSE;
  }

  return FALSE;
}
#else
static int gtkWebBrowserNavigate(WebKitWebView *web_view, WebKitWebFrame *frame, WebKitNetworkRequest *request,
                                 WebKitWebNavigationAction *navigation_action, WebKitWebPolicyDecision *policy_decision, Ihandle *ih)
{
  /*
  char *strReason = iupStrGetMemory(50);
  WebKitWebNavigationReason reason = webkit_web_navigation_action_get_reason(navigation_action);

  switch(reason)
  {
    case WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED:
      sprintf(strReason, "%s", "LINK_CLICKED");
      break;
    case WEBKIT_WEB_NAVIGATION_REASON_FORM_SUBMITTED:
      sprintf(strReason, "%s", "FORM_SUBMITTED");
      break;
    case WEBKIT_WEB_NAVIGATION_REASON_BACK_FORWARD:
      sprintf(strReason, "%s", "BACK_FORWARD");
      break;
    case WEBKIT_WEB_NAVIGATION_REASON_RELOAD:
      sprintf(strReason, "%s", "RELOAD");
      break;
    case WEBKIT_WEB_NAVIGATION_REASON_FORM_RESUBMITTED:
      sprintf(strReason, "%s", "FORM_RESUBMITTED");
      break;
    case WEBKIT_WEB_NAVIGATION_REASON_OTHER:
      sprintf(strReason, "%s", "OTHER");
      break;
  }
  */

  IFns cb = (IFns)IupGetCallback(ih, "NAVIGATE_CB");
  if (cb)
  {
    if (cb(ih, (char*)webkit_network_request_get_uri(request)) == IUP_IGNORE)
      return FALSE;
  }

  return FALSE;
}
#endif

#ifdef USE_WEBKIT2
static WebKitWebView* gtkWebBrowserNewWindow(WebKitWebView *web_view, WebKitNavigationAction *navigation_action, Ihandle *ih)
{
  IFns cb = (IFns)IupGetCallback(ih, "NEWWINDOW_CB");
  if (cb)
    cb(ih, (char*)webkit_web_view_get_uri(web_view));

  return web_view;
}
#else
static WebKitWebView* gtkWebBrowserNewWindow(WebKitWebView *web_view, WebKitWebFrame *frame, Ihandle *ih)
{
  IFns cb = (IFns)IupGetCallback(ih, "NEWWINDOW_CB");
  if (cb)
    cb(ih, (char*)webkit_web_frame_get_uri(frame));

  return web_view;
}
#endif

/*********************************************************************************************/

#ifndef USE_WEBKIT2
static void gtkWebBrowserDummyLogFunc(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
  /* does nothing */
  (void)log_domain;
  (void)log_level;
  (void)message;
  (void)user_data;
}
#endif

static int gtkWebBrowserMapMethod(Ihandle* ih)
{
#ifndef USE_WEBKIT2
  GtkScrolledWindow* scrolled_window;
#endif

  ih->handle = (GtkWidget*)webkit_web_view_new();
  if (!ih->handle)
    return IUP_ERROR;

#ifndef USE_WEBKIT2
  scrolled_window = (GtkScrolledWindow*)gtk_scrolled_window_new(NULL, NULL);
  if (!scrolled_window)
    return IUP_ERROR;

  {
    /* to avoid the "cannot add non scrollable widget" warning */
#if GTK_CHECK_VERSION(2, 6, 0)
    GLogFunc def_func = g_log_set_default_handler(gtkWebBrowserDummyLogFunc, NULL);
#endif
    gtk_container_add((GtkContainer*)scrolled_window, ih->handle);
#if GTK_CHECK_VERSION(2, 6, 0)
    g_log_set_default_handler(def_func, NULL);
#endif
  }

  /* configure scrollbar */
  if (ih->data->sb)
  {
    GtkPolicyType hscrollbar_policy = GTK_POLICY_NEVER, vscrollbar_policy = GTK_POLICY_NEVER;
    if (ih->data->sb & IUP_SB_HORIZ)
      hscrollbar_policy = GTK_POLICY_AUTOMATIC;
    if (ih->data->sb & IUP_SB_VERT)
      vscrollbar_policy = GTK_POLICY_AUTOMATIC;
    gtk_scrolled_window_set_policy(scrolled_window, hscrollbar_policy, vscrollbar_policy);
  }
  else
    gtk_scrolled_window_set_policy(scrolled_window, GTK_POLICY_NEVER, GTK_POLICY_NEVER);

  gtk_widget_show((GtkWidget*)scrolled_window);

  iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)scrolled_window);
#endif

  /* add to the parent, all GTK controls must call this. */
  iupgtkAddToParent(ih);

  g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-out-event",    G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "show-help",          G_CALLBACK(iupgtkShowHelp),        ih);

  g_signal_connect(G_OBJECT(ih->handle), "create-web-view", G_CALLBACK(gtkWebBrowserNewWindow), ih);
#ifdef USE_WEBKIT2
  g_signal_connect(G_OBJECT(ih->handle), "decide-policy", G_CALLBACK(gtkWebBrowserNavigate), ih);
#else
  g_signal_connect(G_OBJECT(ih->handle), "navigation-policy-decision-requested", G_CALLBACK(gtkWebBrowserNavigate), ih);
#endif
#ifdef USE_WEBKIT2
  g_signal_connect(G_OBJECT(ih->handle), "load-failed", G_CALLBACK(gtkWebBrowserLoadError), ih);
#else
  g_signal_connect(G_OBJECT(ih->handle), "load-error", G_CALLBACK(gtkWebBrowserLoadError), ih);
#endif
#ifdef USE_WEBKIT2
  g_signal_connect(G_OBJECT(ih->handle), "load-changed", G_CALLBACK(gtkWebBrowserDocumentLoadFinished), ih);
#else
  g_signal_connect(G_OBJECT(ih->handle), "document-load-finished", G_CALLBACK(gtkWebBrowserDocumentLoadFinished), ih);
#endif

#ifndef USE_WEBKIT2
  gtk_widget_realize((GtkWidget*)scrolled_window);
#endif
  gtk_widget_realize(ih->handle);

  return IUP_NOERROR;
}

static void gtkWebBrowserComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0, natural_h = 0;
  (void)children_expand; /* unset if not a container */

  /* natural size is 1 character */
  iupdrvFontGetCharSize(ih, &natural_w, &natural_h);

  *w = natural_w;
  *h = natural_h;
}

static int gtkWebBrowserCreateMethod(Ihandle* ih, void **params)
{
  (void)params;

  ih->data = iupALLOCCTRLDATA();

  /* default EXPAND is YES */
  ih->expand = IUP_EXPAND_BOTH;
  ih->data->sb = IUP_SB_HORIZ | IUP_SB_VERT;  /* default is YES */

  return IUP_NOERROR; 
}

Iclass* iupWebBrowserNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "webbrowser";
  ic->cons = "WebBrowser";
  ic->format = NULL; /* no parameters */
  ic->nativetype  = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;   /* has attributes with IDs that must be parsed */

  /* Class functions */
  ic->New = iupWebBrowserNewClass;
  ic->Create = gtkWebBrowserCreateMethod;
  ic->Map = gtkWebBrowserMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->ComputeNaturalSize = gtkWebBrowserComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "NEWWINDOW_CB", "s");
  iupClassRegisterCallback(ic, "NAVIGATE_CB", "s");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");
  iupClassRegisterCallback(ic, "COMPLETED_CB", "s");

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT); 

  /* IupWebBrowser only */
  iupClassRegisterAttribute(ic, "VALUE", gtkWebBrowserGetValueAttrib, gtkWebBrowserSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKFORWARD", NULL, gtkWebBrowserSetBackForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOBACK", NULL, gtkWebBrowserSetGoBackAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOFORWARD", NULL, gtkWebBrowserSetGoForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", NULL, gtkWebBrowserSetStopAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RELOAD", NULL, gtkWebBrowserSetReloadAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HTML", gtkWebBrowserGetHTMLAttrib, gtkWebBrowserSetHTMLAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATUS", gtkWebBrowserGetStatusAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZOOM", gtkWebBrowserGetZoomAttrib, gtkWebBrowserSetZoomAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINT", NULL, gtkWebBrowserSetPrintAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOBACK", gtkWebBrowserGetCanGoBackAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOFORWARD", gtkWebBrowserGetCanGoForwardAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EDITABLE", gtkWebBrowserGetEditableAttrib, gtkWebBrowserSetEditableAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NEW", NULL, gtkWebBrowserSetNewAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPENFILE", NULL, gtkWebBrowserSetOpenAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEFILE", NULL, gtkWebBrowserSetSaveAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UNDO", NULL, gtkWebBrowserSetUndoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDO", NULL, gtkWebBrowserSetRedoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUT", NULL, gtkWebBrowserSetCutAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPY", NULL, gtkWebBrowserSetCopyAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTE", NULL, gtkWebBrowserSetPasteAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTALL", NULL, gtkWebBrowserSetSelectAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXECCOMMAND", NULL, gtkWebBrowserExecCommandAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGE", NULL, gtkWebBrowserSetInsertImageAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGEFILE", NULL, gtkWebBrowserSetInsertImageFileAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CREATELINK", NULL, gtkWebBrowserSetCreateLinkAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTTEXT", NULL, gtkWebBrowserSetInsertTextAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTHTML", NULL, gtkWebBrowserSetInsertHtmlAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTNAME", NULL, gtkWebBrowserSetFontNameAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTSIZE", NULL, gtkWebBrowserSetFontSizeAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATBLOCK", NULL, gtkWebBrowserSetFormatBlockAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORECOLOR", NULL, gtkWebBrowserSetForeColorAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", NULL, gtkWebBrowserSetBackColorAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* GTK only */
  iupClassRegisterAttribute(ic, "BACKCOUNT", gtkWebBrowserGetBackCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORWARDCOUNT", gtkWebBrowserGetForwardCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMHISTORY",  gtkWebBrowserGetItemHistoryAttrib,  NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  return ic;
}

/*
Possibilities:

wk2
webkit_web_view_can_execute_editing_command

Dirty
The “user-changed-contents” signal
void user_function (WebKitWebView *web_view, gpointer user_data) wk1

The “selection-changed” signal -- UPDATECOMMANDS_CB ???
void user_function (WebKitWebEditor *editor, gpointer user_data) wk2
void user_function (WebKitWebView *web_view, gpointer user_data) wk1

wk1
webkit_web_view_can_copy_clipboard
webkit_web_view_can_cut_clipboard
webkit_web_view_can_paste_clipboard
webkit_web_view_can_redo
webkit_web_view_can_undo

wk2
guint	webkit_editor_state_get_typing_attributes ()
gboolean	webkit_editor_state_is_cut_available ()
gboolean	webkit_editor_state_is_copy_available ()
gboolean	webkit_editor_state_is_paste_available ()
gboolean	webkit_editor_state_is_undo_available ()
gboolean	webkit_editor_state_is_redo_available ()

Find - no dialog (must build one)
webkit_web_view_search_text wk1
WebKitFindController wk2
*/

/** \file
* \brief Web Browser Control
*
* http://msdn.microsoft.com/en-us/library/aa752040(v=vs.85).aspx
* https://docs.microsoft.com/en-us/previous-versions/windows/internet-explorer/ie-developer/platform-apis/aa752040(v=vs.85)
* https://docs.microsoft.com/en-us/previous-versions/windows/internet-explorer/ie-developer/platform-apis/hh801968(v=vs.85)
*
* See Copyright Notice in "iup.h"
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <exdisp.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupole.h"

#include "iup_object.h"
#include "iup_register.h"
#include "iup_attrib.h"
#include "iup_stdcontrols.h"
#include "iup_str.h"
#include "iup_layout.h"
#include "iup_webbrowser.h"
#include "iup_drv.h"
#include "iup_drvfont.h"

#include <shlguid.h>   /* IID_IWebBrowser2, DIID_DWebBrowserEvents2 */
#include <exdispid.h>  /* DISPID_*   */
#include <mshtml.h>
#include <mshtmcid.h>
//#include <MsHtmHst.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>

#if _MSC_VER < 1500  // VC9: VC8 does not defines this
#define OLECMDID_OPTICAL_ZOOM (OLECMDID)63
#endif

/* Exported from "iupwin_str.c" */
extern "C" {
  IUP_DRV_API WCHAR* iupwinStrChar2Wide(const char* str);
  IUP_DRV_API char*  iupwinStrWide2Char(const WCHAR* wstr);
}

using namespace ATL;

// Should have only one instance of a class
// derived from CAtlModule in a project.
static CComModule* iweb_module = NULL;

interface winWebBrowserSink:public IDispEventImpl<0, winWebBrowserSink, &DIID_DWebBrowserEvents2, &LIBID_SHDocVw, 1, 0>
{
public:
  Ihandle* ih;

  BEGIN_SINK_MAP(winWebBrowserSink)
    SINK_ENTRY_EX(0, DIID_DWebBrowserEvents2, DISPID_BEFORENAVIGATE2, BeforeNavigate2)
#ifdef DISPID_NEWWINDOW3
    SINK_ENTRY_EX(0, DIID_DWebBrowserEvents2, DISPID_NEWWINDOW3, NewWindow3)
#endif
    SINK_ENTRY_EX(0, DIID_DWebBrowserEvents2, DISPID_NAVIGATEERROR, NavigateError)
    SINK_ENTRY_EX(0, DIID_DWebBrowserEvents2, DISPID_DOCUMENTCOMPLETE, DocumentComplete)
    SINK_ENTRY_EX(0, DIID_DWebBrowserEvents2, DISPID_COMMANDSTATECHANGE, CommandStateChange)
  END_SINK_MAP()

  void STDMETHODCALLTYPE BeforeNavigate2(IDispatch *pDispatch, VARIANT *url, VARIANT *Flags, VARIANT *TargetFrameName,
                                         VARIANT *PostData, VARIANT *Headers, VARIANT_BOOL *Cancel)
  {
    if (iupAttribGet(ih, "_IUPWEB_IGNORE_NAVIGATE"))
      return;

    IFns cb = (IFns)IupGetCallback(ih, "NAVIGATE_CB");
    if (cb)
    {
      char* urlString = iupwinStrWide2Char(url->bstrVal);
      int ret = cb(ih, urlString);
      free(urlString);
      if (ret == IUP_IGNORE)
        *Cancel = VARIANT_TRUE;
    }

    (void)Cancel;
    (void)Headers;
    (void)PostData;
    (void)TargetFrameName;
    (void)Flags;
    (void)pDispatch;
  }

#ifdef DISPID_NEWWINDOW3
  void STDMETHODCALLTYPE NewWindow3(IDispatch **ppDispatch, VARIANT_BOOL *Cancel, DWORD dwFlags,
                                    BSTR bstrUrlContext, BSTR bstrUrl)
  {
    IFns cb = (IFns)IupGetCallback(ih, "NEWWINDOW_CB");
    if (cb)
    {
      char* urlString = iupwinStrWide2Char(bstrUrl);
      cb(ih, urlString);
      free(urlString);
    }

    (void)bstrUrlContext;
    (void)dwFlags;
    (void)Cancel;
    (void)ppDispatch;
  }
#endif

  void STDMETHODCALLTYPE NavigateError(IDispatch *pDispatch, VARIANT *url, VARIANT *TargetFrameName, 
                     VARIANT *StatusCode, VARIANT_BOOL *Cancel)
  {
    iupAttribSet(ih, "_IUPWEB_FAILED", "1");
    IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (cb)
    {
      char* urlString = iupwinStrWide2Char(url->bstrVal);
      cb(ih, urlString);
      free(urlString);
    }

    (void)TargetFrameName;
    (void)StatusCode;
    (void)Cancel;
    (void)pDispatch;
  }

  void STDMETHODCALLTYPE DocumentComplete(IDispatch *pDispatch, VARIANT *url)
  {
    IFns cb = (IFns)IupGetCallback(ih, "COMPLETED_CB");
    if (cb)
    {
      char* urlString = iupwinStrWide2Char(url->bstrVal);
      cb(ih, urlString);
      free(urlString);
    }
    (void)pDispatch;
  }

  void STDMETHODCALLTYPE CommandStateChange(LONG Command, VARIANT_BOOL Enable)
  {
    if (Command == CSC_NAVIGATEFORWARD)
    {
      if (Enable == VARIANT_TRUE)
        iupAttribSet(ih, "CANGOFORWARD", "YES");
      else
        iupAttribSet(ih, "CANGOFORWARD", "NO");
    }
    else if (Command == CSC_NAVIGATEBACK)
    {
      if (Enable == VARIANT_TRUE)
        iupAttribSet(ih, "CANGOBACK", "YES");
      else
        iupAttribSet(ih, "CANGOBACK", "NO");
    }
    else if (Command == CSC_UPDATECOMMANDS)
    {
      IFn cb = IupGetCallback(ih, "UPDATE_CB");
      if (cb)
        cb(ih);
    }
  }
};


/**********************************************************************************/


static BSTR winStrChar2BStr(const char* str)
{
  WCHAR* wstr = iupwinStrChar2Wide(str);
  BSTR bstr = SysAllocString(wstr);
  if (wstr) free(wstr);
  return bstr;
}

static void winVariantBStr(VARIANT *var, BSTR bstr)
{
  VariantInit(var);
  var->vt = VT_BSTR;
  var->bstrVal = bstr;
}

static void winVariantLong(VARIANT *var, LONG val)
{
  VariantInit(var);
  var->vt = VT_I4;
  var->lVal = val;
}

static SAFEARRAY* winVariantSafeArray(BSTR bstr)
{
  VARIANT *param;
  SAFEARRAY *sfArray;
  sfArray = SafeArrayCreateVector(VT_VARIANT, 0, 1);    // must call SafeArrayDestroy
  SafeArrayAccessData(sfArray,(LPVOID*) &param);
  param->vt = VT_BSTR;
  param->bstrVal = bstr;
  SafeArrayUnaccessData(sfArray);
  return sfArray;
}

static IDispatch* winWebBrowserGetDispatch(Ihandle* ih, IWebBrowser2 *pWeb)
{
  /* Retrieve the document object. */
  IDispatch* pDispatch = NULL;
  pWeb->get_Document(&pDispatch);
  if (!pDispatch)
  {
    iupAttribSet(ih, "_IUPWEB_FAILED", NULL);
    iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");
    BSTR url = SysAllocString(L"about:blank");
    pWeb->Navigate(url, NULL, NULL, NULL, NULL);
    SysFreeString(url);
    IupFlush();
    iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", NULL);

    pWeb->get_Document(&pDispatch);
  }

  return pDispatch;
}


/********************************************************************************/

static char* winWebBrowserGetDirtyAttrib(Ihandle* ih)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  /* Retrieve the document object */
  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IPersistStreamInit* pPersistStreamInit = NULL;
  pDispatch->QueryInterface(IID_IPersistStreamInit, (void**)&pPersistStreamInit);

  /* Load the contents of the stream.  */
  int ret = pPersistStreamInit->IsDirty() == S_OK? 1: 0;

  /* Releases */
  pPersistStreamInit->Release();
  pDispatch->Release();

  return iupStrReturnBoolean(ret);
}

static int winWebBrowserSetEditableAttrib(Ihandle* ih, const char* value);

static int winWebBrowserSetNewAttrib(Ihandle* ih, const char* value)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  /* Retrieve the document object */
  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IPersistStreamInit* pPersistStreamInit = NULL;
  pDispatch->QueryInterface(IID_IPersistStreamInit, (void**)&pPersistStreamInit);

  /* Load the contents of the stream.  */
  pPersistStreamInit->InitNew();

  /* Releases */
  pPersistStreamInit->Release();
  pDispatch->Release();

  winWebBrowserSetEditableAttrib(ih, "Yes");

  (void)value;
  return 0;
}

#if 1
static int winWebBrowserSetHTMLAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  int size = (int)strlen(value) + 1;

  /* Create the memory for the stream */
  HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);

  /* fill the data */
  LPVOID pData = GlobalLock(hMem);
  memcpy(pData, value, size);
  GlobalUnlock(hMem);

  /* Create the stream  */
  IStream* pStream = NULL;
  CreateStreamOnHGlobal(hMem, FALSE, &pStream);

  /* Retrieve the document object */
  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IPersistStreamInit* pPersistStreamInit = NULL;
  pDispatch->QueryInterface(IID_IPersistStreamInit, (void**)&pPersistStreamInit);

  /* Load the contents of the stream.  */
  pPersistStreamInit->Load(pStream);

  /* Releases */
  pPersistStreamInit->Release();
  pStream->Release();
  pDispatch->Release();
  GlobalFree(hMem);
  
  return 0; /* do not store value in hash table */
}

#else

static int winWebBrowserSetHTMLAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  /* Retrieve the document object. */
  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

	IHTMLDocument2 *pHtmlDoc;
  pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pHtmlDoc);

  BSTR bvalue = winStrChar2BStr(value);

  SAFEARRAY *sfArray = winVariantSafeArray(bvalue);

	pHtmlDoc->write(sfArray);
	pHtmlDoc->close();

  /* Releases */
  SafeArrayDestroy(sfArray);
  SysFreeString(bvalue);
  pHtmlDoc->Release();
  pDispatch->Release();
  
  return 0; /* do not store value in hash table */
}
#endif

static char* winWebBrowserGetHTMLAttrib(Ihandle* ih)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  /* Create the stream  */
  IStream* pStream = NULL;
  CreateStreamOnHGlobal(NULL, TRUE, &pStream);

  /* Retrieve the document object */
  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IPersistStreamInit* pPersistStreamInit = NULL;
  pDispatch->QueryInterface(IID_IPersistStreamInit, (void**)&pPersistStreamInit);

  /* Save the contents of the stream.  */
  pPersistStreamInit->Save(pStream, TRUE); /* clear dirty flag */

  HGLOBAL hMem;
  GetHGlobalFromStream(pStream, &hMem);

  LPVOID pData = GlobalLock(hMem);
  int size = (int)GlobalSize(hMem);
  char* str = (char*)pData;
  if (str[size-1] != 0)
    str[size - 1] = 0;
  char* value = iupStrReturnStr(str);
  GlobalUnlock(hMem);

  /* Releases */
  pPersistStreamInit->Release();
  pStream->Release();
  pDispatch->Release();

  return value;
}

static int write_file(const char* filename, const char* str, int count)
{
  FILE* file = fopen(filename, "wb");
  if (!file)
    return 0;

  fwrite(str, 1, count, file);

  fclose(file);
  return 1;
}

static int winWebBrowserSetSaveAttrib(Ihandle* ih, const char* value)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  /* Create the stream  */
  IStream* pStream = NULL;
  CreateStreamOnHGlobal(NULL, TRUE, &pStream);

  /* Retrieve the document object */
  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IPersistStreamInit* pPersistStreamInit = NULL;
  pDispatch->QueryInterface(IID_IPersistStreamInit, (void**)&pPersistStreamInit);

  /* Save the contents of the stream.  */
  pPersistStreamInit->Save(pStream, TRUE); /* clear dirty flag */

  HGLOBAL hMem;
  GetHGlobalFromStream(pStream, &hMem);

  LPVOID pData = GlobalLock(hMem);
  int size = (int)GlobalSize(hMem);
  char* str = (char*)pData;
  if (str[size - 1] != 0)
    str[size - 1] = 0;

  write_file(value, str, size);

  GlobalUnlock(hMem);

  /* Releases */
  pPersistStreamInit->Release();
  pStream->Release();
  pDispatch->Release();

  return 0;
}

static int winWebBrowserSetValueAttrib(Ihandle* ih, const char* value);

static int winWebBrowserSetOpenAttrib(Ihandle* ih, const char* value)
{
  char* url = iupStrFileMakeURL(value);
  winWebBrowserSetValueAttrib(ih, url);
  winWebBrowserSetEditableAttrib(ih, "Yes");
  free(url);
  return 0;
}

static int winWebBrowserSetCopyAttrib(Ihandle* ih, const char* value)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  pWeb->ExecWB(OLECMDID_COPY, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
  (void)value;
  return 0;
}

static int winWebBrowserSetCutAttrib(Ihandle* ih, const char* value)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  pWeb->ExecWB(OLECMDID_CUT, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
  (void)value;
  return 0;
}

static int winWebBrowserSetPasteAttrib(Ihandle* ih, const char* value)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  pWeb->ExecWB(OLECMDID_PASTE, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
  (void)value;
  return 0;
}

static char* winWebBrowserGetPasteAttrib(Ihandle* ih)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  OLECMDF status;
  if (pWeb->QueryStatusWB(OLECMDID_PASTE, &status) == S_OK)
  {
    int state = status & OLECMDF_ENABLED;
//    int state = status & OLECMDF_LATCHED;
    return iupStrReturnBoolean(state);
  }
  return NULL;
}

static int winWebBrowserSetUndoAttrib(Ihandle* ih, const char* value)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  pWeb->ExecWB(OLECMDID_UNDO, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
  (void)value;
  return 0;
}

static int winWebBrowserSetRedoAttrib(Ihandle* ih, const char* value)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  pWeb->ExecWB(OLECMDID_REDO, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
  (void)value;
  return 0;
}

static int winWebBrowserSetSelectAllAttrib(Ihandle* ih, const char* value)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  pWeb->ExecWB(OLECMDID_SELECTALL, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
  (void)value;
  return 0;
}

static int winWebBrowserSetFindAttrib(Ihandle* ih, const char* value)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  pWeb->ExecWB(OLECMDID_FIND, OLECMDEXECOPT_PROMPTUSER, NULL, NULL);
  (void)value;
  return 0;
}

static void winWebBrowserExecCommandIdBool(Ihandle* ih, UINT nID, int value);

static int winWebBrowserSetPrintAttrib(Ihandle* ih, const char* value)
{
#if 0
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  pWeb->ExecWB(OLECMDID_PRINT2, iupStrBoolean(value)? OLECMDEXECOPT_PROMPTUSER: OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
#else
  winWebBrowserExecCommandIdBool(ih, IDM_PRINT, iupStrBoolean(value));
#endif
  return 0;
}

static int winWebBrowserSetZoomAttrib(Ihandle* ih, const char* value)
{
  int zoom;
  if (iupStrToInt(value, &zoom))
  {
    IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

    VARIANT var;
    winVariantLong(&var, (LONG)zoom);

    // OLECMDID_OPTICAL_ZOOM = VT_I4 (LONG) parameter in the range of 10 to 1000 (percent).
    pWeb->ExecWB(OLECMDID_OPTICAL_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER, &var, NULL);
  }
  return 0;
}

static char* winWebBrowserGetZoomAttrib(Ihandle* ih)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  VARIANT var;
  winVariantLong(&var, 0);

  pWeb->ExecWB(OLECMDID_OPTICAL_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER, NULL, &var);

  return iupStrReturnInt((int)var.lVal);
}

static IHTMLElement* winWebBrowserFindElement(Ihandle* ih, const char* element_id)
{
  HRESULT hr;
  IHTMLDocument2* pHtmlDoc = NULL;
  IWebBrowser2* pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  // Retrieve the document object.
  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  hr = pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pHtmlDoc);

  IHTMLElementCollection* pColl = NULL;
  hr = pHtmlDoc->get_all(&pColl);
  if (SUCCEEDED(hr) && (pColl != NULL))
  {
    // Obtained the Anchor Collection...
    long nLength = 0;
    pColl->get_length(&nLength);

    for (int i = 0; i < nLength; i++)
    {
      VARIANT vIdx;
      winVariantLong(&vIdx, (LONG)i);

      IDispatch* pElemDispatch = NULL;
      IHTMLElement * pElem = NULL;

      hr = pColl->item(vIdx, vIdx, &pElemDispatch);

      if (SUCCEEDED(hr) && (pElemDispatch != NULL))
      {
        hr = pElemDispatch->QueryInterface(IID_IHTMLElement, (void**)&pElem);

        if (SUCCEEDED(hr) && (pElem != NULL))
        {
          BSTR bstrId;
          if (!FAILED(pElem->get_id(&bstrId)))
          {
            char* str = iupwinStrWide2Char(bstrId);
            SysFreeString(bstrId);

            if (iupStrEqual(str, element_id))
            {
              free(str);
              pElemDispatch->Release();
              pColl->Release();
              pHtmlDoc->Release();
              pDispatch->Release();

              return pElem;
            }

            free(str);
          }

          pElem->Release();
        }
        pElemDispatch->Release();
      }
    }
    pColl->Release();
  }

  pHtmlDoc->Release();
  pDispatch->Release();

  return NULL;
}

static int winWebBrowserSetInnerTextAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    char* element_id = iupAttribGet(ih, "ELEMENT_ID");
    if (element_id)
    {
      IHTMLElement* pElem = winWebBrowserFindElement(ih, element_id);
      if (pElem)
      {
        BSTR bvalue = winStrChar2BStr(value);
        pElem->put_innerText(bvalue);
        SysFreeString(bvalue);
        pElem->Release();
      }
    }
  }
  return 0;
}

static char* winWebBrowserGetInnerTextAttrib(Ihandle* ih)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  if (element_id)
  {
    IHTMLElement* pElem = winWebBrowserFindElement(ih, element_id);
    if (pElem)
    {
      BSTR bvalue = NULL;
      if (!FAILED(pElem->get_innerText(&bvalue)))
      {
        char* str = iupwinStrWide2Char(bvalue);
        char* value = iupStrReturnStr(str);
        SysFreeString(bvalue);
        free(str);
        pElem->Release();
        return value;
      }
      pElem->Release();
    }
  }

  return NULL;
}

static int winWebBrowserSetAttributeAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    char* element_id = iupAttribGet(ih, "ELEMENT_ID");
    char* attribute_name = iupAttribGet(ih, "ATTRIBUTE_NAME");
    if (element_id && attribute_name)
    {
      IHTMLElement* pElem = winWebBrowserFindElement(ih, element_id);
      if (pElem)
      {
        BSTR bname = winStrChar2BStr(attribute_name);
        BSTR bvalue = winStrChar2BStr(value);

        VARIANT var;
        winVariantBStr(&var, bvalue);

        pElem->setAttribute(bname, var, 1);  // case sensitive search

        SysFreeString(bvalue);
        SysFreeString(bname);
        pElem->Release();
      }
    }
  }
  return 0;
}

static char* winWebBrowserGetAttributeAttrib(Ihandle* ih)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  char* attribute_name = iupAttribGet(ih, "ATTRIBUTE_NAME");
  if (element_id && attribute_name)
  {
    IHTMLElement* pElem = winWebBrowserFindElement(ih, element_id);
    if (pElem)
    {
      BSTR bname = winStrChar2BStr(attribute_name);
      VARIANT var;
      VariantInit(&var);
      if (!FAILED(pElem->getAttribute(bname, 1, &var)) && var.bstrVal)  // case sensitive search
      {
        char* str = iupwinStrWide2Char(var.bstrVal);
        char* value = iupStrReturnStr(str);
        free(str);
        SysFreeString(bname);
        pElem->Release();
        return value;
      }
      SysFreeString(bname);
      pElem->Release();
    }
  }

  return NULL;
}

static int winWebBrowserSetBackForwardAttrib(Ihandle* ih, const char* value)
{
  int i, val;
  if (iupStrToInt(value, &val))
  {
    IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

    /* Negative values represent steps backward while positive values represent steps forward. */
    if(val > 0)
    {
      for(i = 0; i < val; i++)
        pWeb->GoForward();
    }
    else if(val < 0)
    {
      for(i = 0; i < -(val); i++)
        pWeb->GoBack();
    }
  }

  return 0; /* do not store value in hash table */
}

static int winWebBrowserSetGoBackAttrib(Ihandle* ih, const char* value)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  pWeb->GoBack();
  (void)value;
  return 0; /* do not store value in hash table */
}

static int winWebBrowserSetGoForwardAttrib(Ihandle* ih, const char* value)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  pWeb->GoForward();
  (void)value;
  return 0; /* do not store value in hash table */
}

static int winWebBrowserSetReloadAttrib(Ihandle* ih, const char* value)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  pWeb->Refresh();

  (void)value;
  return 0; /* do not store value in hash table */
}

static int winWebBrowserSetStopAttrib(Ihandle* ih, const char* value)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  pWeb->Stop();

  (void)value;
  return 0; /* do not store value in hash table */
}

static char* winWebBrowserGetStatusAttrib(Ihandle* ih)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  READYSTATE plReadyState;
  pWeb->get_ReadyState(&plReadyState);
  if (iupAttribGet(ih, "_IUPWEB_FAILED"))
    return "FAILED";
  else if (plReadyState == READYSTATE_COMPLETE)
    return "COMPLETED";
  else
    return "LOADING";
}

static int winWebBrowserSetValueAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
    BSTR bvalue = winStrChar2BStr(value);
    BSTR btarget = SysAllocString(L"_top");

    VARIANT var;
    winVariantBStr(&var, btarget);

    iupAttribSet(ih, "_IUPWEB_FAILED", NULL);

    pWeb->Navigate(bvalue, NULL, &var, NULL, NULL);
    SysFreeString(bvalue);
    SysFreeString(btarget);
  }
  return 0;
}

static char* winWebBrowserGetValueAttrib(Ihandle* ih)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  BSTR pbstrLocationURL = NULL;
  if (pWeb->get_LocationURL(&pbstrLocationURL)==S_OK && pbstrLocationURL)
  {
    char* str = iupwinStrWide2Char(pbstrLocationURL);
    SysFreeString(pbstrLocationURL);
    char* value = iupStrReturnStr(str);
    free(str);
    return value;
  }
  return NULL;
}

static int winWebBrowserSetEditableAttrib(Ihandle* ih, const char* value)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  /* Retrieve the document object. */
  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IHTMLDocument2 *pHtmlDoc;
  pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pHtmlDoc);

  BSTR bvalue = SysAllocString(iupStrBoolean(value) ? L"On" : L"Off");

  pHtmlDoc->put_designMode(bvalue);

  /* Releases */
  SysFreeString(bvalue);
  pHtmlDoc->Release();
  pDispatch->Release();

  return 0;
}

static char* winWebBrowserGetEditableAttrib(Ihandle* ih)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  /* Retrieve the document object. */
  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IHTMLDocument2 *pHtmlDoc;
  pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pHtmlDoc);

  BSTR bvalue = NULL;
  pHtmlDoc->get_designMode(&bvalue);

  /* Releases */
  pHtmlDoc->Release();
  pDispatch->Release();

  if (bvalue)
  {
    char* str = iupwinStrWide2Char(bvalue);
    SysFreeString(bvalue);
    int ret = iupStrEqual(str, "On");
    free(str);
    return iupStrReturnBoolean(ret);
  }

  return NULL;
}

static long winWebBrowserQueryStatus(IOleCommandTarget* pCmdTarg, ULONG cmdID)
{
  OLECMD ocmd = { cmdID, 0 };
  if (S_OK == pCmdTarg->QueryStatus(&CGID_MSHTML, 1, &ocmd, NULL))
    return ocmd.cmdf;
  return 0;
}

static HRESULT winWebBrowserExecHelperNN(IOleCommandTarget* pCmdTarg, UINT nID)
{
  HRESULT hr = E_FAIL;
  long nMinSupportLevel = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
  long nExecOpt = OLECMDEXECOPT_DODEFAULT;
  long lStatus = winWebBrowserQueryStatus(pCmdTarg, nID);
  if ((lStatus & nMinSupportLevel) == nMinSupportLevel)
    hr = pCmdTarg->Exec(&CGID_MSHTML, nID, nExecOpt, NULL, NULL);
  return hr;
}

static HRESULT winWebBrowserExecHelperNNBool(IOleCommandTarget* pCmdTarg, UINT nID, int value)
{
  HRESULT hr = E_FAIL;
  long nMinSupportLevel = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
  long nExecOpt = OLECMDEXECOPT_DODEFAULT;
  long lStatus = winWebBrowserQueryStatus(pCmdTarg, nID);
  if ((lStatus & nMinSupportLevel) == nMinSupportLevel)
  {
    VARIANT var;
    VariantInit(&var);

    var.vt = VT_BOOL;
    var.boolVal == value? VARIANT_TRUE: VARIANT_FALSE;

    hr = pCmdTarg->Exec(&CGID_MSHTML, nID, nExecOpt, &var, NULL);
  }
  return hr;
}

static void winWebBrowserExecCommandId(Ihandle* ih, UINT nID)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  /* Retrieve the document object. */
  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IHTMLDocument2 *pHtmlDoc;
  pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pHtmlDoc);

  IOleCommandTarget* pCmdTarg;
  pDispatch->QueryInterface(IID_IOleCommandTarget, (void**)&pCmdTarg);

  winWebBrowserExecHelperNN(pCmdTarg, nID);

  pHtmlDoc->Release();
  pDispatch->Release();
}

static void winWebBrowserExecCommandIdBool(Ihandle* ih, UINT nID, int value)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  /* Retrieve the document object. */
  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IHTMLDocument2 *pHtmlDoc;
  pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pHtmlDoc);

  IOleCommandTarget* pCmdTarg;
  pDispatch->QueryInterface(IID_IOleCommandTarget, (void**)&pCmdTarg);

  winWebBrowserExecHelperNNBool(pCmdTarg, nID, value);

  pHtmlDoc->Release();
  pDispatch->Release();
}

static int winWebBrowserSetPrintPreviewAttrib(Ihandle* ih, const char* value)
{
  winWebBrowserExecCommandId(ih, IDM_PRINTPREVIEW);
  (void)value;
  return 0;
}

static char* winWebBrowserQueryCommandValue(Ihandle* ih, const char* cmd, int is_color)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  /* Retrieve the document object. */
  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IHTMLDocument2 *pHtmlDoc;
  pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pHtmlDoc);

  BSTR bCmd = winStrChar2BStr(cmd);
  VARIANT var;
  VariantInit(&var);
  pHtmlDoc->queryCommandValue(bCmd, &var);

  char* value = NULL;
  if (var.vt == VT_BSTR)
  {
    char* str = iupwinStrWide2Char(var.bstrVal);
    value = iupStrReturnStr(str);
    free(str);
  }
  else if (var.vt == VT_I4)
  {
    if (is_color)
      value = iupStrReturnRGB(GetRValue(var.lVal), GetGValue(var.lVal), GetBValue(var.lVal));
    else
      value = iupStrReturnInt((int)var.lVal);
  }
  else if (var.vt == VT_I2)
    value = iupStrReturnInt((int)var.iVal);
  else if (var.vt == VT_BOOL)
    value = iupStrReturnBoolean(var.boolVal == VARIANT_TRUE ? 1 : 0);

  SysFreeString(bCmd);

  pHtmlDoc->Release();
  pDispatch->Release();

  return value;
}

static char* winWebBrowserQueryCommandText(Ihandle* ih, const char* cmd)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  /* Retrieve the document object. */
  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IHTMLDocument2 *pHtmlDoc;
  pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pHtmlDoc);

  BSTR bCmd = winStrChar2BStr(cmd);
  BSTR cmdText = NULL;
  pHtmlDoc->queryCommandText(bCmd, &cmdText);

  char* value = NULL;
  if (cmdText)
  {
    char* str = iupwinStrWide2Char(cmdText);
    value = iupStrReturnStr(str);
    free(str);
  }

  SysFreeString(bCmd);

  pHtmlDoc->Release();
  pDispatch->Release();

  return value;
}

static void winWebBrowserExecCommand(Ihandle* ih, const char* cmd, int show_ui)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  /* Retrieve the document object. */
  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IHTMLDocument2 *pHtmlDoc;
  pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pHtmlDoc);

  BSTR bCmd = winStrChar2BStr(cmd);
  VARIANT var;
  VariantInit(&var);

  pHtmlDoc->execCommand(bCmd, show_ui? VARIANT_TRUE: VARIANT_FALSE, var, NULL);

  SysFreeString(bCmd);

  pHtmlDoc->Release();
  pDispatch->Release();
}

static void winWebBrowserExecCommandParam(Ihandle* ih, const char* cmd, int show_ui, const char* param)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IHTMLDocument2 *pHtmlDoc;
  pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pHtmlDoc);

  BSTR bCmd = winStrChar2BStr(cmd);
  BSTR bParam = winStrChar2BStr(param);
  VARIANT var;
  winVariantBStr(&var, bParam);

  pHtmlDoc->execCommand(bCmd, show_ui? VARIANT_TRUE: VARIANT_FALSE, var, NULL);

  SysFreeString(bCmd);
  SysFreeString(bParam);

  pHtmlDoc->Release();
  pDispatch->Release();
}

static void winWebBrowserExecCommandParamInt(Ihandle* ih, const char* cmd, int show_ui, int param)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IHTMLDocument2 *pHtmlDoc;
  pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pHtmlDoc);

  BSTR bCmd = winStrChar2BStr(cmd);
  VARIANT var;
  winVariantLong(&var, (LONG)param);

  pHtmlDoc->execCommand(bCmd, show_ui ? VARIANT_TRUE : VARIANT_FALSE, var, NULL);

  SysFreeString(bCmd);

  pHtmlDoc->Release();
  pDispatch->Release();
}

static int winWebBrowserQueryCommandEnabled(Ihandle* ih, const char* cmd)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IHTMLDocument2 *pHtmlDoc;
  pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pHtmlDoc);

  BSTR bCmd = winStrChar2BStr(cmd);
  VARIANT_BOOL vRet;

  pHtmlDoc->queryCommandEnabled(bCmd, &vRet);

  int ret = vRet == VARIANT_TRUE? 1: 0;

  SysFreeString(bCmd);

  pHtmlDoc->Release();
  pDispatch->Release();

  return ret;
}

static int winWebBrowserQueryCommandState(Ihandle* ih, const char* cmd)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IHTMLDocument2 *pHtmlDoc;
  pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pHtmlDoc);

  BSTR bCmd = winStrChar2BStr(cmd);
  VARIANT_BOOL vRet;

  pHtmlDoc->queryCommandState(bCmd, &vRet);

  int ret = vRet == VARIANT_TRUE ? 1 : 0;

  SysFreeString(bCmd);

  pHtmlDoc->Release();
  pDispatch->Release();

  return ret;
}

static int winWebBrowserSetExecCommandAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    int show_ui = iupAttribGetBoolean(ih, "COMMANDSHOWUI");
    winWebBrowserExecCommand(ih, value, show_ui);
  }
  return 0;
}

static char* winWebBrowserGetCommandStateAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
    return iupStrReturnBoolean(winWebBrowserQueryCommandState(ih, cmd));
  return NULL;
}

static char* winWebBrowserGetCommandEnabledAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
    return iupStrReturnBoolean(winWebBrowserQueryCommandEnabled(ih, cmd));
  return NULL;
}

static char* winWebBrowserGetCommandTextAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
    return iupStrReturnStr(winWebBrowserQueryCommandText(ih, cmd));
  return NULL;
}

static char* winWebBrowserGetCommandValueAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
    return iupStrReturnStr(winWebBrowserQueryCommandValue(ih, cmd, 0));
  return NULL;
}

static int winWebBrowserSetInsertImageAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    int show_ui = iupAttribGetBoolean(ih, "COMMANDSHOWUI");
    winWebBrowserExecCommandParam(ih, "insertImage", show_ui, value);
  }
  else
    winWebBrowserExecCommandParam(ih, "insertImage", 1, value);
  return 0;
}

static int winWebBrowserSetInsertImageFileAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    char* url = iupStrFileMakeURL(value);
    int show_ui = iupAttribGetBoolean(ih, "COMMANDSHOWUI");
    winWebBrowserExecCommandParam(ih, "insertImage", show_ui, url);
    free(url);
  }
  return 0;
}

static int winWebBrowserSetCreateLinkAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    int show_ui = iupAttribGetBoolean(ih, "COMMANDSHOWUI");
    winWebBrowserExecCommandParam(ih, "createLink", show_ui, value);
  }
  else
    winWebBrowserExecCommandParam(ih, "createLink", 1, value);
  return 0;
}

static int winWebBrowserSetFontNameAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    int show_ui = iupAttribGetBoolean(ih, "COMMANDSHOWUI");
    winWebBrowserExecCommandParam(ih, "fontName", show_ui, value);
  }
  return 0;
}

static char* winWebBrowserGetFontNameAttrib(Ihandle* ih)
{
  return winWebBrowserQueryCommandValue(ih, "fontName", 0);
}

static int winWebBrowserSetFontSizeAttrib(Ihandle* ih, const char* value)
{
  int param = 0;
  if (iupStrToInt(value, &param) && param > 0 && param < 8)
  {
    int show_ui = iupAttribGetBoolean(ih, "COMMANDSHOWUI");
    winWebBrowserExecCommandParamInt(ih, "fontSize", show_ui, param);
  }
  return 0;
}

static char* winWebBrowserGetFontSizeAttrib(Ihandle* ih)
{
  return winWebBrowserQueryCommandValue(ih, "fontSize", 0);
}

static int winWebBrowserSetFormatBlockAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    int show_ui = iupAttribGetBoolean(ih, "COMMANDSHOWUI");
    winWebBrowserExecCommandParam(ih, "formatBlock", show_ui, value);
  }
  return 0;
}

static char* winWebBrowserGetFormatBlockAttrib(Ihandle* ih)
{
  return winWebBrowserQueryCommandValue(ih, "formatBlock", 0);
}

static int winWebBrowserSetForeColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    int show_ui = iupAttribGetBoolean(ih, "COMMANDSHOWUI");
    winWebBrowserExecCommandParam(ih, "foreColor", show_ui, value);
  }
  return 0;
}

static char* winWebBrowserGetForeColorAttrib(Ihandle* ih)
{
  return winWebBrowserQueryCommandValue(ih, "foreColor", 1);
}

static int winWebBrowserSetBackColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    int show_ui = iupAttribGetBoolean(ih, "COMMANDSHOWUI");
    winWebBrowserExecCommandParam(ih, "backColor", show_ui, value);
  }
  return 0;
}

static char* winWebBrowserGetBackColorAttrib(Ihandle* ih)
{
  return winWebBrowserQueryCommandValue(ih, "backColor", 1);
}

static void winWebBrowserInsertAtCurrentSelection(Ihandle* ih, const char* str, int as_html)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");

  IDispatch* pDispatch = winWebBrowserGetDispatch(ih, pWeb);

  IHTMLDocument2 *pHtmlDoc;
  pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pHtmlDoc);

  IHTMLSelectionObject* pSelection = NULL;
  HRESULT hResult = pHtmlDoc->get_selection(&pSelection);
  if (SUCCEEDED(hResult) && pSelection)
  {
    IDispatch* pTextRangeDisp = NULL;
    hResult = pSelection->createRange(&pTextRangeDisp);
    if (SUCCEEDED(hResult) && pTextRangeDisp)
    {
      IHTMLTxtRange* pRange = NULL;
      hResult = pTextRangeDisp->QueryInterface(IID_IHTMLTxtRange, (void**)&pRange);
      if (SUCCEEDED(hResult) && pRange)
      {
        BSTR b_str = winStrChar2BStr(str);
        if (as_html)
          pRange->pasteHTML(b_str);
        else
          pRange->put_text(b_str);
        SysFreeString(b_str);

        pRange->Release();
      }
      pTextRangeDisp->Release();
    }
    pSelection->Release();
  }
}

static int winWebBrowserSetInsertTextAttrib(Ihandle* ih, const char* value)
{
  if (value)
    winWebBrowserInsertAtCurrentSelection(ih, value, 0);
  return 0;
}

static int winWebBrowserSetInsertHtmlAttrib(Ihandle* ih, const char* value)
{
  if (value)
    winWebBrowserInsertAtCurrentSelection(ih, value, 1);
  return 0;
}


/******************************************************************************************************************/


static int winWebBrowserCreateMethod(Ihandle* ih, void **params)
{
  (void)params;
  IupSetAttribute(ih, "PROGID", "Shell.Explorer.2");
  IupSetAttribute(ih, "DESIGNMODE", "NO");

  /* Get the current IUnknown* (from IupOleControl) */
  IUnknown *punk = (IUnknown*)IupGetAttribute(ih, "IUNKNOWN");

  IWebBrowser2 *pWeb = NULL;
  punk->QueryInterface(IID_IWebBrowser2, (void **)&pWeb);
  iupAttribSet(ih, "_IUPWEB_BROWSER", (char*)pWeb);

  /* winWebBrowserSink object to capture events */
  winWebBrowserSink* sink = new winWebBrowserSink();

  /* Set handle to use in winWebBrowserSink Interface */
  sink->ih = ih;

  /* Connecting to the server's outgoing interface */
  sink->DispEventAdvise(punk);

  iupAttribSet(ih, "_IUPWEB_SINK", (char*)sink);
  punk->Release();

  return IUP_NOERROR; 
}

static void winWebBrowserDestroyMethod(Ihandle* ih)
{
  IWebBrowser2 *pWeb = (IWebBrowser2*)iupAttribGet(ih, "_IUPWEB_BROWSER");
  pWeb->Release();

  winWebBrowserSink* sink = (winWebBrowserSink*)iupAttribGet(ih, "_IUPWEB_SINK");

  /* Get the current IUnknown* */
  IUnknown *punk = (IUnknown*)IupGetAttribute(ih, "IUNKNOWN");

  /* Disconnecting from the server's outgoing interface */
  sink->DispEventUnadvise(punk);
  delete sink;

  punk->Release();
}

static void winWebBrowserRelease(Iclass* ic)
{
  /* Terminating ATL support */
  if (iweb_module)
  {
    iweb_module->Term();
    delete iweb_module;
    iweb_module = NULL;
  }

  (void)ic;
}

Iclass* iupWebBrowserNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("olecontrol"));

  ic->name = "webbrowser";
  ic->cons = "WebBrowser";
  ic->format = NULL; /* no parameters */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupWebBrowserNewClass;
  ic->Create = winWebBrowserCreateMethod;
  ic->Destroy = winWebBrowserDestroyMethod;
  ic->Release = winWebBrowserRelease;

  /* Callbacks */
  iupClassRegisterCallback(ic, "NEWWINDOW_CB", "s");
  iupClassRegisterCallback(ic, "NAVIGATE_CB", "s");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");
  iupClassRegisterCallback(ic, "COMPLETED_CB", "s");
  iupClassRegisterCallback(ic, "UPDATE_CB", "");

  /* Attributes */
  iupClassRegisterAttribute(ic, "VALUE", winWebBrowserGetValueAttrib, winWebBrowserSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKFORWARD", NULL, winWebBrowserSetBackForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOBACK", NULL, winWebBrowserSetGoBackAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOFORWARD", NULL, winWebBrowserSetGoForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", NULL, winWebBrowserSetStopAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RELOAD", NULL, winWebBrowserSetReloadAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HTML", winWebBrowserGetHTMLAttrib, winWebBrowserSetHTMLAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATUS", winWebBrowserGetStatusAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZOOM", winWebBrowserGetZoomAttrib, winWebBrowserSetZoomAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINT", NULL, winWebBrowserSetPrintAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOBACK", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOFORWARD", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EDITABLE", winWebBrowserGetEditableAttrib, winWebBrowserSetEditableAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NEW", NULL, winWebBrowserSetNewAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPENFILE", NULL, winWebBrowserSetOpenAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEFILE", NULL, winWebBrowserSetSaveAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UNDO", NULL, winWebBrowserSetUndoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDO", NULL, winWebBrowserSetRedoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUT", NULL, winWebBrowserSetCutAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPY", NULL, winWebBrowserSetCopyAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTE", winWebBrowserGetPasteAttrib, winWebBrowserSetPasteAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTALL", NULL, winWebBrowserSetSelectAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FIND", NULL, winWebBrowserSetFindAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXECCOMMAND", NULL, winWebBrowserSetExecCommandAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMAND", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSHOWUI", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSTATE", winWebBrowserGetCommandStateAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDENABLED", winWebBrowserGetCommandEnabledAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDTEXT", winWebBrowserGetCommandTextAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDVALUE", winWebBrowserGetCommandValueAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGEFILE", NULL, winWebBrowserSetInsertImageFileAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGE", NULL, winWebBrowserSetInsertImageAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CREATELINK", NULL, winWebBrowserSetCreateLinkAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTTEXT", NULL, winWebBrowserSetInsertTextAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTHTML", NULL, winWebBrowserSetInsertHtmlAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTNAME", winWebBrowserGetFontNameAttrib, winWebBrowserSetFontNameAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTSIZE", winWebBrowserGetFontSizeAttrib, winWebBrowserSetFontSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATBLOCK", winWebBrowserGetFormatBlockAttrib, winWebBrowserSetFormatBlockAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORECOLOR", winWebBrowserGetForeColorAttrib, winWebBrowserSetForeColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", winWebBrowserGetBackColorAttrib, winWebBrowserSetBackColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DIRTY", winWebBrowserGetDirtyAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINTPREVIEW", NULL, winWebBrowserSetPrintPreviewAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* Windows Only */
  iupClassRegisterAttribute(ic, "ELEMENT_ID", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INNERTEXT", winWebBrowserGetInnerTextAttrib, winWebBrowserSetInnerTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE_NAME", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE", winWebBrowserGetAttributeAttrib, winWebBrowserSetAttributeAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  if (!iweb_module)
  {
    /* CComModule implements a COM server module,
       allowing a client to access the module's components  */
    iweb_module = new CComModule();

    /* Initializing ATL Support */
    iweb_module->Init(NULL, (HINSTANCE)IupGetGlobal("HINSTANCE"));
  }

  return ic;
}

#if 0
// HRESULT error code processing, useful for debugging
#include <comdef.h>
DWORD Win32FromHResult(HRESULT hr)
{
  if ((hr & 0xFFFF0000) == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 0))
    return HRESULT_CODE(hr);
  if (hr == S_OK)
    return ERROR_SUCCESS;
  // Not a Win32 HRESULT so return a generic error code.
  return ERROR_CAN_NOT_COMPLETE;
}
_com_error error(Win32FromHResult(err));
LPCTSTR errorText = error.ErrorMessage();
MessageBox(NULL, errorText, "Error", MB_OK | MB_ICONERROR);
#endif

/*
 * iupolectl.cpp
 *
 *   CPI que implementa um container OLE
 */

#include "tOleHandler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "iup.h"
#include "iupole.h"

#include "iup_object.h"
#include "iup_register.h"
#include "iup_attrib.h"
#include "iup_stdcontrols.h"
#include "iup_str.h"
#include "iup_layout.h"


struct _IcontrolData
{
  iupCanvas canvas;  /* from IupCanvas (must reserve it) */

  tOleHandler* olehandler;
};

static char* iOleControlGetDesignModeAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->olehandler->m_ambientProp.getDesignMode()); 
}

static int iOleControlSetDesignModeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->olehandler->m_ambientProp.setDesignMode(true, true);
  else
    ih->data->olehandler->m_ambientProp.setDesignMode(false, true);
  return 1;
}

static int iOleControlSetDesignModeDontNotifyAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->olehandler->m_ambientProp.setDesignMode(true, false);
  else
    ih->data->olehandler->m_ambientProp.setDesignMode(false, false);
  return 1;
}

static int iOleControlSetProgIdAttrib(Ihandle* ih, const char* value)
{
  CLSID clsid;

  if (!value || iupAttribGet(ih, "PROGID"))  /* can only be set once */
    return 1;

  size_t len = strlen(value)+1;
  wchar_t* wcProgId = (wchar_t*) malloc(len * sizeof(wchar_t));
  mbstowcs(wcProgId, value, len);
  HRESULT hr = CLSIDFromProgID(wcProgId, &clsid);
  free(wcProgId);
  if(FAILED(hr))
    return 0;

  if (ih->data->olehandler->Create(&clsid) == CREATE_FAILED)
    return 0;

  return 1;
}

static char* iOleControlGetIUnknownAttrib(Ihandle* ih)
{
  IUnknown *punk = NULL;
  ih->data->olehandler->ObjectGet(&punk);
  return (char*)punk;
}

static int iOleControlResize_CB(Ihandle *ih)
{
  if (!ih->data->olehandler->m_hWnd)
    return IUP_DEFAULT;

  ih->data->olehandler->OnShow();

  return IUP_DEFAULT;
}

static int iOleControlAction_CB(Ihandle *ih)
{
  (void)ih; /* does nothing to avoid background repaint */
  return IUP_DEFAULT;
}

static void iOleControlComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  long natural_w = 0, natural_h = 0;
  (void)children_expand; /* unset if not a container */

  ih->data->olehandler->GetNaturalSize(&natural_w, &natural_h);

  *w = natural_w;
  *h = natural_h;
}

static void iOleControlLayoutUpdateMethod(Ihandle* ih)
{
  SIZEL szl;
  szl.cx = ih->currentwidth;
  szl.cy = ih->currentheight;
  ih->data->olehandler->SizeSet(&szl, TRUE, TRUE);
  ih->data->olehandler->UpdateInPlaceObjectRects(NULL, TRUE);
}

static int iOleControlMapMethod(Ihandle* ih)
{
  /* reset the canvas BGCOLOR */
  IupSetAttribute(ih, "BACKGROUND", NULL); 

  ih->data->olehandler->m_hWnd = ih->handle;

  return IUP_NOERROR;
}

static int iOleControlCreateMethod(Ihandle* ih, void **params)
{
  /* free the data allocated by IupCanvas */
  free(ih->data);
  ih->data = iupALLOCCTRLDATA();
  ih->data->olehandler = new tOleHandler();

  /* change the IupCanvas default values */
  iupAttribSet(ih, "BORDER", "NO");

  /* IupCanvas callbacks */
  IupSetCallback(ih,"ACTION",(Icallback)iOleControlAction_CB);
  IupSetCallback(ih,"RESIZE_CB",(Icallback)iOleControlResize_CB);

  if (!params || !(params[0]))
    return IUP_NOERROR;

  char *progID = (char*)params[0];
  if (!iOleControlSetProgIdAttrib(ih, progID))
    return IUP_ERROR;

  return IUP_NOERROR;
}

static void iOleControlDestroyMethod(Ihandle* ih)
{
  ih->data->olehandler->Close(true);
  delete ih->data->olehandler;
}

static void iOleControlRelease(Iclass* ic)
{
  (void)ic;

  if (IupGetGlobal("_IUPWIN_OLEINITIALIZE"))
  {
    OleUninitialize();
    IupSetGlobal("_IUPWIN_OLEINITIALIZE", NULL);
  }
}

static Iclass* iOleControlNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name = "olecontrol";
  ic->cons = "OleControl";
  ic->format = "s"; /* one string */
  ic->format_attr = "PROGID";
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iOleControlNewClass;
  ic->Create = iOleControlCreateMethod;
  ic->Destroy = iOleControlDestroyMethod;
  ic->Release = iOleControlRelease;
  ic->Map = iOleControlMapMethod;
  ic->LayoutUpdate = iOleControlLayoutUpdateMethod;
  ic->ComputeNaturalSize = iOleControlComputeNaturalSizeMethod;

  iupClassRegisterAttribute(ic, "PROGID", NULL, iOleControlSetProgIdAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DESIGNMODE", iOleControlGetDesignModeAttrib, iOleControlSetDesignModeAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DESIGNMODE_DONT_NOTIFY", iOleControlGetDesignModeAttrib, iOleControlSetDesignModeDontNotifyAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IUNKNOWN", iOleControlGetIUnknownAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);

  /* Overwrite the canvas implementation */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED);

  return ic;
}

IUPOLE_API Ihandle *IupOleControl(const char *ProgID)
{
  void *params[2];
  params[0] = (void*)ProgID;
  params[1] = NULL;
  return IupCreatev("olecontrol", params);
}

IUPOLE_API int IupOleControlOpen(void)
{
  if (!IupIsOpened())
    return IUP_ERROR;

  if (IupGetGlobal("_IUP_OLECONTROL_OPEN"))
    return IUP_OPENED;

  if (!IupGetGlobal("_IUPWIN_OLEINITIALIZE"))
  {
    OleInitialize(NULL);
    IupSetGlobal("_IUPWIN_OLEINITIALIZE", "1");
  }

  iupRegisterClass(iOleControlNewClass());

  IupSetGlobal("_IUP_OLECONTROL_OPEN", "1");
  return IUP_NOERROR;
}

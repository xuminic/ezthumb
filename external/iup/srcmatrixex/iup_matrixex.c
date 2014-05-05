/** \file
 * \brief IupMatrix Expansion Library.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "iup.h"
#include "iupcbs.h"
#include "iupcontrols.h"
#include "iupmatrixex.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_register.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_predialogs.h"

#include "iup_matrixex.h"


void iupMatrixExGetDialogPosition(ImatExData* matex_data, int *x, int *y)
{
  /* return a dialog position aligned with the bottom-right corner of the focus cell,
     and it will make sure that the focus cell is visible */
  int cx, cy, cw, ch, lin, col;
  char attrib[50];
  IupGetIntInt(matex_data->ih, "SCREENPOSITION", x, y);
  IupGetIntInt(matex_data->ih, "FOCUS_CELL", &lin, &col);
  IupSetfAttribute(matex_data->ih,"SHOW", "%d:%d", lin, col);
  sprintf(attrib, "CELLOFFSET%d:%d", lin, col);
  IupGetIntInt(matex_data->ih, attrib, &cx, &cy);
  sprintf(attrib, "CELLSIZE%d:%d", lin, col);
  IupGetIntInt(matex_data->ih, attrib, &cw, &ch);
  *x += cx + cw;
  *y += cy + ch;
}

static void iMatrixListShowLastError(Ihandle* ih)
{
  char* lasterror = iupAttribGet(ih, "LASTERROR");
  if (lasterror)
    iupShowError(IupGetDialog(ih), lasterror);
}

static void iMatrixExSelectAll(Ihandle *ih)
{
  char* markmode = IupGetAttribute(ih, "MARKMODE");

  if (iupStrEqualNoCasePartial(markmode, "LIN")) /* matches also "LINCOL" */
  {
    int lin, num_lin = IupGetInt(ih, "NUMLIN");
    char* marked = malloc(num_lin+2);

    marked[0] = 'L';
    for(lin = 1; lin <= num_lin; ++lin)
      marked[lin] = '1';
    marked[lin] = 0;

    IupSetStrAttribute(ih, "MARKED", marked);

    free(marked);
  }
  else if (iupStrEqualNoCase(markmode, "COL"))
  {
    int col, num_col = IupGetInt(ih, "NUMCOL");
    char* marked = malloc(num_col+2);

    marked[0] = 'C';
    for(col = 1; col <= num_col; ++col)
      marked[col] = '1';
    marked[col] = 0;

    IupSetStrAttribute(ih, "MARKED", marked);

    free(marked);
  }
  else if (iupStrEqualNoCase(markmode, "CELL"))
  {
    int num_col = IupGetInt(ih, "NUMCOL");
    int num_lin = IupGetInt(ih, "NUMLIN");
    int pos, count = num_lin*num_col;
    char* marked = malloc(count+1);

    for(pos = 0; pos < count; pos++)
      marked[pos] = '1';
    marked[pos] = 0;

    IupSetStrAttribute(ih, "MARKED", marked);

    free(marked);
  }
}

void iupMatrixExCheckLimitsOrder(int *v1, int *v2, int min, int max)
{
  if (*v1<min) *v1 = min;
  if (*v2<min) *v2 = min;
  if (*v1>max) *v1 = max;
  if (*v2>max) *v2 = max;
  if (*v1>*v2) {int v=*v1; *v1=*v2; *v2=v;}
}

static int iMatrixExSetFreezeAttrib(Ihandle *ih, const char* value)
{
  int freeze, lin, col;
  int flin, fcol;

  if (iupStrBoolean(value))
  {
    IupGetIntInt(ih, "FOCUS_CELL", &lin, &col);
    freeze = 1;
  }
  else
  {
    if (iupStrToIntInt(value, &lin, &col, ':')==2)
      freeze = 1;
    else
      freeze = 0;
  }

  /* clear the previous freeze first */
  flin = IupGetInt(ih,"NUMLIN_NOSCROLL");
  fcol = IupGetInt(ih,"NUMCOL_NOSCROLL");
  if (flin) IupSetAttributeId2(ih,"FRAMEHORIZCOLOR", flin, IUP_INVALID_ID, NULL);
  if (fcol) IupSetAttributeId2(ih,"FRAMEVERTCOLOR", IUP_INVALID_ID, fcol, NULL);

  if (!freeze)
  {
    IupSetAttribute(ih,"NUMLIN_NOSCROLL","0");
    IupSetAttribute(ih,"NUMCOL_NOSCROLL","0");
    IupSetAttribute(ih,"SHOW","1:1");
  }
  else
  {
    char* fzcolor = IupGetAttribute(ih, "FREEZECOLOR");

    IupSetInt(ih,"NUMLIN_NOSCROLL",lin);
    IupSetInt(ih,"NUMCOL_NOSCROLL",col);  

    IupSetStrAttributeId2(ih,"FRAMEHORIZCOLOR", lin, IUP_INVALID_ID, fzcolor);
    IupSetStrAttributeId2(ih,"FRAMEVERTCOLOR",IUP_INVALID_ID, col, fzcolor);
  }

  IupSetAttribute(ih,"REDRAW","ALL");
  return 1;  /* store freeze state */
}

static char* iMatrixExFileDlg(ImatExData* matex_data, int save, const char* filter, const char* info, const char* extfilter)
{
  Ihandle* dlg = IupFileDlg();

  IupSetAttribute(dlg,"DIALOGTYPE", save? "SAVE": "OPEN");
  IupSetAttribute(dlg,"TITLE","Export Table");
  IupSetStrAttribute(dlg,"FILTER", filter);
  IupSetStrAttribute(dlg,"FILTERINFO", info);
  IupSetStrAttribute(dlg,"EXTFILTER", extfilter);  /* Windows and GTK only, but more flexible */
  IupSetAttributeHandle(dlg,"PARENTDIALOG", IupGetDialog(matex_data->ih));

  IupPopup(dlg,IUP_CENTER,IUP_CENTER);
  if (IupGetInt(dlg,"STATUS")!=-1)
  {
    char* value = IupGetAttribute(dlg, "VALUE");
    value = iupStrReturnStr(value);
    IupDestroy(dlg);
    return value;
  }

  IupDestroy(dlg);
  return NULL;
}

static int iMatrixExItemExport_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  char *filter, *info, *extfilter, *filename;

  if (iupStrEqual(IupGetAttribute(ih_item, "TEXTFORMAT"), "LaTeX"))
  {
    filter = "*.tex";  /* Motif does not support EXTFILTER, so define both */
    info = "LaTeX file (table format)";
    extfilter = "LaTeX file (table format)|*.tex|All Files|*.*|";
  }
  else if (iupStrEqual(IupGetAttribute(ih_item, "TEXTFORMAT"), "HTML"))
  {
    filter = "*.html;*.htm";
    info = "HTML file (table format)";
    extfilter = "HTML file (table format)|*.html;*.htm|All Files|*.*|";
  }
  else
  {
    filter = "*.txt";
    info = "Text file";
    extfilter = "Text file|*.txt|All Files|*.*|";
  }

  filename = iMatrixExFileDlg(matex_data, 1, filter, info, extfilter);

  IupSetStrAttribute(matex_data->ih, "TEXTFORMAT", IupGetAttribute(ih_item, "TEXTFORMAT"));
  IupSetStrAttribute(matex_data->ih, "COPYFILE", filename);

  iMatrixListShowLastError(matex_data->ih);

  return IUP_DEFAULT;
}

static int iMatrixExItemImport_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  char *filter, *info, *extfilter, *filename;

  filter = "*.txt";
  info = "Text file";
  extfilter = "Text file|*.txt|All Files|*.*|";

  filename = iMatrixExFileDlg(matex_data, 0, filter, info, extfilter);

  IupSetStrAttribute(matex_data->ih, "PASTEFILE", filename);

  iMatrixListShowLastError(matex_data->ih);

  return IUP_DEFAULT;
}

static int iMatrixExItemCut_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  IupSetAttribute(matex_data->ih, "COPY", "MARKED");
  IupSetAttribute(matex_data->ih, "CLEARVALUE", "MARKED");
  iMatrixListShowLastError(matex_data->ih);
  return IUP_DEFAULT;
}

static int iMatrixExItemCopy_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  IupSetAttribute(matex_data->ih, "COPY", "MARKED");
  iMatrixListShowLastError(matex_data->ih);
  return IUP_DEFAULT;
}

static int iMatrixExItemPaste_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  if (IupGetAttribute(matex_data->ih, "MARKED"))
    IupSetAttribute(matex_data->ih, "PASTE", "MARKED");
  else
    IupSetAttribute(matex_data->ih, "PASTE", "FOCUS");
  iMatrixListShowLastError(matex_data->ih);
  return IUP_DEFAULT;
}

static int iMatrixExItemDel_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  IupSetAttribute(matex_data->ih, "CLEARVALUE", "MARKED");
  iMatrixListShowLastError(matex_data->ih);
  return IUP_DEFAULT;
}

static int iMatrixExItemCopyColTo_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  char* value = IupGetAttribute(ih_item, "COPYCOLTO");
  int lin, col;

  IupGetIntInt(ih_item, "MENUCONTEXT_CELL", &lin, &col);
  IupSetfAttribute(matex_data->ih, "FOCUS_CELL", "%d:%d", lin, col);

  if (iupStrEqual(value, "INTERVAL"))
  {
    char interval[200] = "";
    value = iupAttribGet(matex_data->ih, "_IUP_LAST_COPYCOL_INTERVAL");
    if (value) iupStrCopyN(interval, 200, value);

    if (IupGetParam(IupGetLanguageString("IUP_COPYTOINTERVALS"), NULL, NULL, "L1-L2,L3,L4-L5,... %s\n", interval, NULL))
    {
      IupSetStrAttributeId2(matex_data->ih, "COPYCOLTO", lin, col, interval);
      iupAttribSetStr(matex_data->ih, "_IUP_LAST_COPYCOL_INTERVAL", interval);
    }
  }
  else
    IupSetStrAttributeId2(matex_data->ih, "COPYCOLTO", lin, col, value);

  return IUP_DEFAULT;
}

static int iMatrixExItemUndo_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  IupSetAttribute(matex_data->ih, "UNDO", NULL);  /* 1 level */
  iMatrixListShowLastError(matex_data->ih);
  return IUP_DEFAULT;
}

static int iMatrixExItemRedo_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  IupSetAttribute(matex_data->ih, "REDO", NULL);  /* 1 level */
  iMatrixListShowLastError(matex_data->ih);
  return IUP_DEFAULT;
}

static int iMatrixExItemUndoList_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  if (!matex_data)  /* will be called also by the shortcut key */
   matex_data = (ImatExData*)iupAttribGet(ih_item, "_IUP_MATEX_DATA");
  iupMatrixExUndoShowDialog(matex_data);
  return IUP_DEFAULT;
}

static int iMatrixExItemFind_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  IupSetStrAttribute(matex_data->ih, "FOCUS_CELL", IupGetAttribute(ih_item, "MENUCONTEXT_CELL"));
  iupMatrixExFindShowDialog(matex_data);
  return IUP_DEFAULT;
}

static int iMatrixExItemGoTo_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  char cell[100] = "";
  char* value;

  if (!matex_data)  /* will be called also by the shortcut key */
   matex_data = (ImatExData*)iupAttribGet(ih_item, "_IUP_MATEX_DATA");

  value = iupAttribGet(matex_data->ih, "_IUP_LAST_GOTO_CELL");
  if (value) iupStrCopyN(cell, 100, value);

  if (IupGetParam(IupGetLanguageString("IUP_GOTO"), NULL, NULL, "L:C %s\n", cell, NULL))
  {
    IupSetStrAttribute(matex_data->ih, "SHOW", cell);
    IupSetStrAttribute(matex_data->ih, "FOCUS_CELL", cell);
    iupAttribSetStr(matex_data->ih, "_IUP_LAST_GOTO_CELL", cell);
  }

  return IUP_DEFAULT;
}

static int iMatrixExItemSort_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  IupSetStrAttribute(matex_data->ih, "FOCUS_CELL", IupGetAttribute(ih_item, "MENUCONTEXT_CELL"));
  iupMatrixExSortShowDialog(matex_data);
  return IUP_DEFAULT;
}

static int iMatrixExItemFreeze_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  int lin, col, flin, fcol;

  IupGetIntInt(ih_item, "MENUCONTEXT_CELL", &lin, &col);

  IupGetIntInt(matex_data->ih, "FREEZE", &flin, &fcol);
  if (lin!=flin || col!=fcol)
    IupSetfAttribute(matex_data->ih, "FREEZE", "%d:%d", lin, col);
  else
    IupSetAttribute(matex_data->ih, "FREEZE", NULL);
  return IUP_DEFAULT;
}

static int iMatrixExItemHideCol_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  int lin, col;

  IupGetIntInt(ih_item, "MENUCONTEXT_CELL", &lin, &col);

  IupSetAttributeId(matex_data->ih, "VISIBLECOL", col, "No");

  return IUP_DEFAULT;
}

static int iMatrixExItemShowCol_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  int col, num_col = IupGetInt(matex_data->ih, "NUMCOL");

  for(col = 1; col <= num_col; ++col)
  {
    if (!IupGetIntId(matex_data->ih, "VISIBLECOL", col))
      IupSetAttributeId(matex_data->ih, "VISIBLECOL", col, "Yes");
  }

  return IUP_DEFAULT;
}

static int iMatrixExItemHideLin_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  int lin, col;

  IupGetIntInt(ih_item, "MENUCONTEXT_CELL", &lin, &col);

  IupSetAttributeId(matex_data->ih, "VISIBLELIN", lin, "No");

  return IUP_DEFAULT;
}

static int iMatrixExItemShowLin_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  int lin, num_lin = IupGetInt(matex_data->ih, "NUMLIN");

  for(lin = 1; lin <= num_lin; ++lin)
  {
    if (!IupGetIntId(matex_data->ih, "VISIBLELIN", lin))
      IupSetAttributeId(matex_data->ih, "VISIBLELIN", lin, "Yes");
  }

  return IUP_DEFAULT;
}

static void iMatrixExInitUnitList(ImatExData* matex_data, int col, char* list_str, int old_unit)
{
  int i, count, len = 0;
  char* unit_name;

  count = IupGetIntId(matex_data->ih, "NUMERICUNITCOUNT", col);
  for (i=0; i<count; i++)
  {
    IupSetIntId(matex_data->ih, "NUMERICUNITSHOWNINDEX", col, i);
    unit_name = IupGetAttributeId(matex_data->ih, "NUMERICUNITSYMBOLSHOWN", col);
    len += sprintf(list_str+len, "%s|", unit_name);
  }

  IupSetIntId(matex_data->ih, "NUMERICUNITSHOWNINDEX", col, old_unit);
}

static int iMatrixExItemNumericUnits_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  int unit, decimals;
  int lin, col;
  char format[200], list_str[200] = "|";

  IupGetIntInt(ih_item, "MENUCONTEXT_CELL", &lin, &col);

  decimals = IupGetIntId(matex_data->ih, "NUMERICFORMATPRECISION", col);
  unit = IupGetIntId(matex_data->ih, "NUMERICUNITSHOWNINDEX", col);

  iMatrixExInitUnitList(matex_data, col, list_str+1, unit);

  sprintf(format, "_@IUP_UNITS%%l%s\n_@IUP_DECIMALS%%i[0]\n", list_str);

  if (IupGetParam("_@IUP_NUMERICUNITS", NULL, NULL, format, &unit, &decimals, NULL))
  {
    IupSetIntId(matex_data->ih, "NUMERICUNITSHOWNINDEX", col, unit);
    IupSetIntId(matex_data->ih, "NUMERICFORMATPRECISION", col, decimals);
    IupSetfAttribute(matex_data->ih, "REDRAW", "C%d", col);
  }

  return IUP_DEFAULT;
}

static int iMatrixExItemNumericDecimals_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  int decimals;
  int lin, col;

  IupGetIntInt(ih_item, "MENUCONTEXT_CELL", &lin, &col);

  decimals = IupGetIntId(matex_data->ih, "NUMERICFORMATPRECISION", col);

  if (IupGetParam("_@IUP_NUMERICDECIMALS", NULL, NULL, "_@IUP_DECIMALS%i[0]\n", &decimals, NULL))
  {
    IupSetIntId(matex_data->ih, "NUMERICFORMATPRECISION", col, decimals);
    IupSetfAttribute(matex_data->ih, "REDRAW", "C%d", col);
  }

  return IUP_DEFAULT;
}

static Ihandle* iMatrixExCreateMenuContext(Ihandle* ih, int lin, int col)
{
  int readonly = IupGetInt(ih, "READONLY");

  /************************** File ****************************/

  Ihandle* menu = IupMenu(
    IupSetAttributes(IupSubmenu("_@IUP_EXPORT",
      IupMenu(
        IupSetCallbacks(IupSetAttributes(IupItem("Txt..." , NULL), "TEXTFORMAT=TXT"),    "ACTION", iMatrixExItemExport_CB, NULL),
        IupSetCallbacks(IupSetAttributes(IupItem("LaTeX...", NULL), "TEXTFORMAT=LaTeX"), "ACTION", iMatrixExItemExport_CB, NULL),
        IupSetCallbacks(IupSetAttributes(IupItem("Html..." , NULL), "TEXTFORMAT=HTML"),  "ACTION", iMatrixExItemExport_CB, NULL),
        NULL)), "IMAGE=IUP_FileOpen"),
    NULL);

  if (!readonly)
  {
    IupAppend(menu, IupSetAttributes(IupSubmenu("_@IUP_IMPORT",
        IupMenu(
          IupSetCallbacks(IupItem("Txt...",  NULL), "ACTION", iMatrixExItemImport_CB, NULL),
          NULL)), "IMAGE=IUP_FileSave"));
  }

  IupAppend(menu, IupSeparator());

  /************************** Edit - Undo ****************************/

  if (!readonly)
  {
    Ihandle *undo, *redo, *undolist;
    IupAppend(menu, undo = IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_UNDO", NULL), "IMAGE=IUP_EditUndo"), "ACTION", iMatrixExItemUndo_CB, NULL));
    IupAppend(menu, redo = IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_REDO", NULL), "IMAGE=IUP_EditRedo"), "ACTION", iMatrixExItemRedo_CB, NULL));
    IupAppend(menu, undolist = IupSetCallbacks(IupItem("_@IUP_UNDOLISTDLG", NULL), "ACTION", iMatrixExItemUndoList_CB, NULL));

    if (!IupGetInt(ih, "UNDO"))
      IupSetAttribute(undo, "ACTIVE", "No");
    if (!IupGetInt(ih, "REDO"))
      IupSetAttribute(redo, "ACTIVE", "No");
    if (!IupGetInt(ih, "UNDO") && !IupGetInt(ih, "REDO"))
      IupSetAttribute(undolist, "ACTIVE", "No");

    IupAppend(menu, IupSeparator());
  }

  /************************** Edit - Clipboard ****************************/

  if (!readonly)
    IupAppend(menu, IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_CUT", NULL), "IMAGE=IUP_EditCut"),  "ACTION", iMatrixExItemCut_CB, NULL));
  IupAppend(menu, IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_COPY",  NULL), "IMAGE=IUP_EditCopy"), "ACTION", iMatrixExItemCopy_CB, NULL));
  if (!readonly)
  {
    IupAppend(menu, IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_PASTE", NULL), "IMAGE=IUP_EditPaste"), "ACTION", iMatrixExItemPaste_CB, NULL));
    IupAppend(menu, IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_ERASE", NULL), "IMAGE=IUP_EditErase"), "ACTION", iMatrixExItemDel_CB, NULL));
  }
  IupAppend(menu, IupSeparator());

  /************************** Edit - Find ****************************/

  IupAppend(menu, IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_FINDDLG", NULL), "IMAGE=IUP_EditFind"), "ACTION", iMatrixExItemFind_CB, NULL));
  IupAppend(menu, IupSetCallbacks(IupItem("_@IUP_GOTODLG", NULL), "ACTION", iMatrixExItemGoTo_CB, NULL));
  IupAppend(menu, IupSeparator());

  /************************** View ****************************/

  if (!readonly)
    IupAppend(menu, IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_SORTDLG", NULL), "IMAGE=IUP_ToolsSortAscend"), "ACTION", iMatrixExItemSort_CB, NULL));

  {
    int flin, fcol;
    IupGetIntInt(ih, "FREEZE", &flin, &fcol);
    if (lin!=flin || col!=fcol)
      IupAppend(menu, IupSetCallbacks(IupItem("_@IUP_FREEZE", NULL), "ACTION", iMatrixExItemFreeze_CB, NULL));
    else
      IupAppend(menu, IupSetCallbacks(IupItem("_@IUP_UNFREEZE", NULL), "ACTION", iMatrixExItemFreeze_CB, NULL));
  }

  IupAppend(menu, IupSubmenu("_@IUP_VISIBILITY",
    IupMenu(
      IupSetCallbacks(IupItem("_@IUP_HIDECOLUMN", NULL),        "ACTION", iMatrixExItemHideCol_CB, NULL),
      IupSetCallbacks(IupItem("_@IUP_SHOWHIDDENCOLUMNS", NULL), "ACTION", iMatrixExItemShowCol_CB, NULL),
      IupSetCallbacks(IupItem("_@IUP_HIDELINE", NULL),          "ACTION", iMatrixExItemHideLin_CB, NULL),
      IupSetCallbacks(IupItem("_@IUP_SHOWHIDDENLINES", NULL),   "ACTION", iMatrixExItemShowLin_CB, NULL),
    NULL)));

  if (IupGetAttributeId(ih, "NUMERICQUANTITY", col))
  {
    if (IupGetIntId(ih, "NUMERICQUANTITYINDEX", col))  /* not None */
      IupAppend(menu, IupSetCallbacks(IupItem("_@IUP_NUMERICUNITSDLG", NULL),   "ACTION", iMatrixExItemNumericUnits_CB, NULL));
    else
      IupAppend(menu, IupSetCallbacks(IupItem("_@IUP_NUMERICDECIMALSDLG", NULL),   "ACTION", iMatrixExItemNumericDecimals_CB, NULL));
  }

  /************************** Data ****************************/

  if (!readonly)
  {
    IupAppend(menu, IupSeparator());

    IupAppend(menu, IupSubmenu("_@IUP_COPYTOSAMECOLUMN",
        IupMenu(
          IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_ALLLINES"      , NULL),  "COPYCOLTO=ALL"),      "ACTION", iMatrixExItemCopyColTo_CB, NULL),     
          IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_HERETOTOP"    , NULL),   "COPYCOLTO=TOP"),      "ACTION", iMatrixExItemCopyColTo_CB, NULL),     
          IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_HERETOBOTTOM" , NULL),   "COPYCOLTO=BOTTOM"),   "ACTION", iMatrixExItemCopyColTo_CB, NULL),     
          IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_INTERVALDLG"    , NULL), "COPYCOLTO=INTERVAL"), "ACTION", iMatrixExItemCopyColTo_CB, NULL),     
          IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_SELECTEDLINES" , NULL),  "COPYCOLTO=MARKED"),   "ACTION", iMatrixExItemCopyColTo_CB, NULL),
          NULL)));
  }

  return menu;
}

static IFniiiis iMatrixOriginalButton_CB = NULL;

static int iMatrixExButton_CB(Ihandle* ih, int b, int press, int x, int y, char* r)
{
  if (iMatrixOriginalButton_CB(ih, b, press, x, y, r)==IUP_IGNORE)
    return IUP_IGNORE;

  if (b == IUP_BUTTON3 && press && IupGetInt(ih, "MENUCONTEXT"))
  {
    int pos = IupConvertXYToPos(ih, x, y);
    if (pos >= 0)
    {
      ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
      int lin, col;
      IFnnii menucontext_cb;
      Ihandle* menu;
      int sx, sy;

      IupTextConvertPosToLinCol(ih, pos, &lin, &col);

      menu = iMatrixExCreateMenuContext(ih, lin, col);
      IupSetAttribute(menu, "MATRIX_EX_DATA", (char*)matex_data);  /* do not use "_IUP_MATEX_DATA" to enable inheritance */
      IupSetfAttribute(menu, "MENUCONTEXT_CELL", "%d:%d", lin, col);

      menucontext_cb = (IFnnii)IupGetCallback(ih, "MENUCONTEXT_CB");
      if (menucontext_cb) menucontext_cb(ih, menu, lin, col);

      IupGetIntInt(ih, "SCREENPOSITION", &sx, &sy);
      IupPopup(menu, sx+x, sy+y);
      IupDestroy(menu);
    }
  }

  return IUP_DEFAULT;
}

static IFnii iMatrixOriginalKeyPress_CB = NULL;

static int iMatrixExKeyPress_CB(Ihandle* ih, int c, int press)
{
  if (press)
  {
    switch (c)
    {
    case K_cT: 
      if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "PORTUGUESE"))
        iMatrixExSelectAll(ih); 
      return IUP_CONTINUE;
    case K_cA: 
      if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "ENGLISH"))
        iMatrixExSelectAll(ih); 
      return IUP_CONTINUE;
    case K_cV: 
      if (IupGetAttribute(ih,"MARKED"))
        IupSetAttribute(ih, "PASTE", "MARKED");
      else
        IupSetAttribute(ih, "PASTE", "FOCUS");
      iMatrixListShowLastError(ih);
      return IUP_IGNORE;
    case K_cX: 
      IupSetAttribute(ih, "COPY", "MARKED");
      iMatrixListShowLastError(ih);
      IupSetAttribute(ih, "CLEARVALUE", "MARKED");
      return IUP_IGNORE;
    case K_cC: 
      IupSetAttribute(ih, "COPY", "MARKED");
      iMatrixListShowLastError(ih);
      return IUP_IGNORE;
    case K_cZ: 
      IupSetAttribute(ih, "UNDO", NULL);  /* 1 level */
      return IUP_IGNORE;
    case K_cR: 
      if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "PORTUGUESE"))
        IupSetAttribute(ih, "REDO", NULL);  /* 1 level */
      return IUP_IGNORE;
    case K_cY: 
      if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "ENGLISH"))
        IupSetAttribute(ih, "REDO", NULL);  /* 1 level */
      return IUP_IGNORE;
    case K_cU: 
      {
        iMatrixExItemUndoList_CB(ih);
        return IUP_IGNORE;
      }
    case K_F3: 
      {
        char* find = IupGetAttribute(ih, "FIND");
        if (find)
        {
          /* invert the direction if not a "forward" one */
          char* direction = iupAttribGet(ih, "FINDDIRECTION");
          if (iupStrEqualNoCase(direction, "LEFTTOP"))
            iupAttribSet(ih, "FINDDIRECTION", "RIGHTBOTTOM");
          else if (iupStrEqualNoCase(direction, "TOPLEFT"))
            iupAttribSet(ih, "FINDDIRECTION", "BOTTOMRIGHT");

          IupSetAttribute(ih, "FIND", find);
        }
        return IUP_IGNORE;
      }
    case K_sF3: 
      {
        char* find = IupGetAttribute(ih, "FIND");
        if (find)
        {
          /* invert the direction if not a "reverse" one */
          char* direction = iupAttribGet(ih, "FINDDIRECTION");
          if (iupStrEqualNoCase(direction, "RIGHTBOTTOM"))
            iupAttribSet(ih, "FINDDIRECTION", "LEFTTOP");
          else if (iupStrEqualNoCase(direction, "BOTTOMRIGHT"))
            iupAttribSet(ih, "FINDDIRECTION", "TOPLEFT");

          IupSetAttribute(ih, "FIND", find);
        }
        return IUP_IGNORE;
      }
    case K_cL: 
      if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "PORTUGUESE"))
      {
        ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
        iupMatrixExFindShowDialog(matex_data);
        return IUP_IGNORE;
      }
      return IUP_CONTINUE;
    case K_cF: 
      if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "ENGLISH"))
      {
        ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
        iupMatrixExFindShowDialog(matex_data);
        return IUP_IGNORE;
      }
      return IUP_CONTINUE;
    case K_mF3: 
      {
        ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
        iupMatrixExFindShowDialog(matex_data);
        return IUP_IGNORE;
      }
    case K_cG: 
      {
        iMatrixExItemGoTo_CB(ih);
        return IUP_IGNORE;
      }
    case K_ESC: 
      {
        ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
        if (matex_data->find_dlg)
          IupHide(matex_data->find_dlg);
        return IUP_CONTINUE;
      }
    }
  }

  return iMatrixOriginalKeyPress_CB(ih, c, press);
}

static int iMatrixExCreateMethod(Ihandle* ih, void **params)
{
  ImatExData* matex_data = (ImatExData*)malloc(sizeof(ImatExData));
  memset(matex_data, 0, sizeof(ImatExData));

  iupAttribSet(ih, "_IUP_MATEX_DATA", (char*)matex_data);
  matex_data->ih = ih;

  if (!iMatrixOriginalKeyPress_CB) iMatrixOriginalKeyPress_CB = (IFnii)IupGetCallback(ih, "KEYPRESS_CB");
  IupSetCallback(ih, "KEYPRESS_CB", (Icallback)iMatrixExKeyPress_CB);

  if (!iMatrixOriginalButton_CB) iMatrixOriginalButton_CB = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  IupSetCallback(ih, "BUTTON_CB", (Icallback)iMatrixExButton_CB);

  (void)params;
  return IUP_NOERROR;
}

static void iMatrixExDestroyMethod(Ihandle* ih)
{
  ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");

  if (matex_data->busy_progress_dlg)
    IupDestroy(matex_data->busy_progress_dlg);

  if (matex_data->find_dlg)
    IupDestroy(matex_data->find_dlg);

  if (matex_data->undo_stack)
  {
    iupAttribSetClassObject(ih, "UNDOCLEAR", NULL);
    iupArrayDestroy(matex_data->undo_stack);
  }

  free(matex_data);
}

static void iMatrixExInitAttribCb(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "FREEZE", NULL, iMatrixExSetFreezeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FREEZECOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "0 0 255", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterCallback(ic, "MENUCONTEXT_CB", "nii");
  iupClassRegisterAttribute(ic, "MENUCONTEXT", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NO_INHERIT);

  iupMatrixExRegisterClipboard(ic);
  iupMatrixExRegisterBusy(ic);
  iupMatrixExRegisterVisible(ic);
  iupMatrixExRegisterExport(ic);
  iupMatrixExRegisterCopy(ic);
  iupMatrixExRegisterUnits(ic);
  iupMatrixExRegisterUndo(ic);
  iupMatrixExRegisterFind(ic);
  iupMatrixExRegisterSort(ic);

  if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "ENGLISH"))
  {
    IupSetLanguageString("IUP_EXPORT", "Export");
    IupSetLanguageString("IUP_IMPORT", "Import");
    IupSetLanguageString("IUP_UNDO", "Undo\tCtrl+Z");
    IupSetLanguageString("IUP_REDO", "Redo\tCtrl+Y");
    IupSetLanguageString("IUP_UNDOLISTDLG", "Undo List...\tCtrl+U");
    IupSetLanguageString("IUP_UNDOLIST", "Undo List");
    IupSetLanguageString("IUP_CURRENTSTATE", "Current State");
    IupSetLanguageString("IUP_CUT", "Cut\tCtrl+X");
    IupSetLanguageString("IUP_COPY", "Copy\tCtrl+C");
    IupSetLanguageString("IUP_PASTE", "Paste\tCtrl+V");
    IupSetLanguageString("IUP_ERASE", "Erase\tDel");
    IupSetLanguageString("IUP_FINDDLG", "Find...\tCtrl+F");
    IupSetLanguageString("IUP_GOTODLG", "Go To...\tCtrl+G");
    IupSetLanguageString("IUP_SORTDLG", "Sort...");
    IupSetLanguageString("IUP_FREEZE", "Freeze");
    IupSetLanguageString("IUP_UNFREEZE", "Unfreeze");
    IupSetLanguageString("IUP_COPYTOSAMECOLUMN", "Copy To (Same Column)");
    IupSetLanguageString("IUP_ALLLINES", "All lines");
    IupSetLanguageString("IUP_HERETOTOP", "Here to top");
    IupSetLanguageString("IUP_HERETOBOTTOM", "Here to bottom");
    IupSetLanguageString("IUP_INTERVALDLG", "Interval...");
    IupSetLanguageString("IUP_SELECTEDLINES", "Selected lines");

    IupSetLanguageString("IUP_VISIBILITY", "Visibility");
    IupSetLanguageString("IUP_HIDECOLUMN", "Hide Column");  
    IupSetLanguageString("IUP_SHOWHIDDENCOLUMNS", "Show Hidden Columns");
    IupSetLanguageString("IUP_HIDELINE", "Hide Line");    
    IupSetLanguageString("IUP_SHOWHIDDENLINES", "Show Hidden Lines");

    IupSetLanguageString("IUP_COPYTOINTERVALS", "Copy To - Intervals");
    IupSetLanguageString("IUP_GOTO", "Go To");

    IupSetLanguageString("IUP_DECIMALS", "Decimals");
    IupSetLanguageString("IUP_NUMERICDECIMALS", "Numeric Decimals");
    IupSetLanguageString("IUP_NUMERICDECIMALSDLG", "Numeric Decimals...");
    IupSetLanguageString("IUP_NUMERICUNITS", "Numeric Units");
    IupSetLanguageString("IUP_NUMERICUNITSDLG", "Numeric Units...");
    IupSetLanguageString("IUP_UNITS", "Units");

    IupSetLanguageString("IUP_ERRORINVALIDSELECTION", "Invalid Selection.");
    IupSetLanguageString("IUP_ERRORNOTEXT", "Empty Text.");
    IupSetLanguageString("IUP_ERRORINVALIDDATA", "Invalid Data.");
    IupSetLanguageString("IUP_ERRORNOSELECTION", "Empty Selection.");
    IupSetLanguageString("IUP_ERRORINVALIDINTERVAL", "Invalid Interval.");
    IupSetLanguageString("IUP_ERRORFILEOPEN", "Failed to open file.");
  }
  else if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "PORTUGUESE"))
  {
    IupSetLanguageString("IUP_EXPORT", "Exportar");
    IupSetLanguageString("IUP_IMPORT", "Importar");
    IupSetLanguageString("IUP_UNDO", "Desfazer\tCtrl+Z");
    IupSetLanguageString("IUP_REDO", "Refazer\tCtrl+R");
    IupSetLanguageString("IUP_UNDOLISTDLG", "Lista de Desfazer...\tCtrl+U");
    IupSetLanguageString("IUP_UNDOLIST", "Lista de Desfazer");
    IupSetLanguageString("IUP_CURRENTSTATE", "Estado Corrente");
    IupSetLanguageString("IUP_CUT", "Recortar\tCtrl+X");
    IupSetLanguageString("IUP_COPY", "Copiar\tCtrl+C");
    IupSetLanguageString("IUP_PASTE", "Colar\tCtrl+V");
    IupSetLanguageString("IUP_ERASE", "Apagar\tDel");
    IupSetLanguageString("IUP_FINDDLG", "Localizar...\tCtrl+L");
    IupSetLanguageString("IUP_GOTODLG", "Ir Para...\tCtrl+G");
    IupSetLanguageString("IUP_SORTDLG", "Classificar...");
    IupSetLanguageString("IUP_FREEZE", "Congelar");
    IupSetLanguageString("IUP_UNFREEZE", "Descongelar");
    IupSetLanguageString("IUP_COPYTOSAMECOLUMN", "Copiar Para (Mesma Coluna)");
    IupSetLanguageString("IUP_ALLLINES", "Todas as linhas");
    IupSetLanguageString("IUP_HERETOTOP", "Daqui para o topo");
    IupSetLanguageString("IUP_HERETOBOTTOM", "Daqui para o fim");
    IupSetLanguageString("IUP_INTERVALDLG", "Intervalo...");
    IupSetLanguageString("IUP_SELECTEDLINES", "Linhas Selecionadas");

    IupSetLanguageString("IUP_VISIBILITY", "Visibilidade");
    IupSetLanguageString("IUP_HIDECOLUMN", "Esconder Coluna");  
    IupSetLanguageString("IUP_SHOWHIDDENCOLUMNS", "Mostrar Coluna Escondidas");
    IupSetLanguageString("IUP_HIDELINE", "Esconder Linha");    
    IupSetLanguageString("IUP_SHOWHIDDENLINES", "Mostrar Linhas Escondidas");

    IupSetLanguageString("IUP_DECIMALS", "Decimais");
    IupSetLanguageString("IUP_NUMERICDECIMALS", "Número de Decimais");
    IupSetLanguageString("IUP_NUMERICDECIMALSDLG", "Número de Decimais...");
    IupSetLanguageString("IUP_UNITS", "Unidades");
    IupSetLanguageString("IUP_NUMERICUNITS", "Unidades Numéricas");
    IupSetLanguageString("IUP_NUMERICUNITSDLG", "Unidades Numéricas...");

    IupSetLanguageString("IUP_COPYTOINTERVALS", "Copiar Para - Intervalos");
    IupSetLanguageString("IUP_GOTO", "Ir Para");

    IupSetLanguageString("IUP_ERRORINVALIDSELECTION", "Seleção inválida.");
    IupSetLanguageString("IUP_ERRORNOTEXT", "Texto vazio.");
    IupSetLanguageString("IUP_ERRORINVALIDDATA", "Dado inválido.");
    IupSetLanguageString("IUP_ERRORNOSELECTION", "Seleção vazia.");
    IupSetLanguageString("IUP_ERRORINVALIDINTERVAL", "Intervalo inválido.");
    IupSetLanguageString("IUP_ERRORFILEOPEN", "Falha ao abrir o arquivo..");

    if (IupGetInt(NULL, "UTF8MODE"))
    {
      /* When seeing this file assuming ISO8859-1 encoding, above will appear correct.
         When seeing this file assuming UTF-8 encoding, bellow will appear correct. */

      IupSetLanguageString("IUP_ERRORINVALIDSELECTION", "SeleÃ§Ã£o invÃ¡lida.");
      IupSetLanguageString("IUP_ERRORINVALIDDATA", "Dado invÃ¡lido.");
      IupSetLanguageString("IUP_ERRORNOSELECTION", "SeleÃ§Ã£o vazia.");
      IupSetLanguageString("IUP_ERRORINVALIDINTERVAL", "Intervalo invÃ¡lido.");
    }
  }
}

static Iclass* iMatrixExNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("matrix"));

  ic->name = "matrixex";
  ic->format = "";
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 2;   /* has attributes with IDs that must be parsed */

  /* Class functions */
  ic->New = iMatrixExNewClass;
  ic->Create  = iMatrixExCreateMethod;
  ic->Destroy  = iMatrixExDestroyMethod;
  
  iMatrixExInitAttribCb(ic);

  return ic;
}

void IupMatrixExInit(Ihandle* ih)
{
  if (ih->iclass->nativetype != IUP_TYPECANVAS || 
      !IupClassMatch(ih, "matrix"))
    return;

  iMatrixExCreateMethod(ih, NULL);
  IupSetCallback(ih, "DESTROY_CB", (Icallback)iMatrixExDestroyMethod);
    
  iMatrixExInitAttribCb(ih->iclass);
}

void IupMatrixExOpen(void)
{
  if (!IupGetGlobal("_IUP_MATRIXEX_OPEN"))
  {
    iupRegisterClass(iMatrixExNewClass());
    IupSetGlobal("_IUP_MATRIXEX_OPEN", "1");
  }
}

Ihandle* IupMatrixEx(void)
{
  return IupCreate("matrixex");
}

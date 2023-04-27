#if 0
/* To check for memory leaks */
#define VLD_MAX_DATA_DUMP 80
#include <vld.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <iup.h>
#include <iupim.h>
#include <iupcbs.h>
#include <iup_scintilla.h>
#include <iup_config.h>
#include <iupcontrols.h>

/* #define USE_NO_OPENGL */
/* #define USE_NO_WEB */
/* #define USE_NO_PLOT */

#ifndef USE_NO_OPENGL  
#include <iupgl.h>
#include <iupglcontrols.h>
#endif  
#ifndef USE_NO_WEB
#include <iupweb.h>
#endif  
#ifndef USE_NO_PLOT
#include <iup_plot.h>
#endif  


#if _MSC_VER > 1900 /* Visual Studio 2017 and newer */
#define USE_UTF8_VC
#include <locale.h>
#endif  

#include "iup_str.h"
#include "iup_object.h"
#include "iup_ledlex.h"
#include "iup_attrib.h"
#include "iup_image.h"
#include "iup_dlglist.h"
#include "iup_register.h"
#include "iup_layout.h"
#include "iup_array.h"
#include "iup_linefile.h"


void vLedLoadImages(void);

Ihandle* vLedImageEditorCreate(Ihandle* parent, Ihandle *config);
void vLedImageEditorNewImage(Ihandle* dlg, const char* filename);
void vLedImageEditorSetImage(Ihandle* dlg, Ihandle* iup_image, const char* name);


#define VLED_FOLDING_MARGIN "20"

#define vled_isprint(_c) ((_c) > 31 && (_c) < 127 && _c != 40 && _c != '(' && _c != ')' && _c != '[' && _c != ']' && _c != ',')

enum {
  FIND_TYPE = 0,
  FIND_HANDLE = 1,
  FIND_NAME = 2,
  FIND_TITLE = 3,
  FIND_ATTRIBUTE = 4
};


static int compare_image_names(const void* i1, const void* i2)
{
  Ihandle* ih1 = *((Ihandle**)i1);
  Ihandle* ih2 = *((Ihandle**)i2);
  char* name1 = IupGetName(ih1);
  char* name2 = IupGetName(ih2);
  return strcmp(name1, name2);
}

static char* vLedGetName(Ihandle* ih)
{
  char* name = IupGetName(ih);
  if (name && iupStrEqualPartial(name, "_IUP_NAME"))  /* ignore internal names */
    return NULL;
  return name;
}

static int compare_named_handles(const void* i1, const void* i2)
{
  Ihandle* ih1 = *((Ihandle**)i1);
  Ihandle* ih2 = *((Ihandle**)i2);
  int line1 = iupAttribGetInt(ih1, "_IUPLED_LINE");
  int line2 = iupAttribGetInt(ih2, "_IUPLED_LINE");
  return line1-line2;
}

static const char* get_img_fileformat(void)
{
#define NUM_FORMATS 4
  int ret, count = NUM_FORMATS;
  static const char *options[NUM_FORMATS] = {
    "ICO",
    "BMP",
    "GIF",
    "PNG"
  };

  ret = IupListDialog(1, "Image File Format", count, options, 1, 9, count + 1, NULL);
  if (ret == -1)
    return NULL;
  else
    return options[ret];
}

static Ihandle* get_config(Ihandle* ih)
{
  Ihandle* config = (Ihandle*)IupGetAttribute(IupGetDialog(ih), "CONFIG_HANDLE");
  return config;
}

static char* getfolder(Ihandle* ih)
{
  static char folder[10240];
  Ihandle *filedlg = IupFileDlg();
  Ihandle* config = get_config(ih);
  const char* dir = IupConfigGetVariableStr(config, "IupVisualLED", "LastExportDirectory");
  if (!dir) dir = IupConfigGetVariableStr(config, "IupVisualLED", "LastDirectory");

  IupSetAttribute(filedlg, "DIALOGTYPE", "DIR");
  IupSetStrAttribute(filedlg, "DIRECTORY", dir);
  IupSetAttributeHandle(filedlg, "PARENTDIALOG", IupGetDialog(ih));
  IupSetAttribute(filedlg, "TITLE", "Select Folder for Files");

  IupPopup(filedlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

  if (IupGetInt(filedlg, "STATUS") != -1)
  {
    strcpy(folder, IupGetAttribute(filedlg, "VALUE"));
    IupDestroy(filedlg);
    return folder;
  }

  IupDestroy(filedlg);
  return NULL;
}

static void replaceDot(char* filename)
{
  /* replace all "." by "_" */
  /* replace all "-" by "_" */
  while (*filename)
  {
    if (*filename == '.')
      *filename = '_';
    if (*filename == '-')
      *filename = '_';

    filename++;
  }
}

static char* strdup_free(const char* str, char* str_ptr)
{
  int len = (int)strlen(str);
  char* tmp = malloc(len + 1);
  memcpy(tmp, str, len + 1);
  free(str_ptr);
  return tmp;
}

static char* strGetFileTitle(const char* filename)
{
  int i, last = 1, len = (int)strlen(filename);
  char* file_title = malloc(len + 1);
  char* dot, *ft_str = file_title;

  memcpy(file_title, filename, len + 1);

  dot = strchr(file_title, '.');
  if (dot) *dot = 0;

  for (i = len - 1; i >= 0; i--)
  {
    if (last && file_title[i] == '.')
    {
      /* cut last "." forward */
      file_title[i] = 0;
      last = 0;
    }

    if (file_title[i] == '\\' || file_title[i] == '/')
    {
      replaceDot(file_title + i + 1);
      return strdup_free(file_title + i + 1, ft_str);
    }
  }

  replaceDot(file_title);
  return strdup_free(file_title, ft_str);
}

static Ihandle* vLedGetCurrentMultitext(Ihandle* ih)
{
  Ihandle* tabs = IupGetDialogChild(ih, "MULTITEXT_TABS");
  return (Ihandle*)IupGetAttribute(tabs, "VALUE_HANDLE");
}

static int LoadImageFile(Ihandle* ih_item, const char* img_filename)
{
  Ihandle* currMultitext = vLedGetCurrentMultitext(ih_item);

  Ihandle* new_image = IupLoadImage(img_filename);

  if (new_image)
  {
    int lin, col, pos, i, n = 0;
    char *buffer;
    char* img_file_title = strGetFileTitle(img_filename);
    char name[80];

    for (i = 0; img_file_title[i] != 0; i++)
    {
      if (!vled_isprint(img_file_title[i]))
        continue;
      if (img_file_title[i] == ' ')
        name[n] = '_';
      else
        name[n] = img_file_title[i];
      n++;
    }
    name[n] = 0;

    IupSetHandle(img_file_title, new_image);

    iupImageExportToString(new_image, &buffer, "LED", name);

    pos = IupGetInt(currMultitext, "CARETPOS");
    IupTextConvertPosToLinCol(currMultitext, pos, &lin, &col);

    IupTextConvertLinColToPos(currMultitext, lin, 0, &pos);

    IupSetAttribute(currMultitext, "UNDOACTION", "BEGIN");

    IupSetAttributeId(currMultitext, "INSERT", pos, buffer);

    IupSetAttribute(currMultitext, "UNDOACTION", "END");

    free(img_file_title);
    free(buffer);
    IupDestroy(new_image);
    return 1;
  }
  else
  {
    char* err_msg = IupGetGlobal("IUPIM_LASTERROR");
    if (err_msg)
      IupMessageError(IupGetDialog(ih_item), err_msg);
    return 0;
  }
}

static int vLedIsAlien(Ihandle *elem, const char* filename)
{
  char *elem_file;

  if (!elem || !filename)
    return 0;

  elem_file = iupAttribGet(elem, "_IUPLED_FILENAME");

  if (!elem_file || !iupStrEqual(elem_file, filename))
    return 1;

  return 0;
}

static char* vLedGetElementTreeTitle(Ihandle* ih)
{
  char* title = iupAttribGetLocal(ih, "TITLE");
  char* str = iupStrGetMemory(200);
  char* name = vLedGetName(ih);
  char* not_def = NULL;

  if (IupClassMatch(ih, "user") && iupAttribGet(ih, "_IUPLED_NOTDEF_NAME"))
  {
    name = iupAttribGet(ih, "_IUPLED_NOTDEF_NAME");
    not_def = " - not defined";
  }

  if (title)
  {
    char buffer[51];

    if (iupStrLineCount(title, (int)strlen(title)) > 1)
    {
      int len;
      iupStrNextLine(title, &len); /* get the size of the first line */
      if (len > 50) len = 50;
      iupStrCopyN(buffer, len + 1, title);
      title = &buffer[0];
    }

    if (name)
      sprintf(str, "[%s] \"%.50s\" (%.50s)%s", IupGetClassName(ih), title, name, not_def? not_def: "");
    else
      sprintf(str, "[%s] \"%.50s\"", IupGetClassName(ih), title);
  }
  else
  {
    if (name)
      sprintf(str, "[%s] (%.50s)%s", IupGetClassName(ih), name, not_def ? not_def : "");
    else
      sprintf(str, "[%s]", IupGetClassName(ih));
  }

  if (ih->iclass->nativetype == IUP_TYPEIMAGE)
    sprintf(str + strlen(str), " - %s", IupGetAttribute(ih, "RASTERSIZE"));

  return str;
}

static void vLedTreeSetNodeInfo(Ihandle* elem_tree, int id, Ihandle* ih, int link)
{
  IupSetAttributeId(elem_tree, "TITLE", id, vLedGetElementTreeTitle(ih));
  IupTreeSetUserId(elem_tree, id, ih);

  if (link)
  {
    IupSetAttributeId(elem_tree, "IMAGE", id, "IupVLedShortcut");
    IupSetAttributeId(elem_tree, "LINK", id, "1");
  }
}

static int vLedTreeAddNode(Ihandle* elem_tree, int id, Ihandle* ih, const char *filename, int add, int root)
{
  int link = 0;

  if (vLedIsAlien(ih, filename) || iupAttribGet(ih, "_IUPLED_NOTDEF_NAME") || (vLedGetName(ih) && !root))
    link = 1;

  if (ih->iclass->childtype != IUP_CHILDNONE && !iupAttribGet(ih, "_IUPLED_NOTDEF_NAME") && !link)
  {
    if (add)
      IupSetAttributeId(elem_tree, "ADDBRANCH", id, "");
    else
      IupSetAttributeId(elem_tree, "INSERTBRANCH", id, "");
  }
  else
  {
    if (add)
      IupSetAttributeId(elem_tree, "ADDLEAF", id, "");
    else
      IupSetAttributeId(elem_tree, "INSERTLEAF", id, "");
  }

  id = IupGetInt(elem_tree, "LASTADDNODE");

  vLedTreeSetNodeInfo(elem_tree, id, ih, link);

  return id;
}

static int vLedTreeAddChildren(Ihandle* elem_tree, int last_child_id, Ihandle* parent, const char *filename)
{
  Ihandle *child;
  int add = 1;  /* add only for the first element */

  for (child = parent->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_INTERNAL)) /* only non-internal */
    {
      last_child_id = vLedTreeAddNode(elem_tree, last_child_id, child, filename, add, 0);  /* add element node or a link */

      if (child->iclass->childtype != IUP_CHILDNONE && !vLedIsAlien(child, filename) && !vLedGetName(child))  /* add its children if not a link and not named */
        vLedTreeAddChildren(elem_tree, last_child_id, child, filename);

      add = 0; /* use insert */
    }
  }

  return last_child_id;
}

static void enable_elem_tools_buttons(Ihandle *elem_tree, int active, Ihandle* elem)
{
  /* elements tools buttons */
  Ihandle *bt;

  IupSetAttribute(IupGetBrother(IupGetParent(elem_tree)), "ACTIVE", active? "YES": "NO");

  bt = IupGetDialogChild(elem_tree, "ELEM_LAYOUT_BUTTON");
  if (elem && IupGetDialog(elem))
    IupSetAttribute(bt, "ACTIVE", "Yes");
  else
    IupSetAttribute(bt, "ACTIVE", "No");

  bt = IupGetDialogChild(elem_tree, "ELEM_IMAGE_BUTTON");
  if (elem && elem->iclass->nativetype == IUP_TYPEIMAGE)
    IupSetAttribute(bt, "ACTIVE", "Yes");
  else
    IupSetAttribute(bt, "ACTIVE", "No");
}

static void updateElemTree(Ihandle* elem_tree, const char* filename)
{
  int last_child_id = -1;
  int i, j, num_names = IupGetAllNames(NULL, -1);
  char* *names = malloc(sizeof(char*)*num_names);
  int num_named_handles = 0;
  Ihandle* *named_handles;
  
  IupGetAllNames(names, num_names);

  IupSetAttribute(elem_tree, "DELNODE0", "ALL");
  enable_elem_tools_buttons(elem_tree, 0, NULL);

  for (i = 0; i < num_names; i++)
  {
    Ihandle* elem = IupGetHandle(names[i]);
    if (iupObjectCheck(elem))
    {
      if (vLedIsAlien(elem, filename) || iupStrEqualPartial(names[i], "_IUP_NAME"))
        names[i] = NULL;
      else
        num_named_handles++;
    }
    else
      names[i] = NULL;
  }

  if (num_named_handles == 0)
    return;

  named_handles = malloc(sizeof(Ihandle*)*num_named_handles);

  j = 0;
  for (i = 0; i < num_names; i++)
  {
    if (names[i])
    {
      Ihandle* elem = IupGetHandle(names[i]);
      named_handles[j] = elem;
      j++;
    }
  }

  qsort(named_handles, num_named_handles, sizeof(Ihandle*), compare_named_handles);

  for (i = 0; i < num_named_handles; i++)
  {
    Ihandle* elem = named_handles[i];

    last_child_id = vLedTreeAddNode(elem_tree, last_child_id, elem, filename, 0, 1);  /* here use insert always */

    if (elem->iclass->childtype != IUP_CHILDNONE)
      vLedTreeAddChildren(elem_tree, last_child_id, elem, filename);
  }

//  IupSetAttribute(elem_tree, "VALUE", "1");
  IupSetAttribute(elem_tree, "VALUE", "0");

  enable_elem_tools_buttons(elem_tree, 1, (Ihandle*)IupTreeGetUserId(elem_tree, 0));

  free(names);
}

static char* load_comments(const char* filename)
{
  if (filename)
  {
    IlineFile* line_file = iupLineFileOpen(filename);
    if (line_file)
    {
      Iarray* str_array = iupArrayCreate(1024, sizeof(char));  /* just set an initial size, but count is 0 */
      char* comments;
      int count, has_comment = 0;

      do
      {
        const char* line_buffer;
        int i = 0;

        int line_len = iupLineFileReadLine(line_file);
        if (line_len == -1)
          break;

        line_buffer = iupLineFileGetBuffer(line_file);

        while (line_buffer[i] == ' ' || line_buffer[i] == '\t') /* skip spaces and tabs */
          i++;

        if (line_buffer[i] == '#')
          has_comment = 1;
        else if (line_buffer[i] != 0)
          break;

        count = iupArrayCount(str_array);
        comments = iupArrayAdd(str_array, line_len + 1);
        if (line_len)
          memcpy(comments + count, line_buffer, line_len);
        comments[count + line_len] = '\n';

      } while (!iupLineFileEOF(line_file));

      count = iupArrayCount(str_array);
      if (has_comment && count != 0)
      {
        comments = iupArrayReleaseData(str_array);
        comments[count] = 0; /* replace the least '\n' by terminator */
      }
      else
        comments = NULL;

      iupArrayDestroy(str_array);
      iupLineFileClose(line_file);
      return comments;
    }
  }
  return NULL;
}

static int unload_led(Ihandle* multitext, const char *filename);

static int load_led(Ihandle *elem_tree, const char *filename, int use_buffer)
{
  Ihandle* multitext = vLedGetCurrentMultitext(elem_tree);
  const char* error;

  IupSetInt(multitext, "MARKERDELETEALL", 1);

  if (use_buffer)
  {
    char* buffer = IupGetAttribute(multitext, "VALUE");
    error = iupLoadLed(filename, buffer, 1);  /* save info */
  }
  else
    error = iupLoadLed(filename, NULL, 1);  /* save info */

  if (error)
  {
    int line;
    const char* error_mark = "bad input at line";
    const char* error_line = strstr(error, error_mark);
    error_line += strlen(error_mark) + 1;
    sscanf(error_line, "%d", &line);    /* line in LED error starts at 1 */
    IupSetIntId(multitext, "MARKERADD", line-1, 1);
    IupSetStrf(multitext, "CARET", "%d,0", line-1);
    IupMessageError(IupGetDialog(elem_tree), error);
  }

  if (error)
  {
    unload_led(multitext, filename);
    IupSetAttribute(multitext, "LOADED", NULL);
  }
  else
  {
    char* comments = load_comments(filename);
    if (comments)
    {
      iupAttribSetStr(multitext, "_IUPLED_COMMENTS", comments);
      free(comments);
    }

    IupSetAttribute(multitext, "LOADED", "1");
    updateElemTree(elem_tree, filename);
  }

  return IUP_DEFAULT;
}

static void unloadNamedElements(Ihandle *elem)
{
  Ihandle *child;

  for (child = elem->firstchild; child; child = child->brother)
  {
    if (child->iclass->childtype != IUP_CHILDNONE)
      unloadNamedElements(child);

    if (vLedGetName(child))
      IupDetach(child);
  }
}

static void check_layout_prop_dialogs(Ihandle* ih)
{
  int i, count = iupDlgListCount();
  Ihandle* dlg;
  Ihandle* elem_tree_box = IupGetDialogChild(ih, "ELEM_TREE_BOX");

  dlg = (Ihandle*)IupGetAttribute(elem_tree_box, "PROPERTIES_DIALOG");
  if (iupObjectCheck(dlg)) /* properties dialog exists */
  {
    Ihandle* elem = (Ihandle*)iupAttribGet(dlg, "_IUP_PROPELEMENT");
    if (!iupObjectCheck(elem)) /* if elem is not valid anymore then hide the properties dialog */
      IupHide(dlg);
  }

  for (dlg = iupDlgListFirst(), i = 0; dlg && i < count; i++)
  {
    Ihandle* next_dlg = iupDlgListNext();

    if (iupAttribGet(dlg, "_IUP_LAYOUTDIALOG")) /* is a IupLayoutDialog */
    {
      Ihandle* dlg_elem = (Ihandle*)iupAttribGet(dlg, "_IUP_LAYOUTDIALOG_DIALOG");
      if (!iupObjectCheck(dlg_elem)) /* if dlg_elem is not valid anymore then destroy the layout dialog */
        IupDestroy(dlg);
    }

    dlg = next_dlg;
  }
}

static int unload_led(Ihandle* multitext, const char *filename)
{
  int i, num_named_elems = 0, num_names = IupGetAllNames(NULL, -1);
  char* *names = malloc(sizeof(char*)*num_names);
  Ihandle* *named_elems = malloc(sizeof(Ihandle*)*num_names);
  IupGetAllNames(names, num_names);

  for (i = 0; i < num_names; i++)
  {
    if (!iupStrEqualPartial(names[i], "_IUP_NAME"))
    {
      named_elems[num_named_elems] = IupGetHandle(names[i]);
      num_named_elems++;
    }
  }
  
  for (i = 0; i < num_named_elems; i++)
  {
    Ihandle *elem = named_elems[i],
           *parent, *brother = NULL;
    char old_name[100] = "";
    int save_not_defined = 0;

    if (!iupObjectCheck(elem))  /* it may already being destroyed in the hierarchy */
    {
      named_elems[i] = NULL;
      continue;
    }

    if (vLedIsAlien(elem, filename))
    {
      named_elems[i] = NULL;
      continue;
    }

    if (IupClassMatch(elem, "menu"))
    {
      parent = elem->parent;
      if (parent && IupClassMatch(parent, "dialog"))
        IupSetAttribute(parent, "MENU", NULL);
    }

    if (IupClassMatch(elem, "dialog"))
    {
      Ihandle *menu = (Ihandle *) IupGetAttribute(elem, "MENU");
      if (menu)
        IupSetAttribute(elem, "MENU", NULL);
    }

    if (elem->iclass->childtype != IUP_CHILDNONE)
      unloadNamedElements(elem);

    parent = elem->parent;

    brother = NULL;
    if (parent && vLedIsAlien(parent, filename))
    {
      strcpy(old_name, vLedGetName(elem));
      brother = elem->brother;
      save_not_defined = 1;
    }

    IupDetach(elem);

    if (save_not_defined)
    {
      Ihandle *user = IupUser();
      iupAttribSetStr(user, "_IUPLED_NOTDEF_NAME", old_name);
      IupInsert(parent, brother, user);
    }
  }

  for (i = 0; i < num_named_elems; i++)
  {
    Ihandle *elem = named_elems[i];
    if (iupObjectCheck(elem))
      IupDestroy(elem);
  }

  check_layout_prop_dialogs(multitext);

  free(names);
  free(named_elems);
  return IUP_DEFAULT;
}

static int item_iup_action_cb(void)
{
  IupHelp("http://www.tecgraf.puc-rio.br/iup");
  return IUP_DEFAULT;
}

static int item_help_action_cb(void)
{
  IupHelp("http://www.tecgraf.puc-rio.br/iup/index.html?url=iupvisualled.html");
  return IUP_DEFAULT;
}

static int item_about_action_cb(void)
{
  IupVersionShow();
  return IUP_DEFAULT;
}

static const char *getLastNonAlphaNumeric(const char *text)
{
  int len = (int)strlen(text);
  const char *c = text + len - 1;
  if (*c == '\n')
    c--;
  for (; c != text; c--)
  {
    if (*c < 48 || (*c > 57 && *c < 65) || (*c > 90 && *c < 97) || *c > 122)
      return c + 1;
  }
  return NULL;
}

static char *filterList(const char *text, const char *list)
{
  char *filteredList[1024]; /* maximum number of items */
  char *retList;
  int count = 0, text_len = (int)strlen(text);

  int i, len;
  const char *lastValue = list;
  const char *nextValue = iupStrNextValue(list, (int)strlen(list), &len, ' ');
  while (len != 0)
  {
    if (text_len <= len && iupStrEqualPartial(lastValue, text))
    {
      char *value = malloc(80);
      strncpy(value, lastValue, len);
      value[len] = 0;
      filteredList[count++] = value;
    }
    lastValue = nextValue;
    nextValue = iupStrNextValue(nextValue, (int)strlen(nextValue), &len, ' ');
  }

  retList = malloc(10240);
  retList[0] = 0;
  for (i = 0; i < count; i++)
  {
    if (i == 0)
    {
      strcpy(retList, filteredList[i]);
      strcat(retList, " ");
    }
    else
    {
      strcat(retList, filteredList[i]);
      strcat(retList, " ");
    }
  }

  for (i = 0; i < count; i++)
    free(filteredList[i]);

  return retList;
}

static int multitext_valuechanged_cb(Ihandle* multitext)
{
  if (IupGetInt(multitext, "AUTOCOMPLETION"))
  {
    const char *t;
    char *text = IupGetAttribute(multitext, "LINEVALUE");
    int col = IupGetInt2(multitext, "CARET");
    text[col + 1] = '\0';
    t = getLastNonAlphaNumeric(text);
    if (t != NULL && *t != '\n' && *t != 0)
    {
      Ihandle* ih = IupGetDialog(multitext);
      char* keywords = IupGetAttribute(ih, "LED_KEYWORDS");
      char *fList = filterList(t, keywords);
      if (strlen(fList) > 0)
        IupSetAttributeId(multitext, "AUTOCSHOW", (int)strlen(t) - 1, fList);
      free(fList);
    }

    return IUP_DEFAULT;
  }

  IupSetAttribute(multitext, "LOADED", NULL);

  return IUP_CONTINUE;
}

static int multitext_kesc_cb(Ihandle *multitext)
{
  if (!IupGetInt(multitext, "AUTOCOMPLETION"))
    return IUP_CONTINUE;

  if (IupGetInt(multitext, "AUTOCACTIVE"))
    IupSetAttribute(multitext, "AUTOCCANCEL", "YES");

  return IUP_CONTINUE;
}

static void set_keywords(Ihandle* multitext)
{
  Ihandle* ih = IupGetDialog(multitext);
  char* value = IupGetAttribute(ih, "LED_KEYWORDS");
  if (!value)
  {
    char keywords[2048] = "";
    char *p_keyw;
    int i, num_classes;
    char **list;

    num_classes = IupGetAllClasses(NULL, -1);
    list = (char **)malloc(num_classes * sizeof(char *));
    IupGetAllClasses(list, num_classes);

    p_keyw = &keywords[0];

    for (i = 0; i < num_classes; i++)
      p_keyw += sprintf(p_keyw, "%s ", list[i]);

    iupStrUpper(keywords, keywords);
    IupSetStrAttribute(ih, "LED_KEYWORDS", keywords);
    value = IupGetAttribute(ih, "LED_KEYWORDS");

    free(list);
  }

  IupSetStrAttribute(multitext, "KEYWORDS0", value);
}

static int multitext_map_cb(Ihandle* multitext)
{
  Ihandle* config = get_config(multitext);
  const char *value;

  IupSetAttribute(multitext, "LEXERLANGUAGE", "led");
  set_keywords(multitext);

  IupSetAttribute(multitext, "STYLEFGCOLOR1", "0 128 0");    /* 1-Led comment */
  IupSetAttribute(multitext, "STYLEFGCOLOR2", "255 128 0");  /* 2-Number  */
  IupSetAttribute(multitext, "STYLEFGCOLOR3", "0 0 255");    /* 3-Keyword  */
  IupSetAttribute(multitext, "STYLEFGCOLOR4", "164 0 164");  /* 4-String  */
  IupSetAttribute(multitext, "STYLEFGCOLOR5", "164 0 164");  /* 5-Character  */
  IupSetAttribute(multitext, "STYLEFGCOLOR6", "164 0 0");   /* 6-Operator  */
  /* 3, 8 and 9 - are not used */
  IupSetAttribute(multitext, "STYLEBOLD10", "YES");

  IupSetAttribute(multitext, "MARKERHIGHLIGHT", "YES");

  IupSetAttributeId(multitext, "MARKERBGCOLOR", 25, "0 0 0");         /* BGCOLOR para SC_MARKNUM_FOLDEREND */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 25, "255 255 255");   /* FGCOLOR para SC_MARKNUM_FOLDEREND */
  IupSetAttributeId(multitext, "MARKERBGCOLOR", 26, "0 0 0");         /* BGCOLOR para SC_MARKNUM_FOLDEROPENMID */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 26, "255 255 255");   /* FGCOLOR para SC_MARKNUM_FOLDEROPENMID */
  IupSetAttributeId(multitext, "MARKERBGCOLOR", 27, "0 0 0");         /* BGCOLOR para SC_MARKNUM_FOLDERMIDTAIL */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 27, "255 255 255");   /* FGCOLOR para SC_MARKNUM_FOLDERMIDTAIL */
  IupSetAttributeId(multitext, "MARKERBGCOLOR", 28, "0 0 0");         /* BGCOLOR para SC_MARKNUM_FOLDERTAIL */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 28, "255 255 255");   /* FGCOLOR para SC_MARKNUM_FOLDERTAIL */
  IupSetAttributeId(multitext, "MARKERBGCOLOR", 29, "0 0 0");         /* BGCOLOR para SC_MARKNUM_FOLDERSUB */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 29, "255 255 255");   /* FGCOLOR para SC_MARKNUM_FOLDERSUB */
  IupSetAttributeId(multitext, "MARKERBGCOLOR", 30, "0 0 0");         /* BGCOLOR para SC_MARKNUM_FOLDER */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 30, "255 255 255");   /* FGCOLOR para SC_MARKNUM_FOLDER */
  IupSetAttributeId(multitext, "MARKERBGCOLOR", 31, "0 0 0");         /* BGCOLOR para SC_MARKNUM_FOLDEROPEN */
  IupSetAttributeId(multitext, "MARKERFGCOLOR", 31, "255 255 255");   /* FGCOLOR para SC_MARKNUM_FOLDEROPEN */

  IupSetAttribute(multitext, "PROPERTY", "fold=1");
  IupSetAttribute(multitext, "PROPERTY", "fold.compact=0"); /* avoid folding of blank lines */
  IupSetAttribute(multitext, "_IUP_FOLDDING", "1");

  /* Folding margin=3 */
  IupSetAttribute(multitext, "MARGINWIDTH3", VLED_FOLDING_MARGIN);
  IupSetAttribute(multitext, "MARGINMASKFOLDERS3", "Yes");
  IupSetAttribute(multitext, "MARGINSENSITIVE3", "YES");

  IupSetAttribute(multitext, "MARKERDEFINE", "FOLDER=BOXPLUS");
  IupSetAttribute(multitext, "MARKERDEFINE", "FOLDEROPEN=BOXMINUS");
  IupSetAttribute(multitext, "MARKERDEFINE", "FOLDEREND=BOXPLUSCONNECTED");
  IupSetAttribute(multitext, "MARKERDEFINE", "FOLDERMIDTAIL=TCORNER");
  IupSetAttribute(multitext, "MARKERDEFINE", "FOLDEROPENMID=BOXMINUSCONNECTED");
  IupSetAttribute(multitext, "MARKERDEFINE", "FOLDERSUB=VLINE");
  IupSetAttribute(multitext, "MARKERDEFINE", "FOLDERTAIL=LCORNER");

  IupSetAttribute(multitext, "FOLDFLAGS", "LINEAFTER_CONTRACTED");

  value = IupConfigGetVariableStr(config, "IupVisualLED", "CommentColor");
  if (value)
    IupSetStrAttribute(multitext, "STYLEFGCOLOR1", value);
  else
    IupConfigSetVariableStr(config, "IupVisualLED", "CommentColor", IupGetAttribute(multitext, "STYLEFGCOLOR1"));

  value = IupConfigGetVariableStr(config, "IupVisualLED", "NumberColor");
  if (value)
    IupSetStrAttribute(multitext, "STYLEFGCOLOR2", value);
  else
    IupConfigSetVariableStr(config, "IupVisualLED", "NumberColor", IupGetAttribute(multitext, "STYLEFGCOLOR4"));

  value = IupConfigGetVariableStr(config, "IupVisualLED", "KeywordColor");
  if (value)
    IupSetStrAttribute(multitext, "STYLEFGCOLOR3", value);
  else
    IupConfigSetVariableStr(config, "IupVisualLED", "KeywordColor", IupGetAttribute(multitext, "STYLEFGCOLOR5"));

  value = IupConfigGetVariableStr(config, "IupVisualLED", "StringColor");
  if (value)
    IupSetStrAttribute(multitext, "STYLEFGCOLOR4", value);
  else
    IupConfigSetVariableStr(config, "IupVisualLED", "StringColor", IupGetAttribute(multitext, "STYLEFGCOLOR6"));

  value = IupConfigGetVariableStr(config, "IupVisualLED", "CharacterColor");
  if (value)
    IupSetStrAttribute(multitext, "STYLEFGCOLOR5", value);
  else
    IupConfigSetVariableStr(config, "IupVisualLED", "CharacterColor", IupGetAttribute(multitext, "STYLEFGCOLOR7"));

  value = IupConfigGetVariableStr(config, "IupVisualLED", "OperatorColor");
  if (value)
    IupSetStrAttribute(multitext, "STYLEFGCOLOR6", value);
  else
    IupConfigSetVariableStr(config, "IupVisualLED", "OperatorColor", IupGetAttribute(multitext, "STYLEFGCOLOR10"));

  value = IupConfigGetVariableStr(config, "IupVisualLED", "AutoCompletion");
  if (value)
    IupSetStrAttribute(multitext, "AUTOCOMPLETION", value);

  value = IupConfigGetVariableStr(config, "IupVisualLED", "Folding");
  if (iupStrBoolean(value))
  {
    IupSetAttribute(multitext, "MARGINWIDTH3", VLED_FOLDING_MARGIN);
    IupSetAttribute(multitext, "PROPERTY", "fold=1");
    IupSetAttribute(multitext, "_IUP_FOLDDING", "1");
    IupSetAttribute(multitext, "FOLDALL", "EXPAND");
  }
  else
  {
    IupSetAttribute(multitext, "MARGINWIDTH3", "0");
    IupSetAttribute(multitext, "PROPERTY", "fold=0");
    IupSetAttribute(multitext, "_IUP_FOLDDING", NULL);
  }

  return IUP_DEFAULT;
}

static Ihandle* vLedGetElemTree(Ihandle* multitext)
{
  Ihandle* elem_tree = (Ihandle*)IupGetAttribute(multitext, "ELEM_TREE");
  return elem_tree;
}

static int tabChange_cb(Ihandle* tabs, Ihandle* new_multitext, Ihandle* old_multitext)
{
  Ihandle* elem_tree_box = IupGetDialogChild(tabs, "ELEM_TREE_BOX");
  Ihandle* elem_tree = vLedGetElemTree(new_multitext);
  int tree_count = IupGetInt(elem_tree, "COUNT");

  IFnnn oldTabChange_cb = (IFnnn)IupGetCallback(tabs, "OLDTABCHANGE_CB");
  if (oldTabChange_cb)
    oldTabChange_cb(tabs, new_multitext, old_multitext);

  IupSetAttribute(elem_tree_box, "VALUE_HANDLE", (char*)elem_tree);

  enable_elem_tools_buttons(elem_tree, tree_count > 0, tree_count > 0? (Ihandle*)IupTreeGetUserId(elem_tree, IupGetInt(elem_tree, "VALUE")) : NULL);

  return IUP_DEFAULT;
}

static void reload_led_file(Ihandle* multitext)
{
  Ihandle* elem_tree = vLedGetElemTree(multitext);
  Ihandle* config = get_config(multitext);
  char* filename = IupGetAttribute(multitext, "FILENAME");  /* never NULL here */

  unload_led(multitext, filename);
  IupSetAttribute(multitext, "LOADED", NULL);

  if (IupConfigGetVariableIntDef(config, "IupVisualLED", "AutoLoad", 1))
    load_led(elem_tree, filename, 0);
  else
  {
    IupSetAttribute(elem_tree, "DELNODE0", "ALL");
    enable_elem_tools_buttons(elem_tree, 0, NULL);
  }
}

static int scidlg_newfilename_cb(Ihandle* main_dialog, char* old_filename, char* new_filename)
{
  int i, num_names = IupGetAllNames(NULL, -1);
  char* *names = malloc(sizeof(char*)*num_names);

  IupGetAllNames(names, num_names);

  for (i = 0; i < num_names; i++)
  {
    Ihandle* elem = IupGetHandle(names[i]);
    if (iupObjectCheck(elem))
    {
      if (!vLedIsAlien(elem, old_filename) && !iupStrEqualPartial(names[i], "_IUP_NAME"))
        iupAttribSetStr(elem, "_IUPLED_FILENAME", new_filename);
    }
  }

  free(names);

  (void)main_dialog;
  return IUP_DEFAULT;
}

static int scidlg_loadfile_cb(Ihandle* main_dialog, Ihandle* multitext)
{
  /* called after the file is loaded */

  reload_led_file(multitext);

  (void)main_dialog;
  return IUP_DEFAULT;
}

static int scidlg_savefile_cb(Ihandle* main_dialog, Ihandle* multitext)
{
  /* called after the file is saved */

  reload_led_file(multitext);

  (void)main_dialog;
  return IUP_DEFAULT;
}

static char* changeTabsForSpaces(const char *text, int tabsize)
{
  char *newText;
  int len = (int)strlen(text);
  int i, j, new_i = 0, tabs = 0;

  for (i = 0; i < len; i++)
  {
    if (text[i] == '\t')
      tabs++;
  }

  if (tabs == 0)
    return iupStrDup(text);

  newText = malloc(len + tabs * tabsize);

  for (i = 0; i < len; i++)
  {
    char c = text[i];
    if (c == '\t')
    {
      int nWhites = tabsize - (new_i % tabsize);

      for (j = 0; j < nWhites; j++)
      {
        newText[new_i] = ' ';
        new_i++;
      }
      continue;
    }

    newText[new_i] = c;
    new_i++;
  }

  newText[new_i] = '\0';

  return newText;
}

static Iclass* find_class_at_pos(Ihandle* multitext, int pos)
{
  int start, end, lin, start_col, end_col;
  int tabSize = IupGetInt(multitext, "TABSIZE");
  char *text;
  Iclass* ic;
  char* word;
  char* wordpos = IupGetAttributeId(multitext, "WORDPOS", pos);
  if (wordpos == NULL)
    return NULL;

  sscanf(wordpos, "%d:%d", &start, &end);
  IupTextConvertPosToLinCol(multitext, start, &lin, &start_col);
  IupTextConvertPosToLinCol(multitext, end, &lin, &end_col);
  text = IupGetAttributeId(multitext, "LINE", lin);
  text = changeTabsForSpaces(text, tabSize); /* this allocates a new string that can be changed */
  text[end_col] = '\0';
  word = text + start_col;
  iupStrLower(word, word);

  ic = iupRegisterFindClass(word);
  free(text);

  return ic;
}

static int multitext_dwell_cb(Ihandle* multitext, int code, int pos, int x, int y)
{
  if (code)
  {
    Iclass* ic = find_class_at_pos(multitext, pos);
    if (ic)
    {
      iupClassInfoGetDesc(ic, multitext, "TIP");
      IupSetAttribute(multitext, "TIPVISIBLE", "Yes");
    }
  }
  else
  {
    IupSetAttribute(multitext, "TIPVISIBLE", "No");
    IupSetAttribute(multitext, "TIP", NULL);
  }

  (void)x;
  (void)y;

  return IUP_DEFAULT;
}

static int multitext_kf1_cb(Ihandle *multitext)
{
  int pos = IupGetInt(multitext, "CARETPOS");
  Iclass* ic = find_class_at_pos(multitext, pos);
  if (ic)
    iupClassInfoShowHelp(ic->name);

  return IUP_CONTINUE;
}

static int tree_selection_cb(Ihandle* elem_tree, int id, int status);
static int tree_executeleaf_cb(Ihandle* elem_tree, int id);
static int tree_rightclick_cb(Ihandle* elem_tree, int id);

static int scidlg_newtext_cb(Ihandle* main_dialog, Ihandle *multitext)
{
  /* this is called before the multitext is mapped */
  Ihandle* elem_tree, *elem_tree_box;

  IupSetCallback(multitext, "VALUECHANGED_CB", (Icallback)multitext_valuechanged_cb);
  IupSetCallback(multitext, "K_ESC", (Icallback)multitext_kesc_cb);
  IupSetCallback(multitext, "K_F1", (Icallback)multitext_kf1_cb);
  IupSetCallback(multitext, "MAP_CB", (Icallback)multitext_map_cb);
  IupSetCallback(multitext, "DWELL_CB", (Icallback)multitext_dwell_cb);

  IupSetAttribute(multitext, "AUTOCOMPLETION", "OFF");
  IupSetAttribute(multitext, "MOUSEDWELLTIME", "1000");

  IupSetAttribute(multitext, "MARGINWIDTH2", "0");
  IupSetAttribute(multitext, "MARGINTYPE2", "SYMBOL");
  IupSetAttribute(multitext, "MARGINSENSITIVE2", "YES");
  IupSetAttribute(multitext, "MARGINMASKFOLDERS2", "NO");  /* (disable folding) */
  IupSetAttributeId(multitext, "MARGINMASK", 2, "10");  /* 1110 - marker=1 and marker=2 ad marker=3 */

  IupSetAttributeId(multitext, "MARKERBGCOLOR", 1, "0 255 0");
  IupSetAttributeId(multitext, "MARKERALPHA", 1, "80");
  IupSetAttributeId(multitext, "MARKERSYMBOL", 1, "BACKGROUND");

#ifdef WIN32
  IupSetAttribute(multitext, "FONT", "Consolas, 11");
#else
  IupSetAttribute(multitext, "FONT", "Monospace, 12");
  /* Other alternatives for "Consolas" in Linux: "DejaVu Sans Mono", "Liberation Mono", "Nimbus Mono L", "FreeMono" */
#endif

  elem_tree = IupTree();
  IupSetAttribute(elem_tree, "EXPAND", "YES");
  IupSetAttribute(elem_tree, "ADDROOT", "NO");
  IupSetAttribute(elem_tree, "ADDEXPANDED", "NO");  /* very important for large LEDs (loads much faster) */
  IupSetCallback(elem_tree, "SELECTION_CB", (Icallback)tree_selection_cb);
  IupSetCallback(elem_tree, "EXECUTELEAF_CB", (Icallback)tree_executeleaf_cb);
  IupSetCallback(elem_tree, "EXECUTEBRANCH_CB", (Icallback)tree_executeleaf_cb);
  IupSetCallback(elem_tree, "RIGHTCLICK_CB", (Icallback)tree_rightclick_cb);
  IupSetAttribute(elem_tree, "VISIBLELINES", "3");
  IupSetAttribute(elem_tree, "IMAGELEAF", "IMGEMPTY");

  IupSetAttribute(multitext, "ELEM_TREE", (char*)elem_tree);

  elem_tree_box = IupGetDialogChild(main_dialog, "ELEM_TREE_BOX");
  IupAppend(elem_tree_box, elem_tree);
  IupMap(elem_tree);

  IupSetAttribute(elem_tree_box, "VALUE_HANDLE", (char*)elem_tree);
  IupRefreshChildren(elem_tree_box);

  enable_elem_tools_buttons(elem_tree, 0, NULL);

  return IUP_DEFAULT;
}

static char* get_filename(Ihandle *multitext)
{
  /* if not saved yet return the displayed file title */
  char *filename = IupGetAttribute(multitext, "FILENAME");
  if (!filename) filename = IupGetAttribute(multitext, "NEW_FILENAME");
  return filename;
}

static int scidlg_closetext_cb(Ihandle* main_dialog, Ihandle *multitext)
{
  Ihandle* elem_tree = vLedGetElemTree(multitext);
  char *filename = get_filename(multitext);

  unload_led(multitext, filename);

  IupDestroy(elem_tree);

  (void)main_dialog;
  return IUP_DEFAULT;
}

static int scidlg_configsave_cb(Ihandle *main_dialog, Ihandle* config)
{
  Ihandle* elem_tree_box = IupGetDialogChild(main_dialog, "ELEM_TREE_BOX");
  Ihandle* extra_dlg = (Ihandle*)IupGetAttribute(elem_tree_box, "FIND_ELEM_DIALOG");
  if (iupObjectCheck(extra_dlg))
  {
    IupConfigSetVariableStr(config, "FindElementDialog", "FindText", IupGetAttribute(IupGetDialogChild(extra_dlg, "FIND_TEXT"), "VALUE"));
    IupConfigSetVariableStr(config, "FindElementDialog", "Target", IupGetAttribute((Ihandle*)IupGetAttribute(IupGetDialogChild(extra_dlg, "TARGET"), "VALUE_HANDLE"), "NAME"));

    IupConfigDialogClosed(config, extra_dlg, "FindElementDialog");
  }

  extra_dlg = (Ihandle*)IupGetAttribute(elem_tree_box, "PROPERTIES_DIALOG");
  if (iupObjectCheck(extra_dlg))
    IupConfigDialogClosed(config, extra_dlg, "ElementPropertiesDialog");

  extra_dlg = (Ihandle*)IupGetAttribute(main_dialog, "IMAGE_EDITOR_DIALOG");
  if (iupObjectCheck(extra_dlg))
    IupConfigDialogClosed(config, extra_dlg, "ImageEditorDialog");

  extra_dlg = (Ihandle*)IupGetAttribute(main_dialog, "GLOBALS_DIALOG");
  if (iupObjectCheck(extra_dlg))
    IupConfigDialogClosed(config, extra_dlg, "GlobalsDialog");

  extra_dlg = (Ihandle*)IupGetAttribute(main_dialog, "CLASSINFO_DIALOG");
  if (iupObjectCheck(extra_dlg))
    IupConfigDialogClosed(config, extra_dlg, "ClassInfoDialog");

  return IUP_DEFAULT;
}

static int scidlg_configload_cb(Ihandle *main_dialog, Ihandle* config)
{
  const char* value;

  value = IupConfigGetVariableStr(config, "IupVisualLED", "AutoCompletion");
  if (value)
  {
    Ihandle* ih_item = IupGetDialogChild(main_dialog, "ITM_AUTOCOMPLETE");
    IupSetStrAttribute(ih_item, "VALUE", value);
  }

  value = IupConfigGetVariableStr(config, "IupVisualLED", "Folding");
  if (value)
  {
    Ihandle* ih_item = IupGetDialogChild(main_dialog, "ITM_FOLDING");
    IupSetStrAttribute(ih_item, "VALUE", value);
  }

  return IUP_DEFAULT;
}

static int scidlg_markerchanged_cb(Ihandle *main_dialog, Ihandle *multitext, int lin, int margin)
{
  if (margin == 3)
    IupSetfAttribute(multitext, "FOLDTOGGLE", "%d", lin);

  (void)main_dialog;
  return IUP_DEFAULT;
}

static int led_menu_open_cb(Ihandle *ih_menu)
{
  Ihandle* menu_foldall = IupGetDialogChild(ih_menu, "ITM_FOLD_ALL");
  Ihandle* item_toggle_folding = IupGetDialogChild(ih_menu, "ITM_TOGGLE_FOLDING");
  Ihandle* item_folding = IupGetDialogChild(ih_menu, "ITM_FOLDING");
  Ihandle* item_rewrite = IupGetDialogChild(ih_menu, "ITM_REWRITE");
  Ihandle* multitext = vLedGetCurrentMultitext(ih_menu);
  int loaded = IupGetInt(multitext, "LOADED");

  if (IupGetInt(item_folding, "VALUE"))
  {
    IupSetAttribute(item_toggle_folding, "ACTIVE", "Yes");
    IupSetAttribute(menu_foldall, "ACTIVE", "Yes");
  }
  else
  {
    IupSetAttribute(item_toggle_folding, "ACTIVE", "NO");
    IupSetAttribute(menu_foldall, "ACTIVE", "NO");
  }

  if (loaded)
    IupSetAttribute(item_rewrite, "ACTIVE", "Yes");
  else
    IupSetAttribute(item_rewrite, "ACTIVE", "NO");

  return IUP_DEFAULT;
}

static int tools_menu_open_cb(Ihandle *ih_menu)
{
  Ihandle* item_export_img = IupGetDialogChild(ih_menu, "ITM_EXP_IMG");
  Ihandle* item_show_all_img = IupGetDialogChild(ih_menu, "ITM_SHOW_ALL_IMG");
  Ihandle* item_export_led = IupGetDialogChild(ih_menu, "ITM_EXP_LED");
  Ihandle* item_export_open_led = IupGetDialogChild(ih_menu, "ITM_EXPORT_OPEN_LED");
  Ihandle* item_export_proj_led = IupGetDialogChild(ih_menu, "ITM_EXP_PROJ_LED");
  Ihandle* item_export_lua = IupGetDialogChild(ih_menu, "ITM_EXP_LUA");
  Ihandle* item_export_open_lua = IupGetDialogChild(ih_menu, "ITM_EXPORT_OPEN_LUA");
  Ihandle* item_export_proj_lua = IupGetDialogChild(ih_menu, "ITM_EXP_PROJ_LUA");
  Ihandle* item_export_c = IupGetDialogChild(ih_menu, "ITM_EXP_C");
  Ihandle* item_export_open_c = IupGetDialogChild(ih_menu, "ITM_EXPORT_OPEN_C");
  Ihandle* item_export_proj_c = IupGetDialogChild(ih_menu, "ITM_EXP_PROJ_C");
  Ihandle *projConfig = (Ihandle*)iupAttribGetInherit(ih_menu, "_IUP_PROJECT_CONFIG");
  Ihandle* multitext = vLedGetCurrentMultitext(ih_menu);
  int loaded = IupGetInt(multitext, "LOADED");

  /* all these tools are based on the elements that are loaded */

  if (loaded)
  {
    IupSetAttribute(item_show_all_img, "ACTIVE", "YES");
    IupSetAttribute(item_export_img, "ACTIVE", "YES");
    IupSetAttribute(item_export_led, "ACTIVE", "YES");
    IupSetAttribute(item_export_open_led, "ACTIVE", "YES");
    IupSetAttribute(item_export_lua, "ACTIVE", "YES");
    IupSetAttribute(item_export_open_lua, "ACTIVE", "YES");
    IupSetAttribute(item_export_c, "ACTIVE", "YES");
    IupSetAttribute(item_export_open_c, "ACTIVE", "YES");
  }
  else
  {
    IupSetAttribute(item_show_all_img, "ACTIVE", "NO");
    IupSetAttribute(item_export_img, "ACTIVE", "NO");
    IupSetAttribute(item_export_led, "ACTIVE", "NO");
    IupSetAttribute(item_export_open_led, "ACTIVE", "NO");
    IupSetAttribute(item_export_lua, "ACTIVE", "NO");
    IupSetAttribute(item_export_open_lua, "ACTIVE", "NO");
    IupSetAttribute(item_export_c, "ACTIVE", "NO");
    IupSetAttribute(item_export_open_c, "ACTIVE", "NO");
  }

  if (projConfig && loaded)
  {
    IupSetAttribute(item_export_proj_c, "ACTIVE", "YES");
    IupSetAttribute(item_export_proj_lua, "ACTIVE", "YES");
    IupSetAttribute(item_export_proj_led, "ACTIVE", "YES");
  }
  else
  {
    IupSetAttribute(item_export_proj_c, "ACTIVE", "NO");
    IupSetAttribute(item_export_proj_lua, "ACTIVE", "NO");
    IupSetAttribute(item_export_proj_led, "ACTIVE", "NO");
  }

  return IUP_DEFAULT;
}

static int item_autocomplete_action_cb(Ihandle* ih_item)
{
  Ihandle* tabs = IupGetDialogChild(ih_item, "MULTITEXT_TABS");
  int children_count = IupGetChildCount(tabs);
  Ihandle* multitext;
  Ihandle* config = get_config(ih_item);
  int i;

  if (IupGetInt(ih_item, "VALUE"))
  {
    IupSetAttribute(ih_item, "VALUE", "OFF");

    for (i = 0; i < children_count; i++)
    {
      multitext = IupGetChild(tabs, i);
      IupSetAttribute(multitext, "AUTOCOMPLETION", "OFF");
    }

  }
  else
  {
    IupSetAttribute(ih_item, "VALUE", "ON");

    for (i = 0; i < children_count; i++)
    {
      multitext = IupGetChild(tabs, i);
      IupSetAttribute(multitext, "AUTOCOMPLETION", "ON");
    }
  }

  IupConfigSetVariableStr(config, "IupVisualLED", "AutoCompletion", IupGetAttribute(ih_item, "VALUE"));

  return IUP_DEFAULT;
}


static int setparent_param_cb(Ihandle* param_dialog, int param_index, void* user_data)
{
  if (param_index == IUP_GETPARAM_MAP)
  {
    Ihandle* ih = (Ihandle*)user_data;
    IupSetAttributeHandle(param_dialog, "PARENTDIALOG", ih);
  }

  return 1;
}

static int item_style_config_action_cb(Ihandle* ih_item)
{
  Ihandle* tabs = IupGetDialogChild(ih_item, "MULTITEXT_TABS");
  int children_count = IupGetChildCount(tabs);
  Ihandle* multitext;
  Ihandle* config = get_config(ih_item);
  char commentColor[30], numberColor[30], keywordColor[30],
    stringColor[30], characterColor[30], operatorColor[30];
  int i;

  strcpy(commentColor, IupConfigGetVariableStr(config, "IupVisualLED", "CommentColor"));
  strcpy(numberColor, IupConfigGetVariableStr(config, "IupVisualLED", "NumberColor"));
  strcpy(keywordColor, IupConfigGetVariableStr(config, "IupVisualLED", "KeywordColor"));
  strcpy(stringColor, IupConfigGetVariableStr(config, "IupVisualLED", "StringColor"));
  strcpy(characterColor, IupConfigGetVariableStr(config, "IupVisualLED", "CharacterColor"));
  strcpy(operatorColor, IupConfigGetVariableStr(config, "IupVisualLED", "OperatorColor"));

  if (!IupGetParam("Syntax Colors", setparent_param_cb, IupGetDialog(ih_item),
    "Comment: %c\n"
    "Number: %c\n"
    "Keyword: %c\n"
    "String: %c\n"
    "Character: %c\n"
    "Operator: %c\n",
    commentColor, numberColor, keywordColor, stringColor, characterColor, operatorColor, NULL))
    return IUP_DEFAULT;

  IupConfigSetVariableStr(config, "IupVisualLED", "CommentColor", commentColor);
  IupConfigSetVariableStr(config, "IupVisualLED", "NumberColor", numberColor);
  IupConfigSetVariableStr(config, "IupVisualLED", "KeywordColor", keywordColor);
  IupConfigSetVariableStr(config, "IupVisualLED", "StringColor", stringColor);
  IupConfigSetVariableStr(config, "IupVisualLED", "CharacterColor", characterColor);
  IupConfigSetVariableStr(config, "IupVisualLED", "OperatorColor", operatorColor);

  for (i = 0; i < children_count; i++)
  {
    multitext = IupGetChild(tabs, i);

    IupSetStrAttribute(multitext, "STYLEFGCOLOR1", commentColor);
    IupSetStrAttribute(multitext, "STYLEFGCOLOR2", numberColor);
    IupSetStrAttribute(multitext, "STYLEFGCOLOR3", keywordColor);
    IupSetStrAttribute(multitext, "STYLEFGCOLOR4", stringColor);
    IupSetStrAttribute(multitext, "STYLEFGCOLOR5", characterColor);
    IupSetStrAttribute(multitext, "STYLEFGCOLOR6", operatorColor);
  }

  return IUP_DEFAULT;
}

static int item_folding_action_cb(Ihandle* ih)
{
  Ihandle* tabs = IupGetDialogChild(ih, "MULTITEXT_TABS");
  int children_count = IupGetChildCount(tabs);
  Ihandle* multitext;
  Ihandle* config = get_config(ih);
  int i;

  if (IupGetInt(ih, "VALUE"))
  {
    IupSetAttribute(ih, "VALUE", "OFF");

    for (i = 0; i < children_count; i++)
    {
      multitext = IupGetChild(tabs, i);

      IupSetAttribute(multitext, "PROPERTY", "fold=0");
      IupSetAttribute(multitext, "MARGINWIDTH3", "0");
      IupSetAttribute(multitext, "_IUP_FOLDDING", NULL);
    }
  }
  else
  {
    IupSetAttribute(ih, "VALUE", "ON");

    for (i = 0; i < children_count; i++)
    {
      multitext = IupGetChild(tabs, i);

      IupSetAttribute(multitext, "PROPERTY", "fold=1");
      IupSetAttribute(multitext, "MARGINWIDTH3", VLED_FOLDING_MARGIN);
      IupSetAttribute(multitext, "_IUP_FOLDDING", "1");
      IupSetAttribute(multitext, "FOLDALL", "EXPAND");
    }
  }

  IupConfigSetVariableStr(config, "IupVisualLED", "Folding", IupGetAttribute(ih, "VALUE"));

  return IUP_DEFAULT;
}

static int item_toggle_folding_action_cb(Ihandle* ih)
{
  Ihandle* multitext = vLedGetCurrentMultitext(ih);
  int pos = IupGetInt(multitext, "CARETPOS");
  int lin, col;

  /* must test again because it can be called by the hot key */
  if (!IupGetInt(multitext, "_IUP_FOLDDING"))
    return IUP_DEFAULT;

  IupTextConvertPosToLinCol(multitext, pos, &lin, &col);

  if (!IupGetIntId(multitext, "FOLDLEVELHEADER", lin))
  {
    lin = IupGetIntId(multitext, "FOLDPARENT", lin);
    if (lin < 0)
      return IUP_DEFAULT;
  }

  IupSetfAttribute(multitext, "FOLDTOGGLE", "%d", lin);
  IupSetfAttribute(multitext, "CARET", "%d:0", lin);

  return IUP_DEFAULT;
}

static int item_fold_collapse_action_cb(Ihandle* ih)
{
  Ihandle* multitext = vLedGetCurrentMultitext(ih);

  IupSetAttribute(multitext, "FOLDALL", "CONTRACT");

  return IUP_DEFAULT;
}

static int item_fold_expand_action_cb(Ihandle* ih)
{
  Ihandle* multitext = vLedGetCurrentMultitext(ih);

  IupSetAttribute(multitext, "FOLDALL", "EXPAND");

  return IUP_DEFAULT;
}

static int item_fold_toggle_action_cb(Ihandle* ih)
{
  Ihandle* multitext = vLedGetCurrentMultitext(ih);

  IupSetAttribute(multitext, "FOLDALL", "TOGGLE");

  return IUP_DEFAULT;
}

static int item_fold_level_action_cb(Ihandle* ih_item)
{
  int level = 0, action = 0;
  Ihandle* config = get_config(ih_item);

  const char* value = IupConfigGetVariableStr(config, "IupVisualLED", "FoldAllLevel");
  if (value) iupStrToInt(value, &level);

  value = IupConfigGetVariableStr(config, "IupVisualLED", "FoldAllLevelAction");
  if (value) iupStrToInt(value, &action);

  if (IupGetParam("Fold All by Level", setparent_param_cb, IupGetDialog(ih_item),
    "Level: %i\n"
    "Options: %o|Collapse|Expand|Toggle|\n",
    &level, &action, NULL))
  {
    Ihandle* multitext = vLedGetCurrentMultitext(ih_item);
    int lin, count = IupGetInt(multitext, "LINECOUNT");

    IupConfigSetVariableInt(config, "IupVisualLED", "FoldAllLevel", level);
    IupConfigSetVariableInt(config, "IupVisualLED", "FoldAllLevelAction", action);

    for (lin = 0; lin < count; lin++)
    {
      if (IupGetIntId(multitext, "FOLDLEVELHEADER", lin))
      {
        int foldLevel = IupGetIntId(multitext, "FOLDLEVEL", lin);
        if (foldLevel + 1 == level) /* level at header is different from child */
        {
          switch (action)
          {
            case 0: /* Collapse */
              if (IupGetIntId(multitext, "FOLDEXPANDED", lin))
                IupSetfAttribute(multitext, "FOLDTOGGLE", "%d", lin);
              break;
            case 1: /* Expand */
              if (!IupGetIntId(multitext, "FOLDEXPANDED", lin))
                IupSetfAttribute(multitext, "FOLDTOGGLE", "%d", lin);
              break;
            case 2: /* Toggle */
              IupSetfAttribute(multitext, "FOLDTOGGLE", "%d", lin);
              break;
          }
        }
      }
    }
  }

  return IUP_DEFAULT;
}

static int item_comment_action_cb(Ihandle* ih_item)
{
  Ihandle* multitext = vLedGetCurrentMultitext(ih_item);
  char *sel = IupGetAttribute(multitext, "SELECTION");
  char *text_line;
  int lin, col, lin1, lin2;

  if (sel)
  {
    int col1, col2;
    sscanf(sel, "%d,%d:%d,%d", &lin1, &col1, &lin2, &col2);
  }
  else
  {
    lin1 = IupGetInt(multitext, "CARET");
    lin2 = lin1;
  }

  IupSetAttribute(multitext, "UNDOACTION", "BEGIN");

  for (lin = lin1; lin <= lin2; lin++)
  {
    int len, pos;
    text_line = IupGetAttributeId(multitext, "LINE", lin);
    len = (int)strlen(text_line);

    for (col = 0; col < len; col++)
    {
      char c = text_line[col];
      if (c != ' ' && c != '\t')
        break;
    }

    IupTextConvertLinColToPos(multitext, lin, col, &pos);
    IupSetAttributeId(multitext, "INSERT", pos, "# ");
  }

  IupSetAttribute(multitext, "UNDOACTION", "END");

  return IUP_DEFAULT;
}

static int item_uncomment_action_cb(Ihandle* ih_item)
{
  Ihandle* multitext = vLedGetCurrentMultitext(ih_item);
  char *sel = IupGetAttribute(multitext, "SELECTION");
  char *text_line;
  int lin, col, lin1, lin2;

  if (sel)
  {
    int col1, col2;
    sscanf(sel, "%d,%d:%d,%d", &lin1, &col1, &lin2, &col2);
  }
  else
  {
    lin1 = IupGetInt(multitext, "CARET");
    lin2 = lin1;
  }

  IupSetAttribute(multitext, "UNDOACTION", "BEGIN");

  for (lin = lin1; lin <= lin2; lin++)
  {
    int len, pos, nChar;
    text_line = IupGetAttributeId(multitext, "LINE", lin);
    len = (int)strlen(text_line);
    nChar = 0;

    for (col = 0; col < len; col++)
    {
      if (text_line[col] == '#')
      {
        nChar = 1;
        if (text_line[col + 2] == ' ')
          nChar++;
        break;
      }
    }

    if (nChar == 0)
      continue;

    IupTextConvertLinColToPos(multitext, lin, col, &pos);
    IupSetStrf(multitext, "DELETERANGE", "%d,%d", pos, nChar);
  }

  IupSetAttribute(multitext, "UNDOACTION", "END");

  return IUP_DEFAULT;
}

static int item_load_action_cb(Ihandle *ih_item)
{
  Ihandle* multitext = vLedGetCurrentMultitext(ih_item);
  Ihandle* elem_tree = vLedGetElemTree(multitext);
  int modified = IupGetInt(multitext, "MODIFIED");
  char *filename = get_filename(multitext);

  unload_led(multitext, filename);
  IupSetAttribute(multitext, "LOADED", NULL);

  if (modified || !IupGetAttribute(multitext, "FILENAME"))
    load_led(elem_tree, filename, 1);
  else
    load_led(elem_tree, filename, 0);

  return IUP_DEFAULT;
}

static int item_unload_action_cb(Ihandle *ih_item)
{
  Ihandle* multitext = vLedGetCurrentMultitext(ih_item);
  Ihandle* elem_tree = vLedGetElemTree(multitext);
  char *filename = get_filename(multitext);

  unload_led(multitext, filename);
  IupSetAttribute(multitext, "LOADED", NULL);

  IupSetAttribute(elem_tree, "DELNODE0", "ALL");
  enable_elem_tools_buttons(elem_tree, 0, NULL);

  return IUP_DEFAULT;
}

static char* readFile(const char* filename)
{
  long size;
  char* str;
  FILE* file = fopen(filename, "rb");
  if (!file)
    return NULL;

  /* calculate file size */
  fseek(file, 0, SEEK_END);
  size = ftell(file);
  if (size <= 0)
  {
    fclose(file);
    return NULL;
  }

  /* allocate memory for the file contents + nul terminator */
  str = malloc(size + 1);
  if (!str)
  {
    fclose(file);
    return NULL;
  }

  /* read all data at once */
  fseek(file, 0, SEEK_SET);
  fread(str, size, 1, file);

  /* set the null terminator */
  str[size] = 0;

  fclose(file);
  return str;
}

static void vLedExport(Ihandle* multitext, const char* src_filename, const char* dst_filename, int export_format);

static void rewrite_led(Ihandle* multitext)
{
  char tempFilename[10240];
  if (iupStrTmpFileName(tempFilename, "~vled"))
  {
    char* new_buffer;
    char *filename = get_filename(multitext);

    vLedExport(multitext, filename, tempFilename, IUP_LAYOUT_EXPORT_LED);

    new_buffer = readFile(tempFilename);
    if (new_buffer)
    {
      Ihandle* elem_tree = vLedGetElemTree(multitext);

      IupSetStrAttribute(multitext, "VALUE", new_buffer);

      IupSetInt(multitext, "MARKERDELETEALL", 1);
      updateElemTree(elem_tree, filename);

      free(new_buffer);
    }

    remove(tempFilename);
  }
}

static int item_rewrite_action_cb(Ihandle *ih_item)
{
  Ihandle* multitext = vLedGetCurrentMultitext(ih_item);

  rewrite_led(multitext);

  return IUP_DEFAULT;
}

static int item_autoload_action_cb(Ihandle *ih_item)
{
  Ihandle* config = get_config(ih_item);

  if (IupGetInt(ih_item, "VALUE"))
    IupConfigSetVariableInt(config, "IupVisualLED", "AutoLoad", 1);
  else
    IupConfigSetVariableInt(config, "IupVisualLED", "AutoLoad", 0);

  return IUP_DEFAULT;
}

static int item_import_img_action_cb(Ihandle *ih_item)
{
  Ihandle* config = get_config(ih_item);
  Ihandle* filedlg = IupFileDlg();
  const char* dir = IupConfigGetVariableStr(config, "IupVisualLED", "LastImageDirectory");
  if (!dir) dir = IupConfigGetVariableStr(config, "IupVisualLED", "LastDirectory");

  IupSetStrAttribute(filedlg, "DIRECTORY", dir);
  IupSetAttribute(filedlg, "DIALOGTYPE", "OPEN");
  IupSetAttribute(filedlg, "TITLE", "Load Image File(s)");
  IupSetAttribute(filedlg, "MULTIPLEFILES", "YES");
  IupSetAttributeHandle(filedlg, "PARENTDIALOG", IupGetDialog(ih_item));
  IupSetAttribute(filedlg, "ICON", IupGetGlobal("ICON"));
  IupSetAttribute(filedlg, "EXTFILTER", "Image Files|*.bmp;*.gif;*.jpg;*.png;*.tif;*.tga|All Files|*.*|");

  IupPopup(filedlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

  if (IupGetInt(filedlg, "STATUS") != -1)
  {
    int i, ret =0 , count = IupGetInt(filedlg, "MULTIVALUECOUNT");
    dir = IupGetAttributeId(filedlg, "MULTIVALUE", 0);

    for (i = 1; i < count; i++)  /* i==0 contains the path */
    {
      char* img_filetitle = IupGetAttributeId(filedlg, "MULTIVALUE", i);
      char img_filename[10240];
      strcpy(img_filename, dir);
      strcat(img_filename, img_filetitle);

      ret |= LoadImageFile(ih_item, img_filename);
    }

    IupConfigSetVariableStr(config, "IupVisualLED", "LastImageDirectory", dir);

    if (ret)
    {
      Ihandle* multitext = vLedGetCurrentMultitext(ih_item);
      Ihandle* elem_tree = vLedGetElemTree(multitext);
      char *filename = get_filename(multitext);
      updateElemTree(elem_tree, filename);
    }
  }

  IupDestroy(filedlg);
  return IUP_DEFAULT;
}

static int item_export_img_cb(Ihandle *ih_item)
{
  char* folder;
  int i, num_images, num_names = IupGetAllNames(NULL, -1);
  char* *names;
  Ihandle* multitext = vLedGetCurrentMultitext(ih_item);
  char *filename = get_filename(multitext);

  const char *img_type = get_img_fileformat();
  if (!img_type)
    return IUP_DEFAULT;

  folder = getfolder(ih_item);
  if (!folder)
    return IUP_DEFAULT;

  names = malloc(sizeof(char*)*num_names);
  IupGetAllNames(names, num_names);

  num_images = 0;
  for (i = 0; i < num_names; i++)
  {
    Ihandle *elem = IupGetHandle(names[i]);

    if (!iupObjectCheck(elem))
      continue;

    if (elem->iclass->nativetype != IUP_TYPEIMAGE)
      continue;

    /* save only loaded images of the current file */
    if (!vLedIsAlien(elem, filename))
    {
      char img_filename[10240] = "";
      char buff[80];

      strcpy(img_filename, folder);
      strcat(img_filename, "/");
      strcat(img_filename, strGetFileTitle(filename));
      strcat(img_filename, "_");
      strcat(img_filename, names[i]);
      iupStrLower(buff, img_type);
      strcat(img_filename, ".");
      strcat(img_filename, buff);

      if (!IupSaveImage(elem, img_filename, img_type))
      {
        char* err_msg = IupGetGlobal("IUPIM_LASTERROR");
        if (err_msg)
          IupMessageError(IupGetDialog(ih_item), err_msg);
        return IUP_DEFAULT;
      }

      num_images++;
    }
  }

  if (num_images == 0)
  {
    IupMessageError(IupGetDialog(ih_item), "No images.");
    return IUP_DEFAULT;
  }

  return IUP_DEFAULT;
}

static int showimages_close_cb(void)
{
  return IUP_CLOSE;
}

static int showimages_inactivetoggle_cb(Ihandle* toggle, int v)
{
  Ihandle* tabs = (Ihandle*)IupGetAttribute(toggle, "TABS");
  Ihandle* child = IupGetNextChild(tabs, NULL);
  while (child)
  {
    IupSetAttribute(child, "ACTIVE", v ? "NO" : "YES");
    child = IupGetNextChild(tabs, child);
  }
  return IUP_DEFAULT;
}

static int showimages_imagebutton_cb(Ihandle* button)
{
  Ihandle* label = (Ihandle*)IupGetAttribute(button, "_INFO_LABEL");
  IupSetAttribute(label, "TITLE", IupGetAttribute(button, "_INFO"));
  return IUP_DEFAULT;
}

static int item_show_all_img_cb(Ihandle *ih_item)
{
  Ihandle *dialog, *box, *files, *tabs, *toggle, *label;
  Ihandle* params[500];
  int show_stock = iupAttribGetInt(ih_item, "_IUP_SHOWSTOCK");
  int num_tabs = 0, num_images;
  int i, num_names = IupGetAllNames(NULL, -1);
  char* *names = malloc(sizeof(char*)*num_names);
  Ihandle* *images = malloc(sizeof(Ihandle*)*num_names);
  IupGetAllNames(names, num_names);

  files = IupUser();

  num_images = 0;
  for (i = 0; i < num_names; i++)
  {
    Ihandle *elem = IupGetHandle(names[i]);

    if (!iupObjectCheck(elem))
      continue;

    if (elem->iclass->nativetype == IUP_TYPEIMAGE)
    {
      /* if stock show only NOT loaded images */
      if (show_stock && iupAttribGet(elem, "_IUPLED_FILENAME"))
        continue;

      /* if not stock show only loaded images */
      if (!show_stock && !iupAttribGet(elem, "_IUPLED_FILENAME"))
        continue;

      if (iupStrEqualPartial(names[i], "_IUP_NAME"))  /* ignore internal names */
        continue;

      images[num_images] = elem;
      num_images++;
    }
  }

  if (num_images == 0)
  {
    IupMessageError(IupGetDialog(ih_item), "No images.");
    free(names);
    free(images);
    return IUP_DEFAULT;
  }

  qsort(images, num_images, sizeof(Ihandle*), compare_image_names);

  for (i = 0; i < num_images; i++)
  {
    Ihandle *tbox, *lbox, *button, *elem;
    char* name;
    char* filename;

    elem = images[i];
    name = vLedGetName(elem);

    if (show_stock)
    {
      if (iupAttribGet(elem, "_IUPIMAGE_STOCK_LOAD"))
        filename = "IupImgLib";
      else
        filename = "Internal";
    }
    else
      filename = iupAttribGet(elem, "_IUPLED_FILENAME");
    if (!filename)
      continue;

    tbox = (Ihandle*)IupGetAttribute(files, filename);
    if (!tbox)
    {
      char* file_title = strGetFileTitle(filename);
      tbox = IupVbox(NULL);
      IupSetAttribute(files, filename, (char*)tbox);
      IupSetStrAttribute(tbox, "TABTITLE", file_title);
      free(file_title);
      params[num_tabs] = tbox;
      num_tabs++;
    }

    lbox = (Ihandle*)IupGetAttribute(tbox, filename);
    if (!lbox || IupGetInt(lbox, "LINE_COUNT") == 10)
    {
      lbox = IupHbox(NULL);
      IupAppend(tbox, lbox);
      IupSetAttribute(tbox, filename, (char*)lbox);
      IupSetAttribute(lbox, "LINE_COUNT", "0");
    }

    button = IupButton("", NULL);
    IupSetStrAttribute(button, "IMAGE", name);
    IupSetAttribute(button, "PADDING", "5x5");
    IupSetAttribute(button, "FLAT", "Yes");
    IupSetfAttribute(button, "_INFO", "%s [%d,%d] - %s", name, IupGetInt(elem, "WIDTH"), IupGetInt(elem, "HEIGHT"), IupGetClassName(elem));
    IupSetCallback(button, "ACTION", (Icallback)showimages_imagebutton_cb);
    IupSetCallback(button, "GETFOCUS_CB", (Icallback)showimages_imagebutton_cb);
    IupAppend(lbox, button);
    IupSetfAttribute(lbox, "LINE_COUNT", "%d", IupGetInt(lbox, "LINE_COUNT") + 1);
  }

  params[num_tabs] = NULL;

  box = IupVbox(toggle = IupToggle("INACTIVE", NULL),
                tabs = IupTabsv(params),
                label = IupLabel(""),
                NULL);
  IupSetAttribute(box, "MARGIN", "10x10");
  IupSetAttribute(box, "GAP", "10");
  IupSetAttribute(tabs, "ALIGNMENT", "NW");
  IupSetAttribute(tabs, "SIZE", "150x80");
  IupSetCallback(toggle, "ACTION", (Icallback)showimages_inactivetoggle_cb);
  IupSetAttribute(toggle, "TABS", (char*)tabs);
  IupSetAttribute(label, "EXPAND", "HORIZONTAL");

  dialog = IupDialog(box);
  IupSetAttribute(dialog, "TITLE", show_stock? "Stock Images": "All Images");
  IupSetCallback(dialog, "CLOSE_CB", (Icallback)showimages_close_cb);
  IupSetAttribute(dialog, "_INFO_LABEL", (char*)label);
  IupSetAttributeHandle(dialog, "PARENTDIALOG", IupGetDialog(ih_item));

  IupPopup(dialog, IUP_CENTERPARENT, IUP_CENTERPARENT);

  IupDestroy(dialog);
  IupDestroy(files);

  free(names);
  free(images);
  return IUP_DEFAULT;
}

static int item_show_stock_img_cb(Ihandle *ih_item)
{
  iupImageStockLoadAll();

  iupAttribSet(ih_item, "_IUP_SHOWSTOCK", "1");
  item_show_all_img_cb(ih_item);
  iupAttribSet(ih_item, "_IUP_SHOWSTOCK", NULL);

  return IUP_DEFAULT;
}

static void iExportRemoveExt(char* title, const char* ext)
{
  int len = (int)strlen(title);
  int len_ext = (int)strlen(ext);
  if (len_ext == 1)
  {
    if (tolower(title[len - 1]) == ext[0] &&
        title[len - 2] == '.')
      title[len - 2] = 0; /* place terminator at dot */
  }
  else
  {
    if (tolower(title[len - 1]) == ext[2] &&
        tolower(title[len - 2]) == ext[1] &&
        tolower(title[len - 3]) == ext[0] &&
        title[len - 4] == '.')
      title[len - 4] = 0; /* place terminator at dot */
  }
}

static void iLayoutFindNamedElem(const char* src_filename, Iarray* names_array)
{
  char* *names;
  int num_names;
  Ihandle* *named_elem = NULL;
  int i, count = 0;

  num_names = IupGetAllNames(NULL, -1);
  names = malloc(sizeof(char*)*num_names);
  IupGetAllNames(names, num_names);

  for (i = 0; i < num_names; i++)
  {
    Ihandle *elem = IupGetHandle(names[i]);
    if (elem)
    {
      if (iupStrEqualPartial(names[i], "_IUP_NAME") ||
          vLedIsAlien(elem, src_filename))
        continue;

      named_elem = iupArrayAdd(names_array, 1);
      named_elem[count] = elem;
      count++;
    }
  }

  free(names);
}

static void vLedExport(Ihandle* multitext, const char* src_filename, const char* dst_filename, int export_format)
{
  Iarray* names_array;
  Ihandle* *named_elem;
  int count;
  FILE* file;
  char* title = NULL;

  names_array = iupArrayCreate(1024, sizeof(Ihandle*));  /* just set an initial size, but count is 0 */

  /* lists all elements that have non automatic names and
     were parsed from the filename */
  iLayoutFindNamedElem(src_filename, names_array);

  count = iupArrayCount(names_array);
  named_elem = iupArrayGetData(names_array);

  if (count == 0)
  {
    iupArrayDestroy(names_array);
    return;
  }

  file = fopen(dst_filename, "wb");
  if (!file)
  {
    iupArrayDestroy(names_array);
    return;
  }

  qsort(named_elem, count, sizeof(Ihandle*), compare_named_handles);

  if (export_format == IUP_LAYOUT_EXPORT_LUA)
  {
    title = iupStrFileGetTitle(dst_filename);
    iExportRemoveExt(title, "lua");
    iupStrReplaceReserved(title, '_');

    fprintf(file, "--   Generated by IupVisualLED export to Lua.\n\n");
  }
  else if (export_format == IUP_LAYOUT_EXPORT_C)
  {
    title = iupStrFileGetTitle(dst_filename);
    iExportRemoveExt(title, "c");
    iupStrReplaceReserved(title, '_');

    fprintf(file, "/*   Generated by IupVisualLED export to C.   */\n\n");

    fprintf(file, "#include <stdlib.h>\n");
    fprintf(file, "#include <iup.h>\n\n");
  }
  else /* IUP_LAYOUT_EXPORT_LED */
  {
    char* comments;

    fprintf(file, "#   Generated by IupVisualLED export to LED.\n\n");

    if (multitext)
      comments = iupAttribGet(multitext, "_IUPLED_COMMENTS");
    else
      comments = load_comments(src_filename);

    if (comments)
      fprintf(file, "%s", comments);

    if (comments && !multitext)
      free(comments);
  }

  /* export all images as individual functions */
  iupLayoutExportNamedImageList(file, named_elem, count, export_format);

  if (export_format == IUP_LAYOUT_EXPORT_LUA)
  {
    fprintf(file, "function create_layout_%s()\n", title);
  }
  else if (export_format == IUP_LAYOUT_EXPORT_C)
  {
    fprintf(file, "void create_layout_%s(void)\n", title);
    fprintf(file, "{\n");
  }

  iupLayoutExportNamedElemList(file, named_elem, count, export_format, 1);  /* export all the attributes that were loaded form the LED */

  /* call the image functions */
  iupLayoutExportNamedImageListSetHandle(file, named_elem, count, export_format);

  if (export_format == IUP_LAYOUT_EXPORT_LUA)
    fprintf(file, "\nend\n");
  else if (export_format == IUP_LAYOUT_EXPORT_C)
    fprintf(file, "\n}\n");

  if (title)
    free(title);
  iupArrayDestroy(names_array);
  fclose(file);
}

static void strAddExtension(char* filename, const char* ext)
{
  char* new_ext = iupStrFileGetExt(filename);
  if (!new_ext)
  {
    strcat(filename, ".");
    strcat(filename, ext);
  }
  free(new_ext);
}

static int ivLedGetExportFile(Ihandle* ih, char* export_filename, const char *extfilter)
{
  Ihandle *filedlg = 0;
  int ret;
  Ihandle* config = get_config(ih);
  const char* dir = IupConfigGetVariableStr(config, "IupVisualLED", "LastExportDirectory");
  if (!dir) dir = IupConfigGetVariableStr(config, "IupVisualLED", "LastDirectory");

  filedlg = IupFileDlg();

  IupSetStrAttribute(filedlg, "EXTFILTER", extfilter);
  IupSetStrAttribute(filedlg, "FILE", export_filename);
  IupSetStrAttribute(filedlg, "DIRECTORY", dir);
  IupSetAttribute(filedlg, "DIALOGTYPE", "SAVE");
  IupSetAttribute(filedlg, "ALLOWNEW", "YES");
  IupSetAttribute(filedlg, "NOCHANGEDIR", "YES");
  IupSetAttributeHandle(filedlg, "PARENTDIALOG", IupGetDialog(ih));
  IupSetAttribute(filedlg, "ICON", IupGetGlobal("ICON"));

  IupPopup(filedlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

  ret = IupGetInt(filedlg, "STATUS");
  if (ret != -1)
  {
    char* ext = iupStrFileGetExt(export_filename);
    char* value = IupGetAttribute(filedlg, "VALUE");
    strcpy(export_filename, value);
    strAddExtension(export_filename, ext);

    dir = IupGetAttribute(filedlg, "DIRECTORY");
    IupConfigSetVariableStr(config, "IupVisualLED", "LastExportDirectory", dir);

    free(ext);
  }

  IupDestroy(filedlg);

  return ret;
}

static int item_export_led_action_cb(Ihandle *ih_item)
{
  int ret;
  char export_filename[4096];
  Ihandle* multitext = vLedGetCurrentMultitext(ih_item);
  char *filename = get_filename(multitext);
  char* title = iupStrFileGetTitle(filename);

  char *ext = strrchr(title, '.');
  if (ext) *ext = 0;

  strcpy(export_filename, title);
  strcat(export_filename, ".led");

  ret = ivLedGetExportFile(ih_item, export_filename, "LED Files|*.led|All Files|*.*|");
  if (ret != -1) /* ret==0 existing file. TODO: check if export_filename is opened. */
    vLedExport(multitext, filename, export_filename, IUP_LAYOUT_EXPORT_LED);

  return IUP_DEFAULT;
}

static int item_export_lua_action_cb(Ihandle *ih_item)
{
  int ret;
  char export_filename[4096];
  Ihandle* multitext = vLedGetCurrentMultitext(ih_item);
  char *filename = get_filename(multitext);
  char* title = iupStrFileGetTitle(filename);

  char *ext = strrchr(title, '.');
  if (ext) *ext = 0;

  strcpy(export_filename, title);
  strcat(export_filename, ".lua");

  ret = ivLedGetExportFile(ih_item, export_filename, "Lua Files|*.lua|All Files|*.*|");
  if (ret != -1) /* ret==0 existing file. TODO: check if export_filename is opened. */
    vLedExport(multitext, filename, export_filename, IUP_LAYOUT_EXPORT_LUA);

  return IUP_DEFAULT;
}

static int item_export_c_action_cb(Ihandle *ih_item)
{
  int ret;
  char export_filename[4096];
  Ihandle* multitext = vLedGetCurrentMultitext(ih_item);
  char *filename = get_filename(multitext);
  char* title = iupStrFileGetTitle(filename);

  char *ext = strrchr(title, '.');
  if (ext) *ext = 0;

  strcpy(export_filename, title);
  strcat(export_filename, ".c");

  ret = ivLedGetExportFile(ih_item, export_filename, "C Files|*.c|All Files|*.*|");
  if (ret != -1) /* ret==0 existing file. TODO: check if export_filename is opened. */
    vLedExport(multitext, filename, export_filename, IUP_LAYOUT_EXPORT_C);

  return IUP_DEFAULT;
}

static int item_export_all_open_action_cb(Ihandle *ih_item)
{
  char *itemName = IupGetAttribute(ih_item, "NAME");
  Ihandle* tabs = IupGetDialogChild(ih_item, "MULTITEXT_TABS");
  int count = IupGetInt(tabs, "COUNT");
  char *folder;
  int i;

  folder = getfolder(ih_item);
  if (!folder)
    return IUP_DEFAULT;

  for (i = 0; i < count; i++)
  {
    char export_filename[10240];
    Ihandle *multitext = IupGetChild(tabs, i);
    char *filename = get_filename(multitext);
    char* title = iupStrFileGetTitle(filename);

    char *ext = strrchr(title, '.');
    if (ext) *ext = 0;

    strcpy(export_filename, folder);
    strcat(export_filename, "\\");
    strcat(export_filename, title);

    if (strcmp(itemName, "ITM_EXPORT_OPEN_LED") == 0)
    {
      strcat(export_filename, ".led");

      vLedExport(multitext, filename, export_filename, IUP_LAYOUT_EXPORT_LED);
    }
    else if (strcmp(itemName, "ITM_EXPORT_OPEN_LUA") == 0)
    {
      strcat(export_filename, ".lua");

      vLedExport(multitext, filename, export_filename, IUP_LAYOUT_EXPORT_LUA);
    }
    else
    {
      strcat(export_filename, ".c");

      vLedExport(multitext, filename, export_filename, IUP_LAYOUT_EXPORT_C);
    }
  }

  return IUP_DEFAULT;
}

static int item_export_proj_action_cb(Ihandle *ih_item)
{
  char *itemName = IupGetAttribute(ih_item, "NAME");
  Ihandle* projectTree = IupGetDialogChild(ih_item, "PROJECTTREE");
  int count = IupGetInt(projectTree, "CHILDCOUNT0");
  int i;

  char *folder = getfolder(ih_item);
  if (!folder)
    return IUP_DEFAULT;

  for (i = 1; i <= count; i++)
  {
    char export_filename[10240];
    char* filename = IupTreeGetUserId(projectTree, i);

    char* title = iupStrFileGetTitle(filename);

    char *ext = strrchr(title, '.');
    if (ext) *ext = 0;

    strcpy(export_filename, folder);
    strcat(export_filename, "\\");
    strcat(export_filename, title);

    if (strcmp(itemName, "ITM_EXP_PROJ_LED") == 0)
    {
      strcat(export_filename, ".led");

      vLedExport(NULL, filename, export_filename, IUP_LAYOUT_EXPORT_LED);
    }
    else if (strcmp(itemName, "ITM_EXP_PROJ_LUA") == 0)
    {
      strcat(export_filename, ".lua");

      vLedExport(NULL, filename, export_filename, IUP_LAYOUT_EXPORT_LUA);
    }
    else
    {
      strcat(export_filename, ".c");

      vLedExport(NULL, filename, export_filename, IUP_LAYOUT_EXPORT_C);
    }
  }

  return IUP_DEFAULT;
}

static int item_use_utf_8_action_cb(Ihandle *ih_item)
{
  Ihandle* bt;
  int utf8;
  Ihandle* config = get_config(ih_item);
  if (IupGetInt(ih_item, "VALUE"))
  {
    IupSetGlobal("UTF8MODE", "Yes");
    IupConfigSetVariableStr(config, "IupVisualLED", "UTF-8", "Yes");
    utf8 = 1;

#ifdef USE_UTF8_VC
    IupSetGlobal("UTF8MODE_FILE", "Yes");
    setlocale(LC_ALL, ".UTF8");
#endif
  }
  else
  {
    IupSetGlobal("UTF8MODE", NULL);
    IupConfigSetVariableStr(config, "IupVisualLED", "UTF-8", NULL);
    utf8 = 0;

#ifdef USE_UTF8_VC
    IupSetGlobal("UTF8MODE_FILE", "No");
    setlocale(LC_ALL, "");
#endif
  }

  bt = IupGetDialogChild(ih_item, "BUTTON_WORDWRAP");
  IupSetAttribute(bt, "TITLE", utf8 ? "W\xC2\xAC" : "W\xAC");

  bt = IupGetDialogChild(ih_item, "BUTTON_SHOWWHITE");
  IupSetAttribute(bt, "TITLE", utf8 ? "a\xC2\xB7\x62" : "a\xB7\x62");

  bt = IupGetDialogChild(ih_item, "BUTTON_SHOWEOL");
  IupSetAttribute(bt, "TITLE", utf8 ? "\xC2\xB6" : "\xB6");

  return IUP_DEFAULT;
}

static int item_imageexport_static_action_cb(Ihandle *ih_item)
{
  Ihandle* config = get_config(ih_item);
  if (IupGetInt(ih_item, "VALUE"))
  {
    IupSetGlobal("IMAGEEXPORT_STATIC", "Yes");
    IupConfigSetVariableStr(config, "IupVisualLED", "ImageExportStatic", "Yes");
  }
  else
  {
    IupSetGlobal("IMAGEEXPORT_STATIC", NULL);
    IupConfigSetVariableStr(config, "IupVisualLED", "ImageExportStatic", NULL);
  }
  return IUP_DEFAULT;
}

static int tree_selection_cb(Ihandle* elem_tree, int id, int status)
{
  if (status == 1)
  {
    Ihandle* bt;
    Ihandle* elem = (Ihandle*)IupTreeGetUserId(elem_tree, id);

    Ihandle* properties_dlg = (Ihandle*)IupGetAttribute(IupGetParent(elem_tree), "PROPERTIES_DIALOG");
    if (properties_dlg && IupGetInt(properties_dlg, "VISIBLE"))
      iupLayoutPropertiesUpdate(properties_dlg, elem);

    bt = IupGetDialogChild(elem_tree, "ELEM_LAYOUT_BUTTON");
    if (IupGetDialog(elem))
      IupSetAttribute(bt, "ACTIVE", "Yes");
    else
      IupSetAttribute(bt, "ACTIVE", "No");

    bt = IupGetDialogChild(elem_tree, "ELEM_IMAGE_BUTTON");
    if (elem->iclass->nativetype == IUP_TYPEIMAGE)
      IupSetAttribute(bt, "ACTIVE", "Yes");
    else
      IupSetAttribute(bt, "ACTIVE", "No");
  }
  return IUP_DEFAULT;
}

static void search_in_text(Ihandle* multitext, const char* str)
{
  int pos_start, pos_end, find_start, find_end;

  IupSetAttribute(multitext, "SEARCHFLAGS", "WHOLEWORD");

  find_start = 0;
  find_end = IupGetInt(multitext, "COUNT");

  IupSetInt(multitext, "TARGETSTART", find_start);
  IupSetInt(multitext, "TARGETEND", find_end);

  IupSetAttribute(multitext, "SEARCHINTARGET", str);

  pos_start = IupGetInt(multitext, "TARGETSTART");
  pos_end = IupGetInt(multitext, "TARGETEND");

  if (pos_start != 0 || pos_end != find_end)
  {
    IupSetFocus(multitext);
    IupSetInt(multitext, "CARETPOS", pos_start);
    IupSetInt(multitext, "SCROLLTOPOS", pos_start);
    IupSetfAttribute(multitext, "SELECTIONPOS", "%d:%d", pos_start, pos_end);
  }
}

static int tree_executeleaf_cb(Ihandle* elem_tree, int id)
{
  Ihandle* multitext = vLedGetCurrentMultitext(elem_tree);
  Ihandle *elem = (Ihandle *)IupTreeGetUserId(elem_tree, id);
  char *filename = get_filename(multitext);
  char *name = vLedGetName(elem);
  Ihandle *parent = elem;
  int link;

  while(!name)
  {
    parent = IupGetParent(parent);
    if (!parent)
      break;
    name = vLedGetName(parent);
  }

  if (!name)
    return IUP_DEFAULT;

  if (vLedIsAlien(elem, filename) || iupAttribGet(elem, "_IUPLED_NOTDEF_NAME"))
    return IUP_DEFAULT;

  link = IupGetIntId(elem_tree, "LINK", id);
  if (link)
  {
    int i, root_count = IupGetInt(elem_tree, "ROOTCOUNT");

    id = 0;
    for (i = 0; i < root_count; i++)
    {
      if (elem == (Ihandle *)IupTreeGetUserId(elem_tree, id))
      {
        IupSetInt(elem_tree, "VALUE", id);
        break;
      }

      id = IupGetIntId(elem_tree, "NEXT", id);
    }
  }

  search_in_text(multitext, name);

  return IUP_DEFAULT;
}

static Ihandle* get_elem_tree(Ihandle* dlg)
{
  Ihandle* multitext = vLedGetCurrentMultitext(dlg);
  Ihandle* elem_tree = vLedGetElemTree(multitext);
  return elem_tree;
}

static int elem_locate_cb(Ihandle* ih_item)
{
  Ihandle* elem_tree = !IupClassMatch(ih_item, "item") ? get_elem_tree(ih_item) : (Ihandle*)IupGetAttribute(ih_item, "ELEMENTS_TREE");
  int id = IupGetInt(elem_tree, "VALUE");

  tree_executeleaf_cb(elem_tree, id);

  return IUP_DEFAULT;
}

static int elem_collapse_all_cb(Ihandle* ih_item)
{
  Ihandle* elem_tree = (Ihandle*)IupGetAttribute(ih_item, "ELEMENTS_TREE");
  IupSetAttribute(elem_tree, "EXPANDALL", "NO");
  return IUP_DEFAULT;
}

static int elem_expand_all_cb(Ihandle* ih_item)
{
  Ihandle* elem_tree = (Ihandle*)IupGetAttribute(ih_item, "ELEMENTS_TREE");
  IupSetAttribute(elem_tree, "EXPANDALL", "YES");
  return IUP_DEFAULT;
}

static int elem_goto_parent_cb(Ihandle* ih_item)
{
  Ihandle* elem_tree = (Ihandle*)IupGetAttribute(ih_item, "ELEMENTS_TREE");
  int id = IupGetInt(elem_tree, "VALUE");

  if (IupGetAttributeId(elem_tree, "PARENT", id) != NULL)
  {
    id = IupGetIntId(elem_tree, "PARENT", id);
    IupSetInt(elem_tree, "VALUE", id);
  }

  return IUP_DEFAULT;
}

static int elem_goto_brother_cb(Ihandle* ih_item)
{
  Ihandle* elem_tree = (Ihandle*)IupGetAttribute(ih_item, "ELEMENTS_TREE");
  int id = IupGetInt(elem_tree, "VALUE");

  if (IupGetAttributeId(elem_tree, "NEXT", id) != NULL)
  {
    id = IupGetIntId(elem_tree, "NEXT", id);
    IupSetInt(elem_tree, "VALUE", id);
  }

  return IUP_DEFAULT;
}

static int globalsdlg_cb(Ihandle* ih)
{
  Ihandle* config = get_config(ih);
  Ihandle* dlg = IupGetDialog(ih);
  Ihandle* globals_dlg = (Ihandle*)IupGetAttribute(dlg, "GLOBALS_DIALOG");
  if (!globals_dlg)
  {
    globals_dlg = IupGlobalsDialog();
    IupSetAttributeHandle(globals_dlg, "PARENTDIALOG", dlg);
    IupSetAttribute(dlg, "GLOBALS_DIALOG", (char*)globals_dlg);
  }

  IupConfigSetVariableInt(config, "GlobalsDialog", "Maximized", 0);
  IupConfigDialogShow(config, globals_dlg, "GlobalsDialog");

  return IUP_DEFAULT;
}

static int classinfo_cb(Ihandle* ih)
{
  Ihandle* config = get_config(ih);
  Ihandle* dlg = IupGetDialog(ih);
  Ihandle* classinfo_dlg = (Ihandle*)IupGetAttribute(dlg, "CLASSINFO_DIALOG");
  if (!classinfo_dlg)
  {
    classinfo_dlg = IupClassInfoDialog(dlg);
    IupSetAttribute(dlg, "CLASSINFO_DIALOG", (char*)classinfo_dlg);
  }

  IupConfigSetVariableInt(config, "ClassInfoDialog", "Maximized", 0);
  IupConfigDialogShow(config, classinfo_dlg, "ClassInfoDialog");

  return IUP_DEFAULT;
}

static void iLayoutFixChildLedLines(Ihandle* parent, const char* filename)
{
  Ihandle *child;

  int parent_line = iupAttribGetInt(parent, "_IUPLED_LINE");

  for (child = parent->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_INTERNAL))
    {
      if (!vLedIsAlien(child, filename) && vLedGetName(child))
        iupAttribSetInt(child, "_IUPLED_LINE", parent_line - 1);

      iLayoutFixChildLedLines(child, filename);
    }
  }
}

static void update_led_line(Ihandle* elem, const char *filename)
{
  if (!iupAttribGet(elem, "_IUPLED_LINE"))
  {
    Ihandle* parent = elem->parent;
    while (parent && !vLedIsAlien(parent, filename))
    {
      int parent_line = iupAttribGetInt(parent, "_IUPLED_LINE");
      if (parent_line)
      {
        iupAttribSetInt(elem, "_IUPLED_LINE", parent_line - 1);
        break;
      }
      parent = parent->parent;
    }
  }
  else
  {
    Ihandle* parent = elem->parent;
    int elem_line = iupAttribGetInt(elem, "_IUPLED_LINE");
    int rebuild_lines = 0;
    while (parent && !vLedIsAlien(parent, filename))
    {
      int parent_line = iupAttribGetInt(parent, "_IUPLED_LINE");
      if (parent_line < elem_line)  /* inconsistent */
      {
        while (parent->parent && !vLedIsAlien(parent->parent, filename))
          parent = parent->parent;

        rebuild_lines = 1;
        break;
      }
      parent = parent->parent;
    }

    if (rebuild_lines)
      iLayoutFixChildLedLines(parent, filename);
  }
}

static int layoutchanged_cb(Ihandle* dlg, Ihandle* elem)
{
  Ihandle* multitext = (Ihandle*)IupGetAttribute(dlg, "MULTITEXT");

  if (elem)
  {
    char *filename = get_filename(multitext);

    if (!iupAttribGet(elem, "_IUPLED_FILENAME")) /* new element */
      iupAttribSetStr(elem, "_IUPLED_FILENAME", filename);

    if (vLedGetName(elem))  /* if it has name then it must have a line for controlling export order */
      update_led_line(elem, filename);
  }

  rewrite_led(multitext);

  return IUP_DEFAULT;
}

static int attribchanged_cb(Ihandle* dlg, char* name)  /* called for layout_dlg and properties_dlg */
{
  Ihandle* multitext = (Ihandle*)IupGetAttribute(dlg, "MULTITEXT");
  Ihandle* elem = (Ihandle*)IupGetAttribute(dlg, "ELEM");
  char led_name[200] = "_IUPLED_SAVED_";
  strcat(led_name, name);
  iupAttribSet(elem, led_name, "1");

  rewrite_led(multitext);

  return IUP_DEFAULT;
}

static int elem_layoutdlg_cb(Ihandle* ih_item)
{
  Ihandle* elem_tree = !IupClassMatch(ih_item, "item") ? get_elem_tree(ih_item) : (Ihandle*)IupGetAttribute(ih_item, "ELEMENTS_TREE");
  Ihandle* multitext = vLedGetCurrentMultitext(elem_tree);
  int id = IupGetInt(elem_tree, "VALUE");
  Ihandle *elem = (Ihandle *)IupTreeGetUserId(elem_tree, id);
  Ihandle* dialog = IupGetDialog(elem);
  if (dialog)
  {
    Ihandle* layout_dlg = IupLayoutDialog(dialog);
    IupSetCallback(layout_dlg, "ATTRIBCHANGED_CB", (Icallback)attribchanged_cb);
    IupSetCallback(layout_dlg, "LAYOUTCHANGED_CB", (Icallback)layoutchanged_cb);
    IupSetAttribute(layout_dlg, "MULTITEXT", (char*)multitext);

    IupShow(layout_dlg);  /* LayoutDialog is automatically destroyed on close */
  }
  else
    IupMessageError(IupGetDialog(elem_tree), "The element must be a dialog or be inside a dialog.");

  return IUP_DEFAULT;
}

static int item_newlayout_cb(Ihandle* ih_bt)
{
  Ihandle* layout_dlg, *new_dialog;
  Ihandle* multitext = vLedGetCurrentMultitext(ih_bt);
  char *filename = get_filename(multitext);
  Ihandle* elem_tree = vLedGetElemTree(multitext);
  char new_name[50];
  int caret_line, caret_pos;
  static int new_dlg_count = 1;

  sprintf(new_name, "new_dialog%d", new_dlg_count);
  new_dlg_count++;

  caret_line = IupGetInt(multitext, "CARET");
  IupTextConvertLinColToPos(multitext, caret_line, 0, &caret_pos);
  IupSetStrfId(multitext, "INSERT", caret_pos, "%s = DIALOG()\n\n", new_name);

  new_dialog = IupDialog(NULL);
  IupSetHandle(new_name, new_dialog);
  iupAttribSetStr(new_dialog, "_IUPLED_FILENAME", filename);
  iupAttribSetInt(new_dialog, "_IUPLED_LINE", caret_line);

  updateElemTree(elem_tree, filename);

  layout_dlg = IupLayoutDialog(new_dialog);
  IupSetCallback(layout_dlg, "ATTRIBCHANGED_CB", (Icallback)attribchanged_cb);
  IupSetCallback(layout_dlg, "LAYOUTCHANGED_CB", (Icallback)layoutchanged_cb);
  IupSetAttribute(layout_dlg, "MULTITEXT", (char*)multitext);

  IupShow(layout_dlg);  /* LayoutDialog is automatically destroyed on close */

  return IUP_DEFAULT;
}

static int imagechanged_cb(Ihandle* image_editor_dlg)
{
  Ihandle* dlg = IupGetAttributeHandle(image_editor_dlg, "PARENTDIALOG");
  Ihandle* multitext = vLedGetCurrentMultitext(dlg);
  rewrite_led(multitext);
  return IUP_DEFAULT;
}

static int elem_imagedlg_cb(Ihandle* ih_item)
{
  Ihandle* elem_tree = !IupClassMatch(ih_item, "item") ? get_elem_tree(ih_item) : (Ihandle*)IupGetAttribute(ih_item, "ELEMENTS_TREE");
  Ihandle* multitext = vLedGetCurrentMultitext(elem_tree);
  int id = IupGetInt(elem_tree, "VALUE");
  Ihandle *elem = (Ihandle *)IupTreeGetUserId(elem_tree, id);
  Ihandle* config = get_config(elem_tree);

  Ihandle* image_editor_dlg = (Ihandle*)IupGetAttribute(IupGetDialog(multitext), "IMAGE_EDITOR_DIALOG");
  if (!image_editor_dlg)
  {
    image_editor_dlg = vLedImageEditorCreate(IupGetDialog(elem_tree), config);
    IupSetCallback(image_editor_dlg, "IMAGECHANGED_CB", (Icallback)imagechanged_cb);
    IupSetAttribute(IupGetDialog(multitext), "IMAGE_EDITOR_DIALOG", (char*)image_editor_dlg);
  }

  IupConfigSetVariableInt(config, "ImageEditorDialog", "Maximized", 0);
  IupConfigDialogShow(config, image_editor_dlg, "ImageEditorDialog");

  vLedImageEditorSetImage(image_editor_dlg, elem, IupGetName(elem));

  IupPopup(image_editor_dlg, IUP_CURRENT, IUP_CURRENT);

  return IUP_DEFAULT;
}

static int item_newimage_cb(Ihandle* ih_item)
{
  Ihandle* multitext = vLedGetCurrentMultitext(ih_item);
  char *filename = get_filename(multitext);
  Ihandle* config = get_config(ih_item);
  Ihandle* image_editor_dlg = (Ihandle*)IupGetAttribute(IupGetDialog(multitext), "IMAGE_EDITOR_DIALOG");
  if (!image_editor_dlg)
  {
    image_editor_dlg = vLedImageEditorCreate(IupGetDialog(multitext), config);
    IupSetCallback(image_editor_dlg, "IMAGECHANGED_CB", (Icallback)imagechanged_cb);
    IupSetAttribute(IupGetDialog(multitext), "IMAGE_EDITOR_DIALOG", (char*)image_editor_dlg);
  }

  IupConfigSetVariableInt(config, "ImageEditorDialog", "Maximized", 0);
  IupConfigDialogShow(config, image_editor_dlg, "ImageEditorDialog");

  vLedImageEditorNewImage(image_editor_dlg, filename);

  IupPopup(image_editor_dlg, IUP_CURRENT, IUP_CURRENT);

  return IUP_DEFAULT;
}

static int elem_propertiesdlg_cb(Ihandle* ih_item)
{
  Ihandle* elem_tree = !IupClassMatch(ih_item, "item") ? get_elem_tree(ih_item) : (Ihandle*)IupGetAttribute(ih_item, "ELEMENTS_TREE");
  Ihandle* multitext = vLedGetCurrentMultitext(elem_tree);
  int id = IupGetInt(elem_tree, "VALUE");
  Ihandle *elem = (Ihandle *)IupTreeGetUserId(elem_tree, id);
  Ihandle* config = get_config(elem_tree);
  Ihandle* properties_dlg = (Ihandle*)IupGetAttribute(IupGetParent(elem_tree), "PROPERTIES_DIALOG");
  if (!properties_dlg)
  {
    properties_dlg = IupElementPropertiesDialog(IupGetDialog(elem_tree), elem);
    IupSetCallback(properties_dlg, "ATTRIBCHANGED_CB", (Icallback)attribchanged_cb);
    IupSetAttribute(IupGetParent(elem_tree), "PROPERTIES_DIALOG", (char*)properties_dlg);
  }
  else
    iupLayoutPropertiesUpdate(properties_dlg, elem);

  IupSetAttribute(properties_dlg, "ELEM", (char*)elem);
  IupSetAttribute(properties_dlg, "MULTITEXT", (char*)multitext);

  IupConfigSetVariableInt(config, "ElementPropertiesDialog", "Maximized", 0);
  IupConfigDialogShow(config, properties_dlg, "ElementPropertiesDialog");
  return IUP_DEFAULT;
}

static int elem_show_cb(Ihandle* ih_item)
{
  Ihandle* elem_tree = (Ihandle*)IupGetAttribute(ih_item, "ELEMENTS_TREE");
  int id = IupGetInt(elem_tree, "VALUE");
  Ihandle *elem = (Ihandle *)IupTreeGetUserId(elem_tree, id);
  Ihandle* dialog = IupGetDialog(elem);
  if (dialog)
    IupShow(dialog);
  else
  {
    if (IupClassMatch(elem, "menu"))
      IupPopup(elem, IUP_MOUSEPOS, IUP_MOUSEPOS);
    else
      IupMessageError(IupGetDialog(elem_tree), "Will only show dialogs and independent menus.");
  }
  return IUP_DEFAULT;
}

static int elem_hide_cb(Ihandle* ih_item)
{
  Ihandle* elem_tree = (Ihandle*)IupGetAttribute(ih_item, "ELEMENTS_TREE");
  int id = IupGetInt(elem_tree, "VALUE");
  Ihandle *elem = (Ihandle *)IupTreeGetUserId(elem_tree, id);
  Ihandle* dialog = IupGetDialog(elem);
  if (dialog && IupGetInt(dialog, "VISIBLE"))
    IupHide(dialog);
  return IUP_DEFAULT;
}

static int elem_find_cb(Ihandle* ih_item)
{
  Ihandle* elem_tree = !IupClassMatch(ih_item, "item") ? get_elem_tree(ih_item) : (Ihandle*)IupGetAttribute(ih_item, "ELEMENTS_TREE");
  Ihandle* config = get_config(elem_tree);
  Ihandle* find_dlg = (Ihandle*)IupGetAttribute(IupGetParent(elem_tree), "FIND_ELEM_DIALOG");
  int id = IupGetInt(elem_tree, "VALUE");
  Ihandle *dialog;
  Ihandle *elem = (Ihandle *)IupTreeGetUserId(elem_tree, id);
  if (!elem)
    return IUP_DEFAULT;

  if (!find_dlg)
  {
    find_dlg = iupLayoutFindElementDialog(elem_tree, elem);
    IupSetAttribute(IupGetParent(elem_tree), "FIND_ELEM_DIALOG", (char*)find_dlg);

    IupSetStrAttribute(IupGetDialogChild(find_dlg, "FIND_TEXT"), "VALUE", IupConfigGetVariableStr(config, "FindElementDialog", "FindText"));
    IupSetAttribute(IupGetDialogChild(find_dlg, "TARGET"), "VALUE_HANDLE", (char*)IupGetDialogChild(find_dlg, IupConfigGetVariableStr(config, "FindElementDialog", "Target")));
  }

  dialog = IupGetDialog(elem);
  if (!dialog)
    IupSetAttribute(IupGetDialogChild(find_dlg, "FIND_NAME"), "ACTIVE", "NO");
  else
    IupSetAttribute(IupGetDialogChild(find_dlg, "FIND_NAME"), "ACTIVE", "YES");

  IupConfigSetVariableInt(config, "FindElementDialog", "Maximized", 0);
  IupConfigDialogShow(config, find_dlg, "FindElementDialog");

  return IUP_DEFAULT;
}

static void show_elements_menu(Ihandle* elem_tree, int id, int x, int y)
{
  Ihandle *popup_menu;
  Ihandle *elem = (Ihandle *)IupTreeGetUserId(elem_tree, id);
  int has_dialog = 0, is_popup_menu = 0, is_image = 0;

  if (IupGetDialog(elem))
    has_dialog = 1;

  if (!has_dialog && IupClassMatch(elem, "menu"))  /* not inside a dialog, it is a popup menu */
    is_popup_menu = 1;

  if (elem->iclass->nativetype == IUP_TYPEIMAGE)
    is_image = 1;

  IupSetInt(elem_tree, "VALUE", id);

  popup_menu = IupMenu(
    IupSetCallbacks(IupItem("Locate in LED\tEnter", NULL), "ACTION", elem_locate_cb, NULL),  /* same as executeleaf_cb */
    IupSeparator(),
    IupSetCallbacks(IupItem("Collapse All", NULL), "ACTION", elem_collapse_all_cb, NULL),
    IupSetCallbacks(IupItem("Expand All", NULL), "ACTION", elem_expand_all_cb, NULL),
    IupSetCallbacks(IupItem("Go to Parent\tLeft", NULL), "ACTION", elem_goto_parent_cb, NULL),
    IupSetCallbacks(IupItem("Go to Brother\tDown", NULL), "ACTION", elem_goto_brother_cb, NULL),
    IupSeparator(),
    has_dialog ? IupSetCallbacks(IupItem("Show Dialog", NULL), "ACTION", elem_show_cb, NULL) : (is_popup_menu ? IupSetCallbacks(IupItem("Popup Menu", NULL), "ACTION", elem_show_cb, NULL) : IupSetAttributes(IupItem("Show", NULL), "ACTIVE=NO")),
    has_dialog ? IupSetCallbacks(IupItem("Hide Dialog", NULL), "ACTION", elem_hide_cb, NULL) : IupSetAttributes(IupItem("Hide", NULL), "ACTIVE=NO"),
    IupSeparator(),
    IupSetAttributes(IupSetCallbacks(IupItem("Edit Dialog Layout...", NULL), "ACTION", (Icallback)elem_layoutdlg_cb, NULL), has_dialog ? "ACTIVE=YES" : "ACTIVE=NO"),
    IupSetAttributes(IupSetCallbacks(IupItem("Edit Image...", NULL), "ACTION", (Icallback)elem_imagedlg_cb, NULL), is_image ? "ACTIVE=YES" : "ACTIVE=NO"),
    IupSeparator(),
    IupSetCallbacks(IupItem("Element Properties...", NULL), "ACTION", elem_propertiesdlg_cb, NULL),
    IupSetCallbacks(IupItem("Find Element...\tCtrl+E", "findElement"), "ACTION", elem_find_cb, NULL),
    NULL);

  iupAttribSet(popup_menu, "ELEMENTS_TREE", (char*)elem_tree);

  IupPopup(popup_menu, x, y);

  IupDestroy(popup_menu);
}

static int tree_rightclick_cb(Ihandle* elem_tree, int id)
{
  IupSetInt(elem_tree, "VALUE", id);
  show_elements_menu(elem_tree, id, IUP_MOUSEPOS, IUP_MOUSEPOS);
  return IUP_DEFAULT;
}

static int bt_show_elem_tools_cb(Ihandle* bt)
{
  Ihandle* elem_tree = get_elem_tree(bt);
  int x = IupGetInt(bt, "X");
  int y = IupGetInt(bt, "Y");
  int h = IupGetInt2(bt, "RASTERSIZE");
  int id = IupGetInt(elem_tree, "VALUE");
  if (id < 0) id = 0;
  y += h;
  show_elements_menu(elem_tree, id, x, y);
  return IUP_DEFAULT;
}

static Ihandle* buildLedMenu(Ihandle* config)
{
  Ihandle *item_load, *item_unload, *item_autoload, *item_autocomplete, *item_style_config, *item_expand, *item_toggle, *item_level,
    *item_folding, *item_toggle_folding, *ledMenu, *item_collapse, *item_rewrite,
    *item_comment, *item_uncomment, *item_newlayout, *item_newimage;

  item_autoload = IupItem("Auto Load (Open or Save)", NULL);
  IupSetAttribute(item_autoload, "AUTOTOGGLE", "YES");
  IupSetAttribute(item_autoload, "NAME", "ITM_LOAD");
  IupSetCallback(item_autoload, "ACTION", (Icallback)item_autoload_action_cb);
  if (IupConfigGetVariableIntDef(config, "IupVisualLED", "AutoLoad", 1))
    IupSetAttribute(item_autoload, "VALUE", "ON");
  else
    IupSetAttribute(item_autoload, "VALUE", "OFF");

  item_load = IupItem("Load\tF5", NULL);
  IupSetCallback(item_load, "ACTION", (Icallback)item_load_action_cb);
  IupSetAttribute(item_load, "IMAGE", "IUP_NavigateRefresh");

  item_unload = IupItem("Unload", NULL);
  IupSetCallback(item_unload, "ACTION", (Icallback)item_unload_action_cb);

  item_rewrite = IupItem("Rewrite (from elements)", NULL);
  IupSetAttribute(item_rewrite, "NAME", "ITM_REWRITE");
  IupSetCallback(item_rewrite, "ACTION", (Icallback)item_rewrite_action_cb);

  item_newlayout = IupItem("New Dialog Layout...", NULL);
  IupSetCallback(item_newlayout, "ACTION", (Icallback)item_newlayout_cb);

  item_newimage = IupItem("New Image...", NULL);
  IupSetCallback(item_newimage, "ACTION", (Icallback)item_newimage_cb);

  item_autocomplete = IupItem("Auto Completion", NULL);
  IupSetAttribute(item_autocomplete, "NAME", "ITM_AUTOCOMPLETE");
  IupSetCallback(item_autocomplete, "ACTION", (Icallback)item_autocomplete_action_cb);

  item_style_config = IupItem("Syntax Colors...", NULL);
  IupSetAttribute(item_style_config, "NAME", "ITM_STYLE");
  IupSetCallback(item_style_config, "ACTION", (Icallback)item_style_config_action_cb);

  item_folding = IupItem("Folding", NULL);
  IupSetAttribute(item_folding, "NAME", "ITM_FOLDING");
  IupSetCallback(item_folding, "ACTION", (Icallback)item_folding_action_cb);

  item_toggle_folding = IupItem("Toggle Fold\tF8", NULL);
  IupSetAttribute(item_toggle_folding, "NAME", "ITM_TOGGLE_FOLDING");
  IupSetCallback(item_toggle_folding, "ACTION", (Icallback)item_toggle_folding_action_cb);

  item_collapse = IupItem("Collapse", NULL);
  IupSetAttribute(item_collapse, "NAME", "ITM_COLLAPSE");
  IupSetCallback(item_collapse, "ACTION", (Icallback)item_fold_collapse_action_cb);

  item_expand = IupItem("Expand", NULL);
  IupSetAttribute(item_expand, "NAME", "ITM_EXPAND");
  IupSetCallback(item_expand, "ACTION", (Icallback)item_fold_expand_action_cb);

  item_toggle = IupItem("Toggle", NULL);
  IupSetAttribute(item_toggle, "NAME", "ITM_TOGGLE");
  IupSetCallback(item_toggle, "ACTION", (Icallback)item_fold_toggle_action_cb);

  item_level = IupItem("by Level...", NULL);
  IupSetAttribute(item_level, "NAME", "ITM_LEVEL");
  IupSetCallback(item_level, "ACTION", (Icallback)item_fold_level_action_cb);

  item_comment = IupItem("Comment\tCtrl+K", NULL);
  IupSetCallback(item_comment, "ACTION", (Icallback)item_comment_action_cb);

  item_uncomment = IupItem("Uncomment\tCtrl+Q", NULL);
  IupSetCallback(item_uncomment, "ACTION", (Icallback)item_uncomment_action_cb);

  ledMenu = IupMenu(
    item_autoload,
    item_load,
    item_unload,
    IupSeparator(),
    item_newlayout,
    item_newimage,
    item_rewrite,
    IupSeparator(),
    item_comment,
    item_uncomment,
    IupSeparator(),
    item_folding,
    item_toggle_folding,
    IupSubmenu("Fold All",
    IupSetAttributes(IupMenu(
      item_collapse,
      item_expand,
      item_toggle,
      item_level,
      NULL), "NAME=ITM_FOLD_ALL")),
    IupSeparator(),
    item_autocomplete,
    item_style_config,
    NULL);

  IupSetCallback(ledMenu, "OPEN_CB", (Icallback)led_menu_open_cb);

  return IupSubmenu("&LED", ledMenu);
}

static Ihandle* buildToolsMenu(void)
{
  Ihandle *item_import_img, *item_export_img, *item_show_all_img, 
    *item_export_led, *item_export_open_led, *item_export_proj_led,
    *item_export_lua, *item_export_open_lua, *item_export_proj_lua,
    *item_export_c, *item_export_open_c, *item_export_proj_c,
    *item_use_utf8, *toolsMenu, *item_imageexport_static, *item_show_stock_img;

  item_import_img = IupItem("Import Images...", NULL);
  IupSetAttribute(item_import_img, "NAME", "ITM_IMP_IMG");
  IupSetCallback(item_import_img, "ACTION", (Icallback)item_import_img_action_cb);

  item_export_img = IupItem("Export Images...", NULL);
  IupSetAttribute(item_export_img, "NAME", "ITM_EXP_IMG");
  IupSetCallback(item_export_img, "ACTION", (Icallback)item_export_img_cb);

  item_show_all_img = IupItem("Show All Images...", NULL);
  IupSetAttribute(item_show_all_img, "NAME", "ITM_SHOW_ALL_IMG");
  IupSetCallback(item_show_all_img, "ACTION", (Icallback)item_show_all_img_cb);

  item_show_stock_img = IupItem("Show Stock Images...", NULL);
  IupSetCallback(item_show_stock_img, "ACTION", (Icallback)item_show_stock_img_cb);

  item_export_led = IupItem("Export to LED...", NULL);
  IupSetAttribute(item_export_led, "NAME", "ITM_EXP_LED");
  IupSetCallback(item_export_led, "ACTION", (Icallback)item_export_led_action_cb);

  item_export_open_led = IupItem("Export All Open to LED(s)...", NULL);
  IupSetAttribute(item_export_open_led, "NAME", "ITM_EXPORT_OPEN_LED");
  IupSetCallback(item_export_open_led, "ACTION", (Icallback)item_export_all_open_action_cb);

  item_export_proj_led = IupItem("Export All Project to LED(s)...", NULL);
  IupSetAttribute(item_export_proj_led, "NAME", "ITM_EXP_PROJ_LED");
  IupSetCallback(item_export_proj_led, "ACTION", (Icallback)item_export_proj_action_cb);

  item_export_lua = IupItem("Export to Lua...", NULL);
  IupSetAttribute(item_export_lua, "NAME", "ITM_EXP_LUA");
  IupSetCallback(item_export_lua, "ACTION", (Icallback)item_export_lua_action_cb);

  item_export_open_lua = IupItem("Export All Open to Lua(s)...", NULL);
  IupSetAttribute(item_export_open_lua, "NAME", "ITM_EXPORT_OPEN_LUA");
  IupSetCallback(item_export_open_lua, "ACTION", (Icallback)item_export_all_open_action_cb);

  item_export_proj_lua = IupItem("Export All Project to Lua(s)...", NULL);
  IupSetAttribute(item_export_proj_lua, "NAME", "ITM_EXP_PROJ_LUA");
  IupSetCallback(item_export_proj_lua, "ACTION", (Icallback)item_export_proj_action_cb);

  item_export_c = IupItem("Export to C...", NULL);
  IupSetAttribute(item_export_c, "NAME", "ITM_EXP_C");
  IupSetCallback(item_export_c, "ACTION", (Icallback)item_export_c_action_cb);

  item_export_open_c = IupItem("Export All Open to C(s)...", NULL);
  IupSetAttribute(item_export_open_c, "NAME", "ITM_EXPORT_OPEN_C");
  IupSetCallback(item_export_open_c, "ACTION", (Icallback)item_export_all_open_action_cb);

  item_export_proj_c = IupItem("Export All Project To C(s)...", NULL);
  IupSetAttribute(item_export_proj_c, "NAME", "ITM_EXP_PROJ_C");
  IupSetCallback(item_export_proj_c, "ACTION", (Icallback)item_export_proj_action_cb);

  item_use_utf8 = IupItem("Use UTF-8", NULL);
  IupSetAttribute(item_use_utf8, "AUTOTOGGLE", "Yes");
  IupSetCallback(item_use_utf8, "ACTION", (Icallback)item_use_utf_8_action_cb);
  if (IupGetInt(NULL, "UTF8MODE"))
  IupSetAttribute(item_use_utf8, "VALUE", "ON");
  else
  IupSetAttribute(item_use_utf8, "VALUE", "OFF");

  item_imageexport_static = IupItem("Image Export Use \"static\"", NULL);
  IupSetAttribute(item_imageexport_static, "AUTOTOGGLE", "Yes");
  IupSetCallback(item_imageexport_static, "ACTION", (Icallback)item_imageexport_static_action_cb);
  if (IupGetInt(NULL, "IMAGEEXPORT_STATIC"))
  IupSetAttribute(item_imageexport_static, "VALUE", "ON");
  else
  IupSetAttribute(item_imageexport_static, "VALUE", "OFF");

  toolsMenu = IupMenu(
    IupSetCallbacks(IupItem("Globals...", NULL), "ACTION", globalsdlg_cb, NULL),
    IupSetCallbacks(IupItem("Class Info...", NULL), "ACTION", classinfo_cb, NULL),
    IupSeparator(),
    item_import_img,
    item_export_img,
    item_show_all_img,
    item_show_stock_img,
    IupSeparator(),
    item_export_led,
    item_export_open_led,
    item_export_proj_led,
    IupSeparator(),
    item_export_lua,
    item_export_open_lua,
    item_export_proj_lua,
    IupSeparator(),
    item_export_c,
    item_export_open_c,
    item_export_proj_c,
    IupSeparator(),
    item_imageexport_static,
    item_use_utf8,
    NULL);

  IupSetCallback(toolsMenu, "OPEN_CB", (Icallback)tools_menu_open_cb);

  return IupSubmenu("&Tools", toolsMenu);
}

static Ihandle* buildElemToolbar(void)
{
  Ihandle* elem_but_box;
  Ihandle* btn_tools, *bt_prop, *bt_find, *bt_locate, *bt_layout, *bt_image;

  btn_tools = IupButton(NULL, NULL);
  IupSetAttribute(btn_tools, "IMAGE", "IUP_ToolsSettings");
  IupSetAttribute(btn_tools, "FLAT", "Yes");
  IupSetCallback(btn_tools, "ACTION", (Icallback)bt_show_elem_tools_cb);
  IupSetAttribute(btn_tools, "TIP", "Elements Tools\n(same as right click the tree)");
  IupSetAttribute(btn_tools, "CANFOCUS", "No");

  bt_locate = IupButton(NULL, NULL);
  IupSetAttribute(bt_locate, "IMAGE", "IupVLedGotoCmd");
  IupSetAttribute(bt_locate, "FLAT", "Yes");
  IupSetCallback(bt_locate, "ACTION", (Icallback)elem_locate_cb);
  IupSetAttribute(bt_locate, "TIP", "Locate in LED");
  IupSetAttribute(bt_locate, "CANFOCUS", "No");

  bt_layout = IupButton(NULL, NULL);
  IupSetAttribute(bt_layout, "IMAGE", "IupVLedLayout");
  IupSetAttribute(bt_layout, "NAME", "ELEM_LAYOUT_BUTTON");
  IupSetAttribute(bt_layout, "FLAT", "Yes");
  IupSetCallback(bt_layout, "ACTION", (Icallback)elem_layoutdlg_cb);
  IupSetAttribute(bt_layout, "TIP", "Edit Dialog Layout...");
  IupSetAttribute(bt_layout, "CANFOCUS", "No");

  bt_image = IupButton(NULL, NULL);
  IupSetAttribute(bt_image, "IMAGE", "IupVLedImageEditor");
  IupSetAttribute(bt_image, "FLAT", "Yes");
  IupSetAttribute(bt_image, "NAME", "ELEM_IMAGE_BUTTON");
  IupSetCallback(bt_image, "ACTION", (Icallback)elem_imagedlg_cb);
  IupSetAttribute(bt_image, "TIP", "Edit Image...");
  IupSetAttribute(bt_image, "CANFOCUS", "No");

  bt_prop = IupButton(NULL, NULL);
  IupSetAttribute(bt_prop, "IMAGE", "IUP_FileProperties");
  IupSetAttribute(bt_prop, "FLAT", "Yes");
  IupSetCallback(bt_prop, "ACTION", (Icallback)elem_propertiesdlg_cb);
  IupSetAttribute(bt_prop, "TIP", "Element Properties...");
  IupSetAttribute(bt_prop, "CANFOCUS", "No");

  bt_find = IupButton(NULL, NULL);
  IupSetAttribute(bt_find, "IMAGE", "IUP_EditFind");
  IupSetAttribute(bt_find, "FLAT", "Yes");
  IupSetCallback(bt_find, "ACTION", (Icallback)elem_find_cb);
  IupSetAttribute(bt_find, "TIP", "Find Element...\tCtrl+E");
  IupSetAttribute(bt_find, "CANFOCUS", "No");

  elem_but_box = IupVbox(
    btn_tools,
    IupSetAttributes(IupSpace(), "RASTERSIZE=0x8"),
    IupSetAttributes(IupLabel(NULL), "SEPARATOR=HORIZONTAL"),
    IupSetAttributes(IupSpace(), "RASTERSIZE=0x8"),
    bt_locate,
    bt_layout,
    bt_image,
    bt_prop,
    bt_find,
    NULL
  );

  return elem_but_box;
}

static void addToolbarButtons(Ihandle* toolbar)
{
  Ihandle* bt;

  IupAppend(toolbar, IupSetAttributes(IupLabel(NULL), "SEPARATOR=VERTICAL"));

  bt = IupButton(NULL, NULL);
  IupSetAttribute(bt, "IMAGE", "IUP_NavigateRefresh");
  IupSetAttribute(bt, "FLAT", "Yes");
  IupSetCallback(bt, "ACTION", (Icallback)item_load_action_cb);
  IupSetAttribute(bt, "TIP", "Re-load the LED and build its elements (F5)");
  IupSetAttribute(bt, "CANFOCUS", "No");
  IupAppend(toolbar, bt);

  bt = IupButton(NULL, NULL);
  IupSetAttribute(bt, "IMAGE", "IupVLedNewLayout");
  IupSetAttribute(bt, "FLAT", "Yes");
  IupSetCallback(bt, "ACTION", (Icallback)item_newlayout_cb);
  IupSetAttribute(bt, "TIP", "New Dialog Layout\n(insert a new dialog at caret and starts editing its layout)");
  IupSetAttribute(bt, "CANFOCUS", "No");
  IupAppend(toolbar, bt);

  bt = IupButton(NULL, NULL);
  IupSetAttribute(bt, "IMAGE", "IupVLedNewImage");
  IupSetAttribute(bt, "FLAT", "Yes");
  IupSetCallback(bt, "ACTION", (Icallback)item_newimage_cb);
  IupSetAttribute(bt, "TIP", "Creates a new image");
  IupSetAttribute(bt, "CANFOCUS", "No");
  IupAppend(toolbar, bt);

  IupAppend(toolbar, IupSetAttributes(IupLabel(NULL), "SEPARATOR=VERTICAL"));

  bt = IupButton("#", NULL);
  IupSetAttribute(bt, "FLAT", "Yes");
  IupSetAttribute(bt, "FONTSTYLE", "Bold");
  IupSetAttribute(bt, "PADDING", "4x0");
  IupSetCallback(bt, "ACTION", (Icallback)item_comment_action_cb);
  IupSetAttribute(bt, "TIP", "Comment one or more lines (Ctrl+K)");
  IupSetInt(bt, "FONTSIZE", IupGetInt(bt, "FONTSIZE") + 2);
  IupSetAttribute(bt, "CANFOCUS", "No");
  IupAppend(toolbar, bt);

  bt = IupButton("#", NULL);
  IupSetAttribute(bt, "FLAT", "Yes");
  IupSetAttribute(bt, "FGCOLOR", "128 80 80");
  IupSetAttribute(bt, "PADDING", "4x0");
  IupSetCallback(bt, "ACTION", (Icallback)item_uncomment_action_cb);
  IupSetAttribute(bt, "TIP", "Uncomment one or more lines (Ctrl+Q)");
  IupSetInt(bt, "FONTSIZE", IupGetInt(bt, "FONTSIZE") + 2);
  IupSetAttribute(bt, "CANFOCUS", "No");
  IupAppend(toolbar, bt);
}

static int scidlg_exit_cb(Ihandle* main_dialog)
{
  int i;
  Ihandle *dlg;
  int count = iupDlgListVisibleCount();

  /* hide all other dialogs, some non application dialogs may be open */

  for (dlg = iupDlgListFirst(), i = 0; dlg && i < count; dlg = iupDlgListNext())
  {
    if (dlg != main_dialog && (dlg->handle && IupGetInt(dlg, "VISIBLE")))
    {
      IupHide(dlg);
      i++;
    }
  }

  return IUP_DEFAULT;
}


/**********************************************************************/


int main(int argc, char **argv)
{
  Ihandle *main_dialog, *menu, *config;
  Ihandle *ledMenu, *toolsMenu, *elem_but_box;
  Ihandle *panelTabs, *multitextTabs;
  Ihandle *elem_tree_box, *elementsFrame;
  Ihandle* extra_dlg, *toolbar;
  Icallback oldTabChange_cb;
  int i;

  IupOpen(&argc, &argv);
  IupImageLibOpen();
  IupScintillaOpen();
  IupControlsOpen();
  IupImOpen();
#ifndef USE_NO_OPENGL  
  IupGLCanvasOpen();
  IupGLControlsOpen();
#endif  
#ifndef USE_NO_WEB
  IupWebBrowserOpen();
#endif  
#ifndef USE_NO_PLOT
  IupPlotOpen();
#endif  

#ifdef _DEBUG
  IupSetGlobal("GLOBALLAYOUTDLGKEY", "Yes");
#endif

  vLedLoadImages();

  config = IupConfig();
  IupSetAttribute(config, "APP_NAME", "iupvled");
  IupConfigLoad(config);

  IupSetGlobal("UTF8MODE", IupConfigGetVariableStr(config, "IupVisualLED", "UTF-8"));
  IupSetGlobal("IMAGEEXPORT_STATIC", IupConfigGetVariableStr(config, "IupVisualLED", "ImageExportStatic"));

#ifdef USE_UTF8_VC
  if (IupGetInt(NULL, "UTF8MODE"))
  {
    IupSetGlobal("UTF8MODE_FILE", "Yes");
    setlocale(LC_ALL, ".UTF8");
  }
#endif

  main_dialog = IupScintillaDlg();
  IupSetAttributeHandle(NULL, "PARENTDIALOG", main_dialog);

  ledMenu = buildLedMenu(config);
  menu = IupGetAttributeHandle(main_dialog, "MENU");
  IupInsert(menu, IupGetChild(menu, IupGetChildCount(menu) - 1), ledMenu);

  toolsMenu = buildToolsMenu();
  IupInsert(menu, IupGetChild(menu, IupGetChildCount(menu) - 1), toolsMenu);

  panelTabs = IupGetDialogChild(main_dialog, "PANEL_TABS");
  multitextTabs = IupGetDialogChild(main_dialog, "MULTITEXT_TABS");

  IupSetCallback(main_dialog, "NEWFILENAME_CB", (Icallback)scidlg_newfilename_cb);
  IupSetCallback(main_dialog, "LOADFILE_CB", (Icallback)scidlg_loadfile_cb);
  IupSetCallback(main_dialog, "SAVEFILE_CB", (Icallback)scidlg_savefile_cb);
  IupSetCallback(main_dialog, "NEWTEXT_CB", (Icallback)scidlg_newtext_cb);
  IupSetCallback(main_dialog, "CLOSETEXT_CB", (Icallback)scidlg_closetext_cb);
  IupSetCallback(main_dialog, "CONFIGSAVE_CB", (Icallback)scidlg_configsave_cb);
  IupSetCallback(main_dialog, "CONFIGLOAD_CB", (Icallback)scidlg_configload_cb);
  IupSetCallback(main_dialog, "MARKERCHANGED_CB", (Icallback)scidlg_markerchanged_cb);
  IupSetCallback(main_dialog, "EXIT_CB", (Icallback)scidlg_exit_cb);
  IupSetCallback(main_dialog, "K_cE", (Icallback)elem_find_cb);
  IupSetCallback(main_dialog, "K_F5", (Icallback)item_load_action_cb);
  IupSetCallback(main_dialog, "K_cK", (Icallback)item_comment_action_cb);
  IupSetCallback(main_dialog, "K_cQ", (Icallback)item_uncomment_action_cb);

  /* Additional toolbar buttons */
  toolbar = IupGetDialogChild(main_dialog, "TOOLBAR");
  addToolbarButtons(toolbar);

  /* Elements trees */
  elem_tree_box = IupZbox(NULL);
  IupSetAttribute(elem_tree_box, "NAME", "ELEM_TREE_BOX");

  elem_but_box = buildElemToolbar();
  IupSetAttribute(elem_but_box, "ACTIVE", "NO");

  elementsFrame = IupFrame(IupHbox(elem_tree_box, elem_but_box, NULL));
  IupSetAttribute(elementsFrame, "MARGIN", "4x4");
  IupSetAttribute(elementsFrame, "GAP", "4");
  IupSetAttribute(elementsFrame, "TITLE", "Elements:");
  IupSetAttribute(elementsFrame, "TABTITLE", "Elements");

  IupAppend(panelTabs, elementsFrame);

  IupSetAttribute(panelTabs, "VALUE_HANDLE", (char*)elementsFrame);

  /* IupScintillaDlg configuration */
  IupSetAttribute(main_dialog, "SUBTITLE", "IupVisualLED");
  IupSetAttribute(main_dialog, "PROJECT_EXT", "vled");
  IupSetAttribute(main_dialog, "EXTRAFILTERS", "Led Files|*.led|");
  IupSetAttribute(main_dialog, "DEFAULT_EXT", "led");
  IupSetAttributeHandle(main_dialog, "CONFIG", config);

  IupSetHandle("VLED_MAIN", main_dialog);

  oldTabChange_cb = IupGetCallback(multitextTabs, "TABCHANGE_CB");
  IupSetCallback(multitextTabs, "OLDTABCHANGE_CB", oldTabChange_cb);
  IupSetCallback(multitextTabs, "TABCHANGE_CB", (Icallback)tabChange_cb);

  menu = IupGetAttributeHandle(main_dialog, "MENU");
  IupAppend(menu, IupSubmenu("&Help", IupMenu(
    IupSetCallbacks(IupItem("&IUP...", NULL), "ACTION", (Icallback)item_iup_action_cb, NULL),
    IupSetCallbacks(IupItem("&Help...", NULL), "ACTION", (Icallback)item_help_action_cb, NULL),
    IupSetCallbacks(IupItem("&About...", NULL), "ACTION", (Icallback)item_about_action_cb, NULL),
    NULL)));

  /* show the dialog at the last position, with the last size */
  IupConfigDialogShow(config, main_dialog, "IupVisualLED");

  /* open a file from the command line (allow file association in Windows) */
  for (i = 1; i < argc; i++)  /* arg0 is executable name */
  {
    const char* filename = argv[i];
    if (strstr(filename, ".vled"))
      IupSetStrAttribute(main_dialog, "OPENPROJECT", filename);
    else
      IupSetStrAttribute(main_dialog, "OPENFILE", filename);
  }

  /* Call NEW_TEXT_CB because the first tab was already created */
  scidlg_newtext_cb(main_dialog, vLedGetCurrentMultitext(main_dialog));
  IupRefresh(main_dialog);

  IupMainLoop();

  extra_dlg = (Ihandle*)IupGetAttribute(elem_tree_box, "FIND_ELEM_DIALOG");
  if (iupObjectCheck(extra_dlg))
    IupDestroy(extra_dlg);
  
  extra_dlg = (Ihandle*)IupGetAttribute(elem_tree_box, "PROPERTIES_DIALOG");
  if (iupObjectCheck(extra_dlg))
    IupDestroy(extra_dlg);

  extra_dlg = (Ihandle*)IupGetAttribute(main_dialog, "IMAGE_EDITOR_DIALOG");
  if (iupObjectCheck(extra_dlg))
    IupDestroy(extra_dlg);

  extra_dlg = (Ihandle*)IupGetAttribute(main_dialog, "GLOBALS_DIALOG");
  if (iupObjectCheck(extra_dlg))
    IupDestroy(extra_dlg);
  
  extra_dlg = (Ihandle*)IupGetAttribute(main_dialog, "CLASSINFO_DIALOG");
  if (iupObjectCheck(extra_dlg))
    IupDestroy(extra_dlg);

  IupDestroy(main_dialog);
  IupDestroy(config);

  IupClose();
  return EXIT_SUCCESS;
}

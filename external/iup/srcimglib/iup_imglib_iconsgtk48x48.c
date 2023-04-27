/** \file
 * \brief IupImgLib
 *
 * See Copyright Notice in iup.h
 */

#include <stdlib.h>

#include "iup.h"
#include "iup_image.h"

#include "iup_imglib.h"

void iupImglibIconsGtk48x48Open(void)
{
#ifdef IUP_IMGLIB_LARGE_ICON
  iupImageStockSet("IUP_IconMessageError", NULL, "gtk-dialog-error");
  iupImageStockSet("IUP_IconMessageHelp", NULL, "gtk-dialog-question");
  iupImageStockSet("IUP_IconMessageInfo", NULL, "gtk-dialog-info");
  iupImageStockSet("IUP_IconMessageSecurity", NULL, "gtk-dialog-authentication");
  iupImageStockSet("IUP_IconMessageWarning", NULL, "gtk-dialog-warning");
#endif
}

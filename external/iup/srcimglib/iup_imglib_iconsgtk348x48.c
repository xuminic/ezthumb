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
#include "iup_imglib_iconsgtk348x48.h"

void iupImglibIconsGtk348x48Open(void)
{
#ifdef IUP_IMGLIB_LARGE_ICON
  iupImageStockSet("IUP_IconMessageError", load_image_dialog_error, NULL);
  iupImageStockSet("IUP_IconMessageHelp", load_image_dialog_question, NULL);
  iupImageStockSet("IUP_IconMessageInfo", load_image_dialog_information, NULL);
  iupImageStockSet("IUP_IconMessageSecurity", load_image_dialog_password, NULL);
  iupImageStockSet("IUP_IconMessageWarning", load_image_dialog_warning, NULL);
#endif
}

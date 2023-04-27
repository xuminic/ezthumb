/** \file
 * \brief IupImgLib
 *
 * See Copyright Notice in iup.h
 */

#include <stdlib.h>

#include "iup.h"
#include "iup_image.h"

#include "iup_imglib.h"

#ifdef IUP_IMGLIB_LARGE
/* source code, included only here */
#include "iup_imglib_logos48x48_8bpp.h"

void iupImglibLogosMot48x48Open(void)
{
  iupImageStockSet("IUP_LogoTecgraf", load_image_LogoTecgraf8, NULL);
  iupImageStockSet("IUP_LogoPUC-Rio", load_image_LogoPUC_Rio8, NULL);
  iupImageStockSet("IUP_LogoBR", load_image_LogoBR8, NULL);
  iupImageStockSet("IUP_LogoLua", load_image_LogoLua8, NULL);
  iupImageStockSet("IUP_LogoTecgrafPUC-Rio", load_image_LogoTecgrafPUC_Rio8, NULL);
  iupImageStockSet("IUP_LogoPetrobras", load_image_LogoPetrobras8, NULL);
}
#endif

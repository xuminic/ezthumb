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
#include "iup_imglib_iconswin48x48.h"

void iupImglibIconsWin48x48Open(void)
{
#ifdef IUP_IMGLIB_LARGE
  iupImageStockSet("IUP_DeviceCamera", load_image_DeviceCamera, NULL);
  iupImageStockSet("IUP_DeviceCD", load_image_DeviceCD, NULL);
  iupImageStockSet("IUP_DeviceCellPhone", load_image_DeviceCellPhone, NULL);
  iupImageStockSet("IUP_DeviceComputer", load_image_DeviceComputer, NULL);
  iupImageStockSet("IUP_DeviceHardDrive", load_image_DeviceHardDrive, NULL);
  iupImageStockSet("IUP_DeviceFax", load_image_DeviceFax, NULL);
  iupImageStockSet("IUP_DeviceMP3", load_image_DeviceMP3, NULL);
  iupImageStockSet("IUP_DeviceNetwork", load_image_DeviceNetwork, NULL);
  iupImageStockSet("IUP_DevicePDA", load_image_DevicePDA, NULL);
  iupImageStockSet("IUP_DevicePrinter", load_image_DevicePrinter, NULL);
  iupImageStockSet("IUP_DeviceScanner", load_image_DeviceScanner, NULL);
  iupImageStockSet("IUP_DeviceSound", load_image_DeviceSound, NULL);
  iupImageStockSet("IUP_DeviceVideo", load_image_DeviceVideo, NULL);
#endif

#ifdef IUP_IMGLIB_LARGE_ICON
  iupImageStockSet("IUP_IconMessageError", load_image_IconMessageError, NULL);
  iupImageStockSet("IUP_IconMessageHelp", load_image_IconMessageHelp, NULL);
  iupImageStockSet("IUP_IconMessageInfo", load_image_IconMessageInfo, NULL);
  iupImageStockSet("IUP_IconMessageSecurity", load_image_IconMessageSecurity, NULL);
  iupImageStockSet("IUP_IconMessageWarning", load_image_IconMessageWarning, NULL);
#endif
}

/** \file
 * \brief MAC Driver TIPS management
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>


#include "iup.h" 

#include "iup_object.h" 
#include "iup_str.h" 
#include "iup_attrib.h" 
#include "iup_image.h" 

#include "iupandroid_drv.h"

int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
#if 0
  id tmpTip = (id)iupAttribGet(ih, "_IUPMAC_TIPSWIN");
  if (!tmpTip)
  {
    NSRect tipFrame;
    id theTipString = [NSString stringWithUTF8String:value];
    BOOL animate = NO;

    tipFrame.origin = [NSEvent mouseLocation];
    tipFrame.size = [TooltipWindow suggestedSizeForTooltip:theTipString];
    [TooltipWindow setDefaultDuration:5];

    tmpTip = [TooltipWindow tipWithAttributedString:theTipString frame:tipFrame display:NO];
    iupAttribSetStr(ih, "_IUPMAC_TIPSWIN", (char*)tmpTip);
  }

  if (value==NULL)
    [tmpTip release];
#endif
  return 1;
}

int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
#if 0
  id tmpTip = (id)iupAttribGet(ih, "_IUPMAC_TIPSWIN");
  if (!tmpTip)
    return 0;

  /* must use IupGetAttribute to use inheritance */
  if (!IupGetAttribute(ih, "TIP"))
    return 0;

  NSRect tipFrame = [tmpTip frame];
  if (iupStrBoolean(value)) {
    unsigned char r, g, b;
    if (iupStrToRGB(IupGetAttribute(ih, "TIPBGCOLOR"), &r, &g, &b))
    {
      [tmpTip setDefaultBackgroundColor:[NSColor colorWithDeviceRed:(r/255.0) green:(g/255.0) blue:(g/255.0) alpha:1.0]];
    }
    tipFrame.origin = [NSEvent mouseLocation];
    [tmpTip frame:tipFrame display:YES];
  } else
    [tmpTip frame:tipFrame display:NO];
#endif
  return 0;
}

char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
	return NULL;
}

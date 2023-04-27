
#include "iupandroid_jnimacros.h"

/*
This file is to declare global variables for JNI caching.
We have some Java classes that are used in multiple files.
While it is possible to have multiple instances of the same Java class object,
this can be further optimized by sharing a single global variable.

Most of our classes aren't shared between files, so they can be static to the file.
But we have at least one class (IupCommon), that is used in multiple files.

To make global:
Add any globals to this list, and then instead of using 
IUPJNI_DECLARE_CLASS_STATIC(IupCommon); in each file, use:
IUPJNI_DECLARE_CLASS_EXTERN(IupCommon);
*/


IUPJNI_DECLARE_CLASS_GLOBAL(IupCommon);

IUPJNI_DECLARE_METHOD_ID_GLOBAL(IupCommon_removeWidgetFromParent);


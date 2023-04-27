/** \file
 * \brief Image Resource.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdbool.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupandroid_drv.h"
#include <android/bitmap.h> // link to jnigraphics

#include <android/log.h>
#include <jni.h>
#include "iupandroid_jnimacros.h"
#include "iupandroid_jnicacheglobals.h"

IUPJNI_DECLARE_CLASS_STATIC(IupImageHelper);
IUPJNI_DECLARE_METHOD_ID_STATIC(IupImageHelper_createBitmap);


/* Adapted from SDL (zlib)
 * Calculate the pad-aligned scanline width of a surface
 */
static int CalculateBytesPerRow(int width, int bytes_per_pixel)
{
	int pitch;
	int bits_per_pixel = bytes_per_pixel * 8;
	/* Surface should be 4-byte aligned for speed */
	pitch = width * bytes_per_pixel;
	switch (bits_per_pixel) {
		case 1:
			pitch = (pitch + 7) / 8;
			break;
		case 4:
			pitch = (pitch + 1) / 2;
			break;
		default:
			break;
	}
	pitch = (pitch + 3) & ~3;   /* 4-byte aligning */
	return (pitch);
}

static int CalculateRowLength(int width, int bytes_per_pixel)
{
	int pitch = CalculateBytesPerRow(width, bytes_per_pixel);
	return pitch/bytes_per_pixel;
}


// FIXME: Carried over implementation. Probably wrong. Untested, don't know what calls this, don't know how to test.
void iupdrvImageGetRawData(void* handle, unsigned char* imgdata)
{
#if 0
  int x,y;
  unsigned char *red,*green,*blue,*alpha;
  NSImage *image = (__bridge NSImage*)handle;
  NSBitmapImageRep *bitmap = nil;
  if([[image representations] count]>0) bitmap = [[image representations] objectAtIndex:0];
  if(bitmap==nil) return;
	NSInteger w = [bitmap pixelsWide];
  NSInteger h = [bitmap pixelsHigh];
  NSInteger bpp = [bitmap bitsPerPixel];
  NSInteger planesize = w*h;
  unsigned char *bits = [bitmap bitmapData]; 
  red = imgdata;
  green = imgdata+planesize;
  blue = imgdata+2*planesize;
  alpha = imgdata+3*planesize;
  for(y=0;y<h;y++) {
    for(x=0;x<w;x++) {
      if(bpp>=24) {
        *red++ = *bits++;
        *green++ = *bits++;
        *blue++ = *bits++;
      }
      if(bpp==32) {
        *alpha++ = *bits++;
      }
    }
  }
#endif
}

// FIXME: Carried over implementation. Probably wrong. Untested, don't know what calls this, don't know how to test.
void* iupdrvImageCreateImageRaw(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jmethodID method_id = NULL;
	jobject java_bitmap = NULL;
	int ret_val;
	AndroidBitmapInfo bitmap_info;


	jclass java_class = IUPJNI_FindClass(IupImageHelper, jni_env, "br/pucrio/tecgraf/iup/IupImageHelper");
	method_id = IUPJNI_GetStaticMethodID(IupImageHelper_createBitmap, jni_env, java_class, "createBitmap", "(III)Landroid.graphics.Bitmap;");


	if(32 == bpp)
	{
		unsigned char* pixels;
		unsigned char* source_pixel = imgdata;

		// Note that the Android format is ARGB
		// createBitmap()
		java_bitmap = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id,
			(jint)width,
			(jint)height,
			(jint)32
		);	
		if(NULL == java_bitmap)
		{
			goto CLEANUP;
		}

		ret_val = AndroidBitmap_getInfo(jni_env, java_bitmap, &bitmap_info);
		if(ret_val < 0)
		{
			(*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
			java_bitmap = NULL;
			__android_log_print(ANDROID_LOG_ERROR, "iupandroid_image", "AndroidBitmap_getInfo() failed:%d", ret_val);
			goto CLEANUP;
		}
		ret_val = AndroidBitmap_lockPixels(jni_env, java_bitmap, (void**)&pixels);
		if(ret_val < 0)
		{
			(*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
			java_bitmap = NULL;
			__android_log_print(ANDROID_LOG_ERROR, "iupandroid_image", "AndroidBitmap_getInfo() failed:%d", ret_val);
			goto CLEANUP;
		}

		// Technically we are iterating through the source image, not the target image, so using bitmap_info.stride feels wrong.
		// int row_length = bitmap_info.stride/(32/8);
		int row_length = CalculateRowLength(width, 4);

		for(int y=0;y<height;y++)
		{
			for(int x=0;x<row_length;x++)
			{
				unsigned char s_r = *source_pixel;
				source_pixel++;
				unsigned char s_g = *source_pixel;
				source_pixel++;
				unsigned char s_b = *source_pixel;
				source_pixel++;
				unsigned char s_a = *source_pixel;
				source_pixel++;

				// Need to swap RGBA to ARGB???
				// Even though the declared format is ARGB, experimentally setting this array, the order seems to be RGBA.
				*pixels = s_r;
				pixels++;
				*pixels = s_g;
				pixels++;
				*pixels = s_b;
				pixels++;
				*pixels = s_a;
				pixels++;
			}
		}
		AndroidBitmap_unlockPixels(jni_env, java_bitmap);

	}
	else if(24 == bpp)
	{
		unsigned char* pixels;
		unsigned char* source_pixel = imgdata;

		// The only useful format Android provides for this case is ARGB (bpp=32), so we need to convert to 32-bit
		// createBitmap()
		java_bitmap = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id,
			(jint)width,
			(jint)height,
			(jint)32
		);	
		if(NULL == java_bitmap)
		{
			goto CLEANUP;
		}

		ret_val = AndroidBitmap_getInfo(jni_env, java_bitmap, &bitmap_info);
		if(ret_val < 0)
		{
			(*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
			java_bitmap = NULL;
			__android_log_print(ANDROID_LOG_ERROR, "iupandroid_image", "AndroidBitmap_getInfo() failed:%d", ret_val);
			goto CLEANUP;
		}
		ret_val = AndroidBitmap_lockPixels(jni_env, java_bitmap, (void**)&pixels);
		if(ret_val < 0)
		{
			(*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
			java_bitmap = NULL;
			__android_log_print(ANDROID_LOG_ERROR, "iupandroid_image", "AndroidBitmap_getInfo() failed:%d", ret_val);
			goto CLEANUP;
		}

		// Technically we are iterating through the source image, not the target image, so using bitmap_info.stride feels wrong.
		// int row_length = bitmap_info.stride/(24/8);
		int row_length = CalculateRowLength(width, 3);

		for(int y=0;y<height;y++)
		{
			for(int x=0;x<row_length;x++)
			{
				// Need to convert RGB to ARGB
				unsigned char s_r = *source_pixel;
				source_pixel++;
				unsigned char s_g = *source_pixel;
				source_pixel++;
				unsigned char s_b = *source_pixel;
				source_pixel++;

				// Even though the declared format is ARGB, experimentally setting this array, the order seems to be RGBA.
				*pixels = s_r;
				pixels++;
				*pixels = s_g;
				pixels++;
				*pixels = s_b;
				pixels++;
				*pixels = 255;
				pixels++;
			}
		}
		AndroidBitmap_unlockPixels(jni_env, java_bitmap);

	}
	else if(8 == bpp)
	{
		unsigned char* pixels;
		unsigned char* source_pixel = imgdata;
		int has_alpha = false;

		// The only useful format Android provides for this case is ARGB (bpp=32), so we need to convert to 32-bit
		// createBitmap()
		java_bitmap = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id,
			(jint)width,
			(jint)height,
			(jint)32
		);	
		if(NULL == java_bitmap)
		{
			goto CLEANUP;
		}

		ret_val = AndroidBitmap_getInfo(jni_env, java_bitmap, &bitmap_info);
		if(ret_val < 0)
		{
			(*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
			java_bitmap = NULL;
			__android_log_print(ANDROID_LOG_ERROR, "iupandroid_image", "AndroidBitmap_getInfo() failed:%d", ret_val);
			goto CLEANUP;
		}
		ret_val = AndroidBitmap_lockPixels(jni_env, java_bitmap, (void**)&pixels);
		if(ret_val < 0)
		{
			(*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
			java_bitmap = NULL;
			__android_log_print(ANDROID_LOG_ERROR, "iupandroid_image", "AndroidBitmap_getInfo() failed:%d", ret_val);
			goto CLEANUP;
		}
		// int row_length = bitmap_info.stride/(32/8);
		int row_length = CalculateRowLength(width, 4);

		int colors_count = 0;
		iupColor colors[256];
		
		
		for(int y=0;y<height;y++)
		{
			for(int x=0;x<row_length;x++)
			{
				unsigned char index = *source_pixel;
				iupColor* c = &colors[index];

				unsigned char s_r = c->r;
				unsigned char s_g = c->g;
				unsigned char s_b = c->b;
				unsigned char s_a;

				if(has_alpha)
				{
					s_a = c->a;
				}
				else
				{
					s_a = 255;
				}

				// Even though the declared format is ARGB, experimentally setting this array, the order seems to be RGBA.
				*pixels = s_r;
				pixels++;
				*pixels = s_g;
				pixels++;
				*pixels = s_b;
				pixels++;
				*pixels = s_a;
				pixels++;


				source_pixel++;

			}
		}
		AndroidBitmap_unlockPixels(jni_env, java_bitmap);

	}
		

CLEANUP:
	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	if(NULL != java_bitmap)
	{
		jobject return_bitmap = (*jni_env)->NewGlobalRef(jni_env, java_bitmap);
		(*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
		return return_bitmap;
	}
	else
	{
		return NULL;
	}
}

int iupdrvImageGetRawInfo(void* handle, int *w, int *h, int *bpp, iupColor* colors, int *colors_count)
{
  /* How to get the pallete? */
  (void)colors;
  (void)colors_count;
  return iupdrvImageGetInfo(handle, w, h, bpp);
}


// NOTE: Returns an autoreleased NSImage.
void* iupdrvImageCreateImage(Ihandle *ih, const char* bgcolor, int make_inactive)
{
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jmethodID method_id = NULL;
	jobject java_bitmap = NULL;
	int ret_val;
	AndroidBitmapInfo  bitmap_info;

	int bpp;
	int width;
	int height;
	unsigned char* imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");

	width = ih->currentwidth;
	height = ih->currentheight;
	bpp = iupAttribGetInt(ih, "BPP");

	unsigned char bg_r=0, bg_g=0, bg_b=0;
	iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);


	jclass java_class = IUPJNI_FindClass(IupImageHelper, jni_env, "br/pucrio/tecgraf/iup/IupImageHelper");
	method_id = IUPJNI_GetStaticMethodID(IupImageHelper_createBitmap, jni_env, java_class, "createBitmap", "(III)Landroid/graphics/Bitmap;");





	if(32 == bpp)
	{
		unsigned char* pixels;
		unsigned char* source_pixel = imgdata;

		// Note that the Android format is ARGB
		// createBitmap()
		java_bitmap = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id,
			(jint)width,
			(jint)height,
			(jint)32
		);	
		if(NULL == java_bitmap)
		{
			goto CLEANUP;
		}

		ret_val = AndroidBitmap_getInfo(jni_env, java_bitmap, &bitmap_info);
		if(ret_val < 0)
		{
			(*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
			java_bitmap = NULL;
			__android_log_print(ANDROID_LOG_ERROR, "iupandroid_image", "AndroidBitmap_getInfo() failed:%d", ret_val);
			goto CLEANUP;
		}
//		void* start_pixels = NULL;
		ret_val = AndroidBitmap_lockPixels(jni_env, java_bitmap, (void**)&pixels);
//		ret_val = AndroidBitmap_lockPixels(jni_env, java_bitmap, &start_pixels);
		if(ret_val < 0)
		{
			(*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
			java_bitmap = NULL;
			__android_log_print(ANDROID_LOG_ERROR, "iupandroid_image", "AndroidBitmap_getInfo() failed:%d", ret_val);
			goto CLEANUP;
		}
//		pixels = (unsigned char*)start_pixels;

		// Technically we are iterating through the source image, not the target image, so using bitmap_info.stride feels wrong.
		// int row_length = bitmap_info.stride/(32/8);
		int row_length = CalculateRowLength(width, 4);

		for(int y=0;y<height;y++)
		{
			for(int x=0;x<row_length;x++)
			{
				unsigned char s_r = *source_pixel;
				source_pixel++;
				unsigned char s_g = *source_pixel;
				source_pixel++;
				unsigned char s_b = *source_pixel;
				source_pixel++;
				unsigned char s_a = *source_pixel;
				source_pixel++;

				if(make_inactive)
				{
					iupImageColorMakeInactive(&s_r, &s_g, &s_b, bg_r, bg_g, bg_b);
				}

				// Need to swap RGBA to ARGB???
				// Even though the declared format is ARGB, experimentally setting this array, the order seems to be RGBA.
				*pixels = s_r;
				pixels++;
				*pixels = s_g;
				pixels++;
				*pixels = s_b;
				pixels++;
				*pixels = s_a;
				pixels++;
			}
		}
		AndroidBitmap_unlockPixels(jni_env, java_bitmap);

	}
	else if(24 == bpp)
	{
		unsigned char* pixels;
		unsigned char* source_pixel = imgdata;

		// The only useful format Android provides for this case is ARGB (bpp=32), so we need to convert to 32-bit
		// NOTE: RGB565 is available, but I need to figure out how to write to pixels (the whole argb vs rgba surprise is making me untrusting).
		// Also, the quality loss going from 24-bit to 16-bit may not be acceptable.
		// createBitmap()
		java_bitmap = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id,
			(jint)width,
			(jint)height,
			(jint)32
		);	
		if(NULL == java_bitmap)
		{
			goto CLEANUP;
		}

		ret_val = AndroidBitmap_getInfo(jni_env, java_bitmap, &bitmap_info);
		if(ret_val < 0)
		{
			(*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
			java_bitmap = NULL;
			__android_log_print(ANDROID_LOG_ERROR, "iupandroid_image", "AndroidBitmap_getInfo() failed:%d", ret_val);
			goto CLEANUP;
		}
		ret_val = AndroidBitmap_lockPixels(jni_env, java_bitmap, (void**)&pixels);
		if(ret_val < 0)
		{
			(*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
			java_bitmap = NULL;
			__android_log_print(ANDROID_LOG_ERROR, "iupandroid_image", "AndroidBitmap_getInfo() failed:%d", ret_val);
			goto CLEANUP;
		}

		// Technically we are iterating through the source image, not the target image, so using bitmap_info.stride feels wrong.
		// int row_length = bitmap_info.stride/(24/8);
		int row_length = CalculateRowLength(width, 3);

		for(int y=0;y<height;y++)
		{
			for(int x=0;x<row_length;x++)
			{
				// Need to convert RGB to ARGB
				unsigned char s_r = *source_pixel;
				source_pixel++;
				unsigned char s_g = *source_pixel;
				source_pixel++;
				unsigned char s_b = *source_pixel;
				source_pixel++;

				if(make_inactive)
				{
					iupImageColorMakeInactive(&s_r, &s_g, &s_b, bg_r, bg_g, bg_b);
				}

				// Even though the declared format is ARGB, experimentally setting this array, the order seems to be RGBA.
				*pixels = s_r;
				pixels++;
				*pixels = s_g;
				pixels++;
				*pixels = s_b;
				pixels++;
				*pixels = 255;
				pixels++;


			}
		}
		AndroidBitmap_unlockPixels(jni_env, java_bitmap);

	}
	else if(8 == bpp)
	{
		unsigned char* pixels;
		unsigned char* source_pixel = imgdata;

		// The only useful format Android provides for this case is ARGB (bpp=32), so we need to convert to 32-bit
		// (Android's 8-bit format seems to be for alpha/luminance, not palettes.)
		// createBitmap()
		java_bitmap = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id,
			(jint)width,
			(jint)height,
			(jint)32
		);	
		if(NULL == java_bitmap)
		{
			goto CLEANUP;
		}

		ret_val = AndroidBitmap_getInfo(jni_env, java_bitmap, &bitmap_info);
		if(ret_val < 0)
		{
			(*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
			java_bitmap = NULL;
			__android_log_print(ANDROID_LOG_ERROR, "iupandroid_image", "AndroidBitmap_getInfo() failed:%d", ret_val);
			goto CLEANUP;
		}
		ret_val = AndroidBitmap_lockPixels(jni_env, java_bitmap, (void**)&pixels);
		if(ret_val < 0)
		{
			(*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
			java_bitmap = NULL;
			__android_log_print(ANDROID_LOG_ERROR, "iupandroid_image", "AndroidBitmap_getInfo() failed:%d", ret_val);
			goto CLEANUP;
		}

		// int row_length = bitmap_info.stride/(32/8);
		int row_length = CalculateRowLength(width, 4);

		int colors_count = 0;
		iupColor colors[256];
		
		int has_alpha = iupImageInitColorTable(ih, colors, &colors_count);

		
		for(int y=0;y<height;y++)
		{
			for(int x=0;x<row_length;x++)
			{
				unsigned char index = *source_pixel;
				iupColor* c = &colors[index];

				unsigned char s_r = c->r;
				unsigned char s_g = c->g;
				unsigned char s_b = c->b;
				unsigned char s_a;

				if(has_alpha)
				{
					s_a = c->a;
				}
				else
				{
					s_a = 255;
				}

				if(make_inactive)
				{
					iupImageColorMakeInactive(&s_r, &s_g, &s_b, bg_r, bg_g, bg_b);
				}


				// Even though the declared format is ARGB, experimentally setting this array, the order seems to be RGBA.
				*pixels = s_r;
				pixels++;
				*pixels = s_g;
				pixels++;
				*pixels = s_b;
				pixels++;
				*pixels = s_a;
				pixels++;


				source_pixel++;

			}
		}
		AndroidBitmap_unlockPixels(jni_env, java_bitmap);

	}
		
	int bgcolor_depend = 0;
	if(bgcolor_depend || make_inactive)
	{
		iupAttribSetStr(ih, "_IUP_BGCOLOR_DEPEND", "1");
	}

CLEANUP:
	(*jni_env)->DeleteLocalRef(jni_env, java_class);

	if(NULL != java_bitmap)
	{
		jobject return_bitmap = (*jni_env)->NewGlobalRef(jni_env, java_bitmap);
		(*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
		return return_bitmap;
	}
	else
	{
		return NULL;
	}
}

void* iupdrvImageCreateIcon(Ihandle *ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

void* iupdrvImageCreateCursor(Ihandle *ih)
{
#if 0
  int bpp,y,x,hx,hy,
      width = ih->currentwidth,
      height = ih->currentheight,
      line_size = (width+7)/8,
      size_bytes = line_size*height;
  unsigned char *imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");
  char *sbits, *mbits, *sb, *mb;
  unsigned char r, g, b;

  bpp = iupAttribGetInt(ih, "BPP");
  if (bpp > 8)
    return NULL;

  sbits = (char*)malloc(2*size_bytes);
  if (!sbits) return NULL;
  memset(sbits, 0, 2*size_bytes);
  mbits = sbits + size_bytes;

  sb = sbits;
  mb = mbits;
  for (y=0; y<height; y++)
  {
    for (x=0; x<width; x++)
    {
      int byte = x/8;
      int bit = x%8;
      int index = (int)imgdata[y*width+x];
      /* index==0 is transparent */
      if (index == 1)
        sb[byte] = (char)(sb[byte] | (1<<bit));
      if (index != 0)
        mb[byte] = (char)(mb[byte] | (1<<bit));
    }

    sb += line_size;
    mb += line_size;
  }

  hx=0; hy=0;
  iupStrToIntInt(iupAttribGet(ih, "HOTSPOT"), &hx, &hy, ':');

  NSData *tiffData = [NSData dataWithBytes:imgdata length:(width*height*(bpp/8))];
  NSImage *source = [[NSImage alloc] initWithData:tiffData];
  NSSize size = {width,height};
  [source setSize:size]; 

  NSPoint point = {hx,hy};

  NSCursor *cursor = [[NSCursor alloc] initWithImage:source hotSpot:point];

  free(sbits);
  return (void*)CFBridgingRetain(cursor);
#endif
  return NULL;
}

void* iupdrvImageCreateMask(Ihandle *ih)
{
#if 0
  int bpp,y,x,
      width = ih->currentwidth,
      height = ih->currentheight,
      line_size = (width+7)/8,
      size_bytes = line_size*height;
  unsigned char *imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");
  char *bits, *sb;
  unsigned char colors[256];

  bpp = iupAttribGetInt(ih, "BPP");
  if (bpp > 8)
    return NULL;

  bits = (char*)malloc(size_bytes);
  if (!bits) return NULL;
  memset(bits, 0, size_bytes);

  iupImageInitNonBgColors(ih, colors);

  sb = bits;
  for (y=0; y<height; y++)
  {
    for (x=0; x<width; x++)
    {
      int byte = x/8;
      int bit = x%8;
      int index = (int)imgdata[y*width+x];
      if (colors[index])
        sb[byte] = (char)(sb[byte] | (1<<bit));
    }

    sb += line_size;
  }

  NSData *tiffData = [NSData dataWithBytes:imgdata length:(width*height*(bpp/8))];
  NSImage *mask = [[NSImage alloc] initWithData:tiffData];
  NSSize size = {width,height};
  [mask setSize:size]; 
  free(bits);
  return (void*)CFBridgingRetain(mask);
#endif
  return NULL;
}

void* iupdrvImageLoad(const char* name, int type)
{
	if(NULL == name)
	{
		return NULL;
	}

	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	jmethodID method_id = NULL;
	jobject java_bitmap = NULL;

	jclass java_class = IUPJNI_FindClass(IupImageHelper, jni_env, "br/pucrio/tecgraf/iup/IupImageHelper");
	method_id = IUPJNI_GetStaticMethodID(IupImageHelper_createBitmap, jni_env, java_class, "loadBitmap", "(Ljava/lang/String;)Landroid.graphics.Bitmap;");


	jstring j_string = (*jni_env)->NewStringUTF(jni_env, name);
	java_bitmap = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id,
		j_string
	);	
	(*jni_env)->DeleteLocalRef(jni_env, j_string);
	(*jni_env)->DeleteLocalRef(jni_env, java_class);


	if(NULL != java_bitmap)
	{
		jobject return_bitmap = (*jni_env)->NewGlobalRef(jni_env, java_bitmap);
		(*jni_env)->DeleteLocalRef(jni_env, java_bitmap);
		return return_bitmap;
	}
	else
	{
		return NULL;
	}
}

int iupdrvImageGetInfo(void* handle, int *w, int *h, int *bpp)
{
	jobject java_bitmap = (jobject)handle;
	if(NULL == java_bitmap)
	{
		return 0;
	}

	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	int ret_val;
	AndroidBitmapInfo bitmap_info;

	ret_val = AndroidBitmap_getInfo(jni_env, java_bitmap, &bitmap_info);
	if(ret_val < 0)
	{
		__android_log_print(ANDROID_LOG_ERROR, "iupandroid_image", "AndroidBitmap_getInfo() failed:%d", ret_val);
		return 0;
	}
	
  	if(w) *w = bitmap_info.width;
	if(h) *h = bitmap_info.height;

	// We only can use ARGB_8888 for all formats, this is always going to be 32
	if(bpp) *bpp = 32;


	return 1;
}

// [NSApp setApplicationIconImage: [NSImage imageNamed: @"Icon_name.icns"]]

void iupdrvImageDestroy(void* handle, int type)
{
	if(NULL == handle)
	{
		return;
	}
	
	JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
	(*jni_env)->DeleteGlobalRef(jni_env, (jobject)handle);
	
}

void iupdrvImageGetData(void* handle, unsigned char* imgdata)
{
	
}



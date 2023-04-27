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

#include "iupemscripten_drv.h"
#include "SDL.h"
#include "SDL_image.h"


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
    SDL_Surface* sdl_surface = NULL;
	if(32 == bpp)
	{
		sdl_surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
		if(NULL == sdl_surface)
		{
			return NULL;
		}

		int row_length = sdl_surface->pitch;
		unsigned char* pixels = sdl_surface->pixels;

		unsigned char* source_pixel = imgdata;

		SDL_LockSurface(sdl_surface);

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
		SDL_UnlockSurface(sdl_surface);


	}
	else if(24 == bpp)
	{
		sdl_surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
		if(NULL == sdl_surface)
		{
			return NULL;
		}

		int row_length = CalculateRowLength(width, 3);
//		int row_length = sdl_surface->pitch;

		unsigned char* pixels = sdl_surface->pixels;

		unsigned char* source_pixel = imgdata;


		SDL_LockSurface(sdl_surface);

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

		SDL_UnlockSurface(sdl_surface);

	}
	else if(8 == bpp)
	{
		// We'll make a full 32-bit image for this case
		// NOTE: Even though SDL_surface actually supports 8-bit palette images, we need to supply the underlying native Canvas with 32-bit RGBA.
		sdl_surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
		if(NULL == sdl_surface)
		{
			return NULL;
		}

		int row_length = sdl_surface->pitch;
		unsigned char* pixels = sdl_surface->pixels;

		unsigned char* source_pixel = imgdata;


		int colors_count = 0;
		iupColor colors[256];
		
		// FIXME: What should we do here?
		int has_alpha = false;

		SDL_LockSurface(sdl_surface);
		
		for(int y=0;y<height;y++)
		{
			for(int x=0;x<row_length;x++)
			{
				unsigned char index = *source_pixel;
				iupColor* c = &colors[index];

				*pixels = c->r;
				pixels++;
				*pixels = c->g;
				pixels++;
				*pixels = c->b;
				pixels++;

				if(has_alpha)
				{
					*pixels = c->a;
				}
				else
				{
					*pixels = 255;
				}
				pixels++;
				source_pixel++;

			}
		}
		SDL_UnlockSurface(sdl_surface);



	}
		

	return sdl_surface;
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
    SDL_Surface* sdl_surface = NULL;
	int bpp;
	int width;
	int height;
	unsigned char* imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");

	width = ih->currentwidth;
	height = ih->currentheight;
	bpp = iupAttribGetInt(ih, "BPP");

	unsigned char bg_r=0, bg_g=0, bg_b=0;
	iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

	if(32 == bpp)
	{
		sdl_surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
		if(NULL == sdl_surface)
		{
			return NULL;
		}

		int row_length = sdl_surface->pitch;
		unsigned char* pixels = sdl_surface->pixels;

		unsigned char* source_pixel = imgdata;

		SDL_LockSurface(sdl_surface);

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

		SDL_UnlockSurface(sdl_surface);

	}
	else if(24 == bpp)
	{
		// Convert to 32-bit because the native CANVAS/IMG needs 32-bit data.
		sdl_surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
		if(NULL == sdl_surface)
		{
			return NULL;
		}

		int row_length = CalculateRowLength(width, 3);
//		int row_length = sdl_surface->pitch;
		unsigned char* pixels = sdl_surface->pixels;

		unsigned char* source_pixel = imgdata;


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


				if(make_inactive)
				{
					iupImageColorMakeInactive(&s_r, &s_g, &s_b, bg_r, bg_g, bg_b);
				}

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


		SDL_UnlockSurface(sdl_surface);


	}
	else if(8 == bpp)
	{
		// We'll make a full 32-bit image for this case
		// NOTE: Even though SDL_surface actually supports 8-bit palette images, we need to supply the underlying native Canvas with 32-bit RGBA.
		sdl_surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
		if(NULL == sdl_surface)
		{
			return NULL;
		}

		int row_length = sdl_surface->pitch;
		unsigned char* pixels = sdl_surface->pixels;

		unsigned char* source_pixel = imgdata;

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

		SDL_UnlockSurface(sdl_surface);


	}
		
	int bgcolor_depend = 0;
	if(bgcolor_depend || make_inactive)
	{
		iupAttribSetStr(ih, "_IUP_BGCOLOR_DEPEND", "1");
	}

	return sdl_surface;
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
	SDL_Surface* sdl_surface = IMG_Load(name);
	// Because the native Canvas/IMG require RGBA, we must make sure to convert the SDL_surface to 32-bit RGBA if it is not already in that format.
	if(NULL == sdl_surface)
	{
		return NULL;
	}

	// UGH: Show stopping bug with Emscripten's IMG_Load implementation.
	// It does not set the correct SDL_PixelFormat values in the surface so I don't know how the data is laid out.
	// And it also breaks SDL_ConvertSurfaceFormat().
	// My bug report: https://github.com/kripken/emscripten/issues/7076


#if 1
	Uint32 fmt = sdl_surface->format->format;
	iupEmscripten_Log("sdl_surface->format->format: %d", sdl_surface->format->format);

	iupEmscripten_Log("SDL_PIXELTYPE: %d", SDL_PIXELTYPE(fmt));
	iupEmscripten_Log("SDL_PIXELORDER: %d", SDL_PIXELORDER(fmt));
	iupEmscripten_Log("SDL_PIXELLAYOUT: %d", SDL_PIXELLAYOUT(fmt));
	iupEmscripten_Log("SDL_BITSPERPIXEL: %d", SDL_BITSPERPIXEL(fmt));
	iupEmscripten_Log("SDL_BYTESPERPIXEL: %d", SDL_BYTESPERPIXEL(fmt));
	iupEmscripten_Log("SDL_BYTESPERPIXEL: %d", SDL_BYTESPERPIXEL(fmt));
	iupEmscripten_Log("SDL_ISPIXELFORMAT_INDEXED: %d", SDL_ISPIXELFORMAT_INDEXED(fmt));
	iupEmscripten_Log("SDL_ISPIXELFORMAT_ALPHA: %d", SDL_ISPIXELFORMAT_ALPHA(fmt));
	iupEmscripten_Log("SDL_ISPIXELFORMAT_FOURCC: %d", SDL_ISPIXELFORMAT_FOURCC(fmt));

	iupEmscripten_Log("SDL_PIXELFORMAT_RGBA8888: %d", SDL_PIXELFORMAT_RGBA8888);

	iupEmscripten_Log("SDL_PIXELTYPE: %d", SDL_PIXELTYPE(SDL_PIXELFORMAT_RGBA8888));
	iupEmscripten_Log("SDL_PIXELORDER: %d", SDL_PIXELORDER(SDL_PIXELFORMAT_RGBA8888));
	iupEmscripten_Log("SDL_PIXELLAYOUT: %d", SDL_PIXELLAYOUT(SDL_PIXELFORMAT_RGBA8888));
	iupEmscripten_Log("SDL_BITSPERPIXEL: %d", SDL_BITSPERPIXEL(SDL_PIXELFORMAT_RGBA8888));
	iupEmscripten_Log("SDL_BYTESPERPIXEL: %d", SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_RGBA8888));
	iupEmscripten_Log("SDL_BYTESPERPIXEL: %d", SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_RGBA8888));
	iupEmscripten_Log("SDL_ISPIXELFORMAT_INDEXED: %d", SDL_ISPIXELFORMAT_INDEXED(SDL_PIXELFORMAT_RGBA8888));
	iupEmscripten_Log("SDL_ISPIXELFORMAT_ALPHA: %d", SDL_ISPIXELFORMAT_ALPHA(SDL_PIXELFORMAT_RGBA8888));
	iupEmscripten_Log("SDL_ISPIXELFORMAT_FOURCC: %d", SDL_ISPIXELFORMAT_FOURCC(SDL_PIXELFORMAT_RGBA8888));
#endif


	if(SDL_PIXELFORMAT_RGBA8888 == sdl_surface->format->format)
	{
		iupEmscripten_Log("iupdrvImageLoad got SDL_PIXELFORMAT_RGBA8888");
		return sdl_surface;
	}
	else
	{
		iupEmscripten_Log("iupdrvImageLoad need to convert to SDL_PIXELFORMAT_RGBA8888 (%d) from %d", SDL_PIXELFORMAT_RGBA8888, sdl_surface->format->format);
		SDL_Surface* converted_format = SDL_ConvertSurfaceFormat(sdl_surface, SDL_PIXELFORMAT_RGBA8888, 0);
		if(converted_format)
		{
			iupEmscripten_Log("iupdrvImageLoad convert worked %d",converted_format->format->format);
		}
		else
		{
			iupEmscripten_Log("iupdrvImageLoad convert failed, %s",  SDL_GetError());
			// I should just fall through, but because of the show stopping bug, for now, I would like to see something, even if corrupted, on screen.
			return sdl_surface;
		}

		SDL_FreeSurface(sdl_surface);
		sdl_surface = NULL;
		return converted_format;
	}
}

int iupdrvImageGetInfo(void* handle, int *w, int *h, int *bpp)
{
 	SDL_Surface* sdl_surface = (SDL_Surface*)handle;
	if(NULL == sdl_surface)
	{
		return 0;
	}
  	if(w) *w = sdl_surface->w;
	if(h) *h = sdl_surface->h;
	if(bpp) *bpp = (int)sdl_surface->format->BitsPerPixel;

	return 1;
}

// [NSApp setApplicationIconImage: [NSImage imageNamed: @"Icon_name.icns"]]

void iupdrvImageDestroy(void* handle, int type)
{
	SDL_FreeSurface((SDL_Surface*)handle);
}

void iupdrvImageGetData(void* handle, unsigned char* imgdata)
{
}


/***************************************************************************
 * define.h is part of Math Graphic Library
 * Copyright (C) 2007-2016 Alexey Balakin <mathgl.abalakin@gmail.ru>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 3 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef _MGL_DEFINE_H_
#define _MGL_DEFINE_H_
//-----------------------------------------------------------------------------
// Disable warnings for MSVC:
// 4190 - C-linkage of std::complex,
// 4996 - deprecated abi functions
// 4786 - disable warnings on 255 char debug symbols
// 4231 - disable warnings on extern before template instantiation
// 4800	- "int,uint32_t,etc" forcing value to bool 'true' or 'false' (performance warning)
// 4244 - conversion from 'mreal,double' to 'float', possible loss of data
// 4267	- conversion from 'size_t' to 'long,int,etc', possible loss of data
// 4305	- truncation from 'double' to 'float'
#if defined(_MSC_VER)
#pragma warning(disable: 4996 4190 4786 4231 4800 4244 4267 4305)
#endif

#include "mgl2/config.h"
#ifndef SWIG

#if MGL_HAVE_PTHR_WIDGET|MGL_HAVE_PTHREAD
#include <pthread.h>
#endif

#include "mgl2/dllexport.h"
#if defined(_MSC_VER)
#define MGL_OBSOLETE	MGL_NO_EXPORT
#else
#define MGL_OBSOLETE	MGL_EXPORT
#endif

#if MGL_HAVE_ATTRIBUTE
#define MGL_FUNC_CONST	__attribute__((const))
#define MGL_FUNC_PURE	__attribute__((pure))
#else
#define MGL_FUNC_CONST
#define MGL_FUNC_PURE
#endif
#define MGL_EXPORT_CONST	MGL_EXPORT MGL_FUNC_CONST
#define MGL_EXPORT_PURE		MGL_EXPORT MGL_FUNC_PURE
#define MGL_LOCAL_CONST		MGL_NO_EXPORT MGL_FUNC_CONST
#define MGL_LOCAL_PURE		MGL_NO_EXPORT MGL_FUNC_PURE

#if MGL_HAVE_RVAL	// C++11 don't support register keyword
#if (!defined(_MSC_VER)) || (defined(_MSC_VER) && (_MSC_VER < 1310))
#define register
#endif
#endif

#endif
//-----------------------------------------------------------------------------
#ifdef WIN32 //_MSC_VER needs this before math.h
#define	_USE_MATH_DEFINES
#endif

#ifdef MGL_SRC
#if MGL_HAVE_ZLIB
#include <zlib.h>
#ifndef Z_BEST_COMPRESSION
#define Z_BEST_COMPRESSION 9
#endif
#else
#define gzFile	FILE*
#define gzread(fp,buf,size)	fread(buf,1,size,fp)
#define gzopen	fopen
#define gzclose	fclose
#define gzprintf	fprintf
#define gzgets(fp,str,size)	fgets(str,size,fp)
#define gzgetc	fgetc
#endif
#endif

#if (defined(_MSC_VER) && (_MSC_VER<1600)) || defined(__BORLANDC__)
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed long int32_t;
typedef signed long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;
#else
#include <stdint.h>
#endif
#if defined(__BORLANDC__)
typedef unsigned long uintptr_t;
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#if defined(_MSC_VER)
#define collapse(a)	// MSVS don't support OpenMP 3.*
#if (_MSC_VER<=1800)
#define strtoull _strtoui64
//#define hypot	_hypot
#define getcwd	_getcwd
#define chdir	_chdir // BORLAND has chdir
#endif
#define snprintf _snprintf
#if (_MSC_VER<1600) // based on https://hg.python.org/cpython/rev/9aedb876c2d7
#define hypot	_hypot
#endif
#endif

#if !MGL_SYS_NAN
#include <float.h>
#include <math.h>
const unsigned long long mgl_nan[2] = {0x7fffffffffffffff, 0x7fffffff};
const unsigned long long mgl_inf[2] = {0x7ff0000000000000, 0x7f800000};
#define NANd    (*(double*)mgl_nan)
#define NANf    (*(float*)(mgl_nan+1))
#define INFd    (*(double*)mgl_inf)
#define INFf    (*(float*)(mgl_inf+1))

#if !defined(NAN)
#if MGL_USE_DOUBLE
#define NAN		NANd
#else
#define NAN		NANf
#endif
#endif

#if !defined(INFINITY)
#if MGL_USE_DOUBLE
#define INFINITY	INFd
#else
#define INFINITY	INFf
#endif
#endif
#endif	// !MGL_SYS_NAN

#ifndef M_PI
#define M_PI	3.14159265358979323846  /* pi */
#endif
//-----------------------------------------------------------------------------
#ifdef WIN32
#define mglprintf    _snwprintf
#else
#define mglprintf    swprintf
#endif
//#define FLT_EPS	1.1920928955078125e-07
//-----------------------------------------------------------------------------
#if MGL_USE_DOUBLE
typedef double mreal;
#define MGL_EPSILON	(1.+1e-10)
#define MGL_MIN_VAL 1e-307
#else
typedef float mreal;
#define MGL_EPSILON	(1.+1e-5)
#define MGL_MIN_VAL 1e-37
#endif
#define MGL_FEPSILON	(1.+1e-5)
//-----------------------------------------------------------------------------
#ifndef MGL_CMAP_COLOR
#define MGL_CMAP_COLOR	32
#endif
//-----------------------------------------------------------------------------
#ifndef MGL_DEF_VIEWER
#define MGL_DEF_VIEWER "evince"
#endif
//-----------------------------------------------------------------------------
#if MGL_HAVE_TYPEOF
#define mgl_isrange(a,b)	({typeof (a) _a = (a); typeof (b) _b = (b); fabs(_a-_b)>MGL_MIN_VAL && _a-_a==mreal(0.) && _b-_b==mreal(0.);})
#define mgl_isbad(a)	({typeof (a) _a = (a); _a-_a!=mreal(0.);})
#define mgl_isfin(a)	({typeof (a) _a = (a); _a-_a==mreal(0.);})
#define mgl_isnum(a)	({typeof (a) _a = (a); _a==_a;})
#define mgl_isnan(a)	({typeof (a) _a = (a); _a!=_a;})
#define mgl_min(a,b)	({typeof (a) _a = (a); typeof (b) _b = (b); _a > _b ? _b : _a;})
#define mgl_max(a,b)	({typeof (a) _a = (a); typeof (b) _b = (b); _a > _b ? _a : _b;})
#define mgl_sign(a)		({typeof (a) _a = (a); _a<0 ? -1:1;})
#define mgl_int(a)		({typeof (a) _a = (a); long(_a+(_a>=0 ? 0.5:-0.5));})
#else
#define mgl_isrange(a,b)	(fabs((a)-(b))>MGL_EPSILON && (a)-(a)==mreal(0.) && (b)-(b)==mreal(0.))
#define mgl_min(a,b)	(((a)>(b)) ? (b) : (a))
#define mgl_max(a,b)	(((a)>(b)) ? (a) : (b))
#define mgl_isnan(a)	((a)!=(a))
#define mgl_isnum(a)	((a)==(a))
#define mgl_isfin(a)	((a)-(a)==mreal(0.))
#define mgl_isbad(a)	((a)-(a)!=mreal(0.))
#define mgl_sign(a)		((a)<0 ? -1:1)
#define mgl_int(a)		(long(a+((a)>=0 ? 0.5:-0.5)))
#endif
//-----------------------------------------------------------------------------
enum{	// types of predefined curvelinear coordinate systems
	mglCartesian = 0,	// no transformation
	mglPolar,
	mglSpherical,
	mglParabolic,
	mglParaboloidal,
	mglOblate,
	mglProlate,
	mglElliptic,
	mglToroidal,
	mglBispherical,
	mglBipolar,
	mglLogLog,
	mglLogX,
	mglLogY
};
//-----------------------------------------------------------------------------
// types of drawing
#define MGL_DRAW_WIRE	0	// fastest, no faces
#define MGL_DRAW_FAST	1	// fast, no color interpolation
#define MGL_DRAW_NORM	2	// high quality, slower
#define MGL_DRAW_LMEM	4	// low memory usage (direct to pixel)
#define MGL_DRAW_DOTS	8	// draw dots instead of primitives
#define MGL_DRAW_NONE	9	// no ouput (for testing only)
//-----------------------------------------------------------------------------
enum{	// Codes for warnings/messages
	mglWarnNone = 0,// Everything OK
	mglWarnDim,		// Data dimension(s) is incompatible
	mglWarnLow,		// Data dimension(s) is too small
	mglWarnNeg,	 	// Minimal data value is negative
	mglWarnFile, 	// No file or wrong data dimensions
	mglWarnMem,		// Not enough memory
	mglWarnZero, 	// Data values are zero
	mglWarnLeg,		// No legend entries
	mglWarnSlc,		// Slice value is out of range
	mglWarnCnt,		// Number of contours is zero or negative
	mglWarnOpen, 	// Couldn't open file
	mglWarnLId,		// Light: ID is out of range
	mglWarnSize, 	// Setsize: size(s) is zero or negative
	mglWarnFmt,		// Format is not supported for that build
	mglWarnTern, 	// Axis ranges are incompatible
	mglWarnNull, 	// Pointer is NULL
	mglWarnSpc,		// Not enough space for plot
	mglScrArg,		// Wrong argument(s) in MGL script
	mglScrCmd,		// Wrong command in MGL script
	mglScrLong,		// Too long line in MGL script
	mglScrStr,		// Unbalanced ' in MGL script
	mglScrTemp,		// Change temporary data in MGL script
	mglWarnEnd		// Maximal number of warnings (must be last)
};
//-----------------------------------------------------------------------------
#define MGL_DEF_PAL	"bgrcmyhlnqeupH"	// default palette
#define MGL_DEF_SCH	"BbcyrR"	// default palette
#define MGL_COLORS	"kwrgbcymhWRGBCYMHlenpquLENPQU"
//-----------------------------------------------------------------------------
/// Brushes for mask with symbol "-+=;oOsS~<>jdD*^" correspondingly
extern uint64_t mgl_mask_val[16];
#define MGL_MASK_ID		"-+=;oOsS~<>jdD*^"
#define MGL_SOLID_MASK	0xffffffffffffffff
//-----------------------------------------------------------------------------
#define MGL_TRANSP_NORM		0x000000
#define MGL_TRANSP_GLASS 	0x000001
#define MGL_TRANSP_LAMP		0x000002
#define MGL_ENABLE_CUT		0x000004 	///< Flag which determines how points outside bounding box are drown.
#define MGL_ENABLE_RTEXT 	0x000008 	///< Use text rotation along axis
#define MGL_AUTO_FACTOR		0x000010 	///< Enable autochange PlotFactor
#define MGL_ENABLE_ALPHA 	0x000020 	///< Flag that Alpha is used
#define MGL_ENABLE_LIGHT 	0x000040 	///< Flag of using lightning
#define MGL_TICKS_ROTATE 	0x000080 	///< Allow ticks rotation
#define MGL_TICKS_SKIP		0x000100 	///< Allow ticks rotation
// flags for internal use only
#define MGL_DISABLE_SCALE	0x000200 	///< Temporary flag for disable scaling (used for axis)
#define MGL_FINISHED 		0x000400 	///< Flag that final picture (i.e. mglCanvas::G) is ready
#define MGL_USE_GMTIME		0x000800 	///< Use gmtime instead of localtime
#define MGL_SHOW_POS		0x001000 	///< Switch to show or not mouse click position
#define MGL_CLF_ON_UPD		0x002000 	///< Clear plot before Update()
#define MGL_NOSUBTICKS		0x004000 	///< Disable subticks drawing (for bounding box)
#define MGL_LOCAL_LIGHT		0x008000 	///< Keep light sources for each inplot
#define MGL_VECT_FRAME		0x010000 	///< Use DrwDat to remember all data of frames
#define MGL_REDUCEACC		0x020000 	///< Reduce accuracy of points (to reduc size of output files)
#define MGL_PREFERVC 		0x040000 	///< Prefer vertex color instead of texture if output format supports
#define MGL_ONESIDED 		0x080000 	///< Render only front side of surfaces if output format supports (for debugging)
#define MGL_NO_ORIGIN 		0x100000 	///< Don't draw tick labels at axis origin
#define MGL_GRAY_MODE 		0x100000 	///< Convert all colors to gray ones
//-----------------------------------------------------------------------------
#if MGL_HAVE_C99_COMPLEX
#include <complex.h>
#if MGL_USE_DOUBLE
typedef double _Complex mdual;
#else
typedef float _Complex mdual;
#endif
#ifndef _Complex_I
#define _Complex_I	1.0i
#endif
const mdual mgl_I=_Complex_I;
#define mgl_abs(x)	cabs(x)
#endif
#ifdef __cplusplus
#include <string>
#include <vector>
#if defined(IUP_MSC_VER)
template class MGL_EXPORT std::allocator<char>;
template class MGL_EXPORT std::allocator<wchar_t>;
template struct MGL_EXPORT std::char_traits<char>;
template struct MGL_EXPORT std::char_traits<wchar_t>;
template class MGL_EXPORT std::basic_string< char, std::char_traits<char>, std::allocator<char> >;
template class MGL_EXPORT std::basic_string< wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >;
template class MGL_EXPORT std::vector<long>;
template class MGL_EXPORT std::vector<mreal>;
#endif
//-----------------------------------------------------------------------------
extern float mgl_cos[360];	///< contain cosine with step 1 degree
//-----------------------------------------------------------------------------
#include <complex>
#if defined(IUP_MSC_VER)
template class MGL_EXPORT std::complex<float>;
template class MGL_EXPORT std::complex<double>;
#endif
typedef std::complex<mreal> dual;
typedef std::complex<double> ddual;
#if !MGL_HAVE_C99_COMPLEX
#define mdual dual
#define mgl_I dual(0,1)
#define mgl_abs(x)	abs(x)
#endif
//-----------------------------------------------------------------------------
extern "C" {
#else
#include <complex.h>
typedef double _Complex ddual;
#define dual	mdual
#endif
/// Find length of wchar_t string (bypass standard wcslen bug)
double MGL_EXPORT_CONST mgl_hypot(double x, double y);
/// Find length of wchar_t string (bypass standard wcslen bug)
size_t MGL_EXPORT mgl_wcslen(const wchar_t *str);
/// Get RGB values for given color id or fill by -1 if no one found
void MGL_EXPORT mgl_chrrgb(char id, float rgb[3]);
/// Check if string contain color id and return its number
long MGL_EXPORT mgl_have_color(const char *stl);
/// Find symbol in string excluding {} and return its position or NULL
MGL_EXPORT const char *mglchr(const char *str, char ch);
/// Find any symbol from chr in string excluding {} and return its position or NULL
MGL_EXPORT const char *mglchrs(const char *str, const char *chr);
/// Set number of thread for plotting and data handling (for pthread version only)
void MGL_EXPORT mgl_set_num_thr(int n);
void MGL_EXPORT mgl_set_num_thr_(int *n);
void MGL_EXPORT mgl_test_txt(const char *str, ...);
void MGL_EXPORT mgl_set_test_mode(int enable);
/// Remove spaces at begining and at the end of the string
void MGL_EXPORT mgl_strtrim(char *str);
void MGL_EXPORT mgl_wcstrim(wchar_t *str);
/** Change register to lowercase (only for ANSI symbols) */
void MGL_EXPORT mgl_strlwr(char *str);
void MGL_EXPORT mgl_wcslwr(wchar_t *str);
/// Convert wchar_t* string into char* one
void MGL_EXPORT mgl_wcstombs(char *dst, const wchar_t *src, int size);
/// Clear internal data for speeding up FFT and Hankel transforms
void MGL_EXPORT mgl_clear_fft();
/// Set global warning message
void MGL_EXPORT mgl_set_global_warn(const char *text);
void MGL_EXPORT mgl_set_global_warn_(const char *text,int);
/// Get text of global warning message(s)
MGL_EXPORT const char *mgl_get_global_warn();
int MGL_EXPORT mgl_get_global_warn_(char *out, int len);
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif
//-----------------------------------------------------------------------------

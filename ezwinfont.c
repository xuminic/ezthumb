/*  ezwinfont.c

    Copyright (C) 2017  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of EZTHUMB, a utility to generate thumbnails

    EZTHUMB is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    EZTHUMB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
 * http://stackoverflow.com/questions/4577784/get-a-font-filename-based-on-
 * font-name-and-style-bold-italic 
 */
#ifdef  HAVE_CONFIG_H
#include <config.h>
#else
#error "Run configure first"
#endif

#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#ifdef HAVE_STRING_H
# if !defined STDC_HEADERS && defined HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <windows.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H 
#include FT_TRUETYPE_IDS_H

#include "iconv.h"

#include "libcsoup.h"
#include "ezthumb.h"

/* re-use the debug convention in libcsoup */
//#define CSOUP_DEBUG_LOCAL	SLOG_CWORD(EZTHUMB_MOD_CORE, SLOG_LVL_WARNING)
#define CSOUP_DEBUG_LOCAL	SLOG_CWORD(EZTHUMB_MOD_CORE, SLOG_LVL_DEBUG)
#include "libcsoup_debug.h"


static	TCHAR	*font_subkey[2] = {
	TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"),
	TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Fonts")
};

static	struct	{
	int	platform;
	int	encoding;
	char	*codename;
} encode_table[] = {
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_DEFAULT,  "UTF-16" },
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_UNICODE_1_1, "UTF-16" },
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_ISO_10646, "ISO-10646" },
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_UNICODE_2_0, "UTF-16" },
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_UNICODE_32, "UTF-32" },

	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_ROMAN, "MACINTOSH" },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_SIMPLIFIED_CHINESE, "gb2312" },
	
	{ TT_PLATFORM_MICROSOFT, TT_MS_ID_SYMBOL_CS, "ucs-2be" },
	{ TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, "ucs-2be" },

	{ -1, -1, NULL }		
};

#define	MAX_FONT_LOOKUP		2048

#define EZFONT_STYLE_NONE	0
#define EZFONT_STYLE_BOLD	1
#define EZFONT_STYLE_ITALIC	2
#define EZFONT_STYLE_ALL	3

static	int	style_eval[3][16] = {
	/*  EXPECT              RECEIVED                VALUE  */
	{ EZFONT_STYLE_NONE,	EZFONT_STYLE_NONE,	0 },
	{ EZFONT_STYLE_NONE,	EZFONT_STYLE_BOLD,	1 },
	{ EZFONT_STYLE_NONE,	EZFONT_STYLE_ITALIC,	1 },
	{ EZFONT_STYLE_NONE,	EZFONT_STYLE_ALL,	1 },
	{ EZFONT_STYLE_BOLD,	EZFONT_STYLE_NONE,	2 },
	{ EZFONT_STYLE_BOLD,	EZFONT_STYLE_BOLD,	0 },
	{ EZFONT_STYLE_BOLD,	EZFONT_STYLE_ITALIC,	3 },
	{ EZFONT_STYLE_BOLD,	EZFONT_STYLE_ALL,	1 },
	{ EZFONT_STYLE_ITALIC,	EZFONT_STYLE_NONE,	2 },
	{ EZFONT_STYLE_ITALIC,	EZFONT_STYLE_BOLD,	3 },
	{ EZFONT_STYLE_ITALIC,	EZFONT_STYLE_ITALIC,	0 },
	{ EZFONT_STYLE_ITALIC,	EZFONT_STYLE_ALL,	1 },
	{ EZFONT_STYLE_ALL,	EZFONT_STYLE_NONE,	2 },
	{ EZFONT_STYLE_ALL,	EZFONT_STYLE_BOLD,	1 },
	{ EZFONT_STYLE_ALL,	EZFONT_STYLE_ITALIC,	1 },
	{ EZFONT_STYLE_ALL,	EZFONT_STYLE_ALL,	0 }
};

static	struct	{
	char	*font_path;
	char	*font_face;
	int	style;		/* 1=bold 2=italic 3=bold&italic */
} winfont_list[MAX_FONT_LOOKUP];

static	int	winfont_idx = 0;


static int ezwinfont_add_fontface(char *ftpath, int style);
static int ezwinfont_convert(FT_SfntName *aname, char *buf, size_t blen);
static int ezwinfont_style_from_face(TCHAR *ftface);

int ezwinfont_open(void)
{
	OSVERSIONINFO	osinfo;
	HKEY		hkey;
	TCHAR		ftfile[MAX_PATH], ftface[MAX_PATH], wpbuf[MAX_PATH];
	DWORD		fflen, fclen;
	LONG		rc;
	char		*ftpath;
	int		i, ftstyle;

	osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&osinfo)) {
		return -1;
	}
	if (!GetWindowsDirectory(wpbuf, MAX_PATH)) {
		return -2;
	}
	if (osinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
		rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, font_subkey[1],
				0, KEY_ALL_ACCESS, &hkey);
	} else {
		rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, font_subkey[0],
				0, KEY_ALL_ACCESS, &hkey);
	}
	if (rc != ERROR_SUCCESS) {
		return -2;
	}

	wcsncat(wpbuf, TEXT("\\Fonts\\"), MAX_PATH);
	for (i = 0; ; i++) {
		fclen = sizeof(ftface) - 1;
		fflen = sizeof(ftfile) - 1;
		if (RegEnumValue(hkey, i, ftface, &fclen, NULL, NULL, 
				(LPBYTE) ftfile, &fflen) != ERROR_SUCCESS) {
			break;
		}

		ftstyle = ezwinfont_style_from_face(ftface);

		if (wcschr(ftfile, L'\\')) {
			/* it's already the full path */
			ftpath = smm_wcstombs_alloc(ftfile);
		} else {
			wcsncpy(ftface, wpbuf, MAX_PATH);
			wcsncat(ftface, ftfile, MAX_PATH);
			ftpath = smm_wcstombs_alloc(ftface);
		}
		//CDB_DEBUG(("Read: %s\n", ftpath));

		/* only support the TrueType */		
		if (csc_cmp_file_extname(ftpath,"ttf") &&
				csc_cmp_file_extname(ftpath,"ttc")) {
			smm_free(ftpath);
			continue;
		}

		/* the ftpath was dynamically allocated so if it's
		 * successfully added to the font lookup table, it shall
		 * be freed only in ezwinfont_close() */
		if (ezwinfont_add_fontface(ftpath, ftstyle) < 0) {
			/* free the memory if it was failed to add up */
			smm_free(ftpath);
		}
	}

	RegCloseKey(hkey);
	return winfont_idx;
}

int ezwinfont_close(void)
{
	int	i;

	CDB_DEBUG(("ezwinfont_close\n"));
	for (i = 0; i < winfont_idx; i++) {
		if (winfont_list[i].font_face) {
			smm_free(winfont_list[i].font_face);
		} else {
			smm_free(winfont_list[i].font_path);
		}
		winfont_list[i].font_path = NULL;
		winfont_list[i].font_face = NULL;
	}
	winfont_idx = 0;
	return 0;
}

char *ezwinfont_faceoff(char *fontface)
{
	char	buf[256];
	int	i, style, can_val, can_idx = -1;
	
	CDB_DEBUG(("FACEOFF: %s\n", fontface));
	
	style = EZFONT_STYLE_NONE;
	if (!strstr(fontface, ":bold")) {
		style |= EZFONT_STYLE_BOLD;
	}
	if (!strstr(fontface, ":italic")) {
		style |= EZFONT_STYLE_ITALIC;
	}

	if (*fontface == '@') {	/* skip the vertical flag */
		fontface++;
	}
	csc_strlcpy(buf, fontface, sizeof(buf));
	if ((fontface = strchr(buf, ':')) != NULL) {       
		*fontface = 0;
	}

	CDB_DEBUG(("FACEOFF: %s %d\n", buf, winfont_idx));
	for (i = 0; i < winfont_idx; i++) {
		if (winfont_list[i].font_face == NULL) {
			continue;
		}
		/*CDB_DEBUG(("FACEOFF: Searching %s %d -> %s %d\n",
 				winfont_list[i].font_face,
				winfont_list[i].style,
				buf, style));*/
		if (strstr(winfont_list[i].font_face, buf) == NULL) {
			continue;
		}

		if (can_idx == -1) {
			can_idx = i;	/* least matched */
			can_val = 
		} else if ((winfont_list[i].style & style) == style) {
			likely = i;	/* exactly matched */
		} else if (winfont_list[i].style & style) {
			if ((winfont_list[likely].style & style) == 0) {
				likely = i;	/* better matched */
			}
		} else {

			int	last, now;
			last = winfont_list[likely].style & style;
			now  = winfont_list[i].style & style;
			


		}
	}
	if (likely == -1) {
		return NULL;
	}
	return winfont_list[likely].font_path;
}

static int ezwinfont_add_fontface(char *ftpath, int ftstyle)
{
	FT_Library	library;
	FT_SfntName	aname;
	FT_Face		face;
	char		tmp[1024];
	int		i, namecnt;
	
	if (FT_Init_FreeType(&library) != 0) {
		return -1;
	}
	if (FT_New_Face(library, ftpath, 0, &face) != 0) {
		FT_Done_FreeType(library);
		return -2;
	}
	if (winfont_idx >= MAX_FONT_LOOKUP) {
		return -3;
	}

	/* store the allocated string of the font path */
	winfont_list[winfont_idx].font_path = ftpath;
	winfont_list[winfont_idx].font_face = NULL;
	winfont_list[winfont_idx].style = ftstyle;

	namecnt = FT_Get_Sfnt_Name_Count(face);
	for (i = 0, winfont_idx++; i < namecnt; i++) {
		if (FT_Get_Sfnt_Name(face, i, &aname)) {
			continue;
		}
		
#if 0
		ezwinfont_convert(&aname, tmp, sizeof(tmp));
		CDB_DEBUG(("FTNAME::%3d %3d %3d %5d %4d %s\n", 
					aname.platform_id,
					aname.encoding_id, 
					aname.name_id,
					aname.language_id,
					aname.string_len,
					tmp));
#endif

		if (aname.name_id != TT_NAME_ID_FONT_FAMILY) {
			continue;
		}
		if (winfont_idx >= MAX_FONT_LOOKUP) {
			break;
		}

		winfont_list[winfont_idx].style = ftstyle;

#if 0
		ftstyle = ezwinfont_convert(&aname, tmp, sizeof(tmp));
		CDB_DEBUG(("FTFACE:: %2d %2d %2d %s [%s]\n", ftstyle,
			aname.platform_id, aname.encoding_id, tmp, ftpath));
#else
		ezwinfont_convert(&aname, tmp, sizeof(tmp));
#endif
		winfont_list[winfont_idx].font_path = ftpath; /* just a copy */
		winfont_list[winfont_idx].font_face = csc_strcpy_alloc(tmp, 0);
		winfont_idx++;
	}

	FT_Done_Face(face);
	FT_Done_FreeType(library);
	return i;
}

static int ezwinfont_convert(FT_SfntName *aname, char *buf, size_t blen)
{
	iconv_t cd;
	char	*iconv_in = (char*) aname->string;
	size_t	i, lenin = aname->string_len;

	for (i = 0; encode_table[i].platform != -1; i++) {
		if (aname->platform_id != encode_table[i].platform) {
			continue;
		}
		if (aname->encoding_id != encode_table[i].encoding) {
			continue;
		}
		if (encode_table[i].codename == NULL) {
			break;
		}

		cd = iconv_open("utf-8", encode_table[i].codename);
		if (cd == (iconv_t) -1) {
			break;
		}
		i = blen;
		iconv(cd, (const char **) &iconv_in, &lenin, &buf, &blen);
		iconv_close(cd);
		*buf = 0;	/* iconv doesn't end string */
		return (int)(i - blen);
	}
	/* doesn't find the encoding, or doesn't need to convert */
	memset(buf, 0, blen);
	memcpy(buf, iconv_in, lenin);
	CDB_DEBUG(("ezwinfont_convert: %d %s\n", lenin, iconv_in));
	return -1;
}

static int ezwinfont_style_from_face(TCHAR *ftface)
{
	char	*s = smm_wcstombs_alloc(ftface);
	int	style = EZFONT_STYLE_NONE;

	if (s) {
		if (!strstr(s, "Bold")) {
			style |= EZFONT_STYLE_BOLD;
		}
		if (!strstr(s, "Italic")) {
			style |= EZFONT_STYLE_ITALIC;
		}
		smm_free(s);
	}
	return style;
}

#if 0
static void CDB_DUMP(char *prompt, void *data, int len)
{
	unsigned char   *s = data;
	char    buf[128], tmp[16];
	int     i;

	if (data == NULL) {
		return;
	}
	if ((len < 1) || (len > 16)) {
		len = 16;
	}
	sprintf(buf, "%s: ", prompt);
	for (i = 0; i < len; i++) {
		sprintf(tmp, "%02x ", s[i]);
		strcat(buf, tmp);
	}
	CDB_SHOW(("%s\n", buf));
}


int CALLBACK GetFontsCallback(const LOGFONT *pk_Font, const TEXTMETRIC* 
		pk_Metric, DWORD e_FontType, LPARAM lParam)
{
	ENUMLOGFONTEX	*ftext = (ENUMLOGFONTEX *) pk_Font;
	char	*ftface, *ftname;

	ftface = smm_wcstombs_alloc(pk_Font->lfFaceName);
	ftname = smm_wcstombs_alloc(ftext->elfFullName);
	CDB_DEBUG(("FONT: %s # %s\n", ftface, ftname));
	smm_free(ftname);
	smm_free(ftface);
	return 1;
}

void dump_win_face(char *fontface)
{
	LOGFONT	lf;
	HDC	hdc;
	TCHAR	*s;

	memset(&lf, 0, sizeof(lf));
	//lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfCharSet = ANSI_CHARSET;

	s = smm_mbstowcs_alloc(fontface);
	CDB_DUMP("WCSFACE", s, 0);
	smm_free(s);

	s = iupwinStrToSystem(fontface);
	CDB_DUMP("IUPFACE", s, 0);
	wcsncpy(lf.lfFaceName, s, LF_FACESIZE);
	free(s);

	hdc = CreateCompatibleDC(NULL);
	EnumFontFamiliesEx(hdc, &lf, GetFontsCallback, 0, 0);
	DeleteDC(hdc);
}

/* http://stackoverflow.com/questions/4577784/get-a-font-filename-based-on-
 * font-name-and-style-bold-italic */
char *GetFontFile(char *fontface)
{
	OSVERSIONINFO	osinfo;
	HKEY		hkey;
	TCHAR		*subkey, reg_name[2 * MAX_PATH];
	char		*ftname, *ftpath, reg_data[2 * MAX_PATH];
	DWORD		namelen, datalen;
	int		i, fclen;

	dump_win_face(fontface);

	osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&osinfo)) {
		return NULL;
	}

	if (osinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
		subkey = font_subkey[1];
	} else {
		subkey = font_subkey[0];
	}

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subkey, 0, KEY_ALL_ACCESS, 
				&hkey) != ERROR_SUCCESS) {
		return NULL;
	}

	fclen = strlen(fontface);
	for (i = 0; ; i++) {
		namelen = sizeof(reg_name) - 1;
		datalen = sizeof(reg_data) - 1;
		if (RegEnumValue(hkey, i, reg_name, &namelen, NULL, NULL, 
				(LPBYTE)reg_data, &datalen) != ERROR_SUCCESS) {
			break;
		}

		if ((ftname = smm_wcstombs_alloc(reg_name)) == NULL) {
			break;
		}
		if ((ftpath =  smm_wcstombs_alloc(reg_data)) == NULL) {
			smm_free(ftname);
			break;
		}
		
		/* ezthumb only working with TTF fonts */
		if (csc_cmp_file_extname(ftpath,"ttf") &&
				csc_cmp_file_extname(ftpath,"ttc")) {
			smm_free(ftpath);
			smm_free(ftname);
			continue;
		}

		dump_win_font(ftname, ftpath);

		if (!strncasecmp(fontface, ftname, fclen)) {
			char *s = smm_fontpath(ftpath, NULL);
			smm_free(ftpath);
			smm_free(ftname);
			RegCloseKey(hkey);
			return s;
		}
		smm_free(ftpath);
		smm_free(ftname);
	}

	RegCloseKey(hkey);
	return NULL;
}
#endif



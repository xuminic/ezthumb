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

#define LIBICONV_STATIC
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

struct  struct	_encode_table	{
	int	platform;
	int	encoding;
	char	*codename;
} encode_table[] = {
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_DEFAULT,  "UNICODE" },
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_UNICODE_1_1,	NULL },
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_ISO_10646, NULL },
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_UNICODE_2_0, NULL },
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_UNICODE_32, NULL },

	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_ROMAN, "macintosh" },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_SIMPLIFIED_CHINESE, "gb2312" },
	
	{ TT_PLATFORM_MICROSOFT, TT_MS_ID_SYMBOL_CS, NULL },
	{ TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, "ucs-2be" },

	{ -1, -1, NULL }		
};

struct	struct	{
	char	*font_path;
	char	*font_face;
};

int ezwinfont_open(void)
{
	OSVERSIONINFO	osinfo;
	HKEY		hkey;
	TCHAR		ftfile[MAX_PATH], wpbuf[MAX_PATH];
	DWORD		fflen;
	LONG		rc;
	char		*ftpath;
	int		i;

	osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&osinfo)) {
		return -1;
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


	for (i = 0; ; i++) {
		fflen = sizeof(ftfile) - 1;
		if (RegEnumValue(hkey, i, NULL, NULL, NULL, NULL, 
				(LPBYTE) ftfile, &fflen) != ERROR_SUCCESS) {
			break;
		}

		if (wcschr(ftfile, L'\\')) {
			/* it's already the full path */
			ftpath = smm_wcstombs_alloc(ftfile);
		} else if (!GetWindowsDirectory(wpbuf, MAX_PATH)) {
			ftpath = NULL;
		} else {
			wcsncat(wpbuf, TEXT("\\Fonts\\"), MAX_PATH);
			wcsncat(wpbuf, ftfile, MAX_PATH);
			ftpath = smm_wcstombs_alloc(wpbuf);
		}

		
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
}

int ezwinfont_close(void)
{
}

char *ezwinfont_faceoff(char *fontface)
{
}

static int ezwinfont_add_fontface(char *ftpath)
{
	FT_Library	library;
	FT_SfntName	aname;
	FT_Face		face;
	char		*s, tmp[256];
	int		i, namecnt;
	
	if (csc_cmp_file_extname(ftpath,"ttf") &&
			csc_cmp_file_extname(ftpath,"ttc")) {
		return 0;	/* only TTF was supported */
	}

	FT_Init_FreeType(&library);
	FT_New_Face(library, ftpath, 0, &face );

	namecnt = FT_Get_Sfnt_Name_Count(face);
	for (i = 0; i < namecnt; i++) {
		if (FT_Get_Sfnt_Name(face, i, &aname)) {
			continue;
		}
		if (aname.name_id != TT_NAME_ID_FONT_FAMILY) {
			continue;
		}

		memset(tmp, 0, sizeof(tmp));
		memcpy(tmp, aname.string, aname.string_len);
		if (aname.encoding_id == TT_MS_ID_UNICODE_CS) {
			iconv_t cd;
			char *iconv_in = (char*) aname.string;
			char *iconv_out = tmp;
			size_t	lenin, lenout;

			lenin = aname.string_len;
			lenout = sizeof(tmp);
			cd = iconv_open ("utf-8", "ucs-2be");
			if (cd < 0) {
				CDB_DEBUG(("BUGGAR\n"));
			}
			iconv(cd, &iconv_in, &lenin, &iconv_out, &lenout);
			iconv_close(cd);
		}
		CDB_DEBUG(("Font enum: %d %d %s\n",
				aname.platform_id,
				aname.encoding_id,
				tmp));

	}

	FT_Done_Face    ( face );
	FT_Done_FreeType( library );

	/*if (!strcasecmp(ftpath, "simhei.ttf")) {
		font_attirb(ftpath);
	}*/
}

static int ezwinfont_convert(FT_SfntName *aname, char *buf, int blen)
{
	iconv_t cd;
	char	*iconv_in, *iconv_out;
	size_t	lenin, lenout;
	int	i;

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

		iconv_in  = (char*) aname->string;
		lenin     = aname->string_len;
		iconv_out = buf;
		lenout    = blen;
		
		if ((cd = iconv_open("utf-8", encode_table[i].codename)) < 0) {
			break;
		}
			if (cd < 0) {
				CDB_DEBUG(("BUGGAR\n"));
			}
			iconv(cd, &iconv_in, &lenin, &iconv_out, &lenout);
			iconv_close(cd);


}

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


/* http://stackoverflow.com/questions/4577784/get-a-font-filename-based-on-font-name-and-style-bold-italic */
int font_attirb(char *fontpath)
{
	FT_Library	library;
	FT_SfntName	aname;
	FT_Face		face;
	char		tmp[1024];
	int		i, n;

	FT_Init_FreeType( &library );

	fontpath = smm_fontpath(fontpath, NULL);
	FT_New_Face(library, fontpath, 0, &face );
	smm_free(fontpath);

	n = FT_Get_Sfnt_Name_Count(face);
	for (i = 0; i < n; i++) {
		if (FT_Get_Sfnt_Name(face, i, &aname)) {
			continue;
		}

		memset(tmp, 0, sizeof(tmp));
		memcpy(tmp, aname.string, aname.string_len);

		fontpath = tmp;
		if (aname.encoding_id == TT_MS_ID_UNICODE_CS) {
			fontpath = smm_wcstombs_alloc((void*) tmp);
		}
		CDB_DEBUG(("FTNAME::%3d %3d %3d %5d %4d %s\n", 
					aname.platform_id,
					aname.encoding_id, 
					aname.name_id,
					aname.language_id,
					aname.string_len,
					fontpath));

		if (fontpath != tmp) {
			smm_free(fontpath);
		}
	}

	FT_Done_Face    ( face );
	FT_Done_FreeType( library );
	return 0;
}

void dump_win_font(char *ftface, char *ftpath)
{
	FT_Library	library;
	FT_SfntName	aname;
	FT_Face		face;
	char		*s, tmp[256];
	int		i, n;
	
	if (strcasecmp(ftpath, "simhei.ttf")) {
		return;
	}
	

	FT_Init_FreeType( &library );

	ftpath = smm_fontpath(ftpath, NULL);
	FT_New_Face(library, ftpath, 0, &face );
	smm_free(ftpath);

	n = FT_Get_Sfnt_Name_Count(face);
	for (i = 0; i < n; i++) {
		if (FT_Get_Sfnt_Name(face, i, &aname)) {
			continue;
		}
		if (aname.name_id != TT_NAME_ID_FONT_FAMILY) {
			continue;
		}

		memset(tmp, 0, sizeof(tmp));
		memcpy(tmp, aname.string, aname.string_len);
		if (aname.encoding_id == TT_MS_ID_UNICODE_CS) {
			iconv_t cd;
			char *iconv_in = (char*) aname.string;
			char *iconv_out = tmp;
			size_t	lenin, lenout;

			lenin = aname.string_len;
			lenout = sizeof(tmp);
			cd = iconv_open ("utf-8", "ucs-2be");
			if (cd < 0) {
				CDB_DEBUG(("BUGGAR\n"));
			}
			iconv(cd, &iconv_in, &lenin, &iconv_out, &lenout);
			iconv_close(cd);
		}
		CDB_DEBUG(("Font enum: %d %d %s\n",
				aname.platform_id,
				aname.encoding_id,
				tmp));


		/*
		if (aname.encoding_id == TT_MS_ID_SYMBOL_CS) {
			CDB_DEBUG(("Font enum: %s [%s]\n", ftface, tmp));
		} else if (aname.encoding_id == TT_MS_ID_UNICODE_CS) {
			s = smm_wcstombs_alloc((void*) tmp);
			CDB_DEBUG(("Font enum: %s [%s]\n", ftface, s));
			smm_free(s);
		} else {
			s = smm_wcstombs_alloc((void*) tmp);
			CDB_DEBUG(("Font enum: %s [%s][%d]\n", 
					ftface, tmp, aname.string_len));
			smm_free(s);
		}*/
	}

	FT_Done_Face    ( face );
	FT_Done_FreeType( library );

	/*if (!strcasecmp(ftpath, "simhei.ttf")) {
		font_attirb(ftpath);
	}*/
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




/*  ezttf.c

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

#ifdef	CFG_WIN32RT
#include <windows.h>
#endif

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
//#define CSOUP_DEBUG_LOCAL	SLOG_CWORD(EZTHUMB_MOD_CORE, SLOG_LVL_MODULE)
#include "libcsoup_debug.h"


/* True type font's face name encoding lookup table */
static	struct	{
	int	platform;
	int	encoding;
	char	*codename;
} ttf_fne_lut[] = {
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_DEFAULT,  "UTF-16BE" },
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_UNICODE_1_1, NULL },
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_ISO_10646, "ISO-10646" },
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_UNICODE_2_0, "UTF-16BE" },
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_UNICODE_32, "UTF-32BE" },
	{ TT_PLATFORM_APPLE_UNICODE, TT_APPLE_ID_VARIANT_SELECTOR, NULL },

	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_ROMAN, "MACINTOSH" },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_JAPANESE, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_TRADITIONAL_CHINESE, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_KOREAN, "UTF-16BE" },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_ARABIC, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_HEBREW, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_GREEK, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_RUSSIAN, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_RSYMBOL, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_DEVANAGARI, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_GURMUKHI, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_GUJARATI, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_ORIYA, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_BENGALI, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_TAMIL, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_TELUGU, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_KANNADA, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_MALAYALAM, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_SINHALESE, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_BURMESE, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_KHMER, "MACINTOSH" },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_THAI, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_LAOTIAN, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_GEORGIAN, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_ARMENIAN, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_MALDIVIAN, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_SIMPLIFIED_CHINESE, "GBK" },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_TIBETAN, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_MONGOLIAN, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_GEEZ, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_SLAVIC, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_VIETNAMESE, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_SINDHI, NULL },
	{ TT_PLATFORM_MACINTOSH, TT_MAC_ID_UNINTERP, NULL },

	{ TT_PLATFORM_ISO, TT_ISO_ID_7BIT_ASCII, "ASCII" },
	{ TT_PLATFORM_ISO, TT_ISO_ID_10646, "ISO-10646" },
	{ TT_PLATFORM_ISO, TT_ISO_ID_8859_1, "8859_1" },
	
	{ TT_PLATFORM_MICROSOFT, TT_MS_ID_SYMBOL_CS, "ucs-2be" },
	{ TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, "ucs-2be" },
	{ TT_PLATFORM_MICROSOFT, TT_MS_ID_SJIS, "SJIS" },
	{ TT_PLATFORM_MICROSOFT, TT_MS_ID_GB2312, "GBK" },
	{ TT_PLATFORM_MICROSOFT, TT_MS_ID_BIG_5, "BIG5" },
	{ TT_PLATFORM_MICROSOFT, TT_MS_ID_WANSUNG, "ucs-2be" },
	{ TT_PLATFORM_MICROSOFT, TT_MS_ID_JOHAB, "ucs-2be" },
	{ TT_PLATFORM_MICROSOFT, TT_MS_ID_UCS_4, "UTF-32BE" },

	{ -1, -1, NULL }		
};

#define	MAX_FONT_LOOKUP		2048

#define EZFONT_STYLE_NONE	0
#define EZFONT_STYLE_BOLD	FT_STYLE_FLAG_BOLD
#define EZFONT_STYLE_ITALIC	FT_STYLE_FLAG_ITALIC
#define EZFONT_STYLE_ALL	(FT_STYLE_FLAG_BOLD | FT_STYLE_FLAG_ITALIC)

static	struct	{
	char	*font_path;
	char	*font_face;
	int	style;		/* 1=bold 2=italic 3=bold&italic */
} ttf_lut[MAX_FONT_LOOKUP];

static	int	ttf_idx = 0;


static int ezttf_add_fontface(char *ftpath, int style);
static char *ezttf_face_alloc(FT_SfntName *aname);
static int ezttf_style_value(int expected, int received);
static char *ez_convert(char *fromcode, char *mem, size_t mlen);
static void ez_dump(char *prompt, void *mem, int mlen);


int ezttf_close(void)
{
	int	i;

	for (i = 0; i < ttf_idx; i++) {
		if (ttf_lut[i].font_face) {
			smm_free(ttf_lut[i].font_face);
		} else {
			smm_free(ttf_lut[i].font_path);
		}
		ttf_lut[i].font_path = NULL;
		ttf_lut[i].font_face = NULL;
	}
	ttf_idx = 0;
	return 0;
}

char *ezttf_faceoff(char *fontface)
{
	char	buf[256];
	int	i, value, style, can_val, can_idx = -1;
	
	//CDB_DEBUG(("FACEOFF: %s\n", fontface));
	
	style = EZFONT_STYLE_NONE;
	if (strstr(fontface, ":bold")) {
		style |= EZFONT_STYLE_BOLD;
	}
	if (strstr(fontface, ":italic")) {
		style |= EZFONT_STYLE_ITALIC;
	}

	if (*fontface == '@') {	/* skip the vertical flag */
		fontface++;
	}
	csc_strlcpy(buf, fontface, sizeof(buf));
	if ((fontface = strchr(buf, ':')) != NULL) {       
		*fontface = 0;
	}

	//CDB_DEBUG(("FACEOFF: %s %d\n", buf, ttf_idx));
	for (i = 0; i < ttf_idx; i++) {
		if (ttf_lut[i].font_face == NULL) {
			continue;
		}
		/*CDB_DEBUG(("FACEOFF: Searching %s %d -> %s %d\n",
 				ttf_lut[i].font_face,
				ttf_lut[i].style,
				buf, style));*/
		if (strstr(ttf_lut[i].font_face, buf) == NULL) {
			continue;
		}

		value = ezttf_style_value(ttf_lut[i].style, style);
		if (can_idx == -1) {
			can_idx = i;	/* least matched */
			can_val = value;
		} else if (value < can_val) {
			can_idx = i;
			can_val = value;
		}
	}
	if (can_idx == -1) {
		return NULL;
	}
	return ttf_lut[can_idx].font_path;
}

static int ezttf_add_fontface(char *ftpath, int ftstyle)
{
	FT_Library	library;
	FT_SfntName	aname;
	FT_Face		face;
	char		*tmp;
	int		i, namecnt;
	
	/* only support the TrueType */
	/*if (csc_cmp_file_extname(ftpath,"ttf") &&
			csc_cmp_file_extname(ftpath,"ttc")) {
		return -4;
	}*/
	if (FT_Init_FreeType(&library) != 0) {
		return -1;
	}
	if (FT_New_Face(library, ftpath, 0, &face) != 0) {
		FT_Done_FreeType(library);
		return -2;
	}
	if (ttf_idx >= MAX_FONT_LOOKUP) {
		return -3;
	}

	/* only support the TrueType fonts whom have the font faces */
	if ((namecnt = FT_Get_Sfnt_Name_Count(face)) < 1) {
		FT_Done_Face(face);
		FT_Done_FreeType(library);
		return -4;
	}

	/* the font style can be loaded from Windows' registry, 
	 * or if in *nix, loaded from the ttf file itself */
	if (ftstyle < 0) {
		ftstyle = face->style_flags;
	}

	CDB_MODL(("FTPATH: %s [%d]\n", ftpath, ftstyle));

	/* store the allocated string of the font path */
	ttf_lut[ttf_idx].font_path = ftpath;
	ttf_lut[ttf_idx].font_face = NULL;
	ttf_lut[ttf_idx].style = ftstyle;

	for (i = 0, ttf_idx++; i < namecnt; i++) {
		if (FT_Get_Sfnt_Name(face, i, &aname)) {
			continue;
		}
		
		/*if (strstr(ftpath, "Padauk") && 
				(aname.name_id == TT_NAME_ID_FONT_FAMILY)) {
			CDB_DEBUG(("FTNAME::%3d %3d %3d %5d %4d %s\n", 
					aname.platform_id, aname.encoding_id, 
					aname.name_id, aname.language_id,
					aname.string_len, aname.string));
		}*/
#if 0
		if ((tmp = ezttf_face_alloc(&aname)) != NULL) {
			CDB_DEBUG(("FTNAME::%3d %3d %3d %5d %4d %s\n", 
					aname.platform_id,
					aname.encoding_id, 
					aname.name_id,
					aname.language_id,
					aname.string_len,
					tmp));
			smm_free(tmp);
		}
#endif

		if (aname.name_id != TT_NAME_ID_FONT_FAMILY) {
			continue;
		}
		if (ttf_idx >= MAX_FONT_LOOKUP) {
			break;
		}

		if ((tmp = ezttf_face_alloc(&aname)) != NULL) {
			/* it's just a link to the font path */
			ttf_lut[ttf_idx].font_path = ftpath; 
			ttf_lut[ttf_idx].font_face = tmp;
			ttf_lut[ttf_idx].style = ftstyle;
			ttf_idx++;
		}
	}

	FT_Done_Face(face);
	FT_Done_FreeType(library);
	return i;
}

static char *ezttf_face_alloc(FT_SfntName *aname)
{
	char	*mp;
	int	i;

#define	TRY_CHARSET(s)	CDB_DEBUG(("Trying %s: %s\n", (s), \
	ez_convert((s), (char*)aname->string, aname->string_len)));

	for (i = 0; ttf_fne_lut[i].platform != -1; i++) {
		if (aname->platform_id != ttf_fne_lut[i].platform) {
			continue;
		}
		if (aname->encoding_id != ttf_fne_lut[i].encoding) {
			continue;
		}
		if (ttf_fne_lut[i].codename == NULL) {
			break;
		}

		mp = ez_convert(ttf_fne_lut[i].codename, 
				(char*) aname->string, aname->string_len);

		CDB_MODL(("FTFACE:: %2d %2d %2d %s\n", strlen(mp),
				aname->platform_id, aname->encoding_id, mp));

		return csc_strcpy_alloc(mp, 0);
	}

	/* doesn't find the encoding, or doesn't need to convert */
	/* issue a warning and do a quick detection */
	CDB_DEBUG(("Font Face Charset Unknown: %d %d %d %d %d\n", 
			aname->platform_id, aname->encoding_id, aname->name_id,
			aname->language_id, aname->string_len));

	/* dump the font face string */
	ez_dump("Font Face String", aname->string, aname->string_len);
	TRY_CHARSET("UTF-16");
	TRY_CHARSET("UTF-16BE");
	TRY_CHARSET("UTF-16LE");


	/* simply allocate and copy out the string field */
	if ((mp = malloc(aname->string_len + 4)) != NULL) {
		memcpy(mp, aname->string, aname->string_len);
		mp[aname->string_len] = 0;
	}
	return mp;
}


static	int	style_eval[16][3] = {
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

static int ezttf_style_value(int expected, int received)
{
	int	i;

	for (i = 0; i < 16; i++) {
		if ((style_eval[i][0] == expected) && 
				(style_eval[i][1] == received)) {
			return style_eval[i][2];
		}
	}
	return 5;	/* lowest */
}

static char *ez_convert(char *fromcode, char *mem, size_t mlen)
{
	static	char	firmbuf[2048];
	iconv_t	cd;
	char	*buf = firmbuf;
	size_t	len = sizeof(firmbuf);

	if ((cd = iconv_open("utf-8", fromcode)) == (iconv_t) -1) {
		return NULL;
	}

#ifdef	CFG_WIN32RT
	iconv(cd, (const char **) &mem, &mlen, &buf, &len);
#else
	iconv(cd, &mem, &mlen, &buf, &len);
#endif
	iconv_close(cd);
	*buf = 0; 	/* iconv doesn't end strings */
	return firmbuf;
}

static void ez_dump(char *prompt, void *mem, int mlen)
{
	char	*mp, buf[256];
	int	len, goc;

#define	EZDUMP_FLAG	CSC_MEMDUMP_NO_ADDR

	for (mp = mem, len = mlen; len > 0; mp += goc, len -= goc) {
		goc = len > 16 ? 16 : len;
		csc_memdump_line(mp, goc, EZDUMP_FLAG, buf, sizeof(buf));
		CDB_DEBUG(("%s: %s\n", prompt, buf));
	}
}


#ifdef	CFG_WIN32RT
static	TCHAR	*font_subkey[2] = {
	TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"),
	TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Fonts")
};

static int ezttf_add_regface(TCHAR *regface, char *ftpath, int ftstyle);
static int ezttf_style_from_face(TCHAR *ftface);
static int CALLBACK ezttf_callback(const LOGFONT *pk_Font, 
	const TEXTMETRIC* pk_Metric, DWORD e_FontType, LPARAM lParam);

int ezttf_open(void)
{
	OSVERSIONINFO	osinfo;
	HKEY		hkey;
	TCHAR		ftfile[MAX_PATH], ftface[MAX_PATH], wpbuf[MAX_PATH];
	DWORD		fflen, fclen;
	LONG		rc;
	char		*ftpath;
	int		i, wpend, ftstyle;

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

	/* the font face and fond path are stored in the lookup table
	 * in utf-8, which came from utf-16be */
	smm_codepage_set(CP_UTF8);

	wcsncat(wpbuf, TEXT("\\Fonts\\"), MAX_PATH);
	wpend = wcslen(wpbuf);
	for (i = 0; ; i++) {
		fclen = sizeof(ftface) - 1;
		fflen = sizeof(ftfile) - 1;
		if (RegEnumValue(hkey, i, ftface, &fclen, NULL, NULL, 
				(LPBYTE) ftfile, &fflen) != ERROR_SUCCESS) {
			break;
		}

		ftstyle = ezttf_style_from_face(ftface);

		if (wcschr(ftfile, L'\\')) {
			/* it's already the full path */
			ftpath = smm_wcstombs_alloc(ftfile);
		} else {
			wcsncpy(&wpbuf[wpend], ftfile, MAX_PATH - wpend - 1);
			ftpath = smm_wcstombs_alloc(wpbuf);
		}
		//CDB_DEBUG(("Read: %s\n", ftpath));

		/* the ftpath was dynamically allocated so if it's
		 * successfully added to the font lookup table, it shall
		 * be freed only in ezttf_close() */
		if (ezttf_add_fontface(ftpath, ftstyle) < 0) {
			/* free the memory if it was failed to add up */
			smm_free(ftpath);
		} else {
			ezttf_add_regface(ftface, ftpath, ftstyle);
		}
	}

	RegCloseKey(hkey);
	CDB_DEBUG(("%d FONT LOADED\n", ttf_idx));

	/* reset the current codepage to ACP so this function can be used
	 * in both the command line and the IUP GUI */
	smm_codepage_reset();
	return ttf_idx;
}

char *ezttf_acp2utf8_alloc(char *acp)
{
	TCHAR	*buf;
	int	len;

	len = MultiByteToWideChar(GetACP(), 0, acp, -1, NULL, 0);
	if ((len > 0) && (buf = smm_alloc((len + 1) * sizeof(TCHAR)))) {
		MultiByteToWideChar(GetACP(), 0, acp, -1, buf, len);

		len = WideCharToMultiByte(CP_UTF8,
				0, buf, -1, NULL, 0, NULL, NULL);
		if ((len > 0) && (acp = smm_alloc(len + 1))) {
			WideCharToMultiByte(CP_UTF8,
					0, buf, -1, acp, len, NULL, NULL);
		}
		smm_free(buf);
	}
	return acp;
}


/* The font face came from the registry like
 *   "Gulim & GulimChe & Dotum & DotumChe (TrueType)" 
 * should be seperated to these fontfaces:
 *   "Gulim", "GulimChe", "Dotum" and "DotumChe"  
 * so all of above could be searched by face name. */
static int ezttf_add_regface(TCHAR *regface, char *ftpath, int ftstyle)
{
	char	*s, *token, *ftface = smm_wcstombs_alloc(regface);
	

	if ((s = strchr(ftface, '(')) != NULL) {
		*s = 0;		/* remove the tailing "(TrueType)" */
	}

	s = ftface;
	while ((s = csc_cuttoken(s, &token, "&")) != NULL) {
		if (ttf_idx >= MAX_FONT_LOOKUP) {
			break;
		}

		/* it's just a link to the font path */
		ttf_lut[ttf_idx].font_path = ftpath; 
		ttf_lut[ttf_idx].font_face = 
			csc_strcpy_alloc(token, 0);
		ttf_lut[ttf_idx].style = ftstyle;
		if (ttf_lut[ttf_idx].font_face) {
			ttf_idx++;
			CDB_MODL(("FTFACE:: Added %s\n", token));
		}
	}
	smm_free(ftface);
	return 0;
}

static int ezttf_style_from_face(TCHAR *ftface)
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

static int CALLBACK ezttf_callback(const LOGFONT *pk_Font, 
	const TEXTMETRIC* pk_Metric, DWORD e_FontType, LPARAM lParam)
{
	//ENUMLOGFONTEX	*ftext = (ENUMLOGFONTEX *) pk_Font;
	TCHAR	facebuf[LF_FACESIZE*2];
	char	*ftface, *ftname;

	(void) pk_Metric; (void) lParam;

	if (e_FontType != TRUETYPE_FONTTYPE) {
		return 1;
	}

	wcscpy(facebuf, pk_Font->lfFaceName);
	if (pk_Font->lfWeight >= 700) {
		wcscat(facebuf, TEXT(":bold"));
	}
	if (pk_Font->lfItalic) {
		wcscat(facebuf, TEXT(":italic"));
	}

	//ftname = smm_wcstombs_alloc(ftext->elfFullName);
	//CDB_DEBUG(("FONT: %s # %s\n", ftface, ftname));
	if ((ftface = smm_wcstombs_alloc(facebuf)) != NULL) {
		ftname = ezttf_faceoff(ftface);
		CDB_SHOW(("%s: %s\n", ftface, ftname));
		smm_free(ftface);
	}
	return 1;
}

/* Note that the font face is supposed to be utf-8 */
int ezttf_testing(char *fontface)
{
	LOGFONT	lf;
	HDC	hdc;
	TCHAR	*s;

	smm_codepage_set(CP_UTF8);

	memset(&lf, 0, sizeof(lf));
	lf.lfCharSet = DEFAULT_CHARSET;

	if (fontface) {
		s = smm_mbstowcs_alloc(fontface);
		//s = iupwinStrToSystem(fontface);
		wcsncpy(lf.lfFaceName, s, LF_FACESIZE);
		free(s);
	}

	hdc = CreateCompatibleDC(NULL);
	EnumFontFamiliesEx(hdc, &lf, ezttf_callback, 0, 0);
	DeleteDC(hdc);

	smm_codepage_reset();
	return 0;
}

#else	/* no CFG_WIN32RT */

static	char	*font_subkey[] = {
	"/usr/share/fonts",
	"/usr/share/X11/fonts",
	NULL
};

static int findfont(void *option, char *path, int type, void *info);

int ezttf_open(void)
{
	int		i;

	for (i = 0; font_subkey[i]; i++) {
		smm_pathtrek(font_subkey[i], 0, findfont, NULL);
	}
	CDB_DEBUG(("%d FONT LOADED\n", ttf_idx));
	return ttf_idx;
}

char *ezttf_acp2utf8_alloc(char *acp)
{
	return csc_strcpy_alloc(acp, 0);
}

int ezttf_testing(char *fontface)
{
	char	*ftname;
	int	i;

	if (fontface) {
		ftname = ezttf_faceoff(fontface);
		CDB_SHOW(("%s: %s\n", fontface, ftname));
	} else {
		/* FIXME: should call a function in main.c to enumerate
		 * all font faces */
		for (i = 0; i < ttf_idx; i++) {
			CDB_SHOW(("%s: %s [%d]\n", ttf_lut[i].font_face, 
				ttf_lut[i].font_path, ttf_lut[i].style));
		}
	}
	return 0;
}

static int findfont(void *option, char *path, int type, void *info)
{
	char	*ftpath;

	(void)info; (void)option;             /* stop the gcc warning */

	switch (type) {
	case SMM_MSG_PATH_ENTER:
		CDB_MODL(("Entering %s:\n", path));
		break;
	case SMM_MSG_PATH_EXEC:
		/* the ftpath was dynamically allocated so if it's
		 * successfully added to the font lookup table, it shall
		 * be freed only in ezttf_close() */
		if ((ftpath = smm_cwd_alloc(strlen(path) + 8)) == NULL) {
			break;
		}
		strcat(ftpath, SMM_DEF_DELIM);
		strcat(ftpath, path);

		if (ezttf_add_fontface(ftpath, -1) < 0) {
			/* free the memory if it was failed to add up */
			smm_free(ftpath);
		}
		break;
	case SMM_MSG_PATH_BREAK:
		CDB_MODL(("Failed to process %s\n", path));
		break;
	case SMM_MSG_PATH_LEAVE:
		CDB_MODL(("Leaving %s\n", path));
		break;
	}
	return SMM_NTF_PATH_NONE;
}

#endif	/* CFG_WIN32RT */


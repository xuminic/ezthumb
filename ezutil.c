
/*  ezutil.c

    Copyright (C) 2013  "Andy Xuming" <xuming@users.sourceforge.net>

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

#include <ctype.h>
#include <math.h>

#include "libcsoup.h"
#include "ezthumb.h"

/* re-use the debug convention in libcsoup */
#define CSOUP_DEBUG_LOCAL	SLOG_CWORD(EZTHUMB_MOD_CORE, SLOG_LVL_WARNING)
#include "libcsoup_debug.h"


static int ezopt_profile_append(EZOPT *ezopt, char *ps);
static char *ezopt_profile_sprint(EZPROF *node, char *buf, int blen);
static EZPROF *ezopt_profile_new(EZOPT *opt, int flag, int wei);
static int ezopt_profile_free(EZPROF *node);
static EZPROF *ezopt_profile_insert(EZPROF *root, EZPROF *leaf);


/****************************************************************************
 * Profile Functions
 ****************************************************************************/

int ezopt_profile_setup(EZOPT *opt, char *s)
{
	char	*tmp, *plist[64];	/* hope that's big enough */
	int	i, len;

	/* duplicate the input profile string */
	if ((tmp = csc_strcpy_alloc(s, 0)) == NULL) {
		return -1;
	}
	
	/* Reset the profile control block pool */
	memset(opt->pro_pool, 0, sizeof(EZPROF) * EZ_PROF_MAX_ENTRY);
	opt->pro_grid = opt->pro_size = NULL;

	len = csc_ziptoken(tmp, plist, sizeof(plist)/sizeof(char*), ":");
	for (i = 0; i < len; i++) {
		ezopt_profile_append(opt, plist[i]);
	}
	/* 20130809 make a copy of pro_size as an extension of grip profile */
	opt->pro_gext = opt->pro_size;
	
	smm_free(tmp);
	return 0;
}

/* for debug purpose only */
int ezopt_profile_dump(EZOPT *opt, char *pmt_grid, char *pmt_size)
{
	EZPROF	*node;
	char	tmp[64];

	CDB_SHOW(("%s", pmt_grid));		/* "Grid: " */
	for (node = opt->pro_grid; node != NULL; node = node->next) {
		CDB_SHOW(("%s ",ezopt_profile_sprint(node, tmp, sizeof(tmp))));
	}
	CDB_SHOW(("\n"));

	CDB_SHOW(("%s", pmt_size));		/* "Size: " */
	for (node = opt->pro_size; node != NULL; node = node->next) {
		CDB_SHOW(("%s ",ezopt_profile_sprint(node, tmp, sizeof(tmp))));
	}
	CDB_SHOW(("\n"));
	return 0;
}

int ezopt_profile_export(EZOPT *ezopt, char *buf, int blen)
{
	EZPROF	*p;
	char	tmp[80];
	int	idx, len;

	idx = 0;
	for (p = ezopt->pro_grid; p; p = p->next) {
		ezopt_profile_sprint(p, tmp, sizeof(tmp));
		strcat(tmp, ":");

		len = strlen(tmp);
		if (buf && ((idx + len) < blen)) {
			strcpy(buf + idx, tmp);
		}
		idx += len;
	}
	for (p = ezopt->pro_size; p; p = p->next) {
		ezopt_profile_sprint(p, tmp, sizeof(tmp));
		strcat(tmp, ":");

		len = strlen(tmp);
		if (buf && ((idx + len) < blen)) {
			strcpy(buf + idx, tmp);
		}
		idx += len;
	}
	/* remove the last delimiter */
	if (idx) {
		idx--;
		if (buf) {
			buf[idx] = 0;
		}
	}
	CDB_DEBUG(("ezopt_profile_export: %s\n", buf));
	return idx;
}

char *ezopt_profile_export_alloc(EZOPT *ezopt)
{
	char	*s;
	int	len;

	len = ezopt_profile_export(ezopt, NULL, 0) + 4;
	if ((s = smm_alloc(len)) != NULL) {
		ezopt_profile_export(ezopt, s, len);
	}
	return s;
}

int ezopt_profile_disable(EZOPT *ezopt, int prof)
{
	if (prof == EZ_PROF_LENGTH) {
		ezopt->pro_grid = NULL;
	} else if (prof == EZ_PROF_WIDTH) {
		ezopt->pro_size = NULL;
	} else if (prof == EZ_PROF_ALL) {
		ezopt->pro_grid = NULL;
		ezopt->pro_size = NULL;
	}
	return 0;
}

int ezopt_profile_sampling(EZOPT *ezopt, int vidsec, int *col, int *row)
{
	EZPROF	*node;
	int	snap;

	if (ezopt->pro_grid == NULL) {
		return -1;	/* profile disabled */
	}

	for (node = ezopt->pro_grid; node->next; node = node->next) {
		if (vidsec <= node->weight) {
			break;
		}
	}

	switch (node->flag) {
	case 's':
	case 'S':
	case 'm':
	case 'M':
		if (col) {
			*col = node->x;
		}
		if (row) {
			*row = node->y;
		}
		CDB_DEBUG(("ezopt_profile_sampling: %d x %d\n", 
					node->x, node->y));
		return 0;	/* returned the matrix */

	case 'l':
	case 'L':
		snap = (int)(log(vidsec / 60 + node->x) / log(node->lbase)) 
			- node->y;
		CDB_DEBUG(("ezopt_profile_sampling: %d+\n", snap));
		return snap;	/* need more info to decide the matrix */
	}
	return -2;	/* no effective profile found */
}

int ezopt_profile_sampled(EZOPT *ezopt, int vw, int bs, int *col, int *row)
{
	EZPROF	*node;

	/* 20130809 Using pro_gext instead of pro_size because it won't 
	 * be reset by -s and -w option */
	if (ezopt->pro_gext == NULL) {
		return bs;	/* profile disabled so ignore it */
	}

	for (node = ezopt->pro_gext; node->next; node = node->next) {
		if (vw <= node->weight) {
			break;
		}
	}

	switch (node->flag) {
	case 'f':
	case 'F':
	case 'r':
	case 'R':
		if (col) {
			*col = node->x;
		}
		bs = (bs + node->x - 1) / node->x * node->x;
		if (row) {
			*row = bs / node->x;
		}
		break;
	}
	CDB_DEBUG(("ezopt_profile_sampled: %d\n", bs));
	return bs;
}


int ezopt_profile_zooming(EZOPT *ezopt, int vw, int *wid, int *hei, int *ra)
{
	EZPROF	*node;
	float	ratio;
	int	neari;

	if (ezopt->pro_size == NULL) {
		return 0;	/* profile disabled so ignore it */
	}

	for (node = ezopt->pro_size; node->next; node = node->next) {
		if (vw <= node->weight) {
			break;
		}
	}

	switch (node->flag) {
	case 'w':
	case 'W':
		if (ra) {
			*ra = node->x;
		}
		break;
	
	case 't':
	case 'T':
		if (wid) {
			*wid = node->x;
		}
		if (hei) {
			*hei = node->y;
		}
		break;

	case 'f':
	case 'F':
		return node->y;	/* return the canvas size if required */

	case 'r':
	case 'R':
		/* find the zoom ratio */
		if (vw > node->y) {
			ratio = (float) vw / node->y;	/* zoom out */
		} else {
			ratio = (float) node->y / vw;	/* zoom in */
		}
		/* quantized the zoom ratio to base-2 exponential 1,2,4,8.. */
		neari = 1 << (int)(log(ratio) / log(2) + 0.5);
		CDB_DEBUG(("ezopt_profile_zooming: ratio=%f quant=%d\n", 
					ratio, neari));
		/* checking the error between the reference width and the 
		 * zoomed width. The error should be less than 20% */
		ratio = abs(neari * node->y - vw) / (float) node->y;
		if (ratio > 0.2) {
			neari = node->y;	/* error over 20% */
		} else if (vw > node->y) {
			neari = vw / neari;
		} else {
			neari = vw * neari;
		}
		if (wid) {
			*wid = neari / 2 * 2;	/* final quantizing width */
		}
		break;
	}
	return 0;
}


/* available profile field example:
 * 12M4x6, 720s4x6, 720S4
 * 160w200%, 320w100%, 320w160x120, 320w160 */
static int ezopt_profile_append(EZOPT *ezopt, char *ps)
{
	EZPROF	*node;
	char	pbuf[256], *argv[8];
	int	val;

	csc_strlcpy(pbuf, ps, sizeof(pbuf));

	val = (int) strtol(pbuf, &ps, 10);
	if (*ps == 0) {	/* pointed to the EOL; no flag error */
		return -1;
	}

	csc_fixtoken(ps + 1, argv, sizeof(argv)/sizeof(char*), "xX");
	node = ezopt_profile_new(ezopt, *ps, val);

	switch (node->flag) {
	case 'm':
	case 'M':
		node->weight *= 60;	/* turn to seconds */
		node->x = (int) strtol(argv[0], NULL, 10);
		node->y = argv[1] ? (int) strtol(argv[1], NULL, 10) : 0;
		ezopt->pro_grid = ezopt_profile_insert(ezopt->pro_grid, node);
		break;

	case 's':
	case 'S':
		node->x = (int) strtol(argv[0], NULL, 10);
		node->y = argv[1] ? (int) strtol(argv[1], NULL, 10) : 0;
		ezopt->pro_grid = ezopt_profile_insert(ezopt->pro_grid, node);
		break;

	case 'l':
	case 'L':
		if ((argv[1] == NULL) || (argv[2] == NULL)) {
			ezopt_profile_free(node);
			return -2;
		}
		node->weight *= 60;	/* turn to seconds */
		node->x = (int) strtol(argv[0], NULL, 10);
		node->y = (int) strtol(argv[1], NULL, 10);
		node->lbase = strtod(argv[2], NULL);
		ezopt->pro_grid = ezopt_profile_insert(ezopt->pro_grid, node);
		break;

	case 'w':
	case 'W':
		node->x = (int) strtol(argv[0], NULL, 10);
		ezopt->pro_size = ezopt_profile_insert(ezopt->pro_size, node);
		break;

	case 't':
	case 'T':
		node->x = (int) strtol(argv[0], NULL, 10);
		node->y = argv[1] ? (int) strtol(argv[1], NULL, 10) : 0;
		ezopt->pro_size = ezopt_profile_insert(ezopt->pro_size, node);
		break;
	
	case 'f':
	case 'F':
	case 'r':
	case 'R':
		node->x = (int) strtol(argv[0], NULL, 10);
		if (argv[1] == NULL) {
			ezopt_profile_free(node);
			return -2;
		}
		node->y = (int) strtol(argv[1], NULL, 10);
		if (node->y <= 0) {
			ezopt_profile_free(node);
			return -2;
		}
		ezopt->pro_size = ezopt_profile_insert(ezopt->pro_size, node);
		break;

	default:
		ezopt_profile_free(node);
		return -2;
	}
	return 0;
}

static char *ezopt_profile_sprint(EZPROF *node, char *buf, int blen)
{
	char	tmp[64];

	if (blen < 32) {	/* FIXME: very rough estimation */
		return NULL;
	}

	switch (node->flag) {
	case 'm':
	case 'M':
		sprintf(buf, "%dM%d", node->weight / 60, node->x);
		if (node->y > 0) {
			sprintf(tmp, "x%d", node->y);
			strcat(buf, tmp);
		}
		break;

	case 's':
	case 'S':
		sprintf(buf, "%dS%d", node->weight, node->x);
		if (node->y > 0) {
			sprintf(tmp, "x%d", node->y);
			strcat(buf, tmp);
		}
		break;

	case 'l':
	case 'L':
		sprintf(buf, "%dL%dx%dx%f", node->weight / 60, 
				node->x, node->y, (float) node->lbase);
		break;

	case 'w':
	case 'W':
		sprintf(buf, "%dW%d%%", node->weight, node->x);
		break;

	case 't':
	case 'T':
		sprintf(buf, "%dT%d", node->weight, node->x);
		if (node->y > 0) {
			sprintf(tmp, "x%d", node->y);
			strcat(buf, tmp);
		}
		break;

	case 'f':
	case 'F':
		sprintf(buf, "%dF%dx%d", node->weight, node->x, node->y);
		break;

	case 'r':
	case 'R':
		sprintf(buf, "%dR%dx%d", node->weight, node->x, node->y);
		break;

	default:
		return NULL;
	}
	return buf;
}

static EZPROF *ezopt_profile_new(EZOPT *opt, int flag, int wei)
{
	int	i;

	for (i = 0; i < EZ_PROF_MAX_ENTRY; i++) {
		if (opt->pro_pool[i].flag == 0) {
			break;
		}
	}
	if (i == EZ_PROF_MAX_ENTRY) {
		return NULL;
	}

	memset(&opt->pro_pool[i], 0, sizeof(EZPROF));
	opt->pro_pool[i].next = NULL;
	opt->pro_pool[i].flag = flag;
	opt->pro_pool[i].weight = wei;

	return &opt->pro_pool[i];
}

/* note that it's NOT a proper free since the 'next' point was not processed
 * at all. It only serves as a quick release */
static int ezopt_profile_free(EZPROF *node)
{
	memset(node, 0, sizeof(EZPROF));
	return 0;
}

static EZPROF *ezopt_profile_insert(EZPROF *root, EZPROF *leaf)
{
	EZPROF	*prev, *now;

	if (leaf == NULL) {
		return root;	/* do nothing */
	}

	CDB_DEBUG(("ezopt_profile_insert: %d\n", leaf->weight));
	if (root == NULL) {
		return leaf;
	}
	if (root->weight > leaf->weight) {
		leaf->next = root;
		return leaf;
	}
	prev = root;
	for (now = root->next; now != NULL; prev = now, now = now->next) {
		if (now->weight > leaf->weight) {
			break;			
		}
	}
	if (now == NULL) {
		prev->next = leaf;
	} else {
		leaf->next = prev->next;
		prev->next = leaf;
	}
	return root;
}

/****************************************************************************
 * Data transform Functions
 ****************************************************************************/

char *meta_filesize(int64_t size, char *buffer)
{
	static	char	tmp[32];

	if (buffer == NULL) {
		buffer = tmp;
	}
	if (size < (int64_t)(1ULL << 20)) {
		sprintf(buffer, "%.2f KB", size / 1024.0);
	} else if (size < (int64_t)(1ULL << 30)) {
		sprintf(buffer, "%.2f MB", size / 1048576.0);
	} else {
		sprintf(buffer, "%.2f GB", size / 1073741824.0); 
	}
	return buffer;
}

char *meta_timestamp(EZTIME ms, int enms, char *buffer)
{
	static	char	tmp[32];
	int	hour, min, sec, msec;

	if (buffer == NULL) {
		buffer = tmp;
	}

	hour = ms / 3600000;
	ms   = ms % 3600000;
	min  = ms / 60000;
	ms   = ms % 60000;
	sec  = ms / 1000;
	msec = ms % 1000;
	if (enms) {
		sprintf(buffer, "%d:%02d:%02d,%03d", hour, min, sec, msec);
	} else {
		sprintf(buffer, "%d:%02d:%02d", hour, min, sec);
	}
	return buffer;
}

/* input: jpg@85, gif@1000, png */
int meta_image_format(char *input, char *fmt, int flen)
{
	char	*p, arg[128];
	int	quality = 0;

	csc_strlcpy(arg, input, sizeof(arg));
	if ((p = strchr(arg, '@')) != NULL) {
		*p++ = 0;
		quality = strtol(p, NULL, 0);
	}
	csc_strlcpy(fmt, arg, flen);

	/* foolproof of the quality parameter */
	if (!strcmp(fmt, "jpg") || !strcmp(fmt, "jpeg")) {
		if ((quality < 5) || (quality > 100)) {
			quality = 85;	/* as default */
		}
	} else if (!strcmp(fmt, "png")) {
		quality = 0;
	} else if (!strcmp(fmt, "gif")) {
		if (quality && (quality < 15)) {
			quality = 1000;	/* 1 second as default */
		}
	} else {
		strcpy(fmt, "jpg");	/* as default */
		quality = 85;
	}
	return quality;
}

int meta_make_color(char *s, EZBYTE *color)
{
	unsigned	rc;

	rc = (unsigned) strtol(s, NULL, 16);
	if (color) {
		color[0] = (unsigned char)((rc >> 16) & 0xff);
		color[1] = (unsigned char)((rc >> 8) & 0xff);
		color[2] = (unsigned char)(rc & 0xff);
		color[3] = (unsigned char)((rc >> 24) & 0xff);
	}
	return (int)rc;
}

int meta_export_color(EZBYTE *color, char *buf, int blen)
{
	char	tmp[16];
	int	rc, len;

	rc = (color[3] << 24) | (color[0] << 16) | (color[1] << 8) | color[2];
	len = sprintf(tmp, "%x", (unsigned) rc);
	if (buf && (blen > len)) {
		strcpy(buf, tmp);
	}
	return len;
}

char *meta_make_fontdir(char *s)
{
#ifdef	HAVE_GD_USE_FONTCONFIG
	char	*p;

	/* review whether the fontconfig pattern like "times:bold:italic"
	 * was specified. The fontconfig pattern could be used directly.
	 * Otherwise a full path like "/usr/local/share/ttf/Times.ttf" */
	if (csc_cmp_file_extname(s,"ttf") && csc_cmp_file_extname(s,"ttc")) {
		/* the fontconfig pattern like "times:bold:italic" shouldn't
		 * be messed with network path like "smb://sdfaadf/abc */
		if (((p = strchr(s, ':')) != NULL) && isalnum(p[1])) {
			gdFTUseFontConfig(1);
			return csc_strcpy_alloc(s, 0);
		}
	}
#endif	/* HAVE_GD_USE_FONTCONFIG */
	return smm_fontpath(s, NULL);
}


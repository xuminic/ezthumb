
/*  eznotify.c - the notification handling functions

    Copyright (C) 2011  "Andy Xuming" <xuming@users.sourceforge.net>

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

static int ezopt_thumb_name(EZOPT *ezopt, char *buf, char *fname, int idx)
{
	char	tmp[128], *inbuf = NULL;
	int	i, rc = 0;

	/* special case for testing purpose
	 * If the output path has the same suffix to the specified suffix,
	 * it will NOT be treated as a path but the output file.
	 * For example, if the 'img_format' was defined as "jpg", and the
	 * 'pathout' is something like "abc.jpg", the 'pathout' actually
	 * is the output file. But if 'pathout' is "abc.jpg/", then it's
	 * still a path */
	if (!csoup_cmp_file_extname(ezopt->pathout, ezopt->img_format)) {
		if (buf) {
			strcpy(buf, ezopt->pathout);
		}
		return EZ_THUMB_VACANT;	/* debug mode always vacant */
	}

	if (buf == NULL) {
		buf = inbuf = malloc(strlen(fname) + 128 + 32);
		if (buf == NULL) {
			return rc;
		}
	}

	if (idx < 0) {
		sprintf(tmp, "%s.", ezopt->suffix);
	} else {
		sprintf(tmp, "%03d.", idx);
	}
	strcat(tmp, ezopt->img_format);
	meta_name_suffix(ezopt->pathout, fname, buf, tmp);

	for (i = 1; i < 256; i++) {
		if (smm_fstat(buf) != SMM_ERR_NONE) {
			if (i == 1) {
				rc = EZ_THUMB_VACANT;	/* file not existed */
			} else {
				rc = EZ_THUMB_COPIABLE;	/* copying file  */
			}
			break;	/* file not existed */
		} else if (ezopt->flags & EZOP_THUMB_OVERRIDE) {
			rc = EZ_THUMB_OVERRIDE;	/* override it */
			break;
		} else if ((ezopt->flags & EZOP_THUMB_COPY) == 0) {
			rc = EZ_THUMB_SKIP;	/* skip the existed files */
			break;
		}
		
		if (idx < 0) {
			sprintf(tmp, "%s.%d.", ezopt->suffix, i);
		} else {
			sprintf(tmp, "%03d.%d.", idx, i);
		}
		strcat(tmp, ezopt->img_format);
		meta_name_suffix(ezopt->pathout, fname, buf, tmp);
	}
	if (i == 256) {
		rc = EZ_THUMB_OVERCOPY;	/* override the last one */
	}
	if (inbuf) {
		free(inbuf);
	}
	//slogz("ezopt_thumb_name: %d\n", rc);
	return rc;
}


/****************************************************************************
 * Profile Functions
 ****************************************************************************/

int ezopt_profile_setup(EZOPT *opt, char *s)
{
	char	*tmp, *plist[64];	/* hope that's big enough */
	int	i, len;

	/* duplicate the input profile string */
	if ((tmp = strcpy_alloc(s, 0)) == NULL) {
		return -1;
	}
	
	/* Reset the profile control block pool */
	memset(opt->pro_pool, 0, sizeof(EZPROF) * EZ_PROF_MAX_ENTRY);
	opt->pro_grid = opt->pro_size = NULL;

	len = ziptoken(tmp, plist, sizeof(plist)/sizeof(char*), ":");
	for (i = 0; i < len; i++) {
		ezopt_profile_append(opt, plist[i]);
	}
	
	free(tmp);
	return 0;
}

/* for debug purpose only */
int ezopt_profile_dump(EZOPT *opt, char *pmt_grid, char *pmt_size)
{
	EZPROF	*node;
	char	tmp[64];

	slogz("%s", pmt_grid);		/* "Grid: " */
	for (node = opt->pro_grid; node != NULL; node = node->next) {
		slogz("%s ", ezopt_profile_sprint(node, tmp, sizeof(tmp)));
	}
	slogz("\n");

	slogz("%s", pmt_size);		/* "Size: " */
	for (node = opt->pro_size; node != NULL; node = node->next) {
		slogz("%s ", ezopt_profile_sprint(node, tmp, sizeof(tmp)));
	}
	slogz("\n");
	return 0;
}

char *ezopt_profile_export_alloc(EZOPT *ezopt)
{
	EZPROF	*p;
	char	*buf, tmp[64];
	int	n = 0;

	for (p = ezopt->pro_grid; p; p = p->next, n++);
	for (p = ezopt->pro_size; p; p = p->next, n++);
	if ((buf = calloc(n, 64)) == NULL) {
		return NULL;
	}
	buf[0] = 0;

	for (p = ezopt->pro_grid; p; p = p->next) {
		ezopt_profile_sprint(p, tmp, sizeof(tmp));
		if (buf[0] == 0) {
			strcpy(buf, tmp);
		} else {
			strcat(buf, ":");
			strcat(buf, tmp);
		}
	}
	for (p = ezopt->pro_size; p; p = p->next) {
		ezopt_profile_sprint(p, tmp, sizeof(tmp));
		if (buf[0] == 0) {
			strcpy(buf, tmp);
		} else {
			strcat(buf, ":");
			strcat(buf, tmp);
		}
	}
	//slogz("ezopt_profile_export: %s\n", buf);
	return buf;
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
		//slogz("ezopt_profile_sampling: %d x %d\n", node->x, node->y);
		return 0;	/* returned the matrix */

	case 'l':
	case 'L':
		snap = (int)(log(vidsec / 60 + node->x) / log(node->lbase)) 
			- node->y;
		//slogz("ezopt_profile_sampling: %d+\n", snap);
		return snap;	/* need more info to decide the matrix */
	}
	return -2;	/* no effective profile found */
}

int ezopt_profile_sampled(EZOPT *ezopt, int vw, int bs, int *col, int *row)
{
	EZPROF	*node;

	if (ezopt->pro_size == NULL) {
		return bs;	/* profile disabled so ignore it */
	}

	for (node = ezopt->pro_size; node->next; node = node->next) {
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
	//slogz("ezopt_profile_sampled: %d\n", bs);
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
		//slogz("ez.._zooming: ratio=%f quant=%d\n", ratio, neari);
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

	strlcopy(pbuf, ps, sizeof(pbuf));

	val = (int) strtol(pbuf, &ps, 10);
	if (*ps == 0) {	/* pointed to the EOL; no flag error */
		return -1;
	}

	fixtoken(ps + 1, argv, sizeof(argv)/sizeof(char*), "xX");
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
		node->lbase = (float) strtof(argv[2], NULL);
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
				node->x, node->y, node->lbase);
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

	//slogz("ezopt_profile_insert: %d\n", leaf->weight);
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
 * Utility Functions
 ****************************************************************************/


char *meta_filesize(int64_t size, char *buffer)
{
	static	char	tmp[32];

	if (buffer == NULL) {
		buffer = tmp;
	}
	if (size < (1ULL << 20)) {
		sprintf(buffer, "%lu KB", (unsigned long)(size >> 10));
	} else if (size < (1ULL << 30)) {
		sprintf(buffer, "%lu.%lu MB", (unsigned long)(size >> 20), 
				(unsigned long)(((size % (1 << 20)) >> 10) / 100));
	} else {
		sprintf(buffer, "%lu.%03lu GB", (unsigned long)(size >> 30), 
				(unsigned long)(((size % (1 << 30)) >> 20) / 100));
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


// FIXME: UTF-8 and widechar?
// Haven't decide how to improve these two functions
#ifdef  CFG_WIN32_API
#define DIRSEP  '\\'
#else
#define DIRSEP  '/'
#endif

char *meta_basename(char *fname, char *buffer)
{
	static	char	tmp[1024];
	char	*p;

	if (buffer == NULL) {
		buffer = tmp;
	}

	if ((p = strrchr(fname, DIRSEP)) == NULL) {
		strcpy(buffer, fname);
	} else {
		strcpy(buffer, p + 1);
	}
	return buffer;
}

char *meta_name_suffix(char *path, char *fname, char *buf, char *sfx)
{
	static	char	tmp[1024];
	char	*p, sep[4];

	if (buf == NULL) {
		buf = tmp;
	}

	if (!path || !*path) {
		strcpy(buf, fname);
	} else {
		strcpy(buf, path);
		if (buf[strlen(buf)-1] != DIRSEP) {
			sep[0] = DIRSEP;
			sep[1] = 0;
			strcat(buf, sep);
		}
		if ((p = strrchr(fname, DIRSEP)) == NULL) {
			strcat(buf, fname);
		} else {
			strcat(buf, p+1);
		}
	}
	if ((p = strrchr(buf, '.')) != NULL) {
		*p = 0;
	}
	strcat(buf, sfx);
	return buf;
}

int64_t meta_bestfit(int64_t ref, int64_t v1, int64_t v2)
{
	int64_t	c1, c2;

	if (v1 < 0) {
		return v2;
	}
	if (v2 < 0) {
		return v1;
	}
	if (ref < 0) {
		return v1 > v2 ? v1 : v2;
	}
	
	c1 = (ref > v1) ? ref - v1 : v1 - ref;
	c2 = (ref > v2) ? ref - v2 : v2 - ref;
	if (c1 < c2) {
		return v1;
	}
	return v2;
}

/* input: jpg@85, gif@1000, png */
int meta_image_format(char *input, char *fmt, int flen)
{
	char	*p, arg[128];
	int	quality = 0;

	strlcopy(arg, input, sizeof(arg));
	if ((p = strchr(arg, '@')) != NULL) {
		*p++ = 0;
		quality = strtol(p, NULL, 0);
	}
	strlcopy(fmt, arg, flen);

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


EZFLT *ezflt_create(char *s)
{
	EZFLT	*flt;
	int	len, fno;
	char	*tmp;

	len = strlen(s);
	fno = len / 2;
	len += fno * sizeof(char*) + sizeof(EZFLT) + 16;
	if ((flt = malloc(len)) == NULL) {
		return NULL;
	}

	memset(flt, 0, len);
	tmp = (char*) &flt->filter[fno];
	strcpy(tmp, s);
	len = ziptoken(tmp, flt->filter, fno, ",;:");
	flt->filter[len] = NULL;
	return flt;
}

int ezflt_match(EZFLT *flt, char *fname)
{
	if (flt == NULL) {
		return 1;	/* no filter means total matched */
	}
	if (!flt->filter || !*flt->filter) {
		return 1;
	}
	if (!csoup_cmp_file_extlist(fname, flt->filter)) {
		return 1;
	}
	return 0;	/* not matched */
}

/*  csc_config.c - simple interface of configure file access
 
    This is a simplified configure file interface so it only supports
    a flat level configure file, which means the subkey structure is
    not supported. For example:

    [main]
    key A=value a
    key B=value b
    [subkey1]
    key A=value a
    key B=value b
    [subkey2]
    key A=value a
    key B=value b
    ...

    or

    key A=value a
    key B=value b
    key C=value c
    ...

    are all okey. But it doesn't support

    [main]
    key A=value a
    key B=value b
    [main/subkey1]
    key A=value a
    key B=value b
    [main/subkey1/subkey2]
    key A=value a
    key B=value b
    ...

    Anything starts with '#' are all treated as comments.

    Copyright (C) 2013  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of CSOUP, Chicken Soup library

    CSOUP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CSOUP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "libcsoup.h"

#define CFGF_TYPE_COMM	0	/* comment */
#define CFGF_TYPE_ROOT	1	/* root control block (only one) */
#define CFGF_TYPE_MAIN	2	/* main key control block (under root) */
#define CFGF_TYPE_KEY	3	/* common key */
#define CFGF_TYPE_PART	4	/* partial key */
#define CFGF_TYPE_MASK	0xf
#define CFGF_RDONLY	0x10

#define CFGF_MAGIC	(('K' << 24) | ('Y' << 16) | ('C' << 8) | 'B')

#define	CFGF_BLOCK_WID		48
#define CFGF_BLOCK_MAGIC	"BINARY"


typedef	struct	_KEYCB	{
	/* CSCLNK compatible head */
	/* If the control block is the root key, 
	 * the 'next' will point to the recent accessed main key.
	 * the 'prev' will point to the recent accessed normal key. */
	void	*next;
	void	*prev;

	int	majesty;	/* "KYCB" */
	int	size;
	/* key and value are matching pairs. 
	 * If value is NULL, the key points to a main key and anchor points
	 * to the sub-key chain */
	char	*key;
	char	*value;
	char	*comment;
	char	store[4];	/* storing the boundary character */

	int	flags;
	CSCLNK	*anchor;

	/* if it's a normal key, the update counts the total modification.
	 * if it's a main key, the update counts the modified keys under 
	 * the main key. 
	 * if it's a root key, the update counts every modified keys */
	int	update;

	char	pool[1];
} KEYCB;


static KEYCB *csc_cfg_kcb_alloc(int psize);
static KEYCB *csc_cfg_kcb_create(char *skey, char *value, char *comm);
static int csc_cfg_kcb_fillup(KEYCB *kp);
static int csc_cfg_update(void *cfg);
static KEYCB *csc_cfg_find_key(void *cfg, char *key, int type);
static int csc_cfg_recent_setup(void *cfg, KEYCB *mkey, KEYCB *kcb);
static KEYCB *csc_cfg_recent_update(void *cfg, int type);
static FILE *csc_cfg_open_file(char *path, char *fname, int mode);
static int csc_cfg_read_next_line(FILE *fp, char *buf);
static int csc_cfg_write_next_line(FILE *fp, KEYCB *kp);
static int csc_cfg_strcmp(char *sour, char *dest);
static int csc_cfg_binary_to_hex(char *src, int slen, char *buf, int blen);
static int csc_cfg_hex_to_binary(char *src, char *buf, int blen);

static inline KEYCB *CFGF_GETOBJ(void *objc)
{
	KEYCB   *kcb = objc;

	if (kcb && (kcb->majesty != CFGF_MAGIC)) {
		return NULL;
	}
	return kcb;
}

static inline int CFGF_TYPE_SET(void *objc, int type)
{
	KEYCB	*kcb = objc;

	if (kcb->majesty != CFGF_MAGIC) {
		return -1;
	}
	kcb->flags = (kcb->flags & ~CFGF_TYPE_MASK) | type;
	return kcb->flags;
}

static inline int CFGF_TYPE_GET(void *objc)
{
	KEYCB	*kcb = objc;

	if (kcb->majesty != CFGF_MAGIC) {
		return -1;
	}
	return kcb->flags & CFGF_TYPE_MASK;
}

void *csc_cfg_open(char *path, char *filename, int mode)
{
	KEYCB	*root, *ckey, *kp;
	FILE	*fp;
	int	len;

	/* try to open the configure file. If the file doesn't exist 
	 * while it's the read/write mode, then create it */
	if ((fp = csc_cfg_open_file(path, filename, mode)) == NULL) {
		return NULL;
	}

	/* create the root control block */
	root = csc_cfg_kcb_alloc(strlen(path) + strlen(filename));
	if (root == NULL) {
		fclose(fp);
		return NULL;
	}

	/* initialize the root control block */
	root->flags = (mode == SMM_CFGMODE_RDONLY) ? CFGF_RDONLY : 0;
	root->flags = CFGF_TYPE_SET(root, CFGF_TYPE_ROOT);
	root->key = root->pool;
	strcpy(root->key, path);
	root->value = root->pool + strlen(root->key) + 1;
	strcpy(root->value, filename);

	ckey = root;
	while (!feof(fp)) {
		if ((len = csc_cfg_read_next_line(fp, NULL)) <= 0) {
			break;
		}
		if ((kp = csc_cfg_kcb_alloc(len)) == NULL) {
			break;
		}
		csc_cfg_read_next_line(fp, kp->pool);

		csc_cfg_kcb_fillup(kp);
	
		if (CFGF_TYPE_GET(kp) == CFGF_TYPE_MAIN) {
			root->anchor = csc_cdl_insert_tail(root->anchor, 
					(CSCLNK*)kp);
			ckey = kp;
		} else {
			ckey->anchor = csc_cdl_insert_tail(ckey->anchor,
					(CSCLNK*)kp);
		}
	}
	fclose(fp);
	return root;
}

int csc_cfg_abort(void *cfg)
{
	KEYCB	*root, *ckey;
	CSCLNK	*p;

	if ((root = CFGF_GETOBJ(cfg)) == NULL) {
		return SMM_ERR_OBJECT;
	}
	for (p = root->anchor; p != NULL; p = csc_cdl_next(root->anchor, p)) {
		if ((ckey = CFGF_GETOBJ(p)) == NULL) {
			break;
		}
		if (ckey->anchor) {
			csc_cdl_destroy(&ckey->anchor);
		}
	}
	csc_cdl_destroy(&root->anchor);
	return SMM_ERR_NONE;
}

int csc_cfg_save(void *cfg)
{
	KEYCB	*mkey, *root;
	CSCLNK	*mp, *sp;
	FILE	*fp;

	if ((root = CFGF_GETOBJ(cfg)) == NULL) {
		return SMM_ERR_OBJECT;
	}
	if ((root->flags & CFGF_TYPE_MASK) != CFGF_TYPE_ROOT) {
		return SMM_ERR_ACCESS;
	}
	if (root->flags & CFGF_RDONLY) {
		return SMM_ERR_ACCESS;
	}

	fp = csc_cfg_open_file(root->key, root->value, SMM_CFGMODE_RWC);
	if (fp == NULL) {
		return SMM_ERR_ACCESS;
	}

	for (mp = root->anchor; mp; mp = csc_cdl_next(root->anchor, mp)) {
		if ((mkey = CFGF_GETOBJ(mp)) == NULL) {
			break;
		}
		csc_cfg_write_next_line(fp, mkey);

		if (CFGF_TYPE_GET(mkey) == CFGF_TYPE_MAIN) {
			for (sp = mkey->anchor; sp != NULL; 
					sp = csc_cdl_next(mkey->anchor, sp)) {
				csc_cfg_write_next_line(fp, CFGF_GETOBJ(sp));
			}
		}
	}
	fclose(fp);
	root->update = 0;	/* reset the update counter */
	return SMM_ERR_NONE;
}

int csc_cfg_saveas(void *cfg, char *path, char *filename)
{
	KEYCB	*root, *newc;
	CSCLNK	*tmp;
	int	rc;

	if ((root = CFGF_GETOBJ(cfg)) == NULL) {
		return SMM_ERR_OBJECT;
	}
	if ((newc = csc_cfg_open(path, filename, 0)) == NULL) {
		return SMM_ERR_OPEN;
	}
	tmp = newc->anchor;
	newc->anchor = root->anchor;
	newc->update = root->update + 1;
	root->update = 0;

	if ((rc = csc_cfg_save(newc)) != SMM_ERR_NONE) {
		return rc;
	}
	newc->anchor = tmp;
	return csc_cfg_close(newc);
}

int csc_cfg_flush(void *cfg)
{
	KEYCB	*root;

	if ((root = CFGF_GETOBJ(cfg)) == NULL) {
		return SMM_ERR_OBJECT;
	}
	if (root->update) {
		return csc_cfg_save(root);
	}
	return SMM_ERR_NONE;
}

int csc_cfg_close(void *cfg)
{
	csc_cfg_flush(cfg);
	csc_cfg_abort(cfg);
	return SMM_ERR_NONE;
}


char *csc_cfg_read(void *cfg, char *mkey, char *skey)
{
	KEYCB	*mcb, *scb;

	if (CFGF_GETOBJ(cfg) == NULL) {
		return NULL;
	}
	if ((mcb = csc_cfg_find_key(cfg, mkey, CFGF_TYPE_MAIN)) == NULL) {
		return NULL;
	}
	if ((scb = csc_cfg_find_key(mcb, skey, CFGF_TYPE_KEY)) == NULL) {
		return NULL;
	}
	csc_cfg_recent_setup(cfg, mcb, scb);
	return scb->value;
}

char *csc_cfg_read_first(void *cfg, char *mkey, char **key)
{
	KEYCB	*root, *scb;

	if (csc_cfg_read(cfg, mkey, NULL) == NULL) {
		return NULL;
	}
	if ((root = CFGF_GETOBJ(cfg)) == NULL) {
		return NULL;
	}
	scb = root->prev;
	if (key) {
		*key = scb->key;
	}
	return scb->value;
}

char *csc_cfg_read_next(void *cfg, char **key)
{
	KEYCB	*scb;

	if (CFGF_GETOBJ(cfg) == NULL) {
		return NULL;
	}
	if ((scb = csc_cfg_recent_update(cfg, CFGF_TYPE_KEY)) == NULL) {
		return NULL;
	}
	if (key) {
		*key = scb->key;
	}
	return scb->value;
}

char *csc_cfg_copy(void *cfg, char *mkey, char *skey, int extra)
{
	char	*value;

	if ((value = csc_cfg_read(cfg, mkey, skey)) == NULL) {
		return NULL;
	}
	return csc_strcpy_alloc(value, extra);
}

int csc_cfg_write(void *cfg, char *mkey, char *skey, char *value)
{
	KEYCB	*mcb, *scb, *ncb, *root;
	int	olen, nlen;

	if (value == NULL) {
		return SMM_ERR_NULL;
	}
	if ((root = CFGF_GETOBJ(cfg)) == NULL) {
		return SMM_ERR_OBJECT;
	}
	if ((mcb = csc_cfg_find_key(cfg, mkey, CFGF_TYPE_MAIN)) == NULL) {
		/* if the main key doesn't exist, create a new key and
		 * insert to the tail */
		if (strchr(mkey, '[') == NULL) {
			return SMM_ERR_NULL;
		}
		if ((mcb = csc_cfg_kcb_create(mkey, NULL, NULL)) == NULL) {
			return SMM_ERR_NULL;
		}
		root->update++;
		root->anchor = csc_cdl_insert_tail(root->anchor, 
				(CSCLNK*) mcb);
	}
	if ((scb = csc_cfg_find_key(mcb, skey, CFGF_TYPE_KEY)) == NULL) {
		/* if the key doesn't exist, it'll create a new key and
		 * insert to the tail. To tail shows a more natural way
		 * of display. But there is no white space line in the head.
		 */
		if ((ncb = csc_cfg_kcb_create(skey, value, NULL)) != NULL) {
			csc_cfg_update(ncb);
			csc_cfg_update(mcb);
			csc_cfg_update(cfg);
			mcb->anchor = csc_cdl_insert_tail(mcb->anchor,
					(CSCLNK*) ncb);
			csc_cfg_recent_setup(cfg, mcb, ncb);
		}
		return SMM_ERR_NONE;
	}

	csc_cfg_recent_setup(cfg, mcb, scb);
	olen = strlen(scb->value);
	nlen = strlen(value);
	if (!csc_cfg_strcmp(value, scb->value)) {
		/* same value so do nothing */
		return SMM_ERR_NONE;
	} else if (nlen <= olen) {
		/* If the new value is smaller than or same to the original 
		 * value, it'll replace the original value directly. */
		strcpy(scb->value, value);
		/* fill the gap */
		while (nlen < olen) {
			scb->value[nlen] = ' ';
			nlen++;
		}
	} else {
		/* If the new value is larger than the original value, it'll
		 * create a new key to replace the old key structure */
		if ((scb->store[1] == 0) || (scb->store[1] == '\n')) {
			/* preview key has no comment */
			ncb = csc_cfg_kcb_create(skey, value, NULL);
		} else {
			/* restore the comments */
			*(scb->comment) = scb->store[1]; 
			ncb = csc_cfg_kcb_create(skey, value, scb->comment);
			*(scb->comment) = 0;
		}
		if (ncb == NULL) {
			return SMM_ERR_LOWMEM;
		}
		ncb->update = scb->update;
		csc_cdl_insert_after((CSCLNK*) scb, (CSCLNK*) ncb);
		csc_cdl_free(&mcb->anchor, (CSCLNK*) scb);
		scb = ncb;
	}
	csc_cfg_update(scb);
	if (scb->update == 1) {
		csc_cfg_update(mcb);
		csc_cfg_update(cfg);
	}
	return SMM_ERR_NONE;
}

int csc_cfg_read_long(void *cfg, char *mkey, char *skey, long *val)
{
	char 	*value;

	if ((value = csc_cfg_read(cfg, mkey, skey)) == NULL) {
		return SMM_ERR_NULL;
	}
	if (val) {
		*val = strtol(value, NULL, 0);
	}
	return SMM_ERR_NONE;
}

int csc_cfg_write_long(void *cfg, char *mkey, char *skey, long val)
{
	char	buf[32];

	sprintf(buf, "%ld", val);
	return csc_cfg_write(cfg, mkey, skey, buf);
}

int csc_cfg_read_longlong(void *cfg, char *mkey, char *skey, long long *val)
{ 
	char	*value;

	if ((value = csc_cfg_read(cfg, mkey, skey)) == NULL) {
		return SMM_ERR_NULL;
	}
	if (val) {
		*val = strtoll(value, NULL, 0);
	}
	return SMM_ERR_NONE;
}

int csc_cfg_write_longlong(void *cfg, char *mkey, char *skey, long long val)
{
	char	buf[32];

	SMM_SPRINT(buf, "%lld", val);
	return csc_cfg_write(cfg, mkey, skey, buf);
}

int csc_cfg_read_bin(void *cfg, char *mkey, char *skey, char *buf, int blen)
{
	char	*src, *value;
	int	len;

	if ((value = csc_cfg_read(cfg, mkey, skey)) == NULL) {
		return SMM_ERR_NULL;
	}

	src = csc_strbody(value, NULL);	/* strip the white space */
	len = csc_cfg_hex_to_binary(src, buf, blen);
	return len;
}

void *csc_cfg_copy_bin(void *cfg, char *mkey, char *skey, int *bsize)
{
	char	*buf;
	int	len;

	if ((len = csc_cfg_read_bin(cfg, mkey, skey, NULL, 0)) <= 0) {
		return NULL;
	}
	if ((buf = smm_alloc(len)) == NULL) {
		return NULL;
	}
	if (bsize) {
		*bsize = len;
	}
	csc_cfg_read_bin(cfg, mkey, skey, buf, len);
	return buf;
}

int csc_cfg_write_bin(void *cfg, char *mkey, char *skey, void *bin, int bsize)
{
	char	*buf;

	if (!bin || !bsize) {
		return SMM_ERR_NULL;
	}
	if ((buf = smm_alloc((bsize+1)*2)) == NULL) {
		return SMM_ERR_LOWMEM;
	}
	csc_cfg_binary_to_hex(bin, bsize, buf, (bsize+1)*2);
	bsize = csc_cfg_write(cfg, mkey, skey, buf);
	smm_free(buf);
	return bsize;
}

int csc_cfg_read_block(void *cfg, char *mkey, char *buf, int blen)
{
	KEYCB	*mcb, *scb;
	char	*src, tmp[256];
	int	len, amnt;

	if ((mcb = csc_cfg_find_key(cfg, mkey, CFGF_TYPE_MAIN)) == NULL) {
		return -1;
	}
	if (strcmp(mcb->value, CFGF_BLOCK_MAGIC)) {
		return -2;
	}
	if ((scb = csc_cfg_find_key(mcb, NULL, CFGF_TYPE_PART)) == NULL) {
		return 0;	/* empty block */
	}
	csc_cfg_recent_setup(cfg, mcb, scb);

	amnt = 0;
	do {
		/* convert ASC to binary */
		src = csc_strbody(scb->key, NULL); /* strip the space */
		len = csc_cfg_hex_to_binary(src, tmp, sizeof(tmp));

		/* store to the buffer */
		if (blen && (blen < (amnt + len))) {
			break;
		}
		if (buf) {
			memcpy(buf + amnt, tmp, len);
		}
		amnt += len;
	} while ((scb = csc_cfg_recent_update(cfg, CFGF_TYPE_PART)) != NULL);
	return amnt;
}

void *csc_cfg_copy_block(void *cfg, char *mkey, int *bsize)
{
	char	*buf;
	int	len;

	if ((len = csc_cfg_read_block(cfg, mkey, NULL, 0)) <= 0) {
		return NULL;
	}
	if ((buf = smm_alloc(len)) == NULL) {
		return NULL;
	}
	csc_cfg_read_block(cfg, mkey, buf, len);
	if (bsize) {
		*bsize = len;
	}
	return buf;
}

int csc_cfg_write_block(void *cfg, char *mkey, void *bin, int bsize)
{
	KEYCB	*mcb, *scb, *root;
	char	*src, tmp[256];
	int	len, rest;

	if (!bin || !bsize) {
		return SMM_ERR_NULL;
	}
	if ((root = CFGF_GETOBJ(cfg)) == NULL) {
		return SMM_ERR_OBJECT;
	}
	if ((mcb = csc_cfg_find_key(cfg, mkey, CFGF_TYPE_MAIN)) == NULL) {
		/* if the main key doesn't exist, create a new key and
		 * insert to the tail */
		if (strchr(mkey, '[') == NULL) {
			return SMM_ERR_OBJECT;
		}
		mcb = csc_cfg_kcb_create(mkey, CFGF_BLOCK_MAGIC, NULL);
		if (mcb == NULL) {
			return SMM_ERR_LOWMEM;
		}
		root->anchor = csc_cdl_insert_tail(root->anchor, (CSCLNK*)mcb);
	} else  if (strcmp(mcb->value, CFGF_BLOCK_MAGIC)) {
		/* you can't write blocks into other types of main keys */
		return SMM_ERR_ACCESS;
	} else {
		/* if the main key does exist, destory its contents */
		csc_cdl_destroy(&mcb->anchor);
		mcb->update = 0;
	}
	root->update++;

	for (src = bin, rest = bsize; rest > 0; rest -= CFGF_BLOCK_WID) {
		/* convert the binary to hex codes */
		len = rest > CFGF_BLOCK_WID ? CFGF_BLOCK_WID : rest;
		csc_cfg_binary_to_hex(src, len, tmp, sizeof(tmp));
		src += len;

		/* insert to the tail */
		if ((scb = csc_cfg_kcb_create(tmp, NULL, NULL)) == NULL) {
			return SMM_ERR_LOWMEM;
		}
		scb->update++;
		mcb->update++;
		mcb->anchor = csc_cdl_insert_tail(mcb->anchor, (CSCLNK*)scb);
		csc_cfg_recent_setup(cfg, mcb, scb);
	}
	return bsize;
}

int csc_cfg_dump_kcb(void *cfg)
{
	KEYCB	*kp;

	if ((kp = CFGF_GETOBJ(cfg)) == NULL) {
		return SMM_ERR_OBJECT;
	}
	switch (CFGF_TYPE_GET(kp)) {
	case CFGF_TYPE_COMM:
		*(kp->comment) = kp->store[1];	/* restore the content */
		slogz("COMM: %s", kp->comment);
		*(kp->comment) = 0;
		break;
	case CFGF_TYPE_MAIN:
		*(kp->comment) = kp->store[1];	/* restore the content */
		if (kp->value) {
			slogz("MAIN: %s | %s", kp->key, kp->value);
		} else {
			slogz("MAIN: %s", kp->key);
		}
		*(kp->comment) = 0;
		break;
	case CFGF_TYPE_KEY:
		*(kp->comment) = kp->store[1];	/* restore the content */
		slogz("KEYP: %s = %s", kp->key, kp->value);
		*(kp->comment) = 0;
		break;
	case CFGF_TYPE_PART:
		*(kp->comment) = kp->store[1];	/* restore the content */
		slogz("PART: %s", kp->key);
		*(kp->comment) = 0;
		break;
	case CFGF_TYPE_ROOT:
		slogz("ROOT: %s/%s\n", kp->key, kp->value);
		break;
	default:
		slogz("BOOM!\n");
		return 0;
	}
	slogz("      Size=%d Count=%d FLAGS=%X\n", 
			kp->size, kp->update, kp->flags);
	return 0;
}

int csc_cfg_dump(void *cfg, char *mkey)
{
	KEYCB	*root, *ckey;
	CSCLNK	*p, *k;

	csc_cfg_dump_kcb(cfg);

	if ((root = CFGF_GETOBJ(cfg)) == NULL) {
		return SMM_ERR_OBJECT;
	}
	for (p = root->anchor; p != NULL; p = csc_cdl_next(root->anchor, p)) {
		if ((ckey = CFGF_GETOBJ(p)) == NULL) {
			break;
		}
		if (mkey && strcmp(ckey->key, mkey)) {
			continue;
		}
		csc_cfg_dump_kcb(ckey);
		if (CFGF_TYPE_GET(ckey) == CFGF_TYPE_MAIN) {
			for (k = ckey->anchor; k != NULL; k = csc_cdl_next(ckey->anchor, k)) {
				csc_cfg_dump_kcb(k);
			}
		}
	}
	return 0;
}


static KEYCB *csc_cfg_kcb_alloc(int psize)
{
	KEYCB	*kp;

	psize += sizeof(KEYCB) + 4;
	psize = (psize + 3) / 4 * 4;	/* round up to 32-bit boundry */
	if ((kp = smm_alloc(psize)) == NULL) {
		return NULL;
	}

	memset(kp, 0, psize);
	kp->size   = psize;
	kp->majesty = CFGF_MAGIC;
	return kp;
}

static KEYCB *csc_cfg_kcb_create(char *skey, char *value, char *comm)
{
	KEYCB	*kp;
	int	len;

	if (!skey && !value && !comm) {
		return NULL;	/* foolproof */
	}

	len = 8;	/* given some extra bytes */
	if (skey) {
		len += strlen(skey);
	}
	if (value) {
		len += strlen(value);
	}
	if (comm) {
		len += strlen(comm);
	}
	if ((kp = csc_cfg_kcb_alloc(len)) == NULL) {
		return NULL;
	}

	len = 0;	/* it's safe to reuse the 'len' */
	if (skey) {
		len += sprintf(kp->pool + len, "%s", skey);
	}
	if (value) {
		if (skey) {
			len += sprintf(kp->pool + len, "=%s", value);
		} else {
			len += sprintf(kp->pool + len, "%s", value);
		}
	}
	if (comm) {
		if (*comm == '#') {
			len += sprintf(kp->pool + len, "%s", comm);
		} else {
			len += sprintf(kp->pool + len, "#%s", comm);
		}
	}
	if (kp->pool[len-1] != '\n') {
		kp->pool[len] = '\n';
		kp->pool[len+1] = 0;
	}

	csc_cfg_kcb_fillup(kp);
	return kp;
}


/* tricky part is, after this call, the key will point to the start address 
 * of it contents. however the value and comment will point to  a '\0', 
 * which content has been saved in store[] */
static int csc_cfg_kcb_fillup(KEYCB *kp)
{
	int	i;

	/* first scan to locate the comments */
	for (i = 0; kp->pool[i]; i++) {
		if ((kp->pool[i] == '#') || (kp->pool[i] == '\n')) {
			break;
		}
	}
	/* 'i' should be indicating either '#', '\n' or '\0' now */
	for (i--; i >= 0; i--) {
		if (!SMM_ISSPACE(kp->pool[i])) {
			break;
		}
	}

	i++;
	kp->store[1] = kp->pool[i];
	kp->pool[i] = 0;
	kp->comment = &kp->pool[i];	/* this is NOT a bug */
	kp->flags = CFGF_TYPE_SET(kp, CFGF_TYPE_COMM);

	if (kp->pool[0] == 0) {		/* empty line is kind of comments */
		return CFGF_TYPE_COMM;
	}

	/* second scan to locate the value */
	for (i = 0; kp->pool[i]; i++) {
		if (kp->pool[i] == '=') {
			break;
		}
	}
	
	/* it could be a partial key or a main key if no '=' appear */
	kp->key = kp->pool;
	kp->flags = CFGF_TYPE_SET(kp, CFGF_TYPE_PART);

	if (kp->pool[i] == '=') {
		/* otherwise it could be a common key or a main key */
		kp->store[0] = kp->pool[i];
		kp->pool[i] = 0;
		kp->value = &kp->pool[i+1];
		kp->flags = CFGF_TYPE_SET(kp, CFGF_TYPE_KEY);
	}

	/* another scan to decide if it is a main key */
	for (i = 0; kp->key[i]; i++) {
		if (kp->key[i] == '[') {
			kp->flags = CFGF_TYPE_SET(kp, CFGF_TYPE_MAIN);
			break;
		}
	}
	return CFGF_TYPE_GET(kp);
}

static int csc_cfg_update(void *cfg)
{
	KEYCB	*kcb;

	if ((kcb = CFGF_GETOBJ(cfg)) == NULL) {
		return SMM_ERR_OBJECT;
	}
	kcb->update++;
	return kcb->update;
}


static KEYCB *csc_cfg_find_key(void *cfg, char *key, int type)
{
	KEYCB	*kcb, *mkey;
	CSCLNK	*mp;

	if ((mkey = CFGF_GETOBJ(cfg)) == NULL) {
		return NULL;
	}
	if ((key == NULL) && (type == CFGF_TYPE_MAIN)) {
		return mkey;
	}
	for (mp = mkey->anchor; mp; mp = csc_cdl_next(mkey->anchor, mp)) {
		if ((kcb = CFGF_GETOBJ(mp)) == NULL) {
			break;
		}
		if (CFGF_TYPE_GET(kcb) != type) {
			continue;
		}
		if (key == NULL) {
			/* if the key is not specified, it'll return the first
			 * available key control */
			return kcb;
		}
		if (kcb->key == NULL) {
			continue;
		}
		if (!csc_cfg_strcmp(kcb->key, key)) {
			return kcb;
		}
	}
	return NULL;	/* main key doesn't exist */
}

static int csc_cfg_recent_setup(void *cfg, KEYCB *mkey, KEYCB *kcb)
{
	KEYCB	*root;

	if ((root = CFGF_GETOBJ(cfg)) == NULL) {
		return SMM_ERR_OBJECT;
	}
	if (CFGF_TYPE_GET(root) != CFGF_TYPE_ROOT) {
		return SMM_ERR_OBJECT;
	}
	root->next = mkey;
	root->prev = kcb;
	return SMM_ERR_NONE;
}

static KEYCB *csc_cfg_recent_update(void *cfg, int type)
{
	KEYCB	*root, *kcb, *mkey;
	CSCLNK	*mp;

	if ((root = CFGF_GETOBJ(cfg)) == NULL) {
		return NULL;
	}
	if (CFGF_TYPE_GET(root) != CFGF_TYPE_ROOT) {
		return NULL;
	}
	mkey = root->next;
	mp = root->prev;
	if ((mkey == NULL) || (mp == NULL)) {
		return NULL;
	}
	mp = csc_cdl_next(mkey->anchor, mp);
	while (mp) {
		if ((kcb = CFGF_GETOBJ(mp)) == NULL) {
			break;
		}
		if (CFGF_TYPE_GET(kcb) == type) {
			root->prev = kcb;
			return kcb;
		}
		mp = csc_cdl_next(mkey->anchor, mp);
	}
	return NULL;
}

static FILE *csc_cfg_open_file(char *path, char *fname, int mode)
{
	FILE	*fp;
	char	*fullpath;

	if ((fullpath = csc_strcpy_alloc(path, strlen(fname)+4)) == NULL) {
		return NULL;
	}
	strcat(fullpath, SMM_DEF_DELIM);
	strcat(fullpath, fname);

	switch (mode) {
	case SMM_CFGMODE_RDONLY:
		fp = fopen(fullpath, "r");
		break;
	case SMM_CFGMODE_RWC:
		smm_mkpath(path);
		if ((fp = fopen(fullpath, "r+")) == NULL) {
			fp = fopen(fullpath, "w+");
		}
		break;
	default:	/* SMM_CFGMODE_RDWR */
		fp = fopen(fullpath, "r+");
		break;
	}
	smm_free(fullpath);
	return fp;
}

static int csc_cfg_read_next_line(FILE *fp, char *buf)
{
	int	amnt, cpos, ch;

	amnt = 0;
	cpos = ftell(fp);
	while ((ch = fgetc(fp)) != EOF) {
		if (buf) {
			*buf++ = (char) ch;
		}
		amnt++;
		if (ch == '\n') {
			break;
		}
	}
	if (buf == NULL) {	/* rewind to the start position */
		fseek(fp, cpos, SEEK_SET);
	} else {
		*buf++ = 0;
	}
	return amnt;
}

static int csc_cfg_write_next_line(FILE *fp, KEYCB *kp)
{
	if (kp == NULL) {
		return 0;
	}

	kp->update = 0;		/* reset the update counter */
	if (kp->key) {
		fputs(kp->key, fp);
	}
	if (kp->value) {
		if (kp->key) {
			fputc(kp->store[0], fp);
		}
		fputs(kp->value, fp);
	}
	
	*(kp->comment) = kp->store[1];
	fputs(kp->comment, fp);
	*(kp->comment) = 0;
	return 0;
}

/* strcmp() without comparing the head and tail white spaces */
static int csc_cfg_strcmp(char *sour, char *dest)
{
	int	slen, dlen;

	/* skip the heading white spaces */
	sour = csc_strbody(sour, &slen);
	dest = csc_strbody(dest, &dlen);

	/* comparing the body content */
	if (slen == dlen) {
		return strncmp(sour, dest, slen);
	}
	return slen - dlen;
}

static int csc_cfg_binary_to_hex(char *src, int slen, char *buf, int blen)
{
	char	temp[4];
	int	i;

	for (i = 0; i < slen; i++) {
		sprintf(temp, "%02X", (unsigned char) *src++);
		if (buf && (blen > 1)) {
			blen -= 2;
			*buf++ = temp[0];
			*buf++ = temp[1];
		}
	}
	if (buf && (blen > 0)) {
		*buf++ = 0;
	}
	return slen * 2;	/* return the length of the hex string */
}

static int csc_cfg_hex_to_binary(char *src, char *buf, int blen)
{
	char	temp[4];
	int	amnt = 0;

	while (*src) {
		if (isxdigit(*src)) {
			temp[0] = *src++;
		}
		if (isxdigit(*src)) {
			temp[1] = *src++;
		} else {
			break;
		}
		temp[2] = 0;

		if (buf && blen) {
			*buf++ = (char)strtol(temp, 0, 16);
			blen--;
		}
		amnt++;
	}
	return amnt;
}


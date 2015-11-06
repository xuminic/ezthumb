/*!\file       csc_config.c - simple interface of configure file access

   \author     "Andy Xuming" <xuming@users.sourceforge.net>
   \date       2013-2014
 
    Terminology

    Path:
    (FILESYSTEM) $HOME/.config/company
    (REGISTRY)   HKEY_CURRENT_USER\\SOFTWARE\\company

    Root for Root Key:
    (FILESYSTEM) $HOME/.config/company/product.conf   
    (REGISTRY)   HKEY_CURRENT_USER\\SOFTWARE\\company\\product

    Directory or Dir Key without the value:
    (FILESYSTEM) [main/section/device]
    (REGISTRY)   main\\section\\device      (Win32 Term: keys and sub-keys)

    Binary Block, a special form of Directory:
    (FILESYSTEM) [main/section/device] = BINARY:128  ##REG_BINARY
    (REGISTRY)   main\\section\\device	REG_BINARY

    Normal Key and Value:
    (FILESYSTEM) key = value #Comments
    (REGISTRY)   key REG_SZ value           (Win32 Term: values)

    Comment: blank line or anything starts with '#' are comments.
    (FILESYSTEM) #xxxxxxxxx

    Partial Key:
    (FILESYSTEM) key

    Partial Value:
    (FILESYSTEM) = value
*/
/* Copyright (C) 1998-2014  "Andy Xuming" <xuming@users.sourceforge.net>

   This file is part of CSOUP library, Chicken Soup for the C

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

#define CSOUP_DEBUG_LOCAL	SLOG_CWORD(CSOUP_MOD_CONFIG, SLOG_LVL_ERROR)
#include "libcsoup.h"
#include "csoup_internal.h"

#define	CFGF_BLOCK_WID		36
#define CFGF_BLOCK_MAGIC	"BINARY:"


/* KEYCB extension for the root structure */
struct	KEYROOT	{
	KEYCB	*dkcb;	/* latest accessed directory key */
	KEYCB	*nkcb;	/* latest accessed sub key */
	int	mode;
	int	sysdir;
	int	items;	/* number of keys that has been read (debug) */
	char	*fulldir;	/* the full path of configure file (debug) */
	char	pool[1];
};

static KEYCB *csc_cfg_root_alloc(int sysdir, char *path, char *filename, int);
static KEYCB *csc_cfg_kcb_create(char *key, char *value, char *comm);
int csc_cfg_kcb_fillup(KEYCB *kp);
static KEYCB *csc_cfg_find_key(KEYCB *cfg, char *key, int type);
static KEYCB *csc_cfg_find_dir_exec(KEYCB *cfg, char *key);
static KEYCB *csc_cfg_find_dir(KEYCB *cfg, char *dkey);
static KEYCB *csc_cfg_mkdir(KEYCB *cfg, char *key, char *value, char *comm);
static int csc_cfg_insert(KEYCB *cfg, KEYCB *kcb);
static int csc_cfg_access_setup(KEYCB *cfg, KEYCB *dkcb, KEYCB *kcb);
static KEYCB *csc_cfg_access_update(KEYCB *cfg, int type);
static int csc_cfg_destroy_links(CSCLNK *anchor);
static int csc_cfg_save_links(struct KeyDev *cfgd, CSCLNK *anchor);
static int csc_cfg_attribute(KEYCB *kcb, int *attr);
static int csc_cfg_strcmp(char *sour, char *dest);
static char *csc_cfg_format_directory(char *dkey);
static char *csc_cfg_format_dir_alloc(char *dkey);

int csc_cfg_binary_to_hex(char *src, int slen, char *buf, int blen);
int csc_cfg_hex_to_binary(char *src, char *buf, int blen);


static KEYCB *CFGF_GETOBJ(CSCLNK *self)
{
	KEYCB   *kcb = csc_cdl_payload(self);

	if ((CFGF_TYPE_GET(kcb->flags) > CFGF_TYPE_UNKWN) && 
			(CFGF_TYPE_GET(kcb->flags) < CFGF_TYPE_NULL)) {
		return kcb;
	}
	CDB_ERROR(("CFGF_GETOBJ: unknown object\n"));
	return NULL;
}

KEYCB *csc_cfg_open(int sysdir, char *path, char *filename, int mode)
{
	struct	KeyDev	*cfgd;
	struct	KEYROOT	*rext;
	KEYCB	*root, *kp;

	/* create the root control block */
	if (sysdir == SMM_CFGROOT_MEMPOOL) {
		root = csc_cfg_root_alloc(sysdir, NULL, NULL, mode);
	} else {
		root = csc_cfg_root_alloc(sysdir, path, filename, mode);
	}
	if (root == NULL) {
		return NULL;
	}

	/** In the csc_cfg_open() function, the configure file will be
	 * read and the contents will be collected into the memory. 
	 * The configure file then will be closed until saving the contents
	 * back to the file. It's pointless to create a new file in the
	 * csc_cfg_open() stage */
	cfgd = smm_config_open(sysdir, path, filename, 
			CFGF_MODE_SET(mode, CSC_CFG_READ));
	if (cfgd == NULL) {
		if (CFGF_MODE_GET(mode) == CSC_CFG_RWC) {
			return root;
		}
		csc_cfg_kcb_free(root);
		return NULL;
	}

	rext = (struct KEYROOT *) root->pool;
	rext->items = smm_config_current(cfgd, NULL, 0) + 4;
	if ((rext->fulldir = smm_alloc(rext->items)) != NULL) {
		smm_config_current(cfgd, rext->fulldir, rext->items);
	}
	rext->items = 0;
	while ((kp = smm_config_read_alloc(cfgd)) != NULL) {
		/* In Win32 registry interface, smm_config_read() will read 
		 * and fill in the KEYCB structure itself. However if the 
		 * configure were read from a file, it need to be break down 
		 * and fill in the KEYCB by csc_cfg_kcb_fillup() */
		if (CFGF_TYPE_GET(kp->flags) == CFGF_TYPE_UNKWN) {
			csc_cfg_kcb_fillup(kp);
		}
		csc_cfg_insert(root, kp);

		switch (CFGF_TYPE_GET(kp->flags)) {
		case CFGF_TYPE_DIR:
		case CFGF_TYPE_KEY:
			rext->items++;
		}
	}
	smm_config_close(cfgd);
	csc_cfg_access_setup(root, root, NULL);	/* reset the directory key */
	CDB_DEBUG(("csc_cfg_open: read %d items from %s\n", 
			rext->items, rext->fulldir));
	return root;
}

int csc_cfg_free(KEYCB *cfg)
{
	struct	KEYROOT	*rext;

	if (cfg == NULL) {
		return SMM_ERR_NULL;
	}
	if (cfg->anchor) {
		csc_cfg_destroy_links(cfg->anchor);
		cfg->anchor = NULL;
	}

	rext = (struct KEYROOT *) cfg->pool;
	if (rext->fulldir) {
		smm_free(rext->fulldir);
		rext->fulldir = NULL;
	}
	csc_cfg_kcb_free(cfg);
	return SMM_ERR_NONE;
}

int csc_cfg_save(KEYCB *cfg)
{
	struct	KEYROOT	*rext;

	if (cfg == NULL) {
		return SMM_ERR_NULL;
	}
	if (CFGF_MODE_GET(cfg->flags) == CSC_CFG_READ) {
		return SMM_ERR_ACCESS;
	}
	rext = (struct KEYROOT *)cfg->pool;
	return csc_cfg_saveas(cfg, rext->sysdir, cfg->key, cfg->value);
}

int csc_cfg_saveas(KEYCB *cfg, int sysdir, char *path, char *filename)
{
	struct	KeyDev	*cfgd;
	struct	KEYROOT	*rext;

	if (cfg == NULL) {
		return SMM_ERR_NULL;
	}
	if (CFGF_TYPE_GET(cfg->flags) != CFGF_TYPE_ROOT) {
		return SMM_ERR_ACCESS;
	}

	if (sysdir != SMM_CFGROOT_MEMPOOL) {
		cfgd = smm_config_open(sysdir, path, filename, CSC_CFG_RWC);
	} else if (path) {
		cfgd = smm_config_open(sysdir, path, filename,strlen(path)+1);
	} else {
		cfgd = smm_config_open(sysdir, path, filename, 0);
	}
	if (cfgd == NULL) {
		return SMM_ERR_ACCESS;
	}

	/* if csc_cfg_open() hasn't create the full directory field, which
	 * is possible by open an empty configure, it's time to make one */
	rext = (struct KEYROOT *) cfg->pool;
	if (rext->fulldir == NULL) {
		int len = smm_config_current(cfgd, NULL, 0) + 4;
		if ((rext->fulldir = smm_alloc(len)) != NULL) {
			smm_config_current(cfgd, rext->fulldir, len);
		}
	}

	rext->items = csc_cfg_save_links(cfgd, cfg->anchor);
	CDB_DEBUG(("csc_cfg_saveas: write %d items to %s\n", 
			rext->items, rext->fulldir));

	smm_config_close(cfgd);
	cfg->update = 0;	/* reset the update counter */
	return SMM_ERR_NONE;
}

int csc_cfg_flush(KEYCB *cfg)
{
	if (cfg == NULL) {
		return SMM_ERR_NULL;
	}
	if (cfg->update) {
		return csc_cfg_save(cfg);
	}
	return SMM_ERR_NONE;
}

int csc_cfg_close(KEYCB *cfg)
{
	int	rc;

	rc = csc_cfg_flush(cfg);
	csc_cfg_free(cfg);
	return rc;
}

char *csc_cfg_status(KEYCB *cfg, int *keys)
{
	struct	KEYROOT	*rext;

	if (cfg == NULL) {
		return NULL;
	}
	rext = (struct KEYROOT *) cfg->pool;
	if (keys) {
		*keys = rext->items;
	}
	return rext->fulldir;
}

char *csc_cfg_read(KEYCB *cfg, char *dkey, char *nkey)
{
	KEYCB	*dcb, *ncb;

	dcb = csc_cfg_find_dir(cfg, dkey);
	if ((ncb = csc_cfg_find_key(dcb, nkey, CFGF_TYPE_KEY)) == NULL) {
		return NULL;
	}
	csc_cfg_access_setup(cfg, dcb, ncb);
	return ncb->value;
}

char *csc_cfg_read_first(KEYCB *cfg, char *dkey, char **key)
{
	struct	KEYROOT	*rext;

	if (csc_cfg_read(cfg, dkey, NULL) == NULL) {
		return NULL;
	}

	rext = (struct KEYROOT *)cfg->pool;
	if (key) {
		*key = rext->nkcb->key;
	}
	return rext->nkcb->value;
}

char *csc_cfg_read_next(KEYCB *cfg, char **key)
{
	KEYCB	*ncb;

	if ((ncb = csc_cfg_access_update(cfg, CFGF_TYPE_KEY)) == NULL) {
		return NULL;
	}
	if (key) {
		*key = ncb->key;
	}
	return ncb->value;
}

char *csc_cfg_copy(KEYCB *cfg, char *dkey, char *nkey, int extra)
{
	char	*value;

	if ((value = csc_cfg_read(cfg, dkey, nkey)) == NULL) {
		return NULL;
	}
	return csc_strcpy_alloc(value, extra);
}

int csc_cfg_write(KEYCB *cfg, char *dkey, char *nkey, char *value)
{
	struct	KEYROOT	*rext;
	KEYCB	*dcb, *ncb, *kcb;
	int	olen, nlen;

	if ((value == NULL) || (nkey == NULL) || (cfg == NULL)) {
		return SMM_ERR_NULL;
	}

	rext = (struct KEYROOT *) cfg->pool;
	if ((dcb = csc_cfg_mkdir(cfg, dkey, NULL, NULL)) == NULL) {
		return SMM_ERR_NULL;
	}
	if ((ncb = csc_cfg_find_key(dcb, nkey, CFGF_TYPE_KEY)) == NULL) {
		/* if the key doesn't exist, it'll create a new key and
		 * insert to the tail. To tail shows a more natural way
		 * of display. But there is no white space line in the head.
		 */
		if ((ncb = csc_cfg_kcb_create(nkey, value, NULL)) != NULL) {
			dcb->update++;
			cfg->update++;
			csc_cdl_list_insert_tail(&dcb->anchor, ncb->self);
			csc_cfg_access_setup(cfg, dcb, ncb);
			rext->items++;
		}
		return SMM_ERR_NONE;
	}

	csc_cfg_access_setup(cfg, dcb, ncb);
	olen = strlen(ncb->value);
	nlen = strlen(value);
	if (!csc_cfg_strcmp(value, ncb->value)) {
		/* same value so do nothing */
		return SMM_ERR_NONE;
	} else if (nlen <= olen) {
		/* If the new value is smaller than or same to the original 
		 * value, it'll replace the original value directly. */
		strcpy(ncb->value, value);
	} else {
		/* If the new value is larger than the original value, it'll
		 * create a new key to replace the old key structure */
		kcb = csc_cfg_kcb_create(nkey, value, ncb->comment);
		if (kcb == NULL) {
			return SMM_ERR_LOWMEM;
		}
		kcb->update = ncb->update;
		csc_cdl_insert_after(ncb->self, kcb->self);
		csc_cdl_list_free(&dcb->anchor, ncb->self);
		ncb = kcb;
	}
	ncb->update++;
	if (ncb->update == 1) {
		dcb->update++;
		cfg->update++;
	}
	rext->items++;
	return SMM_ERR_NONE;
}

int csc_cfg_read_int(KEYCB *cfg, char *dkey, char *nkey, int *val)
{
	char 	*value;

	if ((value = csc_cfg_read(cfg, dkey, nkey)) == NULL) {
		return SMM_ERR_NULL;
	}
	if (val) {
		*val = (int) strtol(value, NULL, 0);
	}
	return SMM_ERR_NONE;
}

int csc_cfg_write_int(KEYCB *cfg, char *dkey, char *nkey, int val)
{
	char	buf[32];

	sprintf(buf, "%d", val);
	return csc_cfg_write(cfg, dkey, nkey, buf);
}

int csc_cfg_read_long(KEYCB *cfg, char *dkey, char *nkey, long *val)
{
	char 	*value;

	if ((value = csc_cfg_read(cfg, dkey, nkey)) == NULL) {
		return SMM_ERR_NULL;
	}
	if (val) {
		*val = strtol(value, NULL, 0);
	}
	return SMM_ERR_NONE;
}

int csc_cfg_write_long(KEYCB *cfg, char *dkey, char *nkey, long val)
{
	char	buf[32];

	sprintf(buf, "%ld", val);
	return csc_cfg_write(cfg, dkey, nkey, buf);
}

int csc_cfg_read_longlong(KEYCB *cfg, char *dkey, char *nkey, long long *val)
{ 
	char	*value;

	if ((value = csc_cfg_read(cfg, dkey, nkey)) == NULL) {
		return SMM_ERR_NULL;
	}
	if (val) {
		*val = strtoll(value, NULL, 0);
	}
	return SMM_ERR_NONE;
}

int csc_cfg_write_longlong(KEYCB *cfg, char *dkey, char *nkey, long long val)
{
	char	buf[32];

	SMM_SPRINT(buf, "%lld", val);
	return csc_cfg_write(cfg, dkey, nkey, buf);
}

int csc_cfg_read_bin(KEYCB *cfg, char *dkey, char *nkey, char *buf, int blen)
{
	char	*src, *value;
	int	len;

	if ((value = csc_cfg_read(cfg, dkey, nkey)) == NULL) {
		return SMM_ERR_NULL;
	}

	src = csc_strbody(value, NULL);	/* strip the white space */
	len = csc_cfg_hex_to_binary(src, buf, blen);
	return len;
}

void *csc_cfg_copy_bin(KEYCB *cfg, char *dkey, char *nkey, int *bsize)
{
	char	*buf;
	int	len;

	if ((len = csc_cfg_read_bin(cfg, dkey, nkey, NULL, 0)) <= 0) {
		return NULL;
	}
	if ((buf = smm_alloc(len)) == NULL) {
		return NULL;
	}
	if (bsize) {
		*bsize = len;
	}
	csc_cfg_read_bin(cfg, dkey, nkey, buf, len);
	return buf;
}

int csc_cfg_write_bin(KEYCB *cfg, char *dkey, char *nkey, void *bin, int bsize)
{
	char	*buf;

	if (!bin || !bsize) {
		return SMM_ERR_NULL;
	}
	if ((buf = smm_alloc((bsize+1)*2)) == NULL) {
		return SMM_ERR_LOWMEM;
	}
	csc_cfg_binary_to_hex(bin, bsize, buf, (bsize+1)*2);
	bsize = csc_cfg_write(cfg, dkey, nkey, buf);
	smm_free(buf);
	return bsize;
}

int csc_cfg_read_block(KEYCB *cfg, char *dkey, char *buf, int blen)
{
	KEYCB	*dcb, *ncb;
	char	*src, tmp[256];
	int	len, amnt;

	if ((dcb = csc_cfg_find_dir(cfg, dkey)) == NULL) {
		return -1; 	/* block not found */
	}
	if (csc_cfg_block_size(dcb) < 0) {
		return -2;	/* wrong block type */
	}
	if ((ncb = csc_cfg_find_key(dcb, NULL, CFGF_TYPE_PART)) == NULL) {
		return 0;	/* empty block */
	}
	csc_cfg_access_setup(cfg, dcb, ncb);

	amnt = 0;
	do {
		/* convert hex to binary */
		src = csc_strbody(ncb->key, NULL); /* strip the space */
		len = csc_cfg_hex_to_binary(src, tmp, sizeof(tmp));

		/* store to the buffer */
		if (blen && (blen < (amnt + len))) {
			break;
		}
		if (buf) {
			memcpy(buf + amnt, tmp, len);
		}
		amnt += len;
	} while ((ncb = csc_cfg_access_update(cfg, CFGF_TYPE_PART)) != NULL);
	return amnt;
}

void *csc_cfg_copy_block(KEYCB *cfg, char *dkey, int *bsize)
{
	char	*buf;
	int	len;

	if ((len = csc_cfg_read_block(cfg, dkey, NULL, 0)) <= 0) {
		return NULL;
	}
	if ((buf = smm_alloc(len)) == NULL) {
		return NULL;
	}
	csc_cfg_read_block(cfg, dkey, buf, len);
	if (bsize) {
		*bsize = len;
	}
	return buf;
}

int csc_cfg_write_block(KEYCB *cfg, char *dkey, void *bin, int bsize)
{
	struct	KEYROOT	*rext;
	KEYCB	*dcb;

	if (cfg == NULL) {
		return SMM_ERR_NULL;
	}

	dcb = csc_cfg_mkdir(cfg, dkey, CFGF_BLOCK_MAGIC "1234567890", NULL);
	if (dcb == NULL) {
		return SMM_ERR_LOWMEM;
	} 

	/* you can't write blocks into other types of dir keys */
	if (csc_cfg_block_size(dcb) < 0) {
		return SMM_ERR_ACCESS;
	}

	if (csc_cfg_link_block(dcb, bin, bsize) == SMM_ERR_NONE) {
		cfg->update++;
	}
	rext = (struct KEYROOT *) cfg->pool;
	rext->items++;
	return bsize;
}

int csc_cfg_link_block(KEYCB *block, void *bin, int bsize)
{
	KEYCB	*ncb;
	char	*src, tmp[CFGF_BLOCK_WID * 4];
	int	len, rest;

	if (bin == NULL) {
		bsize = 0;
	}
	if (block == NULL) {
		return SMM_ERR_NULL;
	}

	/* if the directory key does exist, destory its contents */
	csc_cdl_list_destroy(&block->anchor);
	//block->update = 0;

	for (src = bin, rest = bsize; rest > 0; rest -= CFGF_BLOCK_WID) {
		/* convert the binary to hex codes */
		len = rest > CFGF_BLOCK_WID ? CFGF_BLOCK_WID : rest;
		csc_cfg_binary_to_hex(src, len, tmp, sizeof(tmp));
		src += len;

		/* insert to the tail */
		if ((ncb = csc_cfg_kcb_create(tmp, NULL, NULL)) == NULL) {
			return SMM_ERR_LOWMEM;
		}
		//block->update++;
		csc_cdl_list_insert_tail(&block->anchor, ncb->self);
	}

	/* make sure it's a binary block */
	block->flags = CFGF_TYPE_SET(block->flags, CFGF_TYPE_DIR);
	block->vsize = bsize;
	sprintf(block->value, CFGF_BLOCK_MAGIC "%d", bsize);
	return SMM_ERR_NONE;
}

/* return:
 * <0: not a binary block
 * 0: empty binary block
 * n: content size of the binary block */
#define MAGIC_LENGTH	(sizeof(CFGF_BLOCK_MAGIC) - 1)
int csc_cfg_block_size(KEYCB *kcb)
{
	if (kcb->value == NULL) {
		return -1;
	}
	if (strncmp(kcb->value, CFGF_BLOCK_MAGIC, MAGIC_LENGTH)) {
		return -2;
	}
	return (int) strtol(kcb->value + MAGIC_LENGTH, NULL, 0);
}


int csc_cfg_delete_key(KEYCB *cfg, char *dkey, char *nkey)
{
	struct	KEYROOT	*rext;
	KEYCB	*dcb, *ncb;

	if ((nkey == NULL) || (cfg == NULL)) {
		return SMM_ERR_NULL;
	}

	dcb = csc_cfg_find_dir(cfg, dkey);
	if ((ncb = csc_cfg_find_key(dcb, nkey, CFGF_TYPE_KEY)) == NULL) {
		return SMM_ERR_NONE;
	}
	csc_cdl_list_free(&dcb->anchor, ncb->self);
	dcb->update++;
	cfg->update++;

	rext = (struct KEYROOT *) cfg->pool;
	rext->items--;
	return SMM_ERR_NONE;
}

int csc_cfg_delete_block(KEYCB *cfg, char *dkey)
{
	struct	KEYROOT	*rext;
	KEYCB	*dcb;

	if (cfg == NULL) {
		return SMM_ERR_NULL;
	}

	if ((dcb = csc_cfg_find_dir(cfg, dkey)) == NULL) {
		return SMM_ERR_NULL;
	} 

	/* do not delete other types of dir keys */
	if (csc_cfg_block_size(dcb) < 0) {
		return SMM_ERR_ACCESS;
	}

	csc_cdl_list_destroy(&dcb->anchor);
	csc_cdl_list_free(&cfg->anchor, dcb->self);

	cfg->update++;
	rext = (struct KEYROOT *) cfg->pool;
	rext->items--;
	return SMM_ERR_NONE;

}


KEYCB *csc_cfg_kcb_alloc(int psize)
{
	CSCLNK	*node;
	KEYCB	*kp;

	psize += sizeof(KEYCB) + 8;	/* reserved 8 bytes */
	psize = (psize + 3) / 4 * 4;	/* round up to 32-bit boundry */

	if ((node = csc_cdl_alloc(psize)) == NULL) {
		return NULL;
	}
	kp = (KEYCB*) csc_cdl_payload(node);
	kp->self = node;
	return kp;
}

int csc_cfg_kcb_free(KEYCB *kcb)
{
	return smm_free(kcb->self);
}


#define CFGF_SHOW_CONTENT	(CSC_MEMDUMP_NO_GLYPH | CSC_MEMDUMP_NO_ADDR)
int csc_cfg_dump_kcb(KEYCB *kp)
{
	switch (CFGF_TYPE_GET(kp->flags)) {
	case CFGF_TYPE_COMM:
		CDB_SHOW(("COMM_%d: %s\n", kp->update, kp->comment));
		break;
	case CFGF_TYPE_DIR:
		if (csc_cfg_block_size(kp) >= 0) {
			CDB_SHOW(("BLCK_%d: [%s] = %s %s\n", kp->update,
					kp->key, kp->value, kp->comment));
		} else if (kp->value) {
			CDB_SHOW(("MAIN_%d: [%s] = %s %s\n", kp->update, 
					kp->key, kp->value, kp->comment));
		} else {
			CDB_SHOW(("MAIN_%d: [%s] %s\n", kp->update, 
					kp->key, kp->comment));
		}
		break;
	case CFGF_TYPE_KEY:
		if (kp->comment == NULL) {
			CDB_SHOW(("KEYP_%d: %s = %s %s\n", kp->update, 
					kp->key, kp->value, kp->comment));
		} else if (!csc_strcmp_list(kp->comment, "##REG_BINARY",
					"##REG_NONE", "##REG_UNKNOWN", NULL)) {
			char	buf[80];
			buf[0] = 0;
			csc_memdump_line(kp->value, 
				kp->vsize < 8 ? kp->vsize : 8,
				CFGF_SHOW_CONTENT, buf, 80);
			CDB_SHOW(("KEYP_%d: %s = (%d)%s %s\n", kp->update,
					kp->key, kp->vsize, buf, kp->comment));
		} else {
			CDB_SHOW(("KEYP_%d: %s = %s %s\n", kp->update,
					kp->key, kp->value, kp->comment));
		}
		break;
	case CFGF_TYPE_PART:
		CDB_SHOW(("PART_%d: %s %s %s\n", kp->update,
				kp->key, kp->value, kp->comment));
		break;
	case CFGF_TYPE_VALUE:
		CDB_SHOW(("VALU_%d: %s %s %s\n", kp->update,
				kp->key, kp->value, kp->comment));
		break;
	case CFGF_TYPE_ROOT:
		CDB_SHOW(("ROOT: %s/%s\n", kp->key, kp->value));
		break;
	default:
		CDB_SHOW(("BOOM!\n"));
		return 0;
	}
	return 0;
}

int csc_cfg_dump(KEYCB *entry)
{
	KEYCB	*ckey;
	CSCLNK	*p;

	csc_cfg_dump_kcb(entry);
	if (entry->anchor == NULL) {
		return 0;
	}

	for (p = entry->anchor; p; p = csc_cdl_next(entry->anchor, p)) {
		ckey = CFGF_GETOBJ(p);
		if (ckey->anchor == NULL) {
			csc_cfg_dump_kcb(ckey);
		}
	}
	for (p = entry->anchor; p; p = csc_cdl_next(entry->anchor, p)) {
		ckey = CFGF_GETOBJ(p);
		if (ckey->anchor != NULL) {
			csc_cfg_dump(ckey);
		}
	}
	return 0;
}

int csc_cfg_binary_to_hex(char *src, int slen, char *buf, int blen)
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

int csc_cfg_hex_to_binary(char *src, char *buf, int blen)
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


/****************************************************************************
 * Internal Functions
 ****************************************************************************/
static KEYCB *csc_cfg_root_alloc(int sysdir, char *path, 
		char *filename, int mode)
{
	struct	KEYROOT	*rext;
	KEYCB	*root;
	int	len;

	len = sizeof(struct KEYROOT) + 8;	/* reserve 8 bytes */
	if (path) {
		len += strlen(path);
	}
	if (filename) {
		len += strlen(filename);
	}

	if ((root = csc_cfg_kcb_alloc(len)) == NULL) {
		return NULL;
	}
	root->flags = CFGF_MODE_SET(root->flags, mode);
	root->flags = CFGF_TYPE_SET(root->flags, CFGF_TYPE_ROOT);

	rext = (struct KEYROOT *) root->pool;
	rext->sysdir = sysdir;
	rext->dkcb   = root;
	if (path) {
		root->key = rext->pool;
		strcpy(root->key, path);
	}
	if (filename) {
		root->value = rext->pool;
		if (root->key) {
			root->value += strlen(root->key) + 1;
		}
		strcpy(root->value, filename);
	}
	return root;
}

static KEYCB *csc_cfg_kcb_create(char *key, char *val, char *comm)
{
	KEYCB	*kp;
	char	*p;
	int	len;

	if (!key && !val && !comm) {
		return NULL;	/* foolproof */
	}

	len = 8;	/* given some extra bytes */
	if (key) {
		len += strlen(key);
	}
	if (val) {
		len += strlen(val);
	}
	if (comm) {
		len += strlen(comm);
	}
	if ((kp = csc_cfg_kcb_alloc(len)) == NULL) {
		return NULL;
	}

	len = 0;	/* it's safe to reuse the 'len' */
	if (key) {
		kp->key = kp->pool + len;
		len += sprintf(kp->key, "%s", key) + 1;
	}
	if (val) {
		kp->value = kp->pool + len;
		len += sprintf(kp->value, "%s", val) + 1;
	}
	if (comm) {
		kp->comment = kp->pool + len;
		p = csc_strbody(comm, NULL);
		if ((*p == 0) || (*p == '#')) {
			sprintf(kp->comment, "%s", comm);
		} else {
			sprintf(kp->comment, "#%s", comm);
		}
	}

	if (key) {
		if (val) {
			kp->flags = CFGF_TYPE_SET(kp->flags, CFGF_TYPE_KEY);
		} else {
			kp->flags = CFGF_TYPE_SET(kp->flags, CFGF_TYPE_PART);
		}
	} else if (val) {
		kp->flags = CFGF_TYPE_SET(kp->flags, CFGF_TYPE_VALUE);
	} else {
		kp->flags = CFGF_TYPE_SET(kp->flags, CFGF_TYPE_COMM);
	}
	kp->update++;
	return kp;
}


/* tricky part is, after this call, the key will point to the start address 
 * of it contents. however the value and comment will point to  a '\0', 
 * which content has been saved in store[] */
int csc_cfg_kcb_fillup(KEYCB *kp)
{
	char	*p, *body;
	int	i;

	/* first scan to locate the comments */
	for (i = 0; kp->pool[i]; i++) {
		if ((kp->pool[i] == '#') || (kp->pool[i] == '\n')) {
			break;
		}
	}
	/* after the last loop, 'i' should be indicating either '#', '\n' 
	 * or '\0' now */
	/* starts another loop to include tailing whitespace into comments */
	for (i--; i >= 0; i--) {
		if (!SMM_ISSPACE(kp->pool[i])) {
			break;
		}
	}

	i++;
	memmove(&kp->pool[i+1], &kp->pool[i], strlen(&kp->pool[i])+1);
	kp->pool[i] = 0;
	kp->comment = &kp->pool[i+1];
	if ((p = strchr(kp->comment, '\n')) != NULL) {
		*p = 0;
	}
	if ((p = strchr(kp->comment, '\r')) != NULL) {
		*p = 0;
	}
	kp->flags = CFGF_TYPE_SET(kp->flags, CFGF_TYPE_COMM);

	if (kp->pool[0] == 0) {		/* empty line is kind of comments */
		return CFGF_TYPE_COMM;
	}

	/* Another scan to locate the key and value. */
	body = csc_strbody(kp->pool, NULL);
	if ((p = strchr(body, '=')) == NULL) {
		/* It could be a partial key or a directory key */
		kp->key = body;
		kp->flags = CFGF_TYPE_SET(kp->flags, CFGF_TYPE_PART);
	} else if (*body == '=') {
		/* value only */
		kp->value = body + 1;
		kp->flags = CFGF_TYPE_SET(kp->flags, CFGF_TYPE_VALUE);
		return CFGF_TYPE_VALUE;
	} else {
		/* otherwise it could be a common key or a directory key */
		kp->key = body;
		*p++ = 0;
		kp->value = p;
		kp->flags = CFGF_TYPE_SET(kp->flags, CFGF_TYPE_KEY);
	}

	/* Last scan to decide if it is a directory or a block */
	if ((*kp->key == '[') && strchr(kp->key + 2, ']')) { 
		kp->key++;	/* strip the '[]' pair */
		p = strchr(kp->key + 1, ']');
		*p = 0;
		kp->key   = csc_cfg_format_directory(kp->key);
		kp->flags = CFGF_TYPE_SET(kp->flags, CFGF_TYPE_DIR);
		kp->vsize = csc_cfg_block_size(kp);
	}
	return CFGF_TYPE_GET(kp->flags);
}

static KEYCB *csc_cfg_find_key(KEYCB *cfg, char *key, int type)
{
	KEYCB	*kcb;
	CSCLNK	*mp;

	if (cfg == NULL) {
		return NULL;
	}
	for (mp = cfg->anchor; mp; mp = csc_cdl_next(cfg->anchor, mp)) {
		if ((kcb = CFGF_GETOBJ(mp)) == NULL) {
			break;
		}
		if (CFGF_TYPE_GET(kcb->flags) == type) {
			if (key == NULL) {
				/* if the key is not specified, it'll return 
				 * the first available key control */
				return kcb;
			}
			if (kcb->key && !csc_cfg_strcmp(kcb->key, key)) {
				/* return the matched KEYCB */
				return kcb;
			}
		}
	}
	return NULL;
}

static KEYCB *csc_cfg_find_dir_exec(KEYCB *cfg, char *key)
{
	KEYCB	*kcb;
	CSCLNK	*mp;

	for (mp = cfg->anchor; mp; mp = csc_cdl_next(cfg->anchor, mp)) {
		if ((kcb = CFGF_GETOBJ(mp)) == NULL) {
			break;
		}
		if (CFGF_TYPE_GET(kcb->flags) != CFGF_TYPE_DIR) {
			continue;
		}
		if (kcb->key && !csc_cfg_strcmp(kcb->key, key)) {
			return kcb;
		}
		if (kcb->anchor == NULL) {
			continue;
		}
		if ((kcb = csc_cfg_find_dir_exec(kcb, key)) != NULL) {
			return kcb;
		}
	}
	return NULL;	/* directory key doesn't exist */
}

static KEYCB *csc_cfg_find_dir(KEYCB *cfg, char *dkey)
{
	KEYCB	*dcb;
	char	*fkey;

	if (cfg == NULL) {
		return NULL;
	}
	if ((dkey == NULL) || (*dkey == 0)) {
		return cfg;
	}
	if ((fkey = csc_cfg_format_dir_alloc(dkey)) == NULL) {
		return NULL;
	}
	dcb = csc_cfg_find_dir_exec(cfg, fkey);
	smm_free(fkey);
	return dcb;
}

static KEYCB *csc_cfg_mkdir(KEYCB *cfg, char *dkey, char *value, char *comm)
{
	KEYCB	*dcb, *dlast;
	char	*p, *fkey;

	if (cfg == NULL) {
		return NULL;
	}
	if ((dkey == NULL) || (*dkey == 0)) {
		return cfg;
	}
	if ((fkey = csc_cfg_format_dir_alloc(dkey)) == NULL) {
		return NULL;
	}
	if ((dcb = csc_cfg_find_dir_exec(cfg, fkey)) != NULL) {
		smm_free(fkey);
		return dcb;
	}

	p = fkey;
	dlast = cfg;
	do {
		if ((p = strchr(p, '/')) != NULL) {
			*p = 0;
		}
		if ((dcb = csc_cfg_find_dir_exec(cfg, fkey)) == NULL) {
			if (p == NULL) {	/* last directory */
				dcb = csc_cfg_kcb_create(fkey, value, comm);
			} else {
				dcb = csc_cfg_kcb_create(fkey, NULL, NULL);
			}
			if (dcb == NULL) {
				break;
			}
			dcb->flags  = CFGF_TYPE_SET(dcb->flags, CFGF_TYPE_DIR);
			dcb->update = 0;        /* reset the counter */
			csc_cdl_list_insert_tail(&dlast->anchor, dcb->self);
			/* 20151104 Do not update it because it could be the
			 * existed directory reading from the configure file */
			//cfg->update++;
		}
		dlast = dcb;
		if (p) {
			*p++ = '/';
		}
	} while (p);
	smm_free(fkey);
	return dcb;
}

static int csc_cfg_insert(KEYCB *cfg, KEYCB *kcb)
{
	struct	KEYROOT	*rext;

	rext = (struct KEYROOT *)cfg->pool;
	if (rext->dkcb == NULL) {
		rext->dkcb = cfg;	/* safe zone */
	}

	/* if the kcb is a binary block, it could be a control block
	 * with or without its content block. If the kcb came from the
	 * Win32 registry interface, it should have already generated the
	 * whole list of the block content so it would be easy to be inserted
	 * into the current directory. Otherwise we need to make the control
	 * block to be the current directory and insert further contents in */
	if (CFGF_TYPE_GET(kcb->flags) != CFGF_TYPE_DIR) {
		csc_cdl_list_insert_tail(&rext->dkcb->anchor, kcb->self);
	} else if (kcb->anchor) {	/* directory or binary with contents */
		csc_cdl_list_insert_tail(&rext->dkcb->anchor, kcb->self);
	} else {
		rext->dkcb = csc_cfg_mkdir(cfg, kcb->key, kcb->value,
				kcb->comment);
		if (rext->dkcb == NULL) {
			return SMM_ERR_NULL;
		}
	}
	return SMM_ERR_NONE;
}

static int csc_cfg_access_setup(KEYCB *cfg, KEYCB *dkcb, KEYCB *nkcb)
{
	struct	KEYROOT	*rext;

	if (cfg == NULL) {
		return SMM_ERR_OBJECT;
	}
	if (CFGF_TYPE_GET(cfg->flags) != CFGF_TYPE_ROOT) {
		return SMM_ERR_OBJECT;
	}

	rext = (struct KEYROOT *)cfg->pool;
	rext->dkcb = dkcb;
	rext->nkcb = nkcb;
	return SMM_ERR_NONE;
}

static KEYCB *csc_cfg_access_update(KEYCB *cfg, int type)
{
	struct	KEYROOT	*rext;
	KEYCB	*kcb;
	CSCLNK	*mp;

	if (cfg == NULL) {
		return NULL;
	}
	if (CFGF_TYPE_GET(cfg->flags) != CFGF_TYPE_ROOT) {
		return NULL;
	}

	rext = (struct KEYROOT *)cfg->pool;
	if (rext->dkcb == NULL) {	/* default is the root key */
		rext->dkcb = cfg;
	}
	if (rext->nkcb == NULL) {
		mp = rext->dkcb->anchor;
	} else {
		mp = csc_cdl_next(rext->dkcb->anchor, rext->nkcb->self);
	}
	while (mp) {
		if ((kcb = CFGF_GETOBJ(mp)) == NULL) {
			break;
		}
		if (CFGF_TYPE_GET(kcb->flags) == type) {
			rext->nkcb = kcb;
			return kcb;
		}
		mp = csc_cdl_next(rext->dkcb->anchor, mp);
	}
	return NULL;
}

static int csc_cfg_destroy_links(CSCLNK *anchor)
{
	KEYCB	*ckey;
	CSCLNK	*p;

	for (p = anchor; p; p = csc_cdl_next(anchor, p)) {
		if ((ckey = CFGF_GETOBJ(p)) == NULL) {
			break;
		}
		if (ckey->anchor) {
			csc_cfg_destroy_links(ckey->anchor);
		}
	}
	csc_cdl_list_destroy(&anchor);
	return SMM_ERR_NONE;
}

static int csc_cfg_save_links(struct KeyDev *cfgd, CSCLNK *anchor)
{
	KEYCB	*dkcb;
	CSCLNK	*mp;
	int	count = 0;

	/* output the contents in the current directory */
	for (mp = anchor; mp; mp = csc_cdl_next(anchor, mp)) {
		if ((dkcb = CFGF_GETOBJ(mp)) == NULL) {
			break;
		}
		if (CFGF_TYPE_GET(dkcb->flags) != CFGF_TYPE_DIR) {
			smm_config_write(cfgd, dkcb);
			if (CFGF_TYPE_GET(dkcb->flags) == CFGF_TYPE_KEY) {
				count++;
			}
		}
	}
	
	/* recursively walk through its sub-directories */
	for (mp = anchor; mp; mp = csc_cdl_next(anchor, mp)) {
		if ((dkcb = CFGF_GETOBJ(mp)) == NULL) {
			break;
		}
		if (CFGF_TYPE_GET(dkcb->flags) != CFGF_TYPE_DIR) {
			continue;
		}
		if (dkcb->anchor == NULL) {
			continue;
		}

		/* find how many keys in the current directory 
		 * and eliminate the empty directories */
		if (csc_cfg_attribute(dkcb, NULL)) {
			smm_config_write(cfgd, dkcb);
			count++;
		}

		count += csc_cfg_save_links(cfgd, dkcb->anchor);
	}
	return count;
}

/* attr can be NULL, otherwise it should be at least CFGF_TYPE_MASK+1 long */
static int csc_cfg_attribute(KEYCB *kcb, int *attr)
{
	CSCLNK	*cp;
	KEYCB	*ccb;
	int	keyno, accnt[CFGF_TYPE_MASK+1];

	keyno = 0;
	if (attr == NULL) {
		attr = accnt;
	}
	memset(attr, 0, (CFGF_TYPE_MASK+1)*sizeof(int));
	
	for (cp = kcb->anchor; cp; cp = csc_cdl_next(kcb->anchor, cp)) {
		if ((ccb = CFGF_GETOBJ(cp)) == NULL) {
			break;
		}
		attr[CFGF_TYPE_GET(ccb->flags)]++;
		switch (CFGF_TYPE_GET(ccb->flags)) {
		case CFGF_TYPE_KEY:
		case CFGF_TYPE_PART:
		case CFGF_TYPE_VALUE:
		case CFGF_TYPE_COMM:
			keyno++;
			break;
		}
	}
	attr[CFGF_TYPE_GET(kcb->flags)]++;;
	switch (CFGF_TYPE_GET(kcb->flags)) {
	case CFGF_TYPE_KEY:
	case CFGF_TYPE_PART:
	case CFGF_TYPE_VALUE:
	case CFGF_TYPE_COMM:
		keyno++;
		break;
	default:	/* empty block is acceptable */
		if (csc_cfg_block_size(kcb) >= 0) {
			keyno++;
		}
	}
	return keyno;
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

static char *csc_cfg_format_directory(char *dkey)
{
	char	*dbuf, *drtn;

	dbuf = drtn = dkey;
	while (*dkey == '/') dkey++;	/* skip the leading '/' */
	while (*dkey) {
		while (*dkey && (*dkey != '/')) {
			*dbuf++ = *dkey++;
		}
		while (*dkey == '/') {
			dkey++;
		}
		if (*dkey) {
			*dbuf++ = '/';
		}
	}
	*dbuf++ = 0;
	CDB_PROG(("csc_cfg_format_directory: %s\n", drtn));
	return drtn;
}

static char *csc_cfg_format_dir_alloc(char *dkey)
{
	char	*dbuf;

	if ((dbuf = csc_strcpy_alloc(dkey, 0)) != NULL) {
		csc_cfg_format_directory(dbuf);
	}
	return dbuf;
}


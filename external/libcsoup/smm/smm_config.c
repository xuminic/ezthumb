
/*  smm_config.c - configure parameters management 

    Copyright (C) 2011  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of LIBSMM, System Masquerade Module library

    LIBSMM is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LIBSMM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define CSOUP_DEBUG_LOCAL	SLOG_CWORD(CSOUP_MOD_CONFIG, SLOG_LVL_ERROR)
//#define CSOUP_DEBUG_LOCAL	SLOG_CWORD(CSOUP_MOD_CONFIG, SLOG_LVL_MODULE)

#include "libcsoup.h"
#include "csoup_internal.h"


/* How to find out the input/output device.
 * Input:
 *   *) Win32 Registry: hRootKey != NULL
 *   *) File system: fp != NULL
 *   *) Memory cache: fname == NULL && fpath == configure (kpath = counter)
 *   *) stdin: fname == NULL && fpath == NULL
 * Input priority: Registry > File > Memory > stdio
 * Output:
 *   *) Win32 Registry: hRootKey != NULL
 *   *) File system: fp != NULL
 *   *) Memory cache: fname == NULL && fpath == configure && mode > 0
 *   *) stdout: fname == NULL && (mode == 0 || fpath == NULL)
 */
struct	KeyDev	{
	FILE	*fp;
	int	mode;
	char	*fpath;		/* full path for the file system */
	char	*kpath;		/* key path for the Win32 registry */
	char	*fname;
#ifdef	CFG_WIN32_API
	HKEY	hSysKey;	/* predefined key like HKEY_CURRENT_USER */
	HKEY	hRootKey;	/* root key points to the entrance */
	HKEY	hSaveKey;	/* used to save contents */

	KEYCB	*kbin;		/* for saving binary/block */

	int	idx;
	struct	{
		HKEY	hKey;
		DWORD	n_keys;
		DWORD	n_vals;
		DWORD	l_key;
		DWORD	l_vname;
		DWORD	l_value;
		int	i_val;
		int	i_key;
	} reg[CFGF_MAX_DEPTH];
#endif
	char	pool[1];
};


static struct KeyDev *smm_config_alloc(int sysdir, char *path, char *fname);
static KEYCB *smm_config_mem_read(struct KeyDev *cfgd);
static int smm_config_mem_write(struct KeyDev *cfgd, KEYCB *kp);
static KEYCB *smm_config_file_read(struct KeyDev *cfgd);
static int smm_config_file_write(struct KeyDev *cfgd, KEYCB *kp);
static int mem_copy(char **dest, int *room, char *s);

#ifdef	CFG_WIN32_API
static int smc_reg_open(struct KeyDev *cfgd, int mode);
static KEYCB *smc_reg_read(struct KeyDev *cfgd);
static int smc_reg_write(struct KeyDev *cfgd, KEYCB *kp);
static int smc_reg_delete(struct KeyDev *cfgd, char *fname);
static int smc_reg_binary_close(struct KeyDev *cfgd);
extern int csc_cfg_binary_to_hex(char *src, int slen, char *buf, int blen);
#endif


/*!\brief Open the input/output device for the configuration manager

   \param[in]  sysdir The identity of system directory. In Win32, the system
   directory normally points to the registry like HKEY_CURRENT_USER\\SOFTWARE.
   In Posix, the configure file is normally stored in the file system like
   $HOME/.config. However it is also possible for Win32 to store the configure
   file like Posix. Current identities of system directory are:

   SMM_CFGROOT_DESKTOP: $HOME/.config in Posix, or HKEY_CURRENT_USER\\SOFTWARE
                        in Windows.
   SMM_CFGROOT_USER: $HOME in Posix, or HKEY_CURRENT_USER\\CONSOLE in Windows.
   SMM_CFGROOT_SYSTEM: /etc in Posix, or HKEY_LOCAL_MACHINE\\SOFTWARE
                       in Windows.
   SMM_CFGROOT_CURRENT: starting from the current directory.
   SMM_CFGROOT_MEMPOOL: the contents of the configuration is stored in the 
         memory buffer pointed by 'path'. 'fname' will be ignored.
	 If 'mode' is 0, the configuration memory buffer is read only and 
	 the output will be written to the stdout. If 'mode' is greater 
	 than 0, which means the size of the configuration memory buffer,
	 the output will be written back to the memory buffer until it's full.
                      
   \param[in]  path Specify the path to the configure file in the file system.
        It can also be the path in Registry of Windows pointing to the parent
	key before the configure key. In SMM_CFGROOT_MEMPOOL mode, the 'path'
	points to the memory buffer where the content of configure holded.

   \param[in]  fname Specify the name of the configure in the file system.
        In Registry of Windows, it's the name of the key to the configures.
	In SMM_CFGROOT_MEMPOOL mode, it should be NULL.

   \param[in]  mode The access mode to the registry or to the file. It can be
        CSC_CFG_READ, which means read only, CSC_CFG_RDWR, which means read
        and write, or CSC_CFG_RWC, which means read/write/create. Note that
	the smm_config module doesn't support random access so the write 
	operation will always over write the previous contents.
	In SMM_CFGROOT_MEMPOOL mode, the 'mode' can be 0, whicn means read 
	only, or be the size of the memory buffer. If the 'mode' specifies
	the size of the memory buffer, the access mode is always be read and
	write.

   \return A pointer to the 'KeyDev' structure if succeed, or NULL if failed.
*/
struct KeyDev *smm_config_open(int sysdir, char *path, char *fname, int mode)
{
	struct	KeyDev	*cfgd;
	int	i, ctmp;

	/* configure in memory mode.
	 * Note that in memory mode, the 'path' parameter points to the 
	 * contents of the configure and the 'mode' parameter stores the size
	 * of the memory of the configure. In memory mode, the access mode
	 * is always read/write/create. */
	if (sysdir == SMM_CFGROOT_MEMPOOL) {
		if ((cfgd = smm_alloc(sizeof(struct KeyDev))) != NULL) {
			cfgd->fpath = path;
			cfgd->kpath = path;	/* reset the runtime index */
			cfgd->mode  = mode;
		}
		return cfgd;
	}

	if ((cfgd = smm_config_alloc(sysdir, path, fname)) == NULL) {
		return NULL;
	}
	cfgd->mode = mode;
	
	/* debug mode 0xdeadbeef.
	 * In this mode, the function will return by assuming the output
	 * device is available but empty without verify the descriptor */
	if (mode == (int) 0xdeadbeef) {
		return cfgd;
	}

#ifdef	CFG_WIN32_API
	if (smc_reg_open(cfgd, mode) == SMM_ERR_NONE) {
		return cfgd;
	}
#endif
	switch (CFGF_MODE_GET(mode)) {
	case CSC_CFG_READ:
		cfgd->fp = fopen(cfgd->fpath, "r");
		break;
	case CSC_CFG_RWC:
		for (i = strlen(cfgd->fpath); i >= 0; i--) {
			if (csc_isdelim(SMM_PATH_DELIM, cfgd->fpath[i])) {
				ctmp = cfgd->fpath[i];
				cfgd->fpath[i] = 0;
				smm_mkpath(cfgd->fpath);
				cfgd->fpath[i] = (char)ctmp;
				break;
			}
		}
		if ((cfgd->fp = fopen(cfgd->fpath, "r+")) == NULL) {
			cfgd->fp = fopen(cfgd->fpath, "w+");
		}
		break;
	case CSC_CFG_RDWR:
		cfgd->fp = fopen(cfgd->fpath, "r+");
		break;
	}
	if (cfgd->fp == NULL) {
		smm_free(cfgd);
		cfgd = NULL;
	}
	return cfgd;
}

int smm_config_close(struct KeyDev *cfgd)
{
#ifdef	CFG_WIN32_API
	if (cfgd->hRootKey) {
		/* save, close and clean the binary block */
		if (cfgd->kbin) {
			smc_reg_binary_close(cfgd);
		}
		RegFlushKey(cfgd->hRootKey);
		RegCloseKey(cfgd->hRootKey);
	}
#endif
	if (cfgd->fp) {
		fclose(cfgd->fp);
	}
	return smm_free(cfgd);
}

KEYCB *smm_config_read_alloc(struct KeyDev *cfgd)
{
#ifdef	CFG_WIN32_API
	if (cfgd->hRootKey) {
		return smc_reg_read(cfgd);
	}
#endif
	if (cfgd->fp) {
		return smm_config_file_read(cfgd);
	}
	if (cfgd->fname == NULL) {
		if (cfgd->fpath) {
			return smm_config_mem_read(cfgd);
		}
		/* stdio has not implemented */
	}
	return NULL;
}

int smm_config_write(struct KeyDev *cfgd, KEYCB *kp)
{
#ifdef	CFG_WIN32_API
	if (cfgd->hRootKey) {
		if (CFGF_MODE_GET(cfgd->mode) != CSC_CFG_READ) {
			return smc_reg_write(cfgd, kp);
		}
	}
#endif
	if (cfgd->fname == NULL) {	/* memory mode or stdout */
		if (cfgd->fpath && cfgd->mode) {
			return smm_config_mem_write(cfgd, kp);
		} else {
			return smm_config_file_write(cfgd, kp);
		}
	}
	if (CFGF_MODE_GET(cfgd->mode) == CSC_CFG_READ) {
		return smm_errno_update(SMM_ERR_ACCESS);
	}
	return smm_config_file_write(cfgd, kp);
}

int smm_config_delete(int sysdir, char *path, char *fname)
{
	struct	KeyDev	*cfgd;
	int	rc;

	if (sysdir == SMM_CFGROOT_MEMPOOL) {
		return smm_errno_update(SMM_ERR_NULL);
	}
	if ((cfgd = smm_config_alloc(sysdir, path, fname)) == NULL) {
		return smm_errno_update(SMM_ERR_NULL);
	}
	
#ifdef	CFG_WIN32_API
	if (cfgd->kpath) {
		rc = smc_reg_delete(cfgd, fname);
	} else {
		rc = unlink(cfgd->fpath);
	}
#else
	rc = unlink(cfgd->fpath);
#endif
	smm_free(cfgd);
	if (rc == 0) {
		return smm_errno_update(SMM_ERR_NONE);
	}
	if (errno == EACCES) {
		return smm_errno_update(SMM_ERR_ACCESS);
	}
	return smm_errno_update(SMM_ERR_NULL);
}

int smm_config_current(struct KeyDev *cfgd, char *buf, int blen)
{
	int	n = 0;

#ifdef	CFG_WIN32_API
	if (cfgd->hRootKey) {	/* I/O to the registry */
		char	*path;
		path = cfgd->kpath + strlen(cfgd->kpath) + 1;
		n += mem_copy(&buf, &blen, path);
		n += mem_copy(&buf, &blen, "\\");
		n += mem_copy(&buf, &blen, cfgd->kpath);
		n += mem_copy(&buf, &blen, "\\");
		n += mem_copy(&buf, &blen, cfgd->fname);
		return n;
	}
#endif
	if (cfgd->fp) {		/* I/O to the file system */
		n += mem_copy(&buf, &blen, cfgd->fpath);
		return n;
	}
	if (cfgd->fname == NULL) {
		char	path[128];
		if (cfgd->fpath == NULL) {	/* I/O to the stdin/stdout */
			sprintf(path, "con://stdio");
		} else if (cfgd->mode == 0) {	/* I/O to the read only mem */
			sprintf(path, "mem://%p/%d/read-only", cfgd->fpath, 
					(int)(strlen(cfgd->fpath) + 1));
		} else {			/* I/O to the memory */
			sprintf(path, "mem://%p/%d/read-write", 
					cfgd->fpath, cfgd->mode);
		}
		n += mem_copy(&buf, &blen, path);
		return n;
	}
	return -1;
}

/* return:
 *   'buf' a string list of file path, registry path and registry root. 
 *   Each string ends of \0 and the whole list ends of another \0. 
 *   For example:
 *   "/etc/posix/rcfile\0SOFTWARE\\posix\0HKEY_LOCAL_MACHINE\0\0"
 * The function returns the total size of the string list, includes all \0 */
int smm_config_path(int sysdir, char *path, char *fname, char *buf, int blen)
{
	char	*home;
	int	len = 0;

	home = getenv("HOME");
	switch (sysdir) {
	case SMM_CFGROOT_USER:
		/* SMM_DEF_DELIM causes nasty looking in Windows like:
		 * C:/MinGW/msys/1.0/home/User\.config\7-Zip
		 * so better to keep this a straight "/". */
		if (home) {
			len += mem_copy(&buf, &blen, home);
			len += mem_copy(&buf, &blen, "/");
		}
		if (path) {
			len += mem_copy(&buf, &blen, path);
			len += mem_copy(&buf, &blen, "/");
		}
		len += mem_copy(&buf, &blen, fname);
		len += mem_copy(&buf, &blen, NULL);	/* insert \0 */

		len += mem_copy(&buf, &blen, "CONSOLE/");
		if (path) {
			len += mem_copy(&buf, &blen, path);
		}
		len += mem_copy(&buf, &blen, NULL);	/* insert \0 */

		len += mem_copy(&buf, &blen, "HKEY_CURRENT_USER");
		len += mem_copy(&buf, &blen, NULL);  /* insert \0 */
		break;
	case SMM_CFGROOT_SYSTEM:
		len += mem_copy(&buf, &blen, "/etc/");	//FIXME
		if (path) {
			len += mem_copy(&buf, &blen, path);
			len += mem_copy(&buf, &blen, "/");
		}
		len += mem_copy(&buf, &blen, fname);
		len += mem_copy(&buf, &blen, NULL);	/* insert \0 */

		len += mem_copy(&buf, &blen, "SOFTWARE/");
		if (path) {
			len += mem_copy(&buf, &blen, path);
		}
		len += mem_copy(&buf, &blen, NULL);	/* insert \0 */

		len += mem_copy(&buf, &blen, "HKEY_LOCAL_MACHINE");
		len += mem_copy(&buf, &blen, NULL);  /* insert \0 */
		break;
	case SMM_CFGROOT_DESKTOP:
		if (home) {
			len += mem_copy(&buf, &blen, home);
			len += mem_copy(&buf, &blen, "/");
		}
		len += mem_copy(&buf, &blen, ".config/");
		if (path) {
			len += mem_copy(&buf, &blen, path);
			len += mem_copy(&buf, &blen, "/");
		}
		len += mem_copy(&buf, &blen, fname);
		len += mem_copy(&buf, &blen, NULL);	/* insert \0 */

		len += mem_copy(&buf, &blen, "SOFTWARE/");
		if (path) {
			len += mem_copy(&buf, &blen, path);
		}
		len += mem_copy(&buf, &blen, NULL);	/* insert \0 */

		len += mem_copy(&buf, &blen, "HKEY_CURRENT_USER");
		len += mem_copy(&buf, &blen, NULL);  /* insert \0 */
		break;
	case SMM_CFGROOT_CURRENT:
	default:
		if (path) {
			len += mem_copy(&buf, &blen, path);
			len += mem_copy(&buf, &blen, "/");
		}
		len += mem_copy(&buf, &blen, fname);
		len += mem_copy(&buf, &blen, NULL);	/* insert \0 */
		len += mem_copy(&buf, &blen, NULL);	/* insert \0 */
		len += mem_copy(&buf, &blen, NULL);	/* insert \0 */
		break;
	}
	len += mem_copy(&buf, &blen, NULL);  /* terminate the string list */
	return len;
}

void smm_config_dump(struct KeyDev *cfgd)
{
	CDB_SHOW(("Device:    Read from "));
#ifdef	CFG_WIN32_API
	if (cfgd->hRootKey) {
		CDB_SHOW(("Registry"));
	} else
#endif
	if (cfgd->fp) {
		CDB_SHOW(("%s", cfgd->fname));
	} else if (cfgd->fname == NULL) {
		if (cfgd->fpath) {
			CDB_SHOW(("%p", cfgd->fpath));
		} else {
			CDB_SHOW(("stdin"));
		}
	} else {
		CDB_SHOW(("*%s", cfgd->fname));
	}
	CDB_SHOW((". Write to "));
#ifdef	CFG_WIN32_API
	if (cfgd->hRootKey) {
		CDB_SHOW(("(Registry)"));
	}
#endif
	if (cfgd->fp) {
		CDB_SHOW(("(%s)", cfgd->fname));
	}
	if (cfgd->fname == NULL) {
		if (cfgd->fpath == NULL) {
			CDB_SHOW(("(*stdout)"));
		} else if (cfgd->mode) {
			CDB_SHOW(("(%p+%d)", cfgd->fpath, cfgd->mode));
		} else {
			CDB_SHOW(("(stdout)"));
		}
	} else {
		CDB_SHOW(("(*%s)", cfgd->fname));
	}
	CDB_SHOW(("\n"));

	if (!cfgd->fname && cfgd->fpath) {
		CDB_SHOW(("Memory:    %p %p\n", cfgd->fpath, cfgd->kpath));
	} else {
		CDB_SHOW(("Full Path: %s\n", cfgd->fpath));
		CDB_SHOW(("Reg Path:  %s\n", cfgd->kpath));
	}
	CDB_SHOW(("\n"));
}


static struct KeyDev *smm_config_alloc(int sysdir, char *path, char *fname)
{
	struct	KeyDev	*cfgd;
	int	tlen;

	if (fname == NULL) {
		return NULL;
	}

	CDB_MODL(("smm_config_alloc: %s %s\n", path, fname));
	tlen = sizeof(struct KeyDev) + strlen(fname) + 16;
	tlen += path ? strlen(path) : 0;
	tlen += smm_config_path(sysdir, path, fname, NULL, 0);

	if ((cfgd = smm_alloc(tlen)) == NULL) {
		smm_errno_update(SMM_ERR_LOWMEM);
		return NULL;
	}

	cfgd->fname = cfgd->pool;
	strcpy(cfgd->fname, fname);
	cfgd->fpath = cfgd->fname + strlen(cfgd->fname) + 1;
	tlen -= sizeof(struct KeyDev) + strlen(cfgd->fname) + 1;
	smm_config_path(sysdir, path, fname, cfgd->fpath, tlen);
	
	cfgd->kpath = cfgd->fpath + strlen(cfgd->fpath) + 1;
#ifdef	CFG_WIN32_API
	fname = cfgd->kpath + strlen(cfgd->kpath) + 1;
	if (!strcmp(fname, "HKEY_CURRENT_USER")) {
		cfgd->hSysKey = HKEY_CURRENT_USER;
	} else if (!strcmp(fname, "HKEY_LOCAL_MACHINE")) {
		cfgd->hSysKey = HKEY_LOCAL_MACHINE;
	}
#endif
	/* memory mode will re-use the kpath field so we clear the pointer
	 * when it's not been using */
	if (*cfgd->kpath == '\0') {
		cfgd->kpath = NULL;
	}

	/* check point */
	CDB_INFO(("smm_config_alloc::fpath = %s\n", cfgd->fpath));
	CDB_INFO(("smm_config_alloc::kpath = %s\n", cfgd->kpath));
	CDB_INFO(("smm_config_alloc::fname = %s\n", cfgd->fname));
	return cfgd;
}

static KEYCB *smm_config_mem_read(struct KeyDev *cfgd)
{
	KEYCB	*kp;
	int	n;

	for (n = 0; cfgd->kpath[n]; n++) {
		if (cfgd->kpath[n] == '\n') {
			n++;
			break;
		}
	}

	if ((n == 0) || ((kp = csc_cfg_kcb_alloc(n+1)) == NULL)) {
		return NULL;
	}
	memcpy(kp->pool, cfgd->kpath, n);
	kp->pool[n] = 0;
	cfgd->kpath += n;
	return kp;
}

static int smm_config_mem_write(struct KeyDev *cfgd, KEYCB *kp)
{
	int	wtd = 0;

	if (kp == NULL) {
		return 0;
	}
	if (kp->key) {
		if (CFGF_TYPE_GET(kp->flags) == CFGF_TYPE_DIR) {
			wtd += mem_copy(&cfgd->kpath, &cfgd->mode, "[");
			wtd += mem_copy(&cfgd->kpath, &cfgd->mode, kp->key);
			wtd += mem_copy(&cfgd->kpath, &cfgd->mode, "]");
		} else {
			wtd += mem_copy(&cfgd->kpath, &cfgd->mode, kp->key);
		}
	}
	if (kp->value && *kp->value) {
		wtd += mem_copy(&cfgd->kpath, &cfgd->mode, "=");
		wtd += mem_copy(&cfgd->kpath, &cfgd->mode, kp->value);
	}
	if (kp->comment && *kp->comment) {
		wtd += mem_copy(&cfgd->kpath, &cfgd->mode, "\t");
		wtd += mem_copy(&cfgd->kpath, &cfgd->mode, kp->comment);
	}
	wtd += mem_copy(&cfgd->kpath, &cfgd->mode, "\n");
	kp->update = 0;		/* reset the update counter */
	return wtd;
}

static KEYCB *smm_config_file_read(struct KeyDev *cfgd)
{
	KEYCB	*kp;
	int	amnt, cpos, ch;

	if (cfgd->fp == NULL) {
		return NULL;
	}

	amnt = 0;
	cpos = ftell(cfgd->fp);
	while ((ch = fgetc(cfgd->fp)) != EOF) {
		amnt++;
		if (ch == '\n') {
			break;
		}
	}

	if ((amnt == 0) || ((kp = csc_cfg_kcb_alloc(amnt+1)) == NULL)) {
		return NULL;
	}

	/* rewind to the start position */
	fseek(cfgd->fp, cpos, SEEK_SET);
	amnt = 0;
	while ((ch = fgetc(cfgd->fp)) != EOF) {
		kp->pool[amnt++] = (char) ch;
		if (ch == '\n') {
			break;
		}
	}
	kp->pool[amnt] = 0;
	return kp;
}

static int smm_config_file_write(struct KeyDev *cfgd, KEYCB *kp)
{
	FILE	*fout;

	if (kp == NULL) {
		return 0;
	}
	if ((fout = cfgd->fp) == NULL) {
		fout = stdout;
	}

	kp->update = 0;		/* reset the update counter */
	if (kp->key) {
		if (CFGF_TYPE_GET(kp->flags) == CFGF_TYPE_DIR) {
			fputc('[', fout);
			fputs(kp->key, fout);
			fputc(']', fout);
		} else {
			fputs(kp->key, fout);
		}
	}
	if (kp->value && *kp->value) {
		fputc('=', fout);
		fputs(kp->value, fout);
	}
	if (kp->comment && *kp->comment) {
		fputc('\t', fout);
		fputs(kp->comment, fout);
	}
	fputs("\n", fout);
	return 0;
}

/* Concatenate a string into a memory pool which is specified by 'dest' with 
 * the size of 'room'. After the appending, the 'dest' will be increased and 
 * the 'room' will be decreased by the length of 's'. The memory pool will 
 * always be ended by \0 so if there's not enough memory in 'room', the string
 * will be truncated and the function will return the truly size of copied 
 * string. If 's' is NULL, the function will append a '\0' into the 'dest'.
 * Returns:
 * If the '*dest' is not NULL, it will return the actual lenght of copied
 * string. Otherwise it returns the length of the source string 's'. */
static int mem_copy(char **dest, int *room, char *s)
{
	int	n;

	if (s == NULL) {
		n = 1;		/* extra '\0' */
	} else {
		n = strlen(s);
	}
	if (*dest == NULL) {	/* virtual operation */
		return n;
	}

	if ((room == NULL) ||	/* don't care the buffer length */
			(*room > n)) {
		if (s) {
			strcpy(*dest, s);
		} else {
			**dest = '\0';
		}
	} else if ((n = *room - 1) > 0) {
		if (s) {
			strncpy(*dest, s, n);
		}
	}
	*dest += n;
	if (room) {
		*room -= n;
	}
	**dest = '\0';
	return n;
}



#ifdef	CFG_WIN32_API

struct	RegBuf	{
	DWORD	n_keys;		/* number of of keys (directory) */
	DWORD	n_vals;		/* number of values (key/value pairs) */
	TCHAR	*name;		/* to the buffer for name of values/keys */
	DWORD	nm_len;		/* length of the name buffer in unicode */
	TCHAR	*type;		/* to the buffer for value type */
	DWORD	ty_len;		/* length of the type buffer in unicode */
	DWORD	ty_id;
	void	*content;	/* to the buffer for contents of values */
	DWORD	co_len;		/* length of the content buffer in bytes */
	char	pool[1];
};

#define RREF(d)		((d)->reg[(d)->idx])

static int smc_reg_eof(struct KeyDev *cfgd);
static DWORD smc_reg_load_info(struct KeyDev *cfgd, HKEY hKey);
static DWORD smc_reg_open_subkey(struct KeyDev *cfgd);
static KEYCB *smc_reg_key_alloc(struct KeyDev *cfgd, int idx);
static KEYCB *smc_reg_keyval_alloc(TCHAR *key, int vlen);
static KEYCB *smc_reg_block_alloc(struct KeyDev *cfgd, TCHAR *, void *, int);
static KEYCB *smc_reg_path_alloc(struct KeyDev *cfgd, TCHAR *bkey);
static int smc_reg_getcwd(struct KeyDev *cfgd, TCHAR *path, int n);
static int smc_reg_binary_open(struct KeyDev *cfgd, KEYCB *kp);
static int smc_reg_binary_update(struct KeyDev *cfgd, KEYCB *kp);
static HKEY regy_open_dir(HKEY hRoot, char *dkey);
static DWORD regy_puts(HKEY hKey, TCHAR *key, DWORD dwType, char *val);
static BOOL RegDelnodeRecurse(HKEY hKeyRoot, LPTSTR lpSubKey, int buflen);


static int str_substitue_char(char *s, int len, char src, char dst)
{
	int	i, n = 0;

	if (len < 0) {
		len = strlen(s);
	}
	for (i = n = 0; i < len; i++) {
		if (s[i] == src) {
			s[i] = dst;
			n++;
		}
	}
	return n;
}

static int smc_reg_open(struct KeyDev *cfgd, int mode)
{
	TCHAR	*wkey;
	char	*mkey;	/* key of main entry */

	if (cfgd->kpath == NULL) {
		return SMM_ERR_NULL;
	}
	mkey = csc_strcpy_alloc(cfgd->kpath, strlen(cfgd->fname) + 4);
	if (mkey == NULL) {
		return SMM_ERR_LOWMEM;
	}
	strcat(mkey, "\\");
	strcat(mkey, cfgd->fname);
	str_substitue_char(mkey, -1, '/', '\\');

	CDB_INFO(("smc_reg_open: %s\n", mkey));
	if ((wkey = smm_mbstowcs_alloc(mkey)) == NULL) {
		smm_free(mkey);
		return SMM_ERR_LOWMEM;
	}

	switch (CFGF_MODE_GET(mode)) {
	case CSC_CFG_READ:
		RegOpenKeyExW(cfgd->hSysKey, wkey, 0, KEY_READ, 
				&cfgd->hRootKey);
		break;
	case CSC_CFG_RWC:
		/* The good thing of RegCreateKeyEx() is that it can create 
		 * a string of subkeys without creating one by one. 
		 * For example: A\\B\\C */
		RegCreateKeyEx(cfgd->hSysKey, wkey, 0, NULL, 0, 
				KEY_ALL_ACCESS, NULL, &cfgd->hRootKey, NULL);
		break;
	case CSC_CFG_RDWR:
		RegOpenKeyEx(cfgd->hSysKey, wkey, 0, KEY_ALL_ACCESS, 
				&cfgd->hRootKey);
		break;
	}

	smm_free(wkey);			
	smm_free(mkey);

	if (cfgd->hRootKey == NULL) {
		return SMM_ERR_ACCESS;
	}
	/* load the reg[0] with the root key */
	smc_reg_load_info(cfgd, cfgd->hRootKey);
	/* initialize the key for saving */
	cfgd->hSaveKey = cfgd->hRootKey;
	return SMM_ERR_NONE;
}


/* Note that the reg[0] must be loaded with root key prior to this call. */
static KEYCB *smc_reg_read(struct KeyDev *cfgd)
{
	while (!smc_reg_eof(cfgd)) {
		CDB_FUNC(("smc_reg_read: %d I(%d/%u) V(%d/%u)\n", 
				cfgd->idx, RREF(cfgd).i_key, RREF(cfgd).n_keys,
				RREF(cfgd).i_val, RREF(cfgd).n_vals));
		if (RREF(cfgd).i_val < (int)RREF(cfgd).n_vals) {
			return smc_reg_key_alloc(cfgd, RREF(cfgd).i_val++);
		}

		if (RREF(cfgd).i_key == (int)RREF(cfgd).n_keys) {
			if (cfgd->idx) {
				RegCloseKey(RREF(cfgd).hKey);
				RREF(cfgd).hKey = NULL;
				RREF(cfgd).n_keys = RREF(cfgd).n_vals = 0;
				RREF(cfgd).i_key = RREF(cfgd).i_val = 0;
				cfgd->idx--;
			}
			continue;
		}

		/* move to the subkey; cfgd->idx mostly increased here */
		if (smc_reg_open_subkey(cfgd) == ERROR_SUCCESS) {
			/* the first time entering the current path */
			if (RREF(cfgd).n_vals) {
				return smc_reg_path_alloc(cfgd, NULL);
			}
		}
	}
	return NULL;
}


static int smc_reg_write(struct KeyDev *cfgd, KEYCB *kp)
{
	TCHAR	*wpath;
	DWORD	dwErr;
	
	if (CFGF_TYPE_GET(kp->flags) == CFGF_TYPE_DIR) {
		/* save, close and clean the binary block */
		if (cfgd->kbin) {	
			smc_reg_binary_close(cfgd);
		}
		/* process the binary/block data and open another cfgd->kbin.
	 	* note that the 'vsize' field must be set in the KEYCB */
		if (csc_cfg_block_size(kp) >= 0) {
			return smc_reg_binary_open(cfgd, kp);
		}
		if (cfgd->hSaveKey && (cfgd->hSaveKey != cfgd->hRootKey)) {
			RegCloseKey(cfgd->hSaveKey);
		}
		cfgd->hSaveKey = regy_open_dir(cfgd->hRootKey, kp->key);
		if (cfgd->hSaveKey == NULL) {
			return SMM_ERR_ACCESS;
		}
		return SMM_ERR_NONE;
	}
	if (cfgd->hSaveKey == NULL) {
		return SMM_ERR_ACCESS;
	}

	if (cfgd->kbin) {
		return smc_reg_binary_update(cfgd, kp);
	}

	/* save the normal key/value pair 
	 * note that the binary savings are just dummies. */
	if ((wpath = smm_mbstowcs_alloc(kp->key)) == NULL) {
		return SMM_ERR_LOWMEM;
	}
	/* remove the unnamed key name */
	if (!strcmp(kp->key, "(default)")) {
		wpath[0] = TEXT('\0');
	}
	if (kp->comment == NULL) {
		dwErr = regy_puts(cfgd->hSaveKey, wpath, 
				REG_SZ, kp->value);
	} else if (!strcmp(kp->comment, "##REG_BINARY")) {
		dwErr = RegSetValueEx(cfgd->hSaveKey, wpath, 0, 
				REG_BINARY, (void*)kp->value, kp->vsize);
	} else if (!strcmp(kp->comment, "##REG_DWORD")) {
		DWORD	dwData = (DWORD) strtol(kp->value, NULL, 0);
		dwErr = RegSetValueEx(cfgd->hSaveKey, wpath, 0,
				REG_DWORD, (void*)&dwData, sizeof(dwData));
	} else if (!strcmp(kp->comment, "##REG_DWORD_BIG_ENDIAN")) {
		DWORD	dwData = (DWORD) strtol(kp->value, NULL, 0);
		dwErr = RegSetValueEx(cfgd->hSaveKey, wpath, 0,
			REG_DWORD_BIG_ENDIAN, (void*)&dwData, sizeof(dwData));
	} else if (!strcmp(kp->comment, "##REG_EXPAND_SZ")) {
		dwErr = regy_puts(cfgd->hSaveKey, wpath, 
				REG_EXPAND_SZ, kp->value);
	} else if (!strcmp(kp->comment, "##REG_LINK")) {
		dwErr = regy_puts(cfgd->hSaveKey, wpath, 
				REG_LINK, kp->value);
	} else if (!strcmp(kp->comment, "##REG_MULTI_SZ")) {
		dwErr = regy_puts(cfgd->hSaveKey, wpath, 
				REG_MULTI_SZ, kp->value);
	} else if (!strcmp(kp->comment, "##REG_NONE") ||
			!strcmp(kp->comment, "##REG_UNKNOWN")) {
		dwErr = RegSetValueEx(cfgd->hSaveKey, wpath, 0, 
				REG_NONE, (void*)kp->value, kp->vsize);
	} else if (!strcmp(kp->comment, "##REG_QWORD")) {
		DWORD64	dqData = (DWORD64) strtoll(kp->value, NULL, 0);
		dwErr = RegSetValueEx(cfgd->hSaveKey, wpath, 0,
				REG_QWORD, (void*)&dqData, sizeof(dqData));
	} else {
		dwErr = regy_puts(cfgd->hSaveKey, wpath, 
				REG_SZ, kp->value);
	}
	smm_free(wpath);
	return dwErr;
}

static int smc_reg_delete(struct KeyDev *cfgd, char *fname)
{
	HKEY	hPathKey;
	TCHAR	*wkey;
	LONG	rcode;

	if ((wkey = smm_mbstowcs_alloc(cfgd->kpath)) == NULL) {
		return SMM_ERR_LOWMEM;
	}
	if (RegCreateKeyEx(cfgd->hSysKey, wkey, 0, NULL, 0, KEY_ALL_ACCESS, 
				NULL, &hPathKey, NULL) != ERROR_SUCCESS) {
		smm_free(wkey);
		errno = EACCES;
		return SMM_ERR_ACCESS;
	}
	smm_free(wkey);

	/* fabricate the key name */
	if ((wkey = smm_alloc(MAX_PATH * 2 * sizeof(TCHAR))) == NULL) {
		RegCloseKey(hPathKey);
		return SMM_ERR_LOWMEM;
	}
	MultiByteToWideChar(smm_codepage(), 0, fname, -1, wkey, MAX_PATH);

	rcode = RegDelnodeRecurse(hPathKey, wkey, MAX_PATH * 2);

	RegCloseKey(hPathKey);
	smm_free(wkey);
       
	if (rcode == TRUE) {
		return SMM_ERR_NONE;
	}
	errno = EACCES;
	return SMM_ERR_ACCESS;
}


static int smc_reg_eof(struct KeyDev *cfgd)
{
	return (cfgd->idx == 0) && 
		(RREF(cfgd).i_key == (int) RREF(cfgd).n_keys) &&
		(RREF(cfgd).i_val == (int) RREF(cfgd).n_vals);
}

static DWORD smc_reg_load_info(struct KeyDev *cfgd, HKEY hKey)
{
	RREF(cfgd).hKey  = hKey;
	RREF(cfgd).i_val = 0;
	RREF(cfgd).i_key = 0;

	return RegQueryInfoKey(hKey, NULL, NULL, NULL, 
			&RREF(cfgd).n_keys, &RREF(cfgd).l_key, NULL,
			&RREF(cfgd).n_vals, &RREF(cfgd).l_vname, 
			&RREF(cfgd).l_value, NULL, NULL);
}

static DWORD smc_reg_open_subkey(struct KeyDev *cfgd)
{
	HKEY	hSubKey;
	TCHAR	szName[MAX_PATH];
	DWORD	dwSize, dwErro;

	dwSize = MAX_PATH * sizeof(TCHAR);
	dwErro = RegEnumKeyEx(RREF(cfgd).hKey, RREF(cfgd).i_key++, 
			szName, &dwSize, NULL, NULL, NULL, NULL);
	if (dwErro != ERROR_SUCCESS) {
		return dwErro;
	}

	dwErro = RegOpenKeyEx(RREF(cfgd).hKey, szName, 0, KEY_READ, &hSubKey);
	if (dwErro != ERROR_SUCCESS) {
		return dwErro;
	}

	cfgd->idx++;
	if (cfgd->idx >= CFGF_MAX_DEPTH) {
		cfgd->idx--;
		RegCloseKey(hSubKey);
		return ERROR_INVALID_LEVEL;
	}

	dwErro = smc_reg_load_info(cfgd, hSubKey);
	if (dwErro != ERROR_SUCCESS) {
		cfgd->idx--;
		RegCloseKey(hSubKey);
	}
	return dwErro;
}

static KEYCB *smc_reg_key_alloc(struct KeyDev *cfgd, int idx)
{
	TCHAR	*tbuf;
	DWORD	dwSize, dwLeng, dwType, dwErr, len;
	char 	*content;
	KEYCB	*kp;

	/* allocate a buffer for reading the registry */
	len = RREF(cfgd).l_vname * sizeof(TCHAR) + RREF(cfgd).l_value + 16;
	if ((tbuf = smm_alloc((int)len)) == NULL) {
		return NULL;
	}
	content = (void*) &tbuf[RREF(cfgd).l_vname + 1];

	dwSize = RREF(cfgd).l_vname * sizeof(TCHAR);
	dwLeng = RREF(cfgd).l_value;

	/* need to read it twice if in error because RegEnumValue()
	 * can not pick up unnamed, ie the "(Default)", key. 
	 * The ERROR_MORE_DATA was picked from the return value, maybe buggy.
	 * see winerror.h */
	dwErr = RegEnumValue(RREF(cfgd).hKey, idx, tbuf, &dwSize, NULL, 
			&dwType, (BYTE*)content, &dwLeng);
	if (dwErr == ERROR_MORE_DATA) {
		dwErr = RegQueryValueEx(RREF(cfgd).hKey, NULL, NULL,
				&dwType, (BYTE*)content, &dwLeng);
		tbuf[0] = TEXT('\0');
	}
	if (dwErr != ERROR_SUCCESS) {
		smm_free(tbuf);
		return NULL;
	}

	if ((kp = smc_reg_keyval_alloc(tbuf, (int)dwLeng)) == NULL) {
		smm_free(tbuf);
		return NULL;
	}

	switch (dwType) {
	case REG_BINARY:
		smm_free(kp);
		kp = smc_reg_block_alloc(cfgd, tbuf, content, (int)dwLeng);
		if (kp) {
			kp->comment = kp->value + strlen(kp->value) + 1;
			strcpy(kp->comment, "##REG_BINARY");
		}
		break;
	case REG_DWORD:		/* == REG_DWORD_LITTLE_ENDIAN */
		dwSize = *((DWORD *) content);
		sprintf(kp->value, "%lu", dwSize);
		kp->comment = kp->value + strlen(kp->value) + 1;
		strcpy(kp->comment, "##REG_DWORD");
		break;
	case REG_DWORD_BIG_ENDIAN:
		dwSize = *((DWORD *) content);
		sprintf(kp->value, "%lu", dwSize);
		kp->comment = kp->value + strlen(kp->value) + 1;
		strcpy(kp->comment, "##REG_DWORD_BIG_ENDIAN");
		break;
	case REG_EXPAND_SZ:
		WideCharToMultiByte(smm_codepage(), 0, (TCHAR*)content, -1,
				kp->value, len, NULL, NULL);
		str_substitue_char(kp->value, -1, '#', '_');
		kp->comment = kp->value + strlen(kp->value) + 1;
		strcpy(kp->comment, "##REG_EXPAND_SZ");
		break;
	case REG_LINK:
		WideCharToMultiByte(smm_codepage(), 0, (TCHAR*)content, -1,
				kp->value, len, NULL, NULL);
		str_substitue_char(kp->value, -1, '#', '_');
		kp->comment = kp->value + strlen(kp->value) + 1;
		strcpy(kp->comment, "##REG_LINK");
		break;
	case REG_MULTI_SZ:
		len = WideCharToMultiByte(smm_codepage(), 0, (TCHAR*)content, 
				dwLeng / sizeof(TCHAR),
				kp->value, len, NULL, NULL);
		str_substitue_char(kp->value, len - 1, 0, '~');
		str_substitue_char(kp->value, -1, '#', '_');
		kp->comment = kp->value + strlen(kp->value) + 1;
		strcpy(kp->comment, "##REG_MULTI_SZ");
		break;
	case REG_NONE:
		smm_free(kp);
		kp = smc_reg_block_alloc(cfgd, tbuf, content, (int)dwLeng);
		if (kp) {
			kp->comment = kp->value + strlen(kp->value) + 1;
			strcpy(kp->comment, "##REG_NONE");
		}
		break;
	case REG_QWORD:		/* == REG_QWORD_LITTLE_ENDIAN */
		SMM_SPRINT(kp->value, "%llu", 
				*((unsigned long long *)content));
		kp->comment = kp->value + strlen(kp->value) + 1;
		strcpy(kp->comment, "##REG_QWORD");
		break;
	case REG_SZ:
		WideCharToMultiByte(smm_codepage(), 0, (TCHAR*)content, -1,
				kp->value, len, NULL, NULL);
		str_substitue_char(kp->value, -1, '#', '_');
		kp->comment = kp->value + strlen(kp->value) + 1;
		strcpy(kp->comment, "##REG_SZ");
		break;
	default:
		smm_free(kp);
		kp = smc_reg_block_alloc(cfgd, tbuf, content, (int)dwLeng);
		if (kp) {
			kp->comment = kp->value + strlen(kp->value) + 1;
			strcpy(kp->comment, "##REG_UNKNOWN");
		}
		break;
	}
	smm_free(tbuf);
	return kp;
}

static KEYCB *smc_reg_keyval_alloc(TCHAR *key, int vlen)
{
	KEYCB	*kp;

	/* estimate the total length of the entry according to RFC3629,
	 * the longest UTF-8 character should be 4 bytes. 
	 * Reserved 64 bytes for comments or unnamed key/value. */
	vlen = (vlen + lstrlen(key)) * 4 + 64;
	if ((kp = csc_cfg_kcb_alloc(vlen)) == NULL) {
		return NULL;
	}

	kp->key = kp->pool;
	if (*key == TEXT('\0')) {
		strcpy(kp->key, "(default)");
	} else {
		WideCharToMultiByte(smm_codepage(), 0, key, -1, 
				kp->key, vlen, NULL, NULL);
		/* FIXME: replace all '#' in the key by '_' */
		str_substitue_char(kp->key, -1, '#', '_');
	}

	/* make sure the 'value' field is ready for smc_reg_key_alloc() */
	kp->value = kp->key + strlen(kp->key) + 1;
	kp->flags = CFGF_TYPE_SET(kp->flags, CFGF_TYPE_KEY);
	return kp;
}

static KEYCB *smc_reg_block_alloc(struct KeyDev *cfgd, 
		TCHAR *key, void *bin, int blen)
{
	KEYCB	*kp;

	if ((kp = smc_reg_path_alloc(cfgd, key)) == NULL) {
		return NULL;
	}

	/* make sure the 'value' field is ready for csc_cfg_link_block() */
	kp->value = kp->key + strlen(kp->key) + 1;
	csc_cfg_link_block(kp, bin, blen);
	return kp;
}


/* 'bkey' is used to seperate the directory and binary block.
 * if 'bkey' is not NULL, it should create a binary block. */
static KEYCB *smc_reg_path_alloc(struct KeyDev *cfgd, TCHAR *bkey)
{
	TCHAR	*path;
	KEYCB	*kp;
	int	plen;

	/* retrieve the length of the full path in registery in TCHAR */
	if ((plen = smc_reg_getcwd(cfgd, NULL, 0)) <= 0) {
		return NULL;
	}
	/* if the directory is required to be a binary block by giving
	 * 'bkey' string, the total size would be included the bkey and
	 * an extra 48 bytes so it can fit the "(default)" and CFGF_BLOCK_MAGIC
	 * properly, see csc_config.c. 
	 * I agree it's a bit dirty to achieve this */
	if (bkey) {
		plen += lstrlen(bkey) + 48 + 1;
	}
	/* give some extra TCHARs for comments */
	plen += 48;
	/* estimated the total length of the entry according to RFC3629,
	 * the longest UTF-8 character should be 4 bytes */
	if ((kp = csc_cfg_kcb_alloc(plen * 4)) == NULL) {
		return NULL;
	}

	kp->key = kp->pool;
	strcpy(kp->key, "foolproof");
	if ((path = smm_alloc(plen * sizeof(TCHAR))) != NULL) {
		smc_reg_getcwd(cfgd, path, plen);
		if (bkey) {
			if (*bkey == TEXT('\0')) {
				lstrcat(path, TEXT("/(default)"));
			} else {
				lstrcat(path, TEXT("/"));
				lstrcat(path, bkey);
			}
		}
		WideCharToMultiByte(smm_codepage(), 0, path, -1,
				kp->key, plen * 4, NULL, NULL);
		/* FIXME: replace all '#' in the key by '_' */
		str_substitue_char(kp->key, -1, '#', '_');
		smm_free(path);
	}

	if (bkey) {
		kp->value = kp->key + strlen(kp->key) + 4;
		kp->comment = kp->value + 32;
	} else {
		kp->comment = kp->key + strlen(kp->key) + 4;
		sprintf(kp->comment, "## DIR=%lu KEY=%lu",
				RREF(cfgd).n_keys, RREF(cfgd).n_vals);
	}
	kp->flags = CFGF_TYPE_SET(kp->flags, CFGF_TYPE_DIR);
	return kp;
}

static int smc_reg_getcwd(struct KeyDev *cfgd, TCHAR *path, int n)
{
	TCHAR	szName[MAX_PATH];
	DWORD	dwSize;
	int	i, acc, len;

	if (path && n > 0) {
		path[0] = TEXT('\0');
		n--;
	}
	for (i = acc = 0; i < cfgd->idx; i++) {
		dwSize = MAX_PATH * sizeof(TCHAR);
		if (RegEnumKeyEx(cfgd->reg[i].hKey, cfgd->reg[i].i_key - 1, 
				szName, &dwSize, NULL, NULL, NULL, NULL) 
				!= ERROR_SUCCESS) {
			return SMM_ERR_ACCESS;
		}
		if (i != 0) {
			if (path && (n > 0)) {
				lstrcat(path, TEXT("/"));
				n--;
			}
			acc++;
		}

		len = lstrlen(szName);
		if (path) {
			if (n > len) {
				lstrcat(path, szName);
				n -= len;
			} else  if (n > 0) {
				szName[n] = TEXT('\0');
				lstrcat(path, szName);
				n = 0;
			}
		}
		acc += len;
	}
	return acc;
}

static int smc_reg_binary_open(struct KeyDev *cfgd, KEYCB *kp)
{
	/* the 'value' field of a binary block is always stored the magic
	 * identity so this field can be reused.
	 * note that the 'vsize' field must be set properly in the KEYCB */
	kp->value = NULL;
	if (kp->vsize) {
		kp->value = smm_alloc(kp->vsize);
	}

	/* the 'update' field will be reused too as the index */
	kp->update = 0;

	cfgd->kbin = kp;
	return SMM_ERR_NONE;
}

static int smc_reg_binary_close(struct KeyDev *cfgd)
{
	HKEY	hKey;
	TCHAR	*wpath, *wkey;
	DWORD	dwErr = ERROR_SUCCESS;
	char	*key;
	int	i, len;

	if (cfgd->kbin->value == NULL) {
		cfgd->kbin->vsize = 0;
	}

	/* make sure the length is on the boundry of TCHAR. Note that the 
	 * 'path' is allocated by malloc so it should be always aligned. */
	len = (strlen(cfgd->kbin->key) + sizeof(TCHAR)) / 
		sizeof(TCHAR) * sizeof(TCHAR);
	/* assuming the maximum array size of the UTF-16 string */
	if ((key = csc_strcpy_alloc(cfgd->kbin->key, len * 4)) == NULL) {
		return SMM_ERR_LOWMEM;
	}
	str_substitue_char(key, -1, '/', '\\');
	
	wpath = (TCHAR*)(key + len);
	len = len * 4 / sizeof(TCHAR);
	MultiByteToWideChar(smm_codepage(), 0, key, -1, wpath, len);

	for (wkey = NULL, i = lstrlen(wpath) - 2; i >= 0; i--) {
		if (wpath[i] == TEXT('\\')) {
			wpath[i] = TEXT('\0');
			wkey = &wpath[i+1];
			break;
		}
	}
	if (wkey == NULL) {
		wkey = wpath;
		hKey = cfgd->hRootKey;
	} else {
		dwErr = RegCreateKeyEx(cfgd->hRootKey, wpath, 0, NULL, 0, 
				KEY_ALL_ACCESS, NULL, &hKey, NULL);
		if (dwErr != ERROR_SUCCESS) {
			hKey = NULL;
		}
	}
	//wprintf(TEXT("smc_reg_binary_close: %s %s\n"), wpath, wkey);
	if (hKey) {	//FIXME: do I need to close the hKey?
		DWORD	dwType = REG_BINARY;
		
		if (cfgd->kbin->comment && 
				!strcmp(cfgd->kbin->comment,"##REG_NONE")) {
			dwType = REG_NONE;
		}
		/* remove the unnamed key name */
		if (!lstrcmp(wkey, TEXT("(default)"))) {
			wkey[0] = TEXT('\0');
		}
		dwErr = RegSetValueEx(hKey, wkey, 0, dwType, 
				(void*)cfgd->kbin->value, cfgd->kbin->vsize);
	}

	smm_free(key);
	smm_free(cfgd->kbin->value);
	cfgd->kbin = NULL;
	return dwErr;
}

static int smc_reg_binary_update(struct KeyDev *cfgd, KEYCB *kp)
{
	char	*src, tmp[256];
	int	len;

	if (cfgd->kbin->value == NULL) {
		return 0;
	}

	/* convert ASC to binary */
	src = csc_strbody(kp->key, NULL); /* strip the space */
	len = csc_cfg_hex_to_binary(src, tmp, sizeof(tmp));

	if (cfgd->kbin->update + len > cfgd->kbin->vsize) {
		len = cfgd->kbin->vsize - cfgd->kbin->update;
	}
	if (len) {
		memcpy(cfgd->kbin->value + cfgd->kbin->update, tmp, len);
		cfgd->kbin->update += len;
	}
	return len;
}

static HKEY regy_open_dir(HKEY hRoot, char *dkey)
{
	HKEY	hKey = NULL;
	TCHAR	*wpath;
	char	*path;
	int	len;

	/* make sure the length is on the boundry of TCHAR. Note that the 
	 * 'path' is allocated by malloc so it should be always aligned. */
	len = (strlen(dkey) + sizeof(TCHAR)) / sizeof(TCHAR) * sizeof(TCHAR);
	/* assuming the maximum array size of the UTF-16 string */
	if ((path = csc_strcpy_alloc(dkey, len * 4)) == NULL) {
		return NULL;
	}
	str_substitue_char(path, -1, '/', '\\');

	wpath = (TCHAR*)(path + len);
	len = len * 4 / sizeof(TCHAR);
	MultiByteToWideChar(smm_codepage(), 0, path, -1, wpath, len);

	RegCreateKeyEx(hRoot, wpath, 0, NULL, 0, KEY_ALL_ACCESS, NULL, 
				&hKey, NULL);
	smm_free(path);
	return hKey;
}


static DWORD regy_puts(HKEY hKey, TCHAR *key, DWORD dwType, char *val)
{
	TCHAR	*wval;
	DWORD	dwErr;
	char	*content;
	int	len;

	if (val == NULL) {
		return RegSetValueEx(hKey, key, 0, dwType, NULL, 0);
	}
	if (dwType != REG_MULTI_SZ) {
		if ((wval = smm_mbstowcs_alloc(val)) == NULL) {
			return ERROR_NOT_ENOUGH_MEMORY;
		}
		dwErr = RegSetValueEx(hKey, key, 0, dwType, 
			(void*)wval, (lstrlen(wval)+1) * sizeof(TCHAR));
		smm_free(wval);
		return dwErr;
	}

	/* convert it back from '~' escape if it's REG_MULTI_SZ */
	/* make sure the length is on the boundry of TCHAR. Note that the 
	 * 'content' is allocated by malloc so it should be always aligned. */
	len = (strlen(val) + sizeof(TCHAR)) / sizeof(TCHAR) * sizeof(TCHAR);
	/* assuming the maximum array size of the UTF-16 string */
	if ((content = csc_strcpy_alloc(val, len * 4)) == NULL) {
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	str_substitue_char(content, -1, '~', 0);

	wval = (TCHAR*)(content + len);
	len = MultiByteToWideChar(smm_codepage(), 0, content, strlen(val) + 1, 
			wval, len * 4 / sizeof(TCHAR));
	dwErr = RegSetValueEx(hKey, key, 0, dwType, 
				(void*) wval, len * sizeof(TCHAR));
	smm_free(content);
	return dwErr;
}

/* This code was picked form MSDN, a little modified */
static BOOL RegDelnodeRecurse(HKEY hKeyRoot, LPTSTR lpSubKey, int buflen)
{
	LPTSTR	lpEnd;
	LONG	lResult;
	DWORD	dwSize;
	TCHAR	szName[MAX_PATH];
	HKEY	hKey;

	/* First, see if we can delete the key without having to recurse. */
	if (RegDeleteKey(hKeyRoot, lpSubKey) == ERROR_SUCCESS) {
		return TRUE;
	}

	lResult = RegOpenKeyEx(hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);
	if (lResult != ERROR_SUCCESS) {
		if (lResult == ERROR_FILE_NOT_FOUND) {
			return TRUE;	/* not guilty */
		} else {
			return FALSE;	/* access denied */
		}
	}

	/* Check for an ending slash and add one if it is missing. */
	if (lstrlen(lpSubKey) >= buflen) {
		return FALSE;	/* low buffer memory */
	}
	lpEnd = lpSubKey + lstrlen(lpSubKey);
	if (*(lpEnd - 1) != TEXT('\\')) {
		*lpEnd++ = TEXT('\\');
		*lpEnd = TEXT('\0');
	}

	/* Enumerate the keys */
	/* Original code bugges here. According to MSDN, the size should be
	 * "specified by the lpName parameter, in characters". Therefore
	 * it should be multiplied by the size of TCHAR */
	dwSize = MAX_PATH * sizeof(TCHAR);
	lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL, 
			NULL, NULL, NULL);
	if (lResult == ERROR_SUCCESS) {
		do {
			if (lstrlen(lpSubKey) + lstrlen(szName) >= buflen) {
				break;	/* path is too long */
			}
			lstrcpy(lpEnd, szName);
			if (!RegDelnodeRecurse(hKeyRoot, lpSubKey, buflen)) {
				break;
			}
			dwSize = MAX_PATH * sizeof(TCHAR);
			lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, 
					NULL, NULL, NULL, NULL);
		} while (lResult == ERROR_SUCCESS);
	}
	lpEnd--;
	*lpEnd = TEXT('\0');
	RegCloseKey (hKey);

	/* Try again to delete the key. */
	if (RegDeleteKey(hKeyRoot, lpSubKey) == ERROR_SUCCESS) {
		return TRUE;
	}
	return FALSE;
}

#endif




#if 0
/*
	if (cfgd->hRootKey) {
		return smc_reg_read(cfgd->hRootKey);
	}
*/
static int smc_reg_read(HKEY hKey)
{
	struct	RegBuf	*rbuf;
	HKEY	hNextKey;
	TCHAR	szName[MAX_PATH];
	DWORD	i, dwSize, dwKeys;

	/* get number of keys and values */
	if ((rbuf = registry_buffer_alloc(hKey)) == NULL) {
		return -2;
	}
	for (i = 0; i < rbuf->n_vals; i++) {
		if (registry_buffer_read_value(hKey, i, rbuf) < 0) {
			break;
		}
		if (rbuf->ty_id == REG_SZ) {
			wprintf(TEXT("%s = %s  #%s\n"), 
					rbuf->name, rbuf->content, rbuf->type);
		} else {
			wprintf(TEXT("%s = %p  #%s\n"), 
					rbuf->name, rbuf->content, rbuf->type);
		}
	}

	dwKeys = rbuf->n_keys;
	smm_free(rbuf);

	for (i = 0; i < dwKeys; i++) {
		dwSize = MAX_PATH;	//FIXME confused by MSDN
		if (RegEnumKeyEx(hKey, i, szName, &dwSize, NULL, NULL, 
					NULL, NULL) != ERROR_SUCCESS) {
			continue;
		}
		if (RegOpenKeyEx(hKey, szName, 0, KEY_READ, &hNextKey) 
				== ERROR_SUCCESS) {
			wprintf(TEXT("Entering %s\n"), szName);
			smc_reg_read(hNextKey);
			RegCloseKey(hNextKey);
		}
	}
	return 0;
}
static struct RegBuf *registry_buffer_alloc(HKEY hKey)
{
	struct	RegBuf	*rbuf;
	DWORD	dwKeys, dwKeyLen, dwVals, dwValNmlen, dwValLen, len;

	RegQueryInfoKey(hKey, NULL, NULL, NULL, 
			&dwKeys, &dwKeyLen, NULL,
			&dwVals, &dwValNmlen, &dwValLen, NULL, NULL);

	/* find the longest between name of key and name of value */
	len = dwKeyLen > dwValNmlen ? dwKeyLen : dwValNmlen;
	/* note that name of key and name of value are unicode of Win32 */
	len *= sizeof(TCHAR);
	/* added the longest content of value in byte */
	len += dwValLen;
	/* top up with the structure size and reserve area for type */
	len += sizeof(struct RegBuf) + 64 * sizeof(TCHAR);

	if ((rbuf = smm_alloc((int)len)) == NULL) {
		return NULL;
	}

	rbuf->n_keys = dwKeys;
	rbuf->n_vals = dwVals;
	rbuf->name   = (TCHAR*) rbuf->pool;
	rbuf->nm_len = (dwKeyLen > dwValNmlen ? dwKeyLen : dwValNmlen) + 1;
	rbuf->type   = &rbuf->name[rbuf->nm_len];
	rbuf->ty_len = 60;
	rbuf->content = (void*) &rbuf->type[rbuf->ty_len];
	rbuf->co_len = dwValLen + 1;
	wprintf(TEXT("registry_buffer_alloc: (%u/%u)(%u/%u)\n"),
				 dwKeys, dwKeyLen, dwVals, dwValNmlen);
	return rbuf;
}
#endif


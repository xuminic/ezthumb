/*!\file libcsoup.h
   \brief Head file of CSOUP library, the Chicken Soup for the C

   \details This file is part of CSOUP library, the Chicken Soup for the C.
   CSOUP is a group of functions for general reusing purpose.

   \author "Andy Xuming" <xuming@users.sourceforge.net>
*/
/* Copyright (C) 2013  "Andy Xuming" <xuming@users.sourceforge.net>

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

#ifndef	_LIBCSOUP_H_
#define _LIBCSOUP_H_

#include <stdio.h>
#include <getopt.h>
#include <limits.h>

#define LIBCSOUP_VERSION(x,y,z)	(((x)<<24)|((y)<<12)|(z))
#define LIBCSOUP_VER_MAJOR	1		/* 0-255 */
#define LIBCSOUP_VER_MINOR	1		/* 0-4095 */
#define LIBCSOUP_VER_BUGFIX	2		/* 0-4095 */


/* Forward declaration the structure of circular doubly linked list to hide
 * its details. It will be defined in csc_cdll.c */
struct  _CSCLNK;
typedef	struct	_CSCLNK	CSCLNK;

/*****************************************************************************
 * Command line process functions
 *****************************************************************************/

#define CSC_CLI_UNCMD		(('U'<<24)|('C'<<16)|('M'<<8)|'D')
#define CSC_CLI_MASK		0xf
#define CSC_CLI_PARAM(c)	((c)->param & CSC_CLI_MASK)
#define CSC_CLI_SHOW(c,m)	((m) ? (c)->param & (m) : ((c)->param <= CSC_CLI_MASK))

/*!
 * The option list of Command line interface.
 */
struct	cliopt	{
	int	opt_char;	///Short form of option characters.
	char	*opt_long;	///Long form of option strings.

	/*! param
	 *  0-0xf: how many arguments are required.
	 *  CSC Extension       getopt()
	 *    0                   0        No argument required.
	 *    1                   1        One string argument required.
	 *    2                   2        Optional string argument.
	 *    3                   1        One number required
	 *    4                   2        Optional number argument
	 *    15                 n/a       display comments only   
	 *  0xfffffff0: bitmask of hidden options (CSC Extension)
	 */
	int	param;
	char	*comment;	///A description about this option
};

/*!
 * The command list structure of Command line interface. 
 */
struct	clicmd	{
	char	*cmd;
	int	(*entry)(void *rtime, int argc, char **argv);
	struct	cliopt	*param;
	char	*comment;
};

#ifdef __cplusplus
extern "C"
{
#endif
int csc_cli_make_list(struct cliopt *optbl, char *list, int len);
int csc_cli_make_table(struct cliopt *optbl, struct option *oplst, int len);
int csc_cli_print(struct cliopt *optbl, int mask, int (*show)(char *));

void *csc_cli_getopt_open(struct cliopt *optbl, int *pt_optind);
int csc_cli_getopt_close(void *clibuf);
int csc_cli_getopt(int argc, char * const argv[], void *clibuf);

void *csc_cli_qopt_open(int argc, char **argv);
int csc_cli_qopt_close(void *ropt);
int csc_cli_qopt_optind(void *ropt);
int csc_cli_qopt_optopt(void *ropt);
char *csc_cli_qopt_optarg(void *ropt);
int csc_cli_qopt(void *ropt, struct cliopt *optbl);

int csc_cli_mkargv(char *sour, char **idx, int ids);
int csc_cli_cmd_print(struct clicmd **cmdtbl, int (*show)(char *));
int csc_cli_cmd_run(struct clicmd **cmdtbl, void *rtime, int argc, char **);

#ifdef __cplusplus
} // __cplusplus defined.
#endif



/*****************************************************************************
 * Simple Logger Interface
 *****************************************************************************/
/* README:
Debug level is 0-7 using Bit2 to Bit0 in the control word
  0: unmaskable output (for show-off printf like information)
  1: unmaskable error message (when error occur)
  2: warning output (something might be functionably problem, like server 
     returned not-so-good results. the program itself should be still intact)
  3: information, buffered output (information maybe useful for the user)
  4: debug (debug information for the developer)
  5: program progress (the workflow between modules)
  6: module workflow (the detail progress inside a function module)
  7: function workflow (very trivial information shows how the program 
     running detailly inside a function)
Bit3 is used to indicate flush or non-flush mode.

If the debug level of 'cword' in SMMDBG is 0/SLOG_LVL_AUTO, it means the user
hasn't specified the runtime debug level. The debug level therefore is decided
by the CSOUP_DEBUG_LOCAL macro in every source code file, or otherwise by 
default hardcoded in libcsoup. 

If the debug level in slogs() is 0/SLOG_LVL_AUTO, it means the debug level
is the unmaskable and undecorated.

Module indicator uses Bit31 to Bit4 in the control word (reserved)


slog_init(int default);
slog_set_level(int control_word);
slog_get_level();

slog_bind_stdio();
slog_bind_stderr();
slog_bind_file();
slog_bind_socket();
slog_bind_window();

slog(int control_word, char *fmt, ...);

*/
#define	SLOG_BUFFER		32768	/* maximum log buffer */
#define SLOG_TRANS_CHAIN	32	/* the depth of translator chain */

#define SLOG_LVL_AUTO		0	/* decided by local macroes */
#define SLOG_LVL_ERROR		1
#define SLOG_LVL_WARNING	2
#define SLOG_LVL_INFO		3
#define SLOG_LVL_DEBUG		4
#define SLOG_LVL_PROGRAM	5
#define SLOG_LVL_MODULE		6
#define SLOG_LVL_FUNC		7
#define SLOG_LVL_MASK		7
#define SLOG_FLUSH		8	/* no prefix */
#define SLOG_MODUL_MASK		(UINT_MAX << 4)

#define SLOG_LEVEL_GET(l)	((l) & SLOG_LVL_MASK)
#define SLOG_LEVEL_SET(l,x)	(((l) & ~SLOG_LVL_MASK) | (x))

#define SLOG_MODUL_ENUM(x)	(1 << ((x)+4))		/* x>=0 && x<=27 */
#define SLOG_MODUL_GET(m)	((m) & SLOG_MODUL_MASK)
#define SLOG_MODUL_SET(m,x)	((m) | SLOG_MODUL_ENUM(x))
#define SLOG_MODUL_CLR(m,x)	((m) & ~SLOG_MODUL_ENUM(x))
#define SLOG_MODUL_ALL(m)	((m) | SLOG_MODUL_MASK)

#define SLOG_CWORD(m,l)		(SLOG_MODUL_GET(m) | SLOG_LEVEL_GET(l))

#define SLOG_MAGIC		(('S'<<24) | ('L'<< 16) | ('O'<<8) | 'G')

#define SLOG_OPT_TMSTAMP	1
#define SLOG_OPT_MODULE		2
#define SLOG_OPT_ALL		(SLOG_OPT_TMSTAMP | SLOG_OPT_MODULE)
#define SLOG_OPT_SPLIT		4	/* the log file will be splitted by size or by date */
#define SLOG_OPT_ONCE		8	/* append-and-close mode so logs can be shared */

#define SLOG_TRANSL_MODUL	0
#define SLOG_TRANSL_DATE	1


typedef int	(*F_LCK)(void *);
typedef int	(*F_PREFIX)(int cw, char *buf, int);
typedef	int	(*F_EXT)(void *, void *, char *);

typedef	struct	{
	int	magic;
	int	cword;		/* control word: modules and level */
	int	option;

	/* log into the file */
	char	*filename;	/* log file without the split extension */
	int	fileday;	/* day number for splitting by date */
	FILE	*logd;		/* only meaningful for nonsplit mode */
	size_t	splitlen;	/* file limitation by size, if 0, split by date */
	int	splitnum;	/* maximum splitted logs, if 0, unlimited */

	/* log into the standard output, stdout or stderr */
	FILE	*stdio;

	/* for generating a prefix according to the 'option' field */
	F_PREFIX	trans_module[SLOG_TRANS_CHAIN];
	F_PREFIX	trans_date[SLOG_TRANS_CHAIN];

	/* log into the socket extension */
	F_EXT	f_inet;
	void	*netobj;

	/* mutex for multithread */
	void	*lock;
	F_LCK	f_lock;
	F_LCK	f_unlock;
} SMMDBG;


#ifdef __cplusplus
extern "C"
{
#endif

SMMDBG *slog_initialize(void *mem, int cword);
int slog_shutdown(SMMDBG *dbgc);
int slog_bind_file(SMMDBG *dbgc, char *fname);
int slog_bind_split_file(SMMDBG *dbgc, char *fname, size_t flen, int fnum);
int slog_bind_stdio(SMMDBG *dbgc, FILE *ioptr);
int slog_translate_setup(SMMDBG *dbgc, int which, F_PREFIX func);
int slog_translate_remove(SMMDBG *dbgc, int which, F_PREFIX func);
int slog_translating(SMMDBG *dbgc, int which, int cw, char *buf, int blen);
int slog_output(SMMDBG *dbgc, int cw, char *buf);
int slogs(SMMDBG *dbgc, int cw, char *buf);
int slogs_long(SMMDBG *dbgc, int setcw, int cw, char *buf);
int slogf(SMMDBG *dbgc, int cw, char *fmt, ...);
int slogf_long(SMMDBG *dbgc, int setcw, int cw, char *fmt, ...);
int slog_validate(SMMDBG *dbgc, int setcw, int cw);
void *slog_bind_tcp(SMMDBG *dbgc, int port);

#ifdef __cplusplus
} // __cplusplus defined.
#endif


/*****************************************************************************
 * See csc_cdll.c: circular doubly linked list
 * Definitions and functions for process circular doubly linked list.
 ****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif
CSCLNK *csc_cdl_alloc(int size);
int csc_cdl_insert_after(CSCLNK *refn, CSCLNK *node);
CSCLNK *csc_cdl_insert_head(CSCLNK *anchor, CSCLNK *node);
CSCLNK *csc_cdl_insert_tail(CSCLNK *anchor, CSCLNK *node);
CSCLNK *csc_cdl_remove(CSCLNK *anchor, CSCLNK *node);
CSCLNK *csc_cdl_next(CSCLNK *anchor, CSCLNK *node);
CSCLNK *csc_cdl_search(CSCLNK *anchor, CSCLNK *last,
		int(*compare)(void *, void *), void *refload);
CSCLNK *csc_cdl_goto(CSCLNK *anchor, int idx);
int csc_cdl_index(CSCLNK *anchor, CSCLNK *node);
int csc_cdl_setup(CSCLNK *node, void *prev, void *next, void *rp, int size);
void *csc_cdl_payload(CSCLNK *node);
CSCLNK *csc_cdl_paylink(void *payload);

CSCLNK *csc_cdl_list_alloc_head(CSCLNK **anchor, int size);
CSCLNK *csc_cdl_list_alloc_tail(CSCLNK **anchor, int size);
int csc_cdl_list_insert(CSCLNK **anchor, CSCLNK *node, int idx);
int csc_cdl_list_insert_head(CSCLNK **anchor, CSCLNK *node);
int csc_cdl_list_insert_tail(CSCLNK **anchor, CSCLNK *node);
CSCLNK *csc_cdl_list_free(CSCLNK **anchor, CSCLNK *node);
int csc_cdl_list_destroy(CSCLNK **anchor);
int csc_cdl_list_state(CSCLNK **anchor);
#ifdef __cplusplus
} // __cplusplus defined.
#endif


/*****************************************************************************
 * See memdump.c: Memory dump
 * Definitions and functions for display memory
 ****************************************************************************/
#define CSC_MEMDUMP_BIT_8	0
#define CSC_MEMDUMP_BIT_16	1
#define CSC_MEMDUMP_BIT_32	2
#define CSC_MEMDUMP_BIT_64	3
#define CSC_MEMDUMP_BIT_FLOAT	4
#define CSC_MEMDUMP_BIT_DOUBLE	5
#define CSC_MEMDUMP_BIT_MASK	0xf	/* 8/16/32/64 */

#define CSC_MEMDUMP_TYPE_HEXU	0	/* uppercased hexadecimal */
#define CSC_MEMDUMP_TYPE_HEXL	0x10	/* lowercased hexadecimal */
#define CSC_MEMDUMP_TYPE_UDEC	0x20	/* unsigned decimal */
#define CSC_MEMDUMP_TYPE_IDEC	0x30	/* signed decimal */
#define CSC_MEMDUMP_TYPE_OCT	0x40	/* unsigned octal */
#define CSC_MEMDUMP_TYPE_EE	0x50	/* float, size depend on BIT_MASK */
#define CSC_MEMDUMP_TYPE_MASK	0xf0	

#define CSC_MEMDUMP_WID_MASK	0xf00	/* always plus 2 */
#define CSC_MEMDUMP_WIDTH(n)	(((n)<<8) & CSC_MEMDUMP_WID_MASK)

#define CSC_MEMDUMP_NO_GLYPH	0x1000	/* don't show ASC glyphes */
#define CSC_MEMDUMP_NO_ADDR	0x2000	/* don't show address */
#define CSC_MEMDUMP_NO_FILLING	0x4000	/* don't fill leading 0 */
#define CSC_MEMDUMP_NO_SPACE	0x8000	/* don't fill space between numbers */
#define CSC_MEMDUMP_ALIGN_LEFT	0x10000	/* align to left */
#define CSC_MEMDUMP_REVERSE	0x20000	/* reverse display */

#ifdef __cplusplus
extern "C"
{
#endif
int csc_memdump_line(void *mem, int msize, int flags, char *buf, int blen);
int csc_memdump(void *mem, int range, int column, int flags);
#ifdef __cplusplus
} // __cplusplus defined.
#endif


/*****************************************************************************
 * See csc_config.c: simple interface of a configure file.
 * Definitions and functions for the simple interface of a configure file.
 ****************************************************************************/

#define CFGF_TYPE_UNKWN	0	/* delimiter, not used */
#define CFGF_TYPE_ROOT	1	/* root control block (only one) */
#define CFGF_TYPE_DIR	2	/* directory key control block (under root) */
#define CFGF_TYPE_KEY	3	/* common key */
#define CFGF_TYPE_PART	4	/* partial key without value */
#define CFGF_TYPE_VALUE	5	/* only value without the key */
#define CFGF_TYPE_COMM	6	/* comment */
#define CFGF_TYPE_NULL	8	/* delimiter, not used */
#define CFGF_TYPE_MASK	0xf
#define CFGF_TYPE_SET(f,n)	(((f) & ~CFGF_TYPE_MASK) | (n))
#define CFGF_TYPE_GET(f)	((f) & CFGF_TYPE_MASK)

#define CSC_CFG_READ	0	/* read only */
#define CSC_CFG_RDWR	0x10	/* read and write */
#define CSC_CFG_RWC	0x20	/* read, write and create */
#define CFGF_MODE_MASK  0xf0    /* mask of CSC_CFG_READ,CSC_CFG_RDWR,... */
#define CFGF_MODE_SET(f,n)      (((f) & ~CFGF_MODE_MASK) | (n))
#define CFGF_MODE_GET(f)        ((f) & CFGF_MODE_MASK)


/* define the maximum depth of a directory key */
#define CFGF_MAX_DEPTH	36


typedef	struct	_KEYCB	{
	/* A fixed pointer to its CSCLNK compatible head */
	CSCLNK	*self;
	/* points to the sub-directories chain */
	CSCLNK	*anchor;

	/* Note that the directories must have the '[]' pair.
	 * They will be appended when reading from the registry. */
	char	*key;
	/* The value can be empty, which means a partial key, or points
	 * to binary data, where the vsize is needed */
	char	*value;
	int	vsize;
	/* Note that comments start with '##' are reserved for registry */
	char	*comment;

	int	flags;

	/* if it's a normal key, the update counts the total modification.
	 * if it's a main key, the update counts the modified keys under 
	 * the main key. 
	 * if it's a root key, the update counts every modified keys */
	int	update;

	char	pool[1];
} KEYCB;

#ifdef __cplusplus
extern "C"
{
#endif
KEYCB *csc_cfg_open(int sysdir, char *path, char *filename, int mode);
int csc_cfg_free(KEYCB *cfg);
int csc_cfg_save(KEYCB *cfg);
int csc_cfg_saveas(KEYCB *cfg, int sysdir, char *path, char *filename);
int csc_cfg_flush(KEYCB *cfg);
int csc_cfg_close(KEYCB *cfg);
char *csc_cfg_status(KEYCB *cfg, int *keys);
char *csc_cfg_read(KEYCB *cfg, char *dkey, char *nkey);
char *csc_cfg_read_first(KEYCB *cfg, char *dkey, char **key);
char *csc_cfg_read_next(KEYCB *cfg, char **key);
char *csc_cfg_copy(KEYCB *cfg, char *dkey, char *nkey, int extra);
int csc_cfg_write(KEYCB *cfg, char *dkey, char *nkey, char *value);
int csc_cfg_read_int(KEYCB *cfg, char *dkey, char *nkey, int *val);
int csc_cfg_write_int(KEYCB *cfg, char *dkey, char *nkey, int val);
int csc_cfg_read_long(KEYCB *cfg, char *dkey, char *nkey, long *val);
int csc_cfg_write_long(KEYCB *cfg, char *dkey, char *nkey, long val);
int csc_cfg_read_longlong(KEYCB *cfg, char *dkey, char *nkey, long long *val);
int csc_cfg_write_longlong(KEYCB *cfg, char *dkey, char *nkey, long long val);
int csc_cfg_read_bin(KEYCB *cfg, char *dkey, char *nkey, char *buf, int blen);
void *csc_cfg_copy_bin(KEYCB *cfg, char *dkey, char *nkey, int *bsize);
int csc_cfg_write_bin(KEYCB *cfg, char *dkey, char *nkey, void *bin, int bsize);
int csc_cfg_read_block(KEYCB *cfg, char *dkey, char *buf, int blen);
void *csc_cfg_copy_block(KEYCB *cfg, char *dkey, int *bsize);
int csc_cfg_write_block(KEYCB *cfg, char *dkey, void *bin, int bsize);
int csc_cfg_link_block(KEYCB *block, void *bin, int bsize);
int csc_cfg_block_size(KEYCB *kcb);
int csc_cfg_delete_key(KEYCB *cfg, char *dkey, char *nkey);
int csc_cfg_delete_block(KEYCB *cfg, char *dkey);
KEYCB *csc_cfg_kcb_alloc(int psize);
int csc_cfg_kcb_free(KEYCB *kcb);
int csc_cfg_dump_kcb(KEYCB *cfg);
int csc_cfg_dump(KEYCB *cfg);
int csc_cfg_binary_to_hex(char *src, int slen, char *buf, int blen);
int csc_cfg_hex_to_binary(char *src, char *buf, int blen);
#ifdef __cplusplus
} // __cplusplus defined.
#endif

/*****************************************************************************
 * See csc_pack_hex.c: a simple way to pack files to C array in hex.
 * Definitions and functions for the simple packing hex array.
 ****************************************************************************/
typedef int	(*F_PKHEX)(void *frame, char *fname, void *data, long dlen);

struct	phex_idx	{
	char		*fname;
	void		*data;
	long		dlen;
};

#ifdef __cplusplus
extern "C"
{
#endif
void *csc_pack_hex_verify(void *pachex, long *flen, long *fnsize);
void *csc_pack_hex_find_next(void *pachex);
void csc_pack_hex_list(void *pachex, F_PKHEX lsfunc);
void *csc_pack_hex_load(void *pachex, char *path, long *size);
void *csc_pack_hex_index(void *pachex);
#ifdef __cplusplus
} // __cplusplus defined.
#endif

/*****************************************************************************
 * The dynamic memory allocation management based on:
 * csc_bmem.c: mapping by the bitmap.
 * csc_dmem.c: the doubly linked list. 
 * csc_tmem.c: the single linked list with minimum memory cost.
 * Definitions and functions for the memory management.
 ****************************************************************************/
/* Bit 0-1: memory allocation strategy. */
#define CSC_MEM_FIRST_FIT	0
#define CSC_MEM_BEST_FIT	1
#define CSC_MEM_WORST_FIT	2
#define CSC_MEM_FITMASK		3

/* Bit 2-3: general settings */
#define CSC_MEM_CLEAN		4	/* fill allocated memory with 0 */
#define CSC_MEM_ZERO		8	/* allow allocating empty memory */

/* Bit 4-7: page size for bitmap management and guarding area.
 * 0=32; 1=64; 2=128; 3=256; ...; 11=65536; 
 * Currently it only supports up to 11 so the padding size can be limited to 
 * 16bit in the bitmap management method */
#define CSC_MEM_PAGE(n)		(32<<((((n)>>4)&0xf)%12))

/* Bit 8-11: number of pages for the guarding area.
 * The guarding area is used to debug the memory violation by buffering 
 * both ends. It includes the front guard and back guard, located before and 
 * after the allocated memory.  0=no guardings 1-15=number of pages */
#define CSC_MEM_GUARD(n)	(((n)>>8)&0xf)

/* Set up the page unit and the guarding pages. 
 * The size of guarding page is multiplied by pages size in Bit 4-7. 
 * For example: Bit8-11=3 Bit4-7=2, so both the frond and the back guarding 
 * area is 3*128=384 bytes */
#define CSC_MEM_SETPG(page,guard)	((((page)&15)<<4) | (((guard)&15)<<8))
#define CSC_MEM_GETPG(n)		(CSC_MEM_PAGE(n) * CSC_MEM_GUARD(n))

/* The default memory setting: 
 * First Fit, fill 0 when allocation, not allow empty allocation, no guarding area */
#define CSC_MEM_DEFAULT		(CSC_MEM_FIRST_FIT | CSC_MEM_CLEAN)

#define CSC_MEM_MAGIC_BITMAP	0xAC
#define CSC_MEM_MAGIC_DLINK	0xA6
#define CSC_MEM_MAGIC_TINY	0xA5

#define CSC_MERR_LOWMEM		-1
#define CSC_MERR_INIT		-2
#define CSC_MERR_BROKEN		-3
#define CSC_MERR_RANGE		-4
#define CSC_MERR_TYPE		-5

#ifdef __cplusplus
extern "C"
{
#endif
/* see csc_tmem.c */
void *csc_tmem_init(void *heap, size_t len, int flags);
void *csc_tmem_alloc(void *heap, size_t n);
int csc_tmem_free(void *heap, void *mem);
void *csc_tmem_scan(void *heap, int (*used)(void*), int (*loose)(void*));
size_t csc_tmem_attrib(void *heap, void *mem, int *state);
void *csc_tmem_front_guard(void *heap, void *mem, int *xsize);
void *csc_tmem_back_guard(void *heap, void *mem, int *xsize);

/* see csc_dmem.c */
void *csc_dmem_init(void *heap, size_t len, int flags);
void *csc_dmem_alloc(void *heap, size_t n);
int csc_dmem_free(void *heap, void *mem);
void *csc_dmem_scan(void *heap, int (*used)(void*), int (*loose)(void*));
size_t csc_dmem_attrib(void *heap, void *mem, int *state);
void *csc_dmem_front_guard(void *heap, void *mem, int *xsize);
void *csc_dmem_back_guard(void *heap, void *mem, int *xsize);

/* see csc_bmem.c */
void *csc_bmem_init(void *heap, size_t len, int flags);
void *csc_bmem_alloc(void *heap, size_t n);
int csc_bmem_free(void *heap, void *mem);
void *csc_bmem_scan(void *heap, int (*used)(void*), int (*loose)(void*));
size_t csc_bmem_attrib(void *heap, void *mem, int *state);
void *csc_bmem_front_guard(void *heap, void *mem, int *xsize);
void *csc_bmem_back_guard(void *heap, void *mem, int *xsize);

#ifdef __cplusplus
} // __cplusplus defined.
#endif

/*****************************************************************************
 * Miscellaneous Functions.
 ****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif
void *csc_extname_filter_open(char *s);
int csc_extname_filter_close(void *efft);
int csc_extname_filter_match(void *efft, char *fname);
int csc_extname_filter_export(void *efft, char *buf, int blen);
char *csc_extname_filter_export_alloc(void *efft);

char *csc_strfill(char *s, int padto, int ch);
size_t csc_strlcat(char *dst, const char *src, size_t siz);
size_t csc_strlcpy(char *dst, const char *src, size_t siz);
char *csc_strcpy_alloc(const char *src, int extra);
int csc_fixtoken(char *sour, char **idx, int ids, char *delim);
char **csc_fixtoken_copy(char *sour, char *delim, int *ids);
int csc_ziptoken(char *sour, char **idx, int ids, char *delim);
char **csc_ziptoken_copy(char *sour, char *delim, int *ids);
int csc_isdelim(char *delim, int ch);
char *csc_cuttoken(char *sour, char **token, char *delim);
char *csc_gettoken(char *sour, char *buffer, int blen, char *delim);

/* see csc_cmp_file_extname.c */
int csc_cmp_file_extname(char *fname, char *ext);
int csc_cmp_file_extlist(char *fname, char **ext);
int csc_cmp_file_extargs(char *fname, char *ext, ...);

/* see csc_strbival.c */
int csc_strbival_int(char *s, char *delim, int *opt);
long csc_strbival_long(char *s, char *delim, long *opt);

/* see csc_strbody.c */
char *csc_strbody(char *s, int *len);

/* see csoup_strcmp_list.c */
int csc_strcmp_list(char *dest, char *src, ...);

/* see csc_strcmp_param.c */
int csc_strcmp_param(char *s1, char *s2);

/* see csc_strcount_char.c and csc_strcount_str.c */
int csc_strcount_char(char *s, char *acct);
int csc_strcount_str(char *s, char *needle);

char *csc_path_basename(char *path, char *buffer, int blen);
char *csc_path_path(char *path, char *buffer, int blen);
int csc_strinsert(char *buf, int len, char *ip, int del, char *s);

/* see csc_url_decode.c */
int csc_url_decode(char *dst, int dlen, char *src);
char *csc_url_decode_alloc(char *src);

/* see csc_crc*.c */
unsigned short csc_crc16_byte(unsigned short crc, char data);
unsigned short csc_crc16(unsigned short crc, void *buf, size_t len);
unsigned csc_crc32_byte(unsigned crc, char data);
unsigned csc_crc32(unsigned crc, void  *buf, size_t len);
unsigned char csc_crc8_byte(unsigned char crc, char data);
unsigned char csc_crc8(unsigned char crc, void *buf, size_t len);
unsigned short csc_crc_ccitt_byte(unsigned short crc, char data);
unsigned short csc_crc_ccitt(unsigned short crc, void *buf, size_t len);

/* see iso639.c */
char *csc_iso639_lang_to_iso(char *lang);
char *csc_iso639_lang_to_short(char *lang);
char *csc_iso639_iso_to_lang(char *iso);

/* see csc_file_load.c and csc_file_store.c */
long csc_file_store(char *path, int ovrd, char *src, long len);
char *csc_file_load(char *path, char *buf, long *len);

#ifdef __cplusplus
} // __cplusplus defined.
#endif



/****************************************************************************
 * System Masquerade Module
 ****************************************************************************/
#ifdef __CYGWIN__
#define CFG_WIN32_API	// mingw through cygwin
#endif

#if	(!defined(CFG_WIN32_API) && !defined(CFG_UNIX_API))
/* automatically decide using UNIX or Win32 API */
#if	(defined(_WIN32) || defined(__WIN32__) || defined(__MINGW32__))
#define CFG_WIN32_API
#else
#define CFG_UNIX_API
#endif
#endif

#ifdef  CFG_WIN32_API
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
/* check the WinAPI error code at winerror.h */
#include <windows.h>
#endif

/* Error mask is always 1000 0000 ... in 32-bit error code
 * libsmm error mask uses 0100 0000 ... in 32-bit error code */
#define SMM_ERR_MASK		0xC0000000	/* 1100 0000 0000 ... */
#define SMM_ERR(x)		(SMM_ERR_MASK | (x))

#define SMM_ERR_NONE		0
#define SMM_ERR_NONE_READ	SMM_ERR(0)	/* errno read mode */
#define SMM_ERR_LOWMEM		SMM_ERR(1)
#define SMM_ERR_ACCESS		SMM_ERR(2)	/* access denied */
#define SMM_ERR_EOP		SMM_ERR(3)	/* end of process */
#define SMM_ERR_CHDIR		SMM_ERR(4)	/* change path */
#define SMM_ERR_OPENDIR		SMM_ERR(5)	/* open directory */
#define SMM_ERR_GETCWD		SMM_ERR(6)
#define SMM_ERR_OPEN		SMM_ERR(7)	/* open file */
#define SMM_ERR_STAT		SMM_ERR(8)	/* stat failed */
#define SMM_ERR_LENGTH		SMM_ERR(9)	/* general fail of length */
#define SMM_ERR_PWNAM		SMM_ERR(10)	/* passwd and name */
#define SMM_ERR_MKDIR		SMM_ERR(11)
#define SMM_ERR_RENAME		SMM_ERR(12)
#define SMM_ERR_FOPEN		SMM_ERR(13)
#define SMM_ERR_NULL		SMM_ERR(32)	/* empty content */
#define SMM_ERR_OBJECT		SMM_ERR(33)	/* wrong object */


#define SMM_FSTAT_ERROR		-1
#define	SMM_FSTAT_REGULAR	0
#define SMM_FSTAT_DIR		1
#define SMM_FSTAT_DEVICE	2
#define SMM_FSTAT_LINK		3


/* for smm_pathtrek() */
#define SMM_PATH_DEPTH_MASK	0x0000FFFF	/* should be deep enough */
#define SMM_PATH_DIR_MASK	0xF0000000
#define SMM_PATH_DIR_FIFO	0
#define SMM_PATH_DIR_FIRST	0x10000000
#define SMM_PATH_DIR_LAST	0x20000000

#define SMM_PATH_DIR(f,x)	\
	(((f) & ~SMM_PATH_DIR_MASK) | ((x) & SMM_PATH_DIR_MASK))
#define SMM_PATH_DEPTH(f,x)	\
	(((f) & ~SMM_PATH_DEPTH_MASK) | ((x) & SMM_PATH_DEPTH_MASK))


/* message defines: from main functions to the callback function */
/* for smm_pathtrek() */
#define SMM_MSG_PATH_ENTER	0
#define SMM_MSG_PATH_LEAVE	1
#define SMM_MSG_PATH_EXEC	2
#define SMM_MSG_PATH_STAT	3
#define SMM_MSG_PATH_BREAK	4
#define SMM_MSG_PATH_FLOOR	5


/* notification defines: from callback functions to the main function */
/* for smm_pathtrek() */
#define SMM_NTF_PATH_NONE	0
#define SMM_NTF_PATH_EOP	1	/* end of process: target found */
#define SMM_NTF_PATH_NOACC	2	/* maybe access denied */
#define SMM_NTF_PATH_DEPTH	3	/* maximum depth hit */
#define SMM_NTF_PATH_CHDIR	4	/* can not enter the directory */
#define SMM_NTF_PATH_CHARSET	5	/* charset error in filename */

struct	smmdir	{
	int	flags;

	int	stat_dirs;
	int	stat_files;

	int	depth;		/* 0 = unlimited, 1 = command line level */
	int	depnow;		/* current depth */

	int     (*message)(void *option, char *path, int type, void *info);
	void	*option;

	int	(*path_recur)(struct smmdir *sdir, char *path);
};

typedef int (*F_DIR)(void*, char*, int, void*);

#ifdef	CFG_WIN32_API
#define	SMM_TIME	FILETIME
#else
typedef	struct timeval	SMM_TIME;
#endif

#ifdef	__MINGW32__
#define SMM_PRINT	__mingw_printf
#define SMM_SPRINT	__mingw_sprintf
#define SMM_VSNPRINT	__mingw_vsnprintf
#else	/* should be GCC/UNIX */
#define SMM_PRINT	printf
#define SMM_SPRINT	sprintf
#define SMM_VSNPRINT	vsnprintf
#endif

/* the delimiter of path */
#ifdef	CFG_WIN32_API
#define SMM_DEF_DELIM	"\\"
#define SMM_PATH_DELIM  "\\/"
#else	/* CFG_UNIX_API */
#define SMM_DEF_DELIM	"/"
#define SMM_PATH_DELIM  "/"
#endif

/* Define the root path of configure profiles */
/* $HOME/.config or HKEY_CURRENT_USER\\SOFTWARE\\ */
#define SMM_CFGROOT_DESKTOP     0
/* $HOME or HKEY_CURRENT_USER\\CONSOLE\\ */
#define SMM_CFGROOT_USER        1
/* /etc or HKEY_LOCAL_MACHINE\\SOFTWARE\\ */
#define SMM_CFGROOT_SYSTEM      2
/* current directory (posix only) */
#define SMM_CFGROOT_CURRENT	3
/* read from memory directly (test mode)(same format to file) */
#define SMM_CFGROOT_MEMPOOL	9


/* Forward declaration the structure for reading/writing the configure device.
 * It will be defined in smm_config.c */
struct  KeyDev; 

/* isspace() macro has a problem in Cygwin when compiling it with -mno-cygwin.
 * I assume it is caused by minGW because it works fine with cygwin head files.
 * The problem is it treats some Chinese characters as space characters.
 * A sample is: 0xC5 0xF3 0xD3 0xD1 */
/* In the "C" and "POSIX" locales, white space should be: space, form-feed
 * ('\f'), newline ('\n'), carriage return ('\r'), horizontal tab ('\t'), 
 * and vertical tab ('\v'). */
#define SMM_ISSPACE(c)	((((c) >= 9) && ((c) <= 0xd)) || ((c) == 0x20))


extern	int	smm_error_no;
extern	int	smm_sys_cp;
extern	char	*smm_rt_name;


#ifdef __cplusplus
extern "C"
{
#endif
void *smm_alloc(size_t size);
void *smm_free(void *ptr);
int smm_chdir(char *path);
int smm_codepage(void);
int smm_codepage_set(int cpno);
int smm_codepage_reset(void);

struct KeyDev *smm_config_open(int sysdir, char *path, char *fname, int mode);
int smm_config_close(struct KeyDev *cfgd);
KEYCB *smm_config_read_alloc(struct KeyDev *cfgd);
int smm_config_write(struct KeyDev *cfgd, KEYCB *kp);
int smm_config_delete(int sysdir, char *path, char *fname);
int smm_config_current(struct KeyDev *cfgd, char *buf, int blen);
int smm_config_path(int sysdir, char *path, char *fname, char *buf, int blen);
void smm_config_dump(struct KeyDev *cfgd);

char *smm_cwd_alloc(int extra);
int smm_cwd_pop(void *cwid);
void *smm_cwd_push(void);
int smm_destroy(void);
int smm_errno(void);
int smm_errno_zip(int err);
int smm_errno_update(int value);
long long smm_filesize(char *fname);
char *smm_fontpath(char *ftname, char **userdir);
int smm_fstat(char *fname);
FILE *smm_fopen(char *path, char *mode);
int smm_fncmp(char *dstname, char *srcname);
int smm_init(void);
int smm_mkdir(char *path);
int smm_mkpath(char *path);
int smm_pathtrek(char *path, int flags, F_DIR msg, void *option);
int smm_pwuid(char *uname, long *uid, long *gid);
int smm_rename(char *oldname, char *newname);
int smm_signal_break(int (*handle)(int));
int smm_sleep(int sec, int usec);
int smm_time_diff(SMM_TIME *tmbuf);
int smm_time_get_epoch(SMM_TIME *tmbuf);
char *smm_userpath(char *buffer, int len);
void *smm_mbstowcs_alloc(char *mbs);
char *smm_wcstombs_alloc(void *wcs);
#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif	/* _LIBCSOUP_H_ */


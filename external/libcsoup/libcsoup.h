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

#define LIBCSOUP_VERSION(x,y,z)	(((x)<<24)|((y)<<12)|(z))
#define LIBCSOUP_VER_MAJOR	0		/* 0-255 */
#define LIBCSOUP_VER_MINOR	5		/* 0-4095 */
#define LIBCSOUP_VER_BUGFIX	0		/* 0-4095 */


/****************************************************************************
 * Command line process functions
 ****************************************************************************/

#define CSC_CLI_UNCMD	(('U'<<24)|('C'<<16)|('M'<<8)|'D')

/*!
 * The option list of Command line interface.
 */
struct	cliopt	{
	int	opt_char;	///Short form of option characters.
	char	*opt_long;	///Long form of option strings.

	/*! param 
	 *  How many arguments are required.
	 *  - 0: No argument required.
	 *  - 1: One argument required.
	 *  - 2: Optional argument.  */
	int	param;
	char	*comment;	///A description about thi option
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
int csc_cli_print(struct cliopt *optbl, int (*show)(char *));

void *csc_cli_getopt_open(struct cliopt *optbl);
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



/****************************************************************************
 * Simple Logger Interface
 ****************************************************************************/
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
#define	SLOG_BUFFER		1024	/* maximum log buffer */

#define SLOG_LVL_SHOWOFF	0
#define SLOG_LVL_ERROR		1
#define SLOG_LVL_WARNING	2
#define SLOG_LVL_INFO		3
#define SLOG_LVL_DEBUG		4
#define SLOG_LVL_PROGRAM	5
#define SLOG_LVL_MODULE		6
#define SLOG_LVL_FUNC		7
#define SLOG_LVL_MASK		7
#define SLOG_FLUSH		8

#define SLOG_LEVEL(x)	((x) & SLOG_LVL_MASK)
#define SLOG_MODULE(x)	((x) & ~SLOG_LVL_MASK)

/* short name of slog debug level */
#define SLSHOW		SLOG_LVL_SHOWOFF
#define SLERR		SLOG_LVL_ERROR
#define SLWARN		SLOG_LVL_WARNING
#define SLINFO		SLOG_LVL_INFO
#define SLDBG		SLOG_LVL_DEBUG
#define SLPROG		SLOG_LVL_PROGRAM
#define SLMOD		SLOG_LVL_MODULE
#define SLFUNC		SLOG_LVL_FUNC

/* device bit mask */
#define SLOG_TO_STDOUT		1
#define SLOG_TO_STDERR		2
#define SLOG_TO_FILE		4
#define SLOG_TO_SOCKET		8
#define SLOG_TO_WINDOW		16


typedef int (*F_STD)(int, char*, int);


typedef	struct		{
	unsigned	cword;		/* control word: modules and level */
	unsigned	device;		/* mask of output devices */

	/* log file as output device */
	char		*filename;
	FILE		*logd;

	/* standard i/o as output device */
	F_STD		stdoutput;
	F_STD		stderrput;
} SMMDBG;


#ifdef __cplusplus
extern "C"
{
#endif

void slog_def_open(int cword);
void slog_def_close(void);
int slog_def_stdout(int flush, char *buf, int len);
int slog_def_stderr(int flush, char *buf, int len);
void *slog_open(int cword);
int slog_close(void *control);
SMMDBG *slog_control(void *control);
int slog_validate(SMMDBG *dbgc, int cw);
unsigned slog_control_word_read(void *control);
unsigned slog_control_word_write(void *control, unsigned cword);
int slog_level_read(void *control);
int slog_level_write(void *control, int dbg_lvl);
int slog_bind_stdout(void *control, F_STD func);
int slog_unbind_stdout(void *control);
int slog_bind_stderr(void *control, F_STD func);
int slog_unbind_stderr(void *control);
int slog_bind_file(void *control, char *fname, int append);
int slog_unbind_file(void *control);
int slog_output(SMMDBG *dbgc, int cw, char *buf, int len);
int slogc(void *control, int cw, char *fmt, ...);
int slogs(void *control, int cw, char *buf, int len);

int slog(int cw, char *fmt, ...);
int slos(int cw, char *buf);

int slogz(char *fmt, ...);
int slosz(char *buf);

#ifdef __cplusplus
} // __cplusplus defined.
#endif



/* Extended File Name Filter setting
 *  * Input string could be "avi,wmv,mkv" or "+avi:wmv:-mkv"
 *   * Stored format will be "*.avi", "*.wmv", "*.mkv"
 *    */
typedef struct  {
	char	*filter[1];
} CSEFF;


typedef	struct	_CSCLNK {
	struct	_CSCLNK	*next;
	struct	_CSCLNK	*prev;
	char	payload[1];
} CSCLNK;


#ifdef __cplusplus
extern "C"
{
#endif

void *csc_extname_filter_open(char *s);
int csc_extname_filter_close(void *efft);
int csc_extname_filter_match(void *efft, char *fname);

char *csc_strfill(char *s, int padto, int ch);
size_t csc_strlcpy(char *dst, const char *src, size_t siz);
char *csc_strcpy_alloc(const char *src, int extra);
int csc_fixtoken(char *sour, char **idx, int ids, char *delim);
char **csc_fixtoken_copy(char *sour, char *delim, int *ids);
int csc_ziptoken(char *sour, char **idx, int ids, char *delim);
char **csc_ziptoken_copy(char *sour, char *delim, int *ids);
int csc_isdelim(char *delim, int ch);
char *csc_cuttoken(char *sour, char **token, char *delim);
char *csc_gettoken(char *sour, char *buffer, char *delim);


/* see csc_cmp_file_extname.c */
int csc_cmp_file_extname(char *fname, char *ext);
int csc_cmp_file_extlist(char *fname, char **ext);
int csc_cmp_file_extargs(char *fname, char *ext, ...);

char *csc_strbody(char *s, int *len);

/* see csoup_strcmp_list.c */
int csc_strcmp_list(char *dest, char *src, ...);

char *csc_path_basename(char *path, char *buffer, int blen);
char *csc_path_path(char *path, char *buffer, int blen);

/* see memdump.c */
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

int csc_memdump_line(void *mem, int msize, int flags, char *buf, int blen);
int csc_memdump(void *mem, int range, int column, int flags);

/* see csc_crc*.c */
unsigned short csc_crc16_byte(unsigned short crc, char data);
unsigned short csc_crc16(unsigned short crc, void *buf, size_t len);
unsigned long csc_crc32_byte(unsigned long crc, char data);
unsigned long csc_crc32(unsigned long crc, void  *buf, size_t len);
unsigned char csc_crc8_byte(unsigned char crc, char data);
unsigned char csc_crc8(unsigned char crc, void *buf, size_t len);
unsigned short csc_crc_ccitt_byte(unsigned short crc, char data);
unsigned short csc_crc_ccitt(unsigned short crc, void *buf, size_t len);

/* see csc_cdll.c: circular doubly linked list functions */
void csc_cdl_insert_after(CSCLNK *refn, CSCLNK *node);
CSCLNK *csc_cdl_insert_head(CSCLNK *anchor, CSCLNK *node);
CSCLNK *csc_cdl_insert_tail(CSCLNK *anchor, CSCLNK *node);
CSCLNK *csc_cdl_remove(CSCLNK *anchor, CSCLNK *node);
CSCLNK *csc_cdl_next(CSCLNK *anchor, CSCLNK *node);
CSCLNK *csc_cdl_search(CSCLNK *anchor, CSCLNK *last,
		int(*compare)(void *, void *), void *refload);
CSCLNK *csc_cdl_goto(CSCLNK *anchor, int idx);
CSCLNK *csc_cdl_alloc_head(CSCLNK **anchor, int size);
CSCLNK *csc_cdl_alloc_tail(CSCLNK **anchor, int size);
int csc_cdl_free(CSCLNK **anchor, CSCLNK *node);
int csc_cdl_destroy(CSCLNK **anchor);

/* see csc_config.c: simple configure file */
void *csc_cfg_open(char *path, char *filename, int rdflag);
int csc_cfg_abort(void *cfg);
int csc_cfg_save(void *cfg);
int csc_cfg_saveas(void *cfg, char *path, char *filename);
int csc_cfg_flush(void *cfg);
int csc_cfg_close(void *cfg);
char *csc_cfg_read(void *cfg, char *mkey, char *skey);
char *csc_cfg_read_first(void *cfg, char *mkey, char **key);
char *csc_cfg_read_next(void *cfg, char **key);
char *csc_cfg_copy(void *cfg, char *mkey, char *skey, int extra);
int csc_cfg_write(void *cfg, char *mkey, char *skey, char *value);
int csc_cfg_read_long(void *cfg, char *mkey, char *skey, long *val);
int csc_cfg_write_long(void *cfg, char *mkey, char *skey, long val);
int csc_cfg_read_longlong(void *cfg, char *mkey, char *skey, long long *val);
int csc_cfg_write_longlong(void *cfg, char *mkey, char *skey, long long val);
int csc_cfg_read_bin(void *cfg, char *mkey, char *skey, char *buf, int blen);
void *csc_cfg_copy_bin(void *cfg, char *mkey, char *skey, int *bsize);
int csc_cfg_write_bin(void *cfg, char *mkey, char *skey, void *bin, int bsize);
int csc_cfg_read_block(void *cfg, char *mkey, char *buf, int blen);
void *csc_cfg_copy_block(void *cfg, char *mkey, int *bsize);
int csc_cfg_write_block(void *cfg, char *mkey, void *bin, int bsize);
int csc_cfg_dump_kcb(void *cfg);
int csc_cfg_dump(void *cfg, char *mkey);

/* see iso639.c */
char *csc_iso639_lang_to_iso(char *lang);
char *csc_iso639_lang_to_short(char *lang);
char *csc_iso639_iso_to_lang(char *iso);

long csc_file_store(char *path, int ovrd, char *src, long len);
char *csc_file_load(char *path, char *buf, long *len);

#ifdef __cplusplus
} // __cplusplus defined.
#endif



/****************************************************************************
 * System Masquerade Module
 ****************************************************************************/
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
int smm_free(void *ptr);
int smm_chdir(char *path);
int smm_codepage(void);
int smm_codepage_set(int cpno);
int smm_codepage_reset(void);
void *smm_config_open(int sysroot, char *path, char *fname);
int smm_config_flush(void *cfg);
int smm_config_close(void *cfg);
int smm_config_delete(int sysroot, char *path, char *fname);
char *smm_config_read(void *cfg, char *mkey, char *skey);
int smm_config_write(void *cfg, char *mkey, char *skey, char *value);
int smm_config_read_long(void *cfg, char *mkey, char *skey, long *val);
int smm_config_write_long(void *cfg, char *mkey, char *skey, long val);
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
int smm_init(int logcw);
int smm_mkdir(char *path);
int smm_mkpath(char *path);
int smm_pathtrek(char *path, int flags, F_DIR msg, void *option);
int smm_pwuid(char *uname, long *uid, long *gid);
int smm_signal_break(int (*handle)(int));
int smm_sleep(int sec, int usec);
int smm_time_diff(SMM_TIME *tmbuf);
int smm_time_get_epoch(SMM_TIME *tmbuf);
void *smm_mbstowcs(char *mbs);
char *smm_wcstombs(void *wcs);

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif	/* _LIBCSOUP_H_ */


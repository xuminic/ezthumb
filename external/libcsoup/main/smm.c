
/*  smm_main.c - main entry to test the SMM library

    Copyright (C) 2013  "Andy Xuming" <xuming@users.sourceforge.net>

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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcsoup.h"

#define VERSION	"0.1"

extern SMMDBG  *tstdbg;

static int do_signal_break(int sig)
{
	slogc(tstdbg, SLINFO, "Signal %d received\n", sig);
	return sig;
}

static int do_smm_chdir(char *path)
{
	char	*cwd;
	int	rc;

	rc = smm_chdir(path);
	
	cwd = smm_cwd_alloc(0);
	slogc(tstdbg, SLINFO, "Enter: %s\n", cwd);
	free(cwd);

	slogc(tstdbg, SLINFO, "Press any key to continue ... ");
	getchar();
	slogc(tstdbg, SLINFO, "(%d)\n", rc);
	return 0;
}

#ifdef	CFG_WIN32_API
#define TEST_PATH	"My\\Com\\pany"
#else
#define TEST_PATH	"My/Com/pany"
#endif
#define TEST_FILE	"MyProduct"
static int	syspath[] = { SMM_CFGROOT_DESKTOP, SMM_CFGROOT_USER, 
		SMM_CFGROOT_SYSTEM, SMM_CFGROOT_CURRENT };

extern int csc_cfg_kcb_fillup(KEYCB *kp);

static int do_smm_config(char *path)
{
	struct	KeyDev	*root, *save, *sfd;
	int	i;
	char	sbuf[256];
	char    *config = "\
[main/dev/holiday]\n\
[main/dev]\n\
[/hardware/driver///howsit]\n\
[/usr/andy]\n\
key   =   v alue  #  hello\n\
[/usr/boy]\n\
[MyBlock]=BINARY:132	##REG_BINARY\n\
010000000400000001000000040000000100000008010000070000000100000064000000\n\
0800000001000000640000000C0000000100000064000000350000000100000064000000\n\
1900000001000000640000001A0000000100000064000000360000000100000064000000\n\
1F0000000100000064000000200000000100000064000000\n\
[usr/Columns/7-Zip.7z]=BINARY:144	##REG_BINARY\n\
010000000000000001000000040000000100000031010000070000000100000064000000\n\
0800000001000000640000000C0000000100000064000000090000000100000064000000\n\
1300000001000000640000000F0000000100000064000000160000000100000064000000\n\
1B00000001000000640000001F0000000100000064000000200000000100000064000000\n\
";
	KEYCB	*kbuf;

	slogz("\n[SMMCONFIG] Default Path Test:\n"); 
	for (i = 0; i < (int)(sizeof(syspath)/sizeof(int)); i++) {
		root = smm_config_open(syspath[i], TEST_PATH, 
				TEST_FILE, 0xdeadbeef);
		if (root) {
			smm_config_dump(root);
			smm_config_close(root);
		}
	}
	slogz("\n[SMMCONFIG] Open memory for read:\n");
	root = smm_config_open(SMM_CFGROOT_MEMPOOL, config, NULL, 0);
	if (root) {
		smm_config_dump(root);
		smm_config_close(root);
	}
	slogz("\n[SMMCONFIG] Open memory for read/write:\n");
	root = smm_config_open(SMM_CFGROOT_MEMPOOL, config, 
			NULL, sizeof(config));
	if (root) {
		smm_config_dump(root);
		smm_config_close(root);
	}
	slogz("\n[SMMCONFIG] Open memory for R/W/C:\n");
	/* the CSC_CFG_RWC will be ignored because 'path' is NULL */
	root = smm_config_open(SMM_CFGROOT_MEMPOOL, NULL, NULL, CSC_CFG_RWC);
	if (root) {
		smm_config_dump(root);
		smm_config_close(root);
	}
	slogz("\n[SMMCONFIG] Dump memory configure:\n");
	root = smm_config_open(SMM_CFGROOT_MEMPOOL, config, NULL, 0);
	while ((kbuf = smm_config_read_alloc(root)) != NULL) {
		//slogz("READ: %s", kbuf->pool);
		if (CFGF_TYPE_GET(kbuf->flags) == CFGF_TYPE_UNKWN) {
			csc_cfg_kcb_fillup(kbuf);
		}
		smm_config_write(root, kbuf);
		smm_free(kbuf);
	}
	smm_config_close(root);

	slogz("\n[SMMCONFIG] Save memory configure to memory and [%s]\n",
			TEST_FILE);
	root = smm_config_open(SMM_CFGROOT_MEMPOOL, config, NULL, 0);
	save = smm_config_open(SMM_CFGROOT_MEMPOOL, sbuf, NULL, sizeof(sbuf));
	sfd = smm_config_open(SMM_CFGROOT_CURRENT, NULL, 
			TEST_FILE, CSC_CFG_RWC);
	while ((kbuf = smm_config_read_alloc(root)) != NULL) {
		if (kbuf->key == NULL) {
			csc_cfg_kcb_fillup(kbuf);
		}
		smm_config_write(save, kbuf);
		smm_config_write(sfd, kbuf);
		smm_free(kbuf);
	}
	smm_config_close(sfd);
	smm_config_close(save);
	smm_config_close(root);
	slogz("\n[SMMCONFIG] In Memory (overflowed):\n%s\n", sbuf);

	slogz("\n[SMMCONFIG] Dump/Copy/Append %s in "
			"HKEY_CURRENT_USER\\SOFTWARE\\ or .config\n", path);
	/* open HKEY_CURRENT_USER\\SOFTWARE\\7-Zip */
	root = smm_config_open(SMM_CFGROOT_DESKTOP, NULL, path, CSC_CFG_READ);
	if (root == NULL) {
		slogz("[SMMCONFIG] Can not find %s\n", path);
		return -1;
	}
	sprintf(sbuf, "%s.copy", path);
	save = smm_config_open(SMM_CFGROOT_DESKTOP, NULL, sbuf, CSC_CFG_RWC);
	if (save == NULL) {
		slogz("[SMMCONFIG] Failed to create %s\n", sbuf);
		smm_config_close(root);
		return -2;
	}
	while ((kbuf = smm_config_read_alloc(root)) != NULL) {
		csc_cfg_dump_kcb(kbuf);
		smm_config_write(save, kbuf);
		smm_free(kbuf);
	}
	smm_config_close(root);

	root = smm_config_open(SMM_CFGROOT_MEMPOOL, config, NULL, 0);
	while ((kbuf = smm_config_read_alloc(root)) != NULL) {
		if (CFGF_TYPE_GET(kbuf->flags) == CFGF_TYPE_UNKWN) {
			csc_cfg_kcb_fillup(kbuf);
		}
		csc_cfg_dump_kcb(kbuf);
		smm_config_write(save, kbuf);
		smm_free(kbuf);
	}
	smm_config_close(root);
	smm_config_close(save);
	return 0;
}

static int do_smm_config_dump(char *path)
{
	struct	KeyDev	*root, *output;
	int	sysidx;
	KEYCB	*kbuf;

	sysidx = 0;
	if (isdigit(*path)) {
		sysidx = *path - '0';
		path++;
	}

	root = smm_config_open(syspath[sysidx], NULL, path, CSC_CFG_READ);
	if (root == NULL) {
		slogz("[SMMCONFIG] Can not find %s\n", path);
		return -1;
	}
	output = smm_config_open(SMM_CFGROOT_MEMPOOL, NULL, NULL, 0);
	while ((kbuf = smm_config_read_alloc(root)) != NULL) {
		if (CFGF_TYPE_GET(kbuf->flags) == CFGF_TYPE_UNKWN) {
			csc_cfg_kcb_fillup(kbuf);
		}
		smm_config_write(output, kbuf);
		smm_free(kbuf);
	}
	smm_config_close(output);
	smm_config_close(root);
	return 0;
}


static int do_smm_mkpath(char *path)
{
#ifdef	CFG_UNIX_API
	static	char	*plist[] = {
		NULL,
		"Press/any/key to/continue/./../1234",
		"/root/home/user/folder/file",
		"./user/folder/file",
		"",
		NULL
	};
#else	/* CFG_WIN32_API */
	static	char	*plist[] = {
		NULL,
		//"\\\\UNC@SSL@Port\\SharedFolder\\Resource",
		//"\\\\?\\UNC\\ComputerName\\SharedFolder\\Resource",
		//"\\\\?\\C:\\SharedFolder\\Resource",
		"\\\\VBOXSVR\\Shared\\HelloWorld",
		"C:\\SharedFolder\\Resource",
		"C:SharedFolder\\Resource",
		".\\SharedFolder\\Resource",
		"",
		NULL
	};
#endif
	int	i;

	plist[0] = path;
	for (i = 0; plist[i]; i++) {
		slogc(tstdbg, SLINFO, "MKDIR: %s\n", plist[i]);
		if (smm_mkpath(plist[i]) != SMM_ERR_NONE) {
			slogc(tstdbg, SLINFO, "Boo!\n");
		} else if (smm_fstat(plist[i]) != SMM_FSTAT_DIR) {
			slogc(tstdbg, SLINFO, "FAILED!\n");
		}
	}
	return 0;
}

static int do_push_dir(char *path)
{
	char	*cwd, *cid;
	int	rc;

	cwd = smm_cwd_alloc(0);
	slogc(tstdbg, SLINFO, "Current: %s\n", cwd);
	free(cwd);

	cid = smm_cwd_push();
	
	rc = smm_chdir(path);
	cwd = smm_cwd_alloc(0);
	slogc(tstdbg, SLINFO, "Enter: %s\n", cwd);
	free(cwd);

	smm_cwd_pop(cid);
	
	cwd = smm_cwd_alloc(0);
	slogc(tstdbg, SLINFO, "Return: %s\n", cwd);
	free(cwd);

	slogc(tstdbg, SLINFO, "Press any key to continue ... ");
	getchar();
	slogc(tstdbg, SLINFO, "(%d)\n", rc);
	return 0;
}

static int do_stat_file(char *path)
{
	int	rc;

	rc = smm_fstat(path);
	switch (rc) {
	case SMM_FSTAT_REGULAR:
		slogc(tstdbg, SLINFO, "%s: regular\n", path);
		break;
	case SMM_FSTAT_DIR:
		slogc(tstdbg, SLINFO, "%s: directory\n", path);
		break;
	case SMM_FSTAT_LINK:
		slogc(tstdbg, SLINFO, "%s: link\n", path);
		break;
	case SMM_FSTAT_DEVICE:
		slogc(tstdbg, SLINFO, "%s: device\n", path);
		break;
	}
	slogc(tstdbg, SLINFO, "(%d)\n", rc);
	return 0;
}


static int pathtrek_cb(void *option, char *path, int type, void *info)
{
	struct	smmdir	*sdir = info;

	(void) option;		/* stop the compiler warning */
	switch (type) {
	case SMM_MSG_PATH_ENTER:
		slogc(tstdbg, SLINFO, "Enter %s\n", path);
		break;
	case SMM_MSG_PATH_LEAVE:
		slogc(tstdbg, SLINFO, "Leave %s (%d:%d)\n", path, 
				sdir->stat_dirs, sdir->stat_files);
		break;
	case SMM_MSG_PATH_STAT:
		slogc(tstdbg, SLINFO, "Finish (%d:%d)\n", sdir->stat_dirs, sdir->stat_files);
		break;
	case SMM_MSG_PATH_EXEC:
		slogc(tstdbg, SLINFO, "Processing %s\n", path);
		break;
	}
	return 0;
}

static int do_path_trek(char *path, int flags)
{
	int	rc;

	rc = smm_pathtrek(path, flags, pathtrek_cb, NULL);
	slogc(tstdbg, SLINFO, "(%d)\n", rc);
	return 0;
}


static	struct	cliopt	clist[] = {
	{   0, NULL,      0, "OPTIONS:" },
	{ 'c', NULL,      1, "change current working directory" },
	{ 'f', NULL,	  1, "configure file process" },
	{ 'F', NULL,	  1, "dump configure file" },
	{ 'm', NULL,	  1, "make directory" },
	{ 'p', NULL,      1, "push/pop current working directory" },
	{ 'r', NULL,      1, "process directory recurrsively" },
	{ 's', NULL,      1, "state of the file" },
	{   1, "help",    0, "*Display the help message" },
	{   2, "version", 0, "*Display the version message" },
	{   3, "dir-fifo", 0, NULL },
	{   4, "dir-first", 0, NULL },
	{   5, "dir-last",  0, NULL },
	{ 0, NULL, 0, NULL }
};

static  char    *version = "smm " VERSION
", Test program for the System Masquerade Module library.\n\
Copyright (C) 2011 \"Andy Xuming\" <xuming@users.sourceforge.net>\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n";

int smm_main(void *rtime, int argc, char **argv)
{
	void	*rtbuf;
	int	c, d_flags;

	/* stop the compiler complaining */
	(void) rtime;

	if ((rtbuf = csc_cli_getopt_open(clist)) == NULL) {
		return -1;
	}

	smm_signal_break(do_signal_break);

	d_flags = SMM_PATH_DIR_FIFO;
	while ((c = csc_cli_getopt(argc, argv, rtbuf)) > 0) {
		switch (c) {
		case 1:
			csc_cli_print(clist, NULL);
			goto quick_quit;
		case 2:
			slogs(tstdbg, SLINFO, version, strlen(version));
			goto quick_quit;
		case 3:
			d_flags &= ~SMM_PATH_DIR_MASK;
			d_flags |= SMM_PATH_DIR_FIFO;
			break;
		case 4:
			d_flags &= ~SMM_PATH_DIR_MASK;
			d_flags |= SMM_PATH_DIR_FIRST;
			break;
		case 5:
			d_flags &= ~SMM_PATH_DIR_MASK;
			d_flags |= SMM_PATH_DIR_LAST;
			break;
		case 'c':
			do_smm_chdir(optarg);
			break;
		case 'f':
			do_smm_config(optarg);
			break;
		case 'F':
			do_smm_config_dump(optarg);
			break;
		case 'm':
			do_smm_mkpath(optarg);
			break;
		case 'p':
			do_push_dir(optarg);
			break;
		case 's':
			do_stat_file(optarg);
			break;
		case 'r':
			do_path_trek(*++argv, d_flags);
			break;
		default:
			slogc(tstdbg, SLINFO, "Unknown option. [%c]\n", c);
			goto quick_quit;
		}
	}
quick_quit:
	csc_cli_getopt_close(rtbuf);
	return 0;
}

struct	clicmd	smm_cmd = { 
	"smm", smm_main, clist, "Testing the smm functions" 
};
extern	struct	clicmd	smm_cmd;



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

static int do_smm_config(char *path)
{
	void	*root;
	char	*val;
	long	vlong;

	if ((root = smm_config_open(0, TEST_PATH, TEST_FILE)) == NULL) {
		slogc(tstdbg, SLINFO, "Failed\n");
		return -1;
	}
	smm_config_write(root, NULL, "Hello", "This is a hello key");
	smm_config_write(root, "[main]", "World", "This is a world key");
	smm_config_write_long(root, "[main]", "Width", (long)1024);

	val = smm_config_read(root, "[main]", "World");
	puts(val);
	smm_config_read_long(root, "[main]", "Width", &vlong);
	printf("%ld\n", vlong);
	smm_config_close(root);

	if (!strcmp(path, "del")) {
		smm_config_delete(0, TEST_PATH, TEST_FILE);
	}
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
	{ 'c', NULL,      2, "change current working directory" },
	{ 'f', NULL,	  2, "configure file process" },
	{ 'm', NULL,	  2, "make directory" },
	{ 'p', NULL,      2, "push/pop current working directory" },
	{ 'r', NULL,      2, "process directory recurrsively" },
	{ 's', NULL,      2, "state of the file" },
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

int smm_main(int argc, char **argv)
{
	struct	clirun	*rtbuf;
	int	c, d_flags;

	if ((rtbuf = csc_cli_getopt_alloc(clist)) == NULL) {
		return -1;
	}

	smm_signal_break(do_signal_break);

	d_flags = SMM_PATH_DIR_FIFO;
	while ((c = getopt_long(argc, argv, rtbuf->optarg, rtbuf->oplst, NULL)) > 0) {
		switch (c) {
		case 1:
			csc_cli_print(clist);
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
	free(rtbuf);
	return 0;
}


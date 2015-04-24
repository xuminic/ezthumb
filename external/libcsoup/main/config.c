/*  fontpath.c - test harness of smm_fontpath()

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "libcsoup.h"
#include "csoup_internal.h"

#define	TESTPATH	"longpath/shortpath/config/myconfig"
#define TESTINPUT	"myconfig.test"
#define TESTOUTPUT	"myconfig.out"

static	char	testconf[] = "\
window_width=4096\n\
grid_column=690\n\
zoom_height=how do i know\n\
last_directory=/home/xuxx/zvtest\n\
empty_key=#this is an empty key\n\
this_is_a_partial_key\n\
=another partial key\n\
\n\
[main]\n\
timestamp=Mon Apr 28 16:42:30 2014\n\
simple_profile=8M4x2:10L10x100x1.027000:100R4x320:appendix:appendix:appendix:appendix\n\
grid_define=Auto\n\
zoom_define=Auto\n\
\n\
window_width=1280\n\
window_height=1024\n\
\n\
   [hello]\n\
# this is a comment\n\
window_width=897\n\
window_height=500\n\
window_state=0\n\
grid_column=4=690\n\
grid_row=4\n\
\n\
[what]=purely unknown           # for test only\n\
Binary=00710B0200000000D0720B02000000004243594B74000000F4620B02000000000F630B02000000000000000000000000\n\
time_step=0\n\
zoom_ratio=50\n\
zoom_width=0\n\
zoom_height=0\n\
canvas_width=0\n\
\n\
[print]  #this is a print page\n\
duration_mode=12288\n\
file_format=jpg@85\n\
transparency=no\n\
last_directory=/HOME/XUM1/vMACHINE/sHARED/EZVTEST\n\n\
";


static char *mytimestamp(int mode)
{
	static	char	tmbuf[128];
	time_t	tmtick;

	time(&tmtick);
	if (mode == 0) {
		sprintf(tmbuf, "%u", (unsigned) tmtick);
	} else {
		strcpy(tmbuf, ctime(&tmtick));
		tmbuf[strlen(tmbuf)-1] = 0;
	}
	return tmbuf;
}


static int config_open_rdonly(void)
{
	KEYCB	*root;

	if ((root = csc_cfg_open(SMM_CFGROOT_CURRENT, TESTPATH, TESTINPUT, 
			CSC_CFG_READ)) == NULL) {
		CDB_SHOW(("can't open\n"));
		return -1;
	}
	csc_cfg_dump(root);
	if (csc_cfg_save(root) == SMM_ERR_NONE) {
		CDB_SHOW(("FATAL: should be read only\n"));
		csc_cfg_free(root);
		return -2;
	}

	csc_cfg_saveas(root, SMM_CFGROOT_CURRENT, TESTPATH, TESTOUTPUT);
	csc_cfg_free(root);
	return 0;
}


static int config_open_with_directory(void)
{
	KEYCB	*cfg;
	char	*config = "\
[main/dev/holiday]\n\
[main/dev]\n\
[/hardware/driver///howsit]\n\
[/usr/andy]\n\
key=value\n\
[/usr/boy]\n";

	if ((cfg = csc_cfg_open(SMM_CFGROOT_MEMPOOL, config, 
					NULL, CSC_CFG_READ)) == NULL) {
		CDB_SHOW(("Weird\n"));
		return -1;
	}
	
	csc_cfg_write(cfg, "main/dev/usb", "camara", "xxooxxoo");
	csc_cfg_write(cfg, NULL, "myprofile", "n/a");
	csc_cfg_write(cfg, "", "wtf", "beam");
	csc_cfg_write(cfg, "/hardware/driver///howsit", "good", "day");
	csc_cfg_dump(cfg);

	csc_cfg_write_bin(cfg, "/usr/bin", "simple", cfg, sizeof(KEYCB));
	csc_cfg_write_block(cfg, "/usr/block", config, strlen(config));

	csc_cfg_saveas(cfg, SMM_CFGROOT_MEMPOOL, NULL, NULL);
	csc_cfg_close(cfg);
	return 0;
}

static int config_key_test(void)
{
	KEYCB	*root;
	int	i, n;
	long	lv;
	char	*val, *key, nkey[64], mkey[64];
	char	*rdlist[][2] = {
		{ "hello", "grid_column" },
		{ "hello", "grid_column=4" },
		{ "hello", "window_width" },
		{ "main", "window_width" },
		{ "main", "simple_profile" },
		{ "what", "zoom_height" },
		{ "print", "last_directory" },
		{ NULL, "window_width" },
		{ NULL, NULL }
	};

	if ((root = csc_cfg_open(SMM_CFGROOT_CURRENT, TESTPATH, TESTINPUT, 
					CSC_CFG_RWC)) == NULL) {
		CDB_SHOW(("Weird!\n"));
		return -1;
	}
	for (i = 0; rdlist[i][0] || rdlist[i][1]; i++) {
		CDB_SHOW(("READ %s: %s = %s\n", rdlist[i][0], rdlist[i][1],
			csc_cfg_read(root, rdlist[i][0], rdlist[i][1])));
	}

	if ((val = csc_cfg_read_first(root, NULL, &key)) != NULL) {
		CDB_SHOW(("READ NULL: %s = %s\n", key, val));
		while ((val = csc_cfg_read_next(root, &key)) != NULL) {
			CDB_SHOW(("READ NULL: %s = %s\n", key, val));
		}
	}

	if ((val = csc_cfg_read_first(root, "what", &key)) != NULL) {
		CDB_SHOW(("READ [what]: %s = %s\n", key, val));
		while ((val = csc_cfg_read_next(root, &key)) != NULL) {
			CDB_SHOW(("READ [what]: %s = %s\n", key, val));
		}
	}

	/* read an integer */
	csc_cfg_read_long(root, rdlist[2][0], rdlist[2][1], &lv);
	CDB_SHOW(("READLONG %s: %s = %ld\n", rdlist[2][0], rdlist[2][1], lv));

	/* write a new main key */
	strcpy(mkey, mytimestamp(0));
	sprintf(nkey, "timestamp");
	csc_cfg_write(root, mkey, nkey, mytimestamp(1));
	CDB_SHOW(("WRITENEW %s: %s = %s\n", mkey, nkey, 
			csc_cfg_read(root, mkey, nkey)));

	/* write to the root key */
	csc_cfg_write(root, NULL, nkey, mytimestamp(1));
	CDB_SHOW(("WRITEROOT: %s = %s\n", nkey, 
				csc_cfg_read(root, NULL, nkey)));

	/* write something longer than orignal */
	val = csc_cfg_copy(root, rdlist[4][0], rdlist[4][1], 64);
	if (val == NULL) {
		val = csc_strcpy_alloc(":appendix", 0);
	} else {
		strcat(val, ":appendix");
	}
	csc_cfg_write(root, rdlist[4][0], rdlist[4][1], val);
	CDB_SHOW(("WRITEEXT %s: %s = %s\n", rdlist[4][0], rdlist[4][1],
			csc_cfg_read(root, rdlist[4][0], rdlist[4][1])));
	free(val);

	/* write something shorter than orignal */
	val = csc_cfg_copy(root, rdlist[6][0], rdlist[6][1], 0);
	if (val) {
		for (i = 0; val[i]; i++) {
			if ((val[i] >= 'A') && (val[i] <= 'Z')) {
				val[i] += 'a' - 'A';
			} else if ((val[i] >= 'a') && (val[i] <= 'z')) {
				val[i] -= 'a' - 'A';
			}
		}
		csc_cfg_write(root, rdlist[6][0], rdlist[6][1], val);
		CDB_SHOW(("WRITECUT %s: %s = %s\n", rdlist[6][0], rdlist[6][1],
			csc_cfg_read(root, rdlist[6][0], rdlist[6][1])));
		free(val);
	}

	csc_cfg_write_bin(root, "what", "Binary", root, 48);
	val = csc_cfg_copy_bin(root, "what", "Binary", &n);
	CDB_SHOW(("BINARY %s: %s = (%d) ", "what", "Binary", n));
	if (val) {
		for (i = 0; i < n; i++) {
			CDB_SHOW(("%02x ", (unsigned char)val[i]));
		}
		free(val);
	}
	CDB_SHOW(("\n"));

	CDB_SHOW(("\n\n"));
	csc_cfg_dump(root);
	csc_cfg_delete_key(root, NULL, "timestamp");
	csc_cfg_delete_key(root, "hello", "window_state");
	csc_cfg_delete_block(root, "main_define.h");
	CDB_SHOW(("DELETE: timestamp, hello/window_state, [main_define.h]\n"));
	csc_cfg_dump(root);
	CDB_SHOW(("%x\n", csc_cfg_close(root)));
	return 0;
}


int config_block_test(char *fname)
{
	KEYCB	*root;
	char	*fbuf, *kbuf;
	int	i, klen;
	long	flen = 0;

	if ((fbuf = csc_file_load(fname, NULL, &flen)) == NULL) {
		return -1;
	}
	if ((root = csc_cfg_open(SMM_CFGROOT_CURRENT, TESTPATH, TESTINPUT, 
					CSC_CFG_RWC)) == NULL) {
		free(fbuf);
		return -3;
	}
	csc_cfg_write_block(root, fname, fbuf, (int)flen);
	
	kbuf = csc_cfg_copy_block(root, fname, &klen);
	if (klen != (int)flen) {
		CDB_SHOW(("BLOCK [%s]: %d != %ld\n", fname, klen, flen));
	} else if (memcmp(fbuf, kbuf, klen)) {
		for (i = 0; i < klen; i++) {
			if (fbuf[i] != kbuf[i]) {
				break;
			}
		}
		CDB_SHOW(("BLOCK [%s]: %d at %x %x\n", 
					fname, i, fbuf[i], kbuf[i]));
	}
	csc_cfg_close(root);
	
	free(kbuf);
	free(fbuf);
	return 0;
}


int config_registry_test(char *syspath, char *path, char *fname)
{
	KEYCB	*root;
	char	*buf, *p;
	int	sysp, len;

	//CDB_SHOW(("config_registry_test: %s %s %s\n", syspath, path, fname));
	if (!strcmp(syspath, "DESKTOP")) {
		sysp = SMM_CFGROOT_DESKTOP;
	} else if (!strcmp(syspath, "USER")) {
		sysp = SMM_CFGROOT_USER;
	} else if (!strcmp(syspath, "SYSTEM")) {
		sysp = SMM_CFGROOT_SYSTEM;
	} else if (!strcmp(syspath, "CURRENT")) {
		sysp = SMM_CFGROOT_CURRENT;
	} else {
		CDB_SHOW(("Unknown system path - %s\n", syspath));
		return -1;
	}
		
	/* open HKEY_CURRENT_USER\\SOFTWARE\\7-Zip */
	if ((root = csc_cfg_open(sysp, path, fname, CSC_CFG_READ)) == NULL) {
		CDB_SHOW(("Can't open\n"));
		return -1;
	}

	len = smm_config_path(sysp, path, fname, NULL, 0);
	if ((buf = smm_alloc(len)) != NULL) {
		smm_config_path(sysp, path, fname, buf, len);
		CDB_SHOW(("# File System Path: %s\n", buf));
		buf += strlen(buf) + 1;
		p  = buf + strlen(buf) + 1;
		CDB_SHOW(("# Registry Path:    %s\\%s\n", p, buf));
		smm_free(buf);
	}
	
	csc_cfg_saveas(root, SMM_CFGROOT_MEMPOOL, NULL, NULL);
	csc_cfg_close(root);
	return 0;
}

int config_registry_write(char *syspath)
{
	KEYCB	*root;
	int	sysp;

	//CDB_SHOW(("config_registry_write: %s\n", syspath));
	if (!strcmp(syspath, "DESKTOP")) {
		sysp = SMM_CFGROOT_DESKTOP;
	} else if (!strcmp(syspath, "USER")) {
		sysp = SMM_CFGROOT_USER;
	} else if (!strcmp(syspath, "SYSTEM")) {
		sysp = SMM_CFGROOT_SYSTEM;
	} else if (!strcmp(syspath, "CURRENT")) {
		sysp = SMM_CFGROOT_CURRENT;
	} else {
		CDB_SHOW(("Unknown system path - %s\n", syspath));
		return -1;
	}
	
	if ((root = csc_cfg_open(SMM_CFGROOT_MEMPOOL, testconf,
					NULL, CSC_CFG_READ)) == NULL) {
		CDB_SHOW(("Weird\n"));
		return -1;
	}
	//csc_cfg_dump(root);
	csc_cfg_saveas(root, sysp, "FunSight", "Local/Setting");
	csc_cfg_free(root);
	return 0;
}


int config_create_new(void)
{
	char	*path;

	if ((path = csc_strcpy_alloc(TESTPATH, strlen(TESTINPUT)+4)) == NULL) {
		return -1;
	}
	strcat(path, SMM_DEF_DELIM);
	strcat(path, TESTINPUT);

	smm_mkpath(TESTPATH);
	csc_file_store(path, 1, testconf, strlen(testconf));
	free(path);
	return 0;
}

static	struct	cliopt	clist[] = {
	{   0, NULL,        0, "OPTIONS:" },
	{ 'h', "help",      0, "This help" },
	{ 'o', "open-read", 0, "Open the configure file in read-only mode" },
	{ 'r', "registry",  1, "dump the registry MKEY KEY [KEY]" },
	{ 'R', "regwrite",  1, "write to the registry MKEY" },
	{ 'k', "key-test",  0, "Test the key and value pairs" },
	{ 'b', "block",     1, "Test the block in the configure file" },
	{ 'c', "create",    0, "Create a new configure file" },
	{ 'm', "memtest",   0, "Test memory configure" },
	{ 0, NULL, 0, NULL }
};

int config_main(void *rtime, int argc, char **argv)
{
	int	c;
	char	*sdir = NULL;

	if (argc < 2) {
		csc_cli_print(clist, NULL);
		return 0;
	}
	if ((rtime = csc_cli_qopt_open(argc, argv)) == NULL) {
		return -1;
	}
	while ((c = csc_cli_qopt(rtime, clist)) >= 0) {
		switch (c) {
		case 'h':
			csc_cli_print(clist, NULL);
			break;
		case 'o':
			config_open_rdonly();
			break;
		case 'r':
			sdir = csc_cli_qopt_optarg(rtime);
			c = csc_cli_qopt_optind(rtime);
			if (c + 1 == argc) {
				config_registry_test(sdir, NULL, argv[c]);
			} else if (c + 1 < argc) {
				config_registry_test(sdir, argv[c+1], argv[c]);
			} else {
				CDB_SHOW(("%c: DESKTOP/USER/SYSTEM/CURRENT "
							"KEY ...\n"));
			}
			break;
		case 'R':
			sdir = csc_cli_qopt_optarg(rtime);
			config_registry_write(sdir);
			break;
		case 'k':
			config_key_test();
			break;
		case 'b':
			config_block_test(csc_cli_qopt_optarg(rtime));
			break;
		case 'c':
			config_create_new();
			break;
		case 'm':
			config_open_with_directory();
			break;
		default:
			if (csc_cli_qopt_optopt(rtime) == ':') {
				CDB_SHOW(("%c: missing argument\n", c));
			} else {
				CDB_SHOW(("%c: unknown option\n", c));
			}
			csc_cli_print(clist, NULL);
			break;
		}
	}
	csc_cli_qopt_close(rtime);
	return 0;
}

struct	clicmd	config_cmd = {
	"config", config_main, clist, "Testing the configure file management"
};

extern  struct  clicmd  config_cmd;


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

#define	TESTPATH	"longpath/shortpath/config/myconfig"
#define TESTINPUT	"myconfig.test"
#define TESTOUTPUT	"myconfig.out"

static	char	*testconf = "\
window_width=4096\n\
grid_column=690\n\
zoom_height=how do i know\n\
last_directory=/home/xuxx/zvtest\n\
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
last_directory=/HOME/XUM1/vMACHINE/sHARED/EZVTEST\n\
";


static int config_open_rdonly(void)
{
	void	*root;

	if ((root = csc_cfg_open(TESTPATH, TESTINPUT, 1)) == NULL) {
		slogz("can't open\n");
		return -1;
	}
	csc_cfg_dump(root, NULL);
	if (csc_cfg_save(root) == SMM_ERR_NONE) {
		slogz("FATAL: should be read only\n");
		csc_cfg_abort(root);
		return -2;
	}

	csc_cfg_saveas(root, TESTPATH, TESTOUTPUT);
	csc_cfg_abort(root);
	return 0;
}

static int config_key_test(void)
{
	void	*root;
	int	i, n;
	char	*val, *key;
	char	nkey[64], mkey[64];
	time_t	tmtick;
	char	*rdlist[][2] = {
		{ "[hello]", "grid_column" },
		{ "[hello]", "grid_column=4" },
		{ "[hello]", "window_width" },
		{ "[main]", "window_width" },
		{ "[main]", "simple_profile" },
		{ "[what]", "zoom_height" },
		{ "[print]", "last_directory" },
		{ NULL, "window_width" },
		{ NULL, NULL }
	};

	if ((root = csc_cfg_open(TESTPATH, TESTINPUT, 0)) == NULL) {
		slogz("Weird!\n");
		return -1;
	}
	for (i = 0; rdlist[i][0] || rdlist[i][1]; i++) {
		slogz("READ %s: %s = %s\n", rdlist[i][0], rdlist[i][1],
				csc_cfg_read(root, rdlist[i][0], rdlist[i][1]));
	}

	if ((val = csc_cfg_read_first(root, NULL, &key)) != NULL) {
		slogz("READ NULL: %s = %s\n", key, val);
		while ((val = csc_cfg_read_next(root, &key)) != NULL) {
			slogz("READ NULL: %s = %s\n", key, val);
		}
	}

	if ((val = csc_cfg_read_first(root, "[what]", &key)) != NULL) {
		slogz("READ [what]: %s = %s\n", key, val);
		while ((val = csc_cfg_read_next(root, &key)) != NULL) {
			slogz("READ [what]: %s = %s\n", key, val);
		}
	}

	/* read an integer */
	csc_cfg_read_long(root, rdlist[2][0], rdlist[2][1], (long*)&i);
	slogz("READLONG %s: %s = %d\n", rdlist[2][0], rdlist[2][1], i);

	/* write a new main key */
	time(&tmtick);
	sprintf(mkey, "[%u]", (unsigned) tmtick);
	sprintf(nkey, "timestamp");
	csc_cfg_write(root, mkey, nkey, ctime(&tmtick));
	slogz("WRITENEW %s: %s = %s\n", mkey, nkey, 
			csc_cfg_read(root, mkey, nkey));

	/* write to the root key */
	csc_cfg_write(root, NULL, nkey, ctime(&tmtick));
	slogz("WRITEROOT: %s = %s\n", nkey, csc_cfg_read(root, NULL, nkey));

	/* write something longer than orignal */
	val = csc_cfg_copy(root, rdlist[4][0], rdlist[4][1], 64);
	if (val == NULL) {
		val = csc_strcpy_alloc(":appendix", 0);
	} else {
		strcat(val, ":appendix");
	}
	csc_cfg_write(root, rdlist[4][0], rdlist[4][1], val);
	slogz("WRITEEXT %s: %s = %s\n", rdlist[4][0], rdlist[4][1],
			csc_cfg_read(root, rdlist[4][0], rdlist[4][1]));
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
		slogz("WRITECUT %s: %s = %s\n", rdlist[6][0], rdlist[6][1],
				csc_cfg_read(root, rdlist[6][0], rdlist[6][1]));
		free(val);
	}

	csc_cfg_write_bin(root, "[what]", "Binary", root, 48);
	val = csc_cfg_copy_bin(root, "[what]", "Binary", &n);
	slogz("BINARY %s: %s = (%d) ", "[what]", "Binary", n);
	if (val) {
		for (i = 0; i < n; i++) {
			slogz("%02x ", (unsigned char)val[i]);
		}
		free(val);
	}
	slogz("\n");

	csc_cfg_close(root);
	return 0;
}


int config_block_test(char *fname)
{
	char	*fbuf, *kbuf, key[128];
	int	i, klen;
	long	flen = 0;
	void	*root;

	if ((fbuf = csc_file_load(fname, NULL, &flen)) == NULL) {
		return -1;
	}
	if ((root = csc_cfg_open(TESTPATH, TESTINPUT, 0)) == NULL) {
		free(fbuf);
		return -3;
	}
	sprintf(key, "[%s]", fname);
	csc_cfg_write_block(root, key, fbuf, (int)flen);
	
	kbuf = csc_cfg_copy_block(root, key, &klen);
	if (klen != (int)flen) {
		slogz("BLOCK %s: %d != %ld\n", key, klen, flen);
	} else if (memcmp(fbuf, kbuf, klen)) {
		for (i = 0; i < klen; i++) {
			if (fbuf[i] != kbuf[i]) {
				break;
			}
		}
		slogz("BLOCK %s: %d at %x %x\n", key, i, fbuf[i], kbuf[i]);
	}
	csc_cfg_close(root);
	
	free(kbuf);
	free(fbuf);
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

	csc_file_store(path, 1, testconf, sizeof(testconf));
	free(path);
	return 0;
}

int config_main(int argc, char **argv)
{
	while (--argc && (**++argv == '-')) {
		if (!strcmp(*argv, "-h") || !strcmp(*argv, "--help")) {
			slogz("config \n");
			return 0;
		} else if (!strcmp(*argv, "--open-rdonly")) {
			config_open_rdonly();
		} else if (!strcmp(*argv, "--key-test")) {
			config_key_test();
		} else if (!strcmp(*argv, "--block")) {
			argv++;
			argc--;
			if (argc > 0) {
				config_block_test(*argv);
			}
		} else if (!strcmp(*argv, "--create")) {
			config_create_new();
		} else {
			slogz("Unknown option. [%s]\n", *argv);
			return -1;
		}
	}
	/*if (argc > 0) {
	}*/
	return 0;
}


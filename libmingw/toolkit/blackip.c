/*  blackip.c - download/convert/update the IP block list

    Copyright (C) 2015  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of BLACKIP, a utility to process IP block list.

    This program was inspired by "pg2ipset" and "UpdateList.sh".

    BLACKIP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BLACKIP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/* History:
 * 20150920: V1.0 
 *   Function initialized.
 */
/* Install:
 *   gcc -Wall -O3 -DCFG_LIBCURL -o blackip blackip -lz -lcurl
 * or without libcurl:
 *   gcc -Wall -O3 -o blackip blackip -lz
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "zlib.h"

/* 
 * IE6 on Windows XP: 
 *     Mozilla/4.0 (compatible; MSIE 6.0; Microsoft Windows NT 5.1)
 * Firefox on Windows XP: 
 *     Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.14) Gecko/20080404 Firefox/2.0.0.14
 * Firefox on Ubuntu Gutsy: 
 *     Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.14) Gecko/20080418 Ubuntu/7.10 (gutsy) Firefox/2.0.0.14
 * Safari on Mac OS X Leopard: 
 *     Mozilla/5.0 (Macintosh; U; Intel Mac OS X; en) AppleWebKit/523.12.2 (KHTML, like Gecko) Version/3.0.4 Safari/523.12.2
 */
#define CFG_BROWSER		\
	"Mozilla/4.0 (compatible; MSIE 6.0; Microsoft Windows NT 5.1)"

#define	CFG_VERSION		"1.0"

#define	SETMAP_SIZE		(sizeof(unsigned long) * 8)
#define SETMAP_SET(s,n)		(((n) < SETMAP_SIZE) ? ((s) | ((unsigned long)1) << (n)) : (s))
#define SETMAP_CHECK(s,n)	(((n) < SETMAP_SIZE) ? ((s) & ((unsigned long)1) << (n)) : 0)

/* TODO matrix:
 * DOWNLOAD + CONVERT + UPDATE  : alter SETNAME
 * DOWNLOAD + CONVERT           : true SETNAME
 * DOWNLOAD
 * CONVERT + UPDATE             : alter SETNAME + argv
 * CONVERT                      : true SETNAME + argv
 * UPDATE                       : true SETNAME + argv
 */
#define TODO_HELP		1
#define TODO_VERSION		2
#define TODO_CONVERT		4
#define TODO_DOWNLOAD		8
#define TODO_UPDATE		16
#define TODO_PACKLIST		32
#define TODO_END		0x1000


static	struct	{
	char	*name;
	char	*url;
} iblock[] = {
	{ "Bluetack LVL 1",
	  "http://list.iblocklist.com/?list=bt_level1&fileformat=p2p&archiveformat=gz" },
	{ "Bluetack LVL 2",
          "http://list.iblocklist.com/?list=bt_level2&fileformat=p2p&archiveformat=gz" },
	{ "Bluetack badpeers",
          "http://list.iblocklist.com/?list=bt_templist&fileformat=p2p&archiveformat=gz" },
	{ "Bluetack Microsoft",
          "http://list.iblocklist.com/?list=bt_microsoft&fileformat=p2p&archiveformat=gz" },
	{ "Bluetack spider",
          "http://list.iblocklist.com/?list=bt_spider&fileformat=p2p&archiveformat=gz" },
	{ "Bluetac dshield",
          "http://list.iblocklist.com/?list=bt_dshield&fileformat=p2p&archiveformat=gz" },
	{ "TBG Primary Threats",
          "http://list.iblocklist.com/?list=ijfqtofzixtwayqovmxn&fileformat=p2p&archiveformat=gz" },
	{ "TBG General Corporate Range",
          "http://list.iblocklist.com/?list=ecqbsykllnadihkdirsh&fileformat=p2p&archiveformat=gz" },
	{ "TBG Buissness ISPs",
          "http://list.iblocklist.com/?list=jcjfaxgyyshvdbceroxf&fileformat=p2p&archiveformat=gz" },
	{ NULL, NULL }
};

static	char	*help = "\
Usage: blackip COMMAND [OPTION] [input_file] [output_file]\n\
COMMAND:\n\
  -c                convert to IPSET format from transmission format\n\
  -d[0-m,n]         download block list from iblocklist\n\
  -i                update the IPSET rules (root privilige)\n\
  -l                list the block list packages\n\
OPTION:\n\
  -p xx.xx.xx.xx    specify a proxy server\n\
  -s SETNAME        specify the setname of IPSET\n\
EXAMPLE:\n\
  *) blackip -d                      ##Download all block list and save as Transmission format\n\
  *) blackip -l                      ##List the build-in URLs of block lists\n\
  *) blackip -d0-3 -c -s MYIPSET     ##Download 0 to 3 block list and save as the IPSET rules\n\
  *) blackip -d1,3 -c -i -s MYIPSET  ##Download 1 and 3 block list and update the IPSET rules\n\
  *) blackip -c -s MYIPSET ipblock.txt ipset.txt\n\
                                     ##Convert the block lists to the IPSET rules\n\
  *) blackip -c -i -s MYIPSET ipblock.txt ipset.txt\n\
                                     ##Convert the block lists to and update the IPSET rules\n\
  *) blackip -i -s MYIPSET ipset.txt ##Update the IPSET rules\n\
  *) blackip -d0-3 -c -s DOWNLOADED ipset.txt      ##Download IPSET rules as common user\n\
     sudo blackip -i -s MYIPSET ipset.txt          ##Then update the IPSET rules by sudo call\n\
";

static	char	*testcase = "\
001) List the build-in URLs of block lists:\n\
	blackip -l\n\
002) Download all block list and save as Transmission format:\n\
	blackip -d\n\
003) Download 0-3 block list and save as Transmission format:\n\
	blackip -d0-3\n\
004) Download 2,3 block list and save as IPSET rules:\n\
	blackip -d2,3 iblocktmp.gz\n\
	gunzip iblocktmp.gz\n\
	blackip -c -s MYIPSET iblocktmp ipset.txt\n\
	head ipset.txt\n\
005) Download 2,3 block list and write as IPSET rules to stdout:\n\
	blackip -d2,3 iblocktmp.gz\n\
	gunzip iblocktmp.gz\n\
	blackip -c -s MYIPSET iblocktmp\n\
006) Similar to test 005 but includes update (-i) command:\n\
It should be the same to 005 because IPSET rules were sent to stdout.\n\
	blackip -d2,3 iblocktmp.gz\n\
	gunzip iblocktmp.gz\n\
	blackip -c -i -s MYIPSET iblocktmp\n\
007) Similar to test 005 but using the system default SETNAME:\n\
	blackip -d2,3 iblocktmp.gz\n\
	gunzip iblocktmp.gz\n\
	blackip -c iblocktmp\n\
008) Same to test 007 besides update (-i) command:\n\
The result should be same to 007 because IPSET rules were sent to stdout.\n\
	blackip -d2,3 iblocktmp.gz\n\
	gunzip iblocktmp.gz\n\
	blackip -c -i iblocktmp\n\
009) A fix to test 008 to make it work with the update (-i) command:\n\
	blackip -d2,3 iblocktmp.gz\n\
	gunzip iblocktmp.gz\n\
	blackip -c -i iblocktmp ipset.txt\n\
	head ipset.txt\n\
010) Convert and update the IPSET rules with specified SETNAME:\n\
Most useful for root.\n\
	blackip -d2,3 iblocktmp.gz\n\
	gunzip iblocktmp.gz\n\
	blackip -c -i -s MYIPSET iblocktmp ipset.txt\n\
	head ipset.txt\n\
011) Download 2,3 block list, convert to the IPSET rules and load into IPSET:\n\
Most useful for root.\n\
	blackip -d2,3 -i -s MYIPSET\n\
	head iblock_*_ipset.txt\n\
012) Download 0-2 block list, convert to specified file and load into IPSET:\n\
Most useful for rc.local.\n\
	blackip -d0-2 -i -s MYIPSET /tmp/ipset.txt\n\
	head /tmp/ipset.txt\n\
013) Download 2,3 block list and convert to IPSET rules, save to ipset.txt:\n\
Most useful for sudo user.\n\
	blackip -d2,3 -c -s DOWNLOADED ipset.txt\n\
	head ipset.txt\n\
	sudo blackip -i -s MYIPSET ipset.txt\n\
";

static int iblock_convert_to_ipset(char *setname, char *, char *, int);
static int iblock_compress(char *infile, char *outfile);
static int iblock_uncompress(char *infile, FILE *fout);
static int iblock_download(unsigned long setmap, char *outfile, char *proxy);
static int iblock_download_packages(unsigned long setmap, char *proxy);
static int ipset_update(char *rulefile, char *setname, unsigned elem);
static unsigned long set_package_list(char *s);
static void dump_package_list(unsigned long setmap);
static int progress(size_t update);
static int sys_file_delete(char *fname);
static char *sys_timestamp(char *tmbuf);
static int sys_download_url(char *url, char *fname, char *proxy);

static int zlib_deflate(FILE *source, FILE *dest, int level);
static int zlib_inflate(FILE *source, FILE *dest);
static void zlib_zerr(int ret);


static int	simulation = 0;

int main(int argc, char **argv)
{
	unsigned long	setmap = 0;
	unsigned	maxelem = (unsigned)-1;
	int	i, todo = 0;
	char	blockfile[256], midfile[256];
	char	*setname = "IPFILTER";
	char	*proxy = NULL;

	while (--argc && (**++argv == '-')) {
		if (!strcmp(*argv, "--help")) {
			puts(help);
			todo |= TODO_END;
		} else if (!strcmp(*argv, "--help-gzip")) {	/* unit test */
			iblock_compress(*++argv, "test.gz");
			argc--;
			todo |= TODO_END;
		} else if (!strcmp(*argv, "--help-gunzip")) {	/* unit test */
			FILE	*fout = fopen("test.txt", "w");
			iblock_uncompress(*++argv, fout);
			fclose(fout);
			argc--;
			todo |= TODO_END;
		} else if (!strcmp(*argv, "--help-setmap")) {	/* unit test */
			setmap = set_package_list(*++argv);
			dump_package_list(setmap);
			setmap = 0;
			argc--;
			todo |= TODO_END;
		} else if (!strcmp(*argv, "--help-download")) {	/* unit test */
			setmap = set_package_list(*++argv);
			iblock_download_packages(setmap, proxy);
			setmap = 0;
			argc--;
			todo |= TODO_END;
		} else if (!strcmp(*argv, "--help-examples")) {	/* unit test */
			puts(testcase);
			todo |= TODO_END;
		} else  if (!strcmp(*argv, "--version") || !strcmp(*argv, "-V")) {
			puts(CFG_VERSION);
			todo |= TODO_END;
		} else if (!strcmp(*argv, "-c")) {
			todo |= TODO_CONVERT;
		} else if (!strncmp(*argv, "-d", 2)) {
			if (isdigit(argv[0][2])) {
				setmap = set_package_list((*argv) + 2);
			} else {
				setmap = (unsigned long) -1;	/* download all */
			}
			todo |= TODO_DOWNLOAD;
		} else if (!strcmp(*argv, "-i") || !strcmp(*argv, "--ipset")) {
			todo |= TODO_UPDATE;
		} else if (!strcmp(*argv, "-l")) {
			for (i = 0; iblock[i].name; i++) {
				printf("%2d: %s\n", i, iblock[i].name);
			}
			todo |= TODO_END;
		} else if (!strcmp(*argv, "-p")) {
			if (--argc == 0) {
				puts(help);
				return -1;
			}
			proxy = *++argv;
		} else if (!strcmp(*argv, "-s")) {
			if (--argc == 0) {
				puts(help);
				return -1;
			}
			setname = *++argv;
		} else if (!strcmp(*argv, "--simulation")) {
			simulation = 1;
		} else {
			printf("Invalided option [%s]\n", *argv);
			return -1;
		}
	}

	/* DOWNLOAD + CONVERT + UPDATE  : alter SETNAME
	 * DOWNLOAD + CONVERT           : true SETNAME
	 * DOWNLOAD */
	if (todo & TODO_DOWNLOAD) {
		todo |= todo & TODO_UPDATE ? TODO_CONVERT : 0;
		sprintf(midfile, "iblock_%s", sys_timestamp(NULL));
		if (iblock_download(setmap, midfile, proxy) <= 0) {
			return -1;	/* download fail */
		}
		if (argc > 0) {
			strcpy(blockfile, argv[0]);
		} else {
			strcpy(blockfile, midfile);
			if (todo & TODO_CONVERT) {
				strcat(blockfile, "_ipset.txt");
			} else {
				strcat(blockfile, "_transmission.gz");
			}
		}
		if ((todo & TODO_CONVERT) == 0) {
			iblock_compress(midfile, blockfile);
		} else {
			maxelem = iblock_convert_to_ipset(setname, 
					midfile, blockfile, todo&TODO_UPDATE);
			if (todo & TODO_UPDATE) {
				ipset_update(blockfile, setname, maxelem + 1);
			}
		}
		sys_file_delete(midfile);
		return 0;
	}
	
	/* CONVERT + UPDATE             : alter SETNAME + argv
	 * CONVERT                      : true SETNAME + argv */
	if (todo & TODO_CONVERT) {
		if (argc == 0) {
			iblock_convert_to_ipset(setname, NULL, NULL, 0);
		} else if (argc == 1) {
			iblock_convert_to_ipset(setname, argv[0], NULL, 0);
		} else if ((todo & TODO_UPDATE) == 0) {
			iblock_convert_to_ipset(setname, argv[0], argv[1], 0);
		} else {
			maxelem = iblock_convert_to_ipset(setname, argv[0], argv[1], 1);
			ipset_update(argv[1], setname, maxelem + 1);
		}
		return 0;
	}

	if ((todo & TODO_UPDATE) && (argc >= 1)) {	
		/* Update the IPSET only: true SETNAME + argv */
		ipset_update(argv[0], setname, 0);
		return 0;
	}
	if (todo == 0) {
		puts(help);
	}
	return 0;
}

static int iblock_convert_to_ipset(char *setname, 
		char *infile, char *outfile, int tmpflag)
{
	FILE	*fin, *fout;
	char	*line, *lend;
	char	buffer[512];
	int	num;

	fin = fout = NULL;
	if (infile && (*infile != '-')) {
		fin = fopen(infile, "r");
	}
	if (fin == NULL) {
		fin = stdin;
	}
	if (outfile && (*outfile != '-')) {
		fout = fopen(outfile, "w");
	}
	if (fout == NULL) {
		fout = stdout;
	}
	
	num = 0;
	//while (getline(&line, NULL, fin) > 0) {
	while ((line = fgets(buffer, sizeof(buffer), fin)) != NULL) {
		/* skip the leading white spaces */
		while (*line && isspace(*line)) line++;

		/* skip the useless lines */
		if (*line == '#') {
			continue;
		}
		if (*line == 0) {
			continue;
		}
		if (strstr(line, "127.0.0")) {
			continue;
		}
		if ((line = strrchr(line, ':')) == NULL) {
			continue;
		}

		num++;
		line++;
		for (lend = line + strlen(line) - 1; isspace(*lend);  *lend-- = 0);
		if (tmpflag) {
			fprintf(fout, "add -exist %s_TMP %s\n", setname, line);
		} else {
			fprintf(fout, "add -exist %s %s\n", setname, line);
		}
	}
	if (fin != stdin) {
		fclose(fin);
	}
	if (fout != stdout) {
		fclose(fout);
	}
	return num;
}

static int iblock_compress(char *infile, char *outfile)
{
	FILE	*fin, *fout;
	size_t	inlen;
	int	rc;

	if ((fin = fopen(infile, "r")) == NULL) {
		return -1;
	}
	if ((fout = fopen(outfile, "w")) == NULL) {
		fclose(fin);
		return -1;
	}

	fseek(fin, 0, SEEK_END);
	inlen = ftell(fin);
	rewind(fin);

	progress(-1);
	progress(inlen);
	printf("Compressing %s: ", outfile);

	rc = zlib_deflate(fin, fout, Z_DEFAULT_COMPRESSION);
	if (rc != Z_OK) {
		zlib_zerr(rc);
	}

	progress(inlen);
	fclose(fout);
	fclose(fin);
	return rc;
}

static int iblock_uncompress(char *infile, FILE *fout)
{
	FILE	*fin;
	size_t	inlen;
	int	rc;

	if ((fin = fopen(infile, "r")) == NULL) {
		return -1;
	}

	fseek(fin, 0, SEEK_END);
	inlen = ftell(fin);
	rewind(fin);

	progress(-1);
	progress(inlen);
	printf("Uncompressing: ");

	rc = zlib_inflate(fin, fout);
	if (rc != Z_OK) {
		zlib_zerr(rc);
	}

	progress(inlen);
	fclose(fin);
	return rc;
}

static int iblock_download(unsigned long setmap, char *outfile, char *proxy)
{
	FILE	*fp;
	char	tmpfile[128];
	int	i;

	if ((fp = fopen(outfile, "w")) == NULL) {
		return -1;
	}
	for (i = 0; i < SETMAP_SIZE; i++) {
		if (iblock[i].name == NULL) {
			break;
		}
		if (SETMAP_CHECK(setmap, i) == 0) {
			continue;
		}

		sprintf(tmpfile, "/tmp/iblock_%02d.gz", i);
		printf("Downloading '%s': ", iblock[i].name);
		fflush(stdout);
		if (sys_download_url(iblock[i].url, tmpfile, proxy) == 0) {
			iblock_uncompress(tmpfile, fp); /** append to block */
		} else {
			printf("FAILED!\n");
		}
		sys_file_delete(tmpfile);
	}

	i = (int) ftell(fp);
	fclose(fp);

	if (i == 0) {
		sys_file_delete(outfile);
	}
	return i;
}

static int iblock_download_packages(unsigned long setmap, char *proxy)
{
	char	tmpfile[128];
	int	i, rc;

	for (i = rc = 0; i < SETMAP_SIZE; i++) {
		if (iblock[i].name == NULL) {
			break;
		}
		if (SETMAP_CHECK(setmap, i) == 0) {
			continue;
		}

		sprintf(tmpfile, "iblock_%02d.gz", i);
		sys_download_url(iblock[i].url, tmpfile, proxy);
	}
	return rc;
}


static int ipset_update(char *rulefile, char *setname, unsigned maxelem)
{
	FILE	*fp;
	char	buf[256], insetn[128], *p;
	int	rc;

	//printf("ipset_update: %s %s %u\n", rulefile, setname, maxelem);
	/* review the IPSET runtime environment */
	rc = system("ipset list");
	if (rc == 1) {
		printf("IPSET require root privilige.\n");
		if (simulation == 0) {
			return -1;
		}
	}
	if (rc != 0) {	/* such as 127 */
		printf("IPSET is not found.\n");
		if (simulation == 0) {
			return -2;
		}
	}

	if ((fp = fopen(rulefile, "r")) == NULL) {
		return -3;
	}
	
	/* if the setname and rule number are unknown, grab them from the
	 * file:   add -exist MYIPSET 1.0.64.0-1.0.127.255  */
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if ((p = strstr(buf, "-exist")) == NULL) {
			continue;
		}
		for (p += 6; isspace(*p); p++);
		strncpy(insetn, p, sizeof(insetn));
		insetn[sizeof(insetn)-1] = 0;
		for (p = insetn; !isspace(*p); p++);
		*p = 0;
		break;
	}
	if (maxelem == 0) {
		maxelem = 2;
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			if (strstr(buf, "-exist") != NULL) {
				maxelem++;
			}
		}
	}
	fclose(fp);

	if (!strcmp(setname, insetn)) {
		printf("Failure: SETNAME equals to Temporary SETNAME\n");
		return -1;
	}

	sprintf(buf, "ipset create -exist %s hash:net maxelem %u", insetn, maxelem);
	if (simulation == 0) {
		system(buf);
	} else {
		puts(buf);
	}
	
	sprintf(buf, "ipset flush %s", insetn);
	if (simulation == 0) {
		system(buf);
	} else {
		puts(buf);
	}
	
	sprintf(buf, "ipset restore < %s", rulefile);
	if (simulation == 0) {
		system(buf);
	} else {
		puts(buf);
	}
	
	sprintf(buf, "ipset create -exist %s hash:net maxelem %u", setname, maxelem);
	if (simulation == 0) {
		system(buf);
	} else {
		puts(buf);
	}
	
	sprintf(buf, "ipset swap %s %s", setname, insetn);
	if (simulation == 0) {
		system(buf);
	} else {
		puts(buf);
	}
	
	sprintf(buf, "ipset destroy %s", insetn);
	if (simulation == 0) {
		system(buf);
	} else {
		puts(buf);
	}
	return rc;
}

/* 0,2,5-9,11 */
static unsigned long set_package_list(char *s)
{
	unsigned long	setmap = 0;
	int	begin, end;

	if (!strncmp(s, "all", 3)) {
		return (unsigned long) -1;
	}
	while (isdigit(*s)) {	/* find the first number */
		begin = end = (int) strtol(s, NULL, 0);
		/* find the next number */
		while (*s && isdigit(*s)) s++;
		if (*s == '-') {
			s++;
			if (!isdigit(*s)) {
				break;
			}
			end = (int) strtol(s, NULL, 0);
			while (*s && isdigit(*s)) s++;
		}
		/* fill up the list */
		while (begin <= end) {
			setmap = SETMAP_SET(setmap, begin);
			begin++;
		}
		/* go to the next */
		if (*s != ',') {
			break;
		}
		s++;
	}
	return setmap;
}

static void dump_package_list(unsigned long setmap)
{
	int	i;

	for (i = 0; i < SETMAP_SIZE; i++) {
		if (SETMAP_CHECK(setmap, i)) {
			printf("%d ", i);
		}
	}
	printf("\n");
}



#define PROGRSS_LENGTH		50

static int progress(size_t update)
{
	static	size_t	pc, acc, ending;
	size_t	run;

	if (update == (size_t) -1) {	/* reset */
		ending = update;
		return -1;
	}
	if (ending == (size_t) -1) {	/* initialize */
		ending = update;
		pc = acc = 0;
		return 0;
	}

	acc += update;
	run = acc * PROGRSS_LENGTH / ending;
	if (run > PROGRSS_LENGTH) {
		run = PROGRSS_LENGTH;
	}
	if (pc >= run) {
		return (int) pc;
	}
	while (pc < run) {
		printf(".");
		fflush(stdout);
		pc++;
	}
	if (pc >= PROGRSS_LENGTH) {
		printf("\n");
	}
	return (int) pc;
}

static char *sys_timestamp(char *tmbuf)
{
	static  char    unsafe[32];
	struct  tm      tms;
	time_t  now;
	
	time(&now);
	localtime_r(&now, &tms);
	if (tmbuf == NULL) {
		tmbuf = unsafe;
	}
	sprintf(tmbuf, "%04d%02d%02d%02d%02d%02d",
			tms.tm_year + 1900, tms.tm_mon + 1, tms.tm_mday,
			tms.tm_hour, tms.tm_min, tms.tm_sec);
	return tmbuf;
}

static int sys_file_delete(char *fname)
{
	FILE	*fp;

	if ((fp = fopen(fname, "r")) != NULL) {
		fclose(fp);
		if (unlink(fname) < 0) {
			perror(fname);
		}
	}
	return 0;
}


/* proxy: username:passwd@10.20.30.40:1234 or
 * http://hniksic:mypassword@proxy.company.com:8001/ */
#ifdef	CFG_LIBCURL
#include <curl/curl.h>

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}

static int curl_xferinfo(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
		curl_off_t ultotal, curl_off_t ulnow)
{
	size_t	*total = clientp;
	static	curl_off_t	dllast;

	if ((*total == 0) && (dltotal != 0)) {
		progress(dltotal);
		*total = dltotal;
		dllast = 0;
		return 0;
	}
	if (*total == 0) {
		return 0;
	}
	progress(dlnow - dllast);
	dllast = dlnow;
	return 0;
}

static int curl_progress(void *clientp, double dltotal, double dlnow,
		double ultotal, double ulnow)
{
	return curl_xferinfo(clientp, (curl_off_t)dltotal, (curl_off_t)dlnow,
			(curl_off_t)ultotal, (curl_off_t)ulnow);
}

static int sys_download_url(char *url, char *fname, char *proxy)
{
	static	size_t	total_length;
	CURL	*curl_handle;
	FILE	*fout;
	int	rcode;

	curl_global_init(CURL_GLOBAL_ALL);
	
	/* init the curl session */
	if ((curl_handle = curl_easy_init()) == NULL) {
		return -1;	/* libcurl abnormal */
	}
	
	/* open the file */ 
	if ((fout = fopen(fname, "wb")) == NULL) {
		return -2;	/* not received */
	}
	
	/* set URL to get here */
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	
	/* Switch on full protocol/debug output while testing */ 
	//curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
	
	/* follow HTTP 3xx redirects */
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
	
	/* send all data to this function  */ 
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

	/* write the page body to this file handle */ 
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, fout);
	
	/* setup the progress bar */
	progress(-1);
	total_length = 0;
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, curl_progress);
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, &total_length);
#if LIBCURL_VERSION_NUM >= 0x072000
	curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, curl_xferinfo);
	curl_easy_setopt(curl_handle, CURLOPT_XFERINFODATA, &total_length);
#endif
	/* disable progress meter, set to 0L to enable and disable debug output */ 
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);

	/* setup the proxy server if exists */
	if (proxy) {
		curl_easy_setopt(curl_handle, CURLOPT_PROXY, proxy);
	}
	
	/* get it! */ 
	curl_easy_perform(curl_handle);
		
	/* check if the expected file received */
	rcode = 0;
	if (ftell(fout) == 0) {
		rcode = -3;	/* empty income file */
	}

	/* close the header file */ 
	fclose(fout);

	/* cleanup curl stuff */ 
	curl_easy_cleanup(curl_handle);
	return rcode;
}
#else	/* fork() to wget */
static int sys_download_url(char *url, char *fname, char *proxy)
{
	FILE	*fchk;
	char	*wget_tbl[] = { "-O" };
	char	*argv[64] = { "wget", "-U", CFG_BROWSER, "-t", "1" };
	int	i, rcode;

	i = 5;
	if (fname) {
		argv[i++] = wget_tbl[0];
		argv[i++] = fname;
	}
	if (url) {
		argv[i++] = url;
	}
	argv[i++] = NULL;

	//for (i = 0; argv[i]; printf("%s ", argv[i++])); puts("");

	if (fork() == 0) {
		if (proxy && *proxy) {
			setenv("http_proxy", proxy, 1);
			printf("Proxy: %s\n", proxy);
		} else {
			printf("Proxy: none\n");
		}
		execvp(argv[0], argv);
		//execlp("env", "env", NULL);
		return -1;	/* never goes there */
	}

	wait(&rcode);
	printf("WGET returns: %d\n", rcode);
	if (!WIFEXITED(rcode)) {	/* normal return? */
		return -1;	/* killed/signaled/suspended/coredumped */
	}
	if ((rcode = WEXITSTATUS(rcode)) != 0) {	
		/* pass on the return code of wget */
		return rcode;	/* 1,2,...127 */
	}
	/* check if the expected file received */
	if ((fchk = fopen(fname, "r")) == NULL) {
		return -2;	/* not received */
	}
	fseek(fchk, 0, SEEK_END);
	if (ftell(fchk) == 0) {
		rcode = -3;	/* empty income file */
	}
	fclose(fchk);
	return rcode;
}
#endif


/* zpipe.c: example of proper use of zlib's inflate() and deflate()
   Not copyrighted -- provided to the public domain
   Version 1.4  11 December 2005  Mark Adler 
   Version modified for blackip 10 Sept 2015 Andy X */

/* Version history:
   1.0  30 Oct 2004  First version
   1.1   8 Nov 2004  Add void casting for unused return values
                     Use switch statement for inflate() return values
   1.2   9 Nov 2004  Add assertions to document zlib guarantees
   1.3   6 Apr 2005  Remove incorrect assertion in inf()
   1.4  11 Dec 2005  Add hack to avoid MSDOS end-of-line conversions
                     Avoid some compiler warnings for input and output buffers
 */

#include <assert.h>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. Z_DEFAULT_COMPRESSION */
static int zlib_deflate(FILE *source, FILE *dest, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    //ret = deflateInit(&strm, level);
    ret = deflateInit2(&strm, level, Z_DEFLATED, 16+MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

	progress(strm.avail_in);

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
static int zlib_inflate(FILE *source, FILE *dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    //ret = inflateInit(&strm);
    ret = inflateInit2(&strm, 16+MAX_WBITS);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

	progress(strm.avail_in);

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/* report a zlib or i/o error */
static void zlib_zerr(int ret)
{
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}


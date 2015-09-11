
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
#define BROWSER		\
	"Mozilla/4.0 (compatible; MSIE 6.0; Microsoft Windows NT 5.1)"

#define TODO_HELP		1
#define TODO_VERSION		2
#define TODO_CONVERT		4
#define TODO_DOWNLOAD		8
#define TODO_UPDATE		16
#define TODO_PACKLIST		32


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

static	char	*proxy = NULL;
static	char	*setname = "IPFILTER";

/* download block list and save as transmission format: iblock_transmission_20150806120038.gz
 *   blackip -d[] [-t]
 * download block list and save as IPSET format: iblock_ipset_20150806120038.txt
 *   blackip -d[] -i [-s SETNAME] 
 * download block list and save as IPSET format and update the IPSET rule:
 *   blackip -d[] -I [-s SETNAME]
 * convert block list to IPSET format: 
 *   blackip -c [-s SETNAME] [] []
 * update the IPSET rules:
 *   blackip -I [iblock_ipset_20150806120038.txt]
 */
static	char	*help = "\
Usage: blackip COMMAND [OPTION] [input_file] [output_file]\n\
COMMAND:\n\
  -c                convert to IPSET format from transmission format\n\
  -d[0-m,n]         download block list from iblocklist\n\
  -I                update the IPSET rules\n\
  -l                list the block list packages\n\
OPTION:\n\
  -i,--ipset        save iblock to IPSET format\n\
  -t,--transmission save iblock to transmission format\n\
  -p xx.xx.xx.xx    specify a proxy server\n\
  -s SETNAME        specify the setname of IPSET\n\
";

static int iblock_convert_to_ipset(char *infile, char *outfile);
static int iblock_compress(char *infile, char *outfile);
static int iblock_download(int *plist, int plen, char *outfile);
static int iblock_download_append(FILE *fp, char *url);
static int ipset_update(char *blockfile);
static int set_package_list(char *s, int *lbuf, int len);
static int progress(size_t update);
static int sys_file_delete(char *fname);
static char *sys_timestamp(char *tmbuf);
static int sys_download_wget(char *url, char *fname, char *proxy);

static int zlib_deflate(FILE *source, FILE *dest, int level);
static int zlib_inflate(FILE *source, FILE *dest);
static void zlib_zerr(int ret);

int main(int argc, char **argv)
{
	int	i, todo = 0, dlno, dlbuf[32];
	char	blockfile[256], midfile[256];
	int	format = 0;	/* 0: transmission  others: IPSET */

	while (--argc && (**++argv == '-')) {
		if (!strcmp(*argv, "--help")) {
			puts(help);
		} else if (!strcmp(*argv, "-c")) {
			todo = TODO_CONVERT;
		} else if (!strncmp(*argv, "-d", 2)) {
			if (isdigit(argv[0][2])) {
				dlno = set_package_list((*argv) + 2, dlbuf, 32);
			} else {
				dlno = -1;/* download all */
			}
			todo = TODO_DOWNLOAD;
		} else if (!strcmp(*argv, "-I")) {
			todo = TODO_UPDATE;
		} else if (!strcmp(*argv, "-zip")) {
			iblock_compress(*++argv, "test.gz");
			return 0;
		} else if (!strcmp(*argv, "-l")) {
			for (i = 0; iblock[i].name; i++) {
				printf("%2d: %s\n", i, iblock[i].name);
			}
		} else if (!strcmp(*argv, "-i") || !strcmp(*argv, "--ipset")) {
			format = 1;
		} else if (!strcmp(*argv, "-t") || !strcmp(*argv, "--transmission")) {
			format = 0;
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
		} else {
			printf("Invalided option [%s]\n", *argv);
			return -1;
		}
	}

	blockfile[0] = 0;
	if (todo && TODO_DOWNLOAD) {
		if (todo && TODO_UPDATE) {
			format = 1;
		}
		sprintf(midfile, "iblock_%s", sys_timestamp(NULL));
		if (iblock_download(dlbuf, dlno, midfile) > 0) {
			strcpy(blockfile, midfile);
			if (format) {
				strcat(blockfile, "_ipset.txt");
				iblock_convert_to_ipset(midfile, blockfile);
			} else {
				strcat(blockfile, "_transmission.gz");
				iblock_compress(midfile, blockfile);
			}
			sys_file_delete(midfile);
		}
	}
	if (todo && TODO_CONVERT) {
		if (argc == 0) {
			iblock_convert_to_ipset(NULL, NULL);
		} else if (argc == 1) {
			iblock_convert_to_ipset(argv[1], NULL);
		} else {
			iblock_convert_to_ipset(argv[1], argv[2]);
		}
	}
	if (todo && TODO_UPDATE) {
		if (argc == 1) {
			ipset_update(argv[1]);
		} else if (argc > 1) {
			ipset_update(argv[2]);
		} else if (todo && TODO_DOWNLOAD) {
			ipset_update(blockfile);
		} else {
			ipset_update(NULL);
		}
	}
	if (todo == 0) {
		puts(help);
	}
	return 0;
}

static int iblock_convert_to_ipset(char *infile, char *outfile)
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
		fprintf(fout, "add -exist %s %s\n", setname, line);
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

	rc = zlib_deflate(fin, fout, Z_DEFAULT_COMPRESSION);
	if (rc != Z_OK) {
		zlib_zerr(rc);
	}

	progress(inlen);
	fclose(fout);
	fclose(fin);
	return rc;
}

static int iblock_download(int *plist, int plen, char *outfile)
{
	FILE	*fp;
	int	i, rc = 0;

	if ((fp = fopen(outfile, "w")) == NULL) {
		return -1;
	}
	if (plen > 0) {
		for (i = 0; i < plen; i++) {
			rc += iblock_download_append(fp, iblock[plist[i]].url);
		}
	} else {
		for (i = 0; iblock[i].name; i++) {
			rc += iblock_download_append(fp, iblock[i].url);
		}
	}
	fclose(fp);
	if (rc == 0) {
		sys_file_delete(outfile);
	}
	return rc;
}

static int iblock_download_append(FILE *fp, char *url)
{
	char	*tmpfile;

	sys_download_wget(url, tmpfile, proxy);
	/** append to block */
	sys_file_delete(tmpfile);
	return 0;
}

static int ipset_update(char *blockfile)
{
	FILE	*fp;

	if (blockfile == NULL) {
		fp = stdin;
	} else {
		fp = fopen(blockfile, "r");
	}
	if (fp == NULL) {
		return -1;
	}

	/**/

	if (fp != stdin) {
		fclose(fp);
	}
	return 0;
}

/* 0,2,5-9,11 */
static int set_package_list(char *s, int *lbuf, int len)
{
	int	i, begin, end;

	for (i = 0; i < len; i++) {
		/* find the first number */
		if (!isdigit(*s)) {
			i--;	/* skip the broken group */
			break;
		}
		begin = end = (int) strtol(s, NULL, 0);
		/* find the next number */
		while (*s && isdigit(*s)) s++;
		if (*s == '-') {
			s++;
			if (!isdigit(*s)) {
				i--;	/* skip the broken group */
				break;
			}
			end = (int) strtol(s, NULL, 0);
			while (*s && isdigit(*s)) s++;
		}
		/* fill up the list */
		if (begin == end) {
			lbuf[i] = begin;
		} else {
			while ((i < len) && (begin <= end)) {
				lbuf[i++] = begin++;
			}
			i--;	/* remove the last increment */
		}
		/* go to the next */
		if (*s != ',') {
			break;
		}
		s++;
	}
	
	/* for debugging only */
	/*for (end = 0; end <= i; end++) printf("%d ", lbuf[end]);
	printf("\n");*/
	return i+1;
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
		printf("#");
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
static int sys_download_wget(char *url, char *fname, char *proxy)
{
	char	*wget_tbl[] = { "-O" };
	char	*argv[64] = { "wget", "-U", BROWSER, "-t", "1" };
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

	for (i = 0; argv[i]; printf("%s ", argv[i++])); puts("");

	if (fork() == 0) {
		if (proxy && *proxy) {
			setenv("http_proxy", proxy, 1);
			printf("Proxy: %s\n", proxy);
		} else {
			printf("Proxy: none\n");
		}
		execvp(argv[0], argv);
		//execlp("env", "env", NULL);
		return -1;
	}

	wait(&rcode);
	printf("WGET returns: %d\n", rcode);
	return rcode;
}

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


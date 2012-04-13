
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


/* 
 * IE6 on Windows XP: Mozilla/4.0 (compatible; MSIE 6.0; Microsoft Windows NT 5.1)
 * Firefox on Windows XP: Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.14) Gecko/20080404 Firefox/2.0.0.14
 * Firefox on Ubuntu Gutsy: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.14) Gecko/20080418 Ubuntu/7.10 (gutsy) Firefox/2.0.0.14
 * Safari on Mac OS X Leopard: Mozilla/5.0 (Macintosh; U; Intel Mac OS X; en) AppleWebKit/523.12.2 (KHTML, like Gecko) Version/3.0.4 Safari/523.12.2
 */
#define BROWSER		"Mozilla/4.0 (compatible; MSIE 6.0; Microsoft Windows NT 5.1)"

#define MAX_PAGE_SIZE	(64*1024)
#define MAX_URL_LIST	128
#define MAX_CMD_LINE	1024

//#define MAX_PROXY_SIZE	256
//#define MAX_PROXY_LIST	128

#define URL_CMD_FIRST	0
#define URL_CMD_LAST	1
#define URL_CMD_NEXT	2
#define URL_CMD_IMAGE	3
#define URL_CMD_ALL	9

#define EHENTAI_DUMP		1
#define EHENTAI_DUMP_ALL	2
#define EHENTAI_IMAGE		4
#define EHENTAI_RETURN_NEXT	8
#define EHENTAI_START_NEXT	16
#define EHENTAI_DUMP_IMAGE	32

typedef	struct	_EHBUF	{
	char	*urlist[MAX_URL_LIST][2];
	int	keysn[32];
	char	buffer[4];
} EHBUF;

static	char	*proxy[] = {
	"93.95.66.1:8080",	
	"188.254.250.14:8080",
	"188.242.74.148:3128",	/* fast */
	//"94.229.86.110:8080",
	//"90.182.223.85:8888",	/* slow */
	"75.125.163.212:3128",
	"81.20.196.45:8080",
	"75.125.163.210:3128",
	"186.3.41.38:3128",
	"41.234.202.71:8080",

	//"",		/* direct link */
	NULL		/* end of the list */
};

#define	MAX_PROXY_LIST	(sizeof(proxy)/sizeof(char*) - 1)
static	char	*pcur = NULL;
static	int	pidx = -1;	/* -1: not used  0+: rotating */


static int e_hentai_harvest(char *url, int flags);
static int e_hentai_safe_download(char *urbuf, int todo);
static int e_hentai_download(char *urbuf, int todo);
static char *e_hentai_find_url(EHBUF *ehbuf, int cmd);
static EHBUF *e_hentai_url_list(char *htm_file);
static void e_hentai_page_stat(EHBUF *ehbuf);
static int  e_hentai_file_sn(char *url);
static char *e_hentai_url_filename(char *url, char *buffer, int blen);
static int e_hentai_trap(char *url);
static int e_hentai_is_image(char *url);
static int e_hentai_is_image_path(char *url);

static char *url_get_tail(char *url, int sep);
static char *url_get_path(char *url, int pn, char *buf, int blen);
static int url_dict_lookup(char *url, char *dict[]);
static char *url_reform(char *url, char *buffer, int blen);
static char *url_strncpy(char *dest, char *sour, int len);

static int sys_file_delete(char *fname);
static int sys_proxy_dump(void);
static char *sys_proxy(void);
static int sys_proxy_update(void);
static int sys_download_wget(char *url, char *fname, char *proxy);


#if 0
static int url_download(char *url)
{
	char	cmdbuf[1024];
	int	len;

	/* claim it as Mozilla 4.0, try once only */
	sprintf(cmdbuf, "wget -U \'%s\' -t 1 -O %s  ", BROWSER,
			e_hentai_url_filename(url, NULL, 0));
	
	strcat(cmdbuf, "'");
	len = strlen(cmdbuf);
	url_reform(url, cmdbuf + len, len);
	strcat(cmdbuf, "'");

	puts(cmdbuf);
	system(cmdbuf);
	return 0;
}
#endif

char	*help = "\
Usage: hentai [-d][-D][-n][-c][-pXXX] [html_page | URL]\n\
  -n,--next      download from the next page\n\
  -c,--continue  continuiously downloading\n\
  -d,--dump      dump the useful URL in the page\n\
  -D,--dumpall   dump all of URL in the page\n\
  -de            dump the name of the embedded image\n\
  -p             rotating internal proxy list\n\
  -p[0-nn]       specify an internal proxy by number\n\
  -pxx.xx.xx.xx  specify an external proxy server\n\
";

int main(int argc, char **argv)
{
	int	flags = EHENTAI_DUMP | EHENTAI_IMAGE;

	while (--argc && (**++argv == '-')) {
		if (!strcmp(*argv, "--help")) {
			puts(help);
			sys_proxy_dump();
			return 0;
		} else if (!strcmp(*argv, "-d") || !strcmp(*argv, "--dump")) {
			flags &= ~EHENTAI_IMAGE;
			flags |= EHENTAI_DUMP;
		} else if (!strcmp(*argv, "-D") || !strcmp(*argv, "--dumpall")) {
			flags &= ~EHENTAI_IMAGE;
			flags |= EHENTAI_DUMP;
			flags |= EHENTAI_DUMP_ALL;
		} else if (!strcmp(*argv, "-de")) {
			flags &= ~EHENTAI_IMAGE;
			flags &= ~EHENTAI_DUMP;
			flags |= EHENTAI_DUMP_IMAGE;
		} else if (!strcmp(*argv, "-n") || !strcmp(*argv, "--next")) {
			flags |= EHENTAI_START_NEXT;
		} else if (!strcmp(*argv, "-c") || !strcmp(*argv, "--continue")) {
			flags |= EHENTAI_RETURN_NEXT;
		} else if (!strncmp(*argv, "-p", 2)) {
			if (argv[0][2] == 0) {
				pidx = 0;	/* rotating proxy is hooked on */
			} else if (strchr(*argv, '.')) {
				pcur = &argv[0][2];
			} else {
				pidx = strtol(&argv[0][2], NULL, 0);
				pcur = proxy[pidx % MAX_PROXY_LIST];
				pidx = -1;
			}
		} else {
			printf("Invalided option [%s]\n", *argv);
			return -1;
		}
	}
	if (argc == 0) {
		puts(help);
		sys_proxy_dump();
		return -1;
	}

	while (argc--) {
		e_hentai_harvest(*argv++, flags);
	}
	return 0;
}

static int e_hentai_harvest(char *url, int flags)
{
	char	curl[1024];

	if (((flags & EHENTAI_IMAGE) == 0) && !strncmp(url, "http:", 5)) {
		flags |= EHENTAI_IMAGE;
	}
	strcpy(curl, url);

	if (flags & EHENTAI_START_NEXT) {
		if (e_hentai_safe_download(curl, EHENTAI_DUMP | EHENTAI_RETURN_NEXT)) {
			return -1;
		}
	}
	if (flags & EHENTAI_RETURN_NEXT) {
		while (e_hentai_safe_download(curl, flags) == 0) {
			if (pidx >= 0) {
				sleep(1);
			} else {
				sleep(10);
			}
		}
	} else {
		if (e_hentai_safe_download(curl, flags)) {
			return -1;
		}
	}
	return 0;
}

static int e_hentai_safe_download(char *urbuf, int todo)
{
	int	i, rc;

	for (i = 0; i < 5; i++) {
		rc = e_hentai_download(urbuf, todo);
		if (rc <= 0) {
			break;
		}
	}
	if (i == 5) {
		return 1;
	}
	return rc;
}

static int e_hentai_download(char *urbuf, int todo)
{
	EHBUF	*ehbuf;
	char	*url, *cpage, *fname;


	if (strncmp(urbuf, "http:", 5)) {
		cpage = urbuf;		/* pre-downloaded HTML page */
	} else {
		cpage = e_hentai_url_filename(urbuf, NULL, 0);
		sys_file_delete(cpage);
		
		/* download the web page */
		if (sys_download_wget(urbuf, cpage, sys_proxy()) != 0) {
			return 1;	/* proxy fail */
		}
		//url_download(urbuf);	/* download the web page */
	}

	/* ananlysis the web page */
	if ((ehbuf = e_hentai_url_list(cpage)) == NULL) {
		return -1;
	}

	if (todo & EHENTAI_DUMP) {
		printf("Page: %d %d %d %d (%s)\n", ehbuf->keysn[0], ehbuf->keysn[1], 
				ehbuf->keysn[2], ehbuf->keysn[3], cpage);
		url = e_hentai_find_url(ehbuf, URL_CMD_NEXT);
		printf("First Page: %s\n", 
				e_hentai_find_url(ehbuf, URL_CMD_FIRST));
		printf("Last Page:  %s\n", 
				e_hentai_find_url(ehbuf, URL_CMD_LAST));
		printf("Next Page:  %s (%d)\n", 
				url, e_hentai_trap(url));
		printf("Image URL:  %s\n\n", 
				e_hentai_find_url(ehbuf, URL_CMD_IMAGE));
	}
	if (todo & EHENTAI_DUMP_ALL) {
		e_hentai_find_url(ehbuf, URL_CMD_ALL);	
	}
	if (todo & EHENTAI_DUMP_IMAGE) {
		url = e_hentai_find_url(ehbuf, URL_CMD_IMAGE);
		printf("%s\n", e_hentai_url_filename(url, NULL, 0));
	}
	//sleep(1);

	/* download the embeded image */
	if (todo & EHENTAI_IMAGE) {
		if ((url = e_hentai_find_url(ehbuf, URL_CMD_IMAGE)) != NULL) {
			//url_download(url);
			fname = e_hentai_url_filename(url, NULL, 0);
			if (sys_download_wget(url, fname, sys_proxy()) != 0) {
				free(ehbuf);
				return 1;
			}
		} else {
			printf("WARNING: IMAGE NOT FOUND\n");
			e_hentai_find_url(ehbuf, URL_CMD_ALL);
			free(ehbuf);
			return -2;
		}
	}

	if ((todo & EHENTAI_RETURN_NEXT) == 0) {
		free(ehbuf);
		return -3;
	}

	/* check if this is the last page */
	if ((url = e_hentai_find_url(ehbuf, URL_CMD_LAST)) == NULL) {
		free(ehbuf);
		puts("Failed to retrieve the last URL!\n");
		return -4;
	}
	url = e_hentai_url_filename(url, NULL, 0);
	if (!strcmp(cpage, url)) {
		free(ehbuf);
		puts("Reach the last page.\n");
		return -3;
	}

	/* move to the next page */
	if ((url = e_hentai_find_url(ehbuf, URL_CMD_NEXT)) == NULL) {
		free(ehbuf);
		puts("Failed to retrieve the next URL!\n");
		return -3;
	}
	if (e_hentai_trap(url)) {	/* it maybe a trap */
		printf("TRAP: %s\n", url);
		free(ehbuf);
		return -5;
	}

	strcpy(urbuf, url);
	free(ehbuf);
	sys_proxy_update();
	return 0;
}


static char *e_hentai_find_url(EHBUF *ehbuf, int cmd)
{
	char	*dict_first[] = 
	{ "f.png", "p.afk", "M.iha", "7.qqm", "f.lol", "Q.bbq", "a.tlc", "o.ffs", NULL };
	char	*dict_last[]  = 
	{ "l.png", "O.ffs", "T.afk", "d.lol", "x.qqm", "l.iha", "h.bbq", "Z.tlc", NULL };
	char	*dict_next[]  = 
	{ "n.png", "S.bbq", "b.tlc", "P.afk", "F.lol", "W.ffs", "g.qqm", "e.iha", "next.png", NULL };
	int	i;

	for (i = 0; ehbuf->urlist[i][0]; i++) {
		if (ehbuf->urlist[i][1] == NULL) {
			continue;
		}

		switch (cmd) {
		case URL_CMD_FIRST:
			if (url_dict_lookup(ehbuf->urlist[i][1], dict_first)) {
				return ehbuf->urlist[i][0];
			}
			if (e_hentai_file_sn(ehbuf->urlist[i][0]) == 
					ehbuf->keysn[0]) {
				return ehbuf->urlist[i][0];
			}
			break;
		case URL_CMD_LAST:
			if (url_dict_lookup(ehbuf->urlist[i][1], dict_last)) {
				return ehbuf->urlist[i][0];
			}
			if (e_hentai_file_sn(ehbuf->urlist[i][0]) == 
					ehbuf->keysn[3]) {
				return ehbuf->urlist[i][0];
			}
			break;
		case URL_CMD_NEXT:
			if (url_dict_lookup(ehbuf->urlist[i][1], dict_next)) {
				return ehbuf->urlist[i][0];
			}
			if (e_hentai_file_sn(ehbuf->urlist[i][0]) == 
					ehbuf->keysn[2]) {
				return ehbuf->urlist[i][0];
			}
			break;
		case URL_CMD_IMAGE:
			if (url_dict_lookup(ehbuf->urlist[i][1], dict_next)) {
				break;
			}
			if (url_dict_lookup(ehbuf->urlist[i][1], dict_first)) {
				break;
			}
			if (url_dict_lookup(ehbuf->urlist[i][1], dict_last)) {
				break;
			}
			if (e_hentai_file_sn(ehbuf->urlist[i][0]) != 
					ehbuf->keysn[2]) {
				break;
			}
			if (!e_hentai_is_image(ehbuf->urlist[i][1])) {
				break;
			}
			if (!e_hentai_is_image_path(ehbuf->urlist[i][1])) {
				break;
			}
			if (cmd == URL_CMD_NEXT) {
				return ehbuf->urlist[i][0];
			} else {
				return ehbuf->urlist[i][1];
			}
		default:	/* URL_CMD_ALL */
			puts(ehbuf->urlist[i][0]);
			printf("----> %s\n", ehbuf->urlist[i][1]);
			break;
		}
	}
	return NULL;
}

static EHBUF *e_hentai_url_list(char *htm_file)
{
	EHBUF	*ehbuf;
	FILE	*fp;
	char	*p, *q;
	int	n;

	if ((fp = fopen(htm_file, "r")) == NULL) {
		perror(htm_file);
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	n = ftell(fp);
	rewind(fp);

	if ((ehbuf = calloc(sizeof(EHBUF) + n, 1)) == NULL) {
		fclose(fp);
		return NULL;
	}
	fread(ehbuf->buffer, 1, n, fp);
	fclose(fp);

	p = ehbuf->buffer;
	n = 0;
	while ((p = strstr(p, "a href=\"")) != NULL) {
		p += 8;
		ehbuf->urlist[n][0] = p;
		if ((q = strchr(p, '"')) == NULL) {
			break;
		}
		*q++ = 0;

		p = q;
		if (!strncmp(p, "><img src=\"", 11)) {
			p += 11;
			ehbuf->urlist[n][1] = p;
			if ((q = strchr(p, '"')) == NULL) {
				break;
			}
			*q++ = 0;
			p = q;
			n++;
			if (n >= MAX_URL_LIST) {
				break;
			}
		}
	}

	e_hentai_page_stat(ehbuf);
	return ehbuf;
}

static void e_hentai_page_stat(EHBUF *ehbuf)
{
	char	*keystr[4], *s;
	int	i, k, n, rc;

	for (i = 0; i < 4; i++) {
		keystr[i] = NULL;
		ehbuf->keysn[i] = 0;
	}
	for (i = n = 0; ehbuf->urlist[i][0]; i++) {
		if (ehbuf->urlist[i][1] == NULL) {
			continue;
		}
		if ((rc = e_hentai_file_sn(ehbuf->urlist[i][0])) < 0) {
			continue;
		}
		/* quick fix to this type:
		 * http://g.e-hentai.org/s/72l037b429/436259-135
		 * ----> http://g.ehgt.org/img/n/next.png
		 *  http://g.e-hentai.org/s/3ff0e48c9a/436259-1
		 *  ----> http://g.ehgt.org/img/f.png
		 *  http://g.e-hentai.org/s/dcf6300cdd/436259-133
		 *  ----> http://g.ehgt.org/img/p.png
		 *  http://g.e-hentai.org/s/721037b429/436259-135
		 *  ----> http://g.ehgt.org/img/n.png
		 *  ...  */
		if ((n == 0) && (rc > 4)) {
		       continue;
		}	       
		s = url_get_tail(ehbuf->urlist[i][1], '/');
		for (k = 0; k < n; k++) {
			if (!strcmp(keystr[k], s)) {
				break;
			}
		}
		if (k == n) {
			keystr[k] = s;
			ehbuf->keysn[k] = rc;
			n++;
			if (n == 4) {
				break;
			}
		}
	}
	for (i = 0; i < 3; i++) {
		for (k = i + 1; k < 4; k++) {
			if (ehbuf->keysn[i] > ehbuf->keysn[k]) {
				rc = ehbuf->keysn[i];
				ehbuf->keysn[i] = ehbuf->keysn[k];
				ehbuf->keysn[k] = rc;
			}
		}
	}
}

static int  e_hentai_file_sn(char *url)
{
	char	*mod;

	mod = url_get_tail(url, '/');
	if ((mod = strchr(mod, '-')) == NULL) {
		return -1;
	}
	return (int) strtol(mod+1, NULL, 0);
}

/* http://50.7.233.114/ehg/image.php?f=5154150da59ceb92e8ce9fd13ef7bab7615cd
 *   b58-503119-892-1237-png&amp;t=370249-94283429070c8dadfba302cdbdd75f3dfb
 *   dedc12&amp;n=039.png
 * http://80.222.98.188:5362/h/f4379bf970692e51f925ff40b1957171f60f37b2-5261
 *   65-1053-1500-jpg/keystamp=1332992557-7151651d63/otakumatsuri8_28.jpg
*/
static char *e_hentai_url_filename(char *url, char *buffer, int blen)
{
	char	*p;

	if (strstr(url, "image.php")) {
		p = url_get_tail(url, '=');
	} else {
		p = url_get_tail(url, '/');
	}
	if (buffer) {
		url_strncpy(buffer, p, blen);
		p = buffer;
	}
	return p;
}

static int e_hentai_trap(char *url)
{
	char	*p, tmp[512];

	if (url == NULL) {
		return 0;
	}

	url_strncpy(tmp, url, 512);

	p = url_get_tail(tmp, '/');
	p--;
	*p = 0;
	p = url_get_tail(tmp, '/');
	//puts(p);

	for ( ; *p; p++) {
		if (!isxdigit(*p)) {
			return 1;
		}
	}
	return 0;
}

static int e_hentai_is_image(char *url)
{
	if ((url = strrchr(url, '.')) == NULL) {
		return 0;	/* no image in the URL */
	}
	url++;
	if (!strcmp(url, "jpg") || !strcmp(url, "jpe")) {
		return 1;
	}
	if (!strcmp(url, "gif")) {
		return 1;
	}
	if (!strcmp(url, "png")) {
		return 1;
	}
	if (!strcmp(url, "bmp")) {
		return 1;
	}
	return 0;
}

static int e_hentai_is_image_path(char *url)
{
	char	tmp[32];

	if (url_get_path(url, 1, tmp, sizeof(tmp)) == NULL) {
		return 0;
	}
	if (!strcmp(tmp, "h")) {
		return 1;
	}
	if (!strcmp(tmp, "ehg")) {
		return 1;
	}
	return 0;
}

static char *url_get_tail(char *url, int sep)
{
	char	*p;

	/* remove the trailing / */
	if (url[strlen(url)-1] == sep) {
		url[strlen(url)-1] = 0;
	}
	if ((p = strrchr(url, sep)) == NULL) {
		return url;
	}
	return p + 1;
}

static char *url_get_path(char *url, int pn, char *buf, int blen)
{
	char	*p;
	int	i;

	if ((p = strstr(url, "://")) == NULL) {
		p = url;
	} else {
		p += 3;
	}
	for (i = 0; i < pn; i++) {
		if ((p = strchr(p, '/')) == NULL) {
			return NULL;
		}
		p++;
	}
	for (blen--, i = 0; i < blen; i++) {
		if ((p[i] == 0) || (p[i] == '/')) {
			break;
		}
		buf[i] = p[i];
	}
	buf[i] = 0;
	//printf("url_get_path [%d]: %s\n", pn, buf);
	return buf;
}

static int url_dict_lookup(char *url, char *dict[])
{
	int	i;

	url = url_get_tail(url, '/');
	for (i = 0; dict[i]; i++) {
		//printf("%s --> %s\n", url, dict[i]);
		if (!strcmp(url, dict[i])) {
			return 1;
		}
	}
	return 0;
}

/* http://50.7.233.114/ehg/image.php?f=5154150da59ceb92e8ce9fd13ef7bab7615cd
 *   b58-503119-892-1237-png&amp;t=370249-94283429070c8dadfba302cdbdd75f3dfb
 *   dedc12&amp;n=039.png */
static char *url_reform(char *url, char *buffer, int blen)
{
	static	char	tmpbuf[1024];
	int	i, k;

	if (buffer == NULL) {
		buffer = tmpbuf;
		blen   = sizeof(tmpbuf);
	}
	for (i = k = 0; url[k]; i++, k++) {
		buffer[i] = url[k];
		if (!strncmp(&url[k], "&amp;", 5)) {
			k += 4;
		}
	}
	buffer[i++] = 0;
	return buffer;
}

static char *url_strncpy(char *dest, char *sour, int len)
{
	strncpy(dest, sour, len -1);
	dest[len -1] = 0;
	return dest;
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

static int sys_proxy_dump(void)
{
	int	i;

	for (i = 0; i < MAX_PROXY_LIST; i++) {
		printf("%d: %s\n", i, proxy[i]);
	}
	return 0;
}

static char *sys_proxy(void)
{
	if (pidx >= 0) {
		pcur = proxy[pidx];
	}
	if (pcur && (*pcur == 0)) {
		return NULL;
	}
	return pcur;
}

static int sys_proxy_update(void)
{
	int	i;

	if (pidx < 0) {
		return pidx;
	}
	for (i = 0; i < MAX_PROXY_LIST; i++) {
		pidx = (pidx + 1) % MAX_PROXY_LIST;
		if (proxy[pidx]) {
			break;
		}
	}
	return pidx;
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
		argv[i++] = url_reform(url, NULL, 0);
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


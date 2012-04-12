
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


#define THUMBDIR	".thumbnail"
#define HTMLFILE	"index.htm"
#define COLUMNNO	5

static	char	wpath[4096];	/* WORKPATH/ */
static	char	tpath[4096];	/* WORKPATH/.thumbnail/ */

static	FILE	*fout;
static	int	column;

static 	char	*http_head = "\
<html>\n\
<head>\n\
  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n\
  <meta name=\"author\" value=\"xuming\">\n\
  <meta name=\"keywords\" content=\"family album\">\n\
  <meta name=\"generator\" content=\"all materials were created by bare hand\">\n\
  <title>Xu Ming's Family Album</title>\n\
</head>\n\
\n\
<body link=\"blue\" text=\"#000000\" vlink=\"brown\" bgcolor=\"#ffffff\">\n\
";

static	char	*table_head = "\
<table width=\"100%%\" border=\"0\" cellspacing=\"4\"><tbody>\n\
  <tr><td><font size=\"+3\" color=\"black\">%s</font></td></tr>\n\
";

static	char	*table_body = "\
    <td align=\"center\"><a href=\"%s\">\n\
      <img src=\"%s\"></a><br>\n\
      <a href=\"%s\">\n\
      <font size=\"+0\" color=\"blue\">%s</font></a></td>\n\
";

static int recursive(char *path);
static int process(char *fname);
static int thumb_path(char *path, char *thumb);
static int file_exist(char *fname);
static int file_filter_video(char *fname);
static int file_filter_image(char *fname);

int main(int argc, char **argv)
{
	strcpy(wpath, ".");

	while (--argc && **++argv == '-') {
		if (!strcmp(*argv, "-w")) {
			--argc;
			++argv;
			strncpy(wpath, *argv, sizeof(wpath)-1);
		}
	}

	if (wpath[strlen(wpath)-1] == '/') {
		wpath[strlen(wpath)-1] = 0;
	}
	strcpy(tpath, wpath);
	strcat(tpath, "/");
	strcat(tpath, HTMLFILE);
	if ((fout = fopen(tpath, "w")) == NULL) {
		perror(tpath);
		return -1;
	}
	fputs(http_head, fout);

	if (chdir(wpath) >= 0) {
		recursive(".");
	}

	fputs("</body>\n</html>\n", fout);
	return 0;
}


static int recursive(char *path)
{
	DIR	*dir;
	struct	dirent	*de;
	char	*tmpdir;
	int	dlen;

	if ((dir = opendir(path)) == NULL)  {
		perror("opendir");
		return -1;
	}

	thumb_path(path, tpath);
	if (!strcmp(path, ".")) {
		fprintf(fout, table_head, "Family Album");
	} else {
		fprintf(fout, table_head, path);
	}
	column = 0;

	printf("Entering %s:\n", path);
	while((de = readdir(dir)) != NULL)  {
		if (de->d_name[0] == '.') {
			continue;
		}

		dlen = strlen(path) + strlen(de->d_name) + 16;
		if ((tmpdir = malloc(dlen)) == NULL) {
			break;
		}
		if (!strcmp(path, ".")) {
			tmpdir[0] = 0;
		} else {
			strcpy(tmpdir, path);
			strcat(tmpdir, "/");
		}
		strcat(tmpdir, de->d_name);

		if (file_exist(tmpdir) == 0) {
			process(tmpdir);
		}

		free(tmpdir);
	}
	if (column && ((column % COLUMNNO) != 0)) {
		fputs("  </tr>\n", fout);
	}
	fputs("</tbody></table><p>\n\n", fout);

	rewinddir(dir);
	while((de = readdir(dir)) != NULL)  {
		if (de->d_name[0] == '.') {
			continue;
		}

		dlen = strlen(path) + strlen(de->d_name) + 16;
		if ((tmpdir = malloc(dlen)) == NULL) {
			break;
		}
		if (!strcmp(path, ".")) {
			tmpdir[0] = 0;
		} else {
			strcpy(tmpdir, path);
			strcat(tmpdir, "/");
		}
		strcat(tmpdir, de->d_name);

		if (file_exist(tmpdir) == 1) {
			recursive(tmpdir);
		}

		free(tmpdir);
	}
	closedir(dir);
	printf("Leaving %s\n", path);
	return 0;
}

static int process(char *fname)
{
	char	tmpdir[4096], cmd[4096];
	char	*p;

	if (!file_filter_video(fname) && !file_filter_image(fname)) {
		return 0;
	}

	thumb_path(fname, tmpdir);
	if ((p = strrchr(tmpdir, '.')) == NULL) {
		strcat(tmpdir, "000.jpg");
	} else {
		strcpy(p, "000.jpg");
	}

	if (file_exist(tmpdir) < 0) {
		printf("Updating %s ...\n", fname);

		if (file_filter_video(fname)) {
			sprintf(cmd, "ezthumb -p key@1 -o \"%s\" \"%s\"", tpath, fname);
		} else {
			sprintf(cmd, "convert \"%s\" -scale 10%% \"%s\"", fname, tmpdir);
		}
		//puts(cmd);
		if (system(cmd) < 0) {
			perror("system");
		}
	}

	/* output to the html file */
	if ((column % COLUMNNO) == 0) {
		fputs("  <tr>\n", fout);
	}
	if ((p = strrchr(fname, '/')) == NULL) {
		p = fname;
	} else {
		p++;
	}
	fprintf(fout, table_body, fname, tmpdir, fname, p);
	if ((column % COLUMNNO) == (COLUMNNO - 1)) {
		fputs("  </tr>\n", fout);
	}
	column++;
	return 1;
}

static int thumb_path(char *path, char *thumb)
{
	/*if (strncmp(path, wpath, strlen(wpath))) {
		return -1;
	}*/

	//printf("thumb_path::Path:  %s\n", path);

	/*path += strlen(wpath);
	strcpy(thumb, wpath);
	strcat(thumb, "/");
	strcat(thumb, THUMBDIR);
	strcat(thumb, path);*/
	if (!strcmp(path, ".")) {
		strcpy(thumb, THUMBDIR);
	} else {
		strcpy(thumb, THUMBDIR);
		strcat(thumb, "/");
		strcat(thumb, path);
	}
	//printf("thumb_path::Thumb: %s\n", thumb);

	if (file_exist(thumb) < 0) {
		mkdir(thumb, 0755);
	}
	return 0;
}

static int file_exist(char *fname)
{
	struct	stat	fs;

	if (lstat(fname, &fs) < 0) {
		return -1;	/* file not existed or access denied */
	}
	if (S_ISDIR(fs.st_mode)) {
		return 1;	/* file exists and is a directory */
	}
	return 0;
}

static int file_filter_video(char *fname)
{
	int	i;

	i = strlen(fname);
	if (!strcasecmp(fname + i - 4, ".avi")) {
		return 1;
	}
	if (!strcasecmp(fname + i - 4, ".wmv")) {
		return 1;
	}
	if (!strcasecmp(fname + i - 4, ".mpg")) {
		return 1;
	}
	if (!strcasecmp(fname + i - 5, ".mpeg")) {
		return 1;
	}
	return 0;
}

static int file_filter_image(char *fname)
{
	int	i;

	i = strlen(fname);
	if (!strcasecmp(fname + i - 4, ".jpg")) {
		return 1;
	}
	if (!strcasecmp(fname + i - 4, ".png")) {
		return 1;
	}
	if (!strcasecmp(fname + i - 4, ".bmp")) {
		return 1;
	}
	if (!strcasecmp(fname + i - 5, ".jpeg")) {
		return 1;
	}
	return 0;
}




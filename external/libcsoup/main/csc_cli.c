
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "libcsoup.h"
#include "libcsoup_debug.h"

static	char	*argpool[] = {
	"The quick brown fox jumps over the lazy dog",
	"The quick \" brown fox jumps \" over the 'lazy dog'",
	"The quick brown fox jumps |over the lazy dog",
	"The quick brown fox jumps over >the lazy dog",
	NULL
};

static	struct	cliopt	short_list[] = {
	{   0, NULL,      0, "OPTIONS:" },
	{ 'c', NULL,      0, "change current working directory" },
	{ 'f', NULL,      1, "configure file process" },
	{ 'm', NULL,      2, "make directory" },
	{  0,  NULL,     -1, "This is the second line" },
	{  0,  NULL, 0, NULL }
};

static	struct	cliopt	long_list[] = {
	{   0, NULL,      0, "OPTIONS:" },
	{   0,  NULL,     -1, "This is the second line" },
	{   1, "help",     0, "Display the help message" },
	{   2, "version",  1, "Display the version message" },
	{   3, "dir-fifo", 2, NULL },
	{   4, "superlongbutuselessoptiontotestthelength", 3, "*help" }, 
	{  0,  NULL, 0, NULL }
};

static	struct	cliopt	mixed_list[] = {
	{   0, NULL,      0, "OPTIONS:" },
	{ 'm', NULL,      0, "test the csc_strcpy_alloc()" },
	{ 'p', NULL,      0, "test the csc_cli_print()" },
	{ 'c', "classic", 0, "test the classic table and list" },
	{ '1', "one",     1, "test with one argument" },
	{ 'o', "opt",     2, "test the optional argument", },
	{   1, "version", 0, "Display the version message" },
	{   2, "help",    0, "Display the help message" },
	{   0,  NULL,    -1, "The optional argument should be either -oxxx or '--one=xxx'" },
	{  0,  NULL, 0, NULL }
};


static int make_arguments(void)
{
	char	*argv[32], *s;
	int	i, k, argc;

	for (i = 0; argpool[i]; i++) {
		CDB_SHOW(("INPUT: %s\n", argpool[i]));
		s = csc_strcpy_alloc(argpool[i], 0);
		argc = csc_cli_mkargv(s, argv, 32);
		for (k = 0; k < argc; k++) {
			CDB_SHOW(("[%2d]: %s\n", k, argv[k]));
		}
		free(s);
	}
	return 0;
}

static int myputs(char *s)
{
	puts(s);
	return 0;
}

static int print_options(void)
{
	csc_cli_print(short_list, NULL);
	csc_cli_print(long_list, myputs);
	csc_cli_print(mixed_list, NULL);
	return 0;
}

static int generate_options(struct cliopt *clist)
{
	struct	option	optbl[64];
	char	oplst[128], tmp[8];
	int	i;

	csc_cli_make_list(clist, oplst, sizeof(oplst));
	CDB_SHOW(("OPTS: \"%s\"\n", oplst));

	csc_cli_make_table(clist, optbl, 64);
	for (i = 0; optbl[i].name; i++) {
		if (isgraph(optbl[i].val)) {
			sprintf(tmp, "%c", optbl[i].val);
		} else {
			sprintf(tmp, "\\%d", optbl[i].val);
		}
		csc_strfill(tmp, 4, ' ');
		CDB_SHOW(("[%2d]: %s %d %s\n", i, tmp, 
				optbl[i].has_arg, optbl[i].name));
	}
	return 0;
}

int csc_cli_main(void *rtime, int argc, char **argv)
{
	void	*argp;
	int	c;

	/* stop the compiler complaining */
	(void) rtime;
	
	if (argc < 2) {
		csc_cli_print(mixed_list, NULL);
		return 0;
	}
	if ((argp = csc_cli_getopt_open(mixed_list)) == NULL) {
		return -1;
	}
	while ((c = csc_cli_getopt(argc, argv, argp)) > 0) {
		switch (c) {
		case 'm':
			make_arguments();
			break;
		case 'p':
			print_options();
			break;
		case 'c':
			generate_options(mixed_list);
			break;
		case '1':
			CDB_SHOW(("One ARG: %s\n", optarg));
			break;
		case 'o':
			CDB_SHOW(("Optional ARG: %s\n", optarg));
			break;
		case 2:
			csc_cli_print(mixed_list, NULL);
			break;
		default:
			CDB_SHOW(("Unknown: %c %c\n", c, optopt));
			break;
		}
	}
	csc_cli_getopt_close(argp);
	return 0;
}

int csc_cli_main2(void *rtime, int argc, char **argv)
{
	void	*argp;
	int	c;

	/* stop the compiler complaining */
	(void) rtime;
	
	if (argc < 2) {
		csc_cli_print(mixed_list, NULL);
		return 0;
	}
	if ((argp = csc_cli_qopt_open(argc, argv)) == NULL) {
		return -1;
	}
	while ((c = csc_cli_qopt(argp, mixed_list)) > 0) {
		switch (c) {
		case 'm':
			make_arguments();
			break;
		case 'p':
			print_options();
			break;
		case 'c':
			generate_options(mixed_list);
			break;
		case '1':
			CDB_SHOW(("One ARG: %s\n", csc_cli_qopt_optarg(argp)));
			break;
		case 'o':
			CDB_SHOW(("Optional ARG: %s\n", 
					csc_cli_qopt_optarg(argp)));
			break;
		case 2:
			csc_cli_print(mixed_list, NULL);
			break;
		default:
			CDB_SHOW(("Unknown: %c %c\n", 
					c, csc_cli_qopt_optopt(argp)));
			break;
		}
	}
	c = csc_cli_qopt_optind(argp);
	csc_cli_qopt_close(argp);

	CDB_SHOW(("REST: %d %s\n", c, argv[c]));
	return 0;
}

struct	clicmd	cli_cmd = {
	"cli", csc_cli_main, mixed_list, "Testing the command line interface functions"
};
struct	clicmd	cli_cmd2 = {
	"cli2", csc_cli_main2, mixed_list, "Testing the command line interface qopt"
};


extern  struct  clicmd  cli_cmd;
extern  struct  clicmd  cli_cmd2;



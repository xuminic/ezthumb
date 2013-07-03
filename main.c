
/*  main.c - entry pointer
 
    Copyright (C) 2011  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of EZTHUMB, a utility to generate thumbnails

    EZTHUMB is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    EZTHUMB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>

#include "ezthumb.h"
#ifdef	CFG_GUI_ON
#include "ezgui.h"
#endif


#define TODO_ERROR	-2
#define TODO_UNSET	-1
#define TODO_UNKNOWN	0
#define TODO_HELP	1
#define TODO_VERSION	2
#define TODO_QK_VER	3
#define TODO_PROF_TST	23




static	struct	cliopt	clist[] = {
	{ 0, NULL, 0, "Usage: ezthumb [OPTIONS] video_clip ..." },
	{ 0, NULL, 0, "OPTIONS:" },
	{ 'b', "bind",    0, "binding multiple video sources" },
	{ 'c', "colour",  2, "the colour setting (MI:TM:BG)(RRGGBB)" },
	{ 'd', "during",  2, "the duration finding mode (head)(fast|scan)" },
	{ 'f', "font",    2, "the TrueType font name with the full path" },
	{ 'F', "fontsize",2, "the size setting of the font" },
	{ 'g', "grid",    2, "the thumbnail grid in the canvas." },
#ifdef	CFG_GUI_ON
	{ 'G', "gui",     0, "enable the graphic user interface" },
#endif
	{ 'i', "list",    0, "display the media information in list form" },
	{ 'I', "info",    0, "display the media information" },
	{ 'm', "format",  2, "the output format (jpg@85)" },
	{ 'o', "outdir",  2, "the directory for storing output images" },
	{ 'p', "process", 1, 
		"the process method (skim|scan|2pass|heuri|safe|key[@N])" },
	{ 'P', "profile", 2, "specify the profile string" },
	{ 'R', "recursive", 0, "process files and directories recursively" },
	{ 's', "ssize",   2, "the size of each screen shots (WxH|RR%)" },
	{ 't', "timestep",1, "the time step between each shots in ms" }, 
	{ 'v', "verbose", 1, "*verbose mode (0)(0-7)" },
	{ 'w', "width",   1, "the whole width of the thumbnail canvas" },
	{ 'x', "suffix",  2, "the suffix of output filename (_thumb)" },
	{   6, "accurate", 0, "take accurate shots including P-frames" },
	{   7, "background", 2, "the background picture" },
	{  14, "decode-otf", 0, "decoding on the fly mode for scan process" },
	{  25, "depth",   1, "most levels of directories recursively (FF:0)" },
	{  21, "edge",    1, "the width of the screen shot edge (0)" },
	{  26, "filter",  1, "the filter of the extended file name" },
	{   8, "gap-shots",  1, "the gaps between the screen shots (4)" },
	{   9, "gap-margin", 1, "the margin in the canvas (8)" },
	{  10, "opt-info", 2, "the media infomation (on)" },
	{  11, "opt-time", 2, "the timestamp inside the screen shots (on)" },
	{  12, "opt-ffr",  2, "start from the first frame (off)" },
	{  13, "opt-lfr",  2, "end at the last frame (off)" },
	{  24, "override", 2, "override existed thumbnails (copy)"},
	{  15, "pos-bg",   2, "the position of the background image (mc)" },
	{  16, "pos-time", 2, "the position of the timestamp (rt)" },
	{  17, "pos-info", 2, "the position of the media infomation (lt)" },
	{   0,  NULL, -1, "lt,lc,lb,mt,mc,mb,rt,rc,rb,tt and st,ex,ey,sx,sy" },
	{  18, "time-from",2, "the time in video where begins shooting" },
	{  19, "time-end", 2, "the time in video where ends shooting" },
	{  20, "transparent", 0, "generate the transparent background" },
	{  22, "vindex",   1, "the index of the video stream" },
	{   1, "help",    0, "*Display the help message" },
	{   2, "version", 0, "*Display the version message" },
	{   3, "vernum",  0, "*Display the version number" },
	{  23, "protest",  2, "*testing the profile (@length, +width)" },
	{ 0, NULL, 0, NULL }
};


static	char	*version = "\
ezthumb %s, to generate the thumbnails from video files.\n\n\
Copyright (C) 2011 \"Andy Xuming\" <xuming@users.sourceforge.net>\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\n";


/* predefined profiles */
static	char	*sysprof[] = {
	"8M4x2:10L10x100x1.027:100R4x320",
	"12M4x4:20M4x6:30M4x8:40M4x10:60M4x12:90M4x16:120M4x20:180M4x24:"
		"160w200%:400w100%:640w50%:720w40%:1280w25%:1600w20%:1920w15%",
	"12M3x4:20M3x6:30M3x8:40M3x10:60M3x12:90M3x16:120M3x20:180M3x24:"
		"160w200%:400w100%:640w50%:720w40%:1280w25%:1600w20%:1920w15%",
	"12M6x4:20M6x6:30M6x8:40M6x10:60M6x12:90M6x16:120M6x20:180M6x24:"
		"160w125%:400w50%:640w35%:720w30%:1280w15%:1600w10%:1920w8%"
};

#define PROFLIST	(sizeof(sysprof)/sizeof(int*))

static	EZOPT	sysopt;


static int command_line_parser(int argc, char **argv, EZOPT *opt);
static int signal_handler(int sig);
static int main_close(EZOPT *opt);
static int msg_info(void *option, char *path, int type, void *info);
static int msg_shot(void *option, char *path, int type, void *info);
static int env_init(EZOPT *ezopt);
static int para_get_ratio(char *s);
static int para_get_time_point(char *s);
static int para_get_position(char *s);
static int para_make_postition(char *s);
static int para_get_color(EZOPT *opt, char *s);
static int para_get_fontsize(EZOPT *opt, char *s);
static int event_cb(void *vobj, int event, long param, long opt, void *block);
static int event_list(void *vobj, int event, long param, long opt, void *);
static void version_ffmpeg(void);

static int runtime_profile_test(EZOPT *opt, char *cmd);
static void linefeed_count(int n, int mod, char *con, char *coff);
static void print_profile_shots(EZOPT *opt, int min);
static void print_profile_width(EZOPT *opt, int vidw);




int main(int argc, char **argv)
{
	int	i, todo;

	smm_init(0);				/* initialize the libsmm */
	ezopt_init(&sysopt, sysprof[0]);	/* the default setting */
	env_init(&sysopt);

#ifdef	CFG_GUI_ON
	if (command_line_parser(argc, argv, NULL) == 'G') {
		/* initialize the GUI module and read the configure file */
		sysopt.gui =  ezgui_init(&sysopt, &argc, &argv);
	}
#endif

	todo = command_line_parser(argc, argv, &sysopt);
	//return printf("Todo: %c(%d) ARG=%d/%d\n", todo, todo, optind, argc);
	
	/* review the command option structure to make sure there is no
	 * controdicted options */
	ezopt_review(&sysopt);

	smm_signal_break(signal_handler);

	avcodec_register_all();
	av_register_all();

	switch (todo) {
	case TODO_ERROR:
		printf("Invalid parameters.\n");
		todo = EZ_ERR_PARAM;
		break;
	case TODO_UNSET:
		printf("No action applied\n");
		todo = EZ_ERR_EOP;
		break;
	case TODO_HELP:		/* help */
		cli_print(clist);
		todo = EZ_ERR_EOP;
		break;
	case TODO_VERSION:	/* version */
		printf(version, EZTHUMB_VERSION);
		version_ffmpeg();
#ifdef	CFG_GUI_ON
		ezgui_version();
#endif
		todo = EZ_ERR_EOP;
		break;
	case TODO_QK_VER:	/* simple version */
		printf("%s\n", EZTHUMB_VERSION);
		todo = EZ_ERR_EOP;
		break;
	case TODO_PROF_TST:	/* test the profile */
		runtime_profile_test(&sysopt, (void*)sysopt.pro_grid);
		todo = EZ_ERR_EOP;
		break;
	case 'P':	/* print the internal profile table */
		for (i = 0; i < PROFLIST; i++) {
			printf("%2d: %s\n", i, sysprof[i]);
		}
		todo = EZ_ERR_EOP;
		break;

	case 'I':
	case 'i':
		sysopt.notify = event_list;
		if ((sysopt.flags & EZOP_RECURSIVE) == 0) {
			for (i = optind; i < argc; i++) {
				todo = ezinfo(argv[i], &sysopt, NULL);
			}
		} else if (optind >= argc) {
			todo = smm_pathtrek(".", sysopt.r_flags, 
					msg_info, &sysopt);
		} else {
			for (i = optind; i < argc; i++) {
				todo = smm_pathtrek(argv[i], sysopt.r_flags,
						msg_info, &sysopt);
			}
		}
		break;
	case 'b':
		/* inject the progress report functions */
		if (EZOP_DEBUG(sysopt.flags) <= EZOP_DEBUG_BRIEF) {
			sysopt.notify = event_cb;
		}
		todo = ezthumb_bind(argv + optind, argc - optind, &sysopt);
		break;
	case 'G':
		todo = EZ_ERR_EOP;
		if (sysopt.gui == NULL) {
			cli_print(clist);
		}
#ifdef	CFG_GUI_ON
		else {
			todo = ezgui_run(sysopt.gui, argv+optind, argc-optind);
		}
#endif
		break;
	default:
		/* inject the progress report functions */
		if (EZOP_DEBUG(sysopt.flags) <= EZOP_DEBUG_BRIEF) {
			sysopt.notify = event_cb;
		}
		if ((sysopt.flags & EZOP_RECURSIVE) == 0) {
			for (i = optind; i < argc; i++) {
				todo = ezthumb(argv[i], &sysopt);
			}
		} else if (optind >= argc) {
			todo = smm_pathtrek(".", sysopt.r_flags, 
					msg_shot, &sysopt);
		} else {
			for (i = optind; i < argc; i++) {
				todo = smm_pathtrek(argv[i], sysopt.r_flags, 
						msg_shot, &sysopt);
			}
		}
		break;
	}
	main_close(&sysopt);
	return todo;
}


/* return:
 *  >= 0 : todo
 *  == -1: unset
 *  == -2: command line error
 */
static int command_line_parser(int argc, char **argv, EZOPT *opt)
{
	struct	option	*argtbl;
	EZOPT	*dummy = NULL;
	char	*p, *arglist;
	int	c, todo, prof_grid, prof_size;

	if (opt == NULL) {
		opt = dummy = malloc(sizeof(EZOPT));
		if (opt == NULL) {
			return TODO_UNSET;
		}
	}

	arglist = cli_alloc_list(clist);
	argtbl  = cli_alloc_table(clist);
	//puts(arglist);

	todo = TODO_UNSET;		/* UNSET yet */
	prof_grid = prof_size = 1;	/* enable the profile */
	optind = 1;			/* reset the getopt() function */
	while ((c = getopt_long(argc, argv, arglist, argtbl, NULL)) > 0) {
		switch (c) {
		case 1:
		case 2:
		case 3:
			todo = c;
			goto break_parse;	/* break the analysis */

		case 6:	/* nonkey */
			opt->flags |= EZOP_P_FRAME;
			break;
		case 7:
			opt->background = optarg;
			break;
		case 8:	/* gap-shots: Examples: 5, 5%, 5x8, 5%x8% */
			if (!isdigit(*optarg)) {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			opt->grid_gap_w = para_get_ratio(optarg);
			if ((p = strchr(optarg, 'x')) == NULL) {
				opt->grid_gap_h = opt->grid_gap_w;
			} else {
				opt->grid_gap_h = para_get_ratio(++p);
			}
			break;
		case 9:	/* gap-margin: Examples: 5, 5%, 5x8, 5%x8% */
			if (!isdigit(*optarg)) {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			opt->grid_rim_w = para_get_ratio(optarg);
			if ((p = strchr(optarg, 'x')) == NULL) {
				opt->grid_rim_h = opt->grid_rim_w;
			} else {
				opt->grid_rim_h = para_get_ratio(++p);
			}
			break;
		case 10:	/* opt-info */
			if (!strcmp(optarg, "on")) {
				opt->flags |= EZOP_INFO;
			} else if (!strcmp(optarg, "off")) {
				opt->flags &= ~EZOP_INFO;
			} else {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			break;
		case 11:	/* opt-time */
			if (!strcmp(optarg, "on")) {
				opt->flags |= EZOP_TIMEST;
			} else if (!strcmp(optarg, "off")) {
				opt->flags &= ~EZOP_TIMEST;
			} else {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			break;
		case 12:	/* opt-ffr */
			if (!strcmp(optarg, "on")) {
				opt->flags |= EZOP_FFRAME;
			} else if (!strcmp(optarg, "off")) {
				opt->flags &= ~EZOP_FFRAME;
			} else {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			break;
		case 13:	/* opt-lfr */
			if (!strcmp(optarg, "on")) {
				opt->flags |= EZOP_LFRAME;
			} else if (!strcmp(optarg, "off")) {
				opt->flags &= ~EZOP_LFRAME;
			} else {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			break;
		case 15:	/* "pos-bg" */
			if ((c = para_get_position(optarg)) == -1) {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			} else {
				opt->bg_position = c;
			}
			break;
		case 16:	/* "pos-time" */
			if ((c = para_get_position(optarg)) == -1) {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			} else {
				opt->ins_position = c;
			}
			break;
		case 17:	/* "pos-info" */
			if ((c = para_get_position(optarg)) == -1) {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			} else {
				opt->mi_position = c;
			}
			break;
		case 14:	/* decode-on-the-fly */
			opt->flags |= EZOP_DECODE_OTF;
			break;
		case 18:	/* time-from */
			if (isdigit(*optarg)) {
				opt->time_from = para_get_time_point(optarg);
			} else {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			break;
		case 19:	/* time-end */
			if (isdigit(*optarg)) {
				opt->time_to = para_get_time_point(optarg);
			} else {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			break;
		case 20:
			opt->flags |= EZOP_TRANSPARENT;
			opt->canvas_color[3] = 0;
			break;
		case 22:	/* index */
			if (!isdigit(*optarg)) {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			} else {
				opt->vs_user = strtol(optarg, NULL, 0);
			}
			break;
		case 23:
			todo = c;
			/* borrow this pointer because it won't be used ever*/
			opt->pro_grid = (void*) optarg;
			goto break_parse;	/* break the analysis */

		case 24:
			if (!strcmp(optarg, "on")) {
				opt->flags |= EZOP_THUMB_OVERRIDE;
				opt->flags &= ~EZOP_THUMB_COPY;
			} else if (!strcmp(optarg, "off")) {
				opt->flags &= ~EZOP_THUMB_OVERRIDE;
				opt->flags &= ~EZOP_THUMB_COPY;
			} else if (!strcmp(optarg, "copy")) {
				opt->flags &= ~EZOP_THUMB_OVERRIDE;
				opt->flags |= EZOP_THUMB_COPY;
			} else {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			break;
		case 25:
			/* Recursive depth, for example: "FF:3"
			 * Magic words in first two characters: FF, DF, DL.
			 * First-meet-Firt-process, Directory-First, Directory-Last 
			 * 0 means unlimited */
			if ((optarg[2] == ':') || (optarg[2] == 0)) {
				c = opt->r_flags;
				if (!strncmp(optarg, "FF", 2)) {
					c = SMM_PATH_DIR_FIFO;
					optarg += 3;
				} else if (!strncmp(optarg, "DF", 2)) {
					c = SMM_PATH_DIR_FIRST;
					optarg += 3;
				} else if (!strncmp(optarg, "DL", 2)) {
					c = SMM_PATH_DIR_LAST;
					optarg += 3;
				}
				opt->r_flags = SMM_PATH_DIR(opt->r_flags, c);
			}
			if (isdigit(*optarg)) {
				opt->r_flags = SMM_PATH_DEPTH(opt->r_flags,
						strtol(optarg, NULL, 0));
			}
			break;
		case 26:
			/* file name filter, for example: "avi,wmv,mkv"
 			 * The NULL filter means allow all. */
			if (opt->accept) {
				free(opt->accept);
			}
			opt->accept = ezflt_create(optarg);
			break;

		case 'b':
			todo = c;
			break;
		case 'c':	/* RRGGBB:RRGGBB:RRGGBB */
			if (para_get_color(opt, optarg) != EZ_ERR_NONE) {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			break;
		case 'd':	/* Examples: 0,1,quick,skim,scan */
			if (isdigit((int) optarg[0])) {
				opt->dur_mode = strtol(optarg, NULL, 0);
			} else if (!strcmp(optarg, "fast")) {
				opt->dur_mode = EZ_DUR_QK_SCAN;
			} else if (!strcmp(optarg, "scan")) {
				opt->dur_mode = EZ_DUR_FULLSCAN;
			} else if (!strcmp(optarg, "head")) {
				opt->dur_mode = EZ_DUR_CLIPHEAD;
			} else {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			break;
		case 'e':
			if (!isdigit(*optarg)) {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			} else {
				opt->edge_width = strtol(optarg, NULL, 0);
			}
			break;
		case 'f':
			opt->mi_font = opt->ins_font = optarg;
			/* enable fontconfig patterns like "times:bold:italic"
			 * instead of the full path of the font like
			 * "/usr/local/share/ttf/Times.ttf" */
			if (strchr(optarg, ':')) {
				gdFTUseFontConfig(1);
			}
			break;
		case 'F':	/* MI:TM */
			if (para_get_fontsize(opt, optarg) != EZ_ERR_NONE) {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			break;
		case 'g':	/* Examples: 4, 4x8, 0x8 */
			if (!isdigit(*optarg)) {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			opt->grid_col = strtol(optarg, &p, 10);
			if (*p == 0) {
				opt->grid_row = 0;
			} else {
				opt->grid_row = strtol(++p, NULL, 10);
			}
			prof_grid = 0;	/* disable the profile */
			break;
		case 'G':
			todo = c;
			break;
		case 'I':
			todo = c;
			/* make these options default */
			opt->flags |= EZOP_CLI_INFO;
			break;
		case 'i':
			todo = c;
			opt->flags |= EZOP_CLI_LIST;
			break;
		case 'm':	/* Examples: png, jpg@90, gif, gif@1000 */
			opt->img_quality = meta_image_format(optarg, 
					opt->img_format, 8);
			break;
		case 'o':
			opt->pathout = optarg;
			break;
		case 'p':
			if (!strcmp(optarg, "auto")) {
				opt->flags |= EZOP_PROC_AUTO;
				break;
			}
			if (!strcmp(optarg, "skim")) {
				opt->flags |= EZOP_PROC_SKIM;
				break;
			}
			if (!strcmp(optarg, "scan")) {
				opt->flags |= EZOP_PROC_SCAN;
				break;
			}
			if (!strcmp(optarg, "2pass")) {
				opt->flags |= EZOP_PROC_TWOPASS;
				break;
			}
			if (!strcmp(optarg, "heuri")) {
				opt->flags |= EZOP_PROC_HEURIS;
				break;
			}
			if (!strcmp(optarg, "safe")) {
				opt->flags |= EZOP_PROC_SAFE;
				break;
			}
			if (!strncmp(optarg, "key", 3)) {
				opt->flags |= EZOP_PROC_KEYRIP;
				opt->grid_col = 0;
				opt->grid_row = 0;
				prof_grid = 0;	/* disable the profile */
				c = (*optarg == '6') ? 1 : 3;
				if (optarg[c] == '@') {
					opt->key_ripno = (int)
						strtol(optarg+c+1, NULL, 0);
				}
				break;
			}
			todo = TODO_ERROR;	/* command line error */
			goto break_parse;	/* break the analysis */

		case 'P':
			c = strtol(optarg, &p, 10);
			if (*p != 0) {	/* command line profiles */
				ezopt_profile_setup(opt, optarg);
			} else if ((c >= 0) && (c < PROFLIST)) {
				ezopt_profile_setup(opt, sysprof[c]);
			} else {	/* wrong profile index */
				todo = c;
				goto break_parse;	/* break the analysis */
			}
			break;
		case 'R':
			opt->flags |= EZOP_RECURSIVE;
			break;
		case 's':	/* Examples: 50, 50%, 320x240 */
			if (!isdigit(*optarg)) {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			opt->tn_width = opt->tn_height = 0;	/* 20120720 */
			opt->canvas_width = 0;
			c = strtol(optarg, &p, 0);
			if (*p == 0) {
				opt->tn_facto = c;
			} else if (*p == '%') {
				opt->tn_facto = c;
			} else {
				opt->tn_width  = c;
				opt->tn_height = strtol(++p, NULL, 0);
			}
			prof_size = 0;	/* disable the profile */
			break;
		case 't':
			if (!isdigit(*optarg)) {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			opt->grid_row = 0;
			prof_grid = 0;	/* disable the profile */
			opt->tm_step = strtol(optarg, NULL, 0);
			//if (opt->tm_step < 1000) {/* at least 1 sec */
			//	opt->tm_step = 0;
			//}
			break;
		case 'v':
			if (!strcmp(optarg, "none")) {
				c = 0;
			} else if (!strcmp(optarg, "warning")) {
				c = 1;
			} else if (!strcmp(optarg, "info")) {
				c = 2;
			} else if (!strcmp(optarg, "brief")) {
				c = 3;
			} else if (!strcmp(optarg, "iframe")) {
				c = 4;
			} else if (!strcmp(optarg, "packet")) {
				c = 5;
			} else if (!strcmp(optarg, "verbose")) {
				c = 6;
			} else if (!strcmp(optarg, "ffmpeg")) {
				c = 7;
			} else if (isdigit(*optarg)) {
				c = strtol(optarg, NULL, 0);
			} else {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			opt->flags |= EZOP_DEBUG_MAKE(c);
			break;
		case 'w':
			if (!isdigit(*optarg)) {
				todo = TODO_ERROR;	/* command line error */
				goto break_parse;	/* break the analysis */
			}
			opt->canvas_width = strtol(optarg, NULL, 0);
			prof_size = 0;	/* disable the profile */
			break;
		case 'x':
			strncpy_safe(opt->suffix, optarg, 64);
			break;
		default:
			todo = TODO_ERROR;	/* command line error */
			goto break_parse;	/* break the analysis */
		}
	}

break_parse:
	free(argtbl);
	free(arglist);

	/* disable the unwanted profiles */
	if (prof_grid == 0) {
		ezopt_profile_disable(opt, EZ_PROF_LENGTH);
	}
	if (prof_size == 0) {
		ezopt_profile_disable(opt, EZ_PROF_WIDTH);
	}
	
	/* if file names were found in the command line, or recursive mode
	 * was specified, which would regards the empty filename as current
	 * path, ezthumb would work as a command line tool. Otherwise it
	 * will start the GUI interface */
	if (todo == TODO_UNSET) {
		todo = 'G';
		if ((optind < argc) || (opt->flags & EZOP_RECURSIVE)) {
			todo = 'S';
		}
	}
	if (dummy) {
		free(dummy);
	}
	return todo;
}


static int signal_handler(int sig)
{
	//printf("Signal %d\n", sig);
	sig = ezthumb_break(&sysopt);
	main_close(&sysopt);
	return sig;
}

static int main_close(EZOPT *opt)
{
#ifdef	CFG_GUI_ON
	if (opt->gui) {
		ezgui_close(opt->gui);
		opt->gui = NULL;
	}
#endif
	if (opt->accept) {
		free(opt->accept);
		opt->accept = NULL;
	}
	if (opt->refuse) {
		free(opt->refuse);
		opt->refuse = NULL;
	}
	return 0;
}

static int msg_info(void *option, char *path, int type, void *info)
{
	EZOPT	*ezopt = option;

	switch (type) {
	case SMM_MSG_PATH_ENTER:
		printf("Entering %s:\n", path);
		break;
	case SMM_MSG_PATH_EXEC:
		if (ezflt_match(ezopt->accept, path)) {
			//printf(">>> %s\n", path);
			ezinfo(path, option, NULL);
		}
		break;
	case SMM_MSG_PATH_BREAK:
		printf("Failed to process %s\n", path);
		break;
	case SMM_MSG_PATH_LEAVE:
		printf("Leaving %s\n", path);
		break;
	}
	return 0;
}

static int msg_shot(void *option, char *path, int type, void *info)
{
	EZOPT	*ezopt = option;

	switch (type) {
	case SMM_MSG_PATH_ENTER:
		printf("Entering %s:\n", path);
		break;
	case SMM_MSG_PATH_EXEC:
		if (ezflt_match(ezopt->accept, path)) {
			//printf("+++ %s\n", path);
			ezthumb(path, option);
		}
		break;
	case SMM_MSG_PATH_BREAK:
		printf("Failed to process %s\n", path);
		break;
	case SMM_MSG_PATH_LEAVE:
		printf("Leaving %s\n", path);
		break;
	}
	return 0;
}

static int env_init(EZOPT *ezopt)
{
	EZFLT	*arg;
	//char	*env = "-g 3x5 -s 300x100 -w 1024 -m gif -p 2pass";
	char	*env;
	int	num, len;

	if ((env = getenv("EZTHUMB")) == NULL) {
		return 0;
	}

	len = strlen(env) + 1;
	num = len / 3;
	if ((arg = calloc(num * sizeof(char*) + len + 16, 1)) == NULL) {
		return -1;
	}

	strcpy((char*) &arg->filter[num], "ezthumb ");
	strcat((char*) &arg->filter[num], env);
	env = (char*) &arg->filter[num];

	len = mkargv(env, arg->filter, num);

	/*for (num = 0; num <= len; num++) {
		printf("%d: %s\n", num, arg->filter[num]);
	}*/

	command_line_parser(len, arg->filter, ezopt);
	free(arg);
	return 0;
}

static int para_get_ratio(char *s)
{
	int	val;

	val = strtol(s, &s, 10);
	if (s && (*s == '%')) {
		val |= EZ_RATIO_OFF;
	}
	return val;
}


/* the time point should be either a readable timestamp or a percentage
 * for example: 1:23:32 or 33%. In the timestamp format, it accepts 1 to 4
 * sections, each means second only, minute:second, hour:minute:second
 * and hour:minute:second:millisecond */
static int para_get_time_point(char *s)
{
	char	*argvs[8];
	int	argcs, val = 0;

	if (strchr(s, '%')) {
		val = strtol(s, NULL, 0);
	       	if (val > 0) {
			val |= EZ_RATIO_OFF;
		}
		return val;
	}

	argcs = ziptoken(s, argvs, sizeof(argvs)/sizeof(char*), ":");
	switch (argcs) {
	case 0:	/* 20110301: in case of wrong input */
		puts("Incorrect time format. Try HH:MM:SS or NN%.");
		break;
	case 1:
		val = strtol(argvs[0], NULL, 0);
		break;
	case 2:
		val = strtol(argvs[0], NULL, 0) * 60 +
			strtol(argvs[1], NULL, 0);
		break;
	case 3:
		val = strtol(argvs[0], NULL, 0) * 3600 +
			strtol(argvs[1], NULL, 0) * 60 +
			strtol(argvs[2], NULL, 0);
		break;
	default:
		val = strtol(argvs[0], NULL, 0) * 3600 +
			strtol(argvs[1], NULL, 0) * 60 +
			strtol(argvs[2], NULL, 0);
		return val * 1000 + strtol(argvs[3], NULL, 0);
	}
	return val * 1000;
}


/* The lists of positions are: lt, lc, lb, mt, mc, mb, rt, rc, rb, tt
 * The qualifications are: st, ex, ey, sx, sy
 * The position and qualification are seperated by ':'.
 * For example: "lt", "lt:st" */
static int para_get_position(char *s)
{
	char	*argvs[4];
	int	argcs;

	argcs = ziptoken(s, argvs, sizeof(argvs)/sizeof(char*), ":");
	if (argcs < 2) {
		return para_make_postition(s);
	} 
	return para_make_postition(argvs[0]) | para_make_postition(argvs[1]);
}

static int para_make_postition(char *s)
{
	int	rc = 0;

	if ((s[0] == 'l') && (s[1] == 't')) {
		rc = EZ_POS_LEFTTOP;
	} else if ((s[0] == 'l') && (s[1] == 'c')) {
		rc = EZ_POS_LEFTCENTER;
	} else if ((s[0] == 'l') && (s[1] == 'b')) {
		rc = EZ_POS_LEFTBOTTOM;
	} else if ((s[0] == 'm') && (s[1] == 't')) {
		rc = EZ_POS_MIDTOP;
	} else if ((s[0] == 'm') && (s[1] == 'c')) {
		rc = EZ_POS_MIDCENTER;
	} else if ((s[0] == 'm') && (s[1] == 'b')) {
		rc = EZ_POS_MIDBOTTOM;
	} else if ((s[0] == 'r') && (s[1] == 't')) {
		rc = EZ_POS_RIGHTTOP;
	} else if ((s[0] == 'r') && (s[1] == 'c')) {
		rc = EZ_POS_RIGHTCENTER;
	} else if ((s[0] == 'r') && (s[1] == 'b')) {
		rc = EZ_POS_RIGHTBOTTOM;
	} else if ((s[0] == 't') && (s[1] == 't')) {
		rc = EZ_POS_TILE;
	} else if ((s[0] == 's') && (s[1] == 't')) {
		rc = EZ_POS_STRETCH;
	} else if ((s[0] == 'e') && (s[1] == 'x')) {
		rc = EZ_POS_ENLARGE_X;
	} else if ((s[0] == 'e') && (s[1] == 'y')) {
		rc = EZ_POS_ENLARGE_Y;
	} else if ((s[0] == 's') && (s[1] == 'x')) {
		rc = EZ_POS_STRETCH_X;
	} else if ((s[0] == 's') && (s[1] == 'y')) {
		rc = EZ_POS_STRETCH_Y;
	} else {
		rc = -1;
	}
	return rc;
}

static int para_get_color(EZOPT *opt, char *s)
{
	unsigned long	rc;
	char	*clist[3];

	fixtoken(s, clist, sizeof(clist)/sizeof(char*), ":");

	if (clist[0] && *clist[0]) {
		if (!isxdigit(*clist[0])) {
			return EZ_ERR_PARAM;
		}
		rc = strtoul(clist[0], NULL, 16);
		opt->mi_color[0] = (unsigned char)((rc >> 16) & 0xff);
		opt->mi_color[1] = (unsigned char)((rc >> 8) & 0xff);
		opt->mi_color[2] = (unsigned char)(rc & 0xff);
	}
	if (clist[1] && *clist[1]) {
		if (!isxdigit(*clist[1])) {
			return EZ_ERR_PARAM;
		}
		rc = strtoul(clist[1], NULL, 16);
		opt->ins_color[0] = (unsigned char)((rc >> 16) & 0xff);
		opt->ins_color[1] = (unsigned char)((rc >> 8) & 0xff);
		opt->ins_color[2] = (unsigned char)(rc & 0xff);
	}
	if (clist[2] && *clist[2]) {
		if (!isxdigit(*clist[2])) {
			return EZ_ERR_PARAM;
		}
		rc = strtoul(clist[2], NULL, 16);
		opt->canvas_color[0] = (unsigned char)((rc >> 16) & 0xff);
		opt->canvas_color[1] = (unsigned char)((rc >> 8) & 0xff);
		opt->canvas_color[2] = (unsigned char)(rc & 0xff);
	}
	return EZ_ERR_NONE;
}

static int para_get_fontsize(EZOPT *opt, char *s)
{
	char	*clist[3];

	fixtoken(s, clist, sizeof(clist)/sizeof(char*), ":");

	if (clist[0] && *clist[0]) {
		if (isdigit(*clist[0])) {
			opt->mi_size = (int) strtol(clist[0], NULL, 0);
		} else {
			return EZ_ERR_PARAM;
		}
	}
	if (clist[1] && *clist[1]) {
		if (isdigit(*clist[0])) {
			opt->ins_size = (int) strtol(clist[1], NULL, 0);
		} else {
			return EZ_ERR_PARAM;
		}
	}
	return EZ_ERR_NONE;
}

static int event_cb(void *vobj, int event, long param, long opt, void *block)
{
	int	expect;
	static	int	dotted;

	switch (event) {
	case EN_PROC_BEGIN:
		switch (param) {
		case ENX_SS_SCAN:
			printf("Building (Scan)      ");
			break;
		case ENX_SS_TWOPASS:
			printf("Building (2Pass)     ");
			break;
		case ENX_SS_HEURIS:
			printf("Building (Heur)      ");
			break;
		case ENX_SS_IFRAMES:
			printf("Building (iFrame)      ");
			break;
		case ENX_SS_SKIM:
		default:
			printf("Building (Fast)      ");
			break;
		}
		dotted = 0;
		break;

	case EN_PROC_CURRENT:
		if (param == 0) {	/* for key frame saving only */
			printf(".");
			break;
		}

		expect = opt * 100 / param / 2;
		if (dotted >= expect) {
			printf("\b\b\b\b%3ld%%", opt * 100 / param);
		} else while (dotted < expect) {
			printf("\b\b\b\b\b. %3ld%%", opt * 100 / param);
			dotted++;
		}
		break;

	case EN_PROC_END:
		if (param == 0) {       /* for key frame saving only */
			//printf(" done\n");
			printf("\b\b\b\b100%% done\n");
		} else {
			printf("\b\b\b\bdone\n");
		}
		printf("%s: %ldx%ld canvas.\n", 
				block ? (char*) block : "", param, opt);
		break;

	case EN_STREAM_BROKEN:
		break;

	default:
		return EN_EVENT_PASSTHROUGH;
	}
	fflush(stdout);
	return event;
}


static int event_list(void *vobj, int event, long param, long opt, void *block)
{
	if (event >= 0) {
		return EN_EVENT_PASSTHROUGH;
	}
	return event;
}

static void version_ffmpeg(void)
{
	printf("FFMPEG: libavcodec %d.%d.%d; ", LIBAVCODEC_VERSION_MAJOR, 
			LIBAVCODEC_VERSION_MINOR, LIBAVCODEC_VERSION_MICRO);
	printf("libavformat %d.%d.%d; ", LIBAVFORMAT_VERSION_MAJOR, 
			LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO);
	printf("libavutil %d.%d.%d; ", LIBAVUTIL_VERSION_MAJOR, 
			LIBAVUTIL_VERSION_MINOR, LIBAVUTIL_VERSION_MICRO);
	printf("libswscale %d.%d.%d\n", LIBSWSCALE_VERSION_MAJOR, 
			LIBSWSCALE_VERSION_MINOR, LIBSWSCALE_VERSION_MICRO);
}


/* -o @180    specify the video length
 * -o +1024   specify the width of the video */
static int runtime_profile_test(EZOPT *opt, char *cmd)
{
	int	stdres[] = { 160, 240, 320, 512, 640, 704, 720, 800, 1024,
		1152, 1280, 1366, 1440, 1680, 1920, 2048, 2560, 2880 };
	int	i, val;

	ezopt_profile_dump(opt, "Grid: ", "Size: ");

	switch (*cmd) {
	case '@':
		val = (int)strtol(cmd + 1, 0, 10);
		print_profile_shots(opt, val);
		printf("\n");
		return 0;

	case '+':
		val = (int)strtol(cmd + 1, 0, 10);
		print_profile_width(opt, val);
		printf("\n");
		return 0;
	}

	printf("Reference of Video Length:\n");
	for (i = 0; i < 18; i++) {
		if (i < 10) {
			val = (i+1)*10;
		} else {
			val = (i+1) * 12;
		}
		print_profile_shots(opt, val);
		linefeed_count(i, 3, "\n", "    ");
	}
	linefeed_count(i, 3, "", "\n\n");

	printf("Reference of Width:\n");
	for (i = 0; i < sizeof(stdres)/sizeof(int); i++) {
		print_profile_width(opt, stdres[i]);
		linefeed_count(i, 3, "\n", "    ");
	}
	linefeed_count(i, 3, "", "\n");
	return 0;
}

static void linefeed_count(int n, int mod, char *con, char *coff)
{
	if ((n % mod) == (mod - 1)) {
		printf("%s", con);
	} else {
		printf("%s", coff);
	}
}

static void print_profile_shots(EZOPT *opt, int min)
{
	int	wid, hei, fac;

	wid = hei = 0;
	fac = ezopt_profile_sampling(opt, min * 60, &wid, &hei);
	if (fac > 0) {
		ezopt_profile_sampled(opt, 640, fac, &wid, &hei);
	}
	printf("[%4d]=[%4d %4d %3d %4d]", min, wid, hei, fac, 
			wid * hei > 0 ? min * 60 / (wid * hei) : 0);
}

static void print_profile_width(EZOPT *opt, int vidw)
{
	int	wid, hei, fac, can;

	wid = hei = fac = 0;
	can = ezopt_profile_zooming(opt, vidw, &wid, &hei, &fac);
	printf("[%4d]=[%4d %4d %3d %4d]", vidw, wid, hei, fac, can);
}


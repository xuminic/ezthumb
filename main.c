
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

#include "ezthumb.h"
#include "ezgui.h"
#include "cliopt.h"
#include "libsmm.h"


static	struct	cliopt	clist[] = {
	{ 0, NULL, 0, "Usage: ezthumb [OPTIONS] video_clip ..." },
	{ 0, NULL, 0, "OPTIONS:" },
	{ 'c', "colour",  2, "the colour setting (MI:TM:BG)(RRGGBB)" },
	{ 'd', "during",  2, "the duration finding mode (head)(fast|scan)" },
	{ 'f', "font",    2, "the TrueType font name with the full path" },
	{ 'F', "fontsize",2, "the size setting of the font" },
	{ 'g', "grid",    2, "the thumbnail grid in the canvas.(4x4)" },
	{ 'G', "gui",     0, "enable the graphic user interface" },
	{ 'i', "list",    0, "display the media information in list form" },
	{ 'I', "info",    0, "display the media information" },
	{ 'm', "format",  2, "the output format (jpg@85)" },
	{ 'o', "outdir",  2, "the directory for storing output images" },
	{ 'p', "process", 1, "specify the process method (0|1|2|3|4)" },
	{ 'P', "profile", 2, "specify the profile string" },
	{ 's', "ssize",   2, "the size of each screen shots (WxH|RR%)" },
	{ 't', "timestep",1, "the time step between each shots in ms" }, 
	{ 'v', "verbose", 1, "*verbose mode (0)(0-7)" },
	{ 'w', "width",   1, "the whole width of the thumbnail canvas" },
	{ 'x', "suffix",  2, "the suffix of output filename (_thumb)" },
	{   6, "accurate", 0, "take accurate shots including P-frames" },
	{   7, "background", 2, "the background picture" },
	{  14, "decode-otf", 0, "decoding on the fly mode for scan process" },
	{  21, "edge",    1, "the width of the screen shot edge (0)" },
	{   8, "gap-shots",  1, "the gaps between the screen shots (4)" },
	{   9, "gap-margin", 1, "the margin in the canvas (8)" },
	{  10, "opt-info", 2, "the media infomation (on)" },
	{  11, "opt-time", 2, "the timestamp inside the screen shots (on)" },
	{  12, "opt-ffr",  2, "start from the first frame (off)" },
	{  13, "opt-lfr",  2, "end at the last frame (off)" },
	{  15, "pos-bg",   2, "the position of the background image (mc)" },
	{  16, "pos-time", 2, "the position of the timestamp (rt)" },
	{  17, "pos-info", 2, "the position of the media infomation (lt)" },
	{   0,  NULL, -1, "lt,lc,lb,mt,mc,mb,rt,rc,rb,tt and st,ex,ey,sx,sy" },
	{  18, "time-from",2, "the time in video where begins shooting" },
	{  19, "time-end", 2, "the time in video where ends shooting" },
	{  20, "transparent", 0, "generate the transparent background" },
	{  22, "vindex",   1, "the index of the video stream" },
	{   1, "help",    0, "*Display the help message" },
	{   2, "version", 0, "*Display the version number" },
	{ 0, NULL, 0, NULL }
};


static	char	*version = "\
ezthumb %s, to generate the thumbnails from video files.\n\
Copyright (C) 2011 \"Andy Xuming\" <xuming@users.sourceforge.net>\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n";


/* predefined profiles */
static	char	*sysprof[] = {
	"12M4x4:20M4x6:30M4x8:40M4x10:60M4x12:90M4x16:120M4x20:180M4x24:"
		"160w200%:400w100%:640w50%:720w40%:1280w25%:1600w20%:",
	"12M3x4:20M3x6:30M3x8:40M3x10:60M3x12:90M3x16:120M3x20:180M3x24:"
		"160w200%:400w100%:640w50%:720w40%:1280w25%:1600w20%",
	"12M6x4:20M6x6:30M6x8:40M6x10:60M6x12:90M6x16:120M6x20:180M6x24:"
		"160w125%:400w50%:640w35%:720w30%:1280w15%:1600w10%"
};

#define PROFLIST	(sizeof(sysprof)/sizeof(int*))

static	EZOPT	sysoption;

int signal_break(int (*handle)(int));
int signal_handler(int sig);

static int para_get_ratio(char *s);
static int para_get_time_point(char *s);
static int para_get_position(char *s);
static int para_make_postition(char *s);
static int para_get_color(EZOPT *opt, char *s);
static int para_get_fontsize(EZOPT *opt, char *s);
static int event_cb(void *vobj, int event, long param, long opt, void *block);
static int event_list(void *vobj, int event, long param, long opt, void *);

extern int fixtoken(char *sour, char **idx, int ids, char *delim);
extern int ziptoken(char *sour, char **idx, int ids, char *delim);


int main(int argc, char **argv)
{
	struct	option	*argtbl;
	char	*p, *arglist;
	int	c, todo = -1;
	int	prof_grid, prof_size;

	prof_grid = prof_size = 1;	/* enable the profile */

	smm_init();				/* initialize the libsmm */
	ezopt_init(&sysoption, sysprof[0]);	/* the default setting */
	ezgui_init(&sysoption, &argc, &argv);	/* the config file */

	arglist = cli_alloc_list(clist);
	argtbl  = cli_alloc_table(clist);
	//puts(arglist);
	while ((c = getopt_long(argc, argv, arglist, argtbl, NULL)) > 0) {
		switch (c) {
		case 1:
			cli_print(clist);
			return 0;
		case 2:
			printf(version, EZTHUMB_VERSION);
			printf("FFMPEG: libavcodec %d.%d.%d; ", 
					LIBAVCODEC_VERSION_MAJOR, 
					LIBAVCODEC_VERSION_MINOR,
					LIBAVCODEC_VERSION_MICRO);
			printf("libavformat %d.%d.%d; ", 
					LIBAVFORMAT_VERSION_MAJOR,
					LIBAVFORMAT_VERSION_MINOR,
					LIBAVFORMAT_VERSION_MICRO);
			printf("libavutil %d.%d.%d; ",
					LIBAVUTIL_VERSION_MAJOR,
					LIBAVUTIL_VERSION_MINOR,
					LIBAVUTIL_VERSION_MICRO);
			printf("libswscale %d.%d.%d\n",
					LIBSWSCALE_VERSION_MAJOR,
					LIBSWSCALE_VERSION_MINOR,
					LIBSWSCALE_VERSION_MICRO);
			return 0;
		case 6:	/* nonkey */
			sysoption.flags |= EZOP_P_FRAME;
			break;
		case 7:
			sysoption.background = optarg;
			break;
		case 8:	/* gap-shots: Examples: 5, 5%, 5x8, 5%x8% */
			sysoption.grid_gap_w = para_get_ratio(optarg);
			if ((p = strchr(optarg, 'x')) == NULL) {
				sysoption.grid_gap_h = sysoption.grid_gap_w;
			} else {
				sysoption.grid_gap_h = para_get_ratio(++p);
			}
			break;
		case 9:	/* gap-margin: Examples: 5, 5%, 5x8, 5%x8% */
			sysoption.grid_rim_w = para_get_ratio(optarg);
			if ((p = strchr(optarg, 'x')) == NULL) {
				sysoption.grid_rim_h = sysoption.grid_rim_w;
			} else {
				sysoption.grid_rim_h = para_get_ratio(++p);
			}
			break;
		case 10:	/* opt-info */
			if (!strcmp(optarg, "on")) {
				sysoption.flags |= EZOP_INFO;
			} else if (!strcmp(optarg, "off")) {
				sysoption.flags &= ~EZOP_INFO;
			}
			break;
		case 11:	/* opt-time */
			if (!strcmp(optarg, "on")) {
				sysoption.flags |= EZOP_TIMEST;
			} else if (!strcmp(optarg, "off")) {
				sysoption.flags &= ~EZOP_TIMEST;
			}
			break;
		case 12:	/* opt-ffr */
			if (!strcmp(optarg, "on")) {
				sysoption.flags |= EZOP_FFRAME;
			} else if (!strcmp(optarg, "off")) {
				sysoption.flags &= ~EZOP_FFRAME;
			}
			break;
		case 13:	/* opt-lfr */
			if (!strcmp(optarg, "on")) {
				sysoption.flags |= EZOP_LFRAME;
			} else if (!strcmp(optarg, "off")) {
				sysoption.flags &= ~EZOP_LFRAME;
			}
			break;
		case 15:	/* "pos-bg" */
			if ((c = para_get_position(optarg)) == -1) {
				printf("Error position code!\n");
			} else {
				sysoption.bg_position = c;
			}
			break;
		case 16:	/* "pos-time" */
			if ((c = para_get_position(optarg)) == -1) {
				printf("Error position code!\n");
			} else {
				sysoption.ins_position = c;
			}
			break;
		case 17:	/* "pos-info" */
			if ((c = para_get_position(optarg)) == -1) {
				printf("Error position code!\n");
			} else {
				sysoption.mi_position = c;
			}
			break;
		case 14:	/* decode-on-the-fly */
			sysoption.flags |= EZOP_DECODE_OTF;
			break;
		case 18:	/* time-from */
			sysoption.time_from = para_get_time_point(optarg);
			break;
		case 19:	/* time-end */
			sysoption.time_to = para_get_time_point(optarg);
			break;
		case 20:
			sysoption.flags |= EZOP_TRANSPARENT;
			sysoption.canvas_color[3] = 0;
			break;
		case 22:	/* index */
			sysoption.vs_idx = strtol(optarg, NULL, 0);
			break;
		case 'c':	/* RRGGBB:RRGGBB:RRGGBB */
			para_get_color(&sysoption, optarg);
			break;
		case 'd':	/* Examples: 0,1,quick,skim,scan */
			if (isdigit((int) optarg[0])) {
				sysoption.dur_mode = strtol(optarg, NULL, 0);
			} else if (!strcmp(optarg, "fast")) {
				sysoption.dur_mode = EZ_DUR_QK_SCAN;
			} else if (!strcmp(optarg, "scan")) {
				sysoption.dur_mode = EZ_DUR_FULLSCAN;
			} else {
				sysoption.dur_mode = EZ_DUR_CLIPHEAD;
			}
			break;
		case 'e':
			sysoption.edge_width = strtol(optarg, NULL, 0);
			break;
		case 'f':
			sysoption.mi_font = sysoption.ins_font = optarg;
			/* enable fontconfig patterns like "times:bold:italic"
			 * instead of the full path of the font like
			 * "/usr/local/share/ttf/Times.ttf" */
			if (strchr(optarg, ':')) {
				gdFTUseFontConfig(1);
			}
			break;
		case 'F':	/* MI:TM */
			para_get_fontsize(&sysoption, optarg);
			break;
		case 'g':	/* Examples: 4, 4x8, 0x8 */
			sysoption.grid_col = strtol(optarg, &p, 10);
			if (*p == 0) {
				sysoption.grid_row = 0;
			} else {
				sysoption.grid_row = strtol(++p, NULL, 10);
			}
			prof_grid = 0;	/* disable the profile */
			break;
		case 'G':
			todo = c;
			break;
		case 'I':
			todo = c;
			/* make these options default */
			sysoption.flags |= EZOP_CLI_INFO;
			break;
		case 'i':
			todo = c;
			sysoption.flags |= EZOP_CLI_LIST;
			break;
		case 'm':	/* Examples: png, jpg@90, gif, gif@1000 */
			sysoption.img_quality = 0;
			if ((p = strchr(optarg, '@')) != NULL) {
				*p++ = 0;
				sysoption.img_quality = strtol(p, NULL, 0);
			} else if (!strcmp(optarg, "jpg")) {
				sysoption.img_quality = 80;
			}
			strncpy(sysoption.img_format, optarg, 7);
			break;
		case 'o':
			sysoption.pathout = optarg;
			break;
		case 'p':
			if ((*optarg == '1') || !strcmp(optarg, "skim")) {
				sysoption.flags |= EZOP_PROC_SKIM;
				break;
			}
			if ((*optarg == '2') || !strcmp(optarg, "scan")) {
				sysoption.flags |= EZOP_PROC_SCAN;
				break;
			}
			if ((*optarg == '3') || !strcmp(optarg, "twopass")) {
				sysoption.flags |= EZOP_PROC_TWOPASS;
				break;
			}
			if ((*optarg == '4') || !strcmp(optarg, "heuris")) {
				sysoption.flags |= EZOP_PROC_HEURIS;
				break;
			}
			if ((*optarg == '5') || !strncmp(optarg, "key", 3)) {
				sysoption.flags |= EZOP_PROC_KEYRIP;
				sysoption.grid_col = 0;
				sysoption.grid_row = 0;
				c = (*optarg == '5') ? 1 : 3;
				if (optarg[c] == '@') {
					sysoption.key_ripno = (int)
						strtol(optarg+c+1, NULL, 0);
				}
				break;
			}
			sysoption.flags |= EZOP_PROC_AUTO;
			break;
		case 'P':
			c = strtol(optarg, &p, 10);
			if (*p != 0) {	/* command line profiles */
				ezopt_profile_setup(&sysoption, optarg);
			} else if ((c >= 0) && (c < PROFLIST)) {
				ezopt_profile_setup(&sysoption, sysprof[c]);
			} else {	/* wrong profile index */
				for (c = 0; c < PROFLIST; c++) {
					printf("%2d: %s\n", c, sysprof[c]);
				}
				return 0;
			}
			break;
		case 's':	/* Examples: 50, 50%, 320x240 */
			c = strtol(optarg, &p, 0);
			if (*p == 0) {
				sysoption.tn_facto = c;
			} else if (*p == '%') {
				sysoption.tn_facto = c;
			} else {
				sysoption.tn_width  = c;
				sysoption.tn_height = strtol(++p, NULL, 0);
			}
			prof_size = 0;	/* disable the profile */
			break;
		case 't':
			sysoption.tm_step = strtol(optarg, NULL, 0);
			//if (sysoption.tm_step < 1000) {/* at least 1 sec */
			//	sysoption.tm_step = 0;
			//}
			break;
		case 'v':
			c = strtol(optarg, NULL, 0);
			sysoption.flags |= EZOP_DEBUG_MAKE(c);
			break;
		case 'w':
			sysoption.canvas_width = strtol(optarg, NULL, 0);
			break;
		case 'x':
			strncpy(sysoption.suffix, optarg, 63);
			sysoption.suffix[63] = 0;
			break;
		default:
			printf("Get 0x%x\n", c);
			break;
		}
	}
	free(argtbl);
	free(arglist);

	/* disable the unwanted profiles */
	if (prof_grid == 0) {
		sysoption.pro_grid = NULL;
	}
	if (prof_size == 0) {
		sysoption.pro_size = NULL;
	}

	/* if no video file was specified, the ezthumb starts in GUI mode */
	if (optind >= argc) {
		todo = 'G';
	}

	smm_signal_break(signal_handler);

	avcodec_register_all();
	av_register_all();

	if (EZOP_DEBUG(sysoption.flags) < EZOP_DEBUG_VERBS) {
		av_log_set_level(0);	/* disable all complains from ffmpeg*/
	} else if (EZOP_DEBUG(sysoption.flags) == EZOP_DEBUG_FFM) {
		av_log_set_level(AV_LOG_VERBOSE);	/* enable all logs */
	}

	switch (todo) {
	case 'I':
	case 'i':
		sysoption.notify = event_list;
		for ( ; optind < argc; optind++) {
			c = ezinfo(argv[optind], &sysoption);
		}
		break;
	case 'G':
		if ((sysoption.gui = ezgui_create(&sysoption)) !=NULL) {
			if (argc > optind) {
				ezgui_list_add_file(sysoption.gui,
						argv + optind, argc - optind);
			}
			ezgui_run(sysoption.gui);
			ezgui_close(sysoption.gui);
		}
		break;
	default:
		/* inject the progress report functions */
		if (EZOP_DEBUG(sysoption.flags) == EZOP_DEBUG_NONE) {
			sysoption.notify = event_cb;
		}
		for ( ; optind < argc; optind++) {
			c = ezthumb(argv[optind], &sysoption);
		}
		break;
	}
	return c;
}


int signal_handler(int sig)
{
	printf("Signal %d\n", sig);
	if (sysoption.gui) {
		ezgui_close(sysoption.gui);
	}
	return ezthumb_break(&sysoption);
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

	argcs = ziptoken(s, argvs, 8, ":");
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

	argcs = ziptoken(s, argvs, 4, ":");
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

	fixtoken(s, clist, 3, ":");

	if (clist[0] && *clist[0]) {
		rc = strtoul(clist[0], NULL, 16);
		opt->mi_color[0] = (unsigned char)((rc >> 16) & 0xff);
		opt->mi_color[1] = (unsigned char)((rc >> 8) & 0xff);
		opt->mi_color[2] = (unsigned char)(rc & 0xff);
	}
	if (clist[1] && *clist[1]) {
		rc = strtoul(clist[1], NULL, 16);
		opt->ins_color[0] = (unsigned char)((rc >> 16) & 0xff);
		opt->ins_color[1] = (unsigned char)((rc >> 8) & 0xff);
		opt->ins_color[2] = (unsigned char)(rc & 0xff);
	}
	if (clist[2] && *clist[2]) {
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

	fixtoken(s, clist, 2, ":");

	if (clist[0] && *clist[0]) {
		opt->mi_size = (int) strtol(clist[0], NULL, 0);
	}
	if (clist[1] && *clist[1]) {
		opt->ins_size = (int) strtol(clist[1], NULL, 0);
	}
	return EZ_ERR_NONE;
}

static int event_cb(void *vobj, int event, long param, long opt, void *block)
{
	EZVID	*vidx = vobj;
	int	expect;
	static	int	dotted;

	switch (event) {
	case EN_PROC_BEGIN:
		switch (opt) {
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
			printf("Building (iFrame) ");
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
			printf(" done\n");
		} else {
			printf("\b\b\b\bdone\n");
		}
		printf("%s: %ldx%ld canvas.\n", 
				block ? (char*) block : "", param, opt);

		break;

	default:
		return ezdefault(vidx, event, param, opt, block);
	}
	fflush(stdout);
	return event;
}


static int event_list(void *vobj, int event, long param, long opt, void *block)
{
	EZVID	*vidx = vobj;

	if (event >= 0) {
		return ezdefault(vidx, event, param, opt, block);
	}
	return event;
}



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
#ifdef	HAVE_CONFIG_H
#include <config.h>
#else
#error "Run configure first"
#endif

#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#ifdef HAVE_STRING_H
# if !defined STDC_HEADERS && defined HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <ctype.h>

#include "ezthumb.h"
#include "id_lookup.h"

#ifndef	CFG_GUI_OFF
  #include "ezgui.h"
#endif

/* re-use the debug convention in libcsoup */
//#define CSOUP_DEBUG_LOCAL     SLOG_CWORD(EZTHUMB_MOD_CLI, SLOG_LVL_WARNING)
#define CSOUP_DEBUG_LOCAL     SLOG_CWORD(EZTHUMB_MOD_CLI, SLOG_LVL_DEBUG)
#include "libcsoup_debug.h"


#define CMD_ERROR	-2
#define CMD_UNSET	-1
#define CMD_UNKNOWN	0
#define CMD_HELP	1
#define CMD_VERSION	2
#define CMD_F_RESET	3
#define CMD_ACCURATE	4
#define CMD_BKGROUND	5
#define CMD_OTF		6
#define CMD_DEPTH	7
#define CMD_EDGE	8
#define CMD_FILTER	9
#define CMD_GAP_SHOT	10
#define CMD_GAP_MARG	11
#define CMD_OPT_INFO	12
#define CMD_OPT_TIME	13
#define CMD_OPT_FFR	14
#define CMD_OPT_LFR	15
#define CMD_OVERRIDE	16
#define CMD_POS_BG	17
#define CMD_PROGRESS	18
#define CMD_SIZEUNIT	19
#define CMD_TIME_FROM	20
#define CMD_TIME_END	21
#define CMD_TRANSPRT	22
#define CMD_VID_IDX	23

#define CMD_B_IND	'b'
#define CMD_C_OLOR	'c'
#define CMD_D_URING	'd'
#define CMD_F_ONT	'f'
#define CMD_F_ONTSZ	'F'
#define CMD_G_RID	'g'
#define CMD_G_UI	'G'
#define CMD_I_NFO	'i'
#define CMD_I_NSIDE	'I'
#define CMD_FOR_M_AT	'm'
#define CMD_O_UTPUT	'o'
#define CMD_P_ROCESS	'p'
#define CMD_P_ROFILE	'P'
#define CMD_R_ECURS	'R'
#define CMD_S_IZE	's'
#define CMD_T_IMES	't'
#define CMD_V_ERBOSE	'v'
#define CMD_W_IDTH	'w'
#define CMD_SUFFI_X	'x'

static	struct	idtbl	id_duration[] = {
	{ EZOP_DUR_QSCAN, "fast" },
	{ EZOP_DUR_FSCAN, "scan" },
	{ EZOP_DUR_HEAD,  "head" },
	{ EZOP_DUR_AUTO,  "auto" },
	{ 0, NULL }
};

static	struct	idtbl	id_process[] = {
	{ EZOP_PROC_AUTO,    "auto" },
	{ EZOP_PROC_SKIM,    "skim" },
	{ EZOP_PROC_SCAN,    "scan" },
	{ EZOP_PROC_TWOPASS, "2pass" },
	{ EZOP_PROC_SAFE,    "safe" },
	{ EZOP_PROC_KEYRIP,  "key" },
	{ 0, NULL }
};

static	struct	cliopt	clist[] = {
	{ 0, NULL, 0, "Usage: ezthumb [OPTIONS] video_clip ..." },
	{ 0, NULL, 0, "OPTIONS:" },
	{ CMD_B_IND, "bind",
		0, "binding multiple video sources" },
	{ CMD_C_OLOR, "colour",
		1, "the colour setting (MI:TM:BG)(RRGGBB)" },
	{ CMD_D_URING, "during",  
		1, "the duration finding mode (auto)(head|fast|scan)" },
	{ CMD_F_ONT, "font",
		1, "the TrueType font name with the full path" },
	{ CMD_F_ONTSZ, "fontsize",
		1, "the size setting of the font" },
	{ CMD_G_RID, "grid", 
		1, "the thumbnail grid in the canvas." },
#ifndef	CFG_GUI_OFF
	{ CMD_G_UI, "gui",
		0, "enable the graphic user interface" },
	{ CMD_PROGRESS, "gui-progress",
		0, "display the progress bar without the full GUI window" },
#endif
	{ CMD_I_NFO, "info",
		0, "display the simple information of videos" },
	{ CMD_I_NSIDE, "inside",
		0, "display the detail information of videos" },
	{ CMD_FOR_M_AT, "format",
		1, "the output format (jpg@85)" },
	{ CMD_O_UTPUT, "outdir",  
		1, "the directory for storing output images" },
	{ CMD_P_ROCESS, "process", 
		1, "the process method (skim|scan|2pass|safe|key[@N])"},
	{ CMD_P_ROFILE, "profile", 
		1, "specify the profile string" },
	{ CMD_R_ECURS, "recursive", 
		0, "process files and directories recursively" },
	{ CMD_S_IZE, "ssize",
		1, "the size of each screen shots (WxH|RR%)" },
	{ CMD_T_IMES, "timestep",
		1, "the time step between each shots in ms" }, 
	{ CMD_V_ERBOSE, "verbose", 
		1, "*verbose mode (0)(0-7)" },
	{ CMD_W_IDTH, "width",
		1, "the whole width of the thumbnail canvas" },
	{ CMD_SUFFI_X, "suffix",  
		1, "the suffix of output filename (_thumb)" },
	{ CMD_ACCURATE, "accurate", 
		0, "take accurate shots including P-frames" },
	{ CMD_BKGROUND, "background", 
		1, "the background picture" },
	{ CMD_OTF, "decode-otf", 
		1, "*decoding on the fly mode for scan process" },
	{ CMD_DEPTH, "depth",   
		1, "most levels of directories recursively (FF:0)" },
	{ CMD_EDGE, "edge",
		1, "the width of the screen shot edge (0)" },
	{ CMD_FILTER, "filter",  
		1, "the filter of the extended file name" },
	{ CMD_GAP_SHOT, "gap-shots",  
		1, "the gaps between the screen shots (4)" },
	{ CMD_GAP_MARG, "gap-margin", 
		1, "the margin in the canvas (8)" },
	{ CMD_OPT_INFO, "opt-info", 
		1, "the media infomation (lt)(on|off|mt|rt)" },
	{ CMD_OPT_TIME, "opt-time", 
		1, "the timestamp inside the screen shots (rt)(lt|mt|lb}|mb|rb)" },
	{ CMD_OPT_FFR, "opt-ffr",  
		1, "start from the first frame (off)" },
	{ CMD_OPT_LFR, "opt-lfr",  
		1, "end at the last frame (off)" },
	{ CMD_OVERRIDE, "override", 
		1, "override existed thumbnails (copy)"},
	{ CMD_POS_BG, "pos-bg",
		1, "the position of the background image (mc)" },
	{ 0,  NULL, -1, "lt,lc,lb,mt,mc,mb,rt,rc,rb,tt and st,ex,ey,sx,sy" },
	{ CMD_SIZEUNIT, "size-unit",
		1, "the unit of the video size by byte,kb,mb,gb (auto)" },
	{ CMD_TIME_FROM, "time-from",
		1, "the time in video where begins shooting (HH:MM:SS/NN%)" },
	{ CMD_TIME_END, "time-end", 
		1, "the time in video where ends shooting (HH:MM:SS/NN%)" },
	{ CMD_TRANSPRT, "transparent", 
		0, "generate the transparent background" },
	{ CMD_F_RESET, "factory-reset",
		0, "recover to the default settings" },
	{ CMD_VID_IDX, "vindex",
		1, "*the index of the video stream" },
	{ CMD_HELP, "help",
		0, "*Display the help message" },
	{ CMD_VERSION, "version", 
		0, "*Display the version message" },
	{ 0, NULL, 0, NULL }
};


static	char	*version = "\
ezthumb " VERSION ", to generate the thumbnails from video files.\n\n\
Copyright (C) 2011 \"Andy Xuming\" <xuming@users.sourceforge.net>\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n";

const	char	*description = "\
A video thumbnail generator based on FFMPEG library.\n\
\n\
Copyright (C) 2011-2015 \"Andy Xuming\" <xuming@users.sourceforge.net>\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n";

const	char	*credits = "\
FFmpeg Win32 shared build by Kyle Schwarz from Zeranoe's:\n\
http://ffmpeg.zeranoe.com/builds\n\
You can find source codes and copyrights of FFMPEG at\n\
https://www.ffmpeg.org\n\
\n\
Following Libraries were grabbed from GnuWin:\n\
http://sourceforge.net/projects/gnuwin32/files\n\
\n\
gd-2.0.35\n\
jpeg-6b-4\n\
libiconv-1.9.2-1\n\
libpng-1.2.37\n\
zlib-1.2.8\n\
freetype-2.6.3\n\
\n\
The icon is a public domain under GNU Free Documentation License:\n\
http://commons.wikimedia.org/wiki/File:SMirC-thumbsup.svg\n\
\n\
The GUI frontend is based on IUP, a multi-platform toolkit for building\n\
graphical user interfaces.\n\
http://webserver2.tecgraf.puc-rio.br/iup\n\
\n\
This program was inspired by movie thumbnailer (mtn):\n\
http://sourceforge.net/projects/moviethumbnail\n";


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

#define PROFLIST	((int)(sizeof(sysprof)/sizeof(int*)))

static	EZOPT	sysopt;

static int command_line_parser(int argc, char **argv, EZOPT *opt);
static int signal_handler(int sig);
static int main_close(EZOPT *opt);
static int debug_online(int argc, char **argv);
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
static int event_verbose(void *vobj, int event, long param, long opt, void *);
static int event_list(void *vobj, int event, long param, long opt, void *);
static void version_ffmpeg(void);

static int runtime_profile_test(EZOPT *opt, char *cmd);
static void linefeed_count(int n, int mod, char *con, char *coff);
static void print_profile_shots(EZOPT *opt, int min);
static void print_profile_width(EZOPT *opt, int vidw);
static int load_default_config(EZOPT *opt);
static int ezdebug_trans_module(int cw, char *buf, int blen);

int main(int argc, char **argv)
{
	SMMDBG	*dbgc;
	int	i, todo;

	smm_init();			/* initialize the libsmm */
#if	defined(DEBUG) && defined(CFG_WIN32RT) && defined(CFG_GUI_ON)
	dbgc = slog_csoup_open(NULL, "win32.log");
#else
	dbgc = slog_csoup_open(NULL, NULL);
#endif
	slog_translate_setup(dbgc, SLOG_TRANSL_MODUL,
			ezdebug_trans_module);

	ezopt_init(&sysopt, sysprof[0]);	/* the default setting */
	load_default_config(&sysopt);	/* load configures from files */
	env_init(&sysopt);		/* load configures from environment */

	if (gdFTUseFontConfig(1) == 0) {	/* fontconfig not available */
		ezttf_open();
	}

#ifndef	CFG_GUI_OFF
	if (command_line_parser(argc, argv, NULL) == CMD_G_UI) {
		/* initialize the GUI module and read the configure file */
		sysopt.gui =  ezgui_init(&sysopt, &argc, &argv);
	}
#endif
	todo = command_line_parser(argc, argv, &sysopt);
	CDB_DEBUG(("Todo: %c(%d) ARG=%d/%d\n", todo, todo, optind, argc));
	CDB_DEBUG(("CodePage: %d\n", smm_sys_cp));
	
	smm_signal_break(signal_handler);

#ifdef	HAVE_AVCODEC_REGISTER_ALL
	avcodec_register_all();
#endif
#ifdef	HAVE_AV_REGISTER_ALL
	av_register_all();
#endif

	switch (todo) {
	case CMD_ERROR:
		CDB_ERROR(("Invalid parameters.\n"));
		todo = EZ_ERR_PARAM;
		break;
	case CMD_HELP:		/* help */
		//csc_cli_print(clist, 0, NULL);
		debug_online(argc - optind, argv + optind);
		todo = EZ_ERR_EOP;
		break;
	case CMD_VERSION:	/* version */
		puts(version);
		version_ffmpeg();
		todo = EZ_ERR_EOP;
		break;
	case CMD_F_RESET:
		smm_config_delete(SMM_CFGROOT_DESKTOP, 
				"ezthumb", "ezthumb.conf");
		todo = EZ_ERR_EOP;
		break;
	case CMD_P_ROFILE:	/* print the internal profile table */
		for (i = 0; i < PROFLIST; i++) {
			CDB_SHOW(("%2d: %s\n", i, sysprof[i]));
		}
		todo = EZ_ERR_EOP;
		break;

	case CMD_I_NFO:
	case CMD_I_NSIDE:
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
	case CMD_B_IND:
		if (argc - optind < 1) {
			csc_cli_print(clist, 0, NULL);
			todo = EZ_ERR_EOP;
			break;
		}
		/* inject the progress report functions */
		if (EZOP_DEBUG(sysopt.flags) <= SLOG_LVL_DEBUG) {
			sysopt.notify = event_cb;
		} else {
			sysopt.notify = event_verbose;
		}
#ifndef CFG_GUI_OFF
		ezbar_init(&sysopt);
#endif
		eznotify(&sysopt, EN_BATCH_BEGIN, 0, 0, NULL);
		todo = ezthumb_bind(argv + optind, argc - optind, &sysopt);
		eznotify(&sysopt, EN_BATCH_END, 0, 0, NULL);
		break;
	case CMD_G_UI:
		todo = EZ_ERR_EOP;
		if (sysopt.gui == NULL) {
			csc_cli_print(clist, 0, NULL);
		}
#ifndef	CFG_GUI_OFF
		else {
			todo = ezgui_run(sysopt.gui, argv+optind, argc-optind);
		}
#endif
		break;
	default:
		/* inject the progress report functions */
		if (EZOP_DEBUG(sysopt.flags) < SLOG_LVL_DEBUG) {
			sysopt.notify = event_cb;
		} else {
			sysopt.notify = event_verbose;
		}
#ifndef CFG_GUI_OFF
		ezbar_init(&sysopt);
#endif
		eznotify(&sysopt, EN_BATCH_BEGIN, 0, 0, NULL);
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
		eznotify(&sysopt, EN_BATCH_END, 0, 0, NULL);
		break;
	}
	main_close(&sysopt);
	ezttf_close();	/* closing is safe */
	smm_destroy();
	return todo;
}


/* return:
 *  >= 0 : todo
 *  == -1: unset
 *  == -2: command line error
 */
static int command_line_parser(int argc, char **argv, EZOPT *opt)
{
	EZOPT	*dummy = NULL;
	void	*rtbuf;
	char	*p, tmp[64];
	int	c, todo, prof_grid, prof_size;

	if ((rtbuf = csc_cli_getopt_open(clist, &optind)) == NULL) {
		return CMD_ERROR;
	}

	if (opt == NULL) {
		opt = dummy = smm_alloc(sizeof(EZOPT));
		if (opt == NULL) {
			csc_cli_getopt_close(rtbuf);
			return CMD_ERROR;
		}
	}

	todo = CMD_UNSET;		/* UNSET yet */
	prof_grid = prof_size = 1;	/* enable the profile */
	while ((c = csc_cli_getopt(argc, argv, rtbuf)) > 0) {
		switch (c) {
		case CMD_HELP:
		case CMD_VERSION:
		case CMD_F_RESET:
			todo = c;
			goto break_parse;	/* break the analysis */

		case CMD_ACCURATE:	/* nonkey */
			opt->flags |= EZOP_P_FRAME;
			break;
		case CMD_BKGROUND:
			if (opt->background) {
				smm_free(opt->background);
			}
			opt->background = csc_strcpy_alloc(optarg, 0);
			break;
		case CMD_GAP_SHOT:	
			/* gap-shots: Examples: 5, 5%, 5x8, 5%x8% */
			if (!isdigit(*optarg)) {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
			}
			opt->grid_gap_w = para_get_ratio(optarg);
			if ((p = strchr(optarg, 'x')) == NULL) {
				opt->grid_gap_h = opt->grid_gap_w;
			} else {
				opt->grid_gap_h = para_get_ratio(++p);
			}
			break;
		case CMD_GAP_MARG:	
			/* gap-margin: Examples: 5, 5%, 5x8, 5%x8% */
			if (!isdigit(*optarg)) {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
			}
			opt->grid_rim_w = para_get_ratio(optarg);
			if ((p = strchr(optarg, 'x')) == NULL) {
				opt->grid_rim_h = opt->grid_rim_w;
			} else {
				opt->grid_rim_h = para_get_ratio(++p);
			}
			break;
		case CMD_OPT_INFO:	/* opt-info */
			if (!strcmp(optarg, "on")) {
				opt->flags |= EZOP_INFO;
			} else if (!strcmp(optarg, "off")) {
				opt->flags &= ~EZOP_INFO;
			} else if ((c = para_get_position(optarg)) == -1) {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
			} else {
				opt->flags |= EZOP_INFO;
				opt->mi_position = c;
			}
			break;
		case CMD_OPT_TIME:	/* opt-time */
			if (!strcmp(optarg, "on")) {
				opt->flags |= EZOP_TIMEST;
			} else if (!strcmp(optarg, "off")) {
				opt->flags &= ~EZOP_TIMEST;
			} else if ((c = para_get_position(optarg)) == -1) {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
			} else {
				opt->flags |= EZOP_TIMEST;
				opt->ins_position = c;
			}
			break;
		case CMD_OPT_FFR:	/* opt-ffr */
			if (!strcmp(optarg, "on")) {
				opt->flags |= EZOP_FFRAME;
			} else if (!strcmp(optarg, "off")) {
				opt->flags &= ~EZOP_FFRAME;
			} else {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
			}
			break;
		case CMD_OPT_LFR:	/* opt-lfr */
			if (!strcmp(optarg, "on")) {
				opt->flags |= EZOP_LFRAME;
			} else if (!strcmp(optarg, "off")) {
				opt->flags &= ~EZOP_LFRAME;
			} else {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
			}
			break;
		case CMD_POS_BG:	/* "pos-bg" */
			if ((c = para_get_position(optarg)) == -1) {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
			} else {
				opt->bg_position = c;
			}
			break;
		case CMD_OTF:	/* decode-on-the-fly */
			if (!strcmp(optarg, "on")) {
				opt->flags |= EZOP_DECODE_OTF;
			} else if (!strcmp(optarg, "off")) {
				opt->flags &= ~EZOP_DECODE_OTF;
			} else {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
			}
			break;
		case CMD_TIME_FROM:	/* time-from */
			if (isdigit(*optarg)) {
				opt->time_from = para_get_time_point(optarg);
			} else {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
			}
			break;
		case CMD_TIME_END:	/* time-end */
			if (isdigit(*optarg)) {
				opt->time_to = para_get_time_point(optarg);
			} else {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
			}
			break;
		case CMD_TRANSPRT:	/* set transparent */
			meta_transparent_option(opt, EZOP_TRANSPARENT);	
			break;
		case CMD_VID_IDX:	/* index */
			if (!isdigit(*optarg)) {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
			} else {
				opt->vs_user = strtol(optarg, NULL, 0);
			}
			break;

		case CMD_OVERRIDE:
			if (!strcmp(optarg, "on")) {
				EZOP_THUMB_SET(opt->flags, EZOP_THUMB_OVERRIDE);
			} else if (!strcmp(optarg, "off")) {
				EZOP_THUMB_SET(opt->flags, EZOP_THUMB_SKIP);
			} else if (!strcmp(optarg, "copy")) {
				EZOP_THUMB_SET(opt->flags, EZOP_THUMB_COPY);
			} else {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
			}
			break;
		case CMD_DEPTH:
			/* Recursive depth, for example: "FF:3"
			 * Magic words in first two characters: FF, DF, DL.
			 * First-meet-Firt-process, Directory-First, 
			 * Directory-Last 
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
		case CMD_SIZEUNIT:
			if (!strcmp(optarg, "byte")) {
				opt->size_unit = 'b';
			} else if (!strcmp(optarg, "kb")) {
				opt->size_unit = 'k';
			} else if (!strcmp(optarg, "mb")) {
				opt->size_unit = 'm';
			} else if (!strcmp(optarg, "gb")) {
				opt->size_unit = 'g';
			} else {
				opt->size_unit = 'a';
			}
			break;
		case CMD_FILTER:
			/* file name filter, for example: "avi,wmv,mkv"
 			 * The NULL filter means allow all. */
			if (opt->accept) {
				csc_extname_filter_close(opt->accept);
			}
			opt->accept = csc_extname_filter_open(optarg);
			break;

		case CMD_B_IND:
			todo = c;
			break;
		case CMD_C_OLOR:	/* RRGGBB:RRGGBB:RRGGBB */
			if (para_get_color(opt, optarg) != EZ_ERR_NONE) {
				todo = CMD_ERROR; /* command line error */
				goto break_parse; /* break the analysis */
			}
			break;
		case CMD_D_URING:	/* Examples: 0,1,quick,skim,scan */
			c = lookup_idnum_string(id_duration, -1, optarg);
			if (c == -1) {
				todo = CMD_ERROR; /* command line error */
				goto break_parse; /* break the analysis */
			}
			SETDURMOD(opt->flags, c);
			break;
		case CMD_EDGE:
			if (!isdigit(*optarg)) {
				todo = CMD_ERROR; /* command line error */
				goto break_parse; /* break the analysis */
			} else {
				opt->edge_width = strtol(optarg, NULL, 0);
			}
			break;
		case CMD_F_ONT:
			opt->mi_font = opt->ins_font = optarg;
			break;
		case CMD_F_ONTSZ:	/* MI:TM */
			if (para_get_fontsize(opt, optarg) != EZ_ERR_NONE) {
				todo = CMD_ERROR; /* command line error */
				goto break_parse; /* break the analysis */
			}
			break;
		case CMD_G_RID:	/* Examples: 4, 4x8, 0x8 */
			if (!isdigit(*optarg)) {
				todo = CMD_ERROR; /* command line error */
				goto break_parse; /* break the analysis */
			}
			opt->grid_col = strtol(optarg, &p, 10);
			if (*p == 0) {
				opt->grid_row = 0;
			} else {
				opt->grid_row = strtol(++p, NULL, 10);
			}
			prof_grid = 0;	/* disable the profile */
			break;
		case CMD_G_UI:
			todo = c;
			break;
		case CMD_I_NSIDE:
			todo = c;
			/* make these options default */
			opt->flags |= EZOP_CLI_INSIDE;
			break;
		case CMD_I_NFO:
			todo = c;
			opt->flags |= EZOP_CLI_INFO;
			break;
		case CMD_FOR_M_AT:	
			/* Examples: png, jpg@90, gif, gif@1000 */
			opt->img_format = meta_image_format(optarg);
			break;
		case CMD_O_UTPUT:
			opt->pathout = optarg;
			break;
		case CMD_P_ROCESS:
			csc_strlcpy(tmp, optarg, sizeof(tmp));
			if ((p = strchr(tmp, '@')) != NULL) {
				*p++ = 0;
				opt->key_ripno = (int)strtol(p, NULL, 0);
			}
			c = lookup_idnum_string(id_process, -1, tmp);
			if (c == -1) {
				todo = CMD_ERROR; /* command line error */
				goto break_parse; /* break the analysis */
			}
			EZOP_PROC_MAKE(opt->flags, c);
			if (c == EZOP_PROC_KEYRIP) {
				opt->grid_col = 0;
				opt->grid_row = 0;
				prof_grid = 0;	/* disable the profile */
			}
			break;

		case CMD_P_ROFILE:
			c = strtol(optarg, &p, 10);
			if (*p != 0) {	/* command line profiles */
				ezopt_profile_setup(opt, optarg);
			} else if ((c >= 0) && (c < PROFLIST)) {
				ezopt_profile_setup(opt, sysprof[c]);
			} else {	/* wrong profile index */
				todo = CMD_P_ROFILE;
				goto break_parse;  /* break the analysis */
			}
			break;
		case CMD_R_ECURS:
			opt->flags |= EZOP_RECURSIVE;
			break;
		case CMD_S_IZE:	/* Examples: 50, 50%, 320x240 */
			if (!isdigit(*optarg)) {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
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
		case CMD_T_IMES:
			if (!isdigit(*optarg)) {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
			}
			opt->grid_row = 0;
			prof_grid = 0;	/* disable the profile */
			opt->tm_step = strtol(optarg, NULL, 0);
			//if (opt->tm_step < 1000) {/* at least 1 sec */
			//	opt->tm_step = 0;
			//}
			break;
		case CMD_V_ERBOSE:
			if (!strcmp(optarg, "show")) {
				c = SLOG_LVL_AUTO;
			} else if (!strcmp(optarg, "error")) {
				c = SLOG_LVL_ERROR;
			} else if (!strcmp(optarg, "warning")) {
				c = SLOG_LVL_WARNING;
			} else if (!strcmp(optarg, "info")) {
				c = SLOG_LVL_INFO;
			} else if (!strcmp(optarg, "debug")) {
				c = SLOG_LVL_DEBUG;
			} else if (!strcmp(optarg, "program")) {
				c = SLOG_LVL_PROGRAM;
			} else if (!strcmp(optarg, "module")) {
				c = SLOG_LVL_MODULE;
			} else if (!strcmp(optarg, "function")) {
				c = SLOG_LVL_FUNC;
			} else if (isdigit(*optarg)) {
				c = strtol(optarg, NULL, 0);
			} else {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
			}
			EZOP_DEBUG_MAKE(opt->flags, c);
			CDB_SET_LEVEL(c);
			break;
		case CMD_W_IDTH:
			if (!isdigit(*optarg)) {
				todo = CMD_ERROR;  /* command line error */
				goto break_parse;  /* break the analysis */
			}
			opt->canvas_width = strtol(optarg, NULL, 0);
			prof_size = 0;	/* disable the profile */
			break;
		case CMD_SUFFI_X:
			csc_strlcpy(opt->suffix, optarg, sizeof(opt->suffix));
			break;
		case CMD_PROGRESS:
			opt->flags |= EZOP_PROGRESS_BAR;
			break;
		default:
			todo = CMD_ERROR;	/* command line error */
			goto break_parse;	/* break the analysis */
		}
	}

break_parse:
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
	if ((todo == CMD_UNSET) && (optind >= argc) &&
			((opt->flags & EZOP_RECURSIVE) == 0)) {
		todo = CMD_G_UI;
	}
	/* review the command option structure to make sure there is no
	 * controdicted options */
	ezopt_review(opt);

	if (dummy) {
		main_close(dummy);
		smm_free(dummy);
	}
	csc_cli_getopt_close(rtbuf);
	return todo;
}


static int signal_handler(int sig)
{
	CDB_WARN(("Signal %d\n", sig));
	sig = ezthumb_break(&sysopt);
	main_close(&sysopt);
	ezttf_close();	/* closing is safe */
	smm_destroy();
	return sig;
}

static int main_close(EZOPT *opt)
{
#ifndef	CFG_GUI_OFF
	if (opt->gui) {
		ezgui_close(opt->gui);
		opt->gui = NULL;
	}
#endif
	if (opt->accept) {
		csc_extname_filter_close(opt->accept);
		opt->accept = NULL;
	}
	if (opt->refuse) {
		smm_free(opt->refuse);
		opt->refuse = NULL;
	}
	if (opt->mi_font) {
		smm_free(opt->mi_font);
		opt->mi_font = NULL;
	}
	if (opt->background) {
		smm_free(opt->background);
		opt->background = NULL;
	}
	return 0;
}

static int debug_online(int argc, char **argv)
{
	KEYCB	*kcb;
	char	*s;

	if (argc <= 0) {
		csc_cli_print(clist, 0, NULL);
		return 0;
	}

	if (!strcmp(*argv, "credits")) {
		CDB_SHOW(("%s", credits));
	} else if (!strcmp(*argv, "pro-test")) {	
		/* test the profile (@length, +width) */
		runtime_profile_test(&sysopt, argv[1]);
	} else if (!strcmp(*argv, "pro-export")) {
		s = ezopt_profile_export_alloc(&sysopt);
		if (s) {
			CDB_SHOW(("%s\n", s));
			smm_free(s);
		}
	} else if (!strcmp(*argv, "filter")) {
		s = csc_extname_filter_export_alloc(sysopt.accept);
		if (s) {
			CDB_SHOW(("%s\n", s));
			smm_free(s);
		}
	} else if (!strcmp(*argv, "ttf")) {
		if (gdFTUseFontConfig(1) == 0) {
			ezttf_major_testing(sysopt.mi_font);
		} else {
			ezttf_open();
			ezttf_major_testing(sysopt.mi_font);
			ezttf_close();
		}
	} else if (!strcmp(*argv, "ttf2")) {
		if (gdFTUseFontConfig(1) == 0) {
			ezttf_general_testing(sysopt.mi_font);
		} else {
			ezttf_open();
			ezttf_general_testing(sysopt.mi_font);
			ezttf_close();
		}
	} else if (!strcmp(*argv, "config")) {
		kcb = csc_cfg_open(SMM_CFGROOT_CURRENT, 
				NULL, "ezthumb_sample.rc", CSC_CFG_RWC);
		if (kcb) {
			int	items;
			ezopt_store_config(&sysopt, kcb);
			s = csc_cfg_status(kcb, &items);
			CDB_SHOW(("Read %d configures: %s\n", items, s));
			csc_cfg_close(kcb);
		}
	}
	return 0;
}

static int msg_info(void *option, char *path, int type, void *info)
{
	EZOPT	*ezopt = option;

	(void) info;	/* stop the gcc warning */

	switch (type) {
	case SMM_MSG_PATH_ENTER:
		CDB_SHOW(("Entering %s:\n", path));
		break;
	case SMM_MSG_PATH_EXEC:
		if (csc_extname_filter_match(ezopt->accept, path)) {
			ezinfo(path, option, NULL);
		}
		break;
	case SMM_MSG_PATH_BREAK:
		CDB_WARN(("Failed to process %s\n", path));
		break;
	case SMM_MSG_PATH_LEAVE:
		CDB_SHOW(("Leaving %s\n", path));
		break;
	}
	return SMM_NTF_PATH_NONE;
}

static int msg_shot(void *option, char *path, int type, void *info)
{
	EZOPT	*ezopt = option;

	(void) info;	/* stop the gcc warning */

	switch (type) {
	case SMM_MSG_PATH_ENTER:
		CDB_SHOW(("Entering %s:\n", path));
		break;
	case SMM_MSG_PATH_EXEC:
		if (csc_extname_filter_match(ezopt->accept, path)) {
			CDB_FUNC(("EZTHUMB %s\n", path));
			ezthumb(path, option);
		}
		break;
	case SMM_MSG_PATH_BREAK:
		CDB_WARN(("Failed to process %s\n", path));
		break;
	case SMM_MSG_PATH_LEAVE:
		CDB_SHOW(("Leaving %s\n", path));
		break;
	}
	return SMM_NTF_PATH_NONE;
}

static int env_init(EZOPT *ezopt)
{
	char	*vcmd, *env, *arg[128];	/* should be enough */
	int	len;

	if ((env = getenv("EZTHUMB")) == NULL) {
		return 0;
	}	
	if ((vcmd = csc_strcpy_alloc("ezthumb ", strlen(env)+4)) == NULL) {
		return -1;
	}
	strcat(vcmd, env);

	len = csc_ziptoken(vcmd, arg, 128, " ");

#ifdef	DEBUG
	{
		int	i;
		for (i = 0; i <= len; i++) {
			CDB_DEBUG(("%d: %s\n", i, arg[i]));
		}
	}
#endif
	command_line_parser(len, arg, ezopt);
	smm_free(vcmd);
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
	char	*tmp, *argvs[8];
	int	argcs, val = 0;

	if (strchr(s, '%')) {
		val = strtol(s, NULL, 0);
	       	if (val > 0) {
			val |= EZ_RATIO_OFF;
		}
		return val;
	}

	if ((tmp = csc_strcpy_alloc(s, 0)) == NULL) {
		return EZ_ERR_LOWMEM;
	}
	argcs = csc_ziptoken(tmp, argvs, sizeof(argvs)/sizeof(char*), ":");
	switch (argcs) {
	case 0:	/* 20110301: in case of wrong input */
		CDB_SHOW(("Incorrect time format. Try HH:MM:SS or NN%%.\n"));
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
		smm_free(tmp);
		return val * 1000 + strtol(argvs[3], NULL, 0);
	}
	smm_free(tmp);
	return val * 1000;
}


/* The lists of positions are: lt, lc, lb, mt, mc, mb, rt, rc, rb, tt
 * The qualifications are: st, ex, ey, sx, sy
 * The position and qualification are seperated by ':'.
 * For example: "lt", "lt:st" */
static int para_get_position(char *s)
{
	char	*tmp, *argvs[4];
	int	argcs;

	if ((tmp = csc_strcpy_alloc(s, 0)) == NULL) {
		return EZ_ERR_LOWMEM;
	}
	argcs = csc_ziptoken(tmp, argvs, sizeof(argvs)/sizeof(char*), ":");
	if (argcs < 2) {
		argcs = para_make_postition(s);
	} else {
		argcs = para_make_postition(argvs[0]) | 
			para_make_postition(argvs[1]);
	}
	smm_free(tmp);
	return argcs;
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
	char	*rp, *tmp, *argvs[3];

	if ((tmp = csc_strcpy_alloc(s, 0)) == NULL) {
		return EZ_ERR_LOWMEM;
	}
	csc_fixtoken(tmp, argvs, sizeof(argvs)/sizeof(char*), ":");

	rp = argvs[0];
	if (rp && *rp && isxdigit(*rp)) {
		meta_make_color(rp, opt->mi_color);
	}

	rp = argvs[1];
	if (rp && *rp && isxdigit(*rp)) {
		meta_make_color(rp, opt->ins_color);
	}

	rp = argvs[2];
	if (rp && *rp && isxdigit(*rp)) {
		meta_make_color(rp, opt->canvas_color);
	}
	smm_free(tmp);
	return EZ_ERR_NONE;
}

static int para_get_fontsize(EZOPT *opt, char *s)
{
	char	*rp, *tmp, *argvs[3];

	if ((tmp = csc_strcpy_alloc(s, 0)) == NULL) {
		return EZ_ERR_LOWMEM;
	}
	csc_fixtoken(tmp, argvs, sizeof(argvs)/sizeof(char*), ":");

	rp = argvs[0];
	if (rp && *rp) {
		if (isdigit(*rp)) {
			opt->mi_size = (int) strtol(rp, NULL, 0);
		} else {
			smm_free(tmp);
			return EZ_ERR_PARAM;
		}
	}
	rp = argvs[1];
	if (rp && *rp) {
		if (isdigit(*rp)) {
			opt->ins_size = (int) strtol(rp, NULL, 0);
		} else {
			smm_free(tmp);
			return EZ_ERR_PARAM;
		}
	}
	smm_free(tmp);
	return EZ_ERR_NONE;
}

static int event_cb(void *vobj, int event, 
		long param, long opt, void *block)
{
	int	expect;
	static	int	dotted;

	(void) vobj;		/* stop the gcc warning */
	(void) block;		/* stop the gcc warning */
	switch (event) {
	case EN_PROC_BEGIN:
		dotted = 0;
		return EN_EVENT_PASSTHROUGH;
	
	case EN_PROC_BINDING:
		CDB_SHOW(("\b\b\b\b\b+     "));
		break;

	case EN_PROC_CURRENT:
		if (param == 0) {	/* for key frame saving only */
			CDB_SHOW(("."));
			break;
		}

		expect = opt * 100 / param / 2;
		if (dotted >= expect) {
			CDB_SHOW(("\b\b\b\b%3ld%%", opt * 100 / param));
		} else while (dotted < expect) {
			CDB_SHOW(("\b\b\b\b\b. %3ld%%", opt * 100 / param));
			dotted++;
		}
		break;
	case EN_PROC_END:
		if (param == 0) {       /* for key frame saving only */
			CDB_SHOW(("\b\b\b\b100%% done\n"));
		} else {
			CDB_SHOW(("\b\b\b\b%ldx%ld done\n", param, opt));
		}
		break;

	default:
		return EN_EVENT_PASSTHROUGH;
	}
	return event;
}

static int event_verbose(void *vobj, int event, 
		long param, long opt, void *block)
{
	EZOPT	*ezopt = vobj;
	void	*ftmp;

	switch (event) {
	case EN_OPEN_BEGIN:
	case EN_OPEN_END:
		break;
	case EN_PROC_BEGIN:
		ftmp = (void*) ezopt->notify;
		ezopt->notify = NULL;
		eznotify(ezopt, event, param, opt, block);
		ezopt->notify = ftmp;
		CDB_SHOW(("\n"));
		break;
	case EN_PROC_BINDING:
		CDB_SHOW(("Binding next.\n"));
		break;
	case EN_PROC_CURRENT:
		break;
	default:
		return EN_EVENT_PASSTHROUGH;
	}
	return event;
}

static int event_list(void *vobj, int event, 
		long param, long opt, void *block)
{
	(void) vobj;		/* stop the gcc warning */
	(void) param;		/* stop the gcc warning */
	(void) opt;		/* stop the gcc warning */
	(void) block;		/* stop the gcc warning */

	switch (event) {
	case EN_OPEN_BEGIN:
	case EN_OPEN_END:
		break;
	default:
		if (event >= 0) {
			return EN_EVENT_PASSTHROUGH;
		}
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

	if (cmd) {
		switch (*cmd) {
		case '@':
			val = (int)strtol(cmd + 1, 0, 10);
			print_profile_shots(opt, val);
			CDB_SHOW(("\n"));
			return 0;
		case '+':
			val = (int)strtol(cmd + 1, 0, 10);
			print_profile_width(opt, val);
			CDB_SHOW(("\n"));
			return 0;
		}
	}

	CDB_SHOW(("Reference of Video Length:\n"));
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

	CDB_SHOW(("Reference of Width:\n"));
	for (i = 0; i < (int)(sizeof(stdres)/sizeof(int)); i++) {
		print_profile_width(opt, stdres[i]);
		linefeed_count(i, 3, "\n", "    ");
	}
	linefeed_count(i, 3, "", "\n");
	return 0;
}

static void linefeed_count(int n, int mod, char *con, char *coff)
{
	if ((n % mod) == (mod - 1)) {
		CDB_SHOW(("%s", con));
	} else {
		CDB_SHOW(("%s", coff));
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
	CDB_SHOW(("[%4d]=[%4d %4d %3d %4d]", min, wid, hei, fac, 
			wid * hei > 0 ? min * 60 / (wid * hei) : 0));
}

static void print_profile_width(EZOPT *opt, int vidw)
{
	int	wid, hei, fac, can;

	wid = hei = fac = 0;
	can = ezopt_profile_zooming(opt, vidw, &wid, &hei, &fac);
	CDB_SHOW(("[%4d]=[%4d %4d %3d %4d]", vidw, wid, hei, fac, can));
}

static int load_default_config(EZOPT *opt)
{
	KEYCB	*config;
	int	items;
	char	*path;

	config = csc_cfg_open(SMM_CFGROOT_SYSTEM,
			"ezthumb", "ezthumbrc", CSC_CFG_READ);
	if (config) {
		ezopt_load_config(opt, config);
		if (EZOP_DEBUG(opt->flags) >= SLOG_LVL_INFO) {
			path = csc_cfg_status(config, &items);
			CDB_INFO(("Read %d configures: %s\n", items, path));
		}
		csc_cfg_close(config);
	}

	config = csc_cfg_open(SMM_CFGROOT_USER,
			"ezthumb", "ezthumbrc", CSC_CFG_READ);
	if (config) {
		ezopt_load_config(opt, config);
		if (EZOP_DEBUG(opt->flags) >= SLOG_LVL_INFO) {
			path = csc_cfg_status(config, &items);
			CDB_INFO(("Read %d configures: %s\n", items, path));
		}
		csc_cfg_close(config);
	}
	path = path;	/* to eliminate the gcc -Wuninitialized complain */
	return items;
}

static int ezdebug_trans_module(int cw, char *buf, int blen)
{
	if (cw & EZTHUMB_MOD_CORE) {
		csc_strlcat(buf, "[EZTHUMB]", blen);
	}
	if (cw & EZTHUMB_MOD_CLI) {
		csc_strlcat(buf, "[CLI]", blen);
	}
	if (cw & EZTHUMB_MOD_GUI) {
		csc_strlcat(buf, "[EZGUI]", blen);
	}
	return SMM_ERR_NULL;
}



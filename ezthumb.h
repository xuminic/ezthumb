
/*  Copyright (C) 2011  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of EZTHUMB, a utility to generate thumbnails

    EZTHUMB is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    EZTHUMB, is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef	_EZTHUMB_H_
#define _EZTHUMB_H_

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>

#include <gd.h>
#include "libcsoup.h"

/* define the modules of libcsoup for easy debugging */
#define EZTHUMB_MOD_CORE	SLOG_MODUL_ENUM(3)
#define EZTHUMB_MOD_CLI		SLOG_MODUL_ENUM(4)
#define EZTHUMB_MOD_GUI		SLOG_MODUL_ENUM(5)

#define EZ_ERR_NONE		0
#define EZ_ERR_EOP		-1	/* End Of Process */
#define EZ_ERR_STREAM		-2	/* wrong stream */
#define EZ_ERR_CODEC_FAIL	-3	/* wrong codec */
#define EZ_ERR_CODEC_NONE	-4	/* unsupported codec */
#define EZ_ERR_LOWMEM		-5
#define EZ_ERR_SWSCALE		-6	/* failed to initial swscale */
#define EZ_ERR_IMG_FORMAT	-7	/* unknown wallpaper format */
#define EZ_ERR_PARAM		-8	/* invalid command line parameter */
#define EZ_ERR_FORMAT		-9	/* wrong video format */
#define EZ_ERR_VIDEOSTREAM	-10	/* no viden stream */
#define EZ_ERR_FILE		-11	/* can not open the video file */
#define EZ_ERR_KEYCFG		-12	/* configure key doesn't exist */


#define EN_FILE_OPEN		1000	/* successfully open a video file */
#define EN_MEDIA_OPEN		1001	/* successfully open the media file */
#define EN_IMAGE_CREATED	1002	/* successfully created the EZIMG */
#define EN_PROC_BEGIN		1003	/* start to process the video */
#define EN_PROC_CURRENT		1004	/* current process */
#define EN_PROC_END		1005	/* end of the process */
#define EN_PROC_SAVED		1100
#define EN_PROC_BINDING		1101
#define EN_PACKET_RECV		1006	/* received an effective packet */
#define EN_FRAME_DONE		1007	/* decoded a frame */
#define EN_FRAME_EFFECT		1009	/* received an effective frame */
#define EN_SCAN_PACKET		1010	/* received a packet in scan mode */
#define EN_SCAN_IFRAME		1011
#define EN_TYPE_VIDEO		1012
#define EN_TYPE_AUDIO		1013
#define EN_TYPE_SUBTTL		1014
#define EN_TYPE_UNKNOWN		1015
#define EN_DURATION		1016
#define EN_PACKET_KEY		1017
#define EN_BUMP_BACK		1018
#define EN_DTS_LIST		1019
#define EN_STREAM_FORMAT	1021
#define EN_STREAM_INFO		1022
#define EN_MEDIA_STATIS		1023
#define EN_STREAM_BROKEN	1024
#define EN_IFRAME_CREDIT	1025
#define EN_FRAME_EXCEPTION	1026
#define EN_EVENT_PASSTHROUGH	1027
#define EN_SKIP_EXIST		1028
#define EN_OPEN_BEGIN		1030	/* start to check the duration */
#define EN_OPEN_GOING		1031	/* checking duration in progress */
#define EN_OPEN_END		1032	/* end of the checking */
#define EN_BATCH_BEGIN		1033
#define EN_BATCH_END		1034

#define ENX_DUR_MHEAD		0	/* duration from media head */
#define ENX_DUR_JUMP		1	/* jumping for a quick scan */
#define ENX_DUR_REWIND		2	/* rewinding occurred */
#define ENX_DUR_SCAN		3	/* duration from media scan */

#define ENX_SS_SKIM		0
#define ENX_SS_SCAN		1
#define ENX_SS_TWOPASS		2
#define ENX_SS_HEURIS		3
#define ENX_SS_IFRAMES		4
#define ENX_SS_SAFE		5


#define ENX_SEEK_UNKNOWN	0	/* seeking capablity unknown */
#define ENX_SEEK_NONE		1	/* can not seek at all */
#define ENX_SEEK_FORWARD	2	/* support seeking forward only */
#define ENX_SEEK_FREE		3	/* support seeking freely */
#define SEEKABLE(x)		((x) > ENX_SEEK_NONE)

#define ENX_IFRAME_RESET	0
#define ENX_IFRAME_SET		1
#define ENX_IFRAME_UPDATE	2



#define EZOP_INFO		1	/* include the media info area */
#define EZOP_TIMEST		2	/* include the inset timestamp */
#define EZOP_FFRAME		4	/* start from the first frame */
#define EZOP_LFRAME		8	/* include the last frame */
/* Take shots at any frame, otherwise it only takes shots at key frames. 
 * However, if the shot's step is less than EZ_GATE_KEY_STEP millisecond, 
 * it automatically converts into EZOP_ANYFRAME mode */
#define EZOP_P_FRAME		0x10	
#define SETACCUR(m)		((m) |= EZOP_P_FRAME)
#define GETACCUR(m)		((m) & EZOP_P_FRAME)
#define CLRACCUR(m)		((m) &= ~EZOP_P_FRAME)

/* Display media information in the command line. It just displays the
 * common information, not includes the debug info */
#define EZOP_CLI_INSIDE		0x20
/* Display a short list of the media information in the command line */
#define EZOP_CLI_INFO		0x40
/* Setup the transparent background */
#define EZOP_TRANSPARENT	0x80
/* decoding on the fly */
#define EZOP_DECODE_OTF		0x100
/* font test (obsolete) */
#define EZOP_FONT_TEST		0x200

/* define the process of the existed thumbnails */
#define EZOP_THUMB_COPY		0
#define EZOP_THUMB_OVERRIDE	0x400
#define EZOP_THUMB_SKIP		0x800
#define EZOP_THUMB_MASK		0xC00
#define EZOP_THUMB_SET(m,d)	((m) &= ~EZOP_THUMB_MASK, (m) |= (d))
#define EZOP_THUMB_GET(m)	((m) & EZOP_THUMB_MASK)

/* define the video duration finding mode */
#define EZOP_DUR_AUTO		0		/* try head first or scan */
#define EZOP_DUR_QSCAN		0x1000		/* quick scan */
#define EZOP_DUR_FSCAN		0x2000		/* full scan */
#define EZOP_DUR_HEAD		0x3000		/* head only */
#define EZOP_DUR_MASK		0x3000
#define SETDURMOD(m,d)		((m) &= ~EZOP_DUR_MASK, (m) |= (d))
#define GETDURMOD(m)		((m) & EZOP_DUR_MASK)

/* process the subdirectories if the file name is a folder */
#define EZOP_RECURSIVE		0x8000

/* Process method uses 0xF0000 field */
#define EZOP_PROC_AUTO		0
#define EZOP_PROC_SKIM		1	/* use av_seek_frame() */
#define EZOP_PROC_SCAN		2	/* single pass i-frame scan */
#define EZOP_PROC_TWOPASS	3	/* two pass scan support p-frame */
#define EZOP_PROC_KEYRIP	5	/* rip key frames */
#define EZOP_PROC_SAFE		6	/* safe mode */
#define EZOP_PROC_MASK		15
#define EZOP_PROC_FIELD		16
#define EZOP_PROC(f)		(((f) >> EZOP_PROC_FIELD) & EZOP_PROC_MASK)
#define EZOP_PROC_CLEAR(f)	((f) &= ~(EZOP_PROC_MASK << EZOP_PROC_FIELD))
#define EZOP_PROC_SET(f,p)	((f) |= (((p) & EZOP_PROC_MASK) << \
							EZOP_PROC_FIELD))
#define EZOP_PROC_MAKE(f,p)	(EZOP_PROC_CLEAR(f), EZOP_PROC_SET(f,p))

/* reserved space in 0x0FF00000 */
#define EZOP_PROGRESS_BAR	0x100000  /* standalone progress UI bar */

/* debug use 0xF0000000 mask in the flag word */
#define EZDBG_NONE		SLSHOW	/* no debug information at all */
#define EZDBG_SHOW		(SLSHOW | SLOG_FLUSH)
#define EZDBG_WARNING		SLERR	/* open, close, duration */
#define EZDBG_INFO		SLWARN	/* av info, image info */
#define EZDBG_BRIEF		SLINFO	/* warning from FFMPEG */
#define EZDBG_IFRAME		SLDBG	/* key frame received */
#define EZDBG_PACKET		SLPROG	/* key packet dump */
#define EZDBG_VERBS		SLMOD	/* broken frames, scanned frames */
#define EZDBG_FFM		SLFUNC	/* the FFMPEG debug output */
#define EZDBG_FIELD		28
#define EZOP_DEBUG(f)		(((f) >> EZDBG_FIELD) & SLOG_LVL_MASK)
#define EZOP_DEBUG_CLEAR(f)	((f) &= ~(SLOG_LVL_MASK << EZDBG_FIELD))
#define EZOP_DEBUG_SET(f,x)	((f) |= (((x) & SLOG_LVL_MASK) << EZDBG_FIELD))
#define EZOP_DEBUG_MAKE(f,x)	(EZOP_DEBUG_CLEAR(f),EZOP_DEBUG_SET(f,x))


#define EZ_POS_LEFTTOP		0
#define EZ_POS_LEFTCENTER	1
#define EZ_POS_LEFTBOTTOM	2
#define EZ_POS_MIDTOP		3
#define EZ_POS_MIDCENTER	4
#define EZ_POS_MIDBOTTOM	5
#define EZ_POS_RIGHTTOP		6
#define EZ_POS_RIGHTCENTER   	7
#define EZ_POS_RIGHTBOTTOM	8
#define EZ_POS_TILE		9	/* tile deploy the subject */
#define EZ_POS_MASK		0xff
#define EZ_POS_STRETCH		0x100	/* stretch the subject to fit */
#define EZ_POS_ENLARGE_X	0x200
#define EZ_POS_ENLARGE_Y	0x300
#define EZ_POS_STRETCH_X	0x400
#define EZ_POS_STRETCH_Y	0x500


#define EZ_FONT_AUTO		0
#define EZ_FONT_TINY		8
#define EZ_FONT_SMALL		9
#define EZ_FONT_MEDIUM		10
#define EZ_FONT_LARGE		11
#define EZ_FONT_GIANT		12

#define EZ_TEXT_INSET_GAP	4
#define EZ_TEXT_MINFO_GAP	0
#define EZ_TEXT_SHADOW_OFF	2
#define EZ_SHOT_SHADOW_OFF	3

#define EZ_RATIO_OFF		0x40000000

#define EZ_ST_ALL		0	/* all packet in the whole file */	
#define EZ_ST_KEY		1	/* number of key frames */
#define EZ_ST_REWIND		2	/* number of DTS rewind */
#define EZ_ST_OFF		3
#define EZ_ST_MAX_REC		16	/* maximum monited streams */

#define EZ_PROF_MAX_ENTRY	64	/* maximum profile entries */
#define EZ_PROF_LENGTH		1	/* the length of video */
#define EZ_PROF_WIDTH		2	/* the width of video frame */
#define EZ_PROF_ALL		3

#define EZ_THUMB_VACANT		0	/* thumbnail doesn't exist */
#define EZ_THUMB_OVERRIDE	1	/* file existed: override it */
#define EZ_THUMB_SKIP		2	/* file existed: skip it */
#define EZ_THUMB_COPIABLE	3	/* file existed: make a copy */
#define EZ_THUMB_OVERCOPY	4	/* copy full: override the last one */

/* define image format */
#define EZ_IMG_FMT_MASK		0xf0000000
#define EZ_IMG_FMT_JPEG		0
#define EZ_IMG_FMT_PNG		0x10000000
#define EZ_IMG_FMT_GIF		0x20000000
#define EZ_IMG_FMT_GIFA		0x30000000
#define EZ_IMG_FMT_SET(m,d)	((m) &= ~EZ_IMG_FMT_MASK, (m) |= (d))
#define EZ_IMG_FMT_GET(m)	((m) & EZ_IMG_FMT_MASK)
#define EZ_IMG_PARAM_MASK	0xffffff
#define EZ_IMG_PARAM_SET(m,d)	((m) &= ~EZ_IMG_PARAM_MASK, (m) |= (d))
#define EZ_IMG_PARAM_GET(m)	((m) & EZ_IMG_PARAM_MASK)
#define EZ_IMG_INIT(m,d)	((m) | (d))

/* Duration Seeking Challenge Profile */
#define EZ_DSCP_RANGE_INIT	10000	/* range of initial scan (ms) */
#define EZ_DSCP_RANGE_EXT	10	/* extended rate of initial range */
#define EZ_DSCP_STEP_ERROR	300	/* 30% error acceptable */
#define EZ_DSCP_N_STEP		5	/* number of seeks */

/* define the array of progress time stamp */
#define EZ_PTS_MOPEN		'M'	/* media open */
#define EZ_PTS_DSEEK		'S'	/* video_seek_challenge */
#define EZ_PTS_DSCAN		'A'	/* video_duration_scan */
#define EZ_PTS_COPEN		'C'	/* codec open */
#define EZ_PTS_UPDATE		'U'	/* longest update */
#define EZ_PTS_RESET		0
#define EZ_PTS_CLEAR		1
#define EZ_PTS_MAX		16

#define EZ_DEF_FILTER	"avi,flv,mkv,mov,mp4,mpg,mpeg,rm,rmvb,ts,vob,wmv,"\
			"mpe,mpv,m1v,m2v,mod,mp2,mpa,mpv2,mp2v,asf,asx,wm,wmx"

#define EZBYTE	unsigned char
#define EZTIME	int64_t


#define CFG_KEY_GRID_COLUMN	"grid_column"
#define CFG_KEY_GRID_ROW	"grid_row"
#define CFG_KEY_CANVAS_WIDTH	"canvas_width"
#define CFG_KEY_TIME_STEP	"time_step"
#define CFG_KEY_GRID_GAP_WID	"grid_gap_width"
#define CFG_KEY_GRID_GAP_HEI	"grid_gap_height"
#define CFG_KEY_CANVAS_RIM_WID	"canvas_rim_width"
#define CFG_KEY_CANVAS_RIM_HEI	"canvas_rim_height"
#define CFG_KEY_COLOR_EDGE	"thumbnail_edge_color"
#define CFG_KEY_COLOR_SHADOW	"thumbnail_shadow_color"
#define CFG_KEY_COLOR_CANVAS	"thumbnail_canvas_color"
#define CFG_KEY_EDGE_WIDTH	"thumbnail_edge_width"
#define CFG_KEY_SHADOW_WIDTH	"thumbnail_shadow_width"
#define CFG_KEY_ZOOM_WIDTH	"zoom_width"
#define CFG_KEY_ZOOM_HEIGHT	"zoom_height"
#define CFG_KEY_ZOOM_RATIO	"zoom_ratio"
#define CFG_KEY_INFO_FONT	"media_info_font"
#define CFG_KEY_INFO_SIZE	"media_info_size"
#define CFG_KEY_INFO_COLOR	"media_info_color"
#define CFG_KEY_INFO_SHADOW	"media_info_shadow_size"
#define CFG_KEY_INFO_LAYOUT	"media_info_layout"
#define CFG_KEY_INFO_STATUS	"media_info_status"
#define CFG_KEY_INSET_SIZE	"thumbnail_inset_size"
#define CFG_KEY_INSET_COLOR	"thumbnail_inset_color"
#define CFG_KEY_INSET_SHADOW	"thumbnail_inset_shadow_size"
#define CFG_KEY_INSET_LAYOUT 	"thumbnail_inset_layout"
#define CFG_KEY_FILE_FORMAT	"thumbnail_format"
#define CFG_KEY_FILE_QUALITY	"thumbnail_quality"
#define CFG_KEY_FILE_EXISTED	"thumbnail_existed"
#define CFG_KEY_TRANSPARENCY	"transparency"
#define CFG_KEY_FILE_SUFFIX	"thumbnail_suffix"
#define CFG_KEY_BG_PICTURE	"backgroup_picture"
#define CFG_KEY_BG_LAYOUT	"backgroup_layout"
#define CFG_KEY_BG_QUALITY	"backgroup_quality"
#define CFG_KEY_SUFFIX_FILTER	"accepted_file"
#define CFG_KEY_DURATION	"duration_mode"
#define CFG_KEY_PROF_SIMPLE	"simple_profile"
#define CFG_KEY_MEDIA_PROC	"media_process"

#define CFG_PIC_POS_LFETTOP	"left top"
#define CFG_PIC_POS_LEFTCENTR	"left centre"
#define CFG_PIC_POS_LEFTBOTTOM	"left bottom"
#define CFG_PIC_POS_MIDTOP	"middle top"
#define CFG_PIC_POS_MIDCENTR	"middle centre"
#define CFG_PIC_POS_MIDBOTTOM	"middle bottom"
#define CFG_PIC_POS_RIGHTTOP	"right top"
#define CFG_PIC_POS_RIGHTCENTR 	"right centre"
#define CFG_PIC_POS_RIGHTBOTTOM	"right bottom"
#define CFG_PIC_POS_TILES	"tile"
#define CFG_PIC_QUA_STRETCH	"stretch to canvas"
#define CFG_PIC_QUA_ENLARGE_WID	"enlarge by width"
#define CFG_PIC_QUA_ENLARGE_HEI	"enlarge by height"
#define CFG_PIC_QUA_STRE_WID	"stretch by width"
#define CFG_PIC_QUA_STRE_HEI	"stretch by height"

#define CFG_PIC_DFM_HEAD	"File Head"
#define CFG_PIC_DFM_SCAN	"Full Scan"
#define CFG_PIC_DFM_FAST	"Fast Scan"
#define CFG_PIC_AUTO		"Auto"

#define CFG_PIC_TEX_APPEND	"Create New Thumbnails"
#define CFG_PIC_TEX_OVERRIDE	"Override Existed Thumbnails"
#define CFG_PIC_TEX_SKIP	"Skip Existed Thumbnails"


/* FORMAT: WEIGHT + flag + X + sep + Y + sep + Z
 *
 * Sample shots (weighted by clip length in time, default 0):
 * 12M4x4 : 720S4x4 : create 4x4 thumb matrix when video under 12/720 (m/s)
 * 30L10x50x1.025 : when video under 30 minutes, sample following shots:
 *                  log(1.025)(n+10)-50
 *
 * Zoom of thumbnails (weighted by frame width, default 0):
 * 160W200% : zoom 200% when video width under 160 pixel
 * 160T200: thumbnail sets to 200x(height) when video width under 160 pixel
 * 160T200x80: thumbnail sets to 200x80 when video width under 160 pixel
 * 100F4x1280 : create 4 thumbnails in a row inside a 1280 canvas 
 *              when video width under 100
 * 100R4x320 : create 4 thumbnails in a row, each of them CLOSE to 320 pixels
 *             when video width under 100
 */
typedef	struct	{
	void	*next;
	int	flag;	/* 'M', 'S', 'W', 'L', 'F', 'R' */
	int	weight;	/* time segment in sec, or width in pixel */

	int	x;	/* grid x, or time offset before log()  */
	int	y;	/* grid y, or ratio of width, or fixed canvas, */
			/* or relative width, or error of log() */

	double	lbase;	/* the base of logarithm */
} EZPROF;


/* This structure is used to store the user defined parameters.
 * These parameters are globally avaiable so they affect all video clips. 
 * Most parameters are raw which means the program only uses them indirectly.*/
typedef	struct	{
	/* thumbshots array in the canvas. If grid_col == 0, it will save
	 * screenshot separately, shots count on grid_row. If both grid_col
	 * and grid_row are 0, the shots count on tm_step. If tm_step
	 * is 0 too, it will save every keyframes */
	int	grid_col;	/* number of thumbshots in column (4) */
	int	grid_row;	/* number of thumbshots in row (4) */

	/* the whole width of the thumbnail canvas can be specified first.
	 * the grid_col and grid_row can be decided by other parameters */
	int	canvas_width;	/* canvas width in pixel */
	int	tm_step;	/* timestep in millisecond to take shots */

	/* binary ORed options */
	int	flags;		/* see EZOP_* */

	/* gap and rim size can be decided by pixels if the value is between
	 * 0-255, or by ratio factors if the value is over 1000. For example,
	 * if grid_gap_w is 1003, it means the gap width is 3% of the 
	 * thumbshot's width. The 1000 will be shifted out. */
	int	grid_gap_w;	/* gap width between each thumbshots (2) */
	int	grid_gap_h;	/* gap height between each thumbshots (2) */
	int	grid_rim_w;	/* rim width in the canvas */
	int	grid_rim_h;	/* rim height in the canvas */

	/* color setting is combined by R,G,B and A (array index 0-3) */
	EZBYTE	edge_color[4];		/* edge line wrapping the thumbshot */
	EZBYTE	shadow_color[4];	/* shadow of the thumbshot */
	EZBYTE	canvas_color[4];	/* background color of the canvas */

	/* edge is always inside the thumbshot */
	int	edge_width;	/* thickness of the edge line (0=disable) */
	int	shadow_width;	/* thickness of the shadow (0=disable) */

	/* Scaling setting of single thumbshot. The tn_factor is a 100-based
	 * factor for scaling the original image, for example, 50 means 50%
	 * and 150 means 150%. If tn_factor is enabled, ie > 0, it overwrites
	 * tn_width and tn_height. */
	int	tn_width;	/* width of a thumbnail */
	int	tn_height;	/* height of a thumbnail */
	int	tn_facto;	/* scale facto of a thumbnail (0=disable) */

	/* specify the freetype font and its size. If only one set parameter
	 * is specified, the other one will be copied automatically. 
	 * Shadow color of info area is equal to shadow_color[4] */
	char	*mi_font;	/* freetype font for info area */
	int	mi_size;	/* font size of the info area (0=auto) */
	EZBYTE	mi_color[4];	/* text color of the media info */
	int	mi_shadow;	/* width of media info shadow (shadow_color)*/
	int	mi_position;	/* layout position of the media info */
	int	st_position;	/* layout position of the status line */

	char	*ins_font;	/* freetype font for inset text */
	int	ins_size;	/* font size of the inset text (0=auto) */
	EZBYTE	ins_color[4];	/* text color of the inset text */
	EZBYTE	its_color[4];	/* shadow color of the inset text */
	int	ins_shadow;	/* width of the shadow of the inset text */
	int	ins_position;	/* layout position of the inset text */

	int	img_format;	/* only support jpg, png and gif */
	char	suffix[256];	/* suffix of the target file name (utf-8) */

	char	*background;	/* picture for the canvas background */
	int	bg_position;	/* the position of the background */

	int	time_from;	/* from where to take shots (ms) */
	int	time_to;	/* to where the process end (ms) */
	EZTIME	pre_dura;	/* preset duration in ms */
	int	pre_seek;	/* preset seek flag */
	int	pre_br;		/* preset bitrate */

	int	vs_user;	/* specify the stream index */
	int	key_ripno;	/* specify the number when ripping keyframes*/
	char	*pathout;	/* output path */
	int	grpclips;	/* number of grouped clips */

	/* callback functions to indicate the progress */
	int	(*notify)(void *nobj, int event, long param, long, void *);
	int	(*notiback)(void *nobj, int event, long param, long, void *);

	/* copy of runtime objects for signal breaking */
	void	*vidobj;	/* copy of the runtime EZVID point */
	void	*imgobj;	/* copy of the runtime EZIMG point */
	
	/* GUI pointer */
	void	*gui;

	/* file name filter */
	void	*accept;
	void	*refuse;
	int	r_flags;	/* recursive flags for smm_pathtrek() */

	/* predefined profile structure */
	EZPROF	*pro_grid;	/* profile of the canvas grid */
	EZPROF	*pro_size;	/* profile of the size of each snapshots */
	int	pro_mask;
	EZPROF	pro_pool[EZ_PROF_MAX_ENTRY];
} EZOPT;



typedef	struct	{
	EZOPT	*sysopt;

	/* callback functions to indicate the progress */
	int	(*notify)(void *nobj, int event, long param, long, void *);

	/* copy of runtime objects for signal breaking */
	void	*vidobj;	/* copy of the runtime EZVID point */
	void	*imgobj;	/* copy of the runtime EZIMG point */
	
} RTOPT;

/* This structure is used to store the runtime parameters. Most parameters
 * are transformed from the EZOPT structure. Due to the difference of each
 * video clips, the content of this structure is dynamic in each operation */
typedef	struct	{

	/* scaled frame images */
	int	dst_width;	/* scaled video frame size */
	int	dst_height;
	int	dst_pixfmt;	/* scaled video frame pixel format */

	/* canvas defines */
	int	grid_col;
	int	grid_row;
	int	canvas_width;
	int	canvas_height;
	int	shots;		/* the total screenshots */
	int	taken;		/* number of shots already taken */

	/* time setting: they are all calculated from the duration, not DTS */
	EZTIME	time_from;	/* from where to take shots (ms) */
	EZTIME	time_during;	/* the time range of shots (ms) */
	EZTIME	time_step;	/* timestep in millisecond to take shots */

	/* gaps */
	int	gap_width;	/* gap between shots in column */
	int	gap_height;	/* gap between shots in row */
	int	rim_width;	/* blank area in left and right rim */
	int	rim_height;	/* blank area in top and bottom rim */

	/* font height of media information (gap included) */
	int	mift_height;

	/* colors */
	int	color_canvas;	/* background color of the canvas */
	int	color_shadow;	/* shadow of the thumbshot */
	int	color_minfo;	/* the media info */
	int	color_edge;	/* edge line wrapping the thumbshot */
	int	color_inset;	/* inset text in each shots */
	int	color_inshadow;	/* shadow color of the inset text */

	gdImage	*gdframe;	/* gd context for each frame */
	gdImage	*gdcanvas;	/* gd context for the whole canvas */
	FILE	*gifx_fp;	/* for GIF89 animation */
	int	gifx_opt;	/* for GIF89 animation */

	EZOPT	*sysopt;	/* link to the EZOPT parameters */
	void	*cbparam;	/* the callback parameter block */
	int	canvas_minfo;	/* height of the media info area */
	char	filename[1];	/* file name buffer */
} EZIMG;

typedef	struct		{
	AVFrame		*frame;
	unsigned char	*rf_buffer;	/* the frame buffer */
	int		pixfmt;
	int		width;
	int		height;

	int		keyflag;	/* flag that i-frame received */
	int64_t		rf_pts;		/* the PTS from the decoder */
	int64_t		rf_dts;		/* the DTS from the last packet */
	int64_t		rf_pos;		/* the position of the last packet */
	int		rf_size;	/* total size of packets */
	int		rf_pac;		/* packets number of the frame */
	void		*context;	/* linked packet or sws context */
} EZFRM;


typedef	struct	_EzVid	{
	/*** video_open() / video_close() */
	AVFormatContext	*formatx;	/* must NULL it before use!! */
	AVStream	*vstream;	/* video stream */
	AVCodecContext	*codecx;
	int		vsidx;		/* the index of the video stream */
	int		ezstream;	/* 20130719 recognizable streams */
	int		dts_rate;	/* DTS per frame */

	/*** video_allocate() */
	EZOPT		*sysopt;	/* link to the EZOPT parameters */
	char		*filename;
	int64_t		filesize;
	EZTIME		duration;	/* duration of file in ms */
	int		width;	
	int		height;
	int		streams;	/* total streams in the file */
	int		ar_height;	/* video height after AR correcting */
	int		bitrates;	/* referenced by seek challenge */
	EZTIME		dts_offset;	/* DTS could start from here */
	int		seekable;	/* video keyframe seekable flag */

	/*** video_alloc_queue() */
	EZTIME		dur_all;	/* total duration of all clips */
	EZTIME		dur_off;	/* total duration before this clip */
	int64_t		bind_size;	/* total size of all clips */
	int		bound;		/* total bound clips */
	int		bind_idx;	/* the index number in the queue */

	/*** override fields of EZOPT */
	int		ses_dura;	/* session duration mode */
	int		ses_flags;

	/*** video_connect() / video_disconnect() */
	/* frame extracted and scaled from the video stream */
	EZFRM		*swsframe;	/* scaled frame */
	EZFRM		*picframe;	/* store the recent good frame */
	EZFRM		*vidframe;	/* the decoding frame */

	/* capture video frame for debugging */
#if	defined(CFG_SNAPSHOT_DUMP) || defined(CFG_SNAPSHOT_RGB)
	EZFRM		*capframe;
#endif
#ifdef	CFG_SNAPSHOT_DUMP
	gdImage		*capgdimg;
#endif

	/*** runtime variables in each session */
	SMM_TIME	tmark;		/* the beginning timestamp */
	int		pts[EZ_PTS_MAX<<1]; /* progress timestamp array */
	int		pidx;		/* PTS array index */

	int64_t		keygap;		/* maximum DTS between keyframe */
	int64_t		keydts;		/* avarage DTS per key frame */
	int64_t		keyalldts;	/* total surveyed DTS */
	int64_t		keyallkey;	/* total surveyed key frame */
	int64_t		keyfirst;	/* first DTS since reset */
	int64_t		keylast;	/* the DTS of recent keyframe */
	unsigned	keycount;	/* received keyframes since reset */
	int64_t		keydelta;	/* the delta DTS of snapshots */

	struct	_EzVid	*anchor;	/* always pointing to the anchor */
	struct	_EzVid	*next;
} EZVID;


#define uperror(n,c)	{ if (n) *(n) = (c); }

#define EZ_HI_WORD(n)	(((n) >> 16) & 0xffff)
#define EZ_LO_WORD(n)	((n) & 0xffff)
#define EZ_MK_WORD(w,h)	(((w) << 16) | ((h) & 0xffff))


struct	MeStat		{	/* media statistics */
	unsigned long	received;	/* all received packets */
	unsigned long	key;		/* key frames */
	unsigned long	rewound;	/* rewound occurance counter */

	int64_t		dts_base;
	int64_t		dts_last;
};


/* this structure is used to shipping 64 bit argments to 
 * notification functions */
struct	ezntf	{
	void	*varg1;
	void	*varg2;
	void	*varg3;
	int64_t	iarg1;
	int64_t	iarg2;
	int64_t	iarg3;
};


typedef	int  (*F_BRK)(void*, void*);
typedef	void (*F_HOOK)(F_BRK, void*, void*);

extern	const	char	*description;
extern	const	char	*credits;


/* ezthumb.c */
void ezopt_init(EZOPT *ezopt, char *profile);
int ezopt_load_config(EZOPT *ezopt, void *config);
void ezopt_review(EZOPT *opt);
int ezthumb(char *filename, EZOPT *ezopt);
int ezthumb_bind(char **filename, int fnum, EZOPT *ezopt);
int ezinfo(char *filename, EZOPT *ezopt, EZVID *vout);
int ezthumb_break(EZOPT *ezopt);
int eznotify(EZOPT *ezopt, int event, long param, long opt, void *block);
int ezopt_store_config(EZOPT *ezopt, void *config);

/* ezutil.c */
int ezopt_profile_setup(EZOPT *opt, char *s);
int ezopt_profile_dump(EZOPT *opt, char *pmt_grid, char *pmt_size);
int ezopt_profile_export(EZOPT *ezopt, char *buf, int blen);
char *ezopt_profile_export_alloc(EZOPT *ezopt);
int ezopt_profile_enable(EZOPT *ezopt, int prof);
int ezopt_profile_disable(EZOPT *ezopt, int prof);
int ezopt_profile_stat(EZOPT *ezopt);
int ezopt_profile_sampling(EZOPT *ezopt, int vidsec, int *col, int *row);
int ezopt_profile_sampled(EZOPT *ezopt, int vw, int bs, int *col, int *row);
int ezopt_profile_zooming(EZOPT *ezopt, int vw, int *wid, int *hei, int *ra);
char *meta_filesize(int64_t size, char *buffer);
char *meta_timestamp(EZTIME ms, int enms, char *buffer);
int meta_image_format(char *input);
char *meta_image_abbre(int fmt);
int meta_make_color(char *s, EZBYTE *color);
int meta_export_color(EZBYTE *color, char *buf, int blen);
char *meta_make_fontdir(char *s);

#ifdef	CFG_WIN32RT
/* ezwinfont.c */
int ezwinfont_open(void);
int ezwinfont_close(void);
char *ezwinfont_faceoff(char *fontface);
int ezwinfont_testing(char *fontface);
#endif

#endif	/* _EZTHUMB_H_ */


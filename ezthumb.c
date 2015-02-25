
/*  ezthumb.c - the core functions

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
#include <inttypes.h>
#include <sys/time.h>

//#define CSOUP_DEBUG_LOCAL     SLOG_CWORD(EZTHUMB_MOD_CORE, SLOG_LVL_WARNING)
//#define CSOUP_DEBUG_LOCAL     SLOG_CWORD(EZTHUMB_MOD_CORE, SLOG_LVL_PROGRAM)

#include "ezconfig.h"
#include "ezthumb.h"
#include "id_lookup.h"

#include "gdfontt.h"
#include "gdfonts.h"
#include "gdfontmb.h"
#include "gdfontl.h"
#include "gdfontg.h"


static int video_snapping(EZVID *vidx, EZIMG *image);
static int video_snapshot_keyframes(EZVID *vidx, EZIMG *image);
static int video_snapshot_skim(EZVID *vidx, EZIMG *image);
static int video_snapshot_safemode(EZVID *vidx, EZIMG *image);
static int video_snapshot_scan(EZVID *vidx, EZIMG *image);
static int video_snapshot_twopass(EZVID *vidx, EZIMG *image);

static EZVID *video_allocate(EZOPT *ezopt, char *filename, int *errcode);
static EZVID *video_alloc_queue(EZOPT *ezopt, char **fname, int fnum);
static int video_free(EZVID *vidx);
static int video_open(EZVID *vidx);
static int video_close(EZVID *vidx);
static int video_connect(EZVID *vidx, EZIMG *image);
static int video_disconnect(EZVID *vidx);
static int video_find_main_stream(EZVID *vidx);
static int64_t video_keyframe_next(EZVID *vidx, AVPacket *packet);
static int64_t video_keyframe_to(EZVID *vidx, AVPacket *packet, int64_t pos);
static int video_keyframe_credit(EZVID *vidx, int64_t dts);
static int video_dts_ruler(EZVID *vidx, int64_t cdts, int64_t ndts);
static int64_t video_load_packet(EZVID *vidx, AVPacket *packet);
static int video_media_on_canvas(EZVID *vidx, EZIMG *image);
static EZTIME video_duration(EZVID *vidx);
static EZTIME video_duration_quickscan(EZVID *vidx);
static EZTIME video_duration_fullscan(EZVID *vidx);
static int video_seek_challenge(EZVID *vidx);
static int64_t video_statistics(EZVID *vidx);
static int64_t video_snap_point(EZVID *vidx, EZIMG *image, int index);
static int video_snap_begin(EZVID *vidx, EZIMG *image, int method);
static int video_snap_update(EZVID *vidx, EZIMG *image, int64_t dts);
static int video_snap_end(EZVID *vidx, EZIMG *image);
static EZFRM *video_frame_alloc(int pixfmt, int width, int height);
static int video_frame_free(EZFRM *ezfrm);
static int video_frame_reset(EZVID *vidx);
static int video_frame_update(EZVID *vidx);
static EZFRM *video_frame_best(EZVID *vidx, int64_t refdts);
static int64_t video_decode_next(EZVID *vidx, AVPacket *);
static int64_t video_decode_to(EZVID *vidx, AVPacket *packet, int64_t dtsto);
static int64_t video_decode_valided(EZVID *vidx, AVPacket *packet, int64_t);
static int64_t video_decode_load(EZVID *vidx, AVPacket *packet, int64_t dtsto);
static int64_t video_decode_safe(EZVID *vidx, AVPacket *packet, int64_t dtsto);
static int video_seeking(EZVID *vidx, int64_t dts);
static int video_display_ar(AVStream *stream, AVRational *dar);
static char *video_media_video(AVStream *stream, char *buffer);
static char *video_media_audio(AVStream *stream, char *buffer);
static char *video_media_subtitle(AVStream *stream, char *buffer);
static char *video_stream_language(AVStream *stream);
static int64_t video_packet_timestamp(AVPacket *packet);
static int video_timing(EZVID *vidx, int type);
static EZTIME video_dts_to_ms(EZVID *vidx, int64_t dts);
static int64_t video_ms_to_dts(EZVID *vidx, EZTIME ms);
//static int64_t video_dts_to_system(EZVID *vidx, int64_t dts);
static int64_t video_system_to_dts(EZVID *vidx, int64_t sysdts);

static EZIMG *image_allocate(EZVID *vidx, EZTIME rt_during, int *errcode);
static int image_free(EZIMG *image);
static int image_user_profile(EZIMG *image, int src_width, int *col, int *row,
		int *width, int *height, int *facto);
static int image_font_test(EZIMG *image, char *filename);
static int image_gdframe_update(EZIMG *image, AVFrame *frame);
static int image_gdframe_timestamp(EZIMG *image, char *timestamp);
static int image_gdframe_save(EZIMG *image, char *filename, int idx);
static int image_gdframe_strlen(EZIMG *image, int fsize, char *s);
static int image_gdframe_puts(EZIMG *image, int fsize, int x, int y, 
		int c, char *s);
static int image_gdcanvas_save(EZIMG *image, char *filename);
static int image_gdcanvas_update(EZIMG *image, int idx);
static int image_gdcanvas_print(EZIMG *image, int row, int off, char *s);
static int image_gdcanvas_strlen(EZIMG *image, int fsize, char *s);
static int image_gdcanvas_puts(EZIMG *image, int fsize, int x, int y, 
		int c, char *s);
static int image_gdcanvas_background(EZIMG *image);
#ifdef	HAVE_GD_IMAGE_GIFANIMATION
static FILE *image_gif_anim_open(EZIMG *image, char *filename);
static int image_gif_anim_add(EZIMG *image, FILE *fout, int interval);
static int image_gif_anim_close(EZIMG *image, FILE *fout);
#endif
static FILE *image_create_file(EZIMG *image, char *filename, int idx);
static int image_cal_ratio(int ratio, int refsize);
static EZTIME image_cal_time_range(int ratio, EZTIME reftime);
static int image_cal_shots(EZTIME duration, EZTIME tmstep, int mode);
static EZTIME image_cal_timestep(EZTIME duration, int shots, int mode);
static int image_cal_gif_animix(EZOPT *ezopt);
static gdFont *image_fontset(int fsize);
static int image_fontsize(int fsize, int refsize);
static int image_copy(gdImage *dst, gdImage *src, int x, int, int, int);

static int ezopt_thumb_name(EZOPT *ezopt, char *buf, char *fname, int idx);
static char *ezopt_name_build(char *path, char *fname, char *buf, char *sfx);

static int ezdefault(EZOPT *ezopt, int event, long param, long opt, void *);
static int dump_media_brief(EZVID *vidx);
static int dump_media_statistic(struct MeStat *mestat, int n, EZVID *vidx);
static int dump_format_context(AVFormatContext *format);
static int dump_stream_common(AVStream *stream, int sidx);
static int dump_video_context(AVCodecContext *codec);
static int dump_audio_context(AVCodecContext *codec);
static int dump_subtitle_context(AVCodecContext *codec);
static int dump_other_context(AVCodecContext *codec);
static int dump_packet(AVPacket *p);
static int dump_frame(EZFRM *ezfrm, int got_pic);
static int dump_frame_packet(EZVID *vidx, int sn, EZFRM *ezfrm);
static int dump_metadata(void *dict);
static int dump_duration(EZVID *vidx, int use_ms);
static int dump_ezthumb(EZOPT *ezopt, EZIMG *image);


void ezopt_init(EZOPT *ezopt, char *profile)
{
	memset(ezopt, 0, sizeof(EZOPT));
	ezopt->grid_col = 4;	/* the default thumbnail is a 4x4 array */
	ezopt->grid_row = 4;
	ezopt->canvas_width = 0;

	/* enable media info area and inset timestamp, skip the first and the
	 * last frame, no shadows */
	ezopt->flags = EZOP_INFO | EZOP_TIMEST | EZOP_DECODE_OTF | 
			EZOP_THUMB_COPY | EZOP_DUR_AUTO;

	//ezopt->grid_gap_w = 4 | EZ_RATIO_OFF;
	//ezopt->grid_gap_h = 4 | EZ_RATIO_OFF;
	//ezopt->grid_rim_w = 6 | EZ_RATIO_OFF;
	//ezopt->grid_rim_h = 6 | EZ_RATIO_OFF;
	ezopt->grid_gap_w = 4;
	ezopt->grid_gap_h = 4;
	ezopt->grid_rim_w = 6;
	ezopt->grid_rim_h = 6;

	memcpy(ezopt->edge_color, "\0\0\0\x80", 4);	/* grey */
	memcpy(ezopt->shadow_color, "\x80\x80\x80\x3f", 4);	/* black */
	memcpy(ezopt->canvas_color, "\xff\xff\xff\x80", 4);	/* white */
	//memcpy(ezopt->canvas_color, "\0\0\0\x80", 4);	/* black */

	ezopt->edge_width   = 0;
	//ezopt->shadow_width = EZ_SHOT_SHADOW_OFF;
	ezopt->shadow_width = 0;

	ezopt->tn_facto = 50;

	memcpy(ezopt->mi_color, "\0\0\0\x80", 4); 	/* black */
	//memcpy(ezopt->mi_color, "\xff\xff\xff\x80", 4); 	/* white */
	ezopt->mi_position = EZ_POS_LEFTTOP;
	ezopt->st_position = EZ_POS_RIGHTBOTTOM;
	memcpy(ezopt->ins_color, "\xff\xff\xff\x80", 4);  	/* white */
	memcpy(ezopt->its_color, "\0\0\0\x80", 4);   	/* black */
	ezopt->ins_position = EZ_POS_RIGHTTOP;
	ezopt->ins_shadow = EZ_TEXT_SHADOW_OFF;

	strcpy(ezopt->img_format, "jpg");
	ezopt->img_quality = 85;
	strcpy(ezopt->suffix, "_thumb");

	ezopt->bg_position = EZ_POS_MIDCENTER;
	ezopt->vs_user = -1;	/* default: first found video stream */
	ezopt->r_flags = SMM_PATH_DIR_FIFO;
	ezopt->accept  = csc_extname_filter_open(EZ_DEF_FILTER);

	if (profile) {
		ezopt_profile_setup(ezopt, profile);
	}
}

int ezopt_load_config(EZOPT *ezopt, void *config)
{
	char	*s;

	csc_cfg_read_int(config, NULL, CFG_KEY_GRID_COLUMN, 
			&ezopt->grid_col);
	csc_cfg_read_int(config, NULL, CFG_KEY_GRID_ROW, 
			&ezopt->grid_row);
	csc_cfg_read_int(config, NULL, CFG_KEY_CANVAS_WIDTH, 
			&ezopt->canvas_width);
	csc_cfg_read_int(config, NULL, CFG_KEY_TIME_STEP, 
			&ezopt->tm_step);
	csc_cfg_read_int(config, NULL, CFG_KEY_GRID_GAP_WID, 
			&ezopt->grid_gap_w);
	csc_cfg_read_int(config, NULL, CFG_KEY_GRID_GAP_HEI, 
			&ezopt->grid_gap_h);
	csc_cfg_read_int(config, NULL, CFG_KEY_CANVAS_RIM_WID, 
			&ezopt->grid_rim_w);
	csc_cfg_read_int(config, NULL, CFG_KEY_CANVAS_RIM_HEI, 
			&ezopt->grid_rim_h);

	s = csc_cfg_copy(config, NULL, CFG_KEY_COLOR_EDGE, 0);
	if (s != NULL) {
		meta_make_color(s, ezopt->edge_color);
		smm_free(s);
	}
	s = csc_cfg_copy(config, NULL, CFG_KEY_COLOR_SHADOW, 0);
	if (s != NULL) {
		meta_make_color(s, ezopt->shadow_color);
		smm_free(s);
	}
	s = csc_cfg_copy(config, NULL, CFG_KEY_COLOR_CANVAS, 0);
	if (s != NULL) {
		meta_make_color(s, ezopt->canvas_color);
		smm_free(s);
	}

	csc_cfg_read_int(config, NULL, CFG_KEY_EDGE_WIDTH, 
			&ezopt->edge_width);
	csc_cfg_read_int(config, NULL, CFG_KEY_SHADOW_WIDTH, 
			&ezopt->shadow_width);
	csc_cfg_read_int(config, NULL, CFG_KEY_ZOOM_WIDTH, 
			&ezopt->tn_width);
	csc_cfg_read_int(config, NULL, CFG_KEY_ZOOM_HEIGHT, 
			&ezopt->tn_height);
	csc_cfg_read_int(config, NULL, CFG_KEY_ZOOM_RATIO, 
			&ezopt->tn_facto);

	s = csc_cfg_copy(config, NULL, CFG_KEY_INFO_FONT, 0);
	if (s != NULL) {
		if (ezopt->mi_font) {
			smm_free(ezopt->mi_font);
		}
		ezopt->mi_font = ezopt->ins_font = meta_make_fontdir(s);
		smm_free(s);
	}
	csc_cfg_read_int(config, NULL, CFG_KEY_INFO_SIZE, 
			&ezopt->mi_size);
	
	s = csc_cfg_copy(config, NULL, CFG_KEY_INFO_COLOR, 0);
	if (s != NULL) {
		meta_make_color(s, ezopt->mi_color);
		smm_free(s);
	}
	csc_cfg_read_int(config, NULL, CFG_KEY_INFO_SHADOW, 
			&ezopt->mi_shadow);
	s = csc_cfg_copy(config, NULL, CFG_KEY_INFO_LAYOUT, 0);
	if (s != NULL) {
		ezopt->mi_position = id_lookup_id(id_layout, s);
		smm_free(s);
	}
	s = csc_cfg_copy(config, NULL, CFG_KEY_INFO_STATUS, 0);
	if (s != NULL) {
		ezopt->st_position = id_lookup_id(id_layout, s);
		smm_free(s);
	}

	csc_cfg_read_int(config, NULL, CFG_KEY_INSET_SIZE, 
			&ezopt->ins_size);
	s = csc_cfg_copy(config, NULL, CFG_KEY_INSET_COLOR, 0);
	if (s != NULL) {
		meta_make_color(s, ezopt->ins_color);
		smm_free(s);
	}
	csc_cfg_read_int(config, NULL, CFG_KEY_INSET_SHADOW, 
			&ezopt->ins_shadow);
	s = csc_cfg_copy(config, NULL, CFG_KEY_INSET_LAYOUT, 0);
	if (s != NULL) {
		ezopt->ins_position = id_lookup_id(id_layout, s);
		smm_free(s);
	}

	s = csc_cfg_copy(config, NULL, CFG_KEY_FILE_FORMAT, 0);
	if (s != NULL) {
		ezopt->img_quality = meta_image_format(s, ezopt->img_format, 8);
		smm_free(s);
	}
	s = csc_cfg_copy(config, NULL, CFG_KEY_TRANSPARENCY, 0);
	if (s != NULL) {
		if (!strcasecmp(s, "yes")) {
			ezopt->flags |= EZOP_TRANSPARENT;
			ezopt->canvas_color[3] = 0;
		} else {
			ezopt->flags &= ~EZOP_TRANSPARENT;
		}
		smm_free(s);
	}
	s = csc_cfg_copy(config, NULL, CFG_KEY_FILE_SUFFIX, 0);
	if (s != NULL) {
		csc_strlcpy(ezopt->suffix, s, 64);
		smm_free(s);
	}
	s = csc_cfg_copy(config, NULL, CFG_KEY_BG_PICTURE, 0);
	if (s != NULL) {
		if (ezopt->background) {
			smm_free(ezopt->background);
		}
		ezopt->background = csc_strcpy_alloc(s, 0);
		smm_free(s);
	}
	s = csc_cfg_copy(config, NULL, CFG_KEY_BG_LAYOUT, 0);
	if (s != NULL) {
		ezopt->bg_position |= id_lookup_id(id_layout, s);
		smm_free(s);
	}
	s = csc_cfg_copy(config, NULL, CFG_KEY_BG_QUALITY, 0);
	if (s != NULL) {
		ezopt->bg_position |= id_lookup_id(id_layout, s);
		smm_free(s);
	}
	s = csc_cfg_copy(config, NULL, CFG_KEY_SUFFIX_FILTER, 0);
	if (s != NULL) {
		if (ezopt->accept) {
			csc_extname_filter_close(ezopt->accept);
		}
		ezopt->accept = csc_extname_filter_open(s);
		smm_free(s);
	}
	s = csc_cfg_copy(config, NULL, CFG_KEY_DURATION, 0);
	if (s != NULL) {
		SETDURMOD(ezopt->flags, id_lookup_id(id_duration, s));
		smm_free(s);
	}
	s = csc_cfg_copy(config, NULL, CFG_KEY_PROF_SIMPLE, 0);
	if (s != NULL) {
		ezopt_profile_setup(ezopt, s);
		smm_free(s);
	}
	return 0;
}

void ezopt_review(EZOPT *ezopt)
{
	/* foolproof the right transparent setting */
	if (!strcmp(ezopt->img_format, "jpg")) {
		ezopt->flags &= ~EZOP_TRANSPARENT;
	}

	/* synchronize the full scan and twopass option */
	/* 20120723 this could be confusing. When using '-p 2pass' option, 
	 * the dur_mode was changed automatically so the duration finding 
	 * mode was also changed in the configure file. The better way should
	 * be changing the session mode, not the default one.
	 * So move this functin to video_allocate() */
	/*if ((ezopt->flags & EZOP_PROC_MASK) == EZOP_PROC_TWOPASS) {
		ezopt->dur_mode = EZ_DUR_FULLSCAN;
	} else if ((ezopt->flags & EZOP_PROC_MASK) != EZOP_PROC_KEYRIP) {
		if (ezopt->dur_mode == EZ_DUR_FULLSCAN) {
			ezopt->flags &= ~EZOP_PROC_MASK;
			ezopt->flags |= EZOP_PROC_TWOPASS;
		}
	}*/
}


int ezthumb(char *filename, EZOPT *ezopt)
{
	EZIMG	*image;
	EZVID	*vidx;
	int	rc;

	if (ezopt_thumb_name(ezopt, NULL, filename, -1) == EZ_THUMB_SKIP) {
		eznotify(NULL, EN_SKIP_EXIST, 0, 0, filename);
		return EZ_ERR_EOP;
	}
	if ((vidx = video_allocate(ezopt, filename, &rc)) == NULL) {
		return rc;
	}
	if ((image = image_allocate(vidx, vidx->duration, &rc)) == NULL) {
		video_free(vidx);
		return rc;
	}

	video_open(vidx);
	if ((rc = video_connect(vidx, image)) != EZ_ERR_NONE) {
		image_free(image);
		video_free(vidx);
		return rc;
	}

	/* Register the video and image object so unix signal can intervene */
	ezopt->vidobj = vidx;
	ezopt->imgobj = image;

	rc = video_snapping(vidx, image);

	/* deregister the video and image object so they can be reused */
	ezopt->vidobj = NULL;
	ezopt->imgobj = NULL;

	image_free(image);
	video_free(vidx);
	
	/* 20120724 a supplemental method to cope with DVB rips.
	 * It had been observed in some DVB rips, maybe H.264 stream as well,
	 * there are dodge i-frames which marked as i-frame in the packet. 
	 * However after decoding by ffmpeg, they turned to be P/B frames,
	 * which confused ezthumb. The reason is unknown yet. The safe mode
	 * can handle it without trouble because the safe mode decodes from
	 * the first i-frame packets.
	 *
	 * After weighed by balance, I think it may be a good idea to 
	 * restart the ezthumb in the safe mode if it found no thumbnail
	 * was generated in the required process, better than nothing */
	/*if (rc == 0) {
		ezthumb_safe(filename, ezopt);
	}*/
	return EZ_ERR_NONE;
}

int ezthumb_bind(char **fname, int fnum, EZOPT *ezopt)
{
	EZVID	*vanchor, *vidx;
	EZIMG	*image;
	int	rc, total = 0;

	if ((vanchor = video_alloc_queue(ezopt, fname, fnum)) == NULL) {
		return EZ_ERR_FILE;
	}

	if (ezopt_thumb_name(ezopt, NULL, vanchor->filename, -1) == 
			EZ_THUMB_SKIP) {
		eznotify(NULL, EN_SKIP_EXIST, 0, 0, vanchor->filename);
		video_free(vanchor);
		return EZ_ERR_EOP;
	}

	if ((image = image_allocate(vanchor, vanchor->dur_all, &rc)) == NULL) {
		video_free(vanchor);
		return rc;
	}

	ezopt->imgobj = image;

	for (vidx = vanchor; vidx; vidx = vidx->next) {
		video_open(vidx);
		if (video_connect(vidx, image) != EZ_ERR_NONE) {
			video_close(vidx);
			continue;
		}

		ezopt->vidobj = vidx;
		rc = video_snapping(vidx, image);
		ezopt->vidobj = NULL;

		if (rc > 0) {
			total += rc;
		}

		video_disconnect(vidx);
		video_close(vidx);
	}

	ezopt->imgobj = NULL;

	image_free(image);
	video_free(vanchor);
	return total;
}

int ezinfo(char *filename, EZOPT *ezopt, EZVID *vout)
{
	EZVID	*vidx;
	EZIMG	*image;
	int	rc;

	if ((vidx = video_allocate(ezopt, filename, &rc)) == NULL) {
		return rc;
	}
	if (vout) {
		memcpy(vout, vidx, sizeof(EZVID));
	}

	if (EZOP_DEBUG(ezopt->flags) >= SLOG_LVL_INFO) {
		image = image_allocate(vidx, vidx->duration, &rc);
		if (image != NULL) {
			//dump_ezimage(image);
			if (ezopt->flags & EZOP_FONT_TEST) {
				image_font_test(image, vidx->filename);
			}
			image_free(image);
		}
	}

	video_free(vidx);
	return rc;
}

int ezthumb_break(EZOPT *ezopt)
{
	EZVID	*vidx = ezopt->vidobj;
	EZIMG	*image = ezopt->imgobj;

	if (vidx && image) {
		video_snap_end(vidx, image);
	}
	if (image) {
		image_free(image);
	}
	if (vidx) {
		video_free(vidx);
	}
	return EZ_ERR_NONE;
}

int eznotify(EZOPT *ezopt, int event, long param, long opt, void *block)
{
	int	rc;

	if ((ezopt == NULL) || (ezopt->notify == NULL)) {
		return ezdefault(ezopt, event, param, opt, block);
	}

	rc = ezopt->notify(ezopt, event, param, opt, block);
	if (rc == EN_EVENT_PASSTHROUGH) {
		return ezdefault(ezopt, event, param, opt, block);
	}
	return rc;
}

int ezopt_store_config(EZOPT *ezopt, void *config)
{
	char	buf[64], *s;

	csc_cfg_write_int(config, NULL, CFG_KEY_GRID_COLUMN, 
			ezopt->grid_col);
	csc_cfg_write_int(config, NULL, CFG_KEY_GRID_ROW, 
			ezopt->grid_row);
	csc_cfg_write_int(config, NULL, CFG_KEY_CANVAS_WIDTH, 
			ezopt->canvas_width);
	csc_cfg_write_int(config, NULL, CFG_KEY_TIME_STEP, 
			ezopt->tm_step);
	csc_cfg_write_int(config, NULL, CFG_KEY_GRID_GAP_WID, 
			ezopt->grid_gap_w);
	csc_cfg_write_int(config, NULL, CFG_KEY_GRID_GAP_HEI, 
			ezopt->grid_gap_h);
	csc_cfg_write_int(config, NULL, CFG_KEY_CANVAS_RIM_WID, 
			ezopt->grid_rim_w);
	csc_cfg_write_int(config, NULL, CFG_KEY_CANVAS_RIM_HEI, 
			ezopt->grid_rim_h);

	meta_export_color(ezopt->edge_color, buf, sizeof(buf));
	csc_cfg_write(config, NULL, CFG_KEY_COLOR_EDGE, buf);
	meta_export_color(ezopt->shadow_color, buf, sizeof(buf));
	csc_cfg_write(config, NULL, CFG_KEY_COLOR_SHADOW, buf);
	meta_export_color(ezopt->canvas_color, buf, sizeof(buf));
	csc_cfg_write(config, NULL, CFG_KEY_COLOR_CANVAS, buf);

	csc_cfg_write_int(config, NULL, CFG_KEY_EDGE_WIDTH, 
			ezopt->edge_width);
	csc_cfg_write_int(config, NULL, CFG_KEY_SHADOW_WIDTH, 
			ezopt->shadow_width);
	csc_cfg_write_int(config, NULL, CFG_KEY_ZOOM_WIDTH, 
			ezopt->tn_width);
	csc_cfg_write_int(config, NULL, CFG_KEY_ZOOM_HEIGHT, 
			ezopt->tn_height);
	csc_cfg_write_int(config, NULL, CFG_KEY_ZOOM_RATIO, 
			ezopt->tn_facto);

	if (ezopt->mi_font) {
		csc_cfg_write(config, NULL, 
				CFG_KEY_INFO_FONT, ezopt->mi_font);
	}
	csc_cfg_write_int(config, NULL, CFG_KEY_INFO_SIZE, 
			ezopt->mi_size);
	meta_export_color(ezopt->mi_color, buf, sizeof(buf));
	csc_cfg_write(config, NULL, CFG_KEY_INFO_COLOR, buf);
	csc_cfg_write_int(config, NULL, CFG_KEY_INFO_SHADOW, 
			ezopt->mi_shadow);

	csc_cfg_write(config, NULL, CFG_KEY_INFO_LAYOUT, 
		id_lookup(id_layout, ezopt->mi_position & EZ_POS_MASK));
	csc_cfg_write(config, NULL, CFG_KEY_INFO_STATUS,
		id_lookup(id_layout, ezopt->st_position & EZ_POS_MASK));

	csc_cfg_write_int(config, NULL, CFG_KEY_INSET_SIZE, 
			ezopt->ins_size);
	meta_export_color(ezopt->ins_color, buf, sizeof(buf));
	csc_cfg_write(config, NULL, CFG_KEY_INSET_COLOR, buf);
	csc_cfg_write_int(config, NULL, CFG_KEY_INSET_SHADOW, 
			ezopt->ins_shadow);
	csc_cfg_write(config, NULL, CFG_KEY_INSET_LAYOUT,
		id_lookup(id_layout, ezopt->ins_position & EZ_POS_MASK));

	if (ezopt->img_quality) {
		sprintf(buf, "%s@%d", ezopt->img_format, ezopt->img_quality);
	} else {
		strcpy(buf, ezopt->img_format);
	}
	csc_cfg_write(config, NULL, CFG_KEY_FILE_FORMAT, buf);

	if (ezopt->flags & EZOP_TRANSPARENT) {
		csc_cfg_write(config, NULL, CFG_KEY_TRANSPARENCY, "Yes");
	}
	if (ezopt->suffix[0]) {
		csc_cfg_write(config, NULL, 
				CFG_KEY_FILE_SUFFIX, ezopt->suffix);
	}
	if (ezopt->background) {
		csc_cfg_write(config, NULL, 
				CFG_KEY_BG_PICTURE, ezopt->background);
	}
	csc_cfg_write(config, NULL, CFG_KEY_BG_LAYOUT,
		id_lookup(id_layout, ezopt->bg_position & EZ_POS_MASK));
	csc_cfg_write(config, NULL, CFG_KEY_BG_QUALITY,
		id_lookup(id_layout, ezopt->bg_position & ~EZ_POS_MASK));
	
	if (ezopt->accept) {
		s = csc_extname_filter_export_alloc(ezopt->accept);
		if (s != NULL) {
			csc_cfg_write(config, NULL, 
					CFG_KEY_SUFFIX_FILTER, s);
			smm_free(s);
		}
	}
	csc_cfg_write(config, NULL, CFG_KEY_DURATION,
			id_lookup(id_duration, GETDURMOD(ezopt->flags)));

	if ((s = ezopt_profile_export_alloc(ezopt)) != NULL) {
		csc_cfg_write(config, NULL, CFG_KEY_PROF_SIMPLE, s);
		smm_free(s);
	}
	return 0;
}

/****************************************************************************
 * Internal Functions
 ***************************************************************************/
static int video_snapping(EZVID *vidx, EZIMG *image)
{
	int	rc = 0;

	/* if the expected time_step is 0, then it will save every 
	 * key frames separately. it's good for debug purpose  */
	if (image->time_step > 0) {	
		vidx->keydelta = video_ms_to_dts(vidx, image->time_step);
	} else {
		EZOP_PROC_MAKE(vidx->ses_flags, EZOP_PROC_KEYRIP);
		vidx->keydelta = 0;
	}
	video_keyframe_credit(vidx, -1);

	switch (EZOP_PROC(vidx->ses_flags)) {
	case EZOP_PROC_SKIM:
		rc = video_snapshot_skim(vidx, image);
		break;
	case EZOP_PROC_SCAN:
		rc = video_snapshot_scan(vidx, image);
		break;
	case EZOP_PROC_SAFE:
		rc = video_snapshot_safemode(vidx, image);
		break;
	case EZOP_PROC_TWOPASS:
		rc = video_snapshot_twopass(vidx, image);
		break;
	case EZOP_PROC_KEYRIP:
		rc = video_snapshot_keyframes(vidx, image);
		break;
	default:
		if (SEEKABLE(vidx->seekable)) {
			rc = video_snapshot_skim(vidx, image);
		} else {
			rc = video_snapshot_scan(vidx, image);
		}
		break;
	}
	return rc;
}

/* This function is used to save every key frames in the video clip
 * into individual files. */
static int video_snapshot_keyframes(EZVID *vidx, EZIMG *image)
{
	AVPacket	packet;
	int64_t		dts, dtms;
	int		i;

	video_snap_begin(vidx, image, ENX_SS_IFRAMES);

	i = 0;
	video_keyframe_credit(vidx, -1);
	while ((dts = video_keyframe_next(vidx, &packet)) >= 0) {
		/* convert DTS to relative millisecond in all clips */
		dtms = dts - vidx->dts_offset;
		dtms = video_dts_to_ms(vidx, dtms > 0 ? dtms : 0);
		if (vidx->dur_all) {	/* binding mode */
			dtms += vidx->dur_off;
		}

		if (dtms < image->time_from) {
			if (vidx->ses_flags & EZOP_DECODE_OTF) {
				video_decode_next(vidx, &packet);
			} else {
				av_free_packet(&packet);
			}
			continue;
		}
		if (dtms > image->time_from + image->time_during) {
			av_free_packet(&packet);
			break;
		}

		/* use video_decode_next() instead of video_decode_keyframe()
		 * because sometimes it's good for debugging doggy clips */
		if (video_decode_next(vidx, &packet) < 0) {
			break;
		}

		video_snap_update(vidx, image, dts);
		i++;

		if (vidx->sysopt->key_ripno && 
				(vidx->sysopt->key_ripno <= i)) {
			break;
		}
	}
	video_snap_end(vidx, image);
	return i;	/* return the number of thumbnails */
}


#define VSkLOG(s,a,b)	\
	EDB_PROG(("video_snapshot_skim: %s %lld/%lld\n", (s),(a),(b)))

/* for these conditions: backward seeking available, key frame only,
 * snap interval is larger than maximum key frame interval and no rewind
 * clips */
static int video_snapshot_skim(EZVID *vidx, EZIMG *image)
{
	AVPacket	packet;
	int64_t		dts, dts_snap, last_key;
	int		scnt = 0, disable_seeking = 0;

	video_snap_begin(vidx, image, ENX_SS_SKIM);
	/* setup the first key frame as a reference start point */
	last_key = dts = vidx->dts_offset;
	while (image->taken < image->shots) {
		dts_snap = video_snap_point(vidx, image, image->taken);
		if (dts_snap < 0) {
			break;	/* out of current video range */
		}

		if (video_dts_ruler(vidx, dts, dts_snap) < 2) {
			/* distance between current position to the snap point
			 * is too close to seek, ezthumb will decode to
			 * the snap point directly */
			VSkLOG("[SD]", dts, dts_snap);
			dts = video_decode_load(vidx, &packet, dts_snap);
			goto vs_skim_update;
		}

		/* 20130726 recently found the ffmpeg could not seek to the 
		 * key frame prior to the reference position but after. this 
		 * may be caused by version update because I remember some 
		 * versions ago it could. anyway it will impact the accurate 
		 * mode because ezthumb has done everything to avoid seeking 
		 * backward. the workaround is to seek 2 possible keyframe 
		 * ahead then decode to the target position. 
		 * slower but functional */
		dts = dts_snap;
		if (GETACCUR(vidx->ses_flags)) {
			if ((dts -= vidx->keygap * 2) < 0) {
				dts = 0;
			}
		}
		if (disable_seeking) {
			dts = video_keyframe_to(vidx, &packet, dts);
		} else {
			video_seeking(vidx, dts);
			dts = video_keyframe_next(vidx, &packet);
		}

		if (dts < 0) {
			/* do nothing, let it break */
		} else  if (dts > dts_snap) {
			/* overread the packets. Skim mode doesn't seek back
			 * so ezthumb just decode the nearest one */
			VSkLOG("[OR]", dts, dts_snap);
			dts = video_decode_valided(vidx, &packet, dts_snap);
		} else if (video_dts_ruler(vidx, dts, dts_snap) == INT_MAX) {
			/* probably keyframe accredit system is not ready */
			VSkLOG("[TR]", dts, dts_snap);
			dts = video_decode_safe(vidx, &packet, dts_snap);
		} else if ((dts <= last_key) ||
				(video_dts_ruler(vidx, dts, dts_snap) > 10)) {
			/* sought backward or stopped or big error in 
			 * seeking, ezthumb will change to safe mode */
			VSkLOG("[CS]", dts, dts_snap);
			dts = video_decode_safe(vidx, &packet, dts_snap);
			disable_seeking = 1;	/* no seeking any more */
		} else if (GETACCUR(vidx->ses_flags)) {
			VSkLOG("[AR]", dts, dts_snap);
			dts = video_decode_to(vidx, &packet, dts_snap);
		} else {
			VSkLOG("[IF]", dts, dts_snap);
			dts = video_decode_safe(vidx, &packet, dts_snap);
		}

vs_skim_update:
		/* 20130808 ezthumb should not break from here if decoding 
		 * video failed. Otherwise it'll cause problem to the binding
		 * mode. Binding mode will take previous clip's index to the
		 * next clip. The workaround is updating the index counter */
		if (dts < 0) {
			image->taken++;
		} else {
			last_key = dts;
			video_snap_update(vidx, image, dts_snap);
			scnt++;
		}
	}
	video_snap_end(vidx, image);
	return scnt;	/* return the number of thumbnails */
}


#define VSSLOG(s,a,b)	\
	EDB_PROG(("video_snapshot_scan: %s %lld/%lld\n", (s),(a),(b)))

/* for these conditions: Though backward seeking is NOT available but it is 
 * required to extract key frame only. snap interval is larger than maximum 
 * key frame interval and no rewind clips */
static int video_snapshot_scan(EZVID *vidx, EZIMG *image)
{
	AVPacket	packet;
	int64_t		dts, dts_snap;
	int		scnt = 0;

	video_snap_begin(vidx, image, ENX_SS_SCAN);
	dts = vidx->dts_offset; 	/* setup the reference start point */
	while (image->taken < image->shots) {
		dts_snap = video_snap_point(vidx, image, image->taken);
		if (dts_snap < 0) {
			break;
		}

		if (video_dts_ruler(vidx, dts, dts_snap) < 2) {
			/* the distance between current position to the snap 
			 * point is quite small so ezthumb will decode to the
			 * snap point directly */
			VSSLOG("[SD]", dts, dts_snap);
			dts = video_decode_load(vidx, &packet, dts_snap);
			goto vs_scan_update;
		}

		dts = dts_snap;
		if (GETACCUR(vidx->ses_flags)) {
			if ((dts -= vidx->keygap) < 0) {
				dts = 0;
			}
		}
		dts = video_keyframe_to(vidx, &packet, dts);

		if (dts < 0) {
			/* do nothing, let it break */
		} else if (dts >= dts_snap) {
			/* it's already overread, decode next at once */
			/* Argus_20120222-114427384.ts case:
			 * the HD DVB rip file has some dodge i-frame, 
			 * after decoding, they turned to P/B frames.
			 * This issue may also happen on H.264 streams.
			 * The workaround is the option to decode the
			 * next frame instead of searching an i-frame. */
			VSSLOG("[OR]", dts, dts_snap);
			//dts = video_decode_keyframe(vidx, &packet);
			//dts = video_decode_next(vidx, &packet);
			dts = video_decode_valided(vidx, &packet, dts_snap);
		} else if (GETACCUR(vidx->ses_flags)) {
			/* if accurate mode is set and the current DTS
			 * position is quite close to the snap point, 
			 * ezthumb will commence decoding to the target */
			VSSLOG("[AR]", dts, dts_snap);
			dts = video_decode_to(vidx, &packet, dts_snap);
		} else {
			VSSLOG("[IF]", dts, dts_snap);
			//dts = video_decode_next(vidx, &packet);
			dts = video_decode_valided(vidx, &packet, dts_snap);
		}

vs_scan_update:
		/* 20130808 ezthumb should not break from here if decoding 
		 * video failed. Otherwise it'll cause problem to the binding
		 * mode. Binding mode will take previous clip's index to the
		 * next clip. The workaround is updating the index counter */
		if (dts < 0) {
			image->taken++;
		} else {
			video_snap_update(vidx, image, dts_snap);
			scnt++;
		}
	}
	video_snap_end(vidx, image);
	return scnt;	/* return the number of thumbnails */
}


#define VSTLOG(s,a,b,c)	\
	EDB_PROG(("video_snapshot_twopass: %s %lld/%lld/%lld\n", \
				(s),(a),(b),(c)))

/* for these conditions: Though backward seeking is NOT available and it is 
 * required to extract p-frames. */
static int video_snapshot_twopass(EZVID *vidx, EZIMG *image)
{
	AVPacket	packet;
	int64_t		*refdts;
	int64_t		dts, dts_snap, lastkey;
	int		i, scnt = 0;

	/* find the first key frame */
	if ((lastkey = video_keyframe_next(vidx, &packet)) < 0) {
		return scnt;
	}
	/* allocate a buffer for storing the reference key frames */
	if ((refdts = smm_alloc(image->shots * sizeof(int64_t))) == NULL) {
		return EZ_ERR_LOWMEM;
	}

	video_snap_begin(vidx, image, ENX_SS_TWOPASS);

	/* the first pass to locate the key frames just ahead of the 
	 * snap points */
	dts = lastkey;
	for (i = image->taken; i < image->shots; i++) {
		if ((dts_snap = video_snap_point(vidx, image, i)) < 0) {
			break;
		}
		if (dts_snap < dts) {
			/* reuse the last key frame because there are more 
			 * than 1 snap point in the nearby key frames */
			refdts[i] = lastkey;
			VSTLOG("[KF]", refdts[i], dts_snap, dts);
			continue;
		}
		lastkey = dts;
		while (1) {
			if ((dts = video_keyframe_next(vidx, &packet)) < 0) {
				refdts[i] = lastkey;
				VSTLOG("[KF]", refdts[i], dts_snap, dts);
				break;
			}
			av_free_packet(&packet);
			if (dts > dts_snap) {
				refdts[i] = lastkey;
				VSTLOG("[KF]", refdts[i], dts_snap, dts);
				break;
			}
			lastkey = dts;
		}
	}

	/* rewind the video and begin the second round scan */
	video_seeking(vidx, 0);
	dts = -1;
	while (image->taken < image->shots) {
		dts_snap = video_snap_point(vidx, image, image->taken);
		if (dts_snap < 0) {
			break;
		}

		while (dts < refdts[image->taken]) {
			/* hasn't entered the range yet, go read more */
			if ((dts = video_keyframe_next(vidx, &packet)) < 0) {
				break;
			}
			if (dts >= refdts[image->taken]) {
				//dts = video_decode_next(vidx, &packet);
				dts = video_decode_valided(vidx, &packet, dts_snap);
				break;
			}
			/* discard the current packet */
			if (vidx->ses_flags & EZOP_DECODE_OTF) {
				video_decode_next(vidx, &packet);
				video_frame_update(vidx);
			} else {
				av_free_packet(&packet);
			}
		}

		if ((dts < 0) || (dts >= dts_snap)) {
			/* EOF or overreaded */
			VSTLOG("[OR]", refdts[image->taken], dts_snap, dts);
		} else if (GETACCUR(vidx->ses_flags)) {
			dts = video_decode_load(vidx, &packet, dts_snap);
			VSTLOG("[AR]", refdts[image->taken], dts_snap, dts);
		} else {
			/* reuse the key frame for non-accurate mode */
			VSTLOG("[IF]", refdts[image->taken], dts_snap, dts);
		}

		/* 20130808 ezthumb should not break from here if decoding 
		 * video failed. Otherwise it'll cause problem to the binding
		 * mode. Binding mode will take previous clip's index to the
		 * next clip. The workaround is updating the index counter */
		if (dts < 0) {
			image->taken++;
		} else {
			video_snap_update(vidx, image, dts_snap);
			scnt++;
		}
	}

	video_snap_end(vidx, image);
	smm_free(refdts);
	return scnt;	/* return the number of thumbnails */
}

static int video_snapshot_safemode(EZVID *vidx, EZIMG *image)
{
	AVPacket	packet;
	int64_t		dts, dts_snap;
	int		scnt = 0;

	if ((dts_snap = video_snap_point(vidx, image, image->taken)) < 0) {
		return 0;
	}
	video_snap_begin(vidx, image, ENX_SS_SAFE);
	while (image->taken < image->shots) {
		if ((dts = video_keyframe_next(vidx, &packet)) < 0) {
			break;
		}

		/* use video_decode_next() instead of video_decode_keyframe()
		 * because sometimes it's good for debugging doggy clips */
		//if (video_decode_next(vidx, &packet) < 0) {
		if (video_decode_valided(vidx, &packet, dts_snap) < 0) {
			break;
		}
		if (dts >= dts_snap) {
			video_snap_update(vidx, image, dts_snap);
			scnt++;

			dts_snap = video_snap_point(vidx, image, image->taken);
			if (dts_snap < 0) {
				break;
			}
		}
	}
	video_snap_end(vidx, image);
	return scnt;	/* return the number of thumbnails */
}

static EZVID *video_allocate(EZOPT *ezopt, char *filename, int *errcode)
{
	EZVID	*vidx;
	int	rc;

	/* check if the nominated file existed
	 * no need to print anything if file doesn't exist or is a folder */
	if (smm_fstat(filename) != SMM_FSTAT_REGULAR) {
		return NULL;
	}

	if ((vidx = smm_alloc(sizeof(EZVID))) == NULL) {
		uperror(errcode, EZ_ERR_LOWMEM);
		return NULL;
	}

	vidx->sysopt   = ezopt;
	vidx->filename = filename;
	vidx->seekable = ENX_SEEK_UNKNOWN;
	vidx->filesize = smm_filesize(filename);
	vidx->vsidx    = -1;	/* must be initialized before video_open() */
	vidx->keygap   = -1;	/* for tracking the single i-frame videos */

	smm_time_get_epoch(&vidx->tmark);	/* get current time stamp */
	video_timing(vidx, EZ_PTS_RESET);	/* clear progress timestamp*/
	eznotify(ezopt, EN_OPEN_BEGIN, 0, 0, vidx);

	/* On second thought, the FFMPEG log is better to be enabled while 
	 * loading codecs so we would've known if the video files buggy */
	if (EZOP_DEBUG(ezopt->flags) == SLOG_LVL_FUNC) {
		av_log_set_level(AV_LOG_VERBOSE);	/* enable all logs */
	} else if (EZOP_DEBUG(ezopt->flags) >= SLOG_LVL_INFO) {
		av_log_set_level(AV_LOG_INFO);
	} else {
		av_log_set_level(0);
	}

	/* 20120723 Moved from ezopt_review() */
	vidx->ses_flags = ezopt->flags;
	if (EZOP_PROC(vidx->ses_flags) == EZOP_PROC_TWOPASS) {
		SETDURMOD(vidx->ses_flags, EZOP_DUR_FSCAN);
	} 

	if ((rc = video_open(vidx)) != EZ_ERR_NONE) {
		uperror(errcode, rc);
		smm_free(vidx);
		eznotify(ezopt, EN_OPEN_END, rc, 0, NULL);
		return NULL;
	}

	/* setup the dts_offset field for future reference */
	if (vidx->formatx->start_time && 
			vidx->formatx->start_time != (int64_t)AV_NOPTS_VALUE) {
		vidx->dts_offset = 
			video_system_to_dts(vidx, vidx->formatx->start_time);
	}

	/* update the filesize field with the ffmpeg attribute.
	 * this is a foolproof procedure */
	/* the file_size field will be depreciated soon */
#if	0
#ifdef	HAVE_AVFORMATCONTEXT_FILE_SIZE
	if (vidx->filesize < vidx->formatx->file_size) {
		vidx->filesize = vidx->formatx->file_size;
	}
#elif	defined(HAVE_AVFORMATCONTEXT_PB)
	if (vidx->formatx->pb) {
		if (vidx->filesize < avio_size(vidx->formatx->pb)) {
			vidx->filesize = avio_size(vidx->formatx->pb);
		}
	}
#else
#error	NO file_size or pb field found in AVFormatContext!
#endif
#endif
	eznotify(ezopt, EN_FILE_OPEN, 0, 0, vidx);

	/* 20111213: It seems detecting media length could not block some 
	 * unwanted files. For example, in guidev branch, the ezthumb.o
	 * was treated as a 3 seconds long MP3 file. Thus I set another
	 * filter to check the media's resolution. */
	//if (!vidx->vstream->codec->width || !vidx->vstream->codec->height) {
	if (!vidx->codecx->width || !vidx->codecx->height) {
		uperror(errcode, EZ_ERR_FILE);
		eznotify(vidx->sysopt, EZ_ERR_VIDEOSTREAM, 1, 0, filename);
		video_free(vidx);
		eznotify(ezopt, EN_OPEN_END, EZ_ERR_VIDEOSTREAM, 0, NULL);
		return NULL;
	}

	/* collecting other information */
	vidx->width    = vidx->codecx->width;
	vidx->height   = vidx->codecx->height;
	vidx->streams  = vidx->formatx->nb_streams;

	/* 20120720 Apply the AR correction */
	vidx->ar_height = video_display_ar(vidx->vstream, NULL);

	/* find out the clip duration in millisecond */
	/* 20110301: the still images are acceptable by the ffmpeg library
	 * so it would be wiser to avoid the still image stream, which duration
	 * is only several milliseconds. FIXME if this assumption is wrong */
	if (ezopt->pre_dura == 0) {
		video_duration(vidx);
	} else {
		vidx->duration = ezopt->pre_dura;
		vidx->seekable = ezopt->pre_seek;
		vidx->bitrates = ezopt->pre_br;
	}
	if (vidx->duration < 500) {
		uperror(errcode, EZ_ERR_FILE);
		eznotify(vidx->sysopt, EZ_ERR_VIDEOSTREAM, 1, 0, filename);
		video_free(vidx);
		eznotify(ezopt, EN_OPEN_END, EZ_ERR_VIDEOSTREAM, 0, NULL);
		return NULL;
	}

	//dump_format_context(vidx->formatx);
	eznotify(ezopt, EN_OPEN_END, 0, 0, vidx);
	eznotify(vidx->sysopt, EN_MEDIA_OPEN, 0, 
			smm_time_diff(&vidx->tmark), vidx);
	uperror(errcode, EZ_ERR_NONE);

	video_close(vidx);	/* do not rewinding, reopen it instead */
	return vidx;
}

static EZVID *video_alloc_queue(EZOPT *ezopt, char **fname, int fnum)
{
	EZVID	*vanchor, *vidx, *vp;
	EZTIME	dur_all, dur_off;
	int64_t	bsize;
	int	i, rc, bnum;

	vanchor = NULL;
	dur_all = dur_off = 0;
	bsize = 0;
	for (i = bnum = 0; i < fnum; i++) {
		if ((vidx = video_allocate(ezopt, fname[i], &rc)) == NULL) {
			continue;
		}

		vidx->dur_off = dur_off;
		dur_off += vidx->duration;
		dur_all += vidx->duration;
		
		vidx->bind_idx = bnum++;
		bsize += vidx->filesize;

		if (vanchor == NULL) {
			vanchor = vidx;
		} else if (vanchor->next == NULL) {
			vanchor->next = vidx;
		} else {
			for (vp = vanchor; vp->next; vp = vp->next);
			vp->next = vidx;
		}
		vidx->anchor = vanchor;
	}
	/* update the binding information through out the queue */
	for (vp = vanchor; vp; vp = vp->next) {
		vp->dur_all = dur_all;		//FIXME: vp->dur_all is used to being binding mode flag
		vp->bind_size = bsize;
		vp->bound = bnum;
	}

#if 0
		/* adjust the FirstFrame and LastFrame decoding flag */
		if (vp == vanchor) {	/* the first clip */
			if (vp->next != NULL) {
				vp->ses_flags &= ~EZOP_LFRAME;
			}
		} else {
			vp->ses_flags &= ~EZOP_FFRAME;
			if (vp->next != NULL) {
				vp->ses_flags &= ~EZOP_LFRAME;
			}
		}
	}
#endif
	return vanchor;
}

static int video_free(EZVID *vidx)
{
	EZVID	*vp;

	while (vidx) {
		video_disconnect(vidx);	
		video_close(vidx);

		vp = vidx;
		vidx = vidx->next;
		smm_free(vp);
	}
	return EZ_ERR_NONE;
}


static int video_open(EZVID *vidx)
{
	AVCodec	*codec;
	char	*mblock[] = { "mp3", "image2" };
	int	i, den, num;

	video_timing(vidx, EZ_PTS_CLEAR);

	/* apparently the ubuntu 10.10 still use av_open_input_file() */
	/* FFMPEG/doc/APIchanes claim the avformat_open_input() was introduced
	 * since 53.2.0. Apparently it is wrong. It is at least appeared in
	 * my archlinux 64-bit box by 52.110.0 */
	/* 20120613: What a surprise that avformat_open_input() do support
	 * utf-8 in native MSWindows */
#ifdef	HAVE_AVFORMAT_OPEN_INPUT
	if (avformat_open_input(&vidx->formatx, vidx->filename, 
				NULL, NULL) != 0) {
#elif	defined(HAVE_AV_OPEN_INPUT_FILE)
	if (av_open_input_file(&vidx->formatx, vidx->filename, 
				NULL, 0, NULL) < 0) {
#else
#error	No avformat_open_input() or av_open_input_file() defined!
#endif
		eznotify(NULL, EZ_ERR_FORMAT, 0, 0, vidx->filename);
		return EZ_ERR_FORMAT;
	}
	video_timing(vidx, EZ_PTS_MOPEN);
	eznotify(vidx->sysopt, EN_OPEN_GOING, 0, 0, NULL);

	/* 20120814 Implemented a media type filter to block the unwanted
	 * media files like jpg, mp3, etc */
	for (i = 0; i < (int)(sizeof(mblock)/sizeof(char*)); i++) {
		if (!strcmp(vidx->formatx->iformat->name, mblock[i])) {
			eznotify(NULL, EZ_ERR_FORMAT, 0, 0, vidx->filename);
			video_close(vidx);
			return EZ_ERR_FORMAT;
		}
	}

	/* Generate missing pts even if it requires parsing future frames.*/
	vidx->formatx->flags |= AVFMT_FLAG_GENPTS;
	//vidx->formatx->flags |= AVFMT_FLAG_IGNIDX | AVFMT_TS_DISCONT ;

#ifdef	HAVE_AVFORMAT_FIND_STREAM_INFO
	if (avformat_find_stream_info(vidx->formatx, NULL) < 0) {
#elif	defined(HAVE_AV_FIND_STREAM_INFO)
	if (av_find_stream_info(vidx->formatx) < 0) {
#else
#error	No avformat_find_stream_info() or av_find_stream_info() defined
#endif
		eznotify(NULL, EZ_ERR_STREAM, 0, 0, vidx->filename);
		video_close(vidx);
		return EZ_ERR_STREAM;
	}

	/* If the vsidx is uninitialized (first time opening the video),
	 * ezthumb will go to find the video stream */
	if (vidx->vsidx < 0) {
		if ((vidx->vsidx = video_find_main_stream(vidx)) < 0) {
			eznotify(NULL, EZ_ERR_VIDEOSTREAM, 
					0, 0, vidx->filename);
			video_close(vidx);
			return EZ_ERR_VIDEOSTREAM;
		}
	}

	vidx->vstream = vidx->formatx->streams[vidx->vsidx];
	vidx->codecx  = vidx->vstream->codec;
	/* discard frames; AVDISCARD_NONKEY,AVDISCARD_BIDIR */
	vidx->codecx->skip_frame = AVDISCARD_NONREF | AVDISCARD_BIDIR;
	//vidx->codecx->hurry_up = 1; /* fast decoding mode */

	/* open the codec */
	vidx->vstream->discard = AVDISCARD_ALL;
	codec = avcodec_find_decoder(vidx->codecx->codec_id);
#ifdef	HAVE_AVCODEC_OPEN2
	if (avcodec_open2(vidx->codecx, codec, NULL) < 0) {
#elif	defined(HAVE_AVCODEC_OPEN)
	if (avcodec_open(vidx->codecx, codec) < 0) {
#else
#error	None of avcodec_open2() or avcodec_open() defined!
#endif
		eznotify(vidx->sysopt, EZ_ERR_CODEC_FAIL, 
				vidx->codecx->codec_id, 0, vidx->codecx);
		video_close(vidx);
		return EZ_ERR_CODEC_FAIL;
	}
	vidx->vstream->discard = AVDISCARD_DEFAULT;

	/* calculate the DTS per frame */
	num = vidx->vstream->r_frame_rate.num * vidx->vstream->time_base.num;
	den = vidx->vstream->r_frame_rate.den * vidx->vstream->time_base.den;
	if (den) {
		vidx->dts_rate = (num + den - 1) / den;
	}

	video_timing(vidx, EZ_PTS_COPEN);
	eznotify(vidx->sysopt, EN_OPEN_GOING, 0, 0, NULL);

	//dump_format_context(vidx->formatx);
	return EZ_ERR_NONE;
}

static int video_close(EZVID *vidx)
{
	vidx->vstream = NULL;
	if (vidx->codecx) {
		avcodec_close(vidx->codecx);
		vidx->codecx = NULL;
	}
	if (vidx->formatx) {
#ifdef	HAVE_AVFORMAT_OPEN_INPUT
		avformat_close_input(&vidx->formatx);
#elif	defined(HAVE_AV_OPEN_INPUT_FILE)
		av_close_input_file(vidx->formatx);
#else
#error	None of avformat_close_input() or av_close_input_file() defined!
#endif
		vidx->formatx = NULL;
	}
	return EZ_ERR_NONE;
}

static int video_connect(EZVID *vidx, EZIMG *image)
{
	if ((vidx->picframe = video_frame_alloc(vidx->codecx->pix_fmt, 
					vidx->width, vidx->height)) == NULL) {
		eznotify(vidx->sysopt, EZ_ERR_VIDEOSTREAM, 
				0, 0, vidx->filename);
		return EZ_ERR_LOWMEM;
	}

	if ((vidx->vidframe = video_frame_alloc(0, 0, 0)) == NULL) {
		video_frame_free(vidx->picframe);
		eznotify(vidx->sysopt, EZ_ERR_VIDEOSTREAM, 
				0, 0, vidx->filename);
		return EZ_ERR_LOWMEM;
	}

	/* allocate the swscale structure for scaling the screen image */
	/*vidx->swsctx = sws_getContext(vidx->codecx->width, 
			vidx->codecx->height, vidx->codecx->pix_fmt, 
			image->dst_width, image->dst_height,
			image->dst_pixfmt, SWS_BILINEAR, NULL, NULL, NULL);*/
	/*vidx->swsctx = sws_getContext(vidx->width, 
			vidx->height > vidx->ar_height ? 
					vidx->height : vidx->ar_height,
			vidx->codecx->pix_fmt, 
			image->dst_width, image->dst_height,
			image->dst_pixfmt, SWS_BILINEAR, NULL, NULL, NULL);*/
	/* allocate the frame structure for RGB converter which
	 * will be filled by frames converted from YUV form */
	vidx->swsframe = video_frame_alloc(image->dst_pixfmt,
			image->dst_width, image->dst_height);
	if (vidx->swsframe) {
		vidx->swsframe->context = sws_getContext(vidx->width, 
				vidx->height, vidx->codecx->pix_fmt,
				image->dst_width, image->dst_height, 
				image->dst_pixfmt, 
				SWS_BILINEAR, NULL, NULL, NULL);
		if (vidx->swsframe->context == NULL) {
			video_frame_free(vidx->swsframe);
			vidx->swsframe = NULL;
		}
	}
	if (vidx->swsframe == NULL) {
		video_disconnect(vidx);
		eznotify(vidx->sysopt, EZ_ERR_SWSCALE, 0, 0, vidx->filename);
		return EZ_ERR_SWSCALE;
	}
	return EZ_ERR_NONE;
}

static int video_disconnect(EZVID *vidx)
{
	if (vidx->swsframe) {
		if (vidx->swsframe->context) {
			sws_freeContext(vidx->swsframe->context);
		}
		video_frame_free(vidx->swsframe);
		vidx->swsframe = NULL;
	}
	video_frame_free(vidx->picframe);
	video_frame_free(vidx->vidframe);
	return EZ_ERR_NONE;
}


/* This function is used to find the video stream in the clip 
 * as well as open the related decoder driver */
static int video_find_main_stream(EZVID *vidx)
{
	AVStream	*stream;
	int		i, n;

#ifdef	HAVE_AV_FIND_BEST_STREAM
	int	wanted_stream[AVMEDIA_TYPE_NB] = {
			[AVMEDIA_TYPE_AUDIO]=-1,
			[AVMEDIA_TYPE_VIDEO]=-1,
			[AVMEDIA_TYPE_SUBTITLE]=-1,
	};
#else
	int		wanted_stream = -1;
#endif
	/* set all streams to be AVDISCARD_ALL to speed up the process.
	 * Which selects which packets can be discarded at will and do not 
	 * need to be demuxed. */
	/* 20130719 Remove the unknown streams in display. Currently Ezthumb 
	 * only recognize video, audio and subtitle stream */
	vidx->ezstream = 0;
	for (i = 0; i < (int)vidx->formatx->nb_streams; i++) {
		stream = vidx->formatx->streams[i];
		stream->discard = AVDISCARD_ALL;
		switch (stream->codec->codec_type) {
		case AVMEDIA_TYPE_VIDEO:
			eznotify(vidx->sysopt, EN_TYPE_VIDEO, i, 0, stream);
			vidx->ezstream++;
			break;
		case AVMEDIA_TYPE_AUDIO:
			eznotify(vidx->sysopt, EN_TYPE_AUDIO, i, 0, stream);
			vidx->ezstream++;
			break;
		case AVMEDIA_TYPE_SUBTITLE:
			eznotify(vidx->sysopt, EN_TYPE_SUBTTL, i, 0, stream);
			vidx->ezstream++;
			break;
		default:
			eznotify(vidx->sysopt, EN_TYPE_UNKNOWN, i, 0, stream);
			break;
		}
	}

	/* verify the user define stream index is a valid and 
	 * video attributed stream index */
	n = vidx->sysopt->vs_user;
	if ((n >= 0) && (n < (int)vidx->formatx->nb_streams)) {
		stream = vidx->formatx->streams[n];
		if (stream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			return n;
		}
	}
	
#ifdef	HAVE_AV_FIND_BEST_STREAM
	n = av_find_best_stream(vidx->formatx, AVMEDIA_TYPE_VIDEO,
			wanted_stream[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
#else
	n = -1;
	for (i = 0; i < (int)vidx->formatx->nb_streams; i++) {
		stream = vidx->formatx->streams[i];
		if (stream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			if (wanted_stream < stream->codec_info_nb_frames) {
				n = i;
				wanted_stream = stream->codec_info_nb_frames;
			}
		}
	}
#endif
	return n;
}


static int64_t video_keyframe_next(EZVID *vidx, AVPacket *packet)
{
	int64_t		dts, lastdts = 0;

	while ((dts = video_load_packet(vidx, packet)) >= 0) {
		lastdts = dts;
		if (packet->flags != AV_PKT_FLAG_KEY) {
			av_free_packet(packet);
			continue;
		}
		break;
	}
	/* A small trick here. If video_load_packet() read out of the media
	 * file, the last good dts will be hiden in the packet structure for
	 * the futurn access. It's the last DTS in the file */
	if (dts < 0) {
		packet->dts = lastdts;
	}
	/* reset the key frame decoding flag */
	video_frame_reset(vidx);
	return dts;
}

static int64_t video_keyframe_to(EZVID *vidx, AVPacket *packet, int64_t pos)
{
	int64_t		dts;

	while ((dts = video_load_packet(vidx, packet)) >= 0) {
		if (packet->flags != AV_PKT_FLAG_KEY) {
			av_free_packet(packet);
			continue;
		}

		if (dts >= pos) {
			break;	/* successfully located the keyframe */
		}

		/* reset the key frame decoding flag */
		video_frame_reset(vidx);

		/* FIXME: possibly failure when decoding on the fly
		 * sometimes a decoded frame could turn to a P-frame even 
		 * it came from a packet marked as I-frame. */
		if (vidx->ses_flags & EZOP_DECODE_OTF) {
			video_decode_next(vidx, packet);
			video_frame_update(vidx);
		} else {
			av_free_packet(packet);
		}
	}
	return dts;
}

static int video_keyframe_credit(EZVID *vidx, int64_t dts)
{
	/* reset the key frame crediting */ 
	/* note that ezthumb never reset the keygap */
	EDB_MODL(("video_keyframe_credit: %lld\n", dts));
	if (dts < 0) {
		/* save (top up) the recent session before reset */
		vidx->keyalldts += vidx->keylast - vidx->keyfirst;
		vidx->keyallkey += vidx->keycount;
		/* reset the current session */
		vidx->keylast  = vidx->keyfirst = -1;
		vidx->keycount = 0;
		eznotify(vidx->sysopt, EN_IFRAME_CREDIT, 
				ENX_IFRAME_RESET, 0, vidx);
		return vidx->keycount;
	}

	/* record the status of the first key frame since resetted */
	if (vidx->keylast < 0) {
		vidx->keylast = vidx->keyfirst = dts;
		/* 20140416 Issue 'deashi.wmv': using the keygap to track
		 * the single i-frame videos */
		if (vidx->keygap < 0) {
			vidx->keygap = 0;
		}
		eznotify(vidx->sysopt, EN_IFRAME_CREDIT, 
				ENX_IFRAME_SET, 0, vidx);
		return vidx->keycount;
	}

	if (dts - vidx->keylast > vidx->keygap) {
		vidx->keygap = dts - vidx->keylast;
		eznotify(vidx->sysopt, EN_IFRAME_CREDIT, 
				ENX_IFRAME_UPDATE, 0, vidx);
	}
	vidx->keycount++;
	vidx->keylast = dts;

	/* update the avarage DTS per key frame */
	vidx->keydts = (vidx->keyalldts + vidx->keylast - vidx->keyfirst) /
		(vidx->keyallkey + vidx->keycount);
	return vidx->keycount;
}

static int video_dts_ruler(EZVID *vidx, int64_t cdts, int64_t ndts)
{
	int64_t	span;
	int	n;

	if (ndts > cdts) {
		span = ndts - cdts;
	} else {
		span = cdts - ndts;
	}
	if (vidx->keygap > 0) {
		n = (int)(span / vidx->keygap);
	} else if (vidx->keygap < 0) {
		n = INT_MAX;	/* accredit system is not ready */
	} else {
		n = 0;	/* one i-frame only */
	}
	//printf("video_dts_ruler: %lld %lld %d\n", cdts, ndts, n);
	return n;
}

/* 20130726 Integrated the key frame accrediting into video_load_packet()
 * because of a surprise finding in carcrash.flv that a key frame can be 
 * loaded while decoding by video_decode_next(). It's more consistent that
 * accrediting keyframe in video_load_packet(). However other functions
 * must look care of resetting accrediting */
static int64_t video_load_packet(EZVID *vidx, AVPacket *packet)
{
	int64_t	dts;

	while (av_read_frame(vidx->formatx, packet) >= 0) {
		if (packet->stream_index != vidx->vsidx) {
			av_free_packet(packet);
			continue;
		}
		if ((dts = video_packet_timestamp(packet)) < 0) {
			av_free_packet(packet);
			continue;
		}

		/* accrediting the key frame */
		if (packet->flags == AV_PKT_FLAG_KEY) {
			video_keyframe_credit(vidx, packet->dts);
		}
		return dts;
	}
	return -1;
}


/* This function is used to print the media information to the specified
 * area in the canvas */
static int video_media_on_canvas(EZVID *vidx, EZIMG *image)
{
	AVStream	*stream;
	char		*buffer, tmp[32];
	int		i, line = 0;

	// FIXME: UTF-8 and wchar concern
	/* Line 0: the filename */
	if (vidx->dur_all) {	/* binding mode */
		i = strlen(vidx->anchor->filename) + 32;
	} else {
		i = strlen(vidx->filename) + 32;
	}
	if (i < 256) {
		i = 256;
	}
	if ((buffer = smm_alloc(i)) == NULL) {
		return EZ_ERR_LOWMEM;
	}

	strcpy(buffer, "NAME: ");
	if (vidx->dur_all) {	/* binding mode */
		csc_path_basename(vidx->anchor->filename, buffer + 6, i - 6);
		sprintf(tmp, "  + %d more", vidx->bound - 1);
		strcat(buffer, tmp);
	} else {
		csc_path_basename(vidx->filename, buffer + 6, i - 6);
	}
	image_gdcanvas_print(image, line++, 0, buffer);

	/* Line 1: the duration of the video clip, the file size, 
	 * the frame rates and the bit rates */
	strcpy(buffer, "Duration: ");
	if (vidx->dur_all) {	/* binding mode */
		strcat(buffer, meta_timestamp(vidx->dur_all, 0, tmp));
	} else {
		strcat(buffer, meta_timestamp(vidx->duration, 0, tmp));
	}

	strcat(buffer, " (");
	if (vidx->dur_all) {	/* binding mode */
		strcat(buffer, meta_filesize(vidx->bind_size, tmp));
	} else {
		strcat(buffer, meta_filesize(vidx->filesize, tmp));
	}
	strcat(buffer, ")  ");

	/*i = vidx->formatx->bit_rate;
	if (vidx->formatx->bit_rate == 0) {
		i = (int)(vidx->formatx->file_size * 1000 / vidx->duration);
	}*/
	/* formatx->bit_rate sometimes missed as audio bitrate by ffmpeg.
	 * vidx->bitrates is a reference bitrate
	 * so we calculate an overall bit rate here */
	sprintf(tmp, "%.3f kbps", (float)vidx->bitrates / 1000.0);
	strcat(buffer, tmp);
	image_gdcanvas_print(image, line++, 0, buffer);

	/* Line 2+: the stream information */
	for (i = 0; i < (int)vidx->formatx->nb_streams; i++) {
		stream = vidx->formatx->streams[i];
		sprintf(buffer, "%s: ", id_lookup_tail(id_codec_type, 
					stream->codec->codec_type));
		/* seems higher version doesn't support CODEC_TYPE_xxx */
		switch (stream->codec->codec_type) {
		case AVMEDIA_TYPE_VIDEO:	
			video_media_video(stream, buffer);
			break;
		case AVMEDIA_TYPE_AUDIO:
			video_media_audio(stream, buffer);
			break;
		case AVMEDIA_TYPE_SUBTITLE:
			video_media_subtitle(stream, buffer);
			break;
		default:
			/* 20130719 Remove the unknown streams in display */
			strcat(buffer, "Unknown");
			EDB_SHOW(("%s\n", buffer));
			continue;
		}
		image_gdcanvas_print(image, line++, 0, buffer);
	}
	smm_free(buffer);
	return EZ_ERR_NONE;
}

/* This function is used to find the video clip's duration. There are three
 * methods to retrieve the duration. First and the most common one is to
 * grab the duration data from the clip head, EZOP_DUR_HEAD. It's already 
 * available in the AVFormatContext structure when opening the video. 
 * However, sometimes the information is inaccurate or lost. The second method,
 * EZOP_DUR_FSCAN, is used to scan the entire video file till the last packet
 * to decide the final PTS. To speed up the process, the third method,
 * EZOP_DUR_QSCAN, only scan the last 90% clip. 
 * User need to specify the scan method. */
static EZTIME video_duration(EZVID *vidx)
{
	EZIMG	*image;
	int64_t	ref_dur;
	int	ref_err, shots, key_num;

	video_timing(vidx, EZ_PTS_CLEAR);

	/* find the duration in the header */
	vidx->seekable = ENX_SEEK_UNKNOWN;
	vidx->duration = 
		(EZTIME)(vidx->formatx->duration / AV_TIME_BASE * 1000);

	switch (GETDURMOD(vidx->ses_flags)) {
	case EZOP_DUR_HEAD:
		/* test the seekability of the media file */
		/*vidx->seekable = video_seek_challenge(vidx);*/
		/* On second thought, it perhaps is a good idea that leave
		 * EZOP_DUR_HEAD mode the quickest mode. If user knew the 
		 * video is good, there's no need to do the time consuming
		 * video_seek_challenge */
		vidx->seekable = ENX_SEEK_FREE;
		break;

	case EZOP_DUR_QSCAN:
		/* test the seekability of the media file */
		vidx->seekable = video_seek_challenge(vidx);
		video_timing(vidx, EZ_PTS_DSEEK);
		if (vidx->seekable == ENX_SEEK_UNKNOWN) {
			EDB_PROG(("video_duration: [Q] scan done\n"));
		} else if (!SEEKABLE(vidx->seekable)) {
			EDB_PROG(("video_duration: [Q] fullscan\n"));
			vidx->duration = video_duration_fullscan(vidx);
		} else if ((ref_dur = video_duration_quickscan(vidx)) > 0) {
			EDB_PROG(("video_duration: [Q] quickscan\n"));
			vidx->duration = ref_dur;
		} else {	/* foolproof mode */
			EDB_PROG(("video_duration: [Q] fullscan back\n"));
			video_seeking(vidx, 0);
			vidx->duration = video_duration_fullscan(vidx);
		}
		video_timing(vidx, EZ_PTS_DSCAN);
		break;

	case EZOP_DUR_FSCAN:
		vidx->seekable = video_seek_challenge(vidx);
		video_timing(vidx, EZ_PTS_DSEEK);
		if (vidx->seekable != ENX_SEEK_UNKNOWN) {
			vidx->duration = video_duration_fullscan(vidx);
			video_timing(vidx, EZ_PTS_DSCAN);
		}
		break;

	case EZOP_DUR_AUTO:
	default:
		/* test the seekability of the media file */
		vidx->seekable = video_seek_challenge(vidx);
		video_timing(vidx, EZ_PTS_DSEEK);
		if (vidx->seekable == ENX_SEEK_UNKNOWN) {
			EDB_PROG(("video_duration: [A] scan done\n"));
			break;
		}
		/* estimate the duration of the video file */
		ref_dur = vidx->filesize * 8000 / vidx->bitrates;
		/* get the error of the duration by head and by calculation */
		ref_err = (int)((vidx->duration - ref_dur) * 1000 
					/ vidx->duration);
		EDB_PROG(("video_duration: compared (%lld/%lld/%d)\n", 
				vidx->duration, ref_dur, ref_err));
		/* calculate the possible keyframes in the video file */
		key_num = (int)(video_ms_to_dts(vidx, ref_dur) / vidx->keydts);
		/* estimate the total shots */
		shots = 0;
		if ((image = image_allocate(vidx, ref_dur, NULL)) != NULL) {
			shots = image->shots;
			image_free(image);
		}

		if (shots > key_num / 2) {
			/* if the estimated shots are more than half of 
			 * estimate keyframe, ezthumb enforce the full scan */
			EDB_PROG(("video_duration: few keyframes %d/%d\n",
					shots, key_num));
			vidx->duration = video_duration_fullscan(vidx);
			video_timing(vidx, EZ_PTS_DSCAN);
		} else if (abs(ref_err) < 200) {
			/* In auto mode, ezthumb intends to use the duration
			 * read from the head. However if the error to the 
			 * estimated duration is greater than 20%, ezthumb
			 * will turn to scan mode */
			break;
		} else if (!SEEKABLE(vidx->seekable)) {
			EDB_PROG(("video_duration: error %d fullscan\n",
					ref_err));
			vidx->duration = video_duration_fullscan(vidx);
			video_timing(vidx, EZ_PTS_DSCAN);
		} else if ((ref_dur = video_duration_quickscan(vidx)) > 0) {
			EDB_PROG(("video_duration: error %d quickscan\n",
					ref_err));
			vidx->duration = ref_dur;
			video_timing(vidx, EZ_PTS_DSCAN);
		} else {
			EDB_PROG(("video_duration: error %d full\n", ref_err));
			video_seeking(vidx, 0);
			vidx->duration = video_duration_fullscan(vidx);
			video_timing(vidx, EZ_PTS_DSCAN);
		}
		break;
	}
	vidx->bitrates = (int)(vidx->filesize * 8000 / vidx->duration);

	eznotify(vidx->sysopt, EN_DURATION, 0,
			smm_time_diff(&vidx->tmark), vidx);
	EDB_PROG(("video_duration: %lld S:%d BR:%d (%d ms)\n", 
			vidx->duration, vidx->seekable, vidx->bitrates, 
			smm_time_diff(&vidx->tmark)));
	return vidx->duration;
}

/* 20120308: should seek to position according to the length of the 
 * file rather than the duration. It's quite obvious that when one 
 * need the scan mode, the duration must has been wrong already.
 * 20120313: AVSEEK_FLAG_BYTE is incapable to seek through some video file.
 */
static EZTIME video_duration_quickscan(EZVID *vidx)
{
	AVPacket	packet;
	int64_t		base, rdts, now, recent;
	int		i;

	SETDURMOD(vidx->ses_flags, EZOP_DUR_QSCAN);	/* quick scan */

	/* using the binary seach to constrain to the start point */
	rdts = video_ms_to_dts(vidx, vidx->filesize * 8000 / vidx->bitrates);
	base = now = recent = 0;
	for (i = 0; i < 4; ) {
		if ((rdts /= 2) == 0) {
			break;
		}
		eznotify(vidx->sysopt, EN_OPEN_GOING, 0, 0, vidx);
		video_seeking(vidx, base + rdts);
		/* 20130915 In the second Sjako test, I found av_read_frame() 
		 * always return a packet right after an av_seek, even it's
		 * ready sought out of the boundary. It caused ezthumb think
		 * it's still inside the media file. The simplest fix is to
		 * load the second packet so av_read_frame() could return -1
		 * to indicate its end of stream */
		now = video_load_packet(vidx, &packet);
		EDB_PROG(("video_duration_quickscan: seek %lld get %lld\n", 
				base + rdts, now));
		if (now > 0) {
			av_free_packet(&packet);
			if (now <= recent) {
				break;
			}
			base += rdts;
			recent = now;
			i++;
		}
	}
	/* 20130926 The previous fix by the dummy readout was proved failure 
	 * in the third Sjako test. The av_read_frame() must move the index
	 * to the end of media file once it's out of range. Therefore it could
	 * read back one or more packets */
	while (video_load_packet(vidx, &packet) > 0) {
		recent = packet.dts;
		av_free_packet(&packet);
		eznotify(vidx->sysopt, EN_OPEN_GOING, 0, 0, vidx);
	}
	if ((recent -= vidx->dts_offset) <= 0) {
		EDB_PROG(("video_duration_quickscan: failed\n"));
		return -1;
	}
	EDB_PROG(("video_duration_quickscan: succeed (%lld)\n", recent));
	return (EZTIME) video_dts_to_ms(vidx, recent);
}

static EZTIME video_duration_fullscan(EZVID *vidx)
{
	EZTIME	dts;

	SETDURMOD(vidx->ses_flags, EZOP_DUR_FSCAN);
	dts = video_statistics(vidx) - vidx->dts_offset;
	dts = (EZTIME) video_dts_to_ms(vidx, dts > 0 ? dts : 0);
	EDB_PROG(("video_duration_fullscan: %lld %lld\n", dts, vidx->keygap));
	return dts;
}

static int video_seek_challenge(EZVID *vidx)
{
	AVPacket	packet;
	int64_t		dts_first, pos_first, dts_second, pos_second;
	int64_t		dts_begin, dts_target, dts_span, cur_dts, next_dts;
	int64_t		key_dts, key_num;
	int		i, error, errmin;

	/* initial scan the beginning part of the video up to 10 second
	 * (EZ_DSCP_RANGE_INIT) and 10 keyframe to calculate the bitrates */
	dts_first = pos_first = 0;
	dts_target = video_ms_to_dts(vidx, EZ_DSCP_RANGE_INIT);
	dts_target += vidx->dts_offset;
	video_keyframe_credit(vidx, -1);
	while ((cur_dts = video_keyframe_next(vidx, &packet)) >= 0) {
		dts_first = cur_dts;
		pos_first = packet.pos;
		av_free_packet(&packet);
		eznotify(vidx->sysopt, EN_OPEN_GOING, 0, 0, NULL);
		if ((dts_first > dts_target) && (vidx->keycount > 10)) {
			break;
		}
	}
	if (cur_dts < 0) {	/* END OF FILE so save the last dts */
		/* the media file is too short to do an autodetection;
		 * actually it's done a full scan already */
		SETDURMOD(vidx->ses_flags, EZOP_DUR_FSCAN);
		/* retrieve the last good DTS */
		cur_dts = packet.dts - vidx->dts_offset;
		/* return the duration and mark it SEEK_UNKNOWN */
		vidx->duration = (EZTIME) video_dts_to_ms(vidx, cur_dts);
		EDB_PROG(("video_seek_challenge: short video %lld\n", 
				vidx->duration));
		return ENX_SEEK_UNKNOWN;
	}
	EDB_PROG(("video_seek_challenge: reference i-frame "
			"DTS=%lld POS=%lld\n", dts_first, pos_first));

	/* The current postion should be around the first second in the video
	 * file. If it's smaller than 1/10 (EZ_DSCP_RANGE_EXT) of the whole 
	 * file, ezthumb will extend the sample range to around 100 second to 
	 * get an accurate reading */
	if (vidx->filesize / pos_first > EZ_DSCP_RANGE_EXT) {
		dts_target = dts_first * EZ_DSCP_RANGE_EXT - 
				vidx->dts_offset * (EZ_DSCP_RANGE_EXT - 1);
		while ((cur_dts = video_keyframe_next(vidx, &packet)) >= 0) {
			dts_first = cur_dts;
			pos_first = packet.pos;
			av_free_packet(&packet);
			eznotify(vidx->sysopt, EN_OPEN_GOING, 0, 0, NULL);
			if (dts_first > dts_target) {
				break;
			}
		}
		if (cur_dts < 0) {	/* END OF FILE so save the last dts */
			SETDURMOD(vidx->ses_flags, EZOP_DUR_FSCAN);
			cur_dts = packet.dts - vidx->dts_offset;
			vidx->duration = (EZTIME) 
				video_dts_to_ms(vidx, cur_dts);
			EDB_PROG(("video_seek_challenge: short video %lld\n", 
					vidx->duration));
			return ENX_SEEK_UNKNOWN;
		}
		EDB_PROG(("video_seek_challenge: reference extended "
				"DTS=%lld POS=%lld\n", dts_first, pos_first));
	}

	/* calculate the key-frame rate in dts. I use this rate instead
	 * of bitrate to simplify the calculation.
	 * key frame rate means how many dts in avarage per key frame */
	key_dts = (vidx->keylast - vidx->keyfirst) / vidx->keycount;
	key_num = av_rescale(vidx->filesize, vidx->keycount, pos_first);
	EDB_PROG(("video_seek_challenge: keyframe %lld dts * %lld\n",
			key_dts, key_num));

	if (key_num - vidx->keycount > 50) {
		dts_begin  = dts_first;
		dts_target = key_dts * 5;
	} else if (key_num - vidx->keycount > 20) {
		dts_begin  = dts_first;
		dts_target = key_dts * 2;
	} else {
		/* There's not enough key frames left in the video file 
		 * worthing a seekable test, probably a too small clip. */
		vidx->duration = video_duration_fullscan(vidx);
		EDB_PROG(("video_seek_challenge: short video %lld\n", 
				vidx->duration));
		return ENX_SEEK_UNKNOWN;
	}

	EDB_PROG(("video_seek_challenge: seek forward from %lld by %lld\n",
			dts_begin, dts_target));

	/* seeking forward test */
	dts_second = next_dts = dts_begin;
	pos_second = 0;
	errmin  = 10000;
	for (i = 0; i < EZ_DSCP_N_STEP; i++) {
		next_dts += dts_target;
		eznotify(vidx->sysopt, EN_OPEN_GOING, 0, 0, NULL);
		/* 20130725 changed the calculating method by enlarge
		 * the dts span to restraint the error quicker */
		//if ((dts_span = next_dts - cur_dts) == 0) {
		if ((dts_span = next_dts - dts_begin) == 0) {
			break;
		}
		/* 20130718 Changed from
		 * video_seeking(vidx, next_dts);
		 * to avformat_seek_file() with minimum dts requirement.
		 * Otherwise the broken.avi kept seeking 1 dts pace which
		 * is quite annoying */
		avformat_seek_file(vidx->formatx, vidx->vsidx, next_dts, 
				next_dts, INT64_MAX, AVSEEK_FLAG_ANY);
		video_keyframe_credit(vidx, -1);

		if ((cur_dts = video_load_packet(vidx, &packet)) < 0) {
			next_dts -= dts_target;	/* reverse the last attempt */
			break;
		}
		dts_second = cur_dts;
		pos_second = packet.pos;
		av_free_packet(&packet);
		
		error = abs((int)((cur_dts - next_dts) * 1000 / dts_span));
		EDB_PROG(("video_seek_challenge: forward seeking "
				"%lld/%lld %d\n", cur_dts, next_dts, error));
		errmin = (error < errmin) ? error : errmin;
	}
/*
#if	LIBAVFORMAT_VERSION_INT < (53<<16)
	dts_span = url_ftell(vidx->formatx->pb);
#else
	dts_span = avio_tell(vidx->formatx->pb);
#endif
	printf("postion %lld\n", dts_span);
*/
	/* calculate the reference bitrates by recent seeking results */
	if (dts_second > dts_first) {
		dts_span = dts_second - vidx->dts_offset;
		dts_span = video_dts_to_ms(vidx, dts_span);
		vidx->bitrates = (int)(pos_second * 8000 / dts_span);
	} else {
		dts_span = dts_first - vidx->dts_offset;
		dts_span = video_dts_to_ms(vidx, dts_span);
		vidx->bitrates = (int)(pos_first * 8000 / dts_span);
	}
	EDB_PROG(("video_seek_challenge: reference bitrate is %d\n", 
			vidx->bitrates));

	if (errmin > EZ_DSCP_STEP_ERROR) {
		/* unable to seek or error > 30% */
		EDB_PROG(("video_seek_challenge: forward seeking failed %d\n",
					errmin));
		/* reset the keyframe accrediting because of many seeking */
		video_keyframe_credit(vidx, -1);
		return ENX_SEEK_NONE;
	}

	/* seeking backward test */
	errmin  = 10000;
	cur_dts = dts_second;
	for (i = 0; i < EZ_DSCP_N_STEP; i++) {
		if ((next_dts -= dts_target) < 0) {
			next_dts = 0;
		}
		eznotify(vidx->sysopt, EN_OPEN_GOING, 0, 0, NULL);
		//if ((dts_span = cur_dts - next_dts) == 0) {
		if ((dts_span = dts_second - next_dts) == 0) {
			break;
		}
		avformat_seek_file(vidx->formatx, vidx->vsidx, next_dts, 
				next_dts, INT64_MAX, AVSEEK_FLAG_ANY);
		video_keyframe_credit(vidx, -1);

		if ((cur_dts = video_load_packet(vidx, &packet)) < 0) {
			break;
		}
		av_free_packet(&packet);
		
		error = abs((int)((cur_dts - next_dts) * 1000 / dts_span));
		EDB_PROG(("video_seek_challenge: backward seeking "
				"%lld/%lld %d\n", cur_dts, next_dts, error));
		errmin = (error < errmin) ? error : errmin;
	}
	/* reset the keyframe accrediting because of many seeking */
	video_keyframe_credit(vidx, -1);
	
	if (errmin > EZ_DSCP_STEP_ERROR) {
		EDB_PROG(("video_seek_challenge: backward seeking failed %d\n",
					errmin));
		return ENX_SEEK_FORWARD;
	}
	EDB_PROG(("video_seek_challenge: bi-direction seekable. (%d ms)\n",
			smm_time_diff(&vidx->tmark)));
	return ENX_SEEK_FREE;
}

static int64_t video_statistics(EZVID *vidx)
{
	struct MeStat	mestat[EZ_ST_MAX_REC];	/* shoule be big enough */
	struct ezntf	myntf;
	AVPacket	packet;
	int		i, imax = 0;

	memset(mestat, 0, sizeof(mestat));
	video_keyframe_credit(vidx, -1);
	while (av_read_frame(vidx->formatx, &packet) >= 0) {
		i = packet.stream_index;
		if (i > (int)vidx->formatx->nb_streams) {
			i = vidx->formatx->nb_streams;
		}
		if (i >= EZ_ST_MAX_REC) {
			av_free_packet(&packet);
			continue;
		}
		imax = i > imax ? i : imax;

		mestat[i].received++;
		//dump_packet(&packet);
		/* higher version ffmpeg doesn't support PKT_FLAG_KEY */
		if (packet.flags == AV_PKT_FLAG_KEY) {
			mestat[i].key++;
			if (packet.stream_index == vidx->vsidx) {
				video_keyframe_credit(vidx, packet.dts);
				eznotify(vidx->sysopt, EN_PACKET_KEY, 
						0, 0, &packet);
			}
		}
		if (packet.dts != (int64_t)AV_NOPTS_VALUE) {
			if (packet.dts < mestat[i].dts_last) {
				mestat[i].rewound++;
				mestat[i].dts_base += mestat[i].dts_last;
			}
			mestat[i].dts_last = packet.dts;
		}
		av_free_packet(&packet);
		eznotify(vidx->sysopt, EN_OPEN_GOING, 0, 0, vidx);
	}
	myntf.varg1 = mestat;
	myntf.varg2 = vidx;
	eznotify(vidx->sysopt, EN_MEDIA_STATIS, imax + 1, 0, &myntf);
	if (vidx->vsidx < 0) {
		return 0;
	}
	return mestat[vidx->vsidx].dts_base + mestat[vidx->vsidx].dts_last;
}

static int64_t video_snap_point(EZVID *vidx, EZIMG *image, int index)
{
	EZTIME	vpos;
	int64_t	seekat;

	vpos = image->time_from;

	/* 20130807 use global configure structure instead of local one
	 * to remove the annoying setting in binding mode */
	if ((vidx->sysopt->flags & EZOP_FFRAME) == 0) {
		index++;
	}
	vpos += image->time_step * index;
	
	if (vidx->dur_all) {		/* binding mode */
		vpos -= vidx->dur_off;
		/* 20130808 FIXME: if vpos < 0, it must be caused by the lag 
		 * of the index; the previous index failed to make thumbnail
		 * in the previous clip so it was carried up to the current 
		 * clip. the workaround is to top up the time step to make
		 * it positive, which would produce samesome screenshots */
		while (vpos < 0) {
			vpos += image->time_step;
		}
		EDB_PROG(("video_snap_point: "
					"ID=%d POS=%lld DUR=%lld OFF=%lld\n",
				index, vpos, vidx->duration, vidx->dur_off));
		if (vpos > vidx->duration) {
			return -1;	/* outside this clip */
		}
	}

	seekat = vidx->dts_offset + video_ms_to_dts(vidx, vpos);
	return seekat;
}

static int video_snap_begin(EZVID *vidx, EZIMG *image, int method)
{
	/* check if this is called by the first clip */
	if (vidx->dur_all && vidx->dur_off) {
		eznotify(vidx->sysopt, EN_PROC_BINDING, method, 0, vidx);
		return 0;
	}

	/* If the output format is the animated GIF89a, then it opens
	 * the target file and device */
	image->gifx_fp = NULL;
#ifdef	HAVE_GD_IMAGE_GIFANIMATION
	if ((image->gifx_opt = image_cal_gif_animix(image->sysopt)) > 0) {
		/* 20130627 using vidx->filename should be fine here because 
		 * only the first clip would be processed by this function */
		image->gifx_fp = image_gif_anim_open(image, vidx->filename);
	}
#else
	image->gifx_opt = 0;
#endif

	eznotify(vidx->sysopt, EN_PROC_BEGIN, method, 0, vidx);
	return 0;
}

static int video_snap_update(EZVID *vidx, EZIMG *image, int64_t dts)
{
	struct	ezntf	myntf;
	EZFRM	*ezfrm;
	char	timestamp[64];
	EZTIME	dtms;

	if ((ezfrm = video_frame_best(vidx, dts)) == NULL) {
		return -1;	/* no proper frame */
	}
	myntf.varg1 = vidx;
	myntf.varg2 = ezfrm;
	eznotify(vidx->sysopt, EN_FRAME_EFFECT, image->taken, 0, &myntf);

	/* convert current PTS to millisecond and then 
	 * metamorphose to human readable form */
	dtms = ezfrm->rf_dts - vidx->dts_offset;
	dtms = video_dts_to_ms(vidx, dtms > 0 ? dtms : 0);
	if (vidx->dur_all == 0) {
		meta_timestamp(dtms, 1, timestamp);
	} else {		/* binding mode */
		dtms += vidx->dur_off;	/* aligning the binding clips */
		timestamp[0] = '(';
		meta_timestamp(dtms, 1, timestamp + 1);
		strcat(timestamp, ")");
	}

	/* scale the frame into GD frame structure */
	sws_scale(vidx->swsframe->context, 
			(const uint8_t * const *) ezfrm->frame->data,
			ezfrm->frame->linesize, 0, vidx->height, 
			vidx->swsframe->frame->data, 
			vidx->swsframe->frame->linesize);
	image_gdframe_update(image, vidx->swsframe->frame);

	/* write the timestamp into the shot */
	if (image->sysopt->flags & EZOP_TIMEST) {
		image_gdframe_timestamp(image, timestamp);
	}

	if (image->gdcanvas) {
		image_gdcanvas_update(image, image->taken);
#ifdef	HAVE_GD_IMAGE_GIFANIMATION
	} else if (image->gifx_fp) {
		image_gif_anim_add(image, image->gifx_fp, image->gifx_opt);
#endif
	} else {
		image_gdframe_save(image, vidx->filename, image->taken);
	}

	/* update the number of taken shots, must before displaying */
	image->taken++;

	/* display the on-going information */
	if (image->shots) {
		eznotify(vidx->sysopt, EN_PROC_CURRENT, 
				image->shots, image->taken, &dts);
	} else {	/* i-frame ripping */
		eznotify(vidx->sysopt, EN_PROC_CURRENT, 
				(long)(image->time_during/100), 
				(long)((dtms - image->time_from)/100), &dts);
	}

	/* update the progress time stamp array */
	video_timing(vidx, EZ_PTS_UPDATE);
	return 0;
}

static int video_snap_end(EZVID *vidx, EZIMG *image)
{
	struct	ezntf	myntf;
	char	status[128];

	if (vidx->dur_all && vidx->next) {	/* hasn't finished */
		return 0;
	}

	/* check if all images been taken */
	if (image->taken < image->shots) {
		eznotify(vidx->sysopt, EN_STREAM_BROKEN, 
				image->taken, image->shots, NULL);
#ifdef	CFG_FILL_LAST_IMAGE
		while (image->taken < image->shots) {
			video_snap_update(vidx, image, -1);
		}
#endif
	}

	eznotify(vidx->sysopt, EN_PROC_END, image->canvas_width, 
			image->canvas_height, image);

	/* display the end of the process and generate the status line */
	sprintf(status, "%dx%dx%d Thumbnails Generated by Ezthumb %s (%.3f s)",
			image->dst_width, image->dst_height, image->taken,
			EZTHUMB_VERSION, 
			smm_time_diff(&vidx->tmark) / 1000.0);

	if (image->gdcanvas && (image->sysopt->flags & EZOP_INFO)) {
		/* update the media information area */
		video_media_on_canvas(vidx, image);
		/* Insert as status line */
		image_gdcanvas_print(image, -1, 0, status);
	} else {
		/* display the media information in console */
		video_media_on_canvas(vidx, NULL);
	}
	if (image->gdcanvas) {
		if (vidx->anchor) {
			image_gdcanvas_save(image, vidx->anchor->filename);
		} else {
			image_gdcanvas_save(image, vidx->filename);
		}
#ifdef	HAVE_GD_IMAGE_GIFANIMATION
	} else if (image->gifx_fp) {
		image_gif_anim_close(image, image->gifx_fp);
#endif
	}

	myntf.varg1 = vidx;
	myntf.varg2 = image;
	eznotify(vidx->sysopt, EN_PROC_SAVED, 0, 0, &myntf);
	return 0;
}

static EZFRM *video_frame_alloc(int pixfmt, int width, int height)
{
	EZFRM	*ezfrm;
	int	fbsize;
	
	if ((ezfrm = smm_alloc(sizeof(EZFRM))) == NULL) {
		return NULL;
	}
	
#ifdef	HAVE_AV_FRAME_ALLOC
	ezfrm->frame = av_frame_alloc();
#elif	defined(HAVE_AVCODEC_ALLOC_FRAME)
	ezfrm->frame = avcodec_alloc_frame();
#else
#error	None of av_frame_alloc() or avcodec_alloc_frame() defined!
#endif
	if (ezfrm->frame == NULL) {
		video_frame_free(ezfrm);
		return NULL;
	}

	if (width && height) {
		/* allocate the memory buffer for holding the pixel array of
		 * RGB frame */
		fbsize = avpicture_get_size(pixfmt, width, height);
		if ((ezfrm->rf_buffer = av_malloc(fbsize)) == NULL) {
			video_frame_free(ezfrm);
			return NULL;
		}
		/* link the RGB frame and the RBG pixel buffer */
		avpicture_fill((AVPicture *) ezfrm->frame, ezfrm->rf_buffer,
				pixfmt, width, height);
	}
	
	/* 20120723 Initialize the rf_dts to -1 to avoid the 
	 * first-unavailable-frame issue */
	ezfrm->rf_dts = -1;
	return ezfrm;
}

static int video_frame_free(EZFRM *ezfrm)
{
	unsigned char	*frmbuf;
	
	if (ezfrm == NULL) {
		return 0;
	}
	
	frmbuf = ezfrm->rf_buffer;
	if (ezfrm->rf_buffer) {
		av_free(ezfrm->rf_buffer);
	}
	
	if (ezfrm->frame) {
		/* check if the frame buffer has already been freed */
		if (ezfrm->frame->data[0] == frmbuf) {
			ezfrm->frame->data[0] = NULL;
		}
#ifdef	HAVE_AV_FRAME_ALLOC
		av_frame_free(ezfrm->frame);
#else
		av_free(ezfrm->frame);
#endif
	}
	return smm_free(ezfrm);
}

static int video_frame_reset(EZVID *vidx)
{
	vidx->fkey = 0;
	return 0;
}

static int video_frame_update(EZVID *vidx)
{
	AVPicture	tmpav;
	
	/* 20150223: only store the current frame until a keyframe is 
	 * successfully decoded. Otherwise the frame buffer may be stuffed
	 * by broken P-Frames */
	if (vidx->fkey == 0) {
		return 0;
	}
	
	memcpy(&tmpav, (AVPicture *) vidx->picframe->frame, sizeof(tmpav));
	memcpy(vidx->picframe->frame, vidx->vidframe->frame, sizeof(AVFrame));
	memcpy((AVPicture *) vidx->picframe->frame, &tmpav, sizeof(tmpav));
	av_picture_copy((AVPicture *) vidx->picframe->frame, 
			(AVPicture *) vidx->vidframe->frame, 
			vidx->codecx->pix_fmt, vidx->width, vidx->height);
	
	vidx->picframe->rf_dts  = vidx->vidframe->rf_dts;
	vidx->picframe->rf_pos  = vidx->vidframe->rf_pos;
	vidx->picframe->rf_size = vidx->vidframe->rf_size;
	vidx->picframe->rf_pac  = vidx->vidframe->rf_pac;
	return vidx->fkey;
}

static EZFRM *video_frame_best(EZVID *vidx, int64_t refdts)
{
	if (vidx->picframe->rf_dts <= 0) {
		return vidx->vidframe;
	}
	if (vidx->fkey == 0) {
		return vidx->picframe;
	}
	if (compare_timestamp(refdts, vidx->picframe->rf_dts, 
				vidx->vidframe->rf_dts) <= 0) {
		return vidx->picframe;
	}
	return vidx->vidframe;
}

static int64_t video_decode_next(EZVID *vidx, AVPacket *packet)
{
	int64_t	tmp;
	int	got_pict = 1;

	vidx->vidframe->rf_pos  = packet->pos;
	vidx->vidframe->rf_size = 0;
	vidx->vidframe->rf_pac  = 0;

	do {
		eznotify(vidx->sysopt, EN_PACKET_RECV, 0, 0, packet);
		vidx->vidframe->rf_size += packet->size;
		vidx->vidframe->rf_pac++;
		
		/* 20150115:according to the recent avcodec.h, the DTS of 
		 * a frame should keep up with the received packets */
		vidx->vidframe->rf_dts = video_packet_timestamp(packet);

		avcodec_decode_video2(vidx->codecx, 
				vidx->vidframe->frame, &got_pict, packet);
		
		vidx->vidframe->context = packet;
		vidx->vidframe->frame->opaque = vidx;
		eznotify(vidx->sysopt, EN_FRAME_DONE, 0, 
				got_pict, vidx->vidframe);
		av_free_packet(packet);

		/* 20150108: If a I-Frame has been successfully decoded,
		 * the following frames can be stored in the dual frame
		 * buffer in turn. The following decoding is named as
		 * frame group. The I-Frame is the beginner of the group */
		if (got_pict) {
			//pts = ezfrm->frame->best_effort_timestamp;
			if (vidx->vidframe->frame->key_frame) {
				vidx->fkey = 1;
			}
			return vidx->vidframe->rf_dts;	/* succeeded */
		}
	} while (video_load_packet(vidx, packet) >= 0);
	/* 20130808 this function should never fail unless the last frame was 
	 * broken in the end of stream. In that case it'll return the recent 
	 * DTS but mark it as failure in the frame buffer */
	tmp = vidx->vidframe->rf_dts;
	vidx->vidframe->rf_dts = -1;
	return tmp; 	/* this function never failed */
}

int compare_timestamp(int64_t anchor, int64_t head, int64_t tail)
{
	        if ((head -= anchor) < 0) {
			                head = -head;
					        }
		        if ((tail -= anchor) < 0) {
				                tail = -tail;
						        }
			        return (int)(head - tail); /* <0, close to head; >0 close to tail */
}
static int64_t video_decode_to(EZVID *vidx, AVPacket *packet, int64_t dtsto)
{
	int64_t	dts;

	printf("video_decode_to: decode to %" PRId64 ".\n", dtsto);
	do {
		if ((dts = video_decode_next(vidx, packet)) < 0) {
			break;
		}
		if (dts >= dtsto) {
			return dts;
		}
		video_frame_update(vidx);
	} while (video_load_packet(vidx, packet) >= 0);
	/* 20130808 Return the closest frame */
	return dts;
}

/* 20150115: the principle of this function is to decode a valided frame,
 * which means any frames started from a valided i-frame. Sometimes a packet
 * mark as i-frame would turn out to be a p-frame in decoding by ffmpeg.
 * Therefore ezthumb would keep decoding until an i-frame reached, sometimes
 * it took a quite long time. That's why we need the damage control here */
static int64_t video_decode_valided(EZVID *vidx, 
		AVPacket *packet, int64_t dtsto)
{
	int64_t	dts;
	EZFRM	*ezfrm;
	int	i;

	printf("video_decode_valided: try to decode a valided frame\n");
	/* the simplest damage control: decodes 16 frames at most */
	for (i = 0; ; i++) {
		if ((dts = video_decode_next(vidx, packet)) < 0) {
			break;
		}
		if (vidx->fkey) {
			printf("video_decode_valided: %d tried\n", i);
			return dts;
		}

		/* 20150219: set the break point by the distance between last
		   good frame and the current bad frame. There's no point to
		   keep decoding while the distance of future frames are 
		   longer than the good frame */
		if (compare_timestamp(dtsto, vidx->picframe->rf_dts, dts) <= 0) {
		if ((vidx->picframe->rf_dts > 0) &&
				((dtsto - vidx->picframe->rf_dts) < (dts - dtsto))) {
			printf("video_decode_valided: out of range %lld\n", vidx->picframe->rf_dts);
			vidx->vidframe->rf_dts = -1;
			return vidx->picframe->rf_dts;
		}
		
		/* 20150219: using video_load_packet() instead of 
		   video_keyframe_next() because in dodge encoded files
		   (middle.avi) every frame can be decoded i-frame */
		if (video_load_packet(vidx, packet) < 0) {
			break;
		}
	}
	return -1;
}

static int64_t video_decode_load(EZVID *vidx, AVPacket *packet, int64_t dtsto)
{
	if (video_load_packet(vidx, packet) < 0) {
		return -1;
	}
	return video_decode_to(vidx, packet, dtsto);
}

static int64_t video_decode_safe(EZVID *vidx, AVPacket *packet, int64_t dtsto)
{
	int64_t	dts = -1;

	do {
		if (packet->dts >= dtsto) {	/* overread */
			//return video_decode_next(vidx, packet);
			return video_decode_valided(vidx, packet, dtsto);
		}
		/* if the distance of current key frame to the snap point is 
		 * less than 2 average-key-frame-distance, ezthumb will start
		 * to decode every frames till the snap point */
		if ((video_dts_ruler(vidx, packet->dts, dtsto) < 1) &&
					GETACCUR(vidx->ses_flags)) {
			return video_decode_to(vidx, packet, dtsto);
		}

		/* working on OTF mode as default */
		if (vidx->ses_flags & EZOP_DECODE_OTF) {
			video_decode_next(vidx, packet);
			video_frame_update(vidx);
		} else {
			av_free_packet(packet);
		}
	} while (video_keyframe_next(vidx, packet) >= 0);
	return dts;
}


/* remove the key frame requirement in video_decode_to() because it causes
 * inaccurate results in short video clips. the integrity now rely on
 * the decode-on-the-fly mode */
#if 0
static int64_t video_decode_to(EZVID *vidx, AVPacket *packet, int64_t dtsto)
{
	int64_t	dts;

	/* A workaroud for B-frame error when investigating the DS9 
	 * The FFMPEG reports:
	 *   [mpeg4 @ 0x91a6a60]Invalid and inefficient vfw-avi packed
	 *   B frames detected
	 * The FFMPEG can not decode the i-frame, perhaps in lack of previous
	 * key frame information, which caused failure of later p-frames.
	 * The workabound is to decode an effect i-frame first then start
	 * to decode the proper p-frames */
	dts = video_decode_keyframe(vidx, packet);
	if ((dts < 0) || (dts >= dtsto)) {
		/* successfully decoded a frame and the packet would
		 * be freed outside this function */
		return dts;
	}

	while (video_load_packet(vidx, packet) >= 0) {
		dts = video_decode_next(vidx, packet);
		if ((dts < 0) || (dts >= dtsto)) {
			/* successfully decoded a frame and the packet would
			 * be freed outside this function */
			return dts;
		}

		eznotify(vidx->sysopt, EN_FRAME_EXCEPTION, 0, 0, vidx->frame);
		av_free_packet(packet);
	}
	return -1;
}
#endif

/* To get the file position: AVFormatContext.pb.pos
 *
 * for example:
 *
 * int64_t byteposition = pFormatCtx->pb->pos;
 *
 * To set the file position: url_seek(AVFormatContext.pb, Position, SEEK_SET);
 *
 * for example:
 *
 * url_seek(pFormatCtx->pb, 27909056, SEEK_SET);
 *
 * Don't forget to flush the buffers if you change the location while playing.
 * ff_read_frame_flush()? avcodec_flush_buffers()?
 */
/* 20130718 A new thought about seeking video file.
 * Setup the minimum requirement could be a good idea. Otherwise some times
 * ffmpeg just won't moving its packet position. The question is where the 
 * minimum DTS is. Maybe 10 second is big enough */
static int video_seeking(EZVID *vidx, int64_t dts)
{
	//int64_t	mindts;

	//av_seek_frame(vidx->formatx, vidx->vsidx, dts, AVSEEK_FLAG_BACKWARD);
	//mindts = dts - video_ms_to_dts(vidx, 60000);
	avformat_seek_file(vidx->formatx, vidx->vsidx, 
			//mindts, dts, INT64_MAX, AVSEEK_FLAG_FRAME);
			0, dts, INT64_MAX, AVSEEK_FLAG_BACKWARD);
	avcodec_flush_buffers(vidx->codecx);
	video_keyframe_credit(vidx, -1);
	/* reset the key frame decoding flag */
	video_frame_reset(vidx);
	return 0;
}

/* FIXME: It's quite confusing that FFMPEG stores sample aspect ratio
 * randomly. Sometimes it stores in the stream structure, somtimes
 * in the stream->codec */
static int video_display_ar(AVStream *stream, AVRational *dar)
{
	AVRational	*sar, tmp;
	int		ar_height;

	sar = &stream->sample_aspect_ratio;
	if (!sar->num || !sar->den) {
		sar = &stream->codec->sample_aspect_ratio;
	}
	if (dar == NULL) {
		dar = &tmp;
	}

	/* convert the sample aspect ratio to display aspect ratio */
	av_reduce(&dar->num, &dar->den, stream->codec->width * sar->num,
			stream->codec->height * sar->den, 1024 * 1024);

	/* calculate the display video height by DAR correction */
	ar_height = stream->codec->height;
	if (dar->num && dar->den) {
		ar_height = stream->codec->width * dar->den / dar->num;
	}
	EDB_PROG(("video_display_ar: SAR=%d:%d DAR=%d:%d Height=%d\n", 
			sar->num, sar->den, dar->num, dar->den, ar_height));
	return ar_height;
}

static char *video_media_video(AVStream *stream, char *buffer)
{
	AVCodec	*xcodec;
	AVRational	dar;
	char	tmp[128];

	xcodec = avcodec_find_decoder(stream->codec->codec_id);
	if (xcodec == NULL) {
		strcat(buffer, "Unknown Codec");
	} else {
		strcat(buffer, xcodec->long_name);
	}

	sprintf(tmp, ": %dx%d ", stream->codec->width, stream->codec->height);
	strcat(buffer, tmp);

	video_display_ar(stream, &dar);
	if (dar.num && dar.den) {
		sprintf(tmp, "DAR %d:%d ", dar.num, dar.den);
		strcat(buffer, tmp);
	}

	strcat(buffer, id_lookup_tail(id_pix_fmt, stream->codec->pix_fmt));
	sprintf(tmp, "  %.3f FPS ", (float) stream->r_frame_rate.num / 
			(float) stream->r_frame_rate.den);
	strcat(buffer, tmp);

	if (stream->codec->bit_rate) {
		sprintf(tmp, "%.3f kbps", 
				(float)stream->codec->bit_rate / 1000.0);
		strcat(buffer, tmp);
	}
	return buffer;
}

static char *video_media_audio(AVStream *stream, char *buffer)
{
	AVCodec	*xcodec;
	char	tmp[128];

	xcodec = avcodec_find_decoder(stream->codec->codec_id);
	if (xcodec == NULL) {
		strcat(buffer, "Unknown Codec");
	} else {
		strcat(buffer, xcodec->long_name);
	}

	sprintf(tmp, ": %s %d-CH  %s %dHz ", video_stream_language(stream),
			stream->codec->channels, 
			id_lookup_tail(id_sample_format, 
					stream->codec->sample_fmt),
			stream->codec->sample_rate);
	strcat(buffer, tmp);

	if (stream->codec->bit_rate) {
		sprintf(tmp, "%.3f kbps", 
				(float)stream->codec->bit_rate / 1000.0);
		strcat(buffer, tmp);
	}
	return buffer;
}

static char *video_media_subtitle(AVStream *stream, char *buffer)
{
	strcat(buffer, video_stream_language(stream));
	return buffer;
}

static char *video_stream_language(AVStream *stream)
{
	static	char	*nolan[] = { "(none)", "(unknown)" };
	char	*lanstr = NULL;

#ifdef	HAVE_AV_DICT_GET
	if (stream->metadata) {
		AVDictionaryEntry	*lang;
		lang = av_dict_get(stream->metadata, "language", NULL, 0);
		if (lang) {
			lanstr = lang->value;
		}
	}
#elif	defined(HAVE_AV_METADATA_GET)
	if (stream->metadata) {
		AVMetadataTag	*lang = NULL;
		lang = av_metadata_get(stream->metadata, "language", NULL, 0);
		if (lang) {
			lanstr = lang->value;
		}
	}
#else
	if (stream->language) {
		lanstr = stream->language;
	}
#endif
	if (lanstr == NULL) {
		return nolan[0];
	}
	if (lanstr[0] == (char) 0xff) {
		return nolan[1];
	}
	return lanstr;
}

static int64_t video_packet_timestamp(AVPacket *packet)
{
	int64_t	dts;

	dts = packet->dts;
	if (dts == (int64_t)AV_NOPTS_VALUE) {
		dts = packet->pts;
	}
	if (dts == (int64_t)AV_NOPTS_VALUE) {
		dts = -1;
	}
	return dts;
}

static int video_timing(EZVID *vidx, int type)
{
	static	SMM_TIME	last;
	int	acc;

	if (type == EZ_PTS_RESET) {
		smm_time_get_epoch(&last);
		memset(vidx->pts, 0, sizeof(vidx->pts));
		vidx->pidx = 0;
		return 0;
	}

	acc = smm_time_diff(&last);
	smm_time_get_epoch(&last);
	if (type == EZ_PTS_CLEAR) {
		return acc;
	}

	if (vidx->pidx < EZ_PTS_MAX) {
		vidx->pts[vidx->pidx*2]   = type;
		vidx->pts[vidx->pidx*2+1] = acc;
		vidx->pidx++;
	}
	return acc;
}

/* this function is used to convert the PTS from the video stream
 * based time to the millisecond based time. The formula is:
 *   MS = (PTS * s->time_base.num / s->time_base.den) * 1000
 * then
 *   MS =  PTS * 1000 * s->time_base.num / s->time_base.den */
static EZTIME video_dts_to_ms(EZVID *vidx, int64_t dts)
{
	return (EZTIME) av_rescale(dts * 1000, vidx->vstream->time_base.num,
			vidx->vstream->time_base.den);
}

/* this function is used to convert the timestamp from the millisecond 
 * based time to the video stream based PTS time. The formula is:
 *   PTS = (ms / 1000) * s->time_base.den / s->time_base.num
 * then
 *   PTS = ms * s->time_base.den / (s->time_base.num * 1000) */
static int64_t video_ms_to_dts(EZVID *vidx, EZTIME ms)
{
	return av_rescale((int64_t) ms, vidx->vstream->time_base.den, 
		(int64_t) vidx->vstream->time_base.num * (int64_t) 1000);
}

/* this function is used to convert the PTS from the video stream
 * based time to the default system time base (microseconds). The formula is:
 *   SYS = (PTS * s->time_base.num / s->time_base.den) * AV_TIME_BASE
 * then
 *   SYS = PTS * AV_TIME_BASE * s->time_base.num / s->time_base.den  */
/*static int64_t video_dts_to_system(EZVID *vidx, int64_t dts)
{
	return av_rescale(dts * (int64_t) AV_TIME_BASE,
			vidx->vstream->time_base.num, vidx->vstream->time_base.den);
}*/

/* this function is used to convert the timestamp from the default 
 * system time base (microsecond) to the millisecond based time. The formula:
 *   PTS = (SYS / AV_TIME_BASE) * s->time_base.den / s->time_base.num 
 * then
 *   PTS = SYS * s->time_base.den / (s->time_base.num * AV_TIME_BASE) */
static int64_t video_system_to_dts(EZVID *vidx, int64_t sysdts)
{
	return av_rescale(sysdts, vidx->vstream->time_base.den, 
			(int64_t) vidx->vstream->time_base.num * 
			(int64_t) AV_TIME_BASE);
}


/* Allocate the EZIMG structure and translate the used defined parameter
 * group, EZOPT into this structure.
 *
 * if (grid_col < 1) {
 *   discard canvas_width
 *   if (grid_row > 0) {
 *     gerenate (grid_row) separate shots
 *   } else if (tm_step > 0) {
 *     gerenate separate shots by tm_step
 *   } else {
 *     gerenate separate shots by keyframe
 *   }
 *   if ((tn_width < 1) && (tn_height < 1)) {
 *     if (tn_facto < 1) {
 *       tn_width/tn_height = orignal;
 *     } else {
 *       calculate the tn_width/tn_height by tn_facto;
 *     }
 *   } else if (tn_width < 1) {
 *     calculate tn_width by tn_height;
 *   } else {
 *     calculate tn_height by tn_width
 *   }
 * } else {
 *   if ((grid_row < 1) && (tm_step < 1)) {
 *     grid_row = 4 (default)
 *   }
 *   if (grid_row < 1) {
 *     calculate the grid_row by tm_step
 *   } else {
 *     if (tm_step > 0) {
 *       use this tm_step to generate as many as possible shots
 *     } else {
 *       calculate the tm_step by grid_col*grid_row
 *     }
 *   }
 *   if (canvas_width > 127) {
 *     calculate the gap_width/gap_height/rim_width/rim_height
 *     calculate the tn_width/tn_height
 *     ignore the tn_facto
 *   } else {
 *     if ((tn_width < 1) && (tn_height < 1)) {
 *       if (tn_facto < 1) {
 *         tn_width/tn_height = orignal;
 *       } else {
 *         calculate the tn_width/tn_height by tn_facto;
 *       }
 *     } else if (tn_width < 1) {
 *       calculate tn_width by tn_height;
 *     } else {
 *       calculate tn_height by tn_width
 *     }
 *     calculate the gap_width/gap_height/rim_width/rim_height
 *     calculate the canvas size
 *   }
 * }    
 */   
static EZIMG *image_allocate(EZVID *vidx, EZTIME rt_during, int *errcode)
{
	EZIMG	*image;
	EZOPT	*ezopt;
	char	*ftmp;
	int	size, shots;
	int	pro_col, pro_row, pro_width, pro_height, pro_facto;
	int	pro_canvas;
	int	src_width, ar_height;

	size = sizeof(EZIMG) + strlen(vidx->filename) + 128;
	if ((image = smm_alloc(size)) == NULL) {
		uperror(errcode, EZ_ERR_LOWMEM);
		return NULL;
	}
	
	ezopt = image->sysopt = vidx->sysopt;
	src_width = vidx->width;
	ar_height = vidx->ar_height;	/* 20120720 Apply the AR correction */

	image->time_from = image_cal_time_range(ezopt->time_from, rt_during);
	if (image->time_from >= rt_during) {
		image->time_from = 0;
	}
	image->time_during = image_cal_time_range(ezopt->time_to, rt_during);
	if (image->time_during > rt_during) {
		image->time_during = rt_during - image->time_from;
	} else if (image->time_during <= image->time_from) {
		image->time_during = rt_during - image->time_from;
	} else {
		image->time_during -= image->time_from;
	}

	/* initialize the user defined profile */
	pro_col    = ezopt->grid_col;
	pro_row    = ezopt->grid_row;
	pro_width  = ezopt->tn_width;
	pro_height = ezopt->tn_height;
	pro_facto  = ezopt->tn_facto;
	pro_canvas = image_user_profile(image, src_width, &pro_col, &pro_row, 
			&pro_width, &pro_height, &pro_facto);
	
	/* calculate the expected size of each screen shots.
	 * Note that the result will be overriden by canvas_width */
	if ((pro_width < 1) && (pro_height < 1)) {
		if (pro_facto < 1) {
			image->dst_width  = src_width;
			image->dst_height = ar_height;
		} else {
			image->dst_width  = ((src_width * pro_facto)/100) & ~1;
			image->dst_height = ((ar_height * pro_facto)/100) & ~1;
		}
	} else if ((pro_width > 0) && (pro_height > 0)) {
		image->dst_width  = pro_width & ~1;
		image->dst_height = pro_height & ~1;
	} else if (pro_width > 0) {
		image->dst_width  = pro_width & ~1;
		image->dst_height = (pro_width * ar_height / src_width) & ~1;
	} else {
		image->dst_width  = (pro_height * src_width / ar_height) & ~1;
		image->dst_height = pro_height;
	}
	image->dst_pixfmt = PIX_FMT_RGB24;

	/* calculate the canvas, the screenshots, timestep and the gaps */
	if (pro_col < 1) {	/* user wants separated screen shots */
		image->grid_col  = pro_col;
		image->grid_row  = pro_row;
		image->time_step = ezopt->tm_step;
		if ((image->grid_row < 1) && (image->time_step > 0)) {
			image->grid_row = image_cal_shots(image->time_during,
					image->time_step, ezopt->flags);
		} else if ((image->grid_row > 0) && (image->time_step < 1)) {
			image->time_step = image_cal_timestep(
					image->time_during,
					image->grid_row, ezopt->flags);

		}
	} else {
		image->grid_col = pro_col;
		if ((pro_row < 1) && (ezopt->tm_step < 1)) {
			/* 20120718 fixed the bug that "-g 4" crashed the 
			 * program which introduced by the profile system */
			image->grid_row = pro_row = 4;	/* make it default */
		}
		if (pro_row < 1) {
			shots = image_cal_shots(image->time_during, 
					ezopt->tm_step, ezopt->flags);
			image->grid_row  = (shots + image->grid_col - 1) /
					image->grid_col;
			image->time_step = ezopt->tm_step;
		} else if (ezopt->tm_step > 0) {
			image->grid_row  = pro_row;
			image->time_step = ezopt->tm_step;
		} else {
			image->grid_row  = pro_row;
			image->time_step = image_cal_timestep(
					image->time_during,
					image->grid_col * image->grid_row, 
					ezopt->flags);
		}

		if ((ezopt->canvas_width > 63) || (pro_canvas > 63)) {
			/* if the canvas width is specified, it overrides 
			 * tn_width, tn_height and tn_facto */
			if (ezopt->canvas_width > 63) {
				image->canvas_width = ezopt->canvas_width;
			} else {
				image->canvas_width = pro_canvas;
			}
			image->canvas_width &= ~1;

			/* it's the reference width for getting the gap size */
			size = ezopt->canvas_width / pro_col;
			image->gap_width = image_cal_ratio(ezopt->grid_gap_w, 
					size);
			image->rim_width = image_cal_ratio(ezopt->grid_rim_w, 
					size);

			/* it's the reference height for getting the gap size*/
			size = size * ar_height / src_width;
			image->gap_height = image_cal_ratio(ezopt->grid_gap_h, 
					size);
			image->rim_height = image_cal_ratio(ezopt->grid_rim_h, 
					size);

			/* now calculate the actual shot width and height */
			image->dst_width = (image->canvas_width - 
				image->rim_width * 2 - image->gap_width * 
				(pro_col - 1)) / pro_col;
			/* the dst_height is a little bit tricky. We would
			 * honor the user specified proportion. 
			 * See FTest#036 */
			if ((pro_width > 0) && (pro_height > 0)) {
				image->dst_height = image->dst_width * 
					pro_height / pro_width;
			} else {
				image->dst_height = image->dst_width * 
					ar_height / src_width;
			}
			/* adjust the dimention of shots to even boundry */
			image->dst_width  = image->dst_width & ~1;
			image->dst_height = image->dst_height & ~1;
		} else {
			/* Otherwise the canvas_width will be calculated by 
			 * those actual dimentions */
			image->gap_width = image_cal_ratio(ezopt->grid_gap_w, 
					image->dst_width);
			image->rim_width = image_cal_ratio(ezopt->grid_rim_w, 
					image->dst_width);
			image->gap_height = image_cal_ratio(ezopt->grid_gap_h, 
					image->dst_width);
			image->rim_height = image_cal_ratio(ezopt->grid_rim_h, 
					image->dst_width);
			image->canvas_width = (image->rim_width * 2 + 
				image->gap_width * (pro_col - 1) +
				image->dst_width * pro_col + 1) & ~1;
		}
		image->canvas_height = image->rim_height * 2 + 
			image->gap_height * (image->grid_row - 1) +
			image->dst_height * image->grid_row ;
	}

	/* calculate the total shots and allocate the proposal PTS list */
	if (image->grid_col < 1) {
		image->shots = image->grid_row;
	} else {
		image->shots = image->grid_col * image->grid_row;
	}
	if (image->shots) {
		size = (image->shots + 1) * sizeof(int64_t) * 2;
	}

	/* font and size define */
	//gdFTUseFontConfig(1);	/* enable fontconfig patterns */
	/* 20130801 I've got the idea that the font height should be
	 * standardalized by file name and maximum english glyph array */
	if ((ftmp = csc_strcpy_alloc(vidx->filename, 16)) == NULL) {
		image_free(image);
		uperror(errcode, EZ_ERR_LOWMEM);
		return NULL;
	}
	strcat(ftmp, "bqBQ");
	size = image_gdcanvas_strlen(image, image->sysopt->mi_size, ftmp);
	/* we only need the font height plus the gap size */
	image->mift_height = EZ_LO_WORD(size) + EZ_TEXT_MINFO_GAP;
	smm_free(ftmp);

	/* enlarge the canvas height to include the media information */
	if ((ezopt->flags & EZOP_INFO) == 0) {
		image->canvas_minfo = 0;
	} else if (image->canvas_height > 0) {
		/* One rimedge plus media infos */
		/* 20130719 Remove the unknown streams in display */
		image->canvas_minfo = image->mift_height * 
				(vidx->ezstream + 2) + EZ_TEXT_INSET_GAP;
		image->canvas_height += image->canvas_minfo;
		/* Plus the status line: font height + INSET_GAP */
		image->canvas_height += image->mift_height + 
				EZ_TEXT_INSET_GAP;
	}
	image->canvas_height = (image->canvas_height + 1) & ~1;

	/* create a GD device for handling the screen shots */
	image->gdframe = gdImageCreateTrueColor(image->dst_width, 
			image->dst_height);
	if (image->gdframe == NULL) {
		image_free(image);
		uperror(errcode, EZ_ERR_LOWMEM);
		return NULL;
	}

	if ((image->grid_col > 0) && !image_cal_gif_animix(image->sysopt)) {
		/* only create the GD device for handling the canvas 
		 * when canvas is required */
		image->gdcanvas = gdImageCreateTrueColor(image->canvas_width,
				image->canvas_height);
		if (image->gdcanvas == NULL) {
			image_free(image);
			uperror(errcode, EZ_ERR_LOWMEM);
			return NULL;
		}

		/* define the colors that are used in the canvas
		 * and setup the background color */
		image->color_canvas = gdImageColorResolveAlpha(
				image->gdcanvas,
				ezopt->canvas_color[0], 
				ezopt->canvas_color[1],
				ezopt->canvas_color[2], 
				ezopt->canvas_color[3]);
		image->color_shadow = gdImageColorResolveAlpha(
				image->gdcanvas,
				ezopt->shadow_color[0], 
				ezopt->shadow_color[1],
				ezopt->shadow_color[2], 
				ezopt->shadow_color[3]);
		image->color_minfo = gdImageColorResolve(
				image->gdcanvas,
				ezopt->mi_color[0], 
				ezopt->mi_color[1],
				ezopt->mi_color[2]);

		/* setup the background color */
		gdImageFilledRectangle(image->gdcanvas, 0, 0, 
				image->canvas_width  - 1, 
				image->canvas_height - 1, 
				image->color_canvas);
		/* load the background picture */
		image_gdcanvas_background(image);
	}

	/* define the colors used in the screen shots */
	image->color_edge = gdImageColorResolve(image->gdframe,
			ezopt->edge_color[0], ezopt->edge_color[1],
			ezopt->edge_color[2]);
	image->color_inset = gdImageColorResolve(image->gdframe,
			ezopt->ins_color[0], ezopt->ins_color[1],
			ezopt->ins_color[2]);
	image->color_inshadow = gdImageColorResolveAlpha(image->gdframe,
			ezopt->its_color[0], ezopt->its_color[1],
			ezopt->its_color[2], ezopt->its_color[3]);

	uperror(errcode, EZ_ERR_NONE);

	/* FIXME: this is really a bad idea that hacking the function by the 
	 * availability of errcode. Otherwise the debug info will be printed
	 * twice, one from the video_duration() */
	if (errcode) {
		eznotify(vidx->sysopt, EN_IMAGE_CREATED, 0, 0, image);
	}
	return image;	
}

static int image_free(EZIMG *image)
{
	if (image->gdcanvas) {
		gdImageDestroy(image->gdcanvas);
	}
	if (image->gdframe) {
		gdImageDestroy(image->gdframe);
	}
	smm_free(image);
	return EZ_ERR_NONE;
}

static int image_user_profile(EZIMG *image, int src_width, int *col, int *row,
		int *width, int *height, int *facto)
{
	int	shots;

	shots = ezopt_profile_sampling(image->sysopt, 
			(int)(image->time_during / 1000), col, row);
	if (shots > 0) {
		ezopt_profile_sampled(image->sysopt, 
				src_width, shots, col, row);
	}
	return ezopt_profile_zooming(image->sysopt,
			src_width, width, height, facto);
}

static int image_font_test(EZIMG *image, char *filename)
{
	gdFont	*font;
	char	*s="1234567890ABCDEFGHIJKLMNOPQRSTUabcdefghijklmnopqrstuvwxyz";
	int	y, brect[8];

	if (image->gdcanvas == NULL) {
		return -1;
	}

	y = 20;

	font = image_fontset(EZ_FONT_TINY);
	gdImageString(image->gdcanvas, font, 20, y, (unsigned char *) s, 
			image->color_minfo);
	y += font->h + 2;
	
	font = image_fontset(EZ_FONT_SMALL);
	gdImageString(image->gdcanvas, font, 20, y, (unsigned char *) s, 
			image->color_minfo);
	y += font->h + 2;

	font = image_fontset(EZ_FONT_MEDIUM);
	gdImageString(image->gdcanvas, font, 20, y, (unsigned char *) s, 
			image->color_minfo);
	y += font->h + 2;

	font = image_fontset(EZ_FONT_LARGE);
	gdImageString(image->gdcanvas, font, 20, y, (unsigned char *) s, 
			image->color_minfo);
	y += font->h + 2;

	font = image_fontset(EZ_FONT_GIANT);
	gdImageString(image->gdcanvas, font, 20, y, (unsigned char *) s, 
			image->color_minfo);
	y += font->h + 2;

	if (image->sysopt->mi_font) {
		double	size;

		y += 40;
		size = (double) (EZ_FONT_TINY);
		gdImageStringFT(image->gdcanvas, brect, image->color_minfo,
				image->sysopt->mi_font, size, 0, 20, y, s);
		y += brect[3] - brect[7];

		size = (double) (EZ_FONT_SMALL);
		gdImageStringFT(image->gdcanvas, brect, image->color_minfo,
				image->sysopt->mi_font, size, 0, 20, y, s);
		y += brect[3] - brect[7];

		size = (double) (EZ_FONT_MEDIUM);
		gdImageStringFT(image->gdcanvas, brect, image->color_minfo,
				image->sysopt->mi_font, size, 0, 20, y, s);
		y += brect[3] - brect[7];

		size = (double) (EZ_FONT_LARGE);
		gdImageStringFT(image->gdcanvas, brect, image->color_minfo,
				image->sysopt->mi_font, size, 0, 20, y, s);
		y += brect[3] - brect[7];

		size = (double) (EZ_FONT_GIANT);
		gdImageStringFT(image->gdcanvas, brect, image->color_minfo,
				image->sysopt->mi_font, size, 0, 20, y, s);
		y += brect[3] - brect[7];
	}

	image_gdcanvas_save(image, filename);
	return 0;
}

/* This function is used to fill the GD image device with the content of 
 * the RGB frame buffer. It will flush the last image */
static int image_gdframe_update(EZIMG *image, AVFrame *frame)
{
	unsigned char	*src;
	int	x, y;

	src = frame->data[0];
	for (y = 0; y < image->dst_height; y++) {
		for (x = 0; x < image->dst_width * 3; x += 3) {
			gdImageSetPixel(image->gdframe, x / 3, y,
					gdImageColorResolve(image->gdframe,
						src[x], src[x+1], src[x+2]));
		}
		src += image->dst_width * 3;
	}
	return EZ_ERR_NONE;
}

/* This function is used to write a timestamp into the screen shot */
static int image_gdframe_timestamp(EZIMG *image, char *timestamp)
{
	int	x, y, ts_width, ts_height;
	
	if (*timestamp == 0) {
		return EZ_ERR_NONE;
	}

	/* calculate the rectangle size of the string */
	x = image_gdframe_strlen(image, image->sysopt->ins_size, timestamp);
	ts_width  = EZ_HI_WORD(x);
	ts_height = EZ_LO_WORD(x);

	switch (image->sysopt->ins_position & EZ_POS_MASK) {
	case EZ_POS_LEFTTOP:
		x = EZ_TEXT_INSET_GAP;
		y = EZ_TEXT_INSET_GAP;
		break;
	case EZ_POS_LEFTCENTER:
		x = EZ_TEXT_INSET_GAP;
		y = (image->dst_height - ts_height) / 2;
		break;
	case EZ_POS_LEFTBOTTOM:
		x = EZ_TEXT_INSET_GAP;
		y = image->dst_height - ts_height - EZ_TEXT_INSET_GAP;
		break;
	case EZ_POS_MIDTOP:
		x = (image->dst_width - ts_width) / 2;
		y = EZ_TEXT_INSET_GAP;
		break;
	case EZ_POS_MIDCENTER:
		x = (image->dst_width - ts_width) / 2;
		y = (image->dst_height - ts_height) / 2;
		break;
	case EZ_POS_MIDBOTTOM:
		x = (image->dst_width - ts_width) / 2;
		y = image->dst_height - ts_height - EZ_TEXT_INSET_GAP;
		break;
	case EZ_POS_RIGHTTOP:
		x = image->dst_width - ts_width - EZ_TEXT_INSET_GAP;
		y = EZ_TEXT_INSET_GAP;
		break;
	case EZ_POS_RIGHTCENTER:
		x = image->dst_width - ts_width - EZ_TEXT_INSET_GAP;
		y = (image->dst_height - ts_height) / 2;
		break;
	case EZ_POS_RIGHTBOTTOM:
	default:
		x = image->dst_width - ts_width - EZ_TEXT_INSET_GAP;
		y = image->dst_height - ts_height - EZ_TEXT_INSET_GAP;
		break;
	}
	if (image->sysopt->ins_shadow) {
		image_gdframe_puts(image, image->sysopt->ins_size,
				x + image->sysopt->ins_shadow,
				y + image->sysopt->ins_shadow,
				image->color_inshadow, timestamp);
	}
	image_gdframe_puts(image, image->sysopt->ins_size,
			x, y, image->color_inset, timestamp);
	return EZ_ERR_NONE;
}

/* This functin is used to save the screen shot from the GD image device.
 * Each shots is saved as an individual file. */
static int image_gdframe_save(EZIMG *image, char *filename, int idx)
{
	FILE	*fout;

	if ((fout = image_create_file(image, filename, idx)) == NULL) {
		return EZ_ERR_FILE;
	}

	if (!strcmp(image->sysopt->img_format, "png")) {
		gdImagePng(image->gdframe, fout);
	} else if (!strcmp(image->sysopt->img_format, "gif")) {
		gdImageGif(image->gdframe, fout);
	} else {
		gdImageJpeg(image->gdframe, fout, image->sysopt->img_quality);
	}
	fclose(fout);
	return EZ_ERR_NONE;
}

static int image_gdframe_strlen(EZIMG *image, int fsize, char *s)
{
	gdFont	*font;
	int	brect[8];

	fsize = image_fontsize(fsize, image->dst_width);
	if (image->sysopt->ins_font == NULL) {
		font = image_fontset(fsize);
	} else if (gdImageStringFT(NULL, brect,	0, image->sysopt->ins_font, 
				(double) fsize, 0, 0, 0, s)) {
		font = image_fontset(fsize);
	} else {
		return EZ_MK_WORD(brect[2] - brect[6], brect[3] - brect[7]);
	}
	return EZ_MK_WORD(font->w * strlen(s), font->h);
}

static int image_gdframe_puts(EZIMG *image, int fsize, 
		int x, int y, int c, char *s)
{
	int	brect[8];

	EDB_FUNC(("image_gdframe_puts(%dx%dx%d): %s (0x%x)\n", 
				x, y, fsize, s, c));
	fsize = image_fontsize(fsize, image->dst_width);
	if (image->sysopt->ins_font == NULL) {
		gdImageString(image->gdframe, image_fontset(fsize),
				x, y, (unsigned char *) s, c);
	} else if (gdImageStringFT(NULL, brect, 0, image->sysopt->ins_font,
				(double) fsize, 0, 0, 0, s)) {
		gdImageString(image->gdframe, image_fontset(fsize),
				x, y, (unsigned char *) s, c);
	} else {
		gdImageStringFT(image->gdframe, brect, c, 
				image->sysopt->ins_font, 
				(double) fsize, 0, 
				x - brect[6], y - brect[7], s);
	}
	return 0;
}

/* This function is used to save the whole canvas */
static int image_gdcanvas_save(EZIMG *image, char *filename)
{
	FILE	*fout;

	if ((fout = image_create_file(image, filename, -1)) == NULL) {
		return EZ_ERR_FILE;
	}

	if (image->sysopt->flags & EZOP_TRANSPARENT) {
		gdImageColorTransparent(image->gdcanvas, image->color_canvas);
	}
	if (!strcmp(image->sysopt->img_format, "png")) {
		gdImagePng(image->gdcanvas, fout);
	} else if (!strcmp(image->sysopt->img_format, "gif")) {
		gdImageGif(image->gdcanvas, fout);
	} else {
		gdImageJpeg(image->gdcanvas, fout, image->sysopt->img_quality);
	}
	fclose(fout);
	return EZ_ERR_NONE;
}

/* This function is used to paste a screen shot into the canvas */
static int image_gdcanvas_update(EZIMG *image, int idx)
{
	int	col, row, i, x, y;

	/* draw the edge inside the screen shot */
	for (i = 0; i < image->sysopt->edge_width; i++) {
		gdImageRectangle(image->gdframe, i, i, 
				image->dst_width - i - 1,
				image->dst_height - i - 1, 
				image->color_edge);
	}

	row = idx / image->grid_col;
	col = idx % image->grid_col;

	x = image->rim_width + (image->dst_width + image->gap_width) * col;
	y = image->canvas_minfo + image->rim_height + 
		(image->dst_height + image->gap_height) * row;

	/* draw a shadow of the screen shot */
	if (image->sysopt->shadow_width) {
		gdImageFilledRectangle(image->gdcanvas, 
			x + image->sysopt->shadow_width,
			y + image->sysopt->shadow_width, 
			x + image->sysopt->shadow_width + image->dst_width - 1,
			y + image->sysopt->shadow_width + image->dst_height -1,
			image->color_shadow);
	}
	/*gdImageCopy(image->gdcanvas, image->gdframe, x, y, 0, 0, 
			image->dst_width, image->dst_height);*/
	image_copy(image->gdcanvas, image->gdframe, x, y, 0, 0);
	return EZ_ERR_NONE;
}

/* This function is used to print a string in the canvas. The 'row' and
 * 'off' specify the start coordinate of the string. If the 'row' is -1,
 * then it indicates to display in the status line, which is in the bottom
 * of the canvas. Note that the 'row' is starting from 0,1,2,... as a grid 
 * coordinate. However the 'off' is the offset count in pixel. 
 * It returns the string's length in pixel. */
static int image_gdcanvas_print(EZIMG *image, int row, int off, char *s)
{
	int	x, y, ts_width;
	
	/* 20130719 copy the display to console and avoid NULL pointer */
	EDB_SHOW((s));
	EDB_SHOW(("\n"));
	if (image == NULL) {
		return 0;
	}

	/* calculate the rectangle size of the string. The height of
	 * the string is fixed to the maximum size */
	x = image_gdcanvas_strlen(image, image->sysopt->mi_size, s);
	ts_width  = EZ_HI_WORD(x);

	/* we only concern the left, right and center alignment */
	if (row < 0) {
		x = image->sysopt->st_position;
		y = image->canvas_height - image->mift_height - 
				image->rim_height;
	} else {
		x = image->sysopt->mi_position;
		y = image->rim_height + image->mift_height * row;
	}
	switch (x & EZ_POS_MASK) {
	case EZ_POS_LEFTTOP:
	case EZ_POS_LEFTCENTER:
	case EZ_POS_LEFTBOTTOM:
		x = image->rim_width + off;
		break;
	case EZ_POS_MIDTOP:
	case EZ_POS_MIDCENTER:
	case EZ_POS_MIDBOTTOM:
		x = image->canvas_width - image->rim_width * 2 - off;
		x = (x - ts_width) / 2 + image->rim_width + off;
		break;
	case EZ_POS_RIGHTTOP:
	case EZ_POS_RIGHTCENTER:
	case EZ_POS_RIGHTBOTTOM:
	default:
		x = image->canvas_width - image->rim_width - off - ts_width;
		break;
	}
	if (image->sysopt->mi_shadow) {
		image_gdcanvas_puts(image, image->sysopt->mi_size, 
				x + image->sysopt->mi_shadow, 
				y + image->sysopt->mi_shadow, 
				image->color_shadow, s);
	}
	image_gdcanvas_puts(image, image->sysopt->mi_size, x, y, 
			image->color_minfo, s);
	return ts_width;
}

static int image_gdcanvas_strlen(EZIMG *image, int fsize, char *s)
{
	gdFont	*font;
	int	ref, brect[8];

	ref = image->grid_col ? image->canvas_width / image->grid_col :
		image->canvas_width;
	fsize = image_fontsize(fsize, ref);
	if (image->sysopt->mi_font == NULL) {
		font = image_fontset(fsize);
	} else if (gdImageStringFT(NULL, brect,	0, image->sysopt->mi_font, 
				(double) fsize, 0, 0, 0, s)) {
		font = image_fontset(fsize);
	} else {
		return EZ_MK_WORD(brect[2] - brect[6], brect[3] - brect[7]);
	}
	return EZ_MK_WORD(font->w * strlen(s), font->h);
}

static int image_gdcanvas_puts(EZIMG *image, int fsize, 
		int x, int y, int c, char *s)
{
	int	ref, brect[8];

	ref = image->grid_col ? image->canvas_width / image->grid_col :
		image->canvas_width;
	fsize = image_fontsize(fsize, ref);
	if (image->sysopt->mi_font == NULL) {
		gdImageString(image->gdcanvas, image_fontset(fsize),
				x, y, (unsigned char *) s, c);
	} else if (gdImageStringFT(NULL, brect, 0, image->sysopt->mi_font,
				(double) fsize, 0, 0, 0, s)) {
		gdImageString(image->gdcanvas, image_fontset(fsize),
				x, y, (unsigned char *) s, c);
	} else {
		gdImageStringFT(image->gdcanvas, brect, c, 
				image->sysopt->mi_font, 
				(double) fsize, 0, 
				x - brect[6], y - brect[7], s); 
	}
	return EZ_ERR_NONE;
}

static int image_gdcanvas_background(EZIMG *image)
{
	gdImage	*bgim;
	FILE	*fin;
	int	twid, thei, dx, dy;

	if (image->sysopt->background == NULL) {
		return EZ_ERR_NONE;
	}
	if ((fin = fopen(image->sysopt->background, "rb")) == NULL) {
		perror(image->sysopt->background);
		return EZ_ERR_FILE;
	}

	/* load the background picture with known format */
	bgim = gdImageCreateFromPng(fin);
	if (bgim == NULL) {
		rewind(fin);
		bgim = gdImageCreateFromJpeg(fin);
	}

	fclose(fin);
	if (bgim == NULL) {
		return EZ_ERR_IMG_FORMAT;
	}

	switch (image->sysopt->bg_position & ~EZ_POS_MASK) {
	case EZ_POS_STRETCH:
		twid = image->canvas_width;
		thei = image->canvas_height;
		break;
	case EZ_POS_ENLARGE_X:
		twid = image->canvas_width;
		thei = twid * gdImageSY(bgim) / gdImageSX(bgim);
		break;
	case EZ_POS_ENLARGE_Y:
		thei = image->canvas_height;
		twid = thei * gdImageSX(bgim) / gdImageSY(bgim);
		break;
	case EZ_POS_STRETCH_X:
		twid = image->canvas_width;
		thei = gdImageSY(bgim);
		break;
	case EZ_POS_STRETCH_Y:
		twid = gdImageSX(bgim);
		thei = image->canvas_height;
		break;
	default:
		twid = gdImageSX(bgim);
		thei = gdImageSY(bgim);
		break;
	}

	switch (image->sysopt->bg_position & EZ_POS_MASK) {
	case EZ_POS_LEFTTOP:
		dx = 0;
		dy = 0;
		break;
	case EZ_POS_LEFTCENTER:
		dx = 0;
		dy = (image->canvas_height - thei) / 2;
		break;
	case EZ_POS_LEFTBOTTOM:
		dx = 0;
		dy = image->canvas_height - thei;
		break;
	case EZ_POS_MIDTOP:
		dx = (image->canvas_width - twid) / 2;
		dy = 0;
		break;
	case EZ_POS_MIDCENTER:
		dx = (image->canvas_width - twid) / 2;
		dy = (image->canvas_height - thei) / 2;
		break;
	case EZ_POS_MIDBOTTOM:
		dx = (image->canvas_width - twid) / 2;
		dy = image->canvas_height - thei;
		break;
	case EZ_POS_RIGHTTOP:
		dx = image->canvas_width - twid;
		dy = 0;
		break;
	case EZ_POS_RIGHTCENTER:
		dx = image->canvas_width - twid;
		dy = (image->canvas_height - thei) / 2;
		break;
	case EZ_POS_RIGHTBOTTOM:
		dx = image->canvas_width - twid;
		dy = image->canvas_height - thei;
		break;
	default:	/* EZ_POS_TILE */
		for (dy = 0; dy < image->canvas_height; dy += thei) {
			for (dx = 0; dx < image->canvas_width; dx += twid) {
				image_copy(image->gdcanvas, bgim, 
						dx, dy, twid, thei);
			}
		}
		gdImageDestroy(bgim);
		return 0;
	}

	image_copy(image->gdcanvas, bgim, dx, dy, twid, thei);
	gdImageDestroy(bgim);
	return 0;
}

#ifdef	HAVE_GD_IMAGE_GIFANIMATION
static FILE *image_gif_anim_open(EZIMG *image, char *filename)
{
	gdImage	*imgif;
	FILE	*fout;

	if ((fout = image_create_file(image, filename, -1)) == NULL) {
		return NULL;
	}

	imgif = gdImageCreate(image->dst_width, image->dst_height);
	if (imgif == NULL) {
		fclose(fout);
		return NULL;
	}

	/* set the background and the default palette */
	gdImageColorAllocate(imgif, 255, 255, 255);
	/* write the GIF head, frame and the global palette,
	 * but we don't use it */
	gdImageGifAnimBegin(imgif, fout, 1, 0);

	gdImageDestroy(imgif);
	return fout;
}

static int image_gif_anim_add(EZIMG *image, FILE *fout, int interval)
{
	gdImage	*imgif;

	/*imgif = gdImageCreate(image->dst_width, image->dst_height);
	gdImagePaletteCopy (imgif, imlast);
	gdImageColorAllocate(imgif, 255, 255, 255);*/
	//trans = gdImageColorAllocate(imlast, 1, 1, 1);
	//gdImageColorTransparent (imgif, trans);
	//gdImageCopy(imgif, image->gdframe, 0, 0, 0, 0,
	//		image->dst_width, image->dst_height);
	imgif = gdImageCreatePaletteFromTrueColor(image->gdframe, 1, 256);
	gdImageGifAnimAdd(imgif, fout, 1, 0, 0, 	/* local palette */
			interval, gdDisposalNone, NULL);

	gdImageDestroy(imgif);
	return 0;
}

static int image_gif_anim_close(EZIMG *image, FILE *fout)
{
	(void) image;		/* stop the gcc warning */
	gdImageGifAnimEnd(fout);
	fclose(fout);
	return 0;
}
#endif 	/* HAVE_GD_IMAGE_GIFANIMATION */

static FILE *image_create_file(EZIMG *image, char *filename, int idx)
{
	FILE	*fp;

	if (ezopt_thumb_name(image->sysopt, image->filename, filename, idx) 
			== EZ_THUMB_SKIP) {
		errno = EEXIST;
		EDB_WARN(("%s: skipped.\n", image->filename));
		return NULL;	/* skip the existed files */
	}

	if ((fp = fopen(image->filename, "wb")) == NULL) {
		EDB_ERROR(("%s: failed to create\n", image->filename));
	}
	return fp;
}

static int image_cal_ratio(int ratio, int refsize)
{
	if (ratio & EZ_RATIO_OFF) {
		return (ratio & ~EZ_RATIO_OFF) * refsize / 100;
	} else if (ratio > 0) {
		return ratio;
	}
	return 0;
}

static EZTIME image_cal_time_range(int ratio, EZTIME reftime)
{
	if (ratio & EZ_RATIO_OFF) {
		return ((EZTIME)(ratio & ~EZ_RATIO_OFF)) * reftime / 100;
	} else if (ratio > 0) {
		return (EZTIME) ratio;
	}
	return 0;
}

static int image_cal_shots(EZTIME duration, EZTIME tmstep, int mode)
{
	int	shots;

	shots = (int)(duration / tmstep - 1);
	if (mode & EZOP_FFRAME) {
		shots++;
	}
	if (mode & EZOP_LFRAME) {
		shots++;
	}
	return shots;
}

static EZTIME image_cal_timestep(EZTIME duration, int shots, int mode)
{
	if (mode & EZOP_FFRAME) {
		shots--;
	}
	if (mode & EZOP_LFRAME) {
		shots--;
	}
	return duration / (shots + 1);
}

static int image_cal_gif_animix(EZOPT *ezopt)
{
	if (strcmp(ezopt->img_format, "gif")) {
		return 0;
	}
	if (ezopt->img_quality > 0) {
		return ezopt->img_quality / 10;
	}
	return 0;
}

static gdFont *image_fontset(int fsize)
{
	if (fsize <= EZ_FONT_TINY) {
		return gdFontGetTiny();
	} else if (fsize <= EZ_FONT_SMALL) {
		return gdFontGetSmall();
	} else if (fsize <= EZ_FONT_MEDIUM) {
		return gdFontGetMediumBold();
	} else if (fsize <= EZ_FONT_LARGE) {
		return gdFontGetLarge();
	}
	return gdFontGetGiant();
}

static int image_fontsize(int fsize, int refsize)
{
	if (fsize == EZ_FONT_AUTO) {
		if (refsize < 160) {
			fsize = EZ_FONT_TINY;
		} else if (refsize <  240) {
			fsize = EZ_FONT_SMALL;
		} else if (refsize < 320) {
			fsize = EZ_FONT_MEDIUM;
		} else if (refsize < 640) {
			fsize = EZ_FONT_LARGE;
		} else {
			fsize = EZ_FONT_LARGE;
		}
	}
	return fsize;
}

static int image_copy(gdImage *dst, gdImage *src, int x, int y, 
		int wid, int hei)
{
	wid = (wid < 1) ? gdImageSX(src) : wid;
	hei = (hei < 1) ? gdImageSY(src) : hei;
	if ((gdImageSX(src) == wid) && (gdImageSY(src) == hei)) {
		gdImageCopy(dst, src, x, y, 0, 0, wid, hei);
	} else {
		gdImageCopyResampled(dst, src, x, y, 0, 0, wid, hei, 
				gdImageSX(src), gdImageSY(src));
	}
	return 0;
}


/****************************************************************************
 * Filename process
 ****************************************************************************/
static int ezopt_thumb_name(EZOPT *ezopt, char *buf, char *fname, int idx)
{
	char	tmp[128], *inbuf = NULL;
	int	i, rc = 0;

	/* special case for testing purpose
	 * If the output path has the same suffix to the specified suffix,
	 * it will NOT be treated as a path but the output file.
	 * For example, if the 'img_format' was defined as "jpg", and the
	 * 'pathout' is something like "abc.jpg", the 'pathout' actually
	 * is the output file. But if 'pathout' is "abc.jpg/", then it's
	 * still a path */
	if (!csc_cmp_file_extname(ezopt->pathout, ezopt->img_format)) {
		if (buf) {
			strcpy(buf, ezopt->pathout);
		}
		return EZ_THUMB_VACANT;	/* debug mode always vacant */
	}

	if (buf == NULL) {
		buf = inbuf = smm_alloc(strlen(fname) + 128 + 32);
		if (buf == NULL) {
			return rc;
		}
	}

	if (idx < 0) {
		sprintf(tmp, "%s.", ezopt->suffix);
	} else {
		sprintf(tmp, "%03d.", idx);
	}
	strcat(tmp, ezopt->img_format);
	ezopt_name_build(ezopt->pathout, fname, buf, tmp);

	for (i = 1; i < 256; i++) {
		if (smm_fstat(buf) != SMM_ERR_NONE) {
			if (i == 1) {
				rc = EZ_THUMB_VACANT;	/* file not existed */
			} else {
				rc = EZ_THUMB_COPIABLE;	/* copying file  */
			}
			break;	/* file not existed */
		} else if (ezopt->flags & EZOP_THUMB_OVERRIDE) {
			rc = EZ_THUMB_OVERRIDE;	/* override it */
			break;
		} else if ((ezopt->flags & EZOP_THUMB_COPY) == 0) {
			rc = EZ_THUMB_SKIP;	/* skip the existed files */
			break;
		}
		
		if (idx < 0) {
			sprintf(tmp, "%s.%d.", ezopt->suffix, i);
		} else {
			sprintf(tmp, "%03d.%d.", idx, i);
		}
		strcat(tmp, ezopt->img_format);
		ezopt_name_build(ezopt->pathout, fname, buf, tmp);
	}
	if (i == 256) {
		rc = EZ_THUMB_OVERCOPY;	/* override the last one */
	}
	if (inbuf) {
		smm_free(inbuf);
	}
	EDB_FUNC(("ezopt_thumb_name: %d\n", rc));
	return rc;
}

static char *ezopt_name_build(char *path, char *fname, char *buf, char *sfx)
{
	char	*p;

	if (!path || !*path) {
		strcpy(buf, fname);
	} else {
		strcpy(buf, path);
		if (!csc_isdelim(SMM_PATH_DELIM, buf[strlen(buf)-1])) {
			strcat(buf, SMM_DEF_DELIM);
		}
		strcat(buf, csc_path_basename(fname, NULL, 0));
	}
	if ((p = strrchr(buf, '.')) != NULL) {
		*p = 0;
	}
	strcat(buf, sfx);
	return buf;
}


/****************************************************************************
 * Message/Notification functions
 ****************************************************************************/
static int ezdefault(EZOPT *ezopt, int event, 
		long param, long opt, void *block)
{
	struct	ezntf	*myntf;
	AVStream	*stream;
	EZVID	*vidx;
	EZIMG	*image;
	char	buf[256];
	char	*seekm[] = { "SU", "SN", "SF", "SB" };	/* seekable codes */
	char	*dmod[] = { "AU", "QS", "FS", "HD" };	/* duration mode */
	int	i, n;

	switch (event) {
	case EZ_ERR_LOWMEM:
		EDB_ERROR(("%s: low memory [%ld]\n", (char*) block, param));
		return event;
	case EZ_ERR_FORMAT:
		EDB_ERROR(("%s: unknown format.\n", (char*) block));
		return event;
	case EZ_ERR_STREAM:
		EDB_ERROR(("%s: no stream info found.\n", (char*) block));
		return event;
	case EZ_ERR_VIDEOSTREAM:
		EDB_ERROR(("%s: no video stream found.\n", (char*) block));
		return event;
	case EZ_ERR_CODEC_FAIL:
		EDB_ERROR(("Could not open codec! %ld\n", param));
		return event;
	case EZ_ERR_FILE:
		EDB_ERROR(("%s: file not found.\n", (char*) block));
		return event;
	}

	if (ezopt == NULL) {
		EDB_ERROR(("Unhandled event [0x%x]\n", event));
		return event;
	}

	switch (event) {
	case EN_FILE_OPEN:
		vidx = block;
		if (ezopt->flags & EZOP_CLI_INSIDE) {
			/* This is the ffmpeg function so it must run before
			 * disabling the av_log */
			i = av_log_get_level();
			av_log_set_level(AV_LOG_INFO);
#ifdef	HAVE_AV_DUMP_FORMAT
			av_dump_format(vidx->formatx, 0, vidx->filename, 0);
#elif	defined(HAVE_DUMP_FORMAT)
			dump_format(vidx->formatx, 0, vidx->filename, 0);
#else
#error	None of av_dump_format() or dump_format() defined!
#endif
			av_log_set_level(i);
		}
		EDB_INFO(("%s: file open (%ld ms)\n", vidx->filename, opt));
		break;
	case EN_MEDIA_OPEN:
		vidx = block;
		if (EZOP_DEBUG(vidx->ses_flags) >= SLOG_LVL_INFO) {
			dump_format_context(vidx->formatx);
			dump_metadata(vidx->formatx->metadata);
		}
		if (vidx->ses_flags & EZOP_CLI_INSIDE) {	// FIXME
			dump_duration(vidx, (int) opt);
		}
		if (vidx->ses_flags & EZOP_CLI_INFO) {
			dump_media_brief(vidx);
		}
		break;
	case EN_IMAGE_CREATED:
		if (EZOP_DEBUG(ezopt->flags) >= SLOG_LVL_DEBUG) {
			dump_ezthumb(ezopt, block);
		}
		break;
	/**/
	case EN_PROC_BEGIN:
		switch (param) {
		case ENX_SS_SCAN:
			EDB_SHOW(("Building (Scan)      "));
			break;
		case ENX_SS_SAFE:
			EDB_SHOW(("Building (Safe)      "));
			break;
		case ENX_SS_TWOPASS:
			EDB_SHOW(("Building (2Pass)     "));
			break;
		case ENX_SS_HEURIS:
			EDB_SHOW(("Building (Heur)      "));
			break;
		case ENX_SS_IFRAMES:
			EDB_SHOW(("Building (iFrame)      "));
			break;
		case ENX_SS_SKIM:
		default:
			EDB_SHOW(("Building (Fast)      "));
			break;
		}
		break;
	case EN_PROC_BINDING:
		EDB_SHOW(("+"));
		break;
	case EN_PROC_CURRENT:
		EDB_SHOW(("."));
		break;
	case EN_PROC_END:
		EDB_SHOW((" %ldx%ld done\n", param, opt));
		break;
	case EN_PROC_SAVED:
		myntf = block;
		vidx  = myntf->varg1;
		image = myntf->varg2;
		EDB_SHOW(("OUTPUT: %s\n", image->filename));

		n = sprintf(buf, "MAGIC: %s %s ", 
				seekm[vidx->seekable%4],
				dmod[GETDURMOD(vidx->ses_flags)>>12]);
		for (i = 0; i < vidx->pidx; i++) {
			n += sprintf(buf + n, "%c%d ", 
					vidx->pts[i*2], vidx->pts[i*2+1]);
		}
		strcat(buf, "\n");
		EDB_SHOW((buf));
		break;
	case EN_OPEN_BEGIN:
		vidx = block;
		EDB_SHOW(("Triaging %s ... ", vidx->filename));
		break;
	case EN_OPEN_END:
		if ((vidx = block) != NULL) {
			EDB_SHOW(("%dx%d %lld (ms)\n", vidx->width, 
					vidx->height, vidx->duration));
		} else if (param == EZ_ERR_VIDEOSTREAM) {
			EDB_SHOW(("no media\n"));
		} else {
			EDB_SHOW(("skip\n"));
		}
		break;
	case EN_STREAM_BROKEN:
		break;
	case EN_PACKET_RECV:
		//dump_packet(block);
		break;
	case EN_PACKET_KEY:
		if (EZOP_DEBUG(ezopt->flags) >= SLOG_LVL_FUNC) {
			dump_packet(block);
		}
		break;
	case EN_FRAME_DONE:
		if (EZOP_DEBUG(ezopt->flags) >= SLOG_LVL_PROGRAM) {
			dump_frame(block, opt);
		}
		break;
	case EN_FRAME_EFFECT:
		myntf = block;
		if (EZOP_DEBUG(ezopt->flags) >= SLOG_LVL_DEBUG) {
			dump_frame_packet(myntf->varg1, param, myntf->varg2);
		}
		break;
	case EN_SCAN_PACKET:
		//slogz("Key Frame %d: %lld\n", param, *((long long *)block));
		break;
	case EN_SCAN_IFRAME:
		vidx = block;
		if (EZOP_DEBUG(vidx->ses_flags) >= SLOG_LVL_DEBUG) {
			EDB_SHOW(("I-Frame Scanned (%ld ms):\n", opt));
			for (i = 0; i < param; i++) {
				EDB_SHOW(("%9lld", ((long long *)block)[i]));
				if ((i % 8) == 7) {
					EDB_SHOW(("\n"));
				}
			}
			if ((i % 8) != 0) {
				EDB_SHOW(("\n"));
			}
		}
		break;
	case EN_TYPE_VIDEO:
		if (EZOP_DEBUG(ezopt->flags) >= SLOG_LVL_INFO) {
			stream = block;
			dump_stream_common(stream, param);
			dump_video_context(stream->codec);
			dump_metadata(stream->metadata);
		}
		break;
	case EN_TYPE_AUDIO:
		if (EZOP_DEBUG(ezopt->flags) >= SLOG_LVL_INFO) {
			stream = block;
			dump_stream_common(stream, param);
			dump_audio_context(stream->codec);
			dump_metadata(stream->metadata);
		}
		break;
	case EN_TYPE_SUBTTL:
		if (EZOP_DEBUG(ezopt->flags) >= SLOG_LVL_INFO) {
			stream = block;
			dump_stream_common(stream, param);
			dump_subtitle_context(stream->codec);
			dump_metadata(stream->metadata);
		}
		break;
	case EN_TYPE_UNKNOWN:
		if (EZOP_DEBUG(ezopt->flags) >= SLOG_LVL_INFO) {
			stream = block;
			dump_stream_common(stream, param);
			dump_other_context(stream->codec);
			dump_metadata(stream->metadata);
		}
		break;
	case EN_DURATION:
		vidx = block;
		if (EZOP_DEBUG(ezopt->flags) >= SLOG_LVL_ERROR) {
			dump_duration(vidx, (int)opt);
		}
		break;
	case EN_BUMP_BACK:
		myntf = block;
		vidx = myntf->varg1;
		EDB_ERROR(("Bump back to %lld: %lld (%lld < %lld)\n",
				(long long) myntf->iarg2, 
				(long long) myntf->iarg1,
				(long long) vidx->keydelta, 
				(long long) vidx->keygap));
		break;
	case EN_MEDIA_STATIS:
		myntf = block;
		if (ezopt->flags & EZOP_CLI_INSIDE) {
			dump_media_statistic(myntf->varg1, 
					(int) param, myntf->varg2);
		}
		break;
	case EN_IFRAME_CREDIT:
		vidx = block;
		switch (param) {
		case ENX_IFRAME_RESET:
			EDB_DEBUG(("Key Frame accrediting system reset.\n"));
			break;
		case ENX_IFRAME_SET:
			EDB_FUNC(("Key Frame start from: %lld\n", 
					(long long) vidx->keylast));
			break;
		case ENX_IFRAME_UPDATE:
			EDB_FUNC(("Key Frame Gap Update: %lld\n", 
					(long long) vidx->keygap));
			break;
		}
		break;
	case EN_FRAME_EXCEPTION:
		if (EZOP_DEBUG(ezopt->flags) >= SLOG_LVL_WARNING) {
			EDB_WARN(("Discard Dodge I"));
			//dump_frame(block, 1);
		}
		break;
	case EN_SKIP_EXIST:
		EDB_ERROR(("Thumbnail Existed: %s\n", (char*) block));
		break;
	}
	return event;
}

static int dump_media_brief(EZVID *vidx)
{
	AVCodecContext	*codecx;
	char	tmp[16];
	int	i, sec;

	for (i = 0; i < (int)vidx->formatx->nb_streams; i++) {
		codecx = vidx->formatx->streams[i]->codec;
		if (codecx->codec_type == AVMEDIA_TYPE_VIDEO) {
			/* Fixed: the video information should use the actual
			 * duration of the clip */
			//sec = (int)(vidx->formatx->duration / AV_TIME_BASE);
			sec = vidx->duration / 1000;
			sprintf(tmp, "%dx%d", codecx->width, codecx->height);
			EDB_SHOW(("%2d:%02d:%02d %10s [%d]: %s\n",
					sec / 3600,
					(sec % 3600) / 60, 
					(sec % 3600) % 60,
					tmp, i, vidx->filename));
		}
	}
	return 0;
}

static int dump_media_statistic(struct MeStat *mestat, int n, EZVID *vidx)
{
	int64_t	dts;
	int	i, sec;

	EDB_SHOW(("Media: %s\n", vidx->filename));
	for (i = 0; i < n; i++) {
		EDB_SHOW(("[%d] ", i));
		if (i >= (int)vidx->formatx->nb_streams) {
			EDB_SHOW(("ERROR  %8lu\n", mestat[i].received));
			continue;
		}
		
		switch(vidx->formatx->streams[i]->codec->codec_type) {
		case AVMEDIA_TYPE_VIDEO:
			EDB_SHOW(("VIDEO  "));
			break;
		case AVMEDIA_TYPE_AUDIO:
			EDB_SHOW(("AUDIO  "));
			break;
		case AVMEDIA_TYPE_SUBTITLE:
			EDB_SHOW(("SUBTITL"));
			break;
		default:
			EDB_SHOW(("UNKNOWN"));
			break;
		}
		dts = mestat[i].dts_base + mestat[i].dts_last;
		dts -= vidx->dts_offset;
		sec = (int) (video_dts_to_ms(vidx, dts) / 1000);
		EDB_SHOW((":%-8lu KEY:%-6lu REW:%lu  TIME:%d\n",
				mestat[i].received, mestat[i].key, 
				mestat[i].rewound, sec));
	}
	EDB_SHOW(("Maximum Gap of key frames: %lld\n", 
			(long long) vidx->keygap));
	EDB_SHOW(("Time used: %.3f\n", smm_time_diff(&vidx->tmark) / 1000.0));
	return 0;
}

static int dump_format_context(AVFormatContext *format)
{
#ifdef	HAVE_AVFORMATCONTEXT_FILE_SIZE
	EDB_SHOW(("  Format: %s(%s), Size: %lld, Bitrate: %u\n",
			format->iformat->long_name,
			format->iformat->name,
			(long long) format->file_size,
			format->bit_rate));
#elif	defined(HAVE_AVFORMATCONTEXT_PB)
	EDB_SHOW(("  Format: %s(%s), Size: %lld, Bitrate: %u\n",
			format->iformat->long_name,
			format->iformat->name,
			(long long) avio_size(format->pb),
			format->bit_rate));
#else
	EDB_SHOW(("  Format: %s(%s), Size: unknown, Bitrate: %u\n",
			format->iformat->long_name,
			format->iformat->name,
			format->bit_rate));
#endif
	EDB_SHOW(("  Streams: %d, Start time: %lld, Duration: %lld\n",
			format->nb_streams,
			(long long) format->start_time,
			(long long) format->duration));
	return 0;
}

static int dump_stream_common(AVStream *stream, int sidx)
{
	AVCodec	*xcodec;

	xcodec = avcodec_find_decoder(stream->codec->codec_id);
	EDB_SHOW(("Stream #%d:%d: %s; Codec ID: %s; '%s' %s\n", 
			sidx, stream->id,
			id_lookup(id_codec_type, stream->codec->codec_type),
			id_lookup(id_codec, stream->codec->codec_id),
			xcodec ? xcodec->name : "Unknown",
			xcodec ? xcodec->long_name : "Unknown"));
	EDB_SHOW(("  Start Time  : %" PRId64 "; Duration: %" PRId64 
			"; Frame Rate: %d/%d; Stream_AR: %d/%d; Lang: %s\n",
			(stream->start_time < 0) ? -1 : stream->start_time, 
			(stream->duration < 0) ? -1 : stream->duration,
			stream->r_frame_rate.num, stream->r_frame_rate.den,
			stream->sample_aspect_ratio.num, 
			stream->sample_aspect_ratio.den,
			video_stream_language(stream)));
	return 0;
}

static int dump_video_context(AVCodecContext *codec)
{
	EDB_SHOW(("  Video Codec : %s; %dx%d+%d; Time Base: %d/%d; "
				"BR=%d BF=%d; AR: %d/%d\n",
			id_lookup(id_pix_fmt, codec->pix_fmt),
			codec->width, codec->height, 
			codec->frame_number, 
			codec->time_base.num, codec->time_base.den,
			codec->bit_rate, 
			codec->has_b_frames,
			codec->sample_aspect_ratio.num,
			codec->sample_aspect_ratio.den));
	return 0;
}

static int dump_audio_context(AVCodecContext *codec)
{
	EDB_SHOW(("  Audio Codec : %s; Time Base: %d/%d; "
				"CH=%d SR=%d %s BR=%d\n",
			id_lookup(id_codec, codec->codec_id),
			codec->time_base.num, codec->time_base.den,
			codec->channels, codec->sample_rate,
			id_lookup_tail(id_sample_format, codec->sample_fmt),
			codec->bit_rate));
	return 0;
}

static int dump_subtitle_context(AVCodecContext *codec)
{
	EDB_SHOW(("  Subtitles   : %s; Time Base: %d/%d; BR=%d\n",
			id_lookup(id_codec, codec->codec_id),
			codec->time_base.num, codec->time_base.den,
			codec->bit_rate));
	return 0;
}

static int dump_other_context(AVCodecContext *codec)
{
	(void) codec;		/* stop the gcc warning */
	return 0;
}

static int dump_packet(AVPacket *p)
{
	/* PTS:Presentation timestamp.  DTS:Decompression timestamp */
	EDB_SHOW(("Packet Pos:%" PRId64 ", PTS:%" PRId64 ", DTS:%" PRId64 
			", Dur:%d, Siz:%d, Flag:%d, SI:%d\n",
			p->pos, p->pts,	p->dts, p->duration, p->size,
			p->flags, p->stream_index));
	return 0;
}

static int dump_frame(EZFRM *ezfrm, int got_pic)
{
	EZVID	*vidx = ezfrm->frame->opaque;
	AVPacket	*packet = ezfrm->context;

	/*EDB_SHOW(("%s, KEY:%d, CPN:%d, DPN:%d, I:%d, %s\n", 
			got_pic == 0 ? "Decoding" : "Decoded ", 
			ezfrm->frame->key_frame, 
			ezfrm->frame->coded_picture_number, 
			ezfrm->frame->display_picture_number,
			ezfrm->frame->interlaced_frame,
			id_lookup(id_pict_type, ezfrm->frame->pict_type)));*/

	if (got_pic == 0) {
		return 0;
	}
	EDB_SHOW(("%s, KEY:%d, CPN:%d, DPN:%d, PTS:%" PRId64 ", %s DTS:%" PRId64 ", PKey:%d %s A:%d\n", 
			got_pic == 0 ? "Decoding" : "Decoded ", 
			ezfrm->frame->key_frame, 
			ezfrm->frame->coded_picture_number, 
			ezfrm->frame->display_picture_number,
			packet->dts,	//ezfrm->frame->best_effort_timestamp,
			id_lookup(id_pict_type, ezfrm->frame->pict_type),
			packet->dts, packet->flags, 
			ezfrm == vidx->picframe ? "CACHE" : "FRAME", 
			vidx->fkey));
	return 0;
}

static int dump_frame_packet(EZVID *vidx, int sn, EZFRM *ezfrm)
{
	int64_t	dts;
	char	timestamp[64];

	dts = ezfrm->rf_dts - vidx->dts_offset;
	meta_timestamp((int)video_dts_to_ms(vidx, dts), 1, timestamp);
	/*EDB_SHOW(("Frame %3d: Pos:%lld Size:%d PAC:%d DTS:%lld (%s) Type:%s\n",
			sn, (long long) ezfrm->rf_pos, ezfrm->rf_size, 
			ezfrm->rf_pac, (long long) ezfrm->rf_dts, timestamp, 
			id_lookup(id_pict_type, ezfrm->frame->pict_type)));*/
	EDB_SHOW(("Frame %3d: Pos:%lld Size:%d PAC:%d DTS:%lld (%s) Type:%s %s A:%d\n",
			sn, (long long) ezfrm->rf_pos, ezfrm->rf_size, 
			ezfrm->rf_pac, (long long) ezfrm->rf_dts, timestamp, 
			id_lookup(id_pict_type, ezfrm->frame->pict_type), 
			ezfrm == vidx->picframe ? "CACHE" : "FRAME", 
			vidx->fkey));
	return 0;
}


/* see /usr/include/libavformat/avformat.h */
static int dump_metadata(void *dict)
{
	struct	Metadata	{
		int	count;
#ifdef	HAVE_AV_DICT_GET
		AVDictionaryEntry	*elems;
#elif	defined(HAVE_AV_METADATA_GET)
		AVMetadataTag		*elems;
#else
#error Too old version of FFMPEG
#endif
	} *entry = dict;
	int	i;

	if (entry == NULL) {
		return 0;
	}
	for (i = 0; i < entry->count; i++) {
		EDB_SHOW(("  %-12s: %s\n", 
				entry->elems[i].key, entry->elems[i].value));
	}
	return i;
}

#if 0
int dump_metadata(void *dict)
{
	char	*mtab[] = { "album", "album_artist", "artist", "comment",
		"composer", "copyright", "creation_time", "date", "disc",
		"encoder", "encoded_by", "filename", "genre", "language",
		"performer", "publisher", "service_name", "service_provider",
		"title", "track", "variant_bitrate", NULL };
	int	i;
#ifdef	AVUTIL_DICT_H
	AVDictionaryEntry	*entry;
#elif (LIBAVFORMAT_VERSION_MINOR > 44) || (LIBAVFORMAT_VERSION_MAJOR > 52)
	AVMetadataTag		*entry;
#endif

	for (i = 0; mtab[i]; i++) {
#ifdef	AVUTIL_DICT_H
		entry = av_dict_get(dict, mtab[i], NULL, 0);
#elif (LIBAVFORMAT_VERSION_MINOR > 44) || (LIBAVFORMAT_VERSION_MAJOR > 52)
		entry = av_metadata_get(dict, mtab[i], NULL, 0);
#else
		break;
#endif
		if (entry) {
			slogz("   %s: %s\n", mtab[i], entry->value);
		}			
	}

	return 0;
}
#endif

static int dump_duration(EZVID *vidx, int use_ms)
{
	char	buf[128], tmp[32];

	switch (vidx->seekable) {
	case ENX_SEEK_NONE:
		strcpy(tmp, "Unseekable");
		break;
	case ENX_SEEK_FORWARD:
		strcpy(tmp, "Forward Only");
		break;
	case ENX_SEEK_FREE:
		strcpy(tmp, "Free Seeking");
		break;
	default:
		strcpy(tmp, "Unknown");
		break;
	}
	switch (GETDURMOD(vidx->ses_flags)) {
	case EZOP_DUR_HEAD:
		strcpy(buf, "reading media header");
		break;
	case EZOP_DUR_QSCAN:
		strcpy(buf, "Fast Scanning");
		break;
	case EZOP_DUR_FSCAN:
		strcpy(buf, "Full Scanning");
		break;
	case EZOP_DUR_AUTO:
		strcpy(buf, "Auto Scanning");
		break;
	default:
		strcpy(buf, "Mistake");
		break;
	}
	EDB_SHOW(("Duration found by %s: %lld (%d ms); "
				"Seeking capability: %s\n",
			buf, vidx->duration, use_ms, tmp));
	return 0;
}

static int dump_ezthumb(EZOPT *ezopt, EZIMG *image)
{
	EDB_SHOW(("\n>>>>>>>>>>>>>>>>>>\n"));
	EDB_SHOW(("Single shot size:  %dx%dx%d-%d\n", 
			image->dst_width, image->dst_height, 
			image->dst_pixfmt, ezopt->edge_width));
	EDB_SHOW(("Grid size:         %dx%d+%d\n", 
			image->grid_col, image->grid_row, 
			ezopt->shadow_width));
	EDB_SHOW(("Canvas size:       %dx%d-%d\n", 
			image->canvas_width, image->canvas_height, 
			image->canvas_minfo));
	EDB_SHOW(("Time setting:      %lld-%lld %lld (ms)\n", 
			(long long) image->time_from, 
			(long long) image->time_during, 
			(long long) image->time_step));
	EDB_SHOW(("Margin of canvas:  %dx%d\n", 
			image->rim_width, image->rim_height));
	EDB_SHOW(("Gap of shots:      %dx%d\n", 
			image->gap_width, image->gap_height));
	EDB_SHOW(("Color of Canvas:   BG#%08X SH#%08X MI#%08X\n",
			(unsigned) image->color_canvas,
			(unsigned) image->color_shadow,
			(unsigned) image->color_minfo));
	EDB_SHOW(("Color of Shots:    ED#%08X IN#%08X SH#%08X\n",
			(unsigned) image->color_edge,
			(unsigned) image->color_inset,
			(unsigned) image->color_inshadow));
	EDB_SHOW(("Font size:         MI=%d IN=%d (SH: %d %d)\n", 
			ezopt->mi_size, ezopt->ins_size,
			ezopt->mi_shadow, ezopt->ins_shadow));
	EDB_SHOW(("Font position:     MI=%d IN=%d\n",
			ezopt->mi_position, ezopt->ins_position));
	if (ezopt->mi_font) {
		EDB_SHOW(("Font MediaInfo:    %s\n", ezopt->mi_font));
	}
	if (ezopt->ins_font) {
		EDB_SHOW(("Font Inset Shots:  %s\n", ezopt->ins_font));
	}
	EDB_SHOW(("Output file name:  %s.%s (%d)\n", 
			ezopt->suffix, ezopt->img_format, ezopt->img_quality));
	EDB_SHOW(("Flags:             %s %s %s %s %s %s %s %s %s "
				"0x%x D%d P%d\n", 
			ezopt->flags & EZOP_INFO ? "MI" : "",
			ezopt->flags & EZOP_TIMEST ? "TS" : "",
			ezopt->flags & EZOP_FFRAME ? "FF" : "",
			ezopt->flags & EZOP_LFRAME ? "LF" : "",
			ezopt->flags & EZOP_P_FRAME ? "PF" : "",
			ezopt->flags & EZOP_CLI_INSIDE ? "CI" : "",
			ezopt->flags & EZOP_CLI_INFO ? "CO" : "",
			ezopt->flags & EZOP_TRANSPARENT ? "TP" : "",
			ezopt->flags & EZOP_DECODE_OTF ? "OT" : "",
			GETDURMOD(ezopt->flags),
			EZOP_DEBUG(ezopt->flags), EZOP_PROC(ezopt->flags)));
	EDB_SHOW(("Font numerate:     %dx%d %dx%d %dx%d %dx%d %dx%d\n",
			gdFontGetTiny()->w, gdFontGetTiny()->h,
			gdFontGetSmall()->w, gdFontGetSmall()->h,
			gdFontGetMediumBold()->w, gdFontGetMediumBold()->h,
			gdFontGetLarge()->w, gdFontGetLarge()->h,
			gdFontGetGiant()->w, gdFontGetGiant()->h));
	EDB_SHOW(("Background Image:  %s (0x%x)\n", 
			ezopt->background, ezopt->bg_position));

	ezopt_profile_dump(ezopt,"Profile of Grid:   ", "Profile of Shots:  ");
	EDB_SHOW(("<<<<<<<<<<<<<<<<<<\n"));
	return 0;
}





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
#include <sys/time.h>

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
static int video_snapshot_heuristic(EZVID *vidx, EZIMG *image);

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
//static int64_t video_keyframe_rectify(EZVID *vidx, AVPacket *packet, int64_t);
static int video_keyframe_credit(EZVID *vidx, int64_t dts);
static int64_t video_keyframe_seekat(EZVID *vidx, AVPacket *packet, int64_t);
static int64_t video_load_packet(EZVID *vidx, AVPacket *packet);
static int64_t video_current_dts(EZVID *vidx);
static int video_media_on_canvas(EZVID *vidx, EZIMG *image);
static EZTIME video_duration(EZVID *vidx);
static int video_duration_check(EZVID *vidx);
static int video_duration_seek(EZVID *vidx);
static int video_frame_scale(EZVID *vidx, AVFrame *frame);
static int64_t video_statistics(EZVID *vidx);
static int64_t video_snap_point(EZVID *vidx, EZIMG *image, int index);
static int video_snap_begin(EZVID *vidx, EZIMG *image, int method);
static int video_snap_update(EZVID *vidx, EZIMG *image, int64_t dts);
static int video_snap_end(EZVID *vidx, EZIMG *image);
static EZFRM *video_frame_next(EZVID *vidx);
static EZFRM *video_frame_best(EZVID *vidx, int64_t refdts);
static int64_t video_decode_next(EZVID *vidx, AVPacket *);
static int64_t video_decode_keyframe(EZVID *vidx, AVPacket *);
static int64_t video_decode_to(EZVID *vidx, AVPacket *packet, int64_t dtsto);
static int video_rewind(EZVID *vidx);
static int video_seeking(EZVID *vidx, int64_t dts);
static char *video_media_video(AVStream *stream, char *buffer);
static char *video_media_audio(AVStream *stream, char *buffer);
static char *video_media_subtitle(AVStream *stream, char *buffer);
static int64_t video_packet_timestamp(AVPacket *packet);

static EZIMG *image_allocate(EZVID *vidx, EZTIME rt_during, int *errcode);
static int image_free(EZIMG *image);
static int image_user_profile(EZIMG *image, int src_width, int *col, int *row,
		int *width, int *height, int *facto);
static int image_font_test(EZIMG *image, char *filename);
static int image_gdframe_update(EZIMG *image, AVFrame *swsframe);
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
static FILE *image_gif_anim_open(EZIMG *image, char *filename);
static int image_gif_anim_add(EZIMG *image, FILE *fout, int interval);
static int image_gif_anim_close(EZIMG *image, FILE *fout);
static FILE *image_create_file(EZIMG *image, char *filename, int idx);
static int image_cal_ratio(int ratio, int refsize);
static EZTIME image_cal_time_range(int ratio, EZTIME reftime);
static int image_cal_shots(EZTIME duration, EZTIME tmstep, int mode);
static EZTIME image_cal_timestep(EZTIME duration, int shots, int mode);
static int image_cal_gif_animix(EZOPT *ezopt);
static gdFont *image_fontset(int fsize);
static int image_copy(gdImage *dst, gdImage *src, int x, int, int, int);

static int ezopt_thumb_name(EZOPT *ezopt, char *buf, char *fname, int idx);

static int ezopt_profile_append(EZOPT *ezopt, char *ps);
static char *ezopt_profile_sprint(EZPROF *node, char *buf, int blen);
static EZPROF *ezopt_profile_new(EZOPT *opt, int flag, int wei);
static int ezopt_profile_free(EZPROF *node);
static EZPROF *ezopt_profile_insert(EZPROF *root, EZPROF *leaf);

static void *dts_lib_free(void *anchor);
static void *dts_lib_add(void *anchor, int64_t dts);
static int dts_lib_compress(void *anchor, int64_t *refdts, int num);

extern int ziptoken(char *sour, char **idx, int ids, char *delim);
#ifdef	CFG_GUI_ON
extern FILE *g_fopen(const char *filename, const char *mode);
#endif


void ezopt_init(EZOPT *ezopt, char *profile)
{
	memset(ezopt, 0, sizeof(EZOPT));
	ezopt->grid_col = 4;	/* the default thumbnail is a 4x4 array */
	ezopt->grid_row = 4;
	ezopt->canvas_width = 0;

	/* enable media info area and inset timestamp, skip the first and the
	 * last frame, no shadows */
	ezopt->flags = EZOP_INFO | EZOP_TIMEST | EZOP_DECODE_OTF;
	ezopt->flags |= EZOP_THUMB_COPY;

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

	if (profile) {
		ezopt_profile_setup(ezopt, profile);
	}
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
	if (rc == 0) {
		ezthumb_safe(filename, ezopt);
	}
	return EZ_ERR_NONE;
}

int ezthumb_safe(char *filename, EZOPT *ezopt)
{
	EZIMG	*image;
	EZVID	*vidx;
	int	rc;

	if ((vidx = video_allocate(ezopt, filename, &rc)) == NULL) {
		return rc;
	}
	if ((image = image_allocate(vidx, vidx->duration, &rc)) == NULL) {
		video_free(vidx);
		return rc;
	}
	if ((rc = video_connect(vidx, image)) != EZ_ERR_NONE) {
		image_free(image);
		video_free(vidx);
		return rc;
	}

	/* Register the video and image object so unix signal can intervene */
	ezopt->vidobj = vidx;
	ezopt->imgobj = image;

	if (image->time_step > 0) {	/* no i-frame rip in safe mode */
		video_snap_begin(vidx, image, GETPROCS(vidx->ses_flags));
		video_snapshot_safemode(vidx, image);
		video_snap_end(vidx, image);
	}

	/* deregister the video and image object so they can be reused */
	ezopt->vidobj = NULL;
	ezopt->imgobj = NULL;

	image_free(image);
	video_free(vidx);
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

	if (EZOP_DEBUG(ezopt->flags) >= EZOP_DEBUG_INFO) {
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

static int video_snapping(EZVID *vidx, EZIMG *image)
{
	int	rc = 0;

	/* if the expected time_step is 0, then it will save every 
	 * key frames separately. it's good for debug purpose  */
	if (image->time_step > 0) {	
		vidx->keydelta = video_ms_to_dts(vidx, image->time_step);
	} else {
		SETPROCS(vidx->ses_flags, EZOP_PROC_KEYRIP);
		vidx->keydelta = 0;
	}
	video_keyframe_credit(vidx, -1);

	switch (GETPROCS(vidx->ses_flags)) {
	case EZOP_PROC_SKIM:
		if (vidx->seekable == ENX_SEEK_BW_YES) {	//FIXME
			rc = video_snapshot_skim(vidx, image);
			break;
		}
		/* unseekable clips; fall into scan mode */
	case EZOP_PROC_SCAN:
		rc = video_snapshot_scan(vidx, image);
		break;
	case EZOP_PROC_SAFE:
		rc = video_snapshot_safemode(vidx, image);
		break;
	case EZOP_PROC_TWOPASS:
		rc = video_snapshot_twopass(vidx, image);
		break;
	case EZOP_PROC_HEURIS:
		rc = video_snapshot_heuristic(vidx, image);
		break;
	case EZOP_PROC_KEYRIP:
		rc = video_snapshot_keyframes(vidx, image);
		break;
	default:
		if (vidx->seekable == ENX_SEEK_BW_YES) {
			rc = video_snapshot_skim(vidx, image);
		} else {
			rc = video_snapshot_heuristic(vidx, image);
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
	int64_t		dts, dts_from, dts_to;
	int		i;

	/* set up the border */
	dts_from = 0;
	if (vidx->formatx->start_time != AV_NOPTS_VALUE) {
		dts_from += video_system_to_dts(vidx, 
				vidx->formatx->start_time);
	}
	dts_from += video_ms_to_dts(vidx, image->time_from);
	dts_to = dts_from + video_ms_to_dts(vidx, image->time_during);

	//SMM_PRINT("video_snapshot_keyframes: %lld to %lld\n", 
	//dts_from, dts_to);
	video_snap_begin(vidx, image, ENX_SS_IFRAMES);

	i = 0;
	video_keyframe_credit(vidx, -1);
	while ((dts = video_keyframe_next(vidx, &packet)) >= 0) {
		if (dts < dts_from) {
			if (vidx->ses_flags & EZOP_DECODE_OTF) {
				video_decode_next(vidx, &packet);
			} else {
				av_free_packet(&packet);
			}
			continue;
		}
		if (dts > dts_to) {
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

/* for these conditions: backward seeking available, key frame only,
 * snap interval is larger than maximum key frame interval and no rewind
 * clips */
static int video_snapshot_skim(EZVID *vidx, EZIMG *image)
{
	AVPacket	packet;
	int64_t		dts, dts_snap, last_key;
	int		scnt = 0;

	video_snap_begin(vidx, image, ENX_SS_SKIM);
	last_key = 0;
	while (image->taken < image->shots) {
		dts_snap = video_snap_point(vidx, image, image->taken);
		if (dts_snap < 0) {
			break;
		}

		video_seeking(vidx, dts_snap);

		dts = video_keyframe_next(vidx, &packet);
		//dts = video_keyframe_rectify(vidx, &packet, dts_snap);
		if (dts < 0) {
			break;
		}

		if (dts == last_key) {
			dts = video_decode_to(vidx, &packet, dts_snap);
		} else {
			last_key = dts;
			if (GETACCUR(vidx->ses_flags)) {
				dts = video_decode_to(vidx, &packet, dts_snap);
			} else {
				dts = video_decode_keyframe(vidx, &packet);
			}
		}
		if (dts < 0) {
			break;
		}
		
		video_snap_update(vidx, image, dts_snap);
		scnt++;
	}
	video_snap_end(vidx, image);
	return scnt;	/* return the number of thumbnails */
}

static int video_snapshot_safemode(EZVID *vidx, EZIMG *image)
{
	AVPacket	packet;
	int64_t		dts, dts_snap;
	int		scnt = 0;

	dts_snap = video_snap_point(vidx, image, image->taken);
	if (dts_snap < 0) {
		return scnt;
	}

	while ((dts = video_keyframe_next(vidx, &packet)) >= 0) {
		/* use video_decode_next() instead of video_decode_keyframe()
		 * because sometimes it's good for debugging doggy clips */
		if (video_decode_next(vidx, &packet) < 0) {
			break;
		}
		if (dts >= dts_snap) {
			video_snap_update(vidx, image, dts_snap);
			scnt++;
			if (image->taken >= image->shots) {
				break;
			}
			dts_snap = video_snap_point(vidx, image, image->taken);
			if (dts_snap < 0) {
				break;
			}
		}
	}
	video_snap_end(vidx, image);
	return scnt;	/* return the number of thumbnails */
}


/* for these conditions: Though backward seeking is NOT available but it is 
 * required to extract key frame only. snap interval is larger than maximum 
 * key frame interval and no rewind clips */
static int video_snapshot_scan(EZVID *vidx, EZIMG *image)
{
	AVPacket	packet;
	int64_t		dts, dts_snap;
	int		scnt = 0;

	video_snap_begin(vidx, image, ENX_SS_SCAN);
	dts = 0;
	while (image->taken < image->shots) {
		dts_snap = video_snap_point(vidx, image, image->taken);
		if (dts_snap < 0) {
			break;
		}

		if (dts < dts_snap) {
			dts = video_keyframe_to(vidx, &packet, dts_snap);
			if (dts < 0) {
				break;
			}
			/* Argus_20120222-114427384.ts case:
			 * the HD DVB rip file has some dodge i-frame, 
			 * after decoding, they turned to P/B frames.
			 * This issue may also happen on H.264 streams.
			 * The workaround is the option to decode the
			 * next frame instead of searching an i-frame. */
			if (GETACCUR(vidx->ses_flags)) {
				//SMM_PRINT("DTS=%lld to %lld\n", dts, dts_snap);
				dts = video_decode_next(vidx, &packet);
			} else {
				dts = video_decode_keyframe(vidx, &packet);
			}
			if (dts < 0) {
				break;
			}
		}
		//video_snap_update(vidx, image, dts);
		video_snap_update(vidx, image, dts_snap);
		scnt++;
	}
	video_snap_end(vidx, image);
	return scnt;	/* return the number of thumbnails */
}

/* for these conditions: Though backward seeking is NOT available and it is 
 * required to extract p-frames. */
static int video_snapshot_twopass(EZVID *vidx, EZIMG *image)
{
	AVPacket	packet;
	int64_t		dts, *refdts;
	int		i, scnt = 0;

	/* compress the key frame list */
	if ((refdts = calloc(image->shots * 4, sizeof(int64_t))) == NULL) {
		return EZ_ERR_LOWMEM;
	}
	for (i = 0; i < image->shots; i++) {
		refdts[i*3] = video_snap_point(vidx, image, i);
	}
	dts_lib_compress(vidx->keylib, refdts, image->shots);

	/*
	for (i = 0; i < image->shots; i++) {
		SMM_PRINT("(%lld %lld %lld)", 
				refdts[i*3], refdts[i*3+1], refdts[i*3+2]);
	}
	printf("\n");
	*/
	/* review if more than one snapshots were taken inside nearby 
	 * keyframes. If more than one shots, we will take p-frames instead 
	 * even the EZOP_P_FRAME was not set */
	if (!GETACCUR(vidx->ses_flags)) {
		for (i = 0; i < image->shots; i++) {
			if (refdts[i*3] < 0) {
				continue;
			}
			if (refdts[i*3+1] == refdts[i*3+4]) {
				SETACCUR(vidx->ses_flags);
				break;
			}
		}
	}

	video_snap_begin(vidx, image, ENX_SS_TWOPASS);
	dts = 0;
	for (i = image->taken; i < image->shots; i++) {
		if (refdts[i*3] < 0) {
			break;
		}
		if (dts < refdts[i*3+1]) {
			dts = video_keyframe_to(vidx, &packet, refdts[i*3+1]);
			video_decode_next(vidx, &packet);
		}
		if (GETACCUR(vidx->ses_flags)) {
			dts = video_load_packet(vidx, &packet);
			video_decode_to(vidx, &packet, refdts[i*3]);
		} else if (dts < refdts[i*3+2]) {
			dts = video_keyframe_to(vidx, &packet, refdts[i*3+2]);
			video_decode_next(vidx, &packet);
		}
		if (dts < 0) {
			break;
		}
		video_snap_update(vidx, image, refdts[i*3]);
		scnt++;
	}
	video_snap_end(vidx, image);
	free(refdts);
	return scnt;	/* return the number of thumbnails */
}


static int video_snapshot_heuristic(EZVID *vidx, EZIMG *image)
{
	AVPacket	packet;
	int64_t		dts, dts_snap;
	int		scnt = 0;

	video_snap_begin(vidx, image, ENX_SS_HEURIS);
	dts = 0;
	while (image->taken < image->shots) {
		dts_snap = video_snap_point(vidx, image, image->taken);
		if (dts_snap < 0) {
			break;
		}

		/*SMM_PRINT("Measuring: Snap=%lld Cur=%lld  Dis=%lld Gap=%lld\n",
				dts_snap, dts, dts_snap - dts, vidx->keygap);*/
		if (dts_snap - dts > vidx->keygap) {
			dts = video_keyframe_seekat(vidx, &packet, dts_snap);
		} else {
			dts = video_load_packet(vidx, &packet);
		}
		if (dts < 0) {
			break;
		}

		if (dts >= dts_snap) {
			dts = video_decode_keyframe(vidx, &packet);
		} else {
			dts = video_decode_to(vidx, &packet, dts_snap);
		}
		if (dts < 0) {
			break;
		}

		video_snap_update(vidx, image, dts);	//FIXME: dts or dts_snap?
		scnt++;
	}
	video_snap_end(vidx, image);
	return scnt;	/* return the number of thumbnails */
}



static EZVID *video_allocate(EZOPT *ezopt, char *filename, int *errcode)
{
	EZVID	*vidx;
	int	rc;

	if ((vidx = calloc(sizeof(EZVID), 1)) == NULL) {
		uperror(errcode, EZ_ERR_LOWMEM);
		return NULL;
	}

	vidx->sysopt   = ezopt;
	vidx->filename = filename;
	vidx->seekable = -1;
	smm_time_get_epoch(&vidx->tmark);	/* get current time */

	/* check if the nominated file existed */
	if ((vidx->filesize = smm_filesize(filename)) <= 0) {
		eznotify(NULL, EZ_ERR_FILE, 0, 0, filename);
		uperror(errcode, EZ_ERR_FILE);
		free(vidx);
		return NULL;
	}

	/* On second thought, the FFMPEG log is better to be enabled while 
	 * loading codecs so we would've known if the video files buggy */
	if (EZOP_DEBUG(ezopt->flags) == EZOP_DEBUG_FFM) {
		av_log_set_level(AV_LOG_VERBOSE);	/* enable all logs */
	} else if (EZOP_DEBUG(ezopt->flags) >= EZOP_DEBUG_BRIEF) {
		av_log_set_level(AV_LOG_INFO);
	} else {
		av_log_set_level(0);
	}

	/* 20120723 Moved from ezopt_review() */
	vidx->ses_dura  = ezopt->dur_mode;
	vidx->ses_flags = ezopt->flags;
	if (GETPROCS(vidx->ses_flags) == EZOP_PROC_TWOPASS) {
		vidx->ses_dura = EZ_DUR_FULLSCAN;
	} else if (GETPROCS(vidx->ses_flags) != EZOP_PROC_KEYRIP) {
		if (vidx->ses_dura == EZ_DUR_FULLSCAN) {
			SETPROCS(vidx->ses_flags, EZOP_PROC_TWOPASS);
		}
	}

	if ((rc = video_open(vidx)) != EZ_ERR_NONE) {
		uperror(errcode, rc);
		free(vidx);
		return NULL;
	}

	/* update the filesize field with the ffmpeg attribute.
	 * this is a foolproof procedure */
	/* the file_size field will be depreciated soon */
#if	LIBAVFORMAT_VERSION_INT < (53<<16)
	if (vidx->filesize < vidx->formatx->file_size) {
		vidx->filesize = vidx->formatx->file_size;
	}
#else
	if (vidx->formatx->pb) {
		if (vidx->filesize < avio_size(vidx->formatx->pb)) {
			vidx->filesize = avio_size(vidx->formatx->pb);
		}
	}
#endif
	/* find out the clip duration in millisecond */
	/* 20110301: the still images are acceptable by the ffmpeg library
	 * so it would be wiser to avoid the still image stream, which duration
	 * is only several milliseconds. FIXME if this assumption is wrong */
	if (video_duration(vidx) < 500) {
		uperror(errcode, EZ_ERR_FILE);
		eznotify(vidx->sysopt, EZ_ERR_VIDEOSTREAM, 1, 0, filename);
		video_free(vidx);
		return NULL;
	}

	/* 20111213: It seems detecting media length could not block some 
	 * unwanted files. For example, in guidev branch, the ezthumb.o
	 * was treated as a 3 seconds long MP3 file. Thus I set another
	 * filter to check the media's resolution. */
	if (!vidx->formatx->streams[vidx->vsidx]->codec->width ||
			!vidx->formatx->streams[vidx->vsidx]->codec->height) {
		uperror(errcode, EZ_ERR_FILE);
		eznotify(vidx->sysopt, EZ_ERR_VIDEOSTREAM, 1, 0, filename);
		video_free(vidx);
		return NULL;
	}


	/* collecting other information */
	vidx->width    = vidx->codecx->width;
	vidx->height   = vidx->codecx->height;
	vidx->streams  = vidx->formatx->nb_streams;

	/* 20120720 Apply the AR correction */
	/* calculate the original video height by AR correction */
	vidx->ar_height = vidx->codecx->height;
	if (vidx->codecx->sample_aspect_ratio.num &&
			vidx->codecx->sample_aspect_ratio.den) {
		vidx->ar_height = vidx->codecx->height *
			vidx->codecx->sample_aspect_ratio.den /
			vidx->codecx->sample_aspect_ratio.num;
	}

	//dump_format_context(vidx->formatx);
	//eznotify(ezopt, EN_FILE_OPEN, 0, 0, vidx);
	eznotify(vidx->sysopt, EN_MEDIA_OPEN, 0, 
			smm_time_diff(&vidx->tmark), vidx);
	uperror(errcode, EZ_ERR_NONE);
	return vidx;
}

static EZVID *video_alloc_queue(EZOPT *ezopt, char **fname, int fnum)
{
	EZVID	*vanchor, *vidx, *vp;
	EZTIME	dur_all, dur_off;
	int	i, rc;

	vanchor = NULL;
	dur_all = dur_off = 0;
	for (i = 0; i < fnum; i++) {
		if ((vidx = video_allocate(ezopt, fname[i], &rc)) == NULL) {
			continue;
		}

		vidx->dur_off = dur_off;
		dur_off += vidx->duration;
		dur_all += vidx->duration;

		if (vanchor == NULL) {
			vanchor = vidx;
		} else if (vanchor->next == NULL) {
			vanchor->next = vidx;
		} else {
			for (vp = vanchor; vp->next; vp = vp->next);
			vp->next = vidx;
		}
		vidx->anchor = vanchor;
		video_close(vidx);
	}
	for (vp = vanchor; vp; vp = vp->next) {
		vp->dur_all = dur_all;

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
	return vanchor;
}

static int video_free(EZVID *vidx)
{
	EZVID	*vp;

	while (vidx) {
		video_disconnect(vidx);	
		video_close(vidx);

		vidx->keylib = dts_lib_free(vidx->keylib);

		vp = vidx;
		vidx = vidx->next;
		free(vp);
	}
	return EZ_ERR_NONE;
}


static int video_open(EZVID *vidx)
{
	AVCodec	*codec;
	char	*mblock[] = { "mp3", "image2" };
	int	i;

	/* apparently the ubuntu 10.10 still use av_open_input_file() */
	/* FFMPEG/doc/APIchanes claim the avformat_open_input() was introduced
	 * since 53.2.0. Apparently it is wrong. It is at least appeared in
	 * my archlinux 64-bit box by 52.110.0 */
	/* 20120613: What a surprise that avformat_open_input() do support
	 * utf-8 in native MSWindows */
#if	(LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 110, 0))
	if (avformat_open_input(&vidx->formatx, vidx->filename, 
				NULL, NULL) != 0) {
#else
	if (av_open_input_file(&vidx->formatx, vidx->filename, 
				NULL, 0, NULL) < 0) {
#endif
		eznotify(NULL, EZ_ERR_FORMAT, 0, 0, vidx->filename);
		return EZ_ERR_FORMAT;
	}

	/* 20120814 Implemented a media type filter to block the unwanted
	 * media files like jpg, mp3, etc */
	for (i = 0; i < sizeof(mblock)/sizeof(char*); i++) {
		if (!strcmp(vidx->formatx->iformat->name, mblock[i])) {
			eznotify(NULL, EZ_ERR_FORMAT, 0, 0, vidx->filename);
			video_close(vidx);
			return EZ_ERR_FORMAT;
		}
	}

	/* FIXME: what are these for? */
	vidx->formatx->flags |= AVFMT_FLAG_GENPTS;
#if	(LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53, 3, 0))
	if (avformat_find_stream_info(vidx->formatx, NULL) < 0) {
#else
	if (av_find_stream_info(vidx->formatx) < 0) {
#endif
		eznotify(NULL, EZ_ERR_STREAM, 0, 0, vidx->filename);
		video_close(vidx);
		return EZ_ERR_STREAM;
	}

	/* find the video stream and open the codec driver */
	if (video_find_main_stream(vidx) < 0) {
		eznotify(NULL, EZ_ERR_VIDEOSTREAM, 0, 0, vidx->filename);
		video_close(vidx);
		return EZ_ERR_VIDEOSTREAM;
	}

	vidx->vstream = vidx->formatx->streams[vidx->vsidx];
	vidx->codecx  = vidx->vstream->codec;
	/* discard frames; AVDISCARD_NONKEY,AVDISCARD_BIDIR */
	vidx->codecx->skip_frame = AVDISCARD_NONREF;
	//vidx->codecx->hurry_up = 1; /* fast decoding mode */

	/* open the codec */
	vidx->vstream->discard = AVDISCARD_ALL;
	codec = avcodec_find_decoder(vidx->codecx->codec_id);
#if	(LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 6, 0))
	if (avcodec_open2(vidx->codecx, codec, NULL) < 0) {
#else
	if (avcodec_open(vidx->codecx, codec) < 0) {
#endif
		eznotify(vidx->sysopt, EZ_ERR_CODEC_FAIL, 
				vidx->codecx->codec_id, 0, vidx->codecx);
		video_close(vidx);
		return EZ_ERR_CODEC_FAIL;
	}
	vidx->vstream->discard = AVDISCARD_DEFAULT;

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
#if	(LIBAVFORMAT_VERSION_INT > AV_VERSION_INT(53, 10, 0))
		avformat_close_input(&vidx->formatx);
#else
		av_close_input_file(vidx->formatx);
#endif
		vidx->formatx = NULL;
	}
	return EZ_ERR_NONE;
}

static int video_connect(EZVID *vidx, EZIMG *image)
{
	int	size;

	/* allocate a reusable video frame structure */
	if ((vidx->fgroup[0].frame = avcodec_alloc_frame()) == NULL) {
		eznotify(vidx->sysopt, EZ_ERR_VIDEOSTREAM, 0, 0, vidx->filename);
		return EZ_ERR_LOWMEM;
	}
	if ((vidx->fgroup[1].frame = avcodec_alloc_frame()) == NULL) {
		video_disconnect(vidx);
		eznotify(vidx->sysopt, EZ_ERR_VIDEOSTREAM, 0, 0, vidx->filename);
		return EZ_ERR_LOWMEM;
	}

	/* 20120723 Initialize the rf_dts to -1 to avoid the 
	 * first-unavailable-frame issue */
	vidx->fgroup[0].rf_dts = vidx->fgroup[1].rf_dts = -1;


	/* allocate the swscale structure for scaling the screen image */
	vidx->swsctx = sws_getContext(vidx->codecx->width, 
			vidx->codecx->height, vidx->codecx->pix_fmt, 
			image->dst_width, image->dst_height,
			image->dst_pixfmt, SWS_BILINEAR, NULL, NULL, NULL);
	if (vidx->swsctx == NULL) {
		video_disconnect(vidx);
		eznotify(vidx->sysopt, EZ_ERR_SWSCALE, 0, 0, vidx->filename);
		return EZ_ERR_SWSCALE;
	}

	/* allocate the frame structure for RGB converter which
	 * will be filled by frames converted from YUV form */
	if ((vidx->swsframe = avcodec_alloc_frame()) == NULL) {
		video_disconnect(vidx);
		eznotify(vidx->sysopt, EZ_ERR_SWSCALE, 0, 0, vidx->filename);
		return EZ_ERR_SWSCALE;
	}

	/* allocate the memory buffer for holding the pixel array of
	 * RGB frame */
	size = avpicture_get_size(image->dst_pixfmt, 
			image->dst_width, image->dst_height);
	if ((vidx->swsbuffer = av_malloc(size)) == NULL) {
		video_disconnect(vidx);
		eznotify(vidx->sysopt, EZ_ERR_SWSCALE, 0, 0, vidx->filename);
		return EZ_ERR_SWSCALE;
	}

	/* link the RGB frame and the RBG pixel buffer */
	avpicture_fill((AVPicture *) vidx->swsframe, vidx->swsbuffer, 
		image->dst_pixfmt, image->dst_width, image->dst_height);
	return EZ_ERR_NONE;
}

static int video_disconnect(EZVID *vidx)
{
	if (vidx->swsbuffer) {
		av_free(vidx->swsbuffer);
		vidx->swsbuffer = NULL;
	}
	if (vidx->swsframe) {
		av_free(vidx->swsframe);
		vidx->swsframe = NULL;
	}
	if (vidx->swsctx) {
		sws_freeContext(vidx->swsctx);
		vidx->swsctx = NULL;
	}
	if (vidx->fgroup[1].frame) {
		av_free(vidx->fgroup[1].frame);
		vidx->fgroup[1].frame = NULL;
	}
	if (vidx->fgroup[0].frame) {
		av_free(vidx->fgroup[0].frame);
		vidx->fgroup[0].frame = NULL;
	}
	return EZ_ERR_NONE;
}


/* This function is used to find the video stream in the clip 
 * as well as open the related decoder driver */
static int video_find_main_stream(EZVID *vidx)
{
	AVStream	*stream;
	int	i;

#if	(LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 91, 0))
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
	for (i = 0; i < vidx->formatx->nb_streams; i++) {
		eznotify(vidx->sysopt, EN_STREAM_FORMAT, i, 0, vidx);
		stream = vidx->formatx->streams[i];
		stream->discard = AVDISCARD_ALL;
		switch (stream->codec->codec_type) {
		case AVMEDIA_TYPE_VIDEO:
			eznotify(vidx->sysopt, EN_TYPE_VIDEO, 
					i, 0, stream->codec);
			break;
		case AVMEDIA_TYPE_AUDIO:
			eznotify(vidx->sysopt, EN_TYPE_AUDIO, 
					i, 0, stream->codec);
			break;
		default:
			eznotify(vidx->sysopt, EN_TYPE_UNKNOWN, 
					i, 0, stream->codec);
			break;
		}
	}

	vidx->vsidx = vidx->sysopt->vs_user;
	if ((vidx->vsidx >= 0) && (vidx->vsidx < vidx->formatx->nb_streams)) {
		stream = vidx->formatx->streams[vidx->vsidx];
		if (stream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			return vidx->vsidx;
		}
	}
	
#if	(LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 91, 0))
	vidx->vsidx = av_find_best_stream(vidx->formatx, AVMEDIA_TYPE_VIDEO,
			wanted_stream[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
#else
	vidx->vsidx = -1;
	for (i = 0; i < vidx->formatx->nb_streams; i++) {
		stream = vidx->formatx->streams[i];
		if (stream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			if (wanted_stream < stream->codec_info_nb_frames) {
				vidx->vsidx = i;
				wanted_stream = stream->codec_info_nb_frames;
			}
		}
	}
#endif
	return vidx->vsidx;
}


static int64_t video_keyframe_next(EZVID *vidx, AVPacket *packet)
{
	int64_t		dts;

	while ((dts = video_load_packet(vidx, packet)) >= 0) {
		if (packet->flags != AV_PKT_FLAG_KEY) {
			av_free_packet(packet);
			continue;
		}

		/* find a valid key frame so updating the keyframe gap */
		video_keyframe_credit(vidx, dts);
		break;
	}
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

		/* find a valid key frame so updating the keyframe gap */
		video_keyframe_credit(vidx, dts);

		if (dts >= pos) {
			break;	/* successfully located the keyframe */
		}

		if (vidx->ses_flags & EZOP_DECODE_OTF) {
			video_decode_next(vidx, packet);
		} else {
			av_free_packet(packet);
		}
	}
	return dts;
}

#if 0
static int64_t video_keyframe_rectify(EZVID *vidx, AVPacket *packet, 
		int64_t dtsto)
{
	int64_t	dts;

	dts = video_keyframe_next(vidx, packet);
	if (dts >= dtsto) {
		return dts;
	}
	if (video_dts_to_ms(vidx, dtsto - dts) < 10000) {	/* magic 10s*/
		return dts;
	}
	return video_keyframe_to(vidx, packet, dtsto);
}
#endif

static int video_keyframe_credit(EZVID *vidx, int64_t dts)
{
	/* reset the key frame crediting */ 
	if (dts < 0) {
		vidx->keylast = -1;
		eznotify(vidx->sysopt, EN_IFRAME_CREDIT, 
				ENX_IFRAME_RESET, 0, vidx);
		return vidx->keycount;
	}

	/* record the status of the first key frame since resetted */
	if (vidx->keylast < 0) {
		vidx->keylast = dts;
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
	return vidx->keycount;
}

static int64_t video_keyframe_seekat(EZVID *vidx, AVPacket *packet, int64_t dts_snap)
{
	int64_t		dts, dts_last, dts_diff;
	int		keyflag;

	dts_last = 0;
	while ((dts = video_keyframe_next(vidx, packet)) >= 0) {
		/* if the distance between the snap point and this key frame
		 * is longer than the maximum gap of key frames, we would 
		 * expect for another key frame */
		if ((dts_diff = dts_snap - dts) > vidx->keygap) {
			if (vidx->ses_flags & EZOP_DECODE_OTF) {
				video_decode_next(vidx, packet);
			} else {
				av_free_packet(packet);
			}
			dts_last = dts;
			continue;
		}

		/* setup the key frame decoding flag */
		if (GETACCUR(vidx->ses_flags)) {
			keyflag = 0;
		} else if (vidx->keydelta < vidx->keygap) {
			keyflag = 0;
		} else {
			keyflag = 1;
		}

		/* if the distance is negative, we must have overread to the
		 * next key frame, which is caused by the incomplete maximum
		 * gap of key frames. In that case, we must increate the gap
		 * size and rewind to the previous key frame and run again */
		if ((dts_diff < 0) && (keyflag == 0)) {
			struct	ezntf	myntf;
			myntf.varg1 = vidx;
			myntf.iarg1 = dts_diff;
			myntf.iarg2 = dts_last;
			eznotify(vidx->sysopt, EN_BUMP_BACK, 0, 0, &myntf);
			video_seeking(vidx, dts_last);
			av_free_packet(packet);
			continue;
		}

		/* if we only want to snapshot at key frames, we leap to the
		 * next one and take sanpshot there */
		if ((dts_diff > 0) && (keyflag == 1)) {
			if (vidx->ses_flags & EZOP_DECODE_OTF) {
				video_decode_next(vidx, packet);
			} else {
				av_free_packet(packet);
			}
			dts_last = dts;
			continue;
		}

		/* so the proper key frame has been read */
		return dts;
	}
	return -1;
}

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
		return dts;
	}
	return -1;
}

static int64_t video_current_dts(EZVID *vidx)
{
	AVPacket	packet;
	int64_t		dts;

	dts = video_load_packet(vidx, &packet);
	if (dts >= 0) {
		av_free_packet(&packet);
	}
	return dts;
}

/* This function is used to print the media information to the specified
 * area in the canvas */
static int video_media_on_canvas(EZVID *vidx, EZIMG *image)
{
	AVStream	*stream;
	char		buffer[256], tmp[32];
	int		i;

	// FIXME: UTF-8 and wchar concern
	/* Line 0: the filename */
	strcpy(image->filename, "NAME: ");
	if (vidx->dur_all) {	/* binding mode */
		meta_basename(vidx->anchor->filename, image->filename + 6);
		strcat(image->filename, " ...");
	} else {
		meta_basename(vidx->filename, image->filename + 6);
	}
	image_gdcanvas_print(image, 0, 0, image->filename);
	puts(image->filename);

	/* Line 1: the duration of the video clip, the file size, 
	 * the frame rates and the bit rates */
	strcpy(buffer, "Duration: ");
	if (vidx->dur_all) {	/* binding mode */
		strcat(buffer, meta_timestamp(vidx->dur_all, 0, tmp));
	} else {
		strcat(buffer, meta_timestamp(vidx->duration, 0, tmp));
	}

	strcat(buffer, " (");
	strcat(buffer, meta_filesize(vidx->filesize, tmp));
	strcat(buffer, ")  ");

	/*i = vidx->formatx->bit_rate;
	if (vidx->formatx->bit_rate == 0) {
		i = (int)(vidx->formatx->file_size * 1000 / vidx->duration);
	}*/
	/* formatx->bit_rate sometimes missed as audio bitrate by ffmpeg.
	 * so we calculate its combined bit rate here */
	i = (int) (vidx->filesize * 1000 / vidx->duration);
	strcat(buffer, meta_bitrate(i, tmp));
	image_gdcanvas_print(image, 1, 0, buffer);
	puts(buffer);

	/* Line 2+: the stream information */
	for (i = 0; i < vidx->formatx->nb_streams; i++) {
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
			strcat(buffer, "Unknown");
			break;
		}
		image_gdcanvas_print(image, i + 2, 0, buffer);
		puts(buffer);
	}
	return EZ_ERR_NONE;
}
/* This function is used to find the video clip's duration. There are three
 * methods to retrieve the duration. First and the most common one is to
 * grab the duration data from the clip head, EZ_DUR_CLIPHEAD. It's already 
 * available in the AVFormatContext structure when opening the video. 
 * However, sometimes the information is inaccurate or lost. The second method,
 * EZ_DUR_FULLSCAN, is used to scan the entire video file till the last packet
 * to decide the final PTS. To speed up the process, the third method,
 * EZ_DUR_QK_SCAN, only scan the last 90% clip. 
 * User need to specify the scan method. */
static EZTIME video_duration(EZVID *vidx)
{
	int64_t	cur_dts;

	/* find the duration in the header */
	vidx->duration = (EZTIME)(vidx->formatx->duration / 
			AV_TIME_BASE * 1000);

	/* It seems the header missing issue can not be fixed simply
	 * by seek to tail. In that case a full stream scan is required */
	if (video_duration_check(vidx) == 0) {	/* bad video header */
		vidx->ses_dura = EZ_DUR_FULLSCAN; /* full scan is enforced */
		if (GETPROCS(vidx->ses_flags) != EZOP_PROC_KEYRIP) {
			SETPROCS(vidx->ses_flags, EZOP_PROC_TWOPASS);
		}
	}

	video_duration_seek(vidx);
	switch (vidx->ses_dura) {
	case EZ_DUR_CLIPHEAD:
		video_rewind(vidx);	/* rewind the stream */
		eznotify(vidx->sysopt, EN_DURATION, 
				ENX_DUR_MHEAD, 0, &vidx->duration);
		return vidx->duration;

	case EZ_DUR_FULLSCAN:
		video_rewind(vidx); 
		cur_dts = video_statistics(vidx);
		break;

	default:	/* EZ_DUR_QK_SCAN */
		cur_dts = video_statistics(vidx);
		if (vidx->seekable == ENX_SEEK_BW_YES) {
			eznotify(vidx->sysopt, EN_DURATION, 
					ENX_DUR_JUMP, 0, &cur_dts);
		} else {
			vidx->ses_dura = EZ_DUR_FULLSCAN;
			if (GETPROCS(vidx->ses_flags) != EZOP_PROC_KEYRIP) {
				SETPROCS(vidx->ses_flags, EZOP_PROC_TWOPASS);
			}
		}
		break;
	}
	video_rewind(vidx); 	/* rewind the stream to head */

	/* convert duration from video stream base to milliseconds */
	vidx->duration = video_dts_to_ms(vidx, cur_dts);
	eznotify(vidx->sysopt, EN_DURATION, ENX_DUR_SCAN, 0, &vidx->duration);
	return vidx->duration;
}

static int video_duration_check(EZVID *vidx)
{
	int	br;

	if (vidx->duration <= 0) {
		return 0;	/* bad duration */
	}

	br = (int)(vidx->filesize * 1000 / vidx->duration);
	//printf("video_duration_check: dur=%d br=%d\n", vidx->duration, br);
	if (br < EZ_BR_GATE_LOW) {	/* very suspecious bitrates */
		return 0;
	}
	return 1;
}

/* 20120308: should seek to position according to the length of the 
 * file rather than the duration. It's quite obvious that when one 
 * need the scan mode, the duration must has been wrong already.
 * 20120313: AVSEEK_FLAG_BYTE is incapable to seek through some video file.
 */
static int video_duration_seek(EZVID *vidx)
{
	int64_t	first_dts, cur_dts;

	/* seek to 90% of the clip */
	first_dts = video_current_dts(vidx);
		
	cur_dts = video_system_to_dts(vidx, vidx->formatx->duration);
	video_seeking(vidx, cur_dts * 9 / 10);
	cur_dts = video_current_dts(vidx);

	//SMM_PRINT("DTS: %lld %lld\n", first_dts, cur_dts);
	//SMM_PRINT("POS: %lld\n", vidx->formatx->pb->pos);
	if ((first_dts < 0) || (cur_dts < 0)) {
		vidx->seekable = ENX_SEEK_BW_NO;
	} else if (cur_dts <= first_dts) {
		vidx->seekable = ENX_SEEK_BW_NO;
	} else {
		vidx->seekable = ENX_SEEK_BW_YES;
	}
	eznotify(vidx->sysopt, EN_SEEK_FRAME, vidx->seekable, 0, &cur_dts);
	return vidx->seekable;
}

static int video_frame_scale(EZVID *vidx, AVFrame *frame)
{
	return sws_scale(vidx->swsctx, (const uint8_t * const *)frame->data,
			frame->linesize, 0, vidx->codecx->height, 
			vidx->swsframe->data, vidx->swsframe->linesize);
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
		if (i > vidx->formatx->nb_streams) {
			i = vidx->formatx->nb_streams;
		}
		if (i >= EZ_ST_MAX_REC) {
			av_free_packet(&packet);
			continue;
		}
		imax = i > imax ? i : imax;

		mestat[i].received++;
		/* higher version ffmpeg doesn't support PKT_FLAG_KEY */
		if (packet.flags == AV_PKT_FLAG_KEY) {
			mestat[i].key++;
			if (packet.stream_index == vidx->vsidx) {
				video_keyframe_credit(vidx, packet.dts);
				vidx->keylib = dts_lib_add(vidx->keylib, packet.dts);
				eznotify(vidx->sysopt, EN_PACKET_KEY, 
						0, 0, &packet);
			}
		}
		if (packet.dts != AV_NOPTS_VALUE) {
			if (packet.dts < mestat[i].dts_last) {
				mestat[i].rewound++;
				mestat[i].dts_base += mestat[i].dts_last;
			}
			mestat[i].dts_last = packet.dts;
		}
		av_free_packet(&packet);
	}
	myntf.varg1 = mestat;
	myntf.varg2 = vidx;
	eznotify(vidx->sysopt, EN_MEDIA_STATIS, imax + 1, 0, &myntf);
	return mestat[vidx->vsidx].dts_base + mestat[vidx->vsidx].dts_last;
}

static int64_t video_snap_point(EZVID *vidx, EZIMG *image, int index)
{
	EZTIME	vpos;
	int64_t	seekat;

	vpos = image->time_from;
	if (!GETFFRAME(vidx->ses_flags)) {
		index++;
	}
	vpos += image->time_step * index;
	
	if (vidx->dur_all) {		/* binding mode */
		vpos -= vidx->dur_off;
		if ((vpos < 0) || (vpos > vidx->duration)) {
			return -1;	/* outside this clip */
		}
	}

	seekat = video_ms_to_dts(vidx, vpos);
	if (vidx->formatx->start_time != AV_NOPTS_VALUE) {
		seekat += video_system_to_dts(vidx, vidx->formatx->start_time);
	}
	return seekat;
}

#if 0
static int64_t video_snap_group_point(EZGRP *vgrp, EZIMG *image, int index)
{
	EZTIME	cur_clip_begin, cur_clip_end;

	/* setup the initial seek position */
	if ((image->sysopt->flags & EZOP_FFRAME) == 0) {
		index++;
	}
	cur_ms = image->time_step * index;
	if (cur_ms > cur_dur_end) {
		close(vidx);
		vidx = video_alloc();
		cur_dur_begin = cur_dur_end;
		cur_dur_end += cur_duration;
	}
	clip_ms = cur_ms - cur_dur_begin;
}
#endif

static int video_snap_begin(EZVID *vidx, EZIMG *image, int method)
{
	/* check if this is called by the first clip */
	if (vidx->dur_all && vidx->dur_off) {
		return 0;
	}

	/* If the output format is the animated GIF89a, then it opens
	 * the target file and device */
	image->gifx_fp = NULL;
	if ((image->gifx_opt = image_cal_gif_animix(image->sysopt)) > 0) {
		/* 20130627 using vidx->filename should be fine here because 
		 * only the first clip would be processed by this function */
		image->gifx_fp = image_gif_anim_open(image, vidx->filename);
	}

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
	dtms = video_dts_to_ms(vidx, ezfrm->rf_dts);
	dtms += vidx->dur_off;		/* aligning the binding clips */
	if (vidx->dur_off == 0) {
		meta_timestamp(dtms, 1, timestamp);
	} else {
		timestamp[0] = '(';
		meta_timestamp(dtms, 1, timestamp + 1);
		strcat(timestamp, ")");
	}

	/* scale the frame into GD frame structure */
	video_frame_scale(vidx, ezfrm->frame);
	image_gdframe_update(image, vidx->swsframe);

	/* write the timestamp into the shot */
	if (image->sysopt->flags & EZOP_TIMEST) {
		image_gdframe_timestamp(image, timestamp);
	}

	if (image->gdcanvas) {
		image_gdcanvas_update(image, image->taken);
	} else if (image->gifx_fp) {
		image_gif_anim_add(image, image->gifx_fp, image->gifx_opt);
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
				(long)(vidx->duration/100), 
				(long)(dtms/100), &dts);
	}
	return 0;
}

static int video_snap_end(EZVID *vidx, EZIMG *image)
{
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

	/* display the end of the process and generate the status line */
	sprintf(status, "%dx%dx%d Thumbnails Generated by Ezthumb %s (%.3f s)",
			image->dst_width, image->dst_height, image->taken,
			EZTHUMB_VERSION, 
			smm_time_diff(&vidx->tmark) / 1000.0);
	eznotify(vidx->sysopt, EN_PROC_END, image->canvas_width, 
			image->canvas_height, status);

	if (image->gdcanvas) {
		/* update the media information area */
		if (image->sysopt->flags & EZOP_INFO) {
			video_media_on_canvas(vidx, image);
			/* Insert as status line */
			image_gdcanvas_print(image, -1, 0, status);
		}
		if (vidx->anchor) {
			image_gdcanvas_save(image, vidx->anchor->filename);
		} else {
			image_gdcanvas_save(image, vidx->filename);
		}
	} else if (image->gifx_fp) {
		image_gif_anim_close(image, image->gifx_fp);
	}
	return 0;
}

static EZFRM *video_frame_next(EZVID *vidx)
{
	vidx->fnow++;
	if (vidx->fnow > 1) {
		vidx->fnow = 0;
	}
	return &vidx->fgroup[vidx->fnow];
}

static EZFRM *video_frame_best(EZVID *vidx, int64_t refdts)
{
	int64_t	dts;
	int	i;

	if ((dts = meta_bestfit(refdts, vidx->fgroup[0].rf_dts,
			vidx->fgroup[1].rf_dts)) < 0) {
		return NULL;	/* no proper frame */
	} 
	
	i = 0;
	if (dts == vidx->fgroup[1].rf_dts) {
		i = 1;
	}

	/*SMM_PRINT("FRAME of %s: %lld in (%lld %lld)\n", 
			i == vidx->fnow ? "Current" : "Previous", dts,
			vidx->fgroup[0].rf_dts, vidx->fgroup[1].rf_dts);*/
	return &vidx->fgroup[i];
}

static int64_t video_decode_next(EZVID *vidx, AVPacket *packet)
{
	EZFRM	*ezfrm;
	int	ffin = 1;

	ezfrm = video_frame_next(vidx);
	ezfrm->rf_dts  = video_packet_timestamp(packet);
	ezfrm->rf_pos  = packet->pos;
	ezfrm->rf_size = 0;
	ezfrm->rf_pac  = 0;

	do {
		eznotify(vidx->sysopt, EN_PACKET_RECV, 0, 0, packet);
		ezfrm->rf_size += packet->size;
		ezfrm->rf_pac++;

		avcodec_decode_video2(vidx->codecx, 
				ezfrm->frame, &ffin, packet);
		if (ffin == 0) {
			/* the packet is not finished */
			eznotify(vidx->sysopt, EN_FRAME_PARTIAL, 
					0, ffin, ezfrm->frame);
			av_free_packet(packet);
			continue;
		}

		eznotify(vidx->sysopt, EN_FRAME_COMPLETE, 
				0, ffin, ezfrm->frame);
		av_free_packet(packet);
		vidx->fdec++;
		return ezfrm->rf_dts;	/* succeeded */

	} while (video_load_packet(vidx, packet) >= 0);
	/* this function should never fail unless end of stream. */
	ezfrm->rf_dts = -1;
	return -1;
}

static int64_t video_decode_keyframe(EZVID *vidx, AVPacket *packet)
{
	int64_t	dts;

	do {
		if ((dts = video_decode_next(vidx, packet)) < 0) {
			break;
		}

		/* A workaroud for B-frame error when investigating the DS9 
		 * The FFMPEG reports:
		 *   [mpeg4 @ 0x91a6a60]Invalid and inefficient vfw-avi packed
		 *   B frames detected
		 * The FFMPEG can not decode this i-frame, perhaps in lack of
		 * previous key frame information. The workaround is continuing
		 * decoding until a proper key frame met */
		/* FF_I_TYPE has been deprecated since 4 Jan 2012. */
#ifdef	FF_I_TYPE
		if (vidx->fgroup[vidx->fnow].frame->pict_type == FF_I_TYPE) {
#else
		if (vidx->fgroup[vidx->fnow].frame->pict_type == 
				AV_PICTURE_TYPE_I) {
#endif
			return dts;	
		}
		eznotify(vidx->sysopt, EN_FRAME_EXCEPTION, 0, 0, 
				vidx->fgroup[vidx->fnow].frame);
	} while (video_load_packet(vidx, packet) >= 0);
	return -1;
}

static int64_t video_decode_to(EZVID *vidx, AVPacket *packet, int64_t dtsto)
{
	int64_t	dts;

	do {
		dts = video_decode_next(vidx, packet);
		if ((dts < 0) || (dts >= dtsto)) {
			return dts;
		}

		eznotify(vidx->sysopt, EN_FRAME_EXCEPTION, 0, 0, 
				vidx->fgroup[vidx->fnow].frame);
	} while (video_load_packet(vidx, packet) >= 0);
	return -1;
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
static int video_rewind(EZVID *vidx)
{
	/* for some reason, to rewind the stream to the head,
	 * av_seek_frame() can not do this by AVSEEK_FLAG_BYTE to 0. */
	//av_seek_frame(vidx->formatx, vidx->vsidx, 0, AVSEEK_FLAG_ANY);
	avformat_seek_file(vidx->formatx, vidx->vsidx, 
			INT64_MIN, 0, INT64_MAX, 0);
	avcodec_flush_buffers(vidx->codecx);
	video_keyframe_credit(vidx, -1);
	return 0;
}

static int video_seeking(EZVID *vidx, int64_t dts)
{
	//av_seek_frame(vidx->formatx, vidx->vsidx, dts, AVSEEK_FLAG_BACKWARD);
	avformat_seek_file(vidx->formatx, vidx->vsidx, 
			INT64_MIN, dts, INT64_MAX, AVSEEK_FLAG_BACKWARD);
	avcodec_flush_buffers(vidx->codecx);
	video_keyframe_credit(vidx, -1);
	return 0;
}

static char *video_media_video(AVStream *stream, char *buffer)
{
	AVCodec	*xcodec;
	char	tmp[128];

	xcodec = avcodec_find_decoder(stream->codec->codec_id);
	if (xcodec == NULL) {
		strcat(buffer, "Unknown Codec");
	} else {
		strcat(buffer, xcodec->long_name);
	}

	sprintf(tmp, ": %dx%d ", stream->codec->width, stream->codec->height);
	strcat(buffer, tmp);
	if (stream->codec->sample_aspect_ratio.num) {
		sprintf(tmp, "AR %d:%d ", 
				stream->codec->sample_aspect_ratio.num,
				stream->codec->sample_aspect_ratio.den);
		strcat(buffer, tmp);
	}

	strcat(buffer, id_lookup_tail(id_pix_fmt, stream->codec->pix_fmt));
	sprintf(tmp, "  %.3f FPS ", (float) stream->r_frame_rate.num / 
			(float) stream->r_frame_rate.den);
	strcat(buffer, tmp);

	if (stream->codec->bit_rate) {
		strcat(buffer, meta_bitrate(stream->codec->bit_rate, tmp));
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

	sprintf(tmp, ": %d-CH  %s %dHz ", stream->codec->channels, 
			id_lookup_tail(id_sample_format, 
					stream->codec->sample_fmt),
			stream->codec->sample_rate);
	strcat(buffer, tmp);

	if (stream->codec->bit_rate) {
		strcat(buffer, meta_bitrate(stream->codec->bit_rate, tmp));
	}
	return buffer;
}

static char *video_media_subtitle(AVStream *stream, char *buffer)
{
	return buffer;
}

static int64_t video_packet_timestamp(AVPacket *packet)
{
	int64_t	dts;

	dts = packet->dts;
	if (dts == AV_NOPTS_VALUE) {
		dts = packet->pts;
	}
	if (dts == AV_NOPTS_VALUE) {
		dts = -1;
	}
	return dts;
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
	int	size, shots;
	int	pro_col, pro_row, pro_width, pro_height, pro_facto;
	int	pro_canvas;
	int	src_width, ar_height;

	// FIXME: the filename could be utf-8 or widebytes
	size = sizeof(EZIMG) + strlen(vidx->filename) + 128;
	if ((image = calloc(size, 1)) == NULL) {
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

	/* calculte the canvas, the screenshots, timestep and the gaps */
	if (pro_col < 1) {	/* we want separated screen shots */
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

	/* enlarge the canvas height to include the media information */
	if ((ezopt->flags & EZOP_INFO) == 0) {
		image->canvas_minfo = 0;
	} else if (image->canvas_height > 0) {
		size = image_gdcanvas_strlen(image, 
				image->sysopt->mi_size, "bqBQ");
		/* we only need the font height plus the gap size */
		size = EZ_LO_WORD(size) + EZ_TEXT_MINFO_GAP;
		/* One rimedge plus media infos */
		image->canvas_minfo = size * (vidx->streams + 2) 
						+ EZ_TEXT_INSET_GAP;
		image->canvas_height += image->canvas_minfo;
		/* Plus the status line: font height + INSET_GAP */
		image->canvas_height += size + EZ_TEXT_INSET_GAP;
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

	eznotify(vidx->sysopt, EN_IMAGE_CREATED, 0, 0, image);
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
	free(image);
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
static int image_gdframe_update(EZIMG *image, AVFrame *swsframe)
{
	unsigned char	*src;
	int	x, y;

	src = swsframe->data[0];
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
		perror(image->filename);
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

	fsize = meta_fontsize(fsize, image->dst_width);
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

static int image_gdframe_puts(EZIMG *image, int fsize, int x, int y, int c, char *s)
{
	int	brect[8];

	//printf("image_gdframe_puts(%dx%dx%d): %s (0x%x)\n", x, y, fsize, s, c);
	fsize = meta_fontsize(fsize, image->dst_width);
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
		perror(image->filename);
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
	int	x, y, ts_width, ts_height;
	
	/* calculate the rectangle size of the string. The height of
	 * the string is fixed to the maximum size */
	x = image_gdcanvas_strlen(image, image->sysopt->mi_size, "bqBQ");
	ts_height = EZ_LO_WORD(x);
	x = image_gdcanvas_strlen(image, image->sysopt->mi_size, s);
	ts_width  = EZ_HI_WORD(x);

	/* we only concern the left, right and center alignment */
	if (row < 0) {
		x = image->sysopt->st_position;
		y = image->canvas_height - ts_height - image->rim_height;
	} else {
		x = image->sysopt->mi_position;
		y = image->rim_height + (ts_height + EZ_TEXT_MINFO_GAP) * row;
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
	fsize = meta_fontsize(fsize, ref);
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

static int image_gdcanvas_puts(EZIMG *image, int fsize, int x, int y, int c, char *s)
{
	int	ref, brect[8];

	ref = image->grid_col ? image->canvas_width / image->grid_col :
		image->canvas_width;
	fsize = meta_fontsize(fsize, ref);
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
#ifdef	CFG_GUI_ON
	if ((fin = g_fopen(image->sysopt->background, "rb")) == NULL) {
#else
	if ((fin = fopen(image->sysopt->background, "rb")) == NULL) {
#endif
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

static FILE *image_gif_anim_open(EZIMG *image, char *filename)
{
	gdImage	*imgif;
	FILE	*fout;

	if ((fout = image_create_file(image, filename, -1)) == NULL) {
		perror(image->filename);
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
	gdImageGifAnimEnd(fout);
	fclose(fout);
	return 0;
}

static FILE *image_create_file(EZIMG *image, char *filename, int idx)
{
	if (ezopt_thumb_name(image->sysopt, image->filename, filename, idx) 
			== EZ_THUMB_SKIP) {
		errno = EEXIST;
		return NULL;	/* skip the existed files */
	}

#ifdef	CFG_GUI_ON
	return g_fopen(image->filename, "wb");
#else
	return fopen(image->filename, "wb");
#endif
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



static int ezopt_thumb_name(EZOPT *ezopt, char *buf, char *fname, int idx)
{
	char	tmp[128], *inbuf = NULL;
	int	i, rc = 0;

	if (buf == NULL) {
		buf = inbuf = malloc(strlen(fname) + 128 + 32);
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
	meta_name_suffix(ezopt->pathout, fname, buf, tmp);

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
		meta_name_suffix(ezopt->pathout, fname, buf, tmp);
	}
	if (i == 256) {
		rc = EZ_THUMB_OVERCOPY;	/* override the last one */
	}
	if (inbuf) {
		free(inbuf);
	}
	//printf("ezopt_thumb_name: %d\n", rc);
	return rc;
}


/****************************************************************************
 * Profile Functions
 ****************************************************************************/

int ezopt_profile_setup(EZOPT *opt, char *s)
{
	char	*tmp, *plist[64];	/* hope that's big enough */
	int	i, len;

	/* duplicate the input profile string */
	if ((tmp = strcpy_alloc(s)) == NULL) {
		return -1;
	}
	
	/* Reset the profile control block pool */
	memset(opt->pro_pool, 0, sizeof(EZPROF) * EZ_PROF_MAX_ENTRY);
	opt->pro_grid = opt->pro_size = NULL;

	len = ziptoken(tmp, plist, sizeof(plist)/sizeof(char*), ":");
	for (i = 0; i < len; i++) {
		ezopt_profile_append(opt, plist[i]);
	}
	
	free(tmp);
	return 0;
}

/* for debug purpose only */
int ezopt_profile_dump(EZOPT *opt, char *pmt_grid, char *pmt_size)
{
	EZPROF	*node;
	char	tmp[64];

	printf("%s", pmt_grid);		/* "Grid: " */
	for (node = opt->pro_grid; node != NULL; node = node->next) {
		printf("%s ", ezopt_profile_sprint(node, tmp, sizeof(tmp)));
	}
	printf("\n");

	printf("%s", pmt_size);		/* "Size: " */
	for (node = opt->pro_size; node != NULL; node = node->next) {
		printf("%s ", ezopt_profile_sprint(node, tmp, sizeof(tmp)));
	}
	printf("\n");
	return 0;
}

char *ezopt_profile_export_alloc(EZOPT *ezopt)
{
	EZPROF	*p;
	char	*buf, tmp[64];
	int	n = 0;

	for (p = ezopt->pro_grid; p; p = p->next, n++);
	for (p = ezopt->pro_size; p; p = p->next, n++);
	if ((buf = calloc(n, 64)) == NULL) {
		return NULL;
	}
	buf[0] = 0;

	for (p = ezopt->pro_grid; p; p = p->next) {
		ezopt_profile_sprint(p, tmp, sizeof(tmp));
		if (buf[0] == 0) {
			strcpy(buf, tmp);
		} else {
			strcat(buf, ":");
			strcat(buf, tmp);
		}
	}
	for (p = ezopt->pro_size; p; p = p->next) {
		ezopt_profile_sprint(p, tmp, sizeof(tmp));
		if (buf[0] == 0) {
			strcpy(buf, tmp);
		} else {
			strcat(buf, ":");
			strcat(buf, tmp);
		}
	}
	//printf("ezopt_profile_export: %s\n", buf);
	return buf;
}

int ezopt_profile_disable(EZOPT *ezopt, int prof)
{
	if (prof == EZ_PROF_LENGTH) {
		ezopt->pro_grid = NULL;
	} else if (prof == EZ_PROF_WIDTH) {
		ezopt->pro_size = NULL;
	} else if (prof == EZ_PROF_ALL) {
		ezopt->pro_grid = NULL;
		ezopt->pro_size = NULL;
	}
	return 0;
}

int ezopt_profile_sampling(EZOPT *ezopt, int vidsec, int *col, int *row)
{
	EZPROF	*node;
	int	snap;

	if (ezopt->pro_grid == NULL) {
		return -1;	/* profile disabled */
	}

	for (node = ezopt->pro_grid; node->next; node = node->next) {
		if (vidsec <= node->weight) {
			break;
		}
	}

	switch (node->flag) {
	case 's':
	case 'S':
	case 'm':
	case 'M':
		if (col) {
			*col = node->x;
		}
		if (row) {
			*row = node->y;
		}
		//printf("ezopt_profile_sampling: %d x %d\n", node->x, node->y);
		return 0;	/* returned the matrix */

	case 'l':
	case 'L':
		snap = (int)(log(vidsec / 60 + node->x) / log(node->lbase)) 
			- node->y;
		//printf("ezopt_profile_sampling: %d+\n", snap);
		return snap;	/* need more info to decide the matrix */
	}
	return -2;	/* no effective profile found */
}

int ezopt_profile_sampled(EZOPT *ezopt, int vw, int bs, int *col, int *row)
{
	EZPROF	*node;

	if (ezopt->pro_size == NULL) {
		return bs;	/* profile disabled so ignore it */
	}

	for (node = ezopt->pro_size; node->next; node = node->next) {
		if (vw <= node->weight) {
			break;
		}
	}

	switch (node->flag) {
	case 'f':
	case 'F':
	case 'r':
	case 'R':
		if (col) {
			*col = node->x;
		}
		bs = (bs + node->x - 1) / node->x * node->x;
		if (row) {
			*row = bs / node->x;
		}
		break;
	}
	//printf("ezopt_profile_sampled: %d\n", bs);
	return bs;
}


int ezopt_profile_zooming(EZOPT *ezopt, int vw, int *wid, int *hei, int *ra)
{
	EZPROF	*node;
	float	ratio;
	int	neari;

	if (ezopt->pro_size == NULL) {
		return 0;	/* profile disabled so ignore it */
	}

	for (node = ezopt->pro_size; node->next; node = node->next) {
		if (vw <= node->weight) {
			break;
		}
	}

	switch (node->flag) {
	case 'w':
	case 'W':
		if (ra) {
			*ra = node->x;
		}
		break;
	
	case 't':
	case 'T':
		if (wid) {
			*wid = node->x;
		}
		if (hei) {
			*hei = node->y;
		}
		break;

	case 'f':
	case 'F':
		return node->y;	/* return the canvas size if required */

	case 'r':
	case 'R':
		/* find the zoom ratio */
		if (vw > node->y) {
			ratio = (float) vw / node->y;	/* zoom out */
		} else {
			ratio = (float) node->y / vw;	/* zoom in */
		}
		/* quantized the zoom ratio to base-2 exponential 1,2,4,8.. */
		neari = 1 << (int)(log(ratio) / log(2) + 0.5);
		//printf("ez.._zooming: ratio=%f quant=%d\n", ratio, neari);
		/* checking the error between the reference width and the 
		 * zoomed width. The error should be less than 20% */
		ratio = abs(neari * node->y - vw) / (float) node->y;
		if (ratio > 0.2) {
			neari = node->y;	/* error over 20% */
		} else if (vw > node->y) {
			neari = vw / neari;
		} else {
			neari = vw * neari;
		}
		if (wid) {
			*wid = neari / 2 * 2;	/* final quantizing width */
		}
		break;
	}
	return 0;
}


/* available profile field example:
 * 12M4x6, 720s4x6, 720S4
 * 160w200%, 320w100%, 320w160x120, 320w160 */
static int ezopt_profile_append(EZOPT *ezopt, char *ps)
{
	EZPROF	*node;
	char	pbuf[256], *argv[8];
	int	val;

	strncpy_safe(pbuf, ps, sizeof(pbuf));

	val = (int) strtol(pbuf, &ps, 10);
	if (*ps == 0) {	/* pointed to the EOL; no flag error */
		return -1;
	}

	fixtoken(ps + 1, argv, sizeof(argv)/sizeof(char*), "xX");
	node = ezopt_profile_new(ezopt, *ps, val);

	switch (node->flag) {
	case 'm':
	case 'M':
		node->weight *= 60;	/* turn to seconds */
		node->x = (int) strtol(argv[0], NULL, 10);
		node->y = argv[1] ? (int) strtol(argv[1], NULL, 10) : 0;
		ezopt->pro_grid = ezopt_profile_insert(ezopt->pro_grid, node);
		break;

	case 's':
	case 'S':
		node->x = (int) strtol(argv[0], NULL, 10);
		node->y = argv[1] ? (int) strtol(argv[1], NULL, 10) : 0;
		ezopt->pro_grid = ezopt_profile_insert(ezopt->pro_grid, node);
		break;

	case 'l':
	case 'L':
		if ((argv[1] == NULL) || (argv[2] == NULL)) {
			ezopt_profile_free(node);
			return -2;
		}
		node->weight *= 60;	/* turn to seconds */
		node->x = (int) strtol(argv[0], NULL, 10);
		node->y = (int) strtol(argv[1], NULL, 10);
		node->lbase = (float) strtof(argv[2], NULL);
		ezopt->pro_grid = ezopt_profile_insert(ezopt->pro_grid, node);
		break;

	case 'w':
	case 'W':
		node->x = (int) strtol(argv[0], NULL, 10);
		ezopt->pro_size = ezopt_profile_insert(ezopt->pro_size, node);
		break;

	case 't':
	case 'T':
		node->x = (int) strtol(argv[0], NULL, 10);
		node->y = argv[1] ? (int) strtol(argv[1], NULL, 10) : 0;
		ezopt->pro_size = ezopt_profile_insert(ezopt->pro_size, node);
		break;
	
	case 'f':
	case 'F':
	case 'r':
	case 'R':
		node->x = (int) strtol(argv[0], NULL, 10);
		if (argv[1] == NULL) {
			ezopt_profile_free(node);
			return -2;
		}
		node->y = (int) strtol(argv[1], NULL, 10);
		if (node->y <= 0) {
			ezopt_profile_free(node);
			return -2;
		}
		ezopt->pro_size = ezopt_profile_insert(ezopt->pro_size, node);
		break;

	default:
		ezopt_profile_free(node);
		return -2;
	}
	return 0;
}

static char *ezopt_profile_sprint(EZPROF *node, char *buf, int blen)
{
	char	tmp[64];

	if (blen < 32) {	/* FIXME: very rough estimation */
		return NULL;
	}

	switch (node->flag) {
	case 'm':
	case 'M':
		sprintf(buf, "%dM%d", node->weight / 60, node->x);
		if (node->y > 0) {
			sprintf(tmp, "x%d", node->y);
			strcat(buf, tmp);
		}
		break;

	case 's':
	case 'S':
		sprintf(buf, "%dS%d", node->weight, node->x);
		if (node->y > 0) {
			sprintf(tmp, "x%d", node->y);
			strcat(buf, tmp);
		}
		break;

	case 'l':
	case 'L':
		sprintf(buf, "%dL%dx%dx%f", node->weight / 60, 
				node->x, node->y, node->lbase);
		break;

	case 'w':
	case 'W':
		sprintf(buf, "%dW%d%%", node->weight, node->x);
		break;

	case 't':
	case 'T':
		sprintf(buf, "%dT%d", node->weight, node->x);
		if (node->y > 0) {
			sprintf(tmp, "x%d", node->y);
			strcat(buf, tmp);
		}
		break;

	case 'f':
	case 'F':
		sprintf(buf, "%dF%dx%d", node->weight, node->x, node->y);
		break;

	case 'r':
	case 'R':
		sprintf(buf, "%dR%dx%d", node->weight, node->x, node->y);
		break;

	default:
		return NULL;
	}
	return buf;
}

static EZPROF *ezopt_profile_new(EZOPT *opt, int flag, int wei)
{
	int	i;

	for (i = 0; i < EZ_PROF_MAX_ENTRY; i++) {
		if (opt->pro_pool[i].flag == 0) {
			break;
		}
	}
	if (i == EZ_PROF_MAX_ENTRY) {
		return NULL;
	}

	memset(&opt->pro_pool[i], 0, sizeof(EZPROF));
	opt->pro_pool[i].next = NULL;
	opt->pro_pool[i].flag = flag;
	opt->pro_pool[i].weight = wei;

	return &opt->pro_pool[i];
}

/* note that it's NOT a proper free since the 'next' point was not processed
 * at all. It only serves as a quick release */
static int ezopt_profile_free(EZPROF *node)
{
	memset(node, 0, sizeof(EZPROF));
	return 0;
}

static EZPROF *ezopt_profile_insert(EZPROF *root, EZPROF *leaf)
{
	EZPROF	*prev, *now;

	if (leaf == NULL) {
		return root;	/* do nothing */
	}

	//printf("ezopt_profile_insert: %d\n", leaf->weight);
	if (root == NULL) {
		return leaf;
	}
	if (root->weight > leaf->weight) {
		leaf->next = root;
		return leaf;
	}
	prev = root;
	for (now = root->next; now != NULL; prev = now, now = now->next) {
		if (now->weight > leaf->weight) {
			break;			
		}
	}
	if (now == NULL) {
		prev->next = leaf;
	} else {
		leaf->next = prev->next;
		prev->next = leaf;
	}
	return root;
}


/****************************************************************************
 * Statistical Analysis of DTS stamps in the stream
 ****************************************************************************/

static void *dts_lib_free(void *anchor)
{
	struct	DTSLIB	*lp, *now;

	lp = anchor; 
	while (lp) {
		//printf("dts_lib_free: %d\n", lp->num);
		now = lp;
 		lp = lp->next;
		free(now);
	}
	return NULL;
}

static void *dts_lib_add(void *anchor, int64_t dts)
{
	struct	DTSLIB	*lp, *block;

	for (lp = anchor; lp; lp = lp->next) {
		if (lp->num < MAX_DTS_LIB) {
			lp->dts[lp->num++] = dts;
			return anchor;
		}
	}
	
	if ((block = calloc(sizeof(struct DTSLIB), 1)) == NULL) {
		dts_lib_free(anchor);
		return NULL;
	}

	block->dts[block->num++] = dts;
	if (anchor == NULL) {
		return block;
	}
	for (lp = anchor; lp->next; lp = lp->next);
	lp->next = block;
	return anchor;
}


/* input:  refdts[0] = reference DTS 0;    refdts[1] = 0;    refdts[2] = 0
 *         refdts[3] = reference DTS 1;    refdts[4] = 0;    refdts[5] = 0
 *         ...
 * output: refdts[0] = reference DTS 0;    refdts[1] = KEY1; refdts[2] = KEY2
 *         refdts[3] = reference DTS 1;    refdts[4] = KEY3; refdts[5] = KEY4
 *         ...
 */
static int dts_lib_compress(void *anchor, int64_t *refdts, int num)
{
	struct	DTSLIB	*lp;
	int64_t	prev = 0;
	int	i, n;

	n = 0;
	for (lp = anchor; lp; lp = lp->next) {
		for (i = 0; i < lp->num; i++) {
			while (lp->dts[i] >= refdts[n*3]) {
				refdts[n*3+1] = prev;
				refdts[n*3+2] = lp->dts[i];
				n++;
				if (n >= num) {
					return n;
				}
			}
			prev = lp->dts[i];
		}
	}
	return n;
}


/****************************************************************************
 * Utility Functions
 ****************************************************************************/

/* this function is used to convert the PTS from the video stream
 * based time to the millisecond based time. The formula is:
 *   MS = (PTS * s->time_base.num / s->time_base.den) * 1000
 * then
 *   MS =  PTS * 1000 * s->time_base.num / s->time_base.den */
EZTIME video_dts_to_ms(EZVID *vidx, int64_t dts)
{
	AVStream	*s = vidx->formatx->streams[vidx->vsidx];

	/* the 'start_time' in the AVFormatContext should be offsetted from
	 * the video stream's PTS */
	if (vidx->formatx->start_time != AV_NOPTS_VALUE) {
		dts -= video_system_to_dts(vidx, vidx->formatx->start_time);
	}

	return (EZTIME) av_rescale(dts * 1000, 
			s->time_base.num, s->time_base.den);
}

/* this function is used to convert the timestamp from the millisecond 
 * based time to the video stream based PTS time. The formula is:
 *   PTS = (ms / 1000) * s->time_base.den / s->time_base.num
 * then
 *   PTS = ms * s->time_base.den / (s->time_base.num * 1000) */
int64_t video_ms_to_dts(EZVID *vidx, EZTIME ms)
{
	AVStream	*s = vidx->formatx->streams[vidx->vsidx];

	return av_rescale((int64_t) ms, s->time_base.den, 
			(int64_t) s->time_base.num * (int64_t) 1000);
}

/* this function is used to convert the PTS from the video stream
 * based time to the default system time base (microseconds). The formula is:
 *   SYS = (PTS * s->time_base.num / s->time_base.den) * AV_TIME_BASE
 * then
 *   SYS = PTS * AV_TIME_BASE * s->time_base.num / s->time_base.den  */
int64_t video_dts_to_system(EZVID *vidx, int64_t dts)
{
	AVStream	*s = vidx->formatx->streams[vidx->vsidx];

	return av_rescale(dts * (int64_t) AV_TIME_BASE,
			s->time_base.num, s->time_base.den);
}

/* this function is used to convert the timestamp from the default 
 * system time base (microsecond) to the millisecond based time. The formula:
 *   PTS = (SYS / AV_TIME_BASE) * s->time_base.den / s->time_base.num 
 * then
 *   PTS = SYS * s->time_base.den / (s->time_base.num * AV_TIME_BASE) */
int64_t video_system_to_dts(EZVID *vidx, int64_t sysdts)
{
	AVStream	*s = vidx->formatx->streams[vidx->vsidx];

	return av_rescale(sysdts, s->time_base.den, 
			(int64_t) s->time_base.num * (int64_t) AV_TIME_BASE);
}

char *meta_bitrate(int bitrate, char *buffer)
{
	static	char	tmp[32];

	if (buffer == NULL) {
		buffer = tmp;
	}
	sprintf(buffer, "%.3f kbps", (float)bitrate / 1000.0);
	return buffer;
}

char *meta_filesize(int64_t size, char *buffer)
{
	static	char	tmp[32];

	if (buffer == NULL) {
		buffer = tmp;
	}
	if (size < (1ULL << 20)) {
		sprintf(buffer, "%lu KB", (unsigned long)(size >> 10));
	} else if (size < (1ULL << 30)) {
		sprintf(buffer, "%lu.%lu MB", (unsigned long)(size >> 20), 
				(unsigned long)(((size % (1 << 20)) >> 10) / 100));
	} else {
		sprintf(buffer, "%lu.%03lu GB", (unsigned long)(size >> 30), 
				(unsigned long)(((size % (1 << 30)) >> 20) / 100));
	}
	return buffer;
}

char *meta_timestamp(EZTIME ms, int enms, char *buffer)
{
	static	char	tmp[32];
	int	hour, min, sec, msec;

	if (buffer == NULL) {
		buffer = tmp;
	}

	hour = ms / 3600000;
	ms   = ms % 3600000;
	min  = ms / 60000;
	ms   = ms % 60000;
	sec  = ms / 1000;
	msec = ms % 1000;
	if (enms) {
		sprintf(buffer, "%d:%02d:%02d,%03d", hour, min, sec, msec);
	} else {
		sprintf(buffer, "%d:%02d:%02d", hour, min, sec);
	}
	return buffer;
}

int meta_fontsize(int fsize, int refsize)
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

// FIXME: UTF-8 and widechar?
char *meta_basename(char *fname, char *buffer)
{
	static	char	tmp[1024];
	char	*p;

	if (buffer == NULL) {
		buffer = tmp;
	}

	if ((p = strrchr(fname, '/')) == NULL) {
		strcpy(buffer, fname);
	} else {
		strcpy(buffer, p + 1);
	}
	return buffer;
}

// FIXME: UTF-8 and widechar?
char *meta_name_suffix(char *path, char *fname, char *buf, char *sfx)
{
	static	char	tmp[1024];
	char	*p;

	if (buf == NULL) {
		buf = tmp;
	}

	if (!path || !*path) {
		strcpy(buf, fname);
	} else {
		strcpy(buf, path);
		if (buf[strlen(buf)-1] != '/') {
			strcat(buf, "/");
		}
		if ((p = strrchr(fname, '/')) == NULL) {
			strcat(buf, fname);
		} else {
			strcat(buf, p+1);
		}
	}
	if ((p = strrchr(buf, '.')) != NULL) {
		*p = 0;
	}
	strcat(buf, sfx);
	return buf;
}

int64_t meta_bestfit(int64_t ref, int64_t v1, int64_t v2)
{
	int64_t	c1, c2;

	if (v1 < 0) {
		return v2;
	}
	if (v2 < 0) {
		return v1;
	}
	if (ref < 0) {
		return v1 > v2 ? v1 : v2;
	}
	
	c1 = (ref > v1) ? ref - v1 : v1 - ref;
	c2 = (ref > v2) ? ref - v2 : v2 - ref;
	if (c1 < c2) {
		return v1;
	}
	return v2;
}

/* input: jpg@85, gif@1000, png */
int meta_image_format(char *input, char *fmt, int flen)
{
	char	*p, arg[128];
	int	quality = 0;

	strncpy_safe(arg, input, sizeof(arg));
	if ((p = strchr(arg, '@')) != NULL) {
		*p++ = 0;
		quality = strtol(p, NULL, 0);
	}
	strncpy_safe(fmt, arg, flen);

	/* foolproof of the quality parameter */
	if (!strcmp(fmt, "jpg") || !strcmp(fmt, "jpeg")) {
		if ((quality < 5) || (quality > 100)) {
			quality = 85;	/* as default */
		}
	} else if (!strcmp(fmt, "png")) {
		quality = 0;
	} else if (!strcmp(fmt, "gif")) {
		if (quality && (quality < 15)) {
			quality = 1000;	/* 1 second as default */
		}
	} else {
		strcpy(fmt, "jpg");	/* as default */
		quality = 85;
	}
	return quality;
}

char *strcpy_alloc(const char *src)
{
	char	*dest;
	
	if ((dest = malloc(strlen(src) + 4)) == NULL) {
		return NULL;
	}
	return strcpy(dest, src);
}

char *strncpy_safe(char *dest, const char *src, size_t n)
{
	strncpy(dest, src, n - 1);
	dest[n - 1] = 0;
	return dest;
}



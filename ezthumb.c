
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


static int64_t video_keyframe_next(EZVID *vidx, AVPacket *packet);
static int64_t video_keyframe_to(EZVID *vidx, AVPacket *packet, int64_t pos);
static int64_t *video_keyframe_survey(EZVID *vidx, EZIMG *image);
static int video_keyframe_credit(EZVID *vidx, int64_t dts);
static int64_t video_keyframe_seekat(EZVID *vidx, AVPacket *packet, int64_t);
static int64_t video_load_packet(EZVID *vidx, AVPacket *packet);
static int video_seekable_random(EZVID *vidx, EZIMG *image);
static int video_media_on_canvas(EZVID *vidx, EZIMG *image);
static int video_find_stream(EZVID *vidx, int flags);
static int video_duration(EZVID *vidx, int scanmode);
static int64_t video_statistics(EZVID *vidx);
static int64_t video_snap_point(EZVID *vidx, EZIMG *image, int index);
static int video_snap_begin(EZVID *vidx, EZIMG *image);
static int video_snap_update(EZVID *vidx, EZIMG *image, AVFrame *frame, 
		int sn, int64_t dts);
static int video_snap_end(EZVID *vidx, EZIMG *image);
static int64_t video_decode_packet(EZVID *vidx, AVFrame *frame, AVPacket *);
static int64_t video_decode_to(EZVID *vidx, AVFrame *frame, 
		AVPacket *packet, int64_t dtsto);
static int video_rewind(EZVID *vidx);
static int video_seeking(EZVID *vidx, int64_t dts);
static int image_scale(EZIMG *image, AVFrame *frame);
static int image_gdframe_update(EZIMG *image);
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
static int ezopt_cal_ratio(int ratio, int refsize);
static int ezopt_cal_shots(int duration, int tmstep, int mode);
static int ezopt_cal_timestep(int duration, int shots, int mode);
static int ezopt_gif_anim(EZOPT *ezopt);
static char *minfo_video(AVStream *stream, char *buffer);
static char *minfo_audio(AVStream *stream, char *buffer);
static char *minfo_subtitle(AVStream *stream, char *buffer);
static char *meta_bitrate(int bitrate, char *buffer);
static char *meta_filesize(int64_t size, char *buffer);
static char *meta_timestamp(int ms, int enms, char *buffer);
static int meta_fontsize(int fsize, int refsize);
static gdFont *meta_fontset(int fsize);
static char *meta_basename(char *fname, char *buffer);
static char *meta_name_suffix(char *path, char *fname, char *buf, char *sfx);
static int meta_copy_image(gdImage *dst, gdImage *src, int x, int, int, int);
static int64_t meta_packet_timestamp(AVPacket *packet);


void ezopt_init(EZOPT *ezopt)
{
	memset(ezopt, 0, sizeof(EZOPT));
	ezopt->grid_col = 4;	/* the default thumbnail is a 4x4 array */
	ezopt->grid_row = 4;
	ezopt->canvas_width = 0;

	/* enable media info area and inset timestamp, skip the first and the
	 * last frame, no shadows */
	ezopt->flags = EZOP_INFO | EZOP_TIMEST;

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
	ezopt->vs_idx = -1;	/* default: first found video stream */
}

int ezthumb(char *filename, EZOPT *ezopt)
{
	AVFrame	*frame = NULL;
	EZIMG	*image;
	EZVID	*vidx;
	int	rc;

	if ((vidx = video_allocate(filename, ezopt, &rc)) == NULL) {
		return rc;
	}
	if ((image = image_allocate(vidx, ezopt, &rc)) == NULL) {
		video_free(vidx);
		return rc;
	}

	/* allocate a video frame structure here so it can be reused in
	 * the future */
	if ((frame = avcodec_alloc_frame()) == NULL) {
		image_free(image);
		video_free(vidx);
		return EZ_ERR_LOWMEM;
	}

	/*************************************************************
	 * if the expected time_step is 0, then it will save every 
	 * key frames separately. it's good for debug purpose 
	 *************************************************************/
	if (image->time_step < 1) {	
		/* save every keyframe separately */
		video_snapshot_keyframes(vidx, image, frame);
		av_free(frame);
		image_free(image);
		video_free(vidx);
		return EZ_ERR_NONE;
	}

	/*************************************************************
	 * Otherwise it takes expected shots
	 *************************************************************/
	
	vidx->keydelta = video_ms_to_dts(vidx, image->time_step);
	vidx->keyfirst = -1;
	video_keyframe_credit(vidx, -1);

	switch (ezopt->flags & EZOP_PROC_MASK) {
	case EZOP_PROC_SKIM:
		video_snapshot_skim(vidx, image, frame);
		break;
	case EZOP_PROC_SCAN:
		video_snapshot_scan(vidx, image, frame);
		break;
	case EZOP_PROC_TWOPASS:
		video_snapshot_twopass(vidx, image, frame);
		break;
	case EZOP_PROC_HEURIS:
		video_snapshot_heuristic(vidx, image, frame);
		break;
	default:
		if (video_seekable_random(vidx, image) == ENX_SEEK_BW_YES) {
			video_snapshot_skim(vidx, image, frame);
		} else if (ezopt->flags & EZOP_P_FRAME) {
			video_snapshot_twopass(vidx, image, frame);
		} else {
			video_snapshot_scan(vidx, image, frame);
		}
		break;
	}

	av_free(frame);
	image_free(image);
	video_free(vidx);
	return EZ_ERR_NONE;
}

int ezinfo(char *filename, EZOPT *ezopt)
{
	EZVID		*vidx;
	EZIMG		*image;
	int		rc;

	rc = EZ_ERR_NONE;
	if ((vidx = video_allocate(filename, ezopt, &rc)) == NULL) {
		return rc;
	}

	if (ezopt->flags & EZOP_CLI_DEBUG) {
		if ((image = image_allocate(vidx, ezopt, &rc)) != NULL) {
			//dump_ezimage(image);
			//image_font_test(image, vidx->filename);
			image_free(image);
		}
	}

	video_free(vidx);
	return rc;
}


EZVID *video_allocate(char *filename, EZOPT *ezopt, int *errcode)
{
	EZVID	*vidx;
	int	rc;

	/* allocate the runtime index structure of the video */
	if ((vidx = malloc(sizeof(EZVID))) == NULL) {
		uperror(errcode, EZ_ERR_LOWMEM);
		return NULL;
	}

	memset(vidx, 0, sizeof(EZVID));
	vidx->sysopt   = ezopt;
	vidx->filename = filename;	/* keep a copy of the filename */
	vidx->keyfirst = -1;
	gettimeofday(&vidx->tmark, NULL);	/* get current time */

#if	(LIBAVFORMAT_VERSION_MAJOR > 51) && (LIBAVFORMAT_VERSION_MINOR > 109)
	if (avformat_open_input(&vidx->formatx, filename, NULL, NULL) != 0) {
#else
	if (av_open_input_file(&vidx->formatx, filename, NULL, 0, NULL) < 0) {
#endif
		uperror(errcode, EZ_ERR_FORMAT);
		eznotify(vidx, EZ_ERR_FORMAT, 0, 0, filename);
		free(vidx);
		return NULL;
	}

	/* FIXME: what are these for? */
	vidx->formatx->flags |= AVFMT_FLAG_GENPTS;
	if (av_find_stream_info(vidx->formatx) < 0) {
		uperror(errcode, EZ_ERR_STREAM);
		eznotify(vidx, EZ_ERR_STREAM, 0, 0, filename);
		video_free(vidx);
		return NULL;
	}
	
	/* find the video stream and open the codec driver */
	if ((rc = video_find_stream(vidx, ezopt->flags)) != EZ_ERR_NONE) {
		uperror(errcode, rc);
		eznotify(vidx, EZ_ERR_VIDEOSTREAM, 0, 0, filename);
		video_free(vidx);
		return NULL;
	}

	eznotify(vidx, EN_FILE_OPEN, 0, 
			meta_time_diff(&vidx->tmark), filename);


	/* find out the clip duration in millisecond */
	/* 20110301: the still images are acceptable by the ffmpeg library
	 * so it would be wiser to avoid the still image stream, which duration
	 * is only several milliseconds. FIXME if this assumption is wrong */
	if (video_duration(vidx, ezopt->dur_mode) < 500) {
		uperror(errcode, EZ_ERR_FILE);
		eznotify(vidx, EZ_ERR_VIDEOSTREAM, 1, 0, filename);
		video_free(vidx);
		return NULL;
	}
	eznotify(vidx, EN_MEDIA_OPEN, 0, 
			meta_time_diff(&vidx->tmark), filename);
	return vidx;
}

int video_free(EZVID *vidx)
{
	if (vidx->codecx) {
		avcodec_close(vidx->codecx);
	}
	if (vidx->formatx) {
		av_close_input_file(vidx->formatx);
	}
	free(vidx);
	return EZ_ERR_NONE;
}

/* This function is used to save every key frames in the video clip
 * into individual files. */
int video_snapshot_keyframes(EZVID *vidx, EZIMG *image, AVFrame *frame)
{
	AVPacket	packet;
	int64_t		dts, dts_from, dts_to;
	int		i;

	/* set up the border */
	dts_from = video_system_to_dts(vidx, vidx->formatx->start_time);
	dts_from += video_ms_to_dts(vidx, image->time_from);
	dts_to = dts_from + video_ms_to_dts(vidx, image->time_during);

	eznotify(vidx, EN_SNAP_SHOT, ENX_SS_IFRAMES, 0, NULL);
	video_snap_begin(vidx, image);

	i = 0;
	video_keyframe_credit(vidx, -1);
	while ((dts = video_keyframe_next(vidx, &packet)) >= 0) {
		if (dts < dts_from) {
			av_free_packet(&packet);
			continue;
		}
		if (dts > dts_to) {
			av_free_packet(&packet);
			break;
		}

		if (video_decode_packet(vidx, frame, &packet) < 0) {
			break;
		}
		eznotify(vidx, EN_FRAME_EFFECT, i, (long)&packet, frame);
		av_free_packet(&packet);

		video_snap_update(vidx, image, frame, i, dts);
		i++;
	}
	video_snap_end(vidx, image);
	return EZ_ERR_NONE;
}

/* for these conditions: backward seeking available, key frame only,
 * snap interval is larger than maximum key frame interval and no rewind
 * clips */
int video_snapshot_skim(EZVID *vidx, EZIMG *image, AVFrame *frame)
{
	AVPacket	packet;
	int64_t		dts, dts_snap, last_key;
	int		i;

	eznotify(vidx, EN_SNAP_SHOT, ENX_SS_SKIM, 0, NULL);
	video_snap_begin(vidx, image);
	for (i = last_key = 0; i < image->shots; i++) {
		dts_snap = video_snap_point(vidx, image, i);

		video_seeking(vidx, dts_snap);

		if ((dts = video_keyframe_next(vidx, &packet)) < 0) {
			break;
		}

		if (image->sysopt->flags & EZOP_P_FRAME) {
			last_key = dts;
			dts = video_decode_to(vidx, frame, &packet, dts_snap);
		} else if (dts == last_key) {
			dts = video_decode_to(vidx, frame, &packet, dts_snap);
		} else {
			last_key = dts;
			dts = video_decode_packet(vidx, frame, &packet);
		}
		if (dts < 0) {
			break;
		}

		eznotify(vidx, EN_FRAME_EFFECT, i, (long)&packet, frame);
		av_free_packet(&packet);

		video_snap_update(vidx, image, frame, i, dts);
	}
	if (i < image->shots) {
		eznotify(vidx, EN_STREAM_BROKEN, i, image->shots, NULL);
	}
	video_snap_end(vidx, image);
	return EZ_ERR_NONE;
}


/* for these conditions: Though backward seeking is NOT available but it is 
 * required to extract key frame only. snap interval is larger than maximum 
 * key frame interval and no rewind clips */
int video_snapshot_scan(EZVID *vidx, EZIMG *image, AVFrame *frame)
{
	AVPacket	packet;
	int64_t		dts, dts_snap, last_key = -1;
	int		i;

	eznotify(vidx, EN_SNAP_SHOT, ENX_SS_SCAN, 0, NULL);
	video_snap_begin(vidx, image);
	for (i = dts = 0; i < image->shots; i++) {
		dts_snap = video_snap_point(vidx, image, i);

		if (dts < dts_snap) {
			dts = video_keyframe_to(vidx, &packet, dts_snap);
			if (dts < 0) {
				break;
			}
			dts = video_decode_packet(vidx, frame, &packet);
			if (dts < 0) {
				last_key = -1;
				break;
			}
			eznotify(vidx, EN_FRAME_EFFECT, i, (long)&packet, frame); 
			av_free_packet(&packet);
		}
		video_snap_update(vidx, image, frame, i, dts);
		last_key = dts;
	}
	if (i < image->shots) {
		eznotify(vidx, EN_STREAM_BROKEN, i, image->shots, NULL);
		for ( ; (i < image->shots) && (last_key >= 0); i++) {
			video_snap_update(vidx, image, frame, i, last_key);
		}
	}
	video_snap_end(vidx, image);
	return EZ_ERR_NONE;
}

/* for these conditions: Though backward seeking is NOT available and it is 
 * required to extract p-frames. */
int video_snapshot_twopass(EZVID *vidx, EZIMG *image, AVFrame *frame)
{
	AVPacket	packet;
	int64_t		*keylist;
	int64_t		dts, dts_snap, last_key = -1;
	int		i;

	eznotify(vidx, EN_SNAP_SHOT, ENX_SS_TWOPASS, 0, NULL);	
	if ((keylist = video_keyframe_survey(vidx, image)) == NULL) {
		return EZ_ERR_LOWMEM;
	}

	video_snap_begin(vidx, image);
	for (i = dts = 0; i < image->shots; i++) {
		/* find the snap point to shoot */
		dts_snap = video_snap_point(vidx, image, i);
		if ((image->sysopt->flags & EZOP_P_FRAME) == 0) {
			/* if more than one shots inside a key frame,
			 * we will take p-frames instead even the 
			 * EZOP_P_FRAME was not set */
			if (i && (keylist[i] > keylist[i-1])) {
				dts_snap = keylist[i];
			}
		}

		if (dts < keylist[i]) {
			dts = video_keyframe_to(vidx, &packet, keylist[i]);
		} else {
			dts = video_load_packet(vidx, &packet);
		}
		if (dts < 0) {
			break;
		}

		dts = video_decode_to(vidx, frame, &packet, dts_snap);
		if (dts < 0) {
			last_key = -1;
			break;
		}

		eznotify(vidx, EN_FRAME_EFFECT, i, (long)&packet, frame);
		av_free_packet(&packet);

		video_snap_update(vidx, image, frame, i, dts);
		last_key = dts;
	}
	if (i < image->shots) {
		eznotify(vidx, EN_STREAM_BROKEN, i, image->shots, NULL);
		for ( ; (i < image->shots) && (last_key >= 0); i++) {
			video_snap_update(vidx, image, frame, i, last_key);
		}
	}
	video_snap_end(vidx, image);
	free(keylist);
	return EZ_ERR_NONE;
}

int video_snapshot_heuristic(EZVID *vidx, EZIMG *image, AVFrame *frame)
{
	AVPacket	packet;
	int64_t		dts, dts_snap, last_key = -1;
	int		i;

	eznotify(vidx, EN_SNAP_SHOT, ENX_SS_HEURIS, 0, NULL);
	video_snap_begin(vidx, image);
	for (i = dts = 0; i < image->shots; i++) {
		dts_snap = video_snap_point(vidx, image, i);
		/*printf("Measuring: Snap=%lld Cur=%lld  Dis=%lld Gap=%lld\n",
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
			dts = video_decode_packet(vidx, frame, &packet);
		} else {
			dts = video_decode_to(vidx, frame, &packet, dts_snap);
		}
		if (dts < 0) {
			last_key = -1;
			break;
		}

		eznotify(vidx, EN_FRAME_EFFECT, i, (long)&packet, frame);
		av_free_packet(&packet);
		video_snap_update(vidx, image, frame, i, dts);
		last_key = dts;
	}
	if (i < image->shots) {
		eznotify(vidx, EN_STREAM_BROKEN, i, image->shots, NULL);
		for ( ; (i < image->shots) && (last_key >= 0); i++) {
			video_snap_update(vidx, image, frame, i, last_key);
		}
	}
	video_snap_end(vidx, image);
	return EZ_ERR_NONE;
}


/* this function is used to convert the PTS from the video stream
 * based time to the millisecond based time. The formula is:
 *   MS = (PTS * s->time_base.num / s->time_base.den) * 1000
 * then
 *   MS =  PTS * 1000 * s->time_base.num / s->time_base.den */
int64_t video_dts_to_ms(EZVID *vidx, int64_t dts)
{
	AVStream	*s = vidx->formatx->streams[vidx->vsidx];

	return av_rescale(dts * 1000, s->time_base.num, s->time_base.den);
}

/* this function is used to convert the timestamp from the millisecond 
 * based time to the video stream based PTS time. The formula is:
 *   PTS = (ms / 1000) * s->time_base.den / s->time_base.num
 * then
 *   PTS = ms * s->time_base.den / (s->time_base.num * 1000) */
int64_t video_ms_to_dts(EZVID *vidx, int64_t ms)
{
	AVStream	*s = vidx->formatx->streams[vidx->vsidx];

	return av_rescale(ms, s->time_base.den, 
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


static int64_t video_keyframe_next(EZVID *vidx, AVPacket *packet)
{
	int64_t		dts;

	while (av_read_frame(vidx->formatx, packet) >= 0) {
		if (packet->stream_index != vidx->vsidx) {
			av_free_packet(packet);
			continue;
		}
		if (packet->flags != PKT_FLAG_KEY) {
			av_free_packet(packet);
			continue;
		}
		if ((dts = meta_packet_timestamp(packet)) < 0) {
			av_free_packet(packet);
			continue;
		}

		/* find a valid key frame so updating the keyframe gap */
		video_keyframe_credit(vidx, dts);
		return dts;
	}
	return -1;
}

static int64_t video_keyframe_to(EZVID *vidx, AVPacket *packet, int64_t pos)
{
	int64_t		dts;

	while (av_read_frame(vidx->formatx, packet) >= 0) {
		if (packet->stream_index != vidx->vsidx) {
			av_free_packet(packet);
			continue;
		}
		if (packet->flags != PKT_FLAG_KEY) {
			av_free_packet(packet);
			continue;
		}
		if ((dts = meta_packet_timestamp(packet)) < 0) {
			av_free_packet(packet);
			continue;
		}

		/* find a valid key frame so updating the keyframe gap */
		video_keyframe_credit(vidx, dts);

		if (dts < pos) {
			av_free_packet(packet);
			continue;
		}
		return dts;
	}
	return -1;
}

static int64_t *video_keyframe_survey(EZVID *vidx, EZIMG *image)
{
	AVPacket	packet;
	int64_t		*keylist;
	int64_t		dts, dts_snap;
	int		i;
	struct timeval	tmstart; 

	if ((keylist = malloc(sizeof(int64_t) * image->shots)) == NULL) {
		return NULL;
	}

	gettimeofday(&tmstart, NULL);

	/* locating the first key frame */
	dts_snap = video_snap_point(vidx, image, 0);
	keylist[0] = 0;
	while ((dts = video_keyframe_next(vidx, &packet)) >= 0) {
		av_free_packet(&packet);
		if (dts >= dts_snap) {
			break;
		}
		keylist[0] = dts;
	}
	eznotify(vidx, EN_SCAN_PACKET, 0, 0, keylist);

	/* collecting rest of key frames */
	for (i = 1; i < image->shots; i++) {
	        dts_snap = video_snap_point(vidx, image, i);
		keylist[i] = keylist[i - 1];
		if ((dts < 0) || (dts >= dts_snap)) {
			eznotify(vidx, EN_SCAN_PACKET, i, 0, &keylist[i]);
			continue;
		}

		keylist[i] = dts;
		while ((dts = video_keyframe_next(vidx, &packet)) >= 0) {
			av_free_packet(&packet);
			if (dts >= dts_snap) {
				break;
			}
			keylist[i] = dts;
		}
		eznotify(vidx, EN_SCAN_PACKET, i, 0, &keylist[i]);
	}
	eznotify(vidx, EN_SCAN_IFRAME, i, meta_time_diff(&tmstart), keylist);
	video_rewind(vidx);
	return keylist;
}

static int video_keyframe_credit(EZVID *vidx, int64_t dts)
{
	/* reset the key frame crediting */ 
	if (packet == NULL) {
		vidx->keylast = -1;
		return vidx->keycount;
	}

	/* record the status of the first key frame since resetted */
	if (vidx->keylast < 0) {
		vidx->keylast = dts;
		if (vidx->keyfirst == dts) {
			vidx->keyseek = ENX_SEEK_BW_NO;
		} else {
			vidx->keyseek = ENX_SEEK_BW_YES;
		}
		if (vidx->keyfirst < 0) {
			vidx->keyfirst = dts;
		}
		return vidx->keycount;
	}

	if (dts - vidx->keylast > vidx->keygap) {
		vidx->keygap = dts - vidx->keylast;
	}
	vidx->keycount++;
	vidx->keylast = dts;
	return vidx->keycount;
}

static int64_t video_keyframe_seekat(EZVID *vidx, AVPacket *packet, int64_t dts_snap)
{
	int64_t		dts, dts_last, dts_diff;
	int		keyflag;

	if (vidx->keyseek == ENX_SEEK_BW_YES) {
		video_seeking(vidx, dts_snap);
	}

	dts_last = 0;
	while ((dts = video_keyframe_next(vidx, packet)) >= 0) {
		/* if the distance between the snap point and this key frame
		 * is longer than the maximum gap of key frames, we would 
		 * expect for another key frame */
		if ((dts_diff = dts_snap - dts) > vidx->keygap) {
			av_free_packet(packet);
			dts_last = dts;
			continue;
		}

		/* setup the key frame decoding flag */
		if (vidx->sysopt->flags & EZOP_P_FRAME) {
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
			eznotify(vidx, EN_BUMP_BACK, 
					(long)dts_diff, 0, &dts_last);
			video_seeking(vidx, dts_last);
			av_free_packet(packet);
			continue;
		}

		/* if we only want to snapshot at key frames, we leap to the
		 * next one and take sanpshot there */
		if ((dts_diff > 0) && (keyflag == 1)) {
			av_free_packet(packet);
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
		if ((dts = meta_packet_timestamp(packet)) < 0) {
			av_free_packet(packet);
			continue;
		}
		return dts;
	}
	return -1;
}

static int video_seekable_random(EZVID *vidx, EZIMG *image)
{
	AVPacket	packet;
	struct timeval	tmstart; 
	int64_t		dts[2];

	gettimeofday(&tmstart, NULL);

	vidx->keyseek = ENX_SEEK_BW_YES;

	/* find the first key frame */
	if ((dts[0] = video_keyframe_next(vidx, &packet)) < 0) {
		vidx->keyseek = ENX_SEEK_BW_NO;	/* not seek-able */
		goto vsr_exit;
	}
	av_free_packet(&packet);
	
	/* find the last key frame */
	video_seeking(vidx, video_snap_point(vidx, image, image->shots - 1));
	if ((dts[1] = video_keyframe_next(vidx, &packet)) < 0) {
		vidx->keyseek = ENX_SEEK_BW_NO;	/* not seek-able */
		goto vsr_exit;
	}
	av_free_packet(&packet);

	video_rewind(vidx);
	if (dts[1] <= dts[0]) {
		vidx->keyseek = ENX_SEEK_BW_NO; 
	}

vsr_exit:
	eznotify(vidx, EN_SEEK_FRAME, vidx->keyseek, 
			meta_time_diff(&tmstart), dts);
	return vidx->keyseek;
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
	meta_basename(vidx->filename, image->filename + 6);
	image_gdcanvas_print(image, 0, 0, image->filename);
	puts(image->filename);

	/* Line 1: the duration of the video clip, the file size, 
	 * the frame rates and the bit rates */
	strcpy(buffer, "Duration: ");
	strcat(buffer, meta_timestamp(vidx->duration, 0, tmp));

	strcat(buffer, " (");
	strcat(buffer, meta_filesize(vidx->formatx->file_size, tmp));
	strcat(buffer, ")  ");

	i = vidx->formatx->bit_rate;
	if (vidx->formatx->bit_rate == 0) {
		i = (int)(vidx->formatx->file_size * 1000 / vidx->duration);
	}
	strcat(buffer, meta_bitrate(i, tmp));
	image_gdcanvas_print(image, 1, 0, buffer);
	puts(buffer);

	/* Line 2+: the stream information */
	for (i = 0; i < vidx->formatx->nb_streams; i++) {
		stream = vidx->formatx->streams[i];
		sprintf(buffer, "%s: ", id_lookup(id_codec_type, 
					stream->codec->codec_type) + 11);
		switch (stream->codec->codec_type) {
		case CODEC_TYPE_VIDEO:
			minfo_video(stream, buffer);
			break;
		case CODEC_TYPE_AUDIO:
			minfo_audio(stream, buffer);
			break;
		case CODEC_TYPE_SUBTITLE:
			minfo_subtitle(stream, buffer);
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

/* This function is used to find the video stream in the clip 
 * as well as open the related decoder driver */
static int video_find_stream(EZVID *vidx, int flags)
{
	AVCodecContext	*codecx;
	AVCodec		*codec = NULL;
	int		i, rc;

	rc = EZ_ERR_VIDEOSTREAM;
	for (i = 0; i < vidx->formatx->nb_streams; i++) {
		eznotify(vidx, EN_STREAM_FORMAT, i, 0, vidx->formatx);
		codecx = vidx->formatx->streams[i]->codec;
		switch (codecx->codec_type) {
		case CODEC_TYPE_VIDEO:
			eznotify(vidx, EN_TYPE_VIDEO, i, 0, codecx);

			if ((vidx->sysopt->vs_idx >= 0) && 
					(vidx->sysopt->vs_idx != i)) {
				break;
			}

			vidx->vsidx  = i;
			vidx->codecx = codecx;

			/* discard frames; AVDISCARD_NONKEY,AVDISCARD_BIDIR */
			vidx->codecx->skip_frame = AVDISCARD_NONREF;
			//vidx->codecx->hurry_up = 1; /* fast decoding mode */

			/* open the codec */
			codec = avcodec_find_decoder(vidx->codecx->codec_id);
			if (avcodec_open(vidx->codecx, codec) < 0) {
				rc = EZ_ERR_CODEC_FAIL;
				eznotify(vidx, rc, codecx->codec_id, 0, codecx);
			}
			rc = EZ_ERR_NONE;
			break;

		case CODEC_TYPE_AUDIO:
			eznotify(vidx, EN_TYPE_AUDIO, i, 0, codecx);
			break;

		default:
			eznotify(vidx, EN_TYPE_UNKNOWN, i, 0, codecx);
			break;
		}
	}
	return rc;
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
static int video_duration(EZVID *vidx, int scanmode)
{
	int64_t		cur_dts;

	if (vidx->formatx->duration && (scanmode == EZ_DUR_CLIPHEAD)) {
		/* convert duration from AV_TIME_BASE to video stream base */
		cur_dts = video_system_to_dts(vidx, vidx->formatx->duration);
		/* convert duration from video stream base to milliseconds */
		vidx->duration = (int) video_dts_to_ms(vidx, cur_dts);
		eznotify(vidx, EN_DURATION, ENX_DUR_MHEAD, 
				vidx->duration, NULL);
		return vidx->duration;
	}

	/* quick scan from the tail of the stream */
	if (scanmode != EZ_DUR_FULLSCAN) {
		/* we only do this when the file is not too small */
		if (vidx->formatx->file_size > EZ_GATE_QK_SCAN) {
			/* byte seek from 90% of the clip */
			cur_dts = vidx->formatx->file_size * 9 / 10;
			//printf("Start %lld\n", cur_dts);
			av_seek_frame(vidx->formatx, vidx->vsidx, 
					cur_dts, AVSEEK_FLAG_BYTE);
			eznotify(vidx, EN_DURATION, ENX_DUR_JUMP, 
					0, &cur_dts);
		}
	}

	video_keyframe_credit(vidx, -1);
	cur_dts = video_statistics(vidx);

	video_rewind(vidx); 	/* rewind the stream to head */

	/* the 'start_time' in the AVFormatContext should be offsetted from
	 * the video stream's PTS */
	if (vidx->formatx->start_time) {
		cur_dts -= video_system_to_dts(vidx, 
				vidx->formatx->start_time);
	}

	/* convert duration from video stream base to milliseconds */
	vidx->duration = (int) video_dts_to_ms(vidx, cur_dts);
	eznotify(vidx, EN_DURATION, ENX_DUR_SCAN, vidx->duration, NULL);
	return vidx->duration;
}

static int64_t video_statistics(EZVID *vidx)
{
	struct MeStat	mestat[EZ_ST_MAX_REC];	/* shoule be big enough */
	AVPacket	packet;
	int		i, imax = 0;

	memset(mestat, 0, sizeof(mestat));
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
		if (packet.flags == PKT_FLAG_KEY) {
			mestat[i].key++;
			if (packet.stream_index == vidx->vsidx) {
				video_keyframe_credit(vidx, packet.dts);
				eznotify(vidx, EN_PACKET_KEY, 0, 0, &packet);
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
	eznotify(vidx, EN_MEDIA_STATIS, (long) mestat, imax + 1, vidx);
	return mestat[vidx->vsidx].dts_base + mestat[vidx->vsidx].dts_last;
}

static int64_t video_snap_point(EZVID *vidx, EZIMG *image, int index)
{
	int64_t seekat;

	/* setup the starting PTS and ending PTS */
	seekat = video_system_to_dts(vidx, vidx->formatx->start_time);
	if (image->time_from > 0) {
		seekat += video_ms_to_dts(vidx, image->time_from);
	}

	/* setup the initial seek position */
	if ((image->sysopt->flags & EZOP_FFRAME) == 0) {
		index++;
	}

	/* calculate the snap point */
	seekat += video_ms_to_dts(vidx, image->time_step * index);
	return seekat;
}


static int video_snap_begin(EZVID *vidx, EZIMG *image)
{
	/* If the output format is the animated GIF89a, then it opens
	 * the target file and device */
	vidx->gifx_fp = NULL;
	if ((vidx->gifx_opt = ezopt_gif_anim(image->sysopt)) > 0) {
		vidx->gifx_fp = image_gif_anim_open(image, vidx->filename);
	}

	eznotify(vidx, EN_PROC_BEGIN, (long)vidx->duration, 0, NULL);
	return 0;
}

static int video_snap_update(EZVID *vidx, EZIMG *image, AVFrame *frame, int sn, int64_t dts)
{
	char	timestamp[64];

	/* offset the PTS by the start time */
	if (vidx->formatx->start_time) {
		dts -= video_system_to_dts(vidx, 
				vidx->formatx->start_time);
	}

	/* convert current PTS to millisecond and then 
	 * metamorphose to human readable form */
	meta_timestamp((int)video_dts_to_ms(vidx, dts), 1, timestamp);

	/* write the timestamp into the shot */
	image_scale(image, frame);
	image_gdframe_update(image);

	if (image->sysopt->flags & EZOP_TIMEST) {
		image_gdframe_timestamp(image, timestamp);
	}

	if (image->gdcanvas) {
		image_gdcanvas_update(image, sn);
	} else if (vidx->gifx_fp) {
		image_gif_anim_add(image, vidx->gifx_fp, vidx->gifx_opt);
	} else {
		image_gdframe_save(image, vidx->filename, sn);
	}

	/* display the on-going information */
	eznotify(vidx, EN_PROC_CURRENT, image->shots, sn, &dts);
	return 0;
}

static int video_snap_end(EZVID *vidx, EZIMG *image)
{
	char	status[128];

	/* display the end of the process and generate the status line */
	sprintf(status, "%dx%d Thumbnails Generated by Ezthumb %s (%.3f s)",
			image->dst_width, image->dst_height, 
			EZTHUMB_VERSION, 
			meta_time_diff(&vidx->tmark) / 1000.0);
	eznotify(vidx, EN_PROC_END, image->canvas_width, 
			image->canvas_height, status);

	if (image->gdcanvas) {
		/* update the media information area */
		if (image->sysopt->flags & EZOP_INFO) {
			video_media_on_canvas(vidx, image);
			/* Insert as status line */
			image_gdcanvas_print(image, -1, 0, status);
		}
		image_gdcanvas_save(image, vidx->filename);
	} else if (vidx->gifx_fp) {
		image_gif_anim_close(image, vidx->gifx_fp);
	}
	return 0;
}

static int64_t video_decode_packet(EZVID *vidx, AVFrame *frame, AVPacket *packet)
{
	int64_t	dts;
	int	ffin;

	dts = -1;
	do {
		if (packet->stream_index != vidx->vsidx) {
			av_free_packet(packet);
			continue;
		}
		if (dts < 0) {
			dts = meta_packet_timestamp(packet);
		}

		eznotify(vidx, EN_PACKET_RECV, 0, 0, &packet);
		avcodec_decode_video2(vidx->codecx, frame, &ffin, packet);
		if (ffin) {
			/* Okey......it's not a bug!
			 * The notification and releasing the packet will 
			 * be done outside this function */
			eznotify(vidx, EN_FRAME_COMPLETE, 0, 
					(long)packet, frame);
			break;	/* successfully decoded a frame */
		}
		/* the packet is not finished */
		eznotify(vidx, EN_FRAME_PARTIAL, 0, ffin, frame);
		av_free_packet(packet);
	} while (av_read_frame(vidx->formatx, packet) >= 0);
	return dts;
}

static int64_t video_decode_to(EZVID *vidx, AVFrame *frame, AVPacket *packet, int64_t dtsto)
{
	int64_t	dts;
	int	ffin;

	dts = -1;
	do {
		if (packet->stream_index != vidx->vsidx) {
			av_free_packet(packet);
			continue;
		}
		if (dts < 0) {
			dts = meta_packet_timestamp(packet);
		}

		eznotify(vidx, EN_PACKET_RECV, 0, 0, &packet);
		avcodec_decode_video2(vidx->codecx, frame, &ffin, packet);
		if (ffin == 0) {
			eznotify(vidx, EN_FRAME_PARTIAL, 0, ffin, frame);
			av_free_packet(packet);
			continue;
		}

		eznotify(vidx, EN_FRAME_COMPLETE, 0, (long)&packet, frame);
		if (dts >= dtsto) {
			/* Okey......it's not a bug!
			 * The notification and releasing the packet will 
			 * be done outside this function */
			break;	/* successfully decoded a frame */
		}

		dts = -1;	/* discard this frame */
		av_free_packet(packet);
	} while (av_read_frame(vidx->formatx, packet) >= 0);
	return dts;
}

static int video_rewind(EZVID *vidx)
{
	/* for some reason, to rewind the stream to the head,
	 * av_seek_frame() can not do this by AVSEEK_FLAG_BYTE to 0. */
	av_seek_frame(vidx->formatx, vidx->vsidx, 0, AVSEEK_FLAG_ANY);
	video_keyframe_credit(vidx, -1);
	return 0;
}

static int video_seeking(EZVID *vidx, int64_t dts)
{
	av_seek_frame(vidx->formatx, vidx->vsidx, dts, AVSEEK_FLAG_BACKWARD);
	video_keyframe_credit(vidx, -1);
	return 0;
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
EZIMG *image_allocate(EZVID *vidx, EZOPT *ezopt, int *errcode)
{
	EZIMG	*image;
	int	size, shots;

	// FIXME: the filename could be utf-8 or widebytes
	size = sizeof(EZIMG) + strlen(vidx->filename) + 128;
	if ((image = av_malloc(size)) == NULL) {
		uperror(errcode, EZ_ERR_LOWMEM);
		return NULL;
	}
	memset(image, 0, size);
	image->sysopt = ezopt;
	image->src_width  = vidx->codecx->width;
	image->src_height = vidx->codecx->height;
	image->src_pixfmt = vidx->codecx->pix_fmt;
	
	/* calculate the expected size of each screen shots.
	 * Note that the result will be overriden by canvas_width */
	if ((ezopt->tn_width < 1) && (ezopt->tn_height < 1)) {
		if (ezopt->tn_facto < 1) {
			image->dst_width  = image->src_width;
			image->dst_height = image->src_height;
		} else {
			image->dst_width  = ((image->src_width * 
						ezopt->tn_facto) / 100) & ~1;
			image->dst_height = ((image->src_height *
						ezopt->tn_facto) / 100) & ~1;
		}
	} else if ((ezopt->tn_width > 0) && (ezopt->tn_height > 0)) {
		image->dst_width  = ezopt->tn_width & ~1;
		image->dst_height = ezopt->tn_height & ~1;
	} else if (ezopt->tn_width > 0) {
		image->dst_width  = ezopt->tn_width & ~1;
		image->dst_height = (ezopt->tn_width * image->src_height /
				image->src_width) & ~1;
	} else {
		image->dst_width  = (ezopt->tn_height * image->src_width /
				image->src_height) & ~1;
		image->dst_height = ezopt->tn_height;
	}
	image->dst_pixfmt = PIX_FMT_RGB24;

	/* calculate the expected time range */
	image->time_from = ezopt_cal_ratio(ezopt->time_from, vidx->duration);
	if (image->time_from >= vidx->duration) {
		image->time_from = 0;
	}
	image->time_during = ezopt_cal_ratio(ezopt->time_to, vidx->duration);
	if (image->time_during > vidx->duration) {
		image->time_during = vidx->duration - image->time_from;
	} else if (image->time_during <= image->time_from) {
		image->time_during = vidx->duration - image->time_from;
	} else {
		image->time_during -= image->time_from;
	}


	/* calculte the canvas, the screenshots, timestep and the gaps */
	if (ezopt->grid_col < 1) {	/* we want separated screen shots */
		image->grid_col  = ezopt->grid_col;
		image->grid_row  = ezopt->grid_row;
		image->time_step = ezopt->tm_step;
		if ((image->grid_row < 1) && (image->time_step > 0)) {
			image->grid_row = ezopt_cal_shots(image->time_during,
					image->time_step, ezopt->flags);
		} else if ((image->grid_row > 0) && (image->time_step < 1)) {
			image->time_step = ezopt_cal_timestep(
					image->time_during,
					image->grid_row, ezopt->flags);

		}
	} else {
		image->grid_col = ezopt->grid_col;
		if ((ezopt->grid_row < 1) && (ezopt->tm_step < 1)) {
			ezopt->grid_row = 4;	/* make it default */
		}
		if (ezopt->grid_row < 1) {
			shots = ezopt_cal_shots(image->time_during, 
					ezopt->tm_step, ezopt->flags);
			image->grid_row  = (shots + image->grid_col - 1) /
					image->grid_col;
			image->time_step = ezopt->tm_step;
		} else if (ezopt->tm_step > 0) {
			image->grid_row  = ezopt->grid_row;
			image->time_step = ezopt->tm_step;
		} else {
			image->grid_row  = ezopt->grid_row;
			image->time_step = ezopt_cal_timestep(
					image->time_during,
					image->grid_col * image->grid_row, 
					ezopt->flags);
		}

		if (ezopt->canvas_width > 63) {
			/* if the canvas width is specified, it overrides 
			 * tn_width, tn_height and tn_facto */
			image->canvas_width = ezopt->canvas_width & ~1;

			/* it's the reference width for getting the gap size */
			size = ezopt->canvas_width / ezopt->grid_col;
			image->gap_width = ezopt_cal_ratio(ezopt->grid_gap_w, 
					size);
			image->rim_width = ezopt_cal_ratio(ezopt->grid_rim_w, 
					size);

			/* it's the reference height for getting the gap size*/
			size = size * image->src_height / image->src_width;
			image->gap_height = ezopt_cal_ratio(ezopt->grid_gap_h, 
					size);
			image->rim_height = ezopt_cal_ratio(ezopt->grid_rim_h, 
					size);

			/* now calculate the actual shot width and height */
			image->dst_width = (image->canvas_width - 
				image->rim_width * 2 -
				image->gap_width * (ezopt->grid_col - 1)) /
				ezopt->grid_col;
			image->dst_height = (image->dst_width * 
				image->src_height / image->src_width) & ~1;
			image->dst_width  = image->dst_width & ~1;
		} else {
			image->gap_width = ezopt_cal_ratio(ezopt->grid_gap_w, 
					image->dst_width);
			image->rim_width = ezopt_cal_ratio(ezopt->grid_rim_w, 
					image->dst_width);
			image->gap_height = ezopt_cal_ratio(ezopt->grid_gap_h, 
					image->dst_width);
			image->rim_height = ezopt_cal_ratio(ezopt->grid_rim_h, 
					image->dst_width);
			image->canvas_width = (image->rim_width * 2 + 
				image->gap_width * (ezopt->grid_col - 1) +
				image->dst_width * ezopt->grid_col + 1) & ~1;
		}
		image->canvas_height = image->rim_height * 2 + 
			image->gap_height * (ezopt->grid_row - 1) +
			image->dst_height * ezopt->grid_row ;
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
		image->canvas_minfo = size * (vidx->formatx->nb_streams + 2) 
						+ EZ_TEXT_INSET_GAP;
		image->canvas_height += image->canvas_minfo;
		/* Plus the status line: font height + INSET_GAP */
		image->canvas_height += size + EZ_TEXT_INSET_GAP;
	}
	image->canvas_height = (image->canvas_height + 1) & ~1;

	/* allocate the frame structure for RGB converter which
	 * will be filled by frames converted from YUV form */
	if ((image->rgb_frame = avcodec_alloc_frame()) == NULL) {
		image_free(image);
		uperror(errcode, EZ_ERR_LOWMEM);
		return NULL;
	}

	/* allocate the memory buffer for holding the pixel array of
	 * RGB frame */
	size = avpicture_get_size(image->dst_pixfmt, 
			image->dst_width, image->dst_height);
	if ((image->rgb_buffer = av_malloc(size)) == NULL) {
		image_free(image);
		uperror(errcode, EZ_ERR_LOWMEM);
		return NULL;
	}

	/* link the RGB frame and the RBG pixel buffer */
	avpicture_fill((AVPicture *) image->rgb_frame, image->rgb_buffer, 
			image->dst_pixfmt, image->dst_width,image->dst_height);

	/* allocate the swscale structure for scaling the screen image */
	image->swsctx = sws_getContext(image->src_width, image->src_height, 
			image->src_pixfmt, 
			image->dst_width, image->dst_height,
			image->dst_pixfmt, SWS_BILINEAR, NULL, NULL, NULL);
	if (image->swsctx == NULL) {
		image_free(image);
		uperror(errcode, EZ_ERR_SWSCALE);
		return NULL;
	}

	/* create a GD device for handling the screen shots */
	image->gdframe = gdImageCreateTrueColor(image->dst_width, 
			image->dst_height);
	if (image->gdframe == NULL) {
		image_free(image);
		uperror(errcode, EZ_ERR_LOWMEM);
		return NULL;
	}

	if ((image->grid_col > 0) && !ezopt_gif_anim(image->sysopt)) {
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
		image->color_canvas = gdImageColorAllocateAlpha(
				image->gdcanvas,
				ezopt->canvas_color[0], 
				ezopt->canvas_color[1],
				ezopt->canvas_color[2], 
				ezopt->canvas_color[3]);
		image->color_shadow = gdImageColorAllocateAlpha(
				image->gdcanvas,
				ezopt->shadow_color[0], 
				ezopt->shadow_color[1],
				ezopt->shadow_color[2], 
				ezopt->shadow_color[3]);
		image->color_minfo = gdImageColorAllocateAlpha(
				image->gdcanvas,
				ezopt->mi_color[0], 
				ezopt->mi_color[1],
				ezopt->mi_color[2], 
				ezopt->mi_color[3]);

		/* setup the background color */
		gdImageFilledRectangle(image->gdcanvas, 0, 0, 
				image->canvas_width  - 1, 
				image->canvas_height - 1, 
				image->color_canvas);
		/* load the background picture */
		image_gdcanvas_background(image);
	}

	/* define the colors used in the screen shots */
	image->color_edge = gdImageColorAllocateAlpha(image->gdframe,
			ezopt->edge_color[0], ezopt->edge_color[1],
			ezopt->edge_color[2], ezopt->edge_color[3]);
	image->color_inset = gdImageColorAllocateAlpha(image->gdframe,
			ezopt->ins_color[0], ezopt->ins_color[1],
			ezopt->ins_color[2], ezopt->ins_color[3]);
	image->color_inshadow = gdImageColorAllocateAlpha(image->gdframe,
			ezopt->its_color[0], ezopt->its_color[1],
			ezopt->its_color[2], ezopt->its_color[3]);

	uperror(errcode, EZ_ERR_NONE);
	vidx->image = image;
	eznotify(vidx, EN_IMAGE_CREATED, 0, 0, image);
	return image;	
}

int image_free(EZIMG *image)
{
	if (image->gdcanvas) {
		gdImageDestroy(image->gdcanvas);
	}
	if (image->gdframe) {
		gdImageDestroy(image->gdframe);
	}
	if (image->swsctx) {
		sws_freeContext(image->swsctx);
	}
	if (image->rgb_buffer) {
		av_free(image->rgb_buffer);
	}
	if (image->rgb_frame) {
		av_free(image->rgb_frame);
	}
	av_free(image);
	return EZ_ERR_NONE;
}


static int image_scale(EZIMG *image, AVFrame *frame)
{
	return sws_scale(image->swsctx, (const uint8_t * const *)frame->data,
			frame->linesize, 0, image->src_height, 
			image->rgb_frame->data, image->rgb_frame->linesize);
}

int image_font_test(EZIMG *image, char *filename)
{
	gdFont	*font;
	char	*s="1234567890ABCDEFGHIJKLMNOPQRSTUabcdefghijklmnopqrstuvwxyz";
	int	y, brect[8];

	if (image->gdcanvas == NULL) {
		return -1;
	}

	y = 20;

	font = meta_fontset(EZ_FONT_TINY);
	gdImageString(image->gdcanvas, font, 20, y, (unsigned char *) s, 
			image->color_minfo);
	y += font->h + 2;
	
	font = meta_fontset(EZ_FONT_SMALL);
	gdImageString(image->gdcanvas, font, 20, y, (unsigned char *) s, 
			image->color_minfo);
	y += font->h + 2;

	font = meta_fontset(EZ_FONT_MEDIUM);
	gdImageString(image->gdcanvas, font, 20, y, (unsigned char *) s, 
			image->color_minfo);
	y += font->h + 2;

	font = meta_fontset(EZ_FONT_LARGE);
	gdImageString(image->gdcanvas, font, 20, y, (unsigned char *) s, 
			image->color_minfo);
	y += font->h + 2;

	font = meta_fontset(EZ_FONT_GIANT);
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
static int image_gdframe_update(EZIMG *image)
{
	unsigned char	*src;
	int	x, y;

	src = image->rgb_frame->data[0];
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
	char	tmp[128];

	sprintf(tmp, "%03d.%s", idx, image->sysopt->img_format);
	meta_name_suffix(image->sysopt->pathout, 
			filename, image->filename, tmp);
	if ((fout = fopen(image->filename, "wb")) == NULL) {
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
		font = meta_fontset(fsize);
	} else if (gdImageStringFT(NULL, brect,	0, image->sysopt->ins_font, 
				(double) fsize, 0, 0, 0, s)) {
		font = meta_fontset(fsize);
	} else {
		return EZ_MK_WORD(brect[2] - brect[6], brect[3] - brect[7]);
	}
	return EZ_MK_WORD(font->w * strlen(s), font->h);
}

static int image_gdframe_puts(EZIMG *image, int fsize, int x, int y, int c, char *s)
{
	int	brect[8];

	fsize = meta_fontsize(fsize, image->dst_width);
	if (image->sysopt->ins_font == NULL) {
		gdImageString(image->gdframe, meta_fontset(fsize),
				x, y, (unsigned char *) s, c);
	} else if (gdImageStringFT(NULL, brect, 0, image->sysopt->ins_font,
				(double) fsize, 0, 0, 0, s)) {
		gdImageString(image->gdframe, meta_fontset(fsize),
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
	char	tmp[128];

	sprintf(tmp, "%s.%s",image->sysopt->suffix,image->sysopt->img_format);
	meta_name_suffix(image->sysopt->pathout,
			filename, image->filename, tmp);
	if ((fout = fopen(image->filename, "wb")) == NULL) {
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
	meta_copy_image(image->gdcanvas, image->gdframe, x, y, 0, 0);
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
		font = meta_fontset(fsize);
	} else if (gdImageStringFT(NULL, brect,	0, image->sysopt->mi_font, 
				(double) fsize, 0, 0, 0, s)) {
		font = meta_fontset(fsize);
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
		gdImageString(image->gdcanvas, meta_fontset(fsize),
				x, y, (unsigned char *) s, c);
	} else if (gdImageStringFT(NULL, brect, 0, image->sysopt->mi_font,
				(double) fsize, 0, 0, 0, s)) {
		gdImageString(image->gdcanvas, meta_fontset(fsize),
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
				meta_copy_image(image->gdcanvas, bgim, 
						dx, dy, twid, thei);
			}
		}
		gdImageDestroy(bgim);
		return 0;
	}

	meta_copy_image(image->gdcanvas, bgim, dx, dy, twid, thei);
	gdImageDestroy(bgim);
	return 0;
}

static FILE *image_gif_anim_open(EZIMG *image, char *filename)
{
	gdImage	*imgif;
	FILE	*fout;
	char	tmp[128];

	sprintf(tmp, "%s.%s",image->sysopt->suffix,image->sysopt->img_format);
	meta_name_suffix(image->sysopt->pathout,
			filename, image->filename, tmp);
	if ((fout = fopen(image->filename, "wb")) == NULL) {
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


static int ezopt_cal_ratio(int ratio, int refsize)
{
	if (ratio & EZ_RATIO_OFF) {
		return (ratio & ~EZ_RATIO_OFF) * refsize / 100;
	} else if (ratio > 0) {
		return ratio;
	}
	return 0;
}

static int ezopt_cal_shots(int duration, int tmstep, int mode)
{
	int	shots;

	shots = duration / tmstep - 1;
	if (mode & EZOP_FFRAME) {
		shots++;
	}
	if (mode & EZOP_LFRAME) {
		shots++;
	}
	return shots;
}

static int ezopt_cal_timestep(int duration, int shots, int mode)
{
	if (mode & EZOP_FFRAME) {
		shots--;
	}
	if (mode & EZOP_LFRAME) {
		shots--;
	}
	return duration / (shots + 1);
}

static int ezopt_gif_anim(EZOPT *ezopt)
{
	if (strcmp(ezopt->img_format, "gif")) {
		return 0;
	}
	if (ezopt->img_quality > 0) {
		return ezopt->img_quality / 10;
	}
	return 0;
}

static char *minfo_video(AVStream *stream, char *buffer)
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

	strcat(buffer, id_lookup(id_pix_fmt, stream->codec->pix_fmt) + 8);
	sprintf(tmp, "  %.3f FPS ", (float) stream->r_frame_rate.num / 
			(float) stream->r_frame_rate.den);
	strcat(buffer, tmp);

	if (stream->codec->bit_rate) {
		strcat(buffer, meta_bitrate(stream->codec->bit_rate, tmp));
	}
	return buffer;
}

static char *minfo_audio(AVStream *stream, char *buffer)
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
			id_lookup(id_sam_format, stream->codec->sample_fmt),
			stream->codec->sample_rate);
	strcat(buffer, tmp);

	if (stream->codec->bit_rate) {
		strcat(buffer, meta_bitrate(stream->codec->bit_rate, tmp));
	}
	return buffer;
}

static char *minfo_subtitle(AVStream *stream, char *buffer)
{
	return buffer;
}

static char *meta_bitrate(int bitrate, char *buffer)
{
	static	char	tmp[32];

	if (buffer == NULL) {
		buffer = tmp;
	}
	sprintf(buffer, "%.3f kbps", (float)bitrate / 1000.0);
	return buffer;
}

static char *meta_filesize(int64_t size, char *buffer)
{
	static	char	tmp[32];

	if (buffer == NULL) {
		buffer = tmp;
	}
	if (size < (1ULL << 24)) {
		sprintf(buffer, "%lld KB", (long long)(size >> 10));
	} else if (size < (1ULL << 34)) {
		sprintf(buffer, "%lld.%03lld MB", (long long)(size >> 20), 
				(long long)((size % (1 << 20)) >> 10));
	} else {
		sprintf(buffer, "%lld.%03lld GB", (long long)(size >> 30), 
				(long long)((size % (1 << 30)) >> 20));
	}
	return buffer;
}

static char *meta_timestamp(int ms, int enms, char *buffer)
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

static int meta_fontsize(int fsize, int refsize)
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

static gdFont *meta_fontset(int fsize)
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

// FIXME: UTF-8 and widechar?
static char *meta_basename(char *fname, char *buffer)
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
static char *meta_name_suffix(char *path, char *fname, char *buf, char *sfx)
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

static int meta_copy_image(gdImage *dst, gdImage *src, 
		int x, int y, int wid, int hei)
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

static int64_t meta_packet_timestamp(AVPacket *packet)
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

int meta_time_diff(struct timeval *tvbegin)
{
	struct	timeval	tvnow;
	int	diff;

	gettimeofday(&tvnow, NULL);

	diff = tvnow.tv_usec - tvbegin->tv_usec;
	if (diff < 0) {
		diff += 1000000;
		tvnow.tv_sec--;
	}
	diff += (tvnow.tv_sec - tvbegin->tv_sec) * 1000000;
	return diff / 1000;
}


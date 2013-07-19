
/*  eznotify.c - the notification handling functions

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

#include "ezthumb.h"
#include "id_lookup.h"

#include "gdfontt.h"
#include "gdfonts.h"
#include "gdfontmb.h"
#include "gdfontl.h"
#include "gdfontg.h"


static int ezdefault(EZOPT *ezopt, int event, long param, long opt, void *);
static int ezdump_video_info(EZVID *vidx);
static int ezdump_media_statistics(struct MeStat *mestat, int n, EZVID *vidx);


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

static int ezdefault(EZOPT *ezopt, int event, long param, long opt, void *block)
{
	struct	ezntf	*myntf;
	EZVID	*vidx;
	EZIMG	*image;
	char	buf[256];
	char	*seekm[] = { "SU", "SN", "SF", "SB" };	/* seekable codes */
	char	*dmod[] = { "AU", "QS", "FS", "HD" };	/* duration mode */
	int	i, n;

	switch (event) {
	case EZ_ERR_LOWMEM:
		slog(EZDBG_WARNING, "%s: low memory [%ld]\n", 
				(char*) block, param);
		break;
	case EZ_ERR_FORMAT:
		slog(EZDBG_WARNING, "%s: unknown format.\n", (char*) block);
		break;
	case EZ_ERR_STREAM:
		slog(EZDBG_WARNING, "%s: no stream info found.\n", 
				(char*) block);
		break;
	case EZ_ERR_VIDEOSTREAM:
		slog(EZDBG_WARNING, "%s: no video stream found.\n", 
				(char*) block);
		break;
	case EZ_ERR_CODEC_FAIL:
		slog(EZDBG_WARNING, "Could not open codec! %ld\n", param);
		break;
	case EZ_ERR_FILE:
		slog(EZDBG_WARNING, "%s: file not found.\n", (char*) block);
		break;

	case EN_FILE_OPEN:
		vidx = block;
		if (ezopt->flags & EZOP_CLI_INSIDE) {
			/* This is the ffmpeg function so it must run before
			 * disabling the av_log */
			i = av_log_get_level();
			av_log_set_level(AV_LOG_INFO);
#if	(LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 110, 0))
			av_dump_format(vidx->formatx, 0, vidx->filename, 0);
#else
			dump_format(vidx->formatx, 0, vidx->filename, 0);
#endif
			av_log_set_level(i);
		}
		slog(EZDBG_INFO, "%s: open successed (%ld ms)\n",
				vidx->filename, opt);
		break;
	case EN_MEDIA_OPEN:
		vidx = block;
		if (EZOP_DEBUG(vidx->ses_flags) >= EZDBG_INFO) {
			dump_format_context(vidx->formatx);
		}
		if (vidx->ses_flags & EZOP_CLI_INSIDE) {	// FIXME
			dump_duration(vidx, (int) opt);
		}
		if (vidx->ses_flags & EZOP_CLI_INFO) {
			ezdump_video_info(vidx);
		}
		break;
	case EN_IMAGE_CREATED:
		if (EZOP_DEBUG(ezopt->flags) >= EZDBG_INFO) {
			dump_ezthumb(ezopt, block);
		}
		break;
	/**/
	case EN_PROC_BEGIN:
		switch (param) {
		case ENX_SS_SCAN:
			slog(EZDBG_SHOW, "Building (Scan)      ");
			break;
		case ENX_SS_SAFE:
			slog(EZDBG_SHOW, "Building (Safe)      ");
			break;
		case ENX_SS_TWOPASS:
			slog(EZDBG_SHOW, "Building (2Pass)     ");
			break;
		case ENX_SS_HEURIS:
			slog(EZDBG_SHOW, "Building (Heur)      ");
			break;
		case ENX_SS_IFRAMES:
			slog(EZDBG_SHOW, "Building (iFrame)      ");
			break;
		case ENX_SS_SKIM:
		default:
			slog(EZDBG_SHOW, "Building (Fast)      ");
			break;
		}
		break;
	case EN_PROC_CURRENT:
		//slog(EZDBG_SHOW, ".");
		break;
	case EN_PROC_END:
		slog(EZDBG_SHOW, " %ldx%ld done\n", param, opt);
		break;
	case EN_PROC_SAVED:
		myntf = block;
		vidx  = myntf->varg1;
		image = myntf->varg2;
		slog(EZDBG_SHOW, "OUTPUT: %s\n", image->filename);

		n = sprintf(buf, "MAGIC: %s %s ", 
				seekm[vidx->seekable%4],
				dmod[GETDURMOD(vidx->ses_flags)>>12]);
		for (i = 0; i < vidx->pidx; i++) {
			n += sprintf(buf + n, "%c%d ", 
					vidx->pts[i*2], vidx->pts[i*2+1]);
		}
		strcat(buf, "\n");
		slos(EZDBG_SHOW, buf);
		break;
	case EN_STREAM_BROKEN:
		break;
	case EN_PACKET_RECV:
		//dump_packet(block);
		break;
	case EN_PACKET_KEY:
		if (EZOP_DEBUG(ezopt->flags) >= EZDBG_PACKET) {
			dump_packet(block);
		}
		break;
	case EN_FRAME_COMPLETE:
		if (EZOP_DEBUG(ezopt->flags) >= EZDBG_VERBS) {
			dump_frame(block, opt);
		}
		break;
	case EN_FRAME_PARTIAL:
		if (EZOP_DEBUG(ezopt->flags) >= EZDBG_VERBS) {
			dump_frame(block, opt);
		}
		break;
	case EN_FRAME_EFFECT:
		myntf = block;
		if (EZOP_DEBUG(ezopt->flags) >= EZDBG_IFRAME) {
			dump_frame_packet(myntf->varg1, param, myntf->varg2);
		}
		break;
	case EN_SCAN_PACKET:
		//slogz("Key Frame %d: %lld\n", param, *((long long *)block));
		break;
	case EN_SCAN_IFRAME:
		vidx = block;
		if (EZOP_DEBUG(vidx->ses_flags) >= EZDBG_BRIEF) {
			slogz("I-Frame Scanned (%ld ms):\n", opt);
			for (i = 0; i < param; i++) {
				slogz("%9lld", ((long long *)block)[i]);
				if ((i % 8) == 7) {
					slosz("\n");
				}
			}
			if ((i % 8) != 0) {
				slosz("\n");
			}
		}
		break;
	case EN_STREAM_FORMAT:
		vidx = block;
		if (EZOP_DEBUG(ezopt->flags) >= EZDBG_BRIEF) {
			dump_stream(vidx->formatx->streams[param]);
		} else if (EZOP_DEBUG(ezopt->flags) >= EZDBG_INFO) {
			dump_codec_attr(vidx->formatx, (int) param);
		}
		break;
	case EN_TYPE_VIDEO:
		if (EZOP_DEBUG(ezopt->flags) >= EZDBG_IFRAME) {
			dump_video_context(block);
		} else if (EZOP_DEBUG(ezopt->flags) >= EZDBG_BRIEF) {
			dump_codec_video(block);
		}
		break;
	case EN_TYPE_AUDIO:
		if (EZOP_DEBUG(ezopt->flags) >= EZDBG_IFRAME) {
			dump_audio_context(block);
		} else if (EZOP_DEBUG(ezopt->flags) >= EZDBG_BRIEF) {
			dump_codec_audio(block);
		}
		break;
	case EN_TYPE_UNKNOWN:
		if (EZOP_DEBUG(ezopt->flags) >= EZDBG_BRIEF) {
			dump_other_context(block);
		}
		break;
	case EN_DURATION:
		vidx = block;
		if (EZOP_DEBUG(ezopt->flags) >= EZDBG_WARNING) {
			dump_duration(vidx, (int)opt);
		}
		break;
	case EN_BUMP_BACK:
		myntf = block;
		vidx = myntf->varg1;
		slogz("Bump back to %lld: %lld (%lld < %lld)\n",
				(long long) myntf->iarg2, 
				(long long) myntf->iarg1,
				(long long) vidx->keydelta, 
				(long long) vidx->keygap);
		break;
	case EN_MEDIA_STATIS:
		myntf = block;
		if (ezopt->flags & EZOP_CLI_INSIDE) {
			ezdump_media_statistics(myntf->varg1, 
					(int) param, myntf->varg2);
		}
		break;
	case EN_IFRAME_CREDIT:
		/*
		switch (param) {
		case ENX_IFRAME_RESET:
			break;
		case ENX_IFRAME_SET:
			slogz("Key Frame start from: %lld\n", 
					(long long) vidx->keylast);
			break;
		case ENX_IFRAME_UPDATE:
			slogz("Key Frame Gap Update: %lld\n", 
					(long long) vidx->keygap);
			break;
		}*/
		break;
	case EN_FRAME_EXCEPTION:
		if (EZOP_DEBUG(ezopt->flags) >= EZDBG_INFO) {
			slogz("Discard Dodge I");
			dump_frame(block, 1);
		}
		break;
	case EN_SKIP_EXIST:
		slog(EZDBG_WARNING, "Thumbnail Existed: %s\n", (char*) block);
		break;
	}
	return event;
}

static int ezdump_video_info(EZVID *vidx)
{
	AVCodecContext	*codecx;
	char	tmp[16];
	int	i, sec;

	for (i = 0; i < vidx->formatx->nb_streams; i++) {
		codecx = vidx->formatx->streams[i]->codec;
		if (codecx->codec_type == AVMEDIA_TYPE_VIDEO) {
			/* Fixed: the video information should use the actual
			 * duration of the clip */
			//sec = (int)(vidx->formatx->duration / AV_TIME_BASE);
			sec = vidx->duration / 1000;
			sprintf(tmp, "%dx%d", codecx->width, codecx->height);
			slogz("%2d:%02d:%02d %10s [%d]: %s\n",
					sec / 3600,
					(sec % 3600) / 60, 
					(sec % 3600) % 60,
					tmp, i, vidx->filename);
		}
	}
	return 0;
}

static int ezdump_media_statistics(struct MeStat *mestat, int n, EZVID *vidx)
{
	int64_t	dts;
	int	i, sec;

	slogz("Media: %s\n", vidx->filename);
	for (i = 0; i < n; i++) {
		slogz("[%d] ", i);
		if (i >= vidx->formatx->nb_streams) {
			slogz("ERROR  %8lu\n", mestat[i].received);
			continue;
		}
		
		switch(vidx->formatx->streams[i]->codec->codec_type) {
		case AVMEDIA_TYPE_VIDEO:
			slogz("VIDEO  ");
			break;
		case AVMEDIA_TYPE_AUDIO:
			slogz("AUDIO  ");
			break;
		case AVMEDIA_TYPE_SUBTITLE:
			slogz("SUBTITL");
			break;
		default:
			slogz("UNKNOWN");
			break;
		}
		dts = mestat[i].dts_base + mestat[i].dts_last;
		dts -= vidx->dts_offset;
		sec = (int) (video_dts_to_ms(vidx, dts) / 1000);
		slogz(":%-8lu KEY:%-6lu REW:%lu  TIME:%d\n",
				mestat[i].received, mestat[i].key, 
				mestat[i].rewound, sec);
	}
	slogz("Maximum Gap of key frames: %lld\n", 
			(long long) vidx->keygap);
	slogz("Time used: %.3f\n", smm_time_diff(&vidx->tmark) / 1000.0);
	return 0;
}

int dump_format_context(AVFormatContext *format)
{
#if	LIBAVFORMAT_VERSION_INT < (53<<16)
	slogz("  Format: %s(%s), Size: %lld, Bitrate: %u\n",
			format->iformat->long_name,
			format->iformat->name,
			(long long) format->file_size,
			format->bit_rate);
#else
	slogz("  Format: %s(%s), Size: %lld, Bitrate: %u\n",
			format->iformat->long_name,
			format->iformat->name,
			(long long) avio_size(format->pb),
			format->bit_rate);
#endif
	slogz("  Streams: %d, Start time: %lld, Duration: %lld\n",
			format->nb_streams,
			(long long) format->start_time,
			(long long) format->duration);
	return 0;
}

int dump_video_context(AVCodecContext *codec)
{
	slogz("    Stream Video: %s %s, Time Base: %d/%d, Sample_AR: %d/%d\n",
			id_lookup(id_codec, codec->codec_id),
			id_lookup(id_pix_fmt, codec->pix_fmt),
			codec->time_base.num, codec->time_base.den,
			codec->sample_aspect_ratio.num,
			codec->sample_aspect_ratio.den);
	return 0;
}

int dump_audio_context(AVCodecContext *codec)
{
	slogz("    Stream Audio: %s, Time Base: %d/%d, CH=%d SR=%d %s BR=%d\n",
			id_lookup(id_codec, codec->codec_id),
			codec->time_base.num, codec->time_base.den,
			codec->channels, codec->sample_rate,
			id_lookup_tail(id_sample_format, codec->sample_fmt),
			codec->bit_rate);
	return 0;
}

int dump_other_context(AVCodecContext *codec)
{
	slogz("    Stream %s:\n",
			id_lookup_tail(id_codec_type, codec->codec_type));
	return 0;
}

int dump_codec_attr(AVFormatContext *format, int i)
{
	AVCodec	*codec;

	codec = avcodec_find_decoder(format->streams[i]->codec->codec_id);
	slogz("Stream #%d: %s Codec ID: %s '%s' %s\n", i, 
			id_lookup(id_codec_type, 
				format->streams[i]->codec->codec_type),
			id_lookup(id_codec, 
				format->streams[i]->codec->codec_id),
			codec ? codec->name : "Unknown",
			codec ? codec->long_name : "Unknown");
	return 0;
}

int dump_codec_video(AVCodecContext *codec)
{
	slogz("  Codec Type  : %s, Codec ID: %s (avcodec.h)\n",
			id_lookup(id_codec_type, codec->codec_type), 
			id_lookup(id_codec, codec->codec_id));
	slogz("  Bit Rates   : %d, Time Base: %d/%d\n", 
			codec->bit_rate,
			codec->time_base.num, codec->time_base.den);
	slogz("  Frame Number: %d, Width: %d, Height: %d, "
				"Sample_AR: %d/%d%s\n",
			codec->frame_number, codec->width, codec->height, 
			codec->sample_aspect_ratio.num, 
			codec->sample_aspect_ratio.den,
			(0 == codec->sample_aspect_ratio.num) ? "(-)" : "(+)");
	slogz("  Pixel Format: %s (pixfmt.h), Has B-Frame: %d\n", 
			id_lookup(id_pix_fmt, codec->pix_fmt), 
			codec->has_b_frames);
	return 0;
}

int dump_codec_audio(AVCodecContext *codec)
{
	slogz("  Codec Type  : %s, Codec ID: %s (avcodec.h)\n",
			id_lookup(id_codec_type, codec->codec_type), 
			id_lookup(id_codec, codec->codec_id));
	slogz("  Bit Rates   : %d, Time Base: %d/%d\n", 
			codec->bit_rate,
			codec->time_base.num, codec->time_base.den);
	slogz("  Channel     : %d, Sample Rate: %d, Sample Format: %d\n",
			codec->channels, 
			codec->sample_rate, codec->sample_fmt);
	return 0;
}

int dump_packet(AVPacket *p)
{
	/* PTS:Presentation timestamp.  DTS:Decompression timestamp */
	slogz("Packet Pos:%" PRId64 ", PTS:%" PRId64 ", DTS:%" PRId64 
			", Dur:%d, Siz:%d, Flag:%d, SI:%d\n",
			p->pos, p->pts,	p->dts, p->duration, p->size,
			p->flags, p->stream_index);
	return 0;
}

int dump_frame(AVFrame *frame, int got_pic)
{
	slogz("Frame %s, KEY:%d, CPN:%d, DPN:%d, REF:%d, I:%d, Type:%s\n", 
			got_pic == 0 ? "Partial" : "Complet", 
			frame->key_frame, 
			frame->coded_picture_number, 
			frame->display_picture_number,
			frame->reference, frame->interlaced_frame,
			id_lookup(id_pict_type, frame->pict_type));
	return 0;
}

int dump_frame_packet(EZVID *vidx, int sn, EZFRM *ezfrm)
{
	int64_t	dts;
	char	timestamp[64];

	dts = ezfrm->rf_dts - vidx->dts_offset;
	meta_timestamp((int)video_dts_to_ms(vidx, dts), 1, timestamp);
	slogz("Frame %3d: Pos:%lld Size:%d PAC:%d DTS:%lld (%s) Type:%s\n",
			sn, (long long) ezfrm->rf_pos, ezfrm->rf_size, 
			ezfrm->rf_pac, (long long) ezfrm->rf_dts, timestamp, 
			id_lookup(id_pict_type, ezfrm->frame->pict_type));
	return 0;
}

int dump_stream(AVStream *stream)
{
	slogz("Stream:%d, FRate:%d/%d, Time Base:%d/%d, Start Time:%" 
			PRId64 ", Duration:%" PRId64 ", Lang:%s, AR:%d/%d\n",
			stream->id, 
			stream->r_frame_rate.num, stream->r_frame_rate.den,
			stream->time_base.num, stream->time_base.den,
			stream->start_time, stream->duration, 
			/*stream->language,*/ "UNDEF",
			stream->sample_aspect_ratio.num, 
			stream->sample_aspect_ratio.den);
	return 0;
}

int dump_duration(EZVID *vidx, int use_ms)
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
	default:
		strcpy(buf, "Mistake");
		break;
	}
	slogz("Duration found by %s: %lld (%d ms); Seeking capability: %s\n",
			buf, vidx->duration, use_ms, tmp);
	return 0;
}

int dump_ezthumb(EZOPT *ezopt, EZIMG *image)
{
	slogz("\n>>>>>>>>>>>>>>>>>>\n");
	slogz("Single shot size:  %dx%dx%d-%d\n", 
			image->dst_width, image->dst_height, 
			image->dst_pixfmt, ezopt->edge_width);
	slogz("Grid size:         %dx%d+%d\n", 
			image->grid_col, image->grid_row, 
			ezopt->shadow_width);
	slogz("Canvas size:       %dx%d-%d\n", 
			image->canvas_width, image->canvas_height, 
			image->canvas_minfo);
	slogz("Time setting:      %lld-%lld %lld (ms)\n", 
			(long long) image->time_from, 
			(long long) image->time_during, 
			(long long) image->time_step);
	slogz("Margin of canvas:  %dx%d\n", 
			image->rim_width, image->rim_height);
	slogz("Gap of shots:      %dx%d\n", 
			image->gap_width, image->gap_height);
	slogz("Color of Canvas:   BG#%08X SH#%08X MI#%08X\n",
			(unsigned) image->color_canvas,
			(unsigned) image->color_shadow,
			(unsigned) image->color_minfo);
	slogz("Color of Shots:    ED#%08X IN#%08X SH#%08X\n",
			(unsigned) image->color_edge,
			(unsigned) image->color_inset,
			(unsigned) image->color_inshadow);
	slogz("Font size:         MI=%d IN=%d (SH: %d %d)\n", 
			ezopt->mi_size, ezopt->ins_size,
			ezopt->mi_shadow, ezopt->ins_shadow);
	slogz("Font position:     MI=%d IN=%d\n",
			ezopt->mi_position, ezopt->ins_position);
	if (ezopt->mi_font) {
		slogz("Font MediaInfo:    %s\n", ezopt->mi_font);
	}
	if (ezopt->ins_font) {
		slogz("Font Inset Shots:  %s\n", ezopt->ins_font);
	}
	slogz("Output file name:  %s.%s (%d)\n", 
			ezopt->suffix, ezopt->img_format, ezopt->img_quality);
	slogz("Flags:             %s %s %s %s %s %s %s %s %s 0x%x D%d P%d\n", 
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
			EZOP_DEBUG(ezopt->flags), EZOP_PROC(ezopt->flags));
	slogz("Font numerate:     %dx%d %dx%d %dx%d %dx%d %dx%d\n",
			gdFontGetTiny()->w, gdFontGetTiny()->h,
			gdFontGetSmall()->w, gdFontGetSmall()->h,
			gdFontGetMediumBold()->w, gdFontGetMediumBold()->h,
			gdFontGetLarge()->w, gdFontGetLarge()->h,
			gdFontGetGiant()->w, gdFontGetGiant()->h);
	slogz("Background Image:  %s (0x%x)\n", 
			ezopt->background, ezopt->bg_position);

	ezopt_profile_dump(ezopt,"Profile of Grid:   ", "Profile of Shots:  ");
	slogz("<<<<<<<<<<<<<<<<<<\n");
	return 0;
}





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
#include "libsmm.h"

#include "gdfontt.h"
#include "gdfonts.h"
#include "gdfontmb.h"
#include "gdfontl.h"
#include "gdfontg.h"


static int ezdefault(EZVID *vidx, int event, long param, long opt, void *);
static int ezdump_video_info(EZVID *vidx);
static int ezdump_media_statistics(struct MeStat *mestat, int n, EZVID *vidx);


int eznotify(EZVID *vidx, int event, long param, long opt, void *block)
{
	int	rc;

	if ((vidx == NULL) || (vidx->sysopt->notify == NULL)) {
		return ezdefault(vidx, event, param, opt, block);
	}

	rc = vidx->sysopt->notify(vidx, event, param, opt, block);
	if (rc == EN_EVENT_PASSTHROUGH) {
		return ezdefault(vidx, event, param, opt, block);
	}
	return rc;
}

static int ezdefault(EZVID *vidx, int event, long param, long opt, void *block)
{
	int	i;

	switch (event) {
	case EZ_ERR_LOWMEM:
		printf("%s: low memory [%ld]\n", (char*) block, param);
		break;
	case EZ_ERR_FORMAT:
		printf("%s: unknown format.\n", (char*) block);
		break;
	case EZ_ERR_STREAM:
		printf("%s: no stream info found.\n", (char*) block);
		break;
	case EZ_ERR_VIDEOSTREAM:
		printf("%s: no video stream found.\n", (char*) block);
		break;
	case EZ_ERR_CODEC_FAIL:
		printf("Could not open codec! %ld\n", param);
		break;

	case EN_FILE_OPEN:
		if (vidx->sysopt->flags & EZOP_CLI_INFO) {
			/* This is the ffmpeg function so it must run before
			 * disabling the av_log */
			i = av_log_get_level();
			av_log_set_level(AV_LOG_INFO);
#if	(LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 110, 0))
			av_dump_format(vidx->formatx, 0, block, 0);
#else
			dump_format(vidx->formatx, 0, block, 0);
#endif
			av_log_set_level(i);
		}
		if (EZOP_DEBUG(vidx->sysopt->flags) >= EZOP_DEBUG_WARNING) {
			printf("%s: open successed (%ld ms)\n", 
					(char*) block, opt);
		}
		break;
	case EN_MEDIA_OPEN:
		if (EZOP_DEBUG(vidx->sysopt->flags) >= EZOP_DEBUG_INFO) {
			dump_format_context(vidx->formatx);
		}
		if (vidx->sysopt->flags & EZOP_CLI_INFO) {
			printf("Duration in millisecond by ");
			/*if (vidx->sysopt->dur_mode == EZ_DUR_FULLSCAN) {
				printf("full scan: ");
			} else if (vidx->sysopt->dur_mode == EZ_DUR_QK_SCAN) {
				printf("fast scan: ");
			} else {
				printf("stream head: ");
			}*/
			if (vidx->ses_dura == EZ_DUR_FULLSCAN) {
				printf("full scan: ");
			} else if (vidx->ses_dura == EZ_DUR_QK_SCAN) {
				printf("fast scan: ");
			} else {
				printf("stream head: ");
			}
			/*printf("%d (%d:%ld)\n", vidx->duration,
					vidx->seekable, opt);*/
			SMM_PRINT("%lld (%ld ms)\n", 
					(long long) vidx->duration, opt);
		}
		if (vidx->sysopt->flags & EZOP_CLI_LIST) {
			ezdump_video_info(vidx);
		}
		break;
	case EN_IMAGE_CREATED:
		if (EZOP_DEBUG(vidx->sysopt->flags) >= EZOP_DEBUG_INFO) {
			dump_ezimage(block);
		}
		break;
	/**/
	case EN_PROC_BEGIN:
		break;
	case EN_PROC_CURRENT:
		break;
	case EN_PROC_END:
		printf("%s: %ldx%ld Canvas.\n", block ? (char*) block : "",
				param, opt);
		break;
	/**/

	case EN_PACKET_RECV:
		//dump_packet(block);
		break;
	case EN_PACKET_KEY:
		if (EZOP_DEBUG(vidx->sysopt->flags) >= EZOP_DEBUG_PACKET) {
			dump_packet(block);
		}
		break;
	case EN_FRAME_COMPLETE:
		//dump_frame(block, opt);
		break;
	case EN_FRAME_PARTIAL:
		//dump_frame(block, opt);
		break;
	case EN_FRAME_EFFECT:
		if (EZOP_DEBUG(vidx->sysopt->flags) >= EZOP_DEBUG_IFRAME) {
			dump_frame_packet(vidx, param, block);
		}
		break;
	case EN_SCAN_PACKET:
		//SMM_PRINT("Key Frame %d: %lld\n", param, *((long long *)block));
		break;
	case EN_SCAN_IFRAME:
		if (EZOP_DEBUG(vidx->sysopt->flags) >= EZOP_DEBUG_BRIEF) {
			printf("I-Frame Scanned (%ld ms):\n", opt);
			for (i = 0; i < param; i++) {
				SMM_PRINT("%9lld", ((long long *)block)[i]);
				if ((i % 8) == 7) {
					printf("\n");
				}
			}
			if ((i % 8) != 0) {
				printf("\n");
			}
		}
		break;
	case EN_STREAM_FORMAT:
		if (EZOP_DEBUG(vidx->sysopt->flags) >= EZOP_DEBUG_INFO) {
			dump_codec_attr(block, (int)param);
			//dump_stream(((AVFormatContext*)block)->streams
			//	[(int)param]);
		}
		break;
	case EN_TYPE_VIDEO:
		if (EZOP_DEBUG(vidx->sysopt->flags) > EZOP_DEBUG_INFO) {
			//dump_video_context(block);
			dump_codec_video(block);
		}
		break;
	case EN_TYPE_AUDIO:
		if (EZOP_DEBUG(vidx->sysopt->flags) > EZOP_DEBUG_INFO) {
			//dump_audio_context(block);
			dump_codec_audio(block);
		}
		break;
	case EN_TYPE_UNKNOWN:
		if (EZOP_DEBUG(vidx->sysopt->flags) > EZOP_DEBUG_INFO) {
			dump_other_context(block);
		}
		break;
	case EN_DURATION:
		if (param == ENX_DUR_REWIND) {
			SMM_PRINT("Rewound PTS: %lld < %lld\n",
					(long long)((AVPacket*) block)->dts,
					*((long long *) opt));
		}
		if (EZOP_DEBUG(vidx->sysopt->flags) >= EZOP_DEBUG_WARNING) {
			if (param == ENX_DUR_MHEAD) {
				printf("Duration from Media head: %ld (ms)\n",
						opt);
			} else if (param == ENX_DUR_JUMP) {
				SMM_PRINT("Duration from fast scan at %lld\n", 
						*((long long *)block));
			} else if (param == ENX_DUR_SCAN) {
				printf("Duration from scanning: %ld (ms)\n",
						opt);
			}
		}
		break;
	case EN_BUMP_BACK:
		SMM_PRINT("Bump back to %lld: %ld (%lld < %lld)\n",
				*((long long *) block), param,
				(long long) vidx->keydelta, 
				(long long) vidx->keygap);
		break;
	case EN_SEEK_FRAME:
		if (param == ENX_SEEK_BW_NO) {
			printf("WARNING: Backward Seeking Disabled.\n");
		} else if (EZOP_DEBUG(vidx->sysopt->flags) >= EZOP_DEBUG_WARNING) {
			SMM_PRINT("Backward Seeking Test successed to %lld\n",
					*((long long *) block));
		}
		break;
	case EN_MEDIA_STATIS:
		if (vidx->sysopt->flags & EZOP_CLI_INFO) {
			ezdump_media_statistics((struct MeStat *) param, 
					(int)opt, vidx);
		}
		break;
	case EN_IFRAME_CREDIT:
		/*
		switch (param) {
		case ENX_IFRAME_RESET:
			break;
		case ENX_IFRAME_SET:
			SMM_PRINT("Key Frame start from: %lld\n", 
					(long long) vidx->keylast);
			break;
		case ENX_IFRAME_UPDATE:
			SMM_PRINT("Key Frame Gap Update: %lld\n", 
					(long long) vidx->keygap);
			break;
		}*/
		break;
	case EN_FRAME_EXCEPTION:
		if (EZOP_DEBUG(vidx->sysopt->flags) >= EZOP_DEBUG_VERBS) {
			printf("Discard ");
			dump_frame(block, 1);
		}
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
			printf("%2d:%02d:%02d %10s [%d]: %s\n",
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
	int	i, ms;

	printf("Media: %s\n", vidx->filename);
	for (i = 0; i < n; i++) {
		printf("[%d] ", i);
		if (i >= vidx->formatx->nb_streams) {
			printf("ERROR  %8lu\n", mestat[i].received);
			continue;
		}
		
		switch(vidx->formatx->streams[i]->codec->codec_type) {
		case AVMEDIA_TYPE_VIDEO:
			printf("VIDEO  ");
			break;
		case AVMEDIA_TYPE_AUDIO:
			printf("AUDIO  ");
			break;
		case AVMEDIA_TYPE_SUBTITLE:
			printf("SUBTITL");
			break;
		default:
			printf("UNKNOWN");
			break;
		}
		ms = video_dts_to_ms(vidx, 
				mestat[i].dts_base + mestat[i].dts_last);
		printf(":%-8lu KEY:%-6lu REW:%lu  TIME:%d\n",
				mestat[i].received, mestat[i].key, 
				mestat[i].rewound, ms / 1000);
	}
	SMM_PRINT("Maximum Gap of key frames: %lld\n", 
			(long long) vidx->keygap);
	printf("Time used: %.3f\n", smm_time_diff(&vidx->tmark) / 1000.0);
	return 0;
}

int dump_format_context(AVFormatContext *format)
{
#if	LIBAVFORMAT_VERSION_INT < (53<<16)
	SMM_PRINT("  Format: %s(%s), Size: %lld, Bitrate: %u\n",
			format->iformat->long_name,
			format->iformat->name,
			(long long) format->file_size,
			format->bit_rate);
#else
	SMM_PRINT("  Format: %s(%s), Size: %lld, Bitrate: %u\n",
			format->iformat->long_name,
			format->iformat->name,
			(long long) avio_size(format->pb),
			format->bit_rate);
#endif
	SMM_PRINT("  Streams: %d, Start time: %lld, Duration: %lld\n",
			format->nb_streams,
			(long long) format->start_time,
			(long long) format->duration);
	return 0;
}

int dump_video_context(AVCodecContext *codec)
{
	printf("    Stream Video: %s %s, Time Base: %d/%d, Sample_AR: %d/%d\n",
			id_lookup(id_codec, codec->codec_id),
			id_lookup(id_pix_fmt, codec->pix_fmt),
			codec->time_base.num, codec->time_base.den,
			codec->sample_aspect_ratio.num,
			codec->sample_aspect_ratio.den);
	return 0;
}

int dump_audio_context(AVCodecContext *codec)
{
	printf("    Stream Audio: %s, Time Base: %d/%d, CH=%d SR=%d %s BR=%d\n",
			id_lookup(id_codec, codec->codec_id),
			codec->time_base.num, codec->time_base.den,
			codec->channels, codec->sample_rate,
			id_lookup(id_sam_format, codec->sample_fmt),
			codec->bit_rate);
	return 0;
}

int dump_other_context(AVCodecContext *codec)
{
	printf("    Stream %s:\n",
			id_lookup_tail(id_codec_type, codec->codec_type));
	return 0;
}

int dump_codec_attr(AVFormatContext *format, int i)
{
	AVCodec	*codec;

	codec = avcodec_find_decoder(format->streams[i]->codec->codec_id);
	printf("Stream #%d: %s Codec ID: %s '%s' %s\n", i, 
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
	printf("  Codec Type  : %s, Codec ID: %s (avcodec.h)\n",
			id_lookup(id_codec_type, codec->codec_type), 
			id_lookup(id_codec, codec->codec_id));
	printf("  Bit Rates   : %d, Time Base: %d/%d\n", 
			codec->bit_rate,
			codec->time_base.num, codec->time_base.den);
	printf("  Frame Number: %d, Width: %d, Height: %d, "
				"Sample_AR: %d/%d%s\n",
			codec->frame_number, codec->width, codec->height, 
			codec->sample_aspect_ratio.num, 
			codec->sample_aspect_ratio.den,
			(0 == codec->sample_aspect_ratio.num) ? "(-)" : "(+)");
	printf("  Pixel Format: %s (pixfmt.h), Has B-Frame: %d\n", 
			id_lookup(id_pix_fmt, codec->pix_fmt), 
			codec->has_b_frames);
	return 0;
}

int dump_codec_audio(AVCodecContext *codec)
{
	printf("  Codec Type  : %s, Codec ID: %s (avcodec.h)\n",
			id_lookup(id_codec_type, codec->codec_type), 
			id_lookup(id_codec, codec->codec_id));
	printf("  Bit Rates   : %d, Time Base: %d/%d\n", 
			codec->bit_rate,
			codec->time_base.num, codec->time_base.den);
	printf("  Channel     : %d, Sample Rate: %d, Sample Format: %d\n",
			codec->channels, 
			codec->sample_rate, codec->sample_fmt);
	return 0;
}

int dump_packet(AVPacket *p)
{
	/* PTS:Presentation timestamp.  DTS:Decompression timestamp */
	printf("Packet Pos:%" PRId64 ", PTS:%" PRId64 ", DTS:%" PRId64 
			", Dur:%d, Siz:%d, Flag:%d, SI:%d\n",
			p->pos, p->pts,	p->dts, p->duration, p->size,
			p->flags, p->stream_index);
	return 0;
}

int dump_frame(AVFrame *frame, int got_pic)
{
	printf("Frame %s, KEY:%d, CPN:%d, DPN:%d, REF:%d, I:%d, Type:%s\n", 
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

	dts = ezfrm->rf_dts;
	if (vidx->formatx->start_time) {
		dts -= video_system_to_dts(vidx,
				vidx->formatx->start_time);
	}
	meta_timestamp((int)video_dts_to_ms(vidx, dts), 1, timestamp);
	SMM_PRINT("Frame %3d: Pos:%lld Size:%d PAC:%d DTS:%lld (%s) Type:%s\n",
			sn, (long long) ezfrm->rf_pos, ezfrm->rf_size, 
			ezfrm->rf_pac, (long long) ezfrm->rf_dts, timestamp, 
			id_lookup(id_pict_type, ezfrm->frame->pict_type));
	return 0;
}

int dump_stream(AVStream *stream)
{
	SMM_PRINT("Stream:%d, FRate:%d/%d, Time Base:%d/%d, Start Time:%" 
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

int dump_ezimage(EZIMG *image)
{
	printf("\n>>>>>>>>>>>>>>>>>>\n");
	printf("Single shot size:  %dx%dx%d-%d\n", 
			image->dst_width, image->dst_height, image->dst_pixfmt,
			image->sysopt->edge_width);
	printf("Grid size:         %dx%d+%d\n", 
			image->grid_col, image->grid_row,
			image->sysopt->shadow_width);
	printf("Canvas size:       %dx%d-%d\n", 
			image->canvas_width, image->canvas_height, 
			image->canvas_minfo);
	SMM_PRINT("Time setting:      %lld-%lld %lld (ms)\n", 
			(long long) image->time_from, 
			(long long) image->time_during, 
			(long long) image->time_step);
	printf("Margin of canvas:  %dx%d\n", 
			image->rim_width, image->rim_height);
	printf("Gap of shots:      %dx%d\n", 
			image->gap_width, image->gap_height);
	printf("Color of Canvas:   BG#%08X SH#%08X MI#%08X\n",
			(unsigned) image->color_canvas,
			(unsigned) image->color_shadow,
			(unsigned) image->color_minfo);
	printf("Color of Shots:    ED#%08X IN#%08X SH#%08X\n",
			(unsigned) image->color_edge,
			(unsigned) image->color_inset,
			(unsigned) image->color_inshadow);
	printf("Font size:         MI=%d IN=%d (SH: %d %d)\n", 
			image->sysopt->mi_size, image->sysopt->ins_size,
			image->sysopt->mi_shadow, image->sysopt->ins_shadow);
	printf("Font position:     MI=%d IN=%d\n",
			image->sysopt->mi_position, 
			image->sysopt->ins_position);
	if (image->sysopt->mi_font) {
		printf("Font MediaInfo:    %s\n", image->sysopt->mi_font);
	}
	if (image->sysopt->ins_font) {
		printf("Font Inset Shots:  %s\n", image->sysopt->ins_font);
	}
	printf("Output file name:  %s.%s (%d)\n", 
			image->sysopt->suffix, image->sysopt->img_format,
			image->sysopt->img_quality);
	printf("Flags:             %s %s %s %s %s %s %s %s D%d P%d\n", 
			image->sysopt->flags & EZOP_INFO ? "MI" : "",
			image->sysopt->flags & EZOP_TIMEST ? "TS" : "",
			image->sysopt->flags & EZOP_FFRAME ? "FF" : "",
			image->sysopt->flags & EZOP_LFRAME ? "LF" : "",
			image->sysopt->flags & EZOP_P_FRAME ? "PF" : "",
			image->sysopt->flags & EZOP_CLI_INFO ? "CI" : "",
			image->sysopt->flags & EZOP_CLI_LIST ? "CL" : "",
			image->sysopt->flags & EZOP_TRANSPARENT ? "TP" : "",
			EZOP_DEBUG(image->sysopt->flags) >> 12,
			((image->sysopt->flags & EZOP_PROC_MASK) >> 16) & 15);
	printf("Font numerate:     %dx%d %dx%d %dx%d %dx%d %dx%d\n",
			gdFontGetTiny()->w, gdFontGetTiny()->h,
			gdFontGetSmall()->w, gdFontGetSmall()->h,
			gdFontGetMediumBold()->w, gdFontGetMediumBold()->h,
			gdFontGetLarge()->w, gdFontGetLarge()->h,
			gdFontGetGiant()->w, gdFontGetGiant()->h);
	printf("Background Image:  %s (0x%x)\n", image->sysopt->background,
			image->sysopt->bg_position);

	ezopt_profile_dump(image->sysopt, 
			"Profile of Grid:   ", "Profile of Shots:  ");
	printf("<<<<<<<<<<<<<<<<<<\n");
	return 0;
}




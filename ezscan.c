
#include <stdio.h>
#include <stdlib.h>

#include "ezthumb.h"
#include "id_lookup.h"


int ez_scan_duration(char *filename, int flag)
{
	EZVID	*vidx;
	int64_t	duration;

	if ((vidx = video_allocate(filename, NULL)) == NULL) {
		perror(filename);
		return EZ_ERR_FILE;
	}

	duration = video_duration(vidx, flag);
	printf("video_duration: %lld\n", duration);

	video_free(vidx);
	return 0;
}


int ez_scan_info(char *filename, int flag)
{
	EZVID		*vidx;
	AVFrame		*frame = NULL;
	AVPacket	packet;
	int		pall, ffin = 1, fall, fkey, fbrk;

	if ((vidx = video_allocate(filename, NULL)) == NULL) {
		perror(filename);
		return EZ_ERR_FILE;
	}

	if ((frame = avcodec_alloc_frame()) == NULL) {		/* Allocate video frame */
		return -1;
	}

	fall = fkey = fbrk = pall = 0;
	while (av_read_frame(vidx->formatx, &packet) >= 0) {
		if (packet.stream_index != vidx->vsidx) {
			av_free_packet(&packet);
			continue;
		}
		
		pall++;		/* video packets increased */
		switch (flag & 0xff) {
		case 0:		/* do not decode any packet */
			if (packet.flags == PKT_FLAG_KEY) {
				dump_packet(&packet);
				fkey++;
			} else if (flag & 0x100) {	/* verbose mode */
				dump_packet(&packet);
			}
			break;

		case 1:		/* only decode key frames */
			if (packet.flags == PKT_FLAG_KEY) {
				dump_packet(&packet);
				fkey++;
			} else {
				if (flag & 0x100) {      /* verbose mode */
					dump_packet(&packet);
				}
				if (ffin) {
					break;
				}
			}
	
			avcodec_decode_video2(vidx->codecx, frame, &ffin, &packet);
			if (ffin == 0) {	/* the packet is not finished */
				fbrk++;
			}
			break;

		case 2:		/* decode all frames */
			avcodec_decode_video2(vidx->codecx, frame, &ffin, &packet);
			if (packet.flags == PKT_FLAG_KEY) {
				dump_packet(&packet);
			} else if (flag & 0x100) { 
				dump_frame_packet(&packet, frame, ffin);
			}
			if (ffin == 0) {	/* the packet is not finished */
				fbrk++;
			} else {
				fall++;
				if (frame->key_frame) {
					fkey++;
				}
			}
			break;
		}
		av_free_packet(&packet);
	}
	av_free(frame);
	printf("Total:%d Frame:%d Key:%d Broken:%d\n", pall, fall, fkey, fbrk);

	video_free(vidx);
	return 0;
}

int ez_scan_save(char *filename, char *filetype, int allframe)
{
	AVFrame		*frame = NULL;
	AVPacket	packet;
	EZIMG		*image;
	EZVID		*vidx;
	int		i, ffin;

	if ((vidx = video_allocate(filename, NULL)) == NULL) {
		perror(filename);
		return EZ_ERR_FILE;
	}

	if ((frame = avcodec_alloc_frame()) == NULL) {		/* Allocate video frame */
		return -1;
	}
	if ((image = image_allocate(vidx, NULL)) == NULL) {
		av_free(frame);
		return -2;
	}
	
	i = 0;
	while (av_read_frame(vidx->formatx, &packet) >= 0) {
		if (packet.stream_index != vidx->vsidx) {
			av_free_packet(&packet);
			continue;
		}
		if ((packet.flags != PKT_FLAG_KEY) && (allframe == 0)) {
			av_free_packet(&packet);
			continue;
		}
		//dump_packet(&packet, pStream);

		avcodec_decode_video2(vidx->codecx, frame, &ffin, &packet);
		//dump_packet(&packet);
		dump_frame(frame, ffin);
		if (ffin == 0) {	/* the packet is not finished */
			av_free_packet(&packet);
		}

		image_scale(image, frame);
		if (!strcmp(filetype, "ppm")) {
			image_save_ppm(image, i);
		} else if (!strcmp(filetype, "ffmpeg")) {
			image_save_ffmpeg(image, vidx->codecx, frame, i);
		} else {	/* GD JPEG */
			image_save_jpg(image, i);
		}
		i++;
	}
	image_free(image);
	av_free(frame);

	video_free(vidx);
	return 0;
}

int ez_scan_segment(EZVID *vidx, EZIMG *image, AVFrame *frame, int64_t pts, int sn)
{
	AVPacket	packet;
	int64_t	duration, nowpts;
	int	ffin;

	av_seek_frame(vidx->formatx, vidx->vsidx, pts, AVSEEK_FLAG_BACKWARD);
	vidx->codecx->hurry_up = 1;
	while (av_read_frame(vidx->formatx, &packet) >= 0) {
		if (packet.stream_index != vidx->vsidx) {
			av_free_packet(&packet);
			continue;
		}

		nowpts = packet.pts;

		/* must to decode every frame since the key frame to keep
		 * the picture properly */
		avcodec_decode_video2(vidx->codecx, frame, &ffin, &packet);
		dump_frame_packet(&packet, frame, ffin);
		av_free_packet(&packet);

		if ((nowpts >= pts) && ffin) {
			image_scale(image, frame);
			image_save_ppm(image, sn);
			break;
		}
	}
	vidx->codecx->hurry_up = 0;
	return 0; 
}

int64_t vseek_key_frame(EZVID *vidx)
{
	AVPacket	packet;

	vidx->seek_pos = vidx->seek_pts = -1;
	while (av_read_frame(vidx->formatx, &packet) >= 0) {
		if ((packet.stream_index == vidx->vsidx) && (
					packet.flags == PKT_FLAG_KEY)) {
			vidx->seek_pts = packet.pts;
			vidx->seek_pos = packet.pos;
			//dump_packet(&packet);
			av_free_packet(&packet);
			break;
		}
		av_free_packet(&packet);
	}
	return vidx->seek_pts;
}


int64_t vseek_prev_key_frame(EZVID *vidx, int64_t seekto)
{
	int64_t		pts_end, pts_near, pos_near, pts, range;

	av_seek_frame(vidx->formatx, vidx->vsidx, seekto, 0);
	pts_end = vseek_key_frame(vidx);

	pts_near = pos_near = -1;
	range = 1000;

	seekto = seekto > range ? seekto - range : 0;
	av_seek_frame(vidx->formatx, vidx->vsidx, seekto, 0);
	while (1) {
		pts = vseek_key_frame(vidx);
		if (pts < pts_end) {
			pts_near = vidx->seek_pts;
			pos_near = vidx->seek_pos;
		} else if (pts_near > 0) {
			vidx->seek_pts = pts_near;
			vidx->seek_pos = pos_near;
			break;
		} else if (seekto == 0) {
			break;
		} else {
			range <<= 1;
			seekto = seekto > range ? seekto - range : 0;
			av_seek_frame(vidx->formatx, vidx->vsidx, seekto, 0);
		}
	}
	return pts_near;
}

int ez_screenshot(EZVID *vidx, AVFrame *frame, EZIMG *image, int64_t pts, int pidx)
{
	AVPacket	packet;
	int		ffin;

	while (av_read_frame(vidx->formatx, &packet) >= 0) {
		if (packet.stream_index != vidx->vsidx) {
			av_free_packet(&packet);
			continue;
		}
		dump_packet(&packet);
		if (packet.pts < pts) {
			av_free_packet(&packet);
			continue;
		}
		/*if (packet.flags != PKT_FLAG_KEY) {
			av_free_packet(&packet);
			continue;
		}*/

		avcodec_decode_video2(vidx->codecx, frame, &ffin, &packet);
		dump_frame_packet(&packet, frame, ffin);
		av_free_packet(&packet);

		if (ffin) { 	/* the packet is finished */
			image_scale(image, frame);
			image_save_ppm(image, pidx);
			return 0;
		}
	}
	return -1;
}


int image_save_jpg(EZIMG *image, int idx)
{
	FILE	*fout;
	char	filename[32];

	if (image == NULL) {
		return -1;
	}

	sprintf(filename, "pic%03d.jpg", idx);
	if ((fout = fopen(filename, "wb")) == NULL) {
		perror(filename);
		return -2;
	}

	/* FrameRGB_2_gdImage(image->rgb_frame, image->gdframe, image->dst_width, image->dst_height); */
	image_gdframe_update(image);

	gdImageJpeg(image->gdframe, fout, 90);
	fclose(fout);
	return 0;
}

int image_save_ppm(EZIMG *image, int idx)
{
	FILE	*fout;
	char	filename[32];
	int	i;

	if (image == NULL) {
		return -1;
	}

	sprintf(filename, "pic%03d.ppm", idx);
	if ((fout = fopen(filename, "wb")) == NULL) {
		perror(filename);
		return -2;
	}

	fprintf(fout, "P6\n%d %d\n255\n", image->dst_width,image->dst_height);
	for (i = 0; i < image->dst_height; i++) {
		fwrite(image->rgb_frame->data[0] + 
				i * image->rgb_frame->linesize[0], 
				1, image->dst_width * 3, fout);
	}
	fclose(fout);
	return 0;
}

int image_save_ffmpeg(EZIMG *image, AVCodecContext *pCodecCtx, AVFrame *frame, int idx)
{
	AVCodecContext	*encodex;
	AVCodec		*codec;
	uint8_t		*Buffer;
	int		BufSiz;
	int		BufSizActual;
	int		ImgFmt = PIX_FMT_YUVJ420P;
	FILE    *fout;
	char    filename[32];


	BufSiz = avpicture_get_size (ImgFmt, pCodecCtx->width, pCodecCtx->height);
	if ((Buffer = av_malloc(BufSiz*2)) == NULL) {
		return -1;
	}
	if ((encodex = avcodec_alloc_context()) == NULL) {
		av_free(Buffer);
		return -2;
	}
	
	encodex->bit_rate      = pCodecCtx->bit_rate;
	encodex->width         = pCodecCtx->width;
	encodex->height        = pCodecCtx->height;
	encodex->pix_fmt       = ImgFmt;
	encodex->codec_id      = CODEC_ID_MJPEG;
	encodex->codec_type    = CODEC_TYPE_VIDEO;
	encodex->time_base.num = pCodecCtx->time_base.num;
	encodex->time_base.den = pCodecCtx->time_base.den; 

	if ((codec = avcodec_find_encoder(encodex->codec_id)) == NULL) {
		avcodec_close ( encodex );
		av_free(Buffer);
		return -3;
	}
	if (avcodec_open ( encodex, codec ) < 0 ) { 
		avcodec_close ( encodex );
		av_free(Buffer);
		return -4;
	}

	encodex->mb_lmin        = encodex->lmin = encodex->qmin * FF_QP2LAMBDA;
	encodex->mb_lmax        = encodex->lmax = encodex->qmax * FF_QP2LAMBDA;
	encodex->flags          = CODEC_FLAG_QSCALE;
	encodex->global_quality = encodex->qmin * FF_QP2LAMBDA;

	frame->pts     = 1;
	frame->quality = encodex->global_quality;
	BufSizActual = avcodec_encode_video(encodex,Buffer,BufSiz,frame ); 

	sprintf(filename, "ppc%03d.jpg", idx);
	if ((fout = fopen(filename, "wb")) == NULL) {
		perror(filename);
		avcodec_close ( encodex );
		av_free(Buffer);
		return -2;
	}

	fwrite ( Buffer, 1, BufSizActual, fout ); 
	fclose(fout);

	avcodec_close ( encodex ); 
	av_free(Buffer);
	return 0;
}


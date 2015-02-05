
/*  conftest.c - configure test
 
    Copyright (C) 2015  "Andy Xuming" <xuming@users.sourceforge.net>

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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "ezthumb.h"

int main(void)
{
	printf("Ezthumb %s\n", EZTHUMB_VERSION);
	printf("FFMPEG: libavcodec %d.%d.%d; ", LIBAVCODEC_VERSION_MAJOR,
			LIBAVCODEC_VERSION_MINOR, LIBAVCODEC_VERSION_MICRO);
	printf("libavformat %d.%d.%d; ", LIBAVFORMAT_VERSION_MAJOR, 
			LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO);
	printf("libavutil %d.%d.%d; ", LIBAVUTIL_VERSION_MAJOR, 
			LIBAVUTIL_VERSION_MINOR, LIBAVUTIL_VERSION_MICRO);
	printf("libswscale %d.%d.%d\n", LIBSWSCALE_VERSION_MAJOR, 
			LIBSWSCALE_VERSION_MINOR, LIBSWSCALE_VERSION_MICRO);
#ifdef	TEST_AV_FRAME_ALLOC
	av_frame_alloc();
#endif
#ifdef	TEST_AVFORMAT_OPEN_INPUT
	avformat_open_input(NULL, NULL, NULL, NULL);
#endif
#ifdef	TEST_AVFORMAT_FIND_STREAM_INFO	
	avformat_find_stream_info(NULL, NULL);
#endif
#ifdef	TEST_AVCODEC_OPEN2
	avcodec_open2(NULL, NULL, NULL);
#endif
#ifdef	TEST_AV_FIND_BEST_STREAM
	av_find_best_stream(NULL, 0, 0, 0, NULL, 0);
#endif
#ifdef	TEST_AV_DICT_GET
	av_dict_get(NULL, NULL, NULL, 0);
#endif
#ifdef	TEST_AV_METADATA_GET
	av_metadata_get(NULL, NULL, NULL, 0);
#endif
#ifdef	TEST_AV_DUMP_FORMAT
	av_dump_format(NULL, 0, NULL, 0);
#endif
#ifdef	TEST_AVFORMATCONTEXT_PB
	/* to simplify the life, this test includes avio_size() */
	{
		AVFormatContext format;
		format.pb = NULL;
		avio_size(NULL);
	}
#endif
#ifdef	TEST_GD_IMAGE_GIFANIM
	gdImageGifAnimBegin(NULL, NULL, 0, 0);
#endif
#ifdef	TEST_GD_USE_FONTCONFIG
	gdFTUseFontConfig(1);
#endif
	return 0;
}


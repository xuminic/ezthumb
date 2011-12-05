#include <stdio.h>
#include "ezthumb.h"

int main(void)
{
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
}


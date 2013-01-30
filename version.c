#include <stdio.h>
#include "ezthumb.h"


int main(int argc, char **argv)
{
	while (--argc && (**++argv == '-')) {
		if (!strcmp(*argv, "--help")) {
			printf("Usage: ./version [--ffmpeg][--version]\n");
			return 0;
		} else if (!strcmp(*argv, "--version")) {
			printf("%s (%d-bit)\n", EZTHUMB_VERSION, 
					(int)(sizeof(void*) << 3));
			return 0;
		} else if (!strcmp(*argv, "--ffmpeg")) {
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

	}
	printf("%s\n", EZTHUMB_VERSION);
	return 0;
}




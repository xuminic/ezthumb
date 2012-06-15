#include <stdio.h>
#include <math.h>
#include "ezthumb.h"

static int transolution(int ref, int wid);
static int transcurves(float exp);


int main(int argc, char **argv)
{
	int	i, res;
	int	stdres[] = { 160, 240, 320, 512, 640, 704, 720, 800, 1024,
				1152, 1280, 1366, 1440, 1680, 1920, 2048, 
				2560, 2880 };

	while (--argc && (**++argv == '-')) {
		if (!strcmp(*argv, "--help")) {
			printf("Usage: ./version [--ffmpeg][--version]\n");
			return 0;
		} else if (!strcmp(*argv, "--version")) {
			printf("%s\n", EZTHUMB_VERSION);
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
		} else if (!strcmp(*argv, "-r")) {
			if ((++argv, --argc) == 0) {
				return -1;
			}
			res = (int) strtol(*argv, NULL, 0);
			if (res) {
				printf("Reference: %d\n", transolution(320, res));
				return 0;
			}
			for (i = 0; i < sizeof(stdres)/sizeof(int); i++) {
				printf("Reference: %d\n", transolution(320, stdres[i]));
			}
			return 0;
		} else if (!strcmp(*argv, "-c")) {
			if ((++argv, --argc) == 0) {
				return -1;
			}
			transcurves(strtof(*argv, NULL));
			return 0;
		}

	}
	printf("%s\n", EZTHUMB_VERSION);
	return 0;
}

static int transolution(int ref, int wid)
{
	float	ratio, error;
	int	i, neari;

	if (wid > ref) {
		ratio = (float) wid / ref;
	} else {
		ratio = (float) ref / wid;
	}

	//printf("ratio: %f\n", ratio);
	neari = (int)(ratio + 0.5);
	for (i = 0; i < 32; i++) {
		if (neari == (1 << i)) {
			break;
		}
	}
	if (i == 32) {
		return ref;
	}
	
	//printf("neari: %d\n", neari);
	error = abs(neari * ref - wid) / ref;
	if (error > 0.2) {
		return ref;
	}
	if (wid > ref) {
		return wid / neari;
	}
	return wid * neari;
}


#define LOGA(x)	((int)(log(x)/log(exp)))
static int transcurves(float exp)
{
	int	sample[] = { 30, 60, 90, 120, 180, 240, 300, 360 };
	int	i, cali, shots[32];

	/* first, recalibre from 1 to 29 */
	for (i = 0; i < 6; i++) {
		cali = i * 5 + 1;
		printf("%4d: %4d (%d)\n", cali, cali + 3, (cali + 3) / cali);
	}
	printf("\n");

	/* second, recalibre from 30 */
	for (i = 0; i < sizeof(sample)/sizeof(int); i++) {
		shots[i] = LOGA(sample[i]);
	}
	cali = shots[0] - 32;
	for (i = 0; i < sizeof(sample)/sizeof(int); i++) {
		shots[i] -= cali;
		printf("%4d: %4d (%d)\n", sample[i], shots[i], sample[i] * 60 / shots[i]);
	}

	return 0;
}



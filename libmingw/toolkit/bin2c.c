
#include <stdio.h>

int main(int argc, char **argv)
{
	FILE	*fp;
	long	i, flen;
	unsigned char	buf[16];

	if (argc < 2) {
		printf("Usage: %s filename.bin\n", argv[0]);
		return 1;
	}

	if ((fp = fopen(argv[1], "rb")) == NULL) {
		perror(argv[1]);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	flen = ftell(fp);
	rewind(fp);

	printf("\n/* %s */\n\n", argv[1]);
	printf("unsigned int  %s_len = %ld;\n", argv[1], flen);
	printf("unsigned char %s[] = {\n", argv[1]);

	while ((flen = fread(buf, 1, 16, fp)) > 0) {
		printf("\t");
		for (i = 0; i < flen; i++) {
			printf("%3d,", buf[i]);
		}
		printf("\n");
	}
	printf("};\n");
	fclose(fp);
	return 0;
}
	




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcsoup.h"
#include "csoup_internal.h"

int main(int argc, char **argv)
{
	FILE	*fp;
	char	buf[256], *argvs[8], *payload;
	int	rc;
	CSCLNK	*anchor, *node;

	if (argc < 2) {
		printf("Usage: %s extern_list_head_file\n", argv[0]);
		return 0;
	}

	if ((fp = fopen(argv[1], "r+")) == NULL) {
		perror(argv[1]);
		return -1;
	}

	anchor = NULL;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		rc = csc_ziptoken(buf, argvs, 8, " ");
		if (rc < 4) {
			continue;
		}
		if (strcmp(argvs[2], "clicmd")) {
			continue;
		}

		node = csc_cdl_list_alloc_tail(&anchor, strlen(argvs[3])+8);
		if (node == NULL) {
			break;
		}
		payload = csc_cdl_payload(node);
		strcpy(payload, argvs[3]);
		rc = strlen(payload) - 1;
		if (payload[rc] == ';') {
			payload[rc] = 0;
		}	
	}

	fprintf(fp, "\nstruct	clicmd	*cmdlist[] = {\n");
	for (node = anchor; node; node = csc_cdl_next(anchor, node)) {
		payload = csc_cdl_payload(node);
		fprintf(fp, "	&%s,\n", payload);
	}
	fprintf(fp, "	NULL\n};\n\n");

	csc_cdl_list_destroy(&anchor);
	fclose(fp);
	return 0;
}


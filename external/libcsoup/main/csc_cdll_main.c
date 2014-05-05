#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "libcsoup.h"

int csc_cdll_main(int argc, char **argv)
{
	CSCLNK	*node, *anchor = NULL;
	char	*cont[] = { "Hello", "World", "Peace", "Love", "Bullshit", NULL };

	node = csc_cdl_alloc_head(&anchor, 16);
	strcpy((char*)node->payload, cont[0]);

	node = csc_cdl_alloc_head(&anchor, 16);
	strcpy((char*)node->payload, cont[1]);

	node = csc_cdl_alloc_tail(&anchor, 16);                
	strcpy((char*)node->payload, cont[2]);

	for (node = anchor; node != NULL; node = csc_cdl_next(anchor, node)) {
		printf("%s\n", (char*)node->payload);
	}
	return 0;
}


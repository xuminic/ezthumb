#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "libcsoup.h"
#include "libcsoup_debug.h"


static int csc_cdll_print_test_block(CSCLNK *root)
{
	CSCLNK	*node;

	for (node = root; node; node = csc_cdl_next(root, node)) {
		cslog("%s ", csc_cdl_payload(node));
	}
	cslog("\n");
	return 0;
}

static int csc_cdll_free_test_block(CSCLNK *root)
{
	CSCLNK	*node;

	while (root) {
		node = root;
		root = csc_cdl_remove(root, node);
		smm_free(node);
	}
	return 0;
}

static int csc_cdll_my_compare(void *src, void *dst)
{
	return strcmp(src, dst);
}

static int csc_cdll_basic_function(void)
{
	CSCLNK	*root, *node;
	char	*value;
	int	i;

	for (i = 0, root = NULL; i < 16; i++) {
		if ((node = csc_cdl_alloc(16)) != NULL) {
			value = csc_cdl_payload(node);
			value[0] = 'A' + i;
			value[1] = 0;
			root = csc_cdl_insert_head(root, node);
		}
	}
	cslog("Stack:  ");
	csc_cdll_print_test_block(root);
	csc_cdll_free_test_block(root);

	for (i = 0, root = NULL; i < 16; i++) {
		if ((node = csc_cdl_alloc(16)) != NULL) {
			value = csc_cdl_payload(node);
			value[0] = 'A' + i;
			value[1] = 0;
			root = csc_cdl_insert_tail(root, node);
		}
	}
	cslog("FIFO:   ");
	csc_cdll_print_test_block(root);

	node = csc_cdl_search(root, NULL, csc_cdll_my_compare, "D");
	if (node) {
		cslog("Search: %s\n", csc_cdl_payload(node));
	} else {
		cslog("Search: not found\n");
	}

	for (i = 0, node = root; node; node = csc_cdl_next(root, node), i++) {
		if (i & 1) {
			cslog("Removing: %s\n", csc_cdl_payload(node));
			root = csc_cdl_remove(root, node);
		}
	}
	cslog("Removed: ");
	csc_cdll_print_test_block(root);

	i = 3;
	node = csc_cdl_goto(root, i);
	if (node) {
		cslog("Goto/%d: %s\n", i, csc_cdl_payload(node));
	} else {
		cslog("Goto/%d: out of range.\n", i);
	}

	cslog("Number: %d\n", csc_cdl_index(root, NULL));
	csc_cdll_free_test_block(root);
	return 0;
}

static int csc_cdll_list_function(void)
{
	CSCLNK	*node, *anchor = NULL;
	char	*cont[] = { "Hello", "World", "Peace", "Love", "Bullshit", NULL };

	
	node = csc_cdl_list_alloc_head(&anchor, 16);
	strcpy(csc_cdl_payload(node), cont[0]);

	node = csc_cdl_list_alloc_head(&anchor, 16);
	strcpy(csc_cdl_payload(node), cont[1]);

	node = csc_cdl_list_alloc_tail(&anchor, 16);                
	strcpy(csc_cdl_payload(node), cont[2]);

	cslog("State:  %d\n", csc_cdl_list_state(&anchor));
	for (node = anchor; node; node = csc_cdl_next(anchor, node)) {
		cslog("%s\n", csc_cdl_payload(node));
	}

	csc_cdl_list_destroy(&anchor);
	return 0;
}

int csc_cdll_main(void *rtime, int argc, char **argv)
{
	/* stop the compiler complaining */
	(void) rtime; (void) argc; (void) argv;
	
	csc_cdll_basic_function();
	csc_cdll_list_function();
	return 0;
}

struct	clicmd	cdll_cmd = {
	"csc_cdll", csc_cdll_main, NULL, "Testing the functions of doubly circular link list"
};

extern  struct  clicmd  cdll_cmd;


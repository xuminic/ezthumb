#include <stdio.h>
#include <string.h>

#include "libcsoup.h"
#include "csoup_internal.h"

#include "coo_frmwk.h"

typedef	enum	{
	Attrib_00,
	Attrib_01,
	Attrib_02,
	Attrib_03,
	Attrib_04,
	Attrib_05,
	Attrib_06,
	Attrib_07,
	Attrib_08,
	Attrib_09,
	Attrib_10,
	Attrib_11,
	Attrib_12,
	Attrib_13,
	Attrib_14,
	Attrib_15
} ATTRIB;


static int coo_class_write_over(MUnit *unit, int type, void *buf, int len);
static MUnit *coo_class_block_find(CCLASS *class, int id);
static MUnit *coo_class_block_add(CCLASS *class, int id, int type);
static MUnit *coo_class_link_find(CCLASS *class, int id);
static MUnit *coo_class_link_add(CCLASS *class, int id, int type, int plsize);
static int coo_class_dump_munit(MUnit *unit);

static	CCLASS	*coo_root = NULL;


CCLASS *coo_class_new(char *cname, int attrno)
{
	CCLASS	*class;

	class = smm_alloc(sizeof(CCLASS) + sizeof(MUnit) * attrno);
	if (class == NULL) {
		return NULL;
	}

	class->cname = cname;
	if (attrno == 0) {
		class->repo.d_point = NULL;
		class->repo.type = COO_DTYPE_POINTER | COO_MTYPE_LINK;
	} else {
		class->repo.d_point = class->pool;
		class->repo.type = COO_DTYPE_POINTER | COO_MTYPE_PREDEF;
	}
	class->total = attrno;
	return class;
}


int coo_class_destroy(CCLASS *class)
{
	CSCLNK	*node, *anchor;
	MUnit	*unit;
	int	i;

	if (COO_MTYPE_GET(class->repo.type) == COO_MTYPE_PREDEF) {
		unit = class->repo.d_point;
		for (i = 0; i < class->total; i++) {
			if (COO_MTYPE_GET(unit->type) == COO_MTYPE_ALLOC) {
				smm_free(unit->d_point); /* same to string */
			}
		}
	} else {	/* should be COO_MTYPE_LINK */
		anchor = class->repo.d_point;
		for (node = anchor; node; node = csc_cdl_next(anchor, node)) {
			unit = (MUnit *) csc_cdl_payload(node);
			if (COO_MTYPE_GET(unit->type) == COO_MTYPE_ALLOC) {
				smm_free(unit->d_point); /* same to string */
			}
		}
		csc_cdl_list_destroy(&anchor);
	}
	smm_free(class);
	return 0;
}

int coo_class_read(CCLASS *class, int id, void *output)
{
	MUnit	*unit;

	if (COO_MTYPE_GET(class->repo.type) == COO_MTYPE_PREDEF) {
		unit = coo_class_block_find(class, id);
	} else {
		unit = coo_class_link_find(class, id);
	}
	if (unit == NULL) {
		return -1;	/* attribute not found */
	}

	switch (COO_DTYPE_GET(unit->type)) {
	case COO_DTYPE_I32:
		*((int *)output) = unit->d_int;
		break;
	case COO_DTYPE_I64:
		*((long long *)output) = unit->d_int64;
		break;
	case COO_DTYPE_STRING:
	case COO_DTYPE_POINTER:
		*((char **)output) = unit->d_string;
		break;
	case COO_DTYPE_FUNC:
		*((MFunc *)output) = unit->d_func;
		break;
	default:
		return -2;	/* should not */
	}
	return 0;
}

int coo_class_write(CCLASS *class, int id, int type, void *buf, int len)
{
	MUnit	*unit;
	int	rc;

	if ((len <= 0) && (COO_DTYPE_GET(type) == COO_DTYPE_STRING)) {
		len = strlen(buf) + 1;
	}
	
	if (COO_MTYPE_GET(class->repo.type) == COO_MTYPE_PREDEF) {
		if ((unit = coo_class_block_find(class, id)) != NULL) {
			return coo_class_write_over(unit, type, buf, len);
		}
		if ((unit = coo_class_block_add(class, id, type)) == NULL) {
			return -1;	/* allocating unit failed */
		}
		if ((rc = coo_class_write_over(unit, type, buf, len)) != 0) {
			unit->type = COO_DTYPE_NONE;
			return rc;
		}
	} else {
		if ((unit = coo_class_link_find(class, id)) != NULL) {
			return coo_class_write_over(unit, type, buf, len);
		}
		if (COO_MTYPE_GET(type) == COO_MTYPE_PREDEF) {
			unit = coo_class_link_add(class, id, type, 0);
			if (unit == NULL) {
				return -1;	/* allocating unit failed */
			}
			rc = coo_class_write_over(unit, type, buf, len);
		} else {
			unit = coo_class_link_add(class, id, type, len);
			if (unit == NULL) {
				return -1;	/* allocating unit failed */
			}
			memcpy(&unit[1], buf, len);
			type = COO_MTYPE_SET(type, COO_MTYPE_PREDEF);
			rc = coo_class_write_over(unit, type, &unit[1], len);
		}
		if (rc != 0) {
			CSCLNK	*anchor = class->repo.d_point;
			csc_cdl_list_free(&anchor, csc_cdl_paylink(unit));
			class->total--;
			return rc;
		}
	}
	return 0;
}

int coo_class_delete(CCLASS *class, int id)
{
	MUnit	*unit;

	if (COO_MTYPE_GET(class->repo.type) == COO_MTYPE_PREDEF) {
		if ((unit = coo_class_block_find(class, id)) == NULL) {
			return -1;	/* attribute not found */
		}
		if (COO_MTYPE_GET(unit->type) == COO_MTYPE_ALLOC) {
			smm_free(unit->d_point);	/* same to string */
		}
		unit->id = 0;
		unit->d_int64 = 0;
		unit->type = COO_DTYPE_NONE;
	} else {
		CSCLNK	*anchor;

		if ((unit = coo_class_link_find(class, id)) == NULL) {
			return -1;	/* attribute not found */
		}
		if (COO_MTYPE_GET(unit->type) == COO_MTYPE_ALLOC) {
			smm_free(unit->d_point);	/* same to string */
		}

		anchor = class->repo.d_point;
		csc_cdl_list_free(&anchor, csc_cdl_paylink(unit));
		class->total--;
	}
	return 0;
}

int coo_class_inherit(CCLASS *class, CCLASS *giver)
{
	int	i;

	if ((class->parents + class->childs >= COO_MAX_INHERIT) ||
			(giver->parents + giver->childs >= COO_MAX_INHERIT)) {
		return -1;	/* FIXME: should be unlimited */
	}
	if (class->childs > 0) {
		return -2;	/* NO inheritance once give child */
	}
	for (i = 0; i < class->parents; i++) {
		if (class->inherit[i] == giver) {
			break;	/* already inherited */
		}
	}
	if (i == class->parents) {
		class->inherit[class->parents] = giver;
		class->parents++;
	}

	for (i = 0; i < giver->childs; i++) {
		if (giver->inherit[giver->parents + i] == class) {
			break;
		}
	}
	if (i == giver->childs) {
		giver->inherit[giver->parents + i] = class;
		giver->childs++;
	}
	return 0;
}

int coo_class_dump(CCLASS *class)
{
	CSCLNK	*node, *anchor;
	MUnit	*unit;
	int	i, n;

	if (COO_MTYPE_GET(class->repo.type) == COO_MTYPE_LINK) {
		CDB_SHOW(("%s: %d attributes  ", class->cname, class->total));
	} else {
		unit = class->repo.d_point;
		for (i = n = 0; i < class->total; i++, unit++) {
			if (unit->type != COO_DTYPE_NONE) {
				n++;
			}
		}
		CDB_SHOW(("%s: %d/%d attributes  ", class->cname, n, class->total));
	}
	coo_class_dump_munit(&class->repo);

	if (COO_MTYPE_GET(class->repo.type) == COO_MTYPE_PREDEF) {
		unit = class->repo.d_point;
		for (i = 0; i < class->total; i++, unit++) {
			if (unit->type != COO_DTYPE_NONE) {
				coo_class_dump_munit(unit);
			}
		}
	} else {
		anchor = class->repo.d_point;
		for (node = anchor; node; node = csc_cdl_next(anchor, node)) {
			unit = (MUnit *) csc_cdl_payload(node);
			coo_class_dump_munit(unit);
		}
	}
	return 0;
}

static int coo_class_write_over(MUnit *unit, int type, void *buf, int len)
{
	char	*tmp;

	/* changing data type of the attribution is not allowed */
	if (COO_DTYPE_GET(type) != COO_DTYPE_GET(unit->type)) {
		return -1;
	}
	switch (COO_DTYPE_GET(type)) {
	case COO_DTYPE_I32:
		unit->d_int = *((int *) buf);
		break;
	case COO_DTYPE_I64:
		unit->d_int64 = *((long long *) buf);
		break;
	case COO_DTYPE_STRING:
		/* allocate memory before changing anything in metaunit */
		if (COO_MTYPE_GET(type) == COO_MTYPE_PREDEF) {
			tmp = buf;
		} else if ((tmp = csc_strcpy_alloc(buf, 0)) == NULL) {
			return -1;
		}

		/* free the previous dynamic memory if existed */
		if (COO_MTYPE_GET(unit->type) == COO_MTYPE_ALLOC) {
			smm_free(unit->d_string);
		}
		unit->type = COO_MTYPE_SET(unit->type, type);
		unit->d_string = tmp;
		break;
	case COO_DTYPE_POINTER:
		/* allocate memory before changing anything in metaunit */
		if (COO_MTYPE_GET(type) == COO_MTYPE_PREDEF) {
			tmp = buf;
		} else if ((tmp = smm_alloc(len)) == NULL) {
			return -1;
		} else {
			memcpy(tmp, buf, len);
		}

		/* free the previous dynamic memory if existed */
		if (COO_MTYPE_GET(unit->type) == COO_MTYPE_ALLOC) {
			smm_free(unit->d_point);
		}
		unit->type = COO_MTYPE_SET(unit->type, type);
		unit->d_point = tmp;
		break;
	case COO_DTYPE_FUNC:
		unit->d_func = *((MFunc *) buf);
		break;
	}
	return 0;
}

static MUnit *coo_class_block_find(CCLASS *class, int id)
{
	MUnit	*unit;
	int	i;

	unit = class->repo.d_point;
	for (i = 0; i < class->total; i++, unit++) {
		if (unit->type == COO_DTYPE_NONE) {
			continue;
		}
		if (unit->id == id) {
			return unit;
		}
	}
	return NULL;
}

static MUnit *coo_class_block_add(CCLASS *class, int id, int type)
{
	MUnit	*unit;
	int	i;

	unit = class->repo.d_point;
	for (i = 0; i < class->total; i++, unit++) {
		if (COO_DTYPE_GET(unit->type) == COO_DTYPE_NONE) {
			unit->type = type;
			unit->id = id;
			return unit;
		}
	}
	return NULL;	/* full in attribution repository */
}

static MUnit *coo_class_link_find(CCLASS *class, int id)
{
	CSCLNK	*node, *anchor;
	MUnit	*unit;

	anchor = class->repo.d_point;
	for (node = anchor; node; node = csc_cdl_next(anchor, node)) {
		unit = (MUnit *) csc_cdl_payload(node);
		if (unit->id == id) {
			return unit;
		}
	}
	return NULL;
}

static MUnit *coo_class_link_add(CCLASS *class, int id, int type, int plsize)
{
	CSCLNK	*node;
	MUnit	*unit;

	plsize += sizeof(MUnit);
	node = csc_cdl_list_alloc_tail((CSCLNK**) &class->repo.d_point, plsize);
	if (node == NULL) {
		return NULL;
	}
	class->total++;

	unit = (MUnit *) csc_cdl_payload(node);
	unit->id = id;
	unit->type = type;
	return unit;
}

static int coo_class_dump_munit(MUnit *unit)
{
	char	buf[256];
	int	i = 0;

	i += sprintf(buf + i, "%8d: ", unit->id);
	switch (COO_MTYPE_GET(unit->type)) {
	case COO_MTYPE_PREDEF:
		i += sprintf(buf + i, "PREDEFINED:");
		break;
	case COO_MTYPE_ALLOC:
		i += sprintf(buf + i, "ALLOCATED :");
		break;
	case  COO_MTYPE_LINK:
		i += sprintf(buf + i, "LINKLISTED:");
		break;
	default:
		i += sprintf(buf + i, "ERROR     :");
		break;
	}
	switch (COO_ACCM_GET(unit->type)) {
	case COO_ACCM_PUBLIC:
		i += sprintf(buf + i, "PUBLIC :");
		break;
	case COO_ACCM_PRIVATE:
		i += sprintf(buf + i, "PRIVATE:");
		break;
	case COO_ACCM_PROTECT:
		i += sprintf(buf + i, "PROTECT:");
		break;
	default:
		i += sprintf(buf + i, "ERROR  :");
		break;
	}
	switch (COO_DTYPE_GET(unit->type)) {
	case COO_DTYPE_I32:
		i += sprintf(buf + i, "INT32:::: 0x%x\n", unit->d_int);
		break;
	case COO_DTYPE_I64:
		i += sprintf(buf + i, "INT64:::: 0x%llx\n", unit->d_int64);
		break;
	case COO_DTYPE_STRING:
		i += snprintf(buf + i, sizeof(buf) - i - 4,
				"STRING::: %s\n", unit->d_string);
		break;
	case COO_DTYPE_POINTER:
		i += sprintf(buf + i, "POINTER:: %p\n", unit->d_point);
		break;
	case COO_DTYPE_FUNC:
		i += sprintf(buf + i, "FUNCTIO:: %p\n", unit->d_func);
		break;
	default:
		i += sprintf(buf + i, "ERROR::::\n");
		break;
	}
	CDB_SHOW((buf));
	return 0;
}


int coo_framework_init(void)
{
	coo_root = coo_class_new("coo_framework", 32);
	return 0;
}

CCLASS *coo_frameowrk_find_class(char *cname)
{
	return NULL;
}

/*
CCLASS *coo_object_new(CCLASS *class)
{
	CCLASS	*object;
	MUnit	*unit;
	int	i;

	object = coo_class_new(class->cname, 32);
	for (i = 0; i < class->parents; i++) {
		object->inherit[i] = coo_object_new(class->inherit[i]);
	}

	if (COO_MTYPE_GET(class->repo.type) == COO_MTYPE_PREDEF) {
		sunit = class->repo.d_point;
		dunit = object->repo.d_point;
		for (i = 0; i < class->total; i++, sunit++, dunit++) {
			coo_object_copy_attribute(dunit, sunit);
		}
	} else {
		anchor = class->repo.d_point;
		dunit = object->repo.d_point;
		for (node = anchor; node; node = csc_cdl_next(anchor, node)) {
			sunit = (MUnit *) csc_cdl_payload(node);
			coo_object_copy_attribute(dunit++, sunit);
		}
	}
}

int coo_object_copy_attribute(MUnit *dest, MUnit *sour)
{
*/

static int coo_test_for_block(void)
{
	CCLASS	*class;
	MUnit	*unit;
	int	i, tmp;
	char	*con_string = "Hello world!";
	char    *con_string2 = "All controls can be created free";
	char	*rbuf;
	int	(*myfunc)(MUnit *unit);


	/* attribution inside the class structure */
	CDB_SHOW(("Create Instance with 8 attributions.\n"));
	class = coo_class_new(NULL, 8);
	CDB_SHOW(("Testing: limitation of attributes ... "));
	for (i = 0; i < 40; i++) {
		tmp = coo_class_write(class, i, COO_DTYPE_I32, &i, 0);
		if (tmp != 0) {
			CDB_SHOW(("%d written\n", i));
			break;
		}
	}
	coo_class_dump(class);
	CDB_SHOW(("Testing: deleting all attributes ... "));
	for (i = 30; i >= 0; i--) {
		tmp = coo_class_delete(class, i);
		if (tmp == 0) {
			CDB_SHOW(("O"));
		} else {
			CDB_SHOW(("X"));
		}
	}
	CDB_SHOW(("\n"));
	coo_class_dump(class);
		
	CDB_SHOW(("Testing: read and write functions ... "));
	coo_class_write(class, Attrib_03, COO_DTYPE_STRING|COO_MTYPE_PREDEF,
			con_string, 0);
	coo_class_write(class, Attrib_04, COO_DTYPE_STRING|COO_MTYPE_ALLOC,
			con_string, 0);
	
	myfunc = coo_class_dump_munit;
	coo_class_write(class, Attrib_05, COO_DTYPE_FUNC, (void*) &myfunc, 0);
	coo_class_dump(class);
	
	coo_class_read(class, Attrib_03, (void*) &rbuf);
	CDB_SHOW(("write and read constant string at %p: %s\n", rbuf, rbuf));
	coo_class_read(class, Attrib_04, (void*) &rbuf);
	CDB_SHOW(("write and read allocated string at %p: %s\n", rbuf, rbuf));
	coo_class_read(class, Attrib_05, (void*) &myfunc);
	CDB_SHOW(("write and read function pointer at %p: ", myfunc));
	(*myfunc)(&class->repo);

	CDB_SHOW(("write to a different type: "));
	coo_class_write(class, Attrib_04, COO_DTYPE_I32, &i, 0);
	unit = coo_class_block_find(class, Attrib_04);
	coo_class_dump_munit(unit);
	CDB_SHOW(("write to the same type:    "));
	coo_class_write(class, Attrib_04, COO_DTYPE_STRING|COO_MTYPE_ALLOC,
			con_string2, 0);
	coo_class_dump_munit(unit);

	coo_class_destroy(class);
	return 0;
}

static int coo_test_for_link(void)
{
	CCLASS	*class;
	int	i;
	char	*con_string = "Hello world!";

	/* attribution stored in standalone link list */
	CDB_SHOW(("Create Instance with dynamic linked attributions\n"));
	class = coo_class_new("linkage", 0);

	for (i = 0; i < 8; i++) {
		coo_class_write(class, i, COO_DTYPE_I32, &i, 0);
	}

	/* delete number 3 and 6 */
	coo_class_delete(class, 3);
	coo_class_delete(class, 6);

	coo_class_write(class, 3, COO_DTYPE_STRING|COO_MTYPE_PREDEF,
			con_string, 0);
	coo_class_write(class, 6, COO_DTYPE_STRING|COO_MTYPE_ALLOC,
			con_string, 0);

	coo_class_dump(class);
	coo_class_destroy(class);
	return 0;
}

static int coo_test_for_instance(void)
{
	CCLASS	*human, *woman, *man, *mother, *father, *son;
	CCLASS	*ison;
	int	i;

	coo_framework_init();

	human = coo_class_new("human", 32);
	coo_class_inherit(human, coo_root);
	coo_class_write(human, 1, COO_DTYPE_STRING|COO_MTYPE_PREDEF, 
			"Weight between 0-200kg", 0);
	coo_class_write(human, 2, COO_DTYPE_STRING|COO_MTYPE_PREDEF,
			 "humanoid body type", 0);
	i = 10;
	coo_class_write(human, 3, COO_DTYPE_I32, &i, 0);

	man = coo_class_new("man", 32);
	coo_class_inherit(man, human);
	
	woman = coo_class_new("woman", 32);
	coo_class_inherit(woman, human);
	coo_class_write(woman, 1, COO_DTYPE_STRING|COO_MTYPE_PREDEF, 
			"Weight between 0-100kg", 0);

	father = coo_class_new("father", 32);
	coo_class_inherit(father, man);
	coo_class_write(father, 1, COO_DTYPE_STRING|COO_MTYPE_PREDEF, 
			"Weight between 50-200kg", 0);

	mother = coo_class_new("mother", 32);
	coo_class_inherit(mother, woman);
	coo_class_write(mother, 1, COO_DTYPE_STRING|COO_MTYPE_PREDEF,
			"Weight between 40-100kg", 0);

	son = coo_class_new("son", 32);
	coo_class_inherit(son, father);
	coo_class_inherit(son, mother);
	coo_class_write(son, 1, COO_DTYPE_STRING|COO_MTYPE_PREDEF,
			"Weight between 0-40kg", 0);

	coo_class_dump(son);
	coo_class_destroy(son);
	return 0;
}

int coo_main(void *rtime, int argc, char **argv)
{
	/* stop the compiler complaining */
	(void) rtime; (void) argc; (void) argv;
	
	coo_test_for_block();
	coo_test_for_link();
	return 0;
}

struct	clicmd	coo_cmd = {
	"coo", coo_main, NULL, "Testing the CSoup Object Oriented Framework"
};

extern  struct  clicmd  coo_cmd;


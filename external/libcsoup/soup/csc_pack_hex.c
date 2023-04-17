
/*  csc_pack_hex.c: a simple way to pack files to C array in hex.

    Copyright (C) 2017  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of CSOUP, Chicken Soup library

    CSOUP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CSOUP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcsoup.h"


/*!\brief Verify if the given memory presents a packed hex array.
 
  The csc_pack_hex_verify() function will verify if there is a valid data
  structure since 'pachex'. If the memory is structured with the valid
  hex array, the function will retrieve the size of the file name, 
  the size of the file content and return a pointer to the file name.

  \param[in]    pachex Point to the memory being verified.
  \param[out]   flen The size of the content data.
  \param[out]   fnsize The size of the file name.

  \return  A pointer to the file name, or NULL if the memory is invalided.

  \remark  The structure of the packed hex array is simply combined by 
           a group of files in hex directly converted from your file system, 
	   for example:
	     simple_file.bin
	     root/kernel
	     home/user1/config.rc
	   Though it looks like a file system, there's no directory structure
	   at all. The 'root/kernel' is just a file name for easy remember.

           Each file in the packed hex array has four parts, the length of 
	   the file contents, the length of the filename, the file name and
	   the contents of the file. The first three parts are all char 
	   strings which end by ASC0. For example:

	     "2300980" + "11" + "root/kernel" + "\11\22\255\0\43\177\..."

	   Therefore it's quite easy to retrieve the address of the contents
	   by a group of strlen() calls and retrieve the length of the contents
	   by strtol() call.

	   The packed hex array will be ended by three ASC0s, "" + "" + "",
	   which means neither the content nor the filename were present.
*/
void *csc_pack_hex_verify(void *pachex, long *flen, long *fnsize)
{
	long	tmpfl, tmpfn;
	char	*endp, *p = pachex;

	tmpfl = strtol(p, &endp, 0);
	if (*endp != 0) {
		return NULL;	/* invalided length of file */
	}
	p = endp + 1;

	tmpfn = strtol(p, &endp, 0);
	if (*endp != 0) {
		return NULL;	/* invalided length of file name */
	}
	p = endp + 1;

	if (tmpfn == 0) {
		return NULL;	/* file name can not be empty */
	}
	if (p[tmpfn] != 0) {
		return NULL;	/* file name must match its size */
	}

	if (flen) {
		*flen = tmpfl;
	}
	if (fnsize) {
		*fnsize = tmpfn;
	}
	return p;
}

/* !\brief find the next file in the packed hex array

   The csc_pack_hex_find_next() function will find the next file from the
   specified memory boundry 'pachex'. If 'pachex' is NULL, it will keep 
   searching from the last location.

   \param[in]   pachex Point to the start address of the packed hex array.
                If 'pachex' is set, the csc_pack_hex_find_next() will verify
		its availability. If 'pachex' is NULL, the function will
		search to the next file frame.

   \return  A pointer to the found file frame, which is the string of the 
            length of the file contents. Otherwise it returns NULL if it's
	    the end of the packed hex array, or the broken of the file frame.
*/
void *csc_pack_hex_find_next(void *pachex)
{
	static	char	*basep;
	long	flen, fnsize;
	char	*p;

	if (pachex) {
		basep = pachex;
	}

	if ((p = csc_pack_hex_verify(basep, &flen, &fnsize)) == NULL) {
		return NULL;
	}

	pachex = basep;
	basep = p + fnsize + 1 + flen;	/* move to next hex frame */
	return pachex;
}

/*!\brief enumerate the files in the packed hex array.
 
  The csc_pack_hex_list() will search the whole packed hex array
  and callback the 'lsfunc' upon each files it found.

  \param[in]   pachex Point to the start address of the packed hex array.
  \param[in]   lsfunc callback function, which has a prototype:
               int lsfunc(void *frame, char *fname, void *data, long dlen);
*/
void csc_pack_hex_list(void *pachex, F_PKHEX lsfunc)
{
	long	flen = 0, fnsize = 0;
	char	*p;

	while ((pachex = csc_pack_hex_find_next(pachex)) != NULL) {
		p = csc_pack_hex_verify(pachex, &flen, &fnsize);
		if (lsfunc) {
			lsfunc(pachex, p, p + fnsize + 1, flen);
		}

		pachex = NULL;	/* move to the next hex frame */
	}
}

/*!\brief find the specified file in the packed hex array.
 
  The csc_pack_hex_load() will try to find the specified file frame by
  the given file name.

  \param[in]   pachex Point to the start address of the packed hex array.
  \param[in]   path The required file name.
  \param[out]  size The length of the file contents.

  \return A pointer to the begin address of the contents if successfully
          found the file, otherwise NULL returned.
*/
void *csc_pack_hex_load(void *pachex, char *path, long *size)
{
	long	fnsize = 0;
	char	*p;

	while ((pachex = csc_pack_hex_find_next(pachex)) != NULL) {
		p = csc_pack_hex_verify(pachex, size, &fnsize);
		if (!strcmp(p, path)) {
			return p + fnsize + 1;
		}

		pachex = NULL;	/* move to the next hex frame */
	}
	return NULL;
}

/*!\brief Build an index array from the packed hex array.

  The csc_pack_hex_index() will generate an index array for all files
  in the packed hex array. The structure of the index array is:
  
  struct  phex_idx        {
          char            *fname;       // a string point to the file name.
	  void            *data;        // the point to the file contents.
	  long            dlen;         // the length of the file contents.
  };

  The index array will end by { NULL, NULL, 0 }.

  \param[in]   pachex Point to the start address of the packed hex array.

  \return A pointer to the begin address of the index array.

  \remark The index array is dynamically allocated so don't forget to free
          them when not using.
*/
void *csc_pack_hex_index(void *pachex)
{
	struct	phex_idx	*idx;
	int	i = 0, acc = 0;

	int pack_hex_counter(void *frame, char *fname, void *data, long dlen)
	{
		(void) frame; (void) fname; (void) data; (void) dlen;
		return acc++;
	}

	int pack_hex_indexing(void *frame, char *fname, void *data, long dlen)
	{
		(void) frame; 
		idx[i].fname = fname;
		idx[i].data = data;
		idx[i].dlen = dlen;
		return i++;
	}

	csc_pack_hex_list(pachex, pack_hex_counter);
	
	idx = (struct phex_idx *) calloc(sizeof(struct phex_idx), acc + 1);
	if (idx == NULL) {
		return NULL;
	}
	
	csc_pack_hex_list(pachex, pack_hex_indexing);
	idx[i].fname = NULL;
	idx[i].data = NULL;
	idx[i].dlen = 0;
	return idx;
}


#ifdef	EXECUTABLE

#define	MAX_PATH_DEPTH	128	

static	char	*pathfinder[MAX_PATH_DEPTH];
static	int	pathindex = 0;
static	long	arraylen = 0;

static int bin2c(char *path, char *fullname)
{
	FILE	*fp;
	long	i, flen;
	char	buf[32];


	if ((fp = fopen(path, "rb")) == NULL) {
		perror(path);
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	flen = ftell(fp);
	rewind(fp);

	printf("\n\t/* %s: %ld (%ld) */\n\n", 
			fullname, flen, (long) strlen(fullname));

	/* output the lenght of the file */
	sprintf(buf, "%ld", flen);
	printf("\t");
	for (i = 0; i <= (long) strlen(buf); i++) {
		printf("%3d,", (unsigned char) buf[i]);
	}
	arraylen += strlen(buf) + 1;
	
	/* output the length of the file name */
	sprintf(buf, "%ld", (long) strlen(fullname));
	for (i = 0; i <= (long) strlen(buf); i++) {
		printf("%3d,", (unsigned char) buf[i]);
	}
	printf("\n");
	arraylen += strlen(buf) + 1;

	/* output the file name */
	printf("\t");
	for (i = 0; i <= (long) strlen(fullname); i++) {
		printf("%3d,", (unsigned char) fullname[i]);
		if ((i % 16 == 15)) {
			printf("\n\t");
		}
	}
	printf("\n");
	arraylen += strlen(fullname) + 1;

	/* output the content of the file */
	while ((flen = (long) fread(buf, 1, 16, fp)) > 0) {
		printf("\t");
		for (i = 0; i < flen; i++) {
			printf("%3d,", (unsigned char) buf[i]);
		}
		printf("\n");
		arraylen += flen;
	}
	fclose(fp);
	return 0;
}

static int hexmaker(void *option, char *path, int type, void *info)
{
	int	i, n;

	(void) option;	/* stop the gcc warning */

	switch (type) {
	case SMM_MSG_PATH_ENTER:
		fprintf(stderr, "Entering %s:\n", path);
		if (pathindex + 1 == MAX_PATH_DEPTH) {	/* oops! overflowed */
			break;
		}
		pathfinder[pathindex++] = csc_strcpy_alloc(path, 0);
		break;

	case SMM_MSG_PATH_EXEC:
		for (i = n = 0; i < pathindex; i++) {
			n += strlen(pathfinder[i]) + 4;
		}
		n += strlen(path);

		if (pathindex == 0) {
			info = csc_strcpy_alloc(path, n);
		} else {
			info = csc_strcpy_alloc(pathfinder[0], n);

			for (i = 1; i < pathindex; i++) {
				strcat((char*) info, "/");
				strcat((char*) info, pathfinder[i]);
			}
			strcat((char*) info, "/");
			strcat((char*) info, path);
		}

		bin2c(path, info);
		//fprintf(stderr, "SMM_MSG_PATH_EXEC: %s\n", info);
		smm_free(info);
		break;

	case SMM_MSG_PATH_BREAK:
		fprintf(stderr, "Failed to process %s\n", path);
		break;

	case SMM_MSG_PATH_LEAVE:
		fprintf(stderr, "Leaving %s\n", path);
		smm_free(pathfinder[--pathindex]);
		break;
	}
	return SMM_NTF_PATH_NONE;
}


static  const   char    *prefix = "\
#ifdef __SUNPRO_C\n\
#pragma align 4 (%s)\n\
#endif\n\
#ifdef __GNUC__\n\
const	unsigned char	%s[] __attribute__ ((__aligned__ (4))) =\n\
#else\n\
const	unsigned char	%s[] =\n\
#endif\n\
{\n\
\t/* the structure of the packed hex array:\n\
\t *  String(file_length) + String(filename_length) + String(filename) + \n\
\t *  Hexdump(file_content) + ... (repeat for all files) + '0', '0', '0' \n\
\t */\n";

int main(int argc, char **argv)
{
	char	*arrayid = NULL;

	while (--argc && **++argv == '-') {
		if (!strcmp(*argv, "-i")) {
			--argc; ++argv;
			arrayid = *argv;
		}
	}
	if (argc < 1) {
		printf("Usage: packhex [-i array_id] path ...\n");
		return -1;
	}
	if (arrayid == NULL) {
		arrayid = *argv;
	}

	printf(prefix, arrayid, arrayid, arrayid);
	while (argc--) {
		smm_pathtrek(*argv++, SMM_PATH_DIR_FIFO, hexmaker, NULL);
	}
	puts("\n\t0,  0,  0\t\t/* End of the hex package */\n");
	printf("};\t/* end of %s[%ld] */\n\n", arrayid, arraylen);	
	return 0;
}

#endif


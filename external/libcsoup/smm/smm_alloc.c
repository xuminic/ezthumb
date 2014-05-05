
/*!\file       smm_alloc.c
   \brief      Wrapper of malloc() and free().

   It would be very useful in the embedded system while malloc() and free()
   are not supplied.

   \author     "Andy Xuming" <xuming@users.sourceforge.net>
   \date       2013-2014
   \copyright  GNU Public License.
   \remark     
*/
/*  Copyright (C) 2011  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of LIBSMM, System Masquerade Module library

    LIBSMM is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LIBSMM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*!\brief      allocate a piece of memory.

   \param[in]  size The required size to allocate    

   \return     A pointer to the allocated memory,  or NULL on error.
   \remark     smm_alloc() will fill the allocated memory with 0
*/
void *smm_alloc(size_t size)
{
	void	*mem;

	if ((mem = malloc(size)) != NULL) {
		memset(mem, 0, size);
	}
	return mem;
}

/*!\brief      free the allocated memory by smm_alloc().

   \param[in]  ptr A pointer to the allocated memory. 

   \retval 0   succeed
*/
int smm_free(void *ptr)
{
	free(ptr);
	return 0;
}



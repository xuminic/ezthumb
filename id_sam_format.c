
/*
    Copyright (C) 2011  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of EZTHUMB, a utility to generate thumbnails

    EZTHUMB is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    EZTHUMB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "id_lookup.h"

struct	idtbl	id_sample_format[] = {
	{ SAMPLE_FMT_NONE, "SAMPLE_FMT_NONE" },
	{ SAMPLE_FMT_U8, "SAMPLE_FMT_U8" },
	{ SAMPLE_FMT_S16, "SAMPLE_FMT_S16" },
	{ SAMPLE_FMT_S32, "SAMPLE_FMT_S32" },
	{ SAMPLE_FMT_FLT, "SAMPLE_FMT_FLT" },
	{ SAMPLE_FMT_DBL, "SAMPLE_FMT_DBL" },
	{ SAMPLE_FMT_NB, "SAMPLE_FMT_NB" },
	{ 0, NULL }
};

struct	idtbl	id_sam_format[] = {
	{ SAMPLE_FMT_NONE, "NONE" },
	{ SAMPLE_FMT_U8, "8-bit" },
	{ SAMPLE_FMT_S16, "16-bit" },
	{ SAMPLE_FMT_S32, "32-bit" },
	{ SAMPLE_FMT_FLT, "Float" },
	{ SAMPLE_FMT_DBL, "Double" },
	{ SAMPLE_FMT_NB, "NB" },
	{ 0, NULL }
};

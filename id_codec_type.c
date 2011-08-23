
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

struct	idtbl	id_codec_type[] = {
	{ CODEC_TYPE_UNKNOWN, "CODEC_TYPE_UNKNOWN" },
	{ CODEC_TYPE_VIDEO, "CODEC_TYPE_VIDEO" },
	{ CODEC_TYPE_AUDIO, "CODEC_TYPE_AUDIO" },
	{ CODEC_TYPE_DATA, "CODEC_TYPE_DATA" },
	{ CODEC_TYPE_SUBTITLE, "CODEC_TYPE_SUBTITLE" },
	{ CODEC_TYPE_ATTACHMENT, "CODEC_TYPE_ATTACHMENT" },
	{ CODEC_TYPE_NB, "CODEC_TYPE_NB" },
	{ 0, NULL }
};



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

#ifndef	_ID_LOOKUP_H_
#define _ID_LOOKUP_H_

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#define ID_LOOKUP_VERSION	1

struct	idtbl	{
	int	id;
	char	*s;
};

extern	struct	idtbl	id_layout[];
extern	struct	idtbl	id_duration[];
extern	struct	idtbl	id_codec[];
extern	struct	idtbl	id_codec_flag[];
extern	struct	idtbl	id_codec_type[];
extern	struct	idtbl	id_pix_fmt[];
extern	struct	idtbl	id_pict_type[];
extern	struct	idtbl	id_sample_format[];
extern	struct	idtbl	id_sam_format[];

char *id_lookup(struct idtbl *table, int id);
char *id_lookup_tail(struct idtbl *table, int id);
int id_lookup_id(struct idtbl *table, char *s);

#endif	/* _ID_LOOKUP_H_ */



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
#ifdef  HAVE_CONFIG_H
#include <config.h>
#else
#error "Run configure first"
#endif

#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#ifdef HAVE_STRING_H
# if !defined STDC_HEADERS && defined HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ezthumb.h"
#include "id_lookup.h"

char *id_lookup(struct idtbl *table, int id)
{
	static	char	def[32];
	int	i;

	for (i = 0; table[i].s; i++) {
		if (id == table[i].id) {
			return table[i].s;
		}
	}
	sprintf(def, "[%d]", id);	/* unknown ID */
	return def;
}

char *id_lookup_tail(struct idtbl *table, int id)
{
	static	char	def[32];
	char	*tail;
	int	i;

	for (i = 0; table[i].s; i++) {
		if (id == table[i].id) {
			if ((tail = strrchr(table[i].s, '_')) == NULL) {
				return table[i].s;
			} else {
				return tail+1;
			}
		}
	}
	sprintf(def, "[%d]", id);	/* unknown ID */
	return def;
}

int id_lookup_id(struct idtbl *table, char *s)
{
	int	i;

	for (i = 0; table[i].s; i++) {
		if (!strcasecmp(s, table[i].s)) {
			return table[i].id;
		}
	}
	return 0;
}

char *lookup_string_idnum(struct idtbl *table, int err, int idnum)
{
	int	i;

	for (i = 0; table[i].s; i++) {
		if (idnum == table[i].id) {
			return table[i].s;
		}
	}
	if ((err >= 0) && (err < i)) {
		return table[err].s;
	}
	return NULL;
}

int lookup_idnum_string(struct idtbl *table, int err, char *s)
{
	int	i;

	for (i = 0; table[i].s; i++) {
		if (!strcasecmp(s, table[i].s)) {
			return table[i].id;
		}
	}
	return err;
}

int lookup_index_string(struct idtbl *table, int err, char *s)
{
	int	i;

	for (i = 0; table[i].s; i++) {
		if (!strcasecmp(s, table[i].s)) {
			return i;
		}
	}
	return err;
}

int lookup_index_idnum(struct idtbl *table, int err, int idnum)
{
	int	i;

	for (i = 0; table[i].s; i++) { 
		if (idnum == table[i].id) {
			return i;
		}
	}
	return err;
}



#if 0
/* help to generate the array of ffmpeg IDs */

#include <ctype.h>
#include <stdio.h>

int main()
{
	char	buf[256], *p, *q;

	while (fgets(buf, sizeof(buf), stdin)) {
		for (p = buf; isspace(*p); p++);	/* skip whitespaces */
		for (q = p+1; !isspace(*q) && *q!=','; q++);
		*q = 0;
		printf("\t{ %s, \"%s\" },\n", p, p);

		/*if (!strncmp(buf, "#define", 7)) {
			for (p = buf + 7; isspace(*p); p++);
			for (q = p+1; !isspace(*q); q++);
			*q = 0;
			printf("\t{ %s, \"%s\" },\n", p, p);
		} else {
			printf("%s", buf);
		}*/
	}
	return 0;
}
#endif

struct	idtbl	id_layout[] = {
	{ EZ_POS_LEFTTOP,      CFG_PIC_POS_LFETTOP },
	{ EZ_POS_LEFTCENTER,   CFG_PIC_POS_LEFTCENTR },
	{ EZ_POS_LEFTBOTTOM,   CFG_PIC_POS_LEFTBOTTOM },
	{ EZ_POS_MIDTOP,       CFG_PIC_POS_MIDTOP },
	{ EZ_POS_MIDCENTER,    CFG_PIC_POS_MIDCENTR },
	{ EZ_POS_MIDBOTTOM,    CFG_PIC_POS_MIDBOTTOM },
	{ EZ_POS_RIGHTTOP,     CFG_PIC_POS_RIGHTTOP },
	{ EZ_POS_RIGHTCENTER,  CFG_PIC_POS_RIGHTCENTR },
	{ EZ_POS_RIGHTBOTTOM,  CFG_PIC_POS_RIGHTBOTTOM },
	{ EZ_POS_TILE,         CFG_PIC_POS_TILES },
	{ EZ_POS_STRETCH,      CFG_PIC_QUA_STRETCH },
	{ EZ_POS_ENLARGE_X,    CFG_PIC_QUA_ENLARGE_WID },
	{ EZ_POS_ENLARGE_Y,    CFG_PIC_QUA_ENLARGE_HEI },
	{ EZ_POS_STRETCH_X,    CFG_PIC_QUA_STRE_WID },
	{ EZ_POS_STRETCH_Y,    CFG_PIC_QUA_STRE_HEI },
	{ 0, NULL }
};

struct	idtbl	id_existed[] = {
	{ EZOP_THUMB_COPY,      CFG_PIC_TEX_APPEND },
	{ EZOP_THUMB_OVERRIDE,  CFG_PIC_TEX_OVERRIDE },
	{ EZOP_THUMB_SKIP,      CFG_PIC_TEX_SKIP },
	{ 0, NULL }
};

struct	idtbl	id_duration_long[] = {
	{ EZOP_DUR_AUTO,  CFG_PIC_AUTO },
	{ EZOP_DUR_HEAD,  CFG_PIC_DFM_HEAD },
	{ EZOP_DUR_FSCAN, CFG_PIC_DFM_SCAN },
	{ EZOP_DUR_QSCAN, CFG_PIC_DFM_FAST },
	{ 0, NULL }
};

struct	idtbl	id_mprocess[] = {
	{ EZOP_PROC_AUTO,    "Auto" },
	{ EZOP_PROC_SKIM,    "Upon Key Frames" },
	{ EZOP_PROC_SCAN,    "Single Pass" },
	{ EZOP_PROC_TWOPASS, "Two Pass" },
	{ EZOP_PROC_SAFE,    "Safe Mode" },
	{ EZOP_PROC_KEYRIP,  "Key Frame Rip" },
	{ 0, NULL }
};



#ifdef	HAVE_AVCODEC_DESCRIPTOR_GET
char *id_lookup_codec(int idnum)
{
	const AVCodecDescriptor	*idcodec;

	if ((idcodec = avcodec_descriptor_get(idnum)) == NULL) {
		return NULL;
	}
	if (idcodec->long_name) {
		return (char*) idcodec->long_name;
	}
	return (char*) idcodec->name;
}
#else	/* no avcodec_descriptor_get() defined */
struct	idtbl	id_codec[] = {
	{ CODEC_ID_NONE, "CODEC_ID_NONE" },

	/* video codecs */
	{ CODEC_ID_MPEG1VIDEO, "CODEC_ID_MPEG1VIDEO" },
	{ CODEC_ID_MPEG2VIDEO, "CODEC_ID_MPEG2VIDEO" },
	{ CODEC_ID_MPEG2VIDEO_XVMC, "CODEC_ID_MPEG2VIDEO_XVMC" },
	{ CODEC_ID_H261, "CODEC_ID_H261" },
	{ CODEC_ID_H263, "CODEC_ID_H263" },
	{ CODEC_ID_RV10, "CODEC_ID_RV10" },
	{ CODEC_ID_RV20, "CODEC_ID_RV20" },
	{ CODEC_ID_MJPEG, "CODEC_ID_MJPEGB" },
	{ CODEC_ID_MJPEGB, "CODEC_ID_MJPEGB" },
	{ CODEC_ID_LJPEG, "CODEC_ID_LJPEG" },
	{ CODEC_ID_SP5X, "CODEC_ID_SP5X" },
	{ CODEC_ID_JPEGLS, "CODEC_ID_JPEGLS" },
	{ CODEC_ID_MPEG4, "CODEC_ID_MPEG4" },
	{ CODEC_ID_RAWVIDEO, "CODEC_ID_RAWVIDEO" },
	{ CODEC_ID_MSMPEG4V1, "CODEC_ID_MSMPEG4V1" },
	{ CODEC_ID_MSMPEG4V2, "CODEC_ID_MSMPEG4V2" },
	{ CODEC_ID_MSMPEG4V3, "CODEC_ID_MSMPEG4V3" },
	{ CODEC_ID_WMV1, "CODEC_ID_WMV1" },
	{ CODEC_ID_WMV2, "CODEC_ID_WMV2" },
	{ CODEC_ID_H263P, "CODEC_ID_H263P" },
	{ CODEC_ID_H263I, "CODEC_ID_H263I" },
	{ CODEC_ID_FLV1, "CODEC_ID_FLV1" },
	{ CODEC_ID_SVQ1, "CODEC_ID_SVQ1" },
	{ CODEC_ID_SVQ3, "CODEC_ID_SVQ3" },
	{ CODEC_ID_DVVIDEO, "CODEC_ID_DVVIDEO" },
	{ CODEC_ID_HUFFYUV, "CODEC_ID_HUFFYUV" },
	{ CODEC_ID_CYUV, "CODEC_ID_CYUV" },
	{ CODEC_ID_H264, "CODEC_ID_H264" },
	{ CODEC_ID_INDEO3, "CODEC_ID_INDEO3" },
	{ CODEC_ID_VP3, "CODEC_ID_VP3" },
	{ CODEC_ID_THEORA, "CODEC_ID_THEORA" },
	{ CODEC_ID_ASV1, "CODEC_ID_ASV1" },
	{ CODEC_ID_ASV2, "CODEC_ID_ASV2" },
	{ CODEC_ID_FFV1, "CODEC_ID_FFV1" },
	{ CODEC_ID_4XM, "CODEC_ID_4XM" },
	{ CODEC_ID_VCR1, "CODEC_ID_VCR1" },
	{ CODEC_ID_CLJR, "CODEC_ID_CLJR" },
	{ CODEC_ID_MDEC, "CODEC_ID_MDEC" },
	{ CODEC_ID_ROQ, "CODEC_ID_ROQ" },
	{ CODEC_ID_INTERPLAY_VIDEO, "CODEC_ID_INTERPLAY_VIDEO" },
	{ CODEC_ID_XAN_WC3, "CODEC_ID_XAN_WC3" },
	{ CODEC_ID_XAN_WC4, "CODEC_ID_XAN_WC4" },
	{ CODEC_ID_RPZA, "CODEC_ID_RPZA" },
	{ CODEC_ID_CINEPAK, "CODEC_ID_CINEPAK" },
	{ CODEC_ID_WS_VQA, "CODEC_ID_WS_VQA" },
	{ CODEC_ID_MSRLE, "CODEC_ID_MSRLE" },
	{ CODEC_ID_MSVIDEO1, "CODEC_ID_MSVIDEO1" },
	{ CODEC_ID_IDCIN, "CODEC_ID_IDCIN" },
	{ CODEC_ID_8BPS, "CODEC_ID_8BPS" },
	{ CODEC_ID_SMC, "CODEC_ID_SMC" },
	{ CODEC_ID_FLIC, "CODEC_ID_FLIC" },
	{ CODEC_ID_TRUEMOTION1, "CODEC_ID_TRUEMOTION1" },
	{ CODEC_ID_VMDVIDEO, "CODEC_ID_VMDVIDEO" },
	{ CODEC_ID_MSZH, "CODEC_ID_MSZH" },
	{ CODEC_ID_ZLIB, "CODEC_ID_ZLIB" },
	{ CODEC_ID_QTRLE, "CODEC_ID_QTRLE" },
	{ CODEC_ID_SNOW, "CODEC_ID_SNOW" },
	{ CODEC_ID_TSCC, "CODEC_ID_TSCC" },
	{ CODEC_ID_ULTI, "CODEC_ID_ULTI" },
	{ CODEC_ID_QDRAW, "CODEC_ID_QDRAW" },
	{ CODEC_ID_VIXL, "CODEC_ID_VIXL" },
	{ CODEC_ID_QPEG, "CODEC_ID_QPEG" },
	{ LIBAVCODEC_VERSION_MAJOR, "LIBAVCODEC_VERSION_MAJOR" },
#if	(LIBAVCODEC_VERSION_INT < AV_VERSION_INT(53, 0, 0))
	{ CODEC_ID_XVID, "CODEC_ID_XVID" },
#endif
	{ CODEC_ID_PNG, "CODEC_ID_PNG" },
	{ CODEC_ID_PPM, "CODEC_ID_PPM" },
	{ CODEC_ID_PBM, "CODEC_ID_PBM" },
	{ CODEC_ID_PGM, "CODEC_ID_PGM" },
	{ CODEC_ID_PGMYUV, "CODEC_ID_PGMYUV" },
	{ CODEC_ID_PAM, "CODEC_ID_PAM" },
	{ CODEC_ID_FFVHUFF, "CODEC_ID_FFVHUFF" },
	{ CODEC_ID_RV30, "CODEC_ID_RV30" },
	{ CODEC_ID_RV40, "CODEC_ID_RV40" },
	{ CODEC_ID_VC1, "CODEC_ID_VC1" },
	{ CODEC_ID_WMV3, "CODEC_ID_WMV3" },
	{ CODEC_ID_LOCO, "CODEC_ID_LOCO" },
	{ CODEC_ID_WNV1, "CODEC_ID_WNV1" },
	{ CODEC_ID_AASC, "CODEC_ID_AASC" },
	{ CODEC_ID_INDEO2, "CODEC_ID_INDEO2" },
	{ CODEC_ID_FRAPS, "CODEC_ID_FRAPS" },
	{ CODEC_ID_TRUEMOTION2, "CODEC_ID_TRUEMOTION2" },
	{ CODEC_ID_BMP, "CODEC_ID_BMP" },
	{ CODEC_ID_CSCD, "CODEC_ID_CSCD" },
	{ CODEC_ID_MMVIDEO, "CODEC_ID_MMVIDEO" },
	{ CODEC_ID_ZMBV, "CODEC_ID_ZMBV" },
	{ CODEC_ID_AVS, "CODEC_ID_AVS" },
	{ CODEC_ID_SMACKVIDEO, "CODEC_ID_SMACKVIDEO" },
	{ CODEC_ID_NUV, "CODEC_ID_NUV" },
	{ CODEC_ID_KMVC, "CODEC_ID_KMVC" },
	{ CODEC_ID_FLASHSV, "CODEC_ID_FLASHSV" },
	{ CODEC_ID_CAVS, "CODEC_ID_CAVS" },
	{ CODEC_ID_JPEG2000, "CODEC_ID_JPEG2000" },
	{ CODEC_ID_VMNC, "CODEC_ID_VMNC" },
	{ CODEC_ID_VP5, "CODEC_ID_VP5" },
	{ CODEC_ID_VP6, "CODEC_ID_VP6" },
	{ CODEC_ID_VP6F, "CODEC_ID_VP6F" },
	{ CODEC_ID_TARGA, "CODEC_ID_TARGA" },
	{ CODEC_ID_DSICINVIDEO, "CODEC_ID_DSICINVIDEO" },
	{ CODEC_ID_TIERTEXSEQVIDEO, "CODEC_ID_TIERTEXSEQVIDEO" },
	{ CODEC_ID_TIFF, "CODEC_ID_TIFF" },
	{ CODEC_ID_GIF, "CODEC_ID_GIF" },
#if	LIBAVCODEC_VERSION_MAJOR <= 53
	{ CODEC_ID_FFH264, "CODEC_ID_FFH264" },
#endif
	{ CODEC_ID_DXA, "CODEC_ID_DXA" },
	{ CODEC_ID_DNXHD, "CODEC_ID_DNXHD" },
	{ CODEC_ID_THP, "CODEC_ID_THP" },
	{ CODEC_ID_SGI, "CODEC_ID_SGI" },
	{ CODEC_ID_C93, "CODEC_ID_C93" },
	{ CODEC_ID_BETHSOFTVID, "CODEC_ID_BETHSOFTVID" },
	{ CODEC_ID_PTX, "CODEC_ID_PTX" },
	{ CODEC_ID_TXD, "CODEC_ID_TXD" },
	{ CODEC_ID_VP6A, "CODEC_ID_VP6A" },
	{ CODEC_ID_AMV, "CODEC_ID_AMV" },
	{ CODEC_ID_VB, "CODEC_ID_VB" },
	{ CODEC_ID_PCX, "CODEC_ID_PCX" },
	{ CODEC_ID_SUNRAST, "CODEC_ID_SUNRAST" },
	{ CODEC_ID_INDEO4, "CODEC_ID_INDEO4" },
	{ CODEC_ID_INDEO5, "CODEC_ID_INDEO5" },
	{ CODEC_ID_MIMIC, "CODEC_ID_MIMIC" },
	{ CODEC_ID_RL2, "CODEC_ID_RL2" },
	{ CODEC_ID_8SVX_EXP, "CODEC_ID_8SVX_EXP" },
	{ CODEC_ID_8SVX_FIB, "CODEC_ID_8SVX_FIB" },
	{ CODEC_ID_ESCAPE124, "CODEC_ID_ESCAPE124" },
	{ CODEC_ID_DIRAC, "CODEC_ID_DIRAC" },
	{ CODEC_ID_BFI, "CODEC_ID_BFI" },
	{ CODEC_ID_CMV, "CODEC_ID_CMV" },
	{ CODEC_ID_MOTIONPIXELS, "CODEC_ID_MOTIONPIXELS" },
	{ CODEC_ID_TGV, "CODEC_ID_TGV" },
	{ CODEC_ID_TGQ, "CODEC_ID_TGQ" },
	{ CODEC_ID_TQI, "CODEC_ID_TQI" },
	{ CODEC_ID_AURA, "CODEC_ID_AURA" },
	{ CODEC_ID_AURA2, "CODEC_ID_AURA2" },
	{ CODEC_ID_V210X, "CODEC_ID_V210X" },
	{ CODEC_ID_TMV, "CODEC_ID_TMV" },
	{ CODEC_ID_V210, "CODEC_ID_V210" },
	{ CODEC_ID_DPX, "CODEC_ID_DPX" },
	{ CODEC_ID_MAD, "CODEC_ID_MAD" },
	{ CODEC_ID_FRWU, "CODEC_ID_FRWU" },
	{ CODEC_ID_FLASHSV2, "CODEC_ID_FLASHSV2" },
	{ CODEC_ID_CDGRAPHICS, "CODEC_ID_CDGRAPHICS" },
	{ CODEC_ID_R210, "CODEC_ID_R210" },
	{ CODEC_ID_ANM, "CODEC_ID_ANM" },
	{ CODEC_ID_BINKVIDEO, "CODEC_ID_BINKVIDEO" },
	{ CODEC_ID_IFF_ILBM, "CODEC_ID_IFF_ILBM" },
	{ CODEC_ID_IFF_BYTERUN1, "CODEC_ID_IFF_BYTERUN1" },
	{ CODEC_ID_KGV1, "CODEC_ID_KGV1" },
	{ CODEC_ID_YOP, "CODEC_ID_YOP" },
	{ CODEC_ID_VP8, "CODEC_ID_VP8" },
	{ CODEC_ID_VP8, "CODEC_ID_VP8" },
	/* various PCM "codecs" */ 
	{ CODEC_ID_PCM_S16LE, "CODEC_ID_PCM_S16LE" },
	{ CODEC_ID_PCM_S16BE, "CODEC_ID_PCM_S16BE" },
	{ CODEC_ID_PCM_U16LE, "CODEC_ID_PCM_U16LE" },
	{ CODEC_ID_PCM_U16BE, "CODEC_ID_PCM_U16BE" },
	{ CODEC_ID_PCM_S8, "CODEC_ID_PCM_S8" },
	{ CODEC_ID_PCM_U8, "CODEC_ID_PCM_U8" },
	{ CODEC_ID_PCM_MULAW, "CODEC_ID_PCM_MULAW" },
	{ CODEC_ID_PCM_ALAW, "CODEC_ID_PCM_ALAW" },
	{ CODEC_ID_PCM_S32LE, "CODEC_ID_PCM_S32LE" },
	{ CODEC_ID_PCM_S32BE, "CODEC_ID_PCM_S32BE" },
	{ CODEC_ID_PCM_U32LE, "CODEC_ID_PCM_U32LE" },
	{ CODEC_ID_PCM_U32BE, "CODEC_ID_PCM_U32BE" },
	{ CODEC_ID_PCM_S24LE, "CODEC_ID_PCM_S24LE" },
	{ CODEC_ID_PCM_S24BE, "CODEC_ID_PCM_S24BE" },
	{ CODEC_ID_PCM_U24LE, "CODEC_ID_PCM_U24LE" },
	{ CODEC_ID_PCM_U24BE, "CODEC_ID_PCM_U24BE" },
	{ CODEC_ID_PCM_S24DAUD, "CODEC_ID_PCM_S24DAUD" },
	{ CODEC_ID_PCM_ZORK, "CODEC_ID_PCM_ZORK" },
	{ CODEC_ID_PCM_S16LE_PLANAR, "CODEC_ID_PCM_S16LE_PLANAR" },
	{ CODEC_ID_PCM_DVD, "CODEC_ID_PCM_DVD" },
	{ CODEC_ID_PCM_F32BE, "CODEC_ID_PCM_F32BE" },
	{ CODEC_ID_PCM_F32LE, "CODEC_ID_PCM_F32LE" },
	{ CODEC_ID_PCM_F64BE, "CODEC_ID_PCM_F64BE" },
	{ CODEC_ID_PCM_F64LE, "CODEC_ID_PCM_F64LE" },
	{ CODEC_ID_PCM_BLURAY, "CODEC_ID_PCM_BLURAY" },
	{ CODEC_ID_PCM_BLURAY, "CODEC_ID_PCM_BLURAY" },
	/* various ADPCM codecs */
	{ CODEC_ID_ADPCM_IMA_QT, "CODEC_ID_ADPCM_IMA_QT" },
	{ CODEC_ID_ADPCM_IMA_WAV, "CODEC_ID_ADPCM_IMA_WAV" },
	{ CODEC_ID_ADPCM_IMA_DK3, "CODEC_ID_ADPCM_IMA_DK3" },
	{ CODEC_ID_ADPCM_IMA_DK4, "CODEC_ID_ADPCM_IMA_DK4" },
	{ CODEC_ID_ADPCM_IMA_WS, "CODEC_ID_ADPCM_IMA_WS" },
	{ CODEC_ID_ADPCM_IMA_SMJPEG, "CODEC_ID_ADPCM_IMA_SMJPEG" },
	{ CODEC_ID_ADPCM_MS, "CODEC_ID_ADPCM_MS" },
	{ CODEC_ID_ADPCM_4XM, "CODEC_ID_ADPCM_4XM" },
	{ CODEC_ID_ADPCM_XA, "CODEC_ID_ADPCM_XA" },
	{ CODEC_ID_ADPCM_ADX, "CODEC_ID_ADPCM_ADX" },
	{ CODEC_ID_ADPCM_EA, "CODEC_ID_ADPCM_EA" },
	{ CODEC_ID_ADPCM_G726, "CODEC_ID_ADPCM_G726" },
	{ CODEC_ID_ADPCM_CT, "CODEC_ID_ADPCM_CT" },
	{ CODEC_ID_ADPCM_SWF, "CODEC_ID_ADPCM_SWF" },
	{ CODEC_ID_ADPCM_YAMAHA, "CODEC_ID_ADPCM_YAMAHA" },
	{ CODEC_ID_ADPCM_SBPRO_4, "CODEC_ID_ADPCM_SBPRO_4" },
	{ CODEC_ID_ADPCM_SBPRO_3, "CODEC_ID_ADPCM_SBPRO_3" },
	{ CODEC_ID_ADPCM_SBPRO_2, "CODEC_ID_ADPCM_SBPRO_2" },
	{ CODEC_ID_ADPCM_THP, "CODEC_ID_ADPCM_THP" },
	{ CODEC_ID_ADPCM_IMA_AMV, "CODEC_ID_ADPCM_IMA_AMV" },
	{ CODEC_ID_ADPCM_EA_R1, "CODEC_ID_ADPCM_EA_R1" },
	{ CODEC_ID_ADPCM_EA_R3, "CODEC_ID_ADPCM_EA_R3" },
	{ CODEC_ID_ADPCM_EA_R2, "CODEC_ID_ADPCM_EA_R2" },
	{ CODEC_ID_ADPCM_IMA_EA_SEAD, "CODEC_ID_ADPCM_IMA_EA_SEAD" },
	{ CODEC_ID_ADPCM_IMA_EA_EACS, "CODEC_ID_ADPCM_IMA_EA_EACS" },
	{ CODEC_ID_ADPCM_EA_XAS, "CODEC_ID_ADPCM_EA_XAS" },
	{ CODEC_ID_ADPCM_EA_MAXIS_XA, "CODEC_ID_ADPCM_EA_MAXIS_XA" },
	{ CODEC_ID_ADPCM_IMA_ISS, "CODEC_ID_ADPCM_IMA_ISS" },
	{ CODEC_ID_ADPCM_IMA_ISS, "CODEC_ID_ADPCM_IMA_ISS" },
	/* AMR */
	{ CODEC_ID_AMR_NB, "CODEC_ID_AMR_NB" },
	{ CODEC_ID_AMR_WB, "CODEC_ID_AMR_WB" },
	{ CODEC_ID_AMR_WB, "CODEC_ID_AMR_WB" },
	/* RealAudio codecs*/
	{ CODEC_ID_RA_144, "CODEC_ID_RA_144" },
	{ CODEC_ID_RA_288, "CODEC_ID_RA_288" },
	{ CODEC_ID_RA_288, "CODEC_ID_RA_288" },
	/* various DPCM codecs */
	{ CODEC_ID_ROQ_DPCM, "CODEC_ID_ROQ_DPCM" },
	{ CODEC_ID_INTERPLAY_DPCM, "CODEC_ID_INTERPLAY_DPCM" },
	{ CODEC_ID_XAN_DPCM, "CODEC_ID_XAN_DPCM" },
	{ CODEC_ID_SOL_DPCM, "CODEC_ID_SOL_DPCM" },
	{ CODEC_ID_SOL_DPCM, "CODEC_ID_SOL_DPCM" },
	/* audio codecs */
	{ CODEC_ID_MP2, "CODEC_ID_MP2" },
	{ CODEC_ID_MP3, "CODEC_ID_MP3" },	/* preferred ID for decoding MPEG audio layer 1, 2 or 3 */
	{ CODEC_ID_AAC, "CODEC_ID_AAC" },
	{ CODEC_ID_AC3, "CODEC_ID_AC3" },
	{ CODEC_ID_DTS, "CODEC_ID_DTS" },
	{ CODEC_ID_VORBIS, "CODEC_ID_VORBIS" },
	{ CODEC_ID_DVAUDIO, "CODEC_ID_DVAUDIO" },
	{ CODEC_ID_WMAV1, "CODEC_ID_WMAV1" },
	{ CODEC_ID_WMAV2, "CODEC_ID_WMAV2" },
	{ CODEC_ID_MACE3, "CODEC_ID_MACE3" },
	{ CODEC_ID_MACE6, "CODEC_ID_MACE6" },
	{ CODEC_ID_VMDAUDIO, "CODEC_ID_VMDAUDIO" },
	{ CODEC_ID_SONIC, "CODEC_ID_SONIC" },
	{ CODEC_ID_SONIC_LS, "CODEC_ID_SONIC_LS" },
	{ CODEC_ID_FLAC, "CODEC_ID_FLAC" },
	{ CODEC_ID_MP3ADU, "CODEC_ID_MP3ADU" },
	{ CODEC_ID_MP3ON4, "CODEC_ID_MP3ON4" },
	{ CODEC_ID_SHORTEN, "CODEC_ID_SHORTEN" },
	{ CODEC_ID_ALAC, "CODEC_ID_ALAC" },
	{ CODEC_ID_WESTWOOD_SND1, "CODEC_ID_WESTWOOD_SND1" },
	{ CODEC_ID_GSM, "CODEC_ID_GSM" },	/* as in Berlin toast format */
	{ CODEC_ID_QDM2, "CODEC_ID_QDM2" },
	{ CODEC_ID_COOK, "CODEC_ID_COOK" },
	{ CODEC_ID_TRUESPEECH, "CODEC_ID_TRUESPEECH" },
	{ CODEC_ID_TTA, "CODEC_ID_TTA" },
	{ CODEC_ID_SMACKAUDIO, "CODEC_ID_SMACKAUDIO" },
	{ CODEC_ID_QCELP, "CODEC_ID_QCELP" },
	{ CODEC_ID_WAVPACK, "CODEC_ID_WAVPACK" },
	{ CODEC_ID_DSICINAUDIO, "CODEC_ID_DSICINAUDIO" },
	{ CODEC_ID_IMC, "CODEC_ID_IMC" },
	{ CODEC_ID_MUSEPACK7, "CODEC_ID_MUSEPACK7" },
	{ CODEC_ID_MLP, "CODEC_ID_MLP" },
	{ CODEC_ID_GSM_MS, "CODEC_ID_GSM_MS" },	/* as found in WAV */
	{ CODEC_ID_ATRAC3, "CODEC_ID_ATRAC3" },
	{ CODEC_ID_VOXWARE, "CODEC_ID_VOXWARE" },
	{ CODEC_ID_APE, "CODEC_ID_APE" },
	{ CODEC_ID_NELLYMOSER, "CODEC_ID_NELLYMOSER" },
	{ CODEC_ID_MUSEPACK8, "CODEC_ID_MUSEPACK8" },
	{ CODEC_ID_SPEEX, "CODEC_ID_SPEEX" },
	{ CODEC_ID_WMAVOICE, "CODEC_ID_WMAVOICE" },
	{ CODEC_ID_WMAPRO, "CODEC_ID_WMAPRO" },
	{ CODEC_ID_WMALOSSLESS, "CODEC_ID_WMALOSSLESS" },
	{ CODEC_ID_ATRAC3P, "CODEC_ID_ATRAC3P" },
	{ CODEC_ID_EAC3, "CODEC_ID_EAC3" },
	{ CODEC_ID_SIPR, "CODEC_ID_SIPR" },
	{ CODEC_ID_MP1, "CODEC_ID_MP1" },
	{ CODEC_ID_TWINVQ, "CODEC_ID_TWINVQ" },
	{ CODEC_ID_TRUEHD, "CODEC_ID_TRUEHD" },
	{ CODEC_ID_MP4ALS, "CODEC_ID_MP4ALS" },
	{ CODEC_ID_ATRAC1, "CODEC_ID_ATRAC1" },
	{ CODEC_ID_BINKAUDIO_RDFT, "CODEC_ID_BINKAUDIO_RDFT" },
	{ CODEC_ID_BINKAUDIO_DCT, "CODEC_ID_BINKAUDIO_DCT" },
	{ CODEC_ID_BINKAUDIO_DCT, "CODEC_ID_BINKAUDIO_DCT" },
	/* subtitle codecs */
	{ CODEC_ID_DVD_SUBTITLE, "CODEC_ID_DVD_SUBTITLE" },
	{ CODEC_ID_DVB_SUBTITLE, "CODEC_ID_DVB_SUBTITLE" },
	{ CODEC_ID_TEXT, "CODEC_ID_TEXT " },	/* raw UTF-8 text */
	{ CODEC_ID_XSUB, "CODEC_ID_XSUB" },
	{ CODEC_ID_SSA, "CODEC_ID_SSA" },
	{ CODEC_ID_MOV_TEXT, "CODEC_ID_MOV_TEXT" },
	{ CODEC_ID_HDMV_PGS_SUBTITLE, "CODEC_ID_HDMV_PGS_SUBTITLE" },
	{ CODEC_ID_DVB_TELETEXT, "CODEC_ID_DVB_TELETEXT" },
	{ CODEC_ID_DVB_TELETEXT, "CODEC_ID_DVB_TELETEXT" },
	/* other specific kind of codecs (generally used for attachments) */
	{ CODEC_ID_TTF, "CODEC_ID_TTF" },
	{ CODEC_ID_PROBE, "CODEC_ID_PROBE" },
	{ CODEC_ID_MPEG2TS, "CODEC_ID_MPEG2TS" }, /* _FAKE_ codec to indicate a raw MPEG-2 TS */
	{ 0, NULL }
};

char *id_lookup_codec(int idnum)
{
	return id_lookup(id_codec, idnum);
}
#endif	/* HAVE_AVCODEC_DESCRIPTOR_GET */



#ifdef	HAVE_AV_GET_MEDIA_TYPE_STRING
char *id_lookup_codec_type(int idnum)
{
	return (char*) av_get_media_type_string(idnum);
}
#else	/* no av_get_media_type_string() defined */
/* Note that CODEC_TYPE_* are macros but AVMEDIA_TYPE_* are enums */
#ifdef	CODEC_TYPE_UNKNOWN
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
#else
struct	idtbl	id_codec_type[] = {
	{ AVMEDIA_TYPE_UNKNOWN, "AVMEDIA_TYPE_UNKNOWN" },
	{ AVMEDIA_TYPE_VIDEO, "AVMEDIA_TYPE_VIDEO" },
	{ AVMEDIA_TYPE_AUDIO, "AVMEDIA_TYPE_AUDIO" },
	{ AVMEDIA_TYPE_DATA, "AVMEDIA_TYPE_DATA" },
	{ AVMEDIA_TYPE_SUBTITLE, "AVMEDIA_TYPE_SUBTITLE" },
	{ AVMEDIA_TYPE_ATTACHMENT, "AVMEDIA_TYPE_ATTACHMENT" },
	{ AVMEDIA_TYPE_NB, "AVMEDIA_TYPE_NB" },
	{ 0, NULL }
};
#endif	/* CODEC_TYPE_UNKNOWN */

char *id_lookup_codec_type(int idnum)
{
	return id_lookup(id_codec_type, idnum);
}
#endif	/* HAVE_AV_GET_MEDIA_TYPE_STRING */


#ifdef  HAVE_AV_GET_PICTURE_TYPE_CHAR
struct	idtbl	id_pict_type[] = {
	{ '?', "TYPE_NONE" },
	{ 'I', "TYPE_I" },	///< Intra
	{ 'P', "TYPE_P" },	///< Predicted
	{ 'B', "TYPE_B" },	///< Bi-dir predicted
	{ 'S', "TYPE_S" },	///< S(GMC)-VOP MPEG4
	{ 'i', "TYPE_SI" },	///< Switching Intra
	{ 'p', "TYPE_SP" },	///< Switching Predicted
	{ 'b', "TYPE_BI" },	///< BI type
	{ 0, NULL }
};

char *id_lookup_pict_type(int idnum)
{
	return id_lookup(id_pict_type, 
			av_get_picture_type_char(idnum));
}
#else	/* no av_get_picture_type_char() defined */
/* Note that FF_I_* are macros but AV_PICTURE_TYPE_* are enums */
#ifdef	FF_I_TYPE
struct	idtbl	id_pict_type[] = {
	{ FF_I_TYPE, "FF_I_TYPE" },
	{ FF_P_TYPE, "FF_P_TYPE" },
	{ FF_B_TYPE, "FF_B_TYPE" },
	{ FF_S_TYPE, "FF_S_TYPE" },
	{ FF_SI_TYPE, "FF_SI_TYPE" }, 
	{ FF_SP_TYPE, "FF_SP_TYPE" },
	{ FF_BI_TYPE, "FF_BI_TYPE" },
	{ 0, NULL }
};
#else
struct	idtbl	id_pict_type[] = {
	{ AV_PICTURE_TYPE_NONE, "AV_PICTURE_TYPE_NONE" },
	{ AV_PICTURE_TYPE_I, "AV_PICTURE_TYPE_I" },	///< Intra
	{ AV_PICTURE_TYPE_P, "AV_PICTURE_TYPE_P" },	///< Predicted
	{ AV_PICTURE_TYPE_B, "AV_PICTURE_TYPE_B" },	///< Bi-dir predicted
	{ AV_PICTURE_TYPE_S, "AV_PICTURE_TYPE_S" },	///< S(GMC)-VOP MPEG4
	{ AV_PICTURE_TYPE_SI, "AV_PICTURE_TYPE_SI" },	///< Switching Intra
	{ AV_PICTURE_TYPE_SP, "AV_PICTURE_TYPE_SP" },	///< Switching Predicted
	{ AV_PICTURE_TYPE_BI, "AV_PICTURE_TYPE_BI" },	///< BI type
	{ 0, NULL }
};
#endif	/* FF_I_TYPE */

char *id_lookup_pict_type(int idnum)
{
	return id_lookup(id_pict_type, idnum);
}
#endif /* HAVE_AV_GET_PICTURE_TYPE_CHAR */


#ifdef	HAVE_AV_GET_PIX_FMT_NAME
#include <libavutil/pixdesc.h>

char *id_lookup_pix_fmt(int idnum)
{
	return (char*) av_get_pix_fmt_name(idnum);
}
#else	/* no av_get_pix_fmt_name() defined */
struct	idtbl	id_pix_fmt[] = {
	{ PIX_FMT_NONE, "PIX_FMT_NONE" },
	{ PIX_FMT_YUV420P, "PIX_FMT_YUV420P" },
	{ PIX_FMT_YUYV422, "PIX_FMT_YUYV422" },
	{ PIX_FMT_RGB24, "PIX_FMT_RGB24" },
	{ PIX_FMT_BGR24, "PIX_FMT_BGR24" },
	{ PIX_FMT_YUV422P, "PIX_FMT_YUV422P" },
	{ PIX_FMT_YUV444P, "PIX_FMT_YUV444P" },
	{ PIX_FMT_YUV410P, "PIX_FMT_YUV410P" },
	{ PIX_FMT_YUV411P, "PIX_FMT_YUV411P" },
	{ PIX_FMT_GRAY8, "PIX_FMT_GRAY8" },
	{ PIX_FMT_MONOWHITE, "PIX_FMT_MONOWHITE" },
	{ PIX_FMT_MONOBLACK, "PIX_FMT_MONOBLACK" },
	{ PIX_FMT_PAL8, "PIX_FMT_PAL8" },
	{ PIX_FMT_YUVJ420P, "PIX_FMT_YUVJ420P" },
	{ PIX_FMT_YUVJ422P, "PIX_FMT_YUVJ422P" },
	{ PIX_FMT_YUVJ444P, "PIX_FMT_YUVJ444P" },
	{ PIX_FMT_XVMC_MPEG2_MC, "PIX_FMT_XVMC_MPEG2_MC" },
	{ PIX_FMT_XVMC_MPEG2_IDCT, "PIX_FMT_XVMC_MPEG2_IDCT" },
	{ PIX_FMT_UYVY422, "PIX_FMT_UYVY422" },
	{ PIX_FMT_UYYVYY411, "PIX_FMT_UYYVYY411" },
	{ PIX_FMT_BGR8, "PIX_FMT_BGR8" },
	{ PIX_FMT_BGR4, "PIX_FMT_BGR4" },
	{ PIX_FMT_BGR4_BYTE, "PIX_FMT_BGR4_BYTE" },
	{ PIX_FMT_RGB8, "PIX_FMT_RGB8" },
	{ PIX_FMT_RGB4, "PIX_FMT_RGB4" },
	{ PIX_FMT_RGB4_BYTE, "PIX_FMT_RGB4_BYTE" },
	{ PIX_FMT_NV12, "PIX_FMT_NV12" },
	{ PIX_FMT_NV21, "PIX_FMT_NV21" },
	{ PIX_FMT_ARGB, "PIX_FMT_ARGB" },
	{ PIX_FMT_RGBA, "PIX_FMT_RGBA" },
	{ PIX_FMT_ABGR, "PIX_FMT_ABGR" },
	{ PIX_FMT_BGRA, "PIX_FMT_BGRA" },
	{ PIX_FMT_GRAY16BE, "PIX_FMT_GRAY16BE" },
	{ PIX_FMT_GRAY16LE, "PIX_FMT_GRAY16LE" },
	{ PIX_FMT_YUV440P, "PIX_FMT_YUV440P" },
	{ PIX_FMT_YUVJ440P, "PIX_FMT_YUVJ440P" },
	{ PIX_FMT_YUVA420P, "PIX_FMT_YUVA420P" },
	{ PIX_FMT_VDPAU_H264, "PIX_FMT_VDPAU_H264" },
	{ PIX_FMT_VDPAU_MPEG1, "PIX_FMT_VDPAU_MPEG1" },
	{ PIX_FMT_VDPAU_MPEG2, "PIX_FMT_VDPAU_MPEG2" },
	{ PIX_FMT_VDPAU_WMV3, "PIX_FMT_VDPAU_WMV3" },
	{ PIX_FMT_VDPAU_VC1, "PIX_FMT_VDPAU_VC1" },
	{ PIX_FMT_RGB48BE, "PIX_FMT_RGB48BE" },
	{ PIX_FMT_RGB48LE, "PIX_FMT_RGB48LE" },
	{ PIX_FMT_RGB565BE, "PIX_FMT_RGB565BE" },
	{ PIX_FMT_RGB565LE, "PIX_FMT_RGB565LE" },
	{ PIX_FMT_RGB555BE, "PIX_FMT_RGB555BE" },
	{ PIX_FMT_RGB555LE, "PIX_FMT_RGB555LE" },
	{ PIX_FMT_BGR565BE, "PIX_FMT_BGR565BE" },
	{ PIX_FMT_BGR565LE, "PIX_FMT_BGR565LE" },
	{ PIX_FMT_BGR555BE, "PIX_FMT_BGR555BE" },
	{ PIX_FMT_BGR555LE, "PIX_FMT_BGR555LE" },
	{ PIX_FMT_VAAPI_MOCO, "PIX_FMT_VAAPI_MOCO" },
	{ PIX_FMT_VAAPI_IDCT, "PIX_FMT_VAAPI_IDCT" },
	{ PIX_FMT_VAAPI_VLD, "PIX_FMT_VAAPI_VLD" },
	{ PIX_FMT_YUV420P16LE, "PIX_FMT_YUV420P16LE" },
	{ PIX_FMT_YUV420P16BE, "PIX_FMT_YUV420P16BE" },
	{ PIX_FMT_YUV422P16LE, "PIX_FMT_YUV422P16LE" },
	{ PIX_FMT_YUV422P16BE, "PIX_FMT_YUV422P16BE" },
	{ PIX_FMT_YUV444P16LE, "PIX_FMT_YUV444P16LE" },
	{ PIX_FMT_YUV444P16BE, "PIX_FMT_YUV444P16BE" },
	{ PIX_FMT_VDPAU_MPEG4, "PIX_FMT_VDPAU_MPEG4" },
	{ PIX_FMT_DXVA2_VLD, "PIX_FMT_DXVA2_VLD" },
	{ PIX_FMT_RGB444BE, "PIX_FMT_RGB444BE" },
	{ PIX_FMT_RGB444LE, "PIX_FMT_RGB444LE" },
	{ PIX_FMT_BGR444BE, "PIX_FMT_BGR444BE" },
	{ PIX_FMT_BGR444LE, "PIX_FMT_BGR444LE" },
	{ PIX_FMT_Y400A, "PIX_FMT_Y400A" },
	{ PIX_FMT_NB, "PIX_FMT_NB" },
	{ 0, NULL },
};

char *id_lookup_pix_fmt(int idnum)
{
	return id_lookup(id_pix_fmt, idnum);
}
#endif	/* HAVE_AV_GET_PIX_FMT_NAME */



#ifdef	HAVE_AV_GET_SAMPLE_FMT_NAME
char *id_lookup_sample_format(int idnum)
{
	return (char*) av_get_sample_fmt_name(idnum);
}
#else	/* no av_get_sample_fmt_name() defined */
#if	LIBAVCODEC_VERSION_MAJOR <= 53
struct	idtbl	id_sample_format[] = {
	{ SAMPLE_FMT_NONE, "SAMPLE_FMT_NONE" },
	{ SAMPLE_FMT_U8, "SAMPLE_FMT_U8_8-bit" },
	{ SAMPLE_FMT_S16, "SAMPLE_FMT_S16_16-bit" },
	{ SAMPLE_FMT_S32, "SAMPLE_FMT_S32_32-bit" },
	{ SAMPLE_FMT_FLT, "SAMPLE_FMT_FLT_Float" },
	{ SAMPLE_FMT_DBL, "SAMPLE_FMT_DBL_Double" },
	{ SAMPLE_FMT_NB, "SAMPLE_FMT_NB" },
	{ 0, NULL }
};
#else
struct	idtbl	id_sample_format[] = {	/* libavutil/samplefmt.h */
	{ AV_SAMPLE_FMT_NONE, "AV_SAMPLE_FMT_NONE" },
	{ AV_SAMPLE_FMT_U8, "AV_SAMPLE_FMT_U8" },	///< unsigned 8 bits
	{ AV_SAMPLE_FMT_S16, "AV_SAMPLE_FMT_S16" },	///< signed 16 bits
	{ AV_SAMPLE_FMT_S32, "AV_SAMPLE_FMT_S32" },	///< signed 32 bits
	{ AV_SAMPLE_FMT_FLT, "AV_SAMPLE_FMT_Float" },	///< float
	{ AV_SAMPLE_FMT_DBL, "AV_SAMPLE_FMT_Double" },	///< double
	{ AV_SAMPLE_FMT_U8P, "AV_SAMPLE_FMT_U8P" },	///< u8 bits, planar
	{ AV_SAMPLE_FMT_S16P, "AV_SAMPLE_FMT_S16P" },	///< s16 bits, planar
	{ AV_SAMPLE_FMT_S32P, "AV_SAMPLE_FMT_S32P" },	///< s32 bits, planar
	{ AV_SAMPLE_FMT_FLTP, "AV_SAMPLE_FMT_FLTP" },	///< float, planar
	{ AV_SAMPLE_FMT_DBLP, "AV_SAMPLE_FMT_DBLP" },	///< double, planar	
	{ AV_SAMPLE_FMT_NB, "AV_SAMPLE_FMT_NB" },
	{ 0, NULL }
};
#endif	/* LIBAVCODEC_VERSION_MAJOR <= 53 */
char *id_lookup_sample_format(int idnum)
{
	return id_lookup(id_sample_format, idnum);
}
#endif	/* HAVE_AV_GET_SAMPLE_FMT_NAME */




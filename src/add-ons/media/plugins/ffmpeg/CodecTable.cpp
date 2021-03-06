/*
 * Copyright (C) 2001 Carlos Hasan. All Rights Reserved.
 * Copyright (C) 2001 François Revol. All Rights Reserved.
 * Copyright (C) 2001 Axel Dörfler. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */

#include "CodecTable.h"

#define HAS_WMA_AUDIO
//#define HAS_MACE_AUDIO
#define HAS_PHOTO_JPEG
#define HAS_MOTION_JPEG

#define FOURCC(a) B_HOST_TO_BENDIAN_INT32(a)

const struct codec_table gCodecTable[] = {

	{CODEC_ID_PCM_ALAW,		B_MEDIA_ENCODED_AUDIO,	B_WAV_FORMAT_FAMILY,	0x06, "aLaw"},
	{CODEC_ID_PCM_ALAW,		B_MEDIA_ENCODED_AUDIO,	B_AIFF_FORMAT_FAMILY,	'alaw' , "aLaw"},
	{CODEC_ID_PCM_ALAW,		B_MEDIA_ENCODED_AUDIO,	B_AIFF_FORMAT_FAMILY,	'ALAW' , "aLaw"},
	{CODEC_ID_PCM_ALAW,		B_MEDIA_ENCODED_AUDIO,	B_MISC_FORMAT_FAMILY,	(uint64('au') << 32) | 27, "aLaw"},

	{CODEC_ID_PCM_MULAW,	B_MEDIA_ENCODED_AUDIO,	B_QUICKTIME_FORMAT_FAMILY,	'ulaw', "µLaw"},
	{CODEC_ID_PCM_ALAW,		B_MEDIA_ENCODED_AUDIO,	B_QUICKTIME_FORMAT_FAMILY,	'alaw', "aLaw"},
	{CODEC_ID_PCM_ALAW,		B_MEDIA_ENCODED_AUDIO,	B_QUICKTIME_FORMAT_FAMILY,	'ALAW', "aLaw"},

	{CODEC_ID_PCM_MULAW,	B_MEDIA_ENCODED_AUDIO,	B_WAV_FORMAT_FAMILY,	0x07, "µLaw"},
	{CODEC_ID_PCM_MULAW,	B_MEDIA_ENCODED_AUDIO,	B_AIFF_FORMAT_FAMILY,	'ulaw', "µLaw"},
	{CODEC_ID_PCM_MULAW,	B_MEDIA_ENCODED_AUDIO,	B_AIFF_FORMAT_FAMILY,	'ULAW', "µLaw"},
	{CODEC_ID_PCM_MULAW,	B_MEDIA_ENCODED_AUDIO,	B_MISC_FORMAT_FAMILY,	(uint64('au') << 32) | 1, "µLaw"},

	{CODEC_ID_ADPCM_IMA_WAV,	B_MEDIA_ENCODED_AUDIO,	B_WAV_FORMAT_FAMILY,	0x0011, "IMA ADPCM"},
	{CODEC_ID_ADPCM_MS,			B_MEDIA_ENCODED_AUDIO,	B_WAV_FORMAT_FAMILY,	0x0002, "MS ADPCM"},
	{CODEC_ID_ADPCM_IMA_WAV,	B_MEDIA_ENCODED_AUDIO,	B_QUICKTIME_FORMAT_FAMILY,	0x6D730011, "IMA ADPCM"},
	{CODEC_ID_ADPCM_MS,			B_MEDIA_ENCODED_AUDIO,	B_QUICKTIME_FORMAT_FAMILY,	0x6D730002, "MS ADPCM"},
	{CODEC_ID_MP2,				B_MEDIA_ENCODED_AUDIO,	B_QUICKTIME_FORMAT_FAMILY,	0x6D730050, "MP Layer2"},
	{CODEC_ID_MP2,				B_MEDIA_ENCODED_AUDIO,	B_QUICKTIME_FORMAT_FAMILY,	0x6D730055, "MP Layer3"},
	{CODEC_ID_MP2,				B_MEDIA_ENCODED_AUDIO,	B_MPEG_FORMAT_FAMILY,	0x6D730050, "MP Layer2"},
	{CODEC_ID_MP2,				B_MEDIA_ENCODED_AUDIO,	B_MPEG_FORMAT_FAMILY,	0x6D730055, "MP Layer3"},
#if 0
	{CODEC_ID_MP2,		B_MEDIA_ENCODED_AUDIO,	B_WAV_FORMAT_FAMILY,	0x0050, "MPEG Audio Layer 2"},	/* mpeg audio layer 2 */
	{CODEC_ID_MP2,		B_MEDIA_ENCODED_AUDIO,	B_WAV_FORMAT_FAMILY,	0x0055, "MPEG Audio Layer 3"},	/* mpeg audio layer 3 */
#endif

	{CODEC_ID_ADPCM_IMA_QT,		B_MEDIA_ENCODED_AUDIO,	B_QUICKTIME_FORMAT_FAMILY,	'ima4', "Quicktime IMA4"},
	{CODEC_ID_ADPCM_IMA_QT,		B_MEDIA_ENCODED_AUDIO,	B_QUICKTIME_FORMAT_FAMILY,	'IMA4', "Quicktime IMA4"},
	{CODEC_ID_AAC,			B_MEDIA_ENCODED_AUDIO,	B_QUICKTIME_FORMAT_FAMILY,	'mp4a', "MPEG4 AAC"},
	{CODEC_ID_MPEG4,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'mp4v', "MPEG4 Video"},
	{CODEC_ID_AAC,			B_MEDIA_ENCODED_AUDIO,	B_QUICKTIME_FORMAT_FAMILY,	'a4pm', "MPEG4 AAC"},
	{CODEC_ID_MPEG4,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'v4pm', "MPEG4 Video"},
	{CODEC_ID_AAC,			B_MEDIA_ENCODED_AUDIO,	B_MISC_FORMAT_FAMILY,	'mp4a', "MPEG4 AAC"},	/* For matroska */
	{CODEC_ID_AAC,			B_MEDIA_ENCODED_AUDIO,	B_MISC_FORMAT_FAMILY,	'a4pm', "MPEG4 AAC"},	/* For matroska */
	{CODEC_ID_AC3,			B_MEDIA_ENCODED_AUDIO,	B_WAV_FORMAT_FAMILY,	0x2000, "AC-3"},
	{CODEC_ID_AC3,			B_MEDIA_ENCODED_AUDIO,	B_WAV_FORMAT_FAMILY,	'3-CA', "AC-3"},

#ifdef HAS_MACE_AUDIO
	{CODEC_ID_MACE3,	B_MEDIA_ENCODED_AUDIO,	B_QUICKTIME_FORMAT_FAMILY,	'MAC3', "MACE 3:1"},
	{CODEC_ID_MACE6,	B_MEDIA_ENCODED_AUDIO,	B_QUICKTIME_FORMAT_FAMILY,	'MAC6', "MACE 6:1"},
#endif

	{CODEC_ID_WMAV1,	B_MEDIA_ENCODED_AUDIO,	B_WAV_FORMAT_FAMILY,	0x160, "MS WMA v1"},
	{CODEC_ID_WMAV2,	B_MEDIA_ENCODED_AUDIO,	B_WAV_FORMAT_FAMILY,	0x161, "MS WMA v2"},

	{CODEC_ID_FLAC,		B_MEDIA_ENCODED_AUDIO,	B_WAV_FORMAT_FAMILY,	'flac', "FLAC"},

	{CODEC_ID_CINEPAK,	B_MEDIA_ENCODED_VIDEO, B_AVI_FORMAT_FAMILY,	FOURCC('cvid'), "Cinepak Video"},
	{CODEC_ID_CINEPAK,	B_MEDIA_ENCODED_VIDEO, B_QUICKTIME_FORMAT_FAMILY,	'cvid', "Cinepak Video"},

	{CODEC_ID_MSRLE,	B_MEDIA_ENCODED_VIDEO, B_QUICKTIME_FORMAT_FAMILY,	FOURCC(' elr'), "MS RLE"}, // ???

	{CODEC_ID_MSRLE,	B_MEDIA_ENCODED_VIDEO, B_AVI_FORMAT_FAMILY,	FOURCC('RLE '), "MS RLE"}, // ???
	{CODEC_ID_MSRLE,	B_MEDIA_ENCODED_VIDEO, B_AVI_FORMAT_FAMILY,	FOURCC('mrle'), "MS RLE"}, // ???

	{CODEC_ID_MSVIDEO1,	B_MEDIA_ENCODED_VIDEO, B_AVI_FORMAT_FAMILY,	FOURCC('MSVC'), "MS Video 1 (MSVC)"},
	{CODEC_ID_MSVIDEO1,	B_MEDIA_ENCODED_VIDEO, B_AVI_FORMAT_FAMILY,	FOURCC('msvc'), "MS Video 1 (msvc)"},
	{CODEC_ID_MSVIDEO1,	B_MEDIA_ENCODED_VIDEO, B_AVI_FORMAT_FAMILY,	FOURCC('CRAM'), "MS Video 1 (CRAM)"},
	{CODEC_ID_MSVIDEO1,	B_MEDIA_ENCODED_VIDEO, B_AVI_FORMAT_FAMILY,	FOURCC('cram'), "MS Video 1 (cram)"},
	{CODEC_ID_MSVIDEO1,	B_MEDIA_ENCODED_VIDEO, B_AVI_FORMAT_FAMILY,	FOURCC('WHAM'), "MS Video 1 (WHAM)"},
	{CODEC_ID_MSVIDEO1,	B_MEDIA_ENCODED_VIDEO, B_AVI_FORMAT_FAMILY,	FOURCC('wham'), "MS Video 1 (wham)"},
	{CODEC_ID_MSVIDEO1,	B_MEDIA_ENCODED_VIDEO, B_AVI_FORMAT_FAMILY,	0x01, "MS Video 1 (1)"},
	{CODEC_ID_MSVIDEO1,	B_MEDIA_ENCODED_VIDEO, B_AVI_FORMAT_FAMILY,	FOURCC(0x01), "MS Video 1 (not 1)"},

	{CODEC_ID_H263,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('U263'), "U263"},
	{CODEC_ID_H263,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('H263'), "U263"},
//	{CODEC_ID_H263P,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('U263'), "U263"},
	{CODEC_ID_H263I,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('I263'), "Intel H263"},	/* intel h263 */
	{CODEC_ID_H263,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'h263', "H263"},
	{CODEC_ID_H263,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'H263', "H263"},

	{CODEC_ID_H264,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('h264'), "H264"},
	{CODEC_ID_H264,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('H264'), "H264"},
	{CODEC_ID_H264,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('x264'), "H264"},
	{CODEC_ID_H264,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'avc1', "AVC"},	/* MPEG-4 AVC */
	{CODEC_ID_H264,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	'VMDH', "AVC"},	/* MPEG-4 AVC */

#ifdef HAS_PHOTO_JPEG
	{CODEC_ID_MJPEG,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'JPEG', "Photo-JPEG"},
	{CODEC_ID_MJPEG,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'jpeg', "Photo-JPEG"}, /* used in BeOS_BBC.mov */
	{CODEC_ID_MJPEG,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'GEPJ', "Photo-JPEG"},
	{CODEC_ID_MJPEG,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'gepj', "Photo-JPEG"}, /* used in BeOS_BBC.mov */
#endif
#ifdef HAS_MOTION_JPEG
	{CODEC_ID_MJPEG,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('MJPG'), "Motion JPEG"},
	{CODEC_ID_MJPEG,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('mjpg'), "Motion JPEG"},
	{CODEC_ID_MJPEG,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'AVDJ', "Motion JPEG"},
	{CODEC_ID_MJPEG,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'mjpa', "Motion JPEG"},
	{CODEC_ID_MJPEGB,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'mjpb', "Motion JPEG"},
	{CODEC_ID_MJPEG,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'JDVA', "Motion JPEG"},
	{CODEC_ID_MJPEG,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'apjm', "Motion JPEG"},
	{CODEC_ID_MJPEGB,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'bpjm', "Motion JPEG"},
#endif

	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'FMP4', "ffmpeg MPEG4"},
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'4PMF', "ffmpeg MPEG4"},
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'DIVX', "MPEG4"},	/* OpenDivX */ /* XXX: doesn't seem to work */
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'divx', "MPEG4"},	/* OpenDivX */
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'mp4v', "MPEG4"},	/* MPEG-4 ASP */
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'XVID', "XviD (MPEG4)"},	/* OpenDivX ??? XXX: test */
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	'FMP4', "ffmpeg MPEG4"},
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	'4PMF', "ffmpeg MPEG4"},
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('DIVX'), "MPEG4"},	/* OpenDivX */
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('divx'), "MPEG4"},	/* OpenDivX */
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('XVID'), "XviD (MPEG4)"},	/* XVID */
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('XviD'), "XviD (MPEG4)"},	/* XVID */
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('xvid'), "XviD (MPEG4)"},	/* XVID */
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('DX50'), "DivX 5 (MPEG4)"},	/* DivX 5.0 */
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('dx50'), "DivX 5 (MPEG4)"},	/* DivX 5.0 */
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('3IV2'), "3ivx v2"},	/* 3ivx v2 */
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('3iv2'), "3ivx v2"},	/* 3ivx v2 */
	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('ffds'), "ff DirectShow"},	/* XVID Variant */

	{CODEC_ID_MPEG4,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('\004\0\0\0'), "MPEG4"}, /* some broken avi use this */
	{CODEC_ID_MSMPEG4V3,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('DIV3'), "DivX ;-) (MS MPEG4 v3)"},	/* default signature when using MSMPEG4 */
	{CODEC_ID_MSMPEG4V3,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('div3'), "DivX ;-) (MS MPEG4 v3)"},
	{CODEC_ID_MSMPEG4V3,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('DIV4'), "DivX ;-) (MS MPEG4 v3)"},
	{CODEC_ID_MSMPEG4V3,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('div4'), "DivX ;-) (MS MPEG4 v3)"},
	{CODEC_ID_MSMPEG4V3,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('DIV5'), "DivX ;-) (MS MPEG4 v3)"},
	{CODEC_ID_MSMPEG4V3,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('div5'), "DivX ;-) (MS MPEG4 v3)"},
	{CODEC_ID_MSMPEG4V3,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('DIV6'), "DivX ;-) (MS MPEG4 v3)"},
	{CODEC_ID_MSMPEG4V3,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('div6'), "DivX ;-) (MS MPEG4 v3)"},
	{CODEC_ID_MSMPEG4V1,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('MP41'), "MS MPEG4 v1"},	/* microsoft mpeg4 v1 */
	{CODEC_ID_MSMPEG4V1,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('mp41'), "MS MPEG4 v1"},	/* microsoft mpeg4 v1 */
	{CODEC_ID_MSMPEG4V2,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('MP42'), "MS MPEG4 v2"},	/* seems to be broken */
	{CODEC_ID_MSMPEG4V2,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('mp42'), "MS MPEG4 v2"},	/* seems to be broken */
	{CODEC_ID_MSMPEG4V3,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('MP43'), "MS MPEG4 v3"},	/* microsoft mpeg4 v3 */
	{CODEC_ID_MPEG4,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('S4PM'), "MPEG4"},	/* mpeg4 */
	{CODEC_ID_MSMPEG4V3,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('34PM'), "MS MPEG4 v3"},	/* microsoft mpeg4 v3 */
	{CODEC_ID_MSMPEG4V3,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('mp43'), "MS MPEG4 v3"},	/* microsoft mpeg4 v3 */
	{CODEC_ID_MSMPEG4V1,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('MPG4'), "MS MPEG4"},
	{CODEC_ID_MSMPEG4V3,	B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('AP41'), "Angel Potion (MS MPEG4 v3)"},	/* AngelPotion 1 (it's so simple to release a new codec... :^) ) */

	{CODEC_ID_WMV1,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('WMV1'), "Microsoft WMV v1"},
	{CODEC_ID_WMV2,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('WMV2'), "Microsoft WMV v2"},
	{CODEC_ID_WMV3,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('WMV3'), "Microsoft WMV v3"},

//SVQ1
	{CODEC_ID_SVQ1,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	'SVQ1', "Sorenson Video v1"},
	{CODEC_ID_SVQ1,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	'svq1', "Sorenson Video v1"},
	{CODEC_ID_SVQ1,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'SVQ1', "Sorenson Video v1"},
	{CODEC_ID_SVQ1,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'svq1', "Sorenson Video v1"},
	{CODEC_ID_SVQ1,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'svqi', "Sorenson Video v1"}, /* (from QT specs) */

	{CODEC_ID_SVQ3,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'SVQ3', "Sorenson Video v3"},
	{CODEC_ID_SVQ3,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'svq3', "Sorenson Video v3"},

	{CODEC_ID_SVQ1,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	'1QVS', "Sorenson Video v1"},
	{CODEC_ID_SVQ1,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	'1qvs', "Sorenson Video v1"},
	{CODEC_ID_SVQ1,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'1QVS', "Sorenson Video v1"},
	{CODEC_ID_SVQ1,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'1qvs', "Sorenson Video v1"},
	{CODEC_ID_SVQ1,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'iqvs', "Sorenson Video v1"}, /* (from QT specs) */

	{CODEC_ID_SVQ3,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'3QVS', "Sorenson Video v3"},
	{CODEC_ID_SVQ3,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'3qvs', "Sorenson Video v3"},

/*
	{CODEC_ID_RV10,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	'RV10', "RealVideo v1"},
	{CODEC_ID_RV10,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	'rv10', "RealVideo v1"},
	{CODEC_ID_RV10,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'RV10', "RealVideo v1"},
	{CODEC_ID_RV10,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'rv10', "RealVideo v1"},
*/
	{CODEC_ID_DVVIDEO,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('dvsd'), "DV Video"},
	{CODEC_ID_DVVIDEO,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('DVSD'), "DV Video"},
	{CODEC_ID_DVVIDEO,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('dvhd'), "DV Video"},
	{CODEC_ID_DVVIDEO,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('dvsl'), "DV Video"},
	{CODEC_ID_DVVIDEO,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('dv25'), "DV Video"},
	{CODEC_ID_DVVIDEO,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'dvc ', "DV Video"},
	{CODEC_ID_DVVIDEO,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'dvcp', "DV Video"},

	{CODEC_ID_MPEG1VIDEO,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'MPG1', "MPEG1 Video"},
	{CODEC_ID_MPEG1VIDEO,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'MPG2', "MPEG1 Video"},
	{CODEC_ID_MPEG1VIDEO,	B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'PIM1', "MPEG1 Video"},

	{CODEC_ID_MPEG1VIDEO,	B_MEDIA_ENCODED_VIDEO,	B_MPEG_FORMAT_FAMILY,	B_MPEG_1_VIDEO, "MPEG1 Video"},
	{CODEC_ID_MPEG2VIDEO,	B_MEDIA_ENCODED_VIDEO,	B_MPEG_FORMAT_FAMILY,	B_MPEG_2_VIDEO, "MPEG2 Video"},

	{CODEC_ID_INDEO3,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('iv31'), "Indeo 3"},
	{CODEC_ID_INDEO3,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('iv32'), "Indeo 3"},
	{CODEC_ID_INDEO3,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('IV31'), "Indeo 3"},
	{CODEC_ID_INDEO3,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	FOURCC('IV32'), "Indeo 3"},

	{CODEC_ID_VP3,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'VP31', "On2 VP3"},
	{CODEC_ID_VP6F,		B_MEDIA_ENCODED_VIDEO,	B_QUICKTIME_FORMAT_FAMILY,	'VP6F', "On2 VP6"},

	{CODEC_ID_CYUV,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	'cyuv', "CYUV"},
	{CODEC_ID_CYUV,		B_MEDIA_ENCODED_VIDEO,	B_AVI_FORMAT_FAMILY,	'CYUV', "CYUV"},

	{CODEC_ID_NONE,		B_MEDIA_UNKNOWN_TYPE,	B_ANY_FORMAT_FAMILY,	0, "null codec !!!"}
};

const int gCodecCount = (sizeof(gCodecTable) / sizeof(codec_table) - 1);


media_format gAVCodecFormats[sizeof(gCodecTable)];


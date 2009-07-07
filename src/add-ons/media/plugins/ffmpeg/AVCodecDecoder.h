/*
 * Copyright (C) 2001 Carlos Hasan.
 * Copyright (C) 2001 François Revol.
 * Copyright (C) 2001 Axel Dörfler.
 * Copyright (C) 2004 Marcus Overhagen.
 * Copyright (C) 2009 Stephan Aßmus <superstippi@gmx.de>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef AVCODEC_DECODER_H
#define AVCODEC_DECODER_H

//! libavcodec based decoder for Haiku

#include <MediaFormats.h>

#include "DecoderPlugin.h"
#include "ReaderPlugin.h"

#include "CodecTable.h"


class AVCodecDecoder : public Decoder {
public:
								AVCodecDecoder();
		
	virtual						~AVCodecDecoder();
		
	virtual	void				GetCodecInfo(media_codec_info* mci);
	
	virtual	status_t			Setup(media_format* ioEncodedFormat,
								   const void* infoBuffer, size_t infoSize);
   
	virtual	status_t			NegotiateOutputFormat(
									media_format* outputFormat);
	
	virtual	status_t			Decode(void* outBuffer, int64* outFrameCount,
						    		media_header* mediaHeader,
						    		media_decode_info* info);

	virtual	status_t			Seek(uint32 seekTo, int64 seekFrame,
									int64* frame, bigtime_t seekTime,
									bigtime_t* time);
	
	
protected:
			media_header		fHeader;
			media_decode_info	fInfo;
	
//	friend class avCodecInputStream;
		
private:
			media_format		fInputFormat;
			media_raw_video_format fOutputVideoFormat;

			int64				fFrame;
			bool				isAudio;
	
			int					ffcodec_index_in_table;
									// helps to find codecpretty
		
			// ffmpeg related datas
			AVCodec*			fCodec;
			AVCodecContext*		ffc;
			AVFrame*			ffpicture;
			AVFrame*			opicture;
		
			bool 				fCodecInitDone;

			gfx_convert_func	conv_func; // colorspace convert func

			char*				fExtraData;
			int					fExtraDataSize;
			int					fBlockAlign;
		
			bigtime_t			fStartTime;
			int32				fOutputFrameCount;
			float				fOutputFrameRate;
			int					fOutputFrameSize; // sample size * channel count
		
			const void*			fChunkBuffer;
			int32				fChunkBufferOffset;
			size_t				fChunkBufferSize;

			char*				fOutputBuffer;
			int32				fOutputBufferOffset;
			int32				fOutputBufferSize;

};

#endif // AVCODEC_DECODER_H
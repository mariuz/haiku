#ifndef _DECODER_PLUGIN_H
#define _DECODER_PLUGIN_H

#include <MediaTrack.h>
#include <MediaFormats.h>
#include "MediaPlugin.h"

class AddOnManager;

namespace BPrivate { namespace media {

class ChunkProvider {
public:
	virtual			~ChunkProvider() {};
	virtual status_t	GetNextChunk(const void **chunkBuffer, size_t *chunkSize,
						             media_header *mediaHeader) = 0;
};

class Decoder
{
public:
						Decoder();
	virtual				~Decoder();

	virtual void		GetCodecInfo(media_codec_info *codecInfo) = 0;

						// Setup get's called with the info data from Reader::GetStreamInfo
	virtual status_t	Setup(media_format *ioEncodedFormat, const void *infoBuffer, size_t infoSize) = 0;

	virtual status_t	NegotiateOutputFormat(media_format *ioDecodedFormat) = 0;
	
	virtual status_t	Seek(uint32 seekTo,
							 int64 seekFrame, int64 *frame,
							 bigtime_t seekTime, bigtime_t *time) = 0;
							 
	virtual status_t	Decode(void *buffer, int64 *frameCount,
							   media_header *mediaHeader, media_decode_info *info = 0) = 0;
							   
	status_t			GetNextChunk(const void **chunkBuffer, size_t *chunkSize,
									 media_header *mediaHeader);

	void				SetChunkProvider(ChunkProvider *provider);
private:
	ChunkProvider *		fChunkProvider;
};


class DecoderPlugin : public virtual MediaPlugin
{
	public:
		DecoderPlugin();

		virtual Decoder *NewDecoder(uint index) = 0;
		virtual status_t GetSupportedFormats(media_format ** formats, size_t * count) = 0;
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif

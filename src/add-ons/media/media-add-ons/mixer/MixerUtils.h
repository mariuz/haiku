#ifndef _MIXER_UTILS_H
#define _MIXER_UTILS_H

#if DEBUG > 0
	#define PRINT_CHANNEL_MASK(fmt) do { char s[200]; StringForChannelMask(s, (fmt).u.raw_audio.channel_mask); printf(" channel_mask 0x%08X %s\n", (fmt).u.raw_audio.channel_mask, s); } while (0)
#else
	#define PRINT_CHANNEL_MASK(fmt) ((void)0)
#endif

template<class t> const t & max(const t &t1, const t &t2) { return (t1 > t2) ?  t1 : t2; }
template<class t> const t & min(const t &t1, const t &t2) { return (t1 < t2) ?  t1 : t2; }

void fix_multiaudio_format(media_multi_audio_format *format);

int count_nonzero_bits(uint32 value);

uint32 GetChannelMask(int channel, uint32 all_channel_masks);
int GetChannelType(int channel, uint32 all_channel_masks);

bool HasKawamba();

void ZeroFill(float *_dst, int32 _dst_sample_offset, int32 _sample_count);

bigtime_t buffer_duration(const media_multi_audio_format & format);

int bytes_per_sample(const media_multi_audio_format & format);

int bytes_per_frame(const media_multi_audio_format & format);
int frames_per_buffer(const media_multi_audio_format & format);

int64 frames_for_duration(double framerate, bigtime_t duration);
bigtime_t duration_for_frames(double framerate, int64 frames);

int ChannelMaskToChannelType(uint32 mask);
uint32 ChannelTypeToChannelMask(int type);

double us_to_s(bigtime_t usecs);
bigtime_t s_to_us(double secs);

class MixerInput;
class MixerOutput;

const char *StringForFormat(char *buf, MixerOutput *output);
const char *StringForFormat(char *buf, MixerInput *input);
const char *StringForChannelMask(char *buf, uint32 mask);
const char *StringForChannelType(char *buf, int type);

#endif //_MIXER_UTILS_H

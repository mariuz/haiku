#include <MediaNode.h>
#include <Buffer.h>
#include <RealtimeAlloc.h>
#include "MixerInput.h"
#include "MixerCore.h"
#include "MixerUtils.h"
#include "debug.h"

MixerInput::MixerInput(MixerCore *core, const media_input &input, float mixSampleRate, int32 mixFramesCount, bigtime_t mixStartTime)
 :	fCore(core),
 	fInput(input),
	fInputChannelInfo(0),
	fInputChannelCount(0),
	fInputChannelMask(0),
	fMixerChannelInfo(0),
	fMixerChannelCount(0),
	fMixBuffer(0),
	fMixBufferSampleRate(0),
	fMixBufferFrames(0),
	fMixBufferStartTime(0),
	fUserOverridesChannelDesignations(false)
{
	fix_multiaudio_format(&fInput.format.u.raw_audio);
	PRINT_INPUT("MixerInput::MixerInput", fInput);
	PRINT_CHANNEL_MASK(fInput.format);
	
	ASSERT(fInput.format.u.raw_audio.channel_count > 0);

	fInputChannelCount = fInput.format.u.raw_audio.channel_count;
	fInputChannelMask = fInput.format.u.raw_audio.channel_mask;
	fInputChannelInfo = new input_chan_info[fInputChannelCount];
	
	// initialize fInputChannelInfo
	for (int i = 0; i < fInputChannelCount; i++) {
		fInputChannelInfo[i].buffer_base = 0;	// will be set by SetMixBufferFormat()
		fInputChannelInfo[i].designations = 0;	// will be set by UpdateChannelDesignations()
		fInputChannelInfo[i].gain = 1.0;
	}
	
	// fMixerChannelInfo and fMixerChannelCount will be initialized by UpdateMixerChannels()

	SetMixBufferFormat(mixSampleRate, mixFramesCount, mixStartTime);
	UpdateChannelDesignations();
	UpdateMixerChannels();
	
	
	// XXX a test:
	SetMixerChannelGain(0, 0.222);
	SetMixerChannelGain(1, 0.444);
	AddInputChannelDesignation(0, B_CHANNEL_REARRIGHT);
	SetMixerChannelGain(2, 0.666);
	AddInputChannelDesignation(1, B_CHANNEL_REARLEFT);
}

MixerInput::~MixerInput()
{
	if (fMixBuffer)
		rtm_free(fMixBuffer);
	delete [] fInputChannelInfo;
	delete [] fMixerChannelInfo;
}
	
void
MixerInput::BufferReceived(BBuffer *buffer)
{
	ASSERT(fMixBuffer);
	
	printf("mix buffer start %14Ld, buffer start %14Ld\n", fMixBufferStartTime, buffer->Header()->start_time);
}

media_input &
MixerInput::MediaInput()
{
	return fInput;
}

int32
MixerInput::ID()
{
	return fInput.destination.id;
}

void
MixerInput::AddInputChannelDesignation(int channel, uint32 des)
{
	ASSERT(count_nonzero_bits(des) == 1);
	
	// test if the channel is valid
	if (channel < 0 || channel >= fInputChannelCount)
		return;

	// test if it is already set
	if (fInputChannelInfo[channel].designations & des)
		return;

	// remove it from all other channels that might have it
	for (int i = 0; i < fInputChannelCount; i++)
		fInputChannelInfo[i].designations &= ~des;
	
	// add it to specified channel
	fInputChannelInfo[channel].designations |= des;

	fUserOverridesChannelDesignations = true;
	UpdateMixerChannels();
}

void
MixerInput::RemoveInputChannelDesignation(int channel, uint32 des)
{
	ASSERT(count_nonzero_bits(des) == 1);

	// test if the channel is valid
	if (channel < 0 || channel >= fInputChannelCount)
		return;

	// test if it is really set
	if ((fInputChannelInfo[channel].designations & des) == 0)
		return;

	// remove it from specified channel
	fInputChannelInfo[channel].designations &= ~des;

	fUserOverridesChannelDesignations = true;
	UpdateMixerChannels();
}

uint32
MixerInput::GetInputChannelDesignations(int channel)
{
	// test if the channel is valid
	if (channel < 0 || channel >= fInputChannelCount)
		return 0;
	return fInputChannelInfo[channel].designations;
}

uint32
MixerInput::GetInputChannelType(int channel)
{
	// test if the channel is valid
	if (channel < 0 || channel >= fInputChannelCount)
		return 0;
	return GetChannelMask(channel, fInputChannelMask);
}

void
MixerInput::SetInputChannelGain(int channel, float gain)
{
	// test if the channel is valid
	if (channel < 0 || channel >= fInputChannelCount)
		return;
	if (gain < 0.0f)
		gain = 0.0f;
		
	fInputChannelInfo[channel].gain = gain;
}

float
MixerInput::GetInputChannelGain(int channel)
{
	// test if the channel is valid
	if (channel < 0 || channel >= fInputChannelCount)
		return 0.0f;
	return fInputChannelInfo[channel].gain;
}

void
MixerInput::UpdateChannelDesignations()
{
	// is the user already messed with the assignmens, don't do anything.
	if (fUserOverridesChannelDesignations)
		return;

	printf("UpdateChannelDesignations: enter\n");

	if (fInputChannelCount == 1 && (GetChannelMask(0, fInputChannelMask) & (B_CHANNEL_LEFT | B_CHANNEL_RIGHT))) {
		// a left or right channel get's output as stereo on both
		fInputChannelInfo[0].designations = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
	} else {
		// everything else get's mapped 1:1
		for (int i = 0; i < fInputChannelCount; i++)
			fInputChannelInfo[i].designations = GetChannelMask(i, fInputChannelMask);
	}

	for (int i = 0; i < fInputChannelCount; i++)
		printf("UpdateChannelDesignations: input channel %d, designations 0x%08X, base %p, gain %.3f\n", i, fInputChannelInfo[i].designations, fInputChannelInfo[i].buffer_base, fInputChannelInfo[i].gain);

	printf("UpdateChannelDesignations: enter\n");
}

void
MixerInput::UpdateMixerChannels()
{
	uint32 channel_count;
	uint32 all_bits;
	uint32 mask;
	
	mixer_chan_info *old_mixer_channel_info;
	uint32 old_mixer_channel_count;

	ASSERT(fMixBuffer);
	
	printf("UpdateMixerChannels: enter\n");
	
	for (int i = 0; i < fInputChannelCount; i++)
		printf("UpdateMixerChannels: input channel %d, designations 0x%08X, base %p, gain %.3f\n", i, fInputChannelInfo[i].designations, fInputChannelInfo[i].buffer_base, fInputChannelInfo[i].gain);
	
	all_bits = 0;
	for (int i = 0; i < fInputChannelCount; i++)
		all_bits |= fInputChannelInfo[i].designations;

	printf("UpdateMixerChannels: all_bits = %08x\n", all_bits);
	
	channel_count = count_nonzero_bits(all_bits);
		
	printf("UpdateMixerChannels: %ld input channels, %ld mixer channels (%ld old)\n", fInputChannelCount, channel_count, fMixerChannelCount);

	// If we resize the channel info array, we preserve the gain setting
	// by saving the old array until new assignments are finished, and
	// then applying the old gains. New gains are set to 1.0
	if (channel_count != fMixerChannelCount) {
		old_mixer_channel_info = fMixerChannelInfo;
		old_mixer_channel_count = fMixerChannelCount;
		fMixerChannelInfo = new mixer_chan_info[channel_count];
		fMixerChannelCount = channel_count;
		for (int i = 0; i < fMixerChannelCount; i++)
			fMixerChannelInfo[i].gain = 1.0;
	} else {
		old_mixer_channel_info = 0;
		old_mixer_channel_count = 0;
	}
	
	// assign each mixer channel one type
	for (int i  = 0, mask = 1; i < fMixerChannelCount; i++) {
		while (mask != 0 && (all_bits & mask) == 0)
			mask <<= 1;
		fMixerChannelInfo[i].type = ChannelMaskToChannelType(mask);
		mask <<= 1;
	}

	// assign buffer_base pointer for each mixer channel
	for (int i  = 0; i < fMixerChannelCount; i++) {
		int j;
		for (j = 0; j < fInputChannelCount; j++) {
			if (fInputChannelInfo[j].designations & ChannelTypeToChannelMask(fMixerChannelInfo[i].type)) {
				fMixerChannelInfo[i].buffer_base = &fMixBuffer[j];
				break;
			}
		}
		if (j == fInputChannelCount) {
			printf("buffer assignment failed for mixer chan %d\n", i);
			fMixerChannelInfo[i].buffer_base = fMixBuffer;
		}
	}
	
	// apply old gains, overriding the 1.0 defaults for the old channels
	if (old_mixer_channel_info != 0) {
		for (int i  = 0; i < fMixerChannelCount; i++) {
			for (int j = 0; j < old_mixer_channel_count; j++) {
				if (fMixerChannelInfo[i].type == old_mixer_channel_info[j].type) {
					fMixerChannelInfo[i].gain = old_mixer_channel_info[j].gain;
					break;
				}
			}
		}
		// also delete the old info array
		delete [] old_mixer_channel_info;
	}

	for (int i = 0; i < fMixerChannelCount; i++)
		printf("UpdateMixerChannels: mixer channel %d, type %2d, des 0x%08X, base %p, gain %.3f\n", i, fMixerChannelInfo[i].type, ChannelTypeToChannelMask(fMixerChannelInfo[i].type), fMixerChannelInfo[i].buffer_base, fMixerChannelInfo[i].gain);

	printf("UpdateMixerChannels: leave\n");
}

uint32
MixerInput::GetMixerChannelCount()
{
	return fMixerChannelCount;
}

void
MixerInput::GetMixerChannelInfo(int channel, const float **buffer, uint32 *sample_offset, int *type, float *gain)
{
	ASSERT(fMixBuffer);
	ASSERT(channel >= 0 && channel < fMixerChannelCount);
	*buffer = fMixerChannelInfo[channel].buffer_base;
	*sample_offset = sizeof(float) * fInputChannelCount;
	*type = fMixerChannelInfo[channel].type;
	*gain = fMixerChannelInfo[channel].gain;
}

void
MixerInput::SetMixerChannelGain(int channel, float gain)
{
	if (channel < 0 || channel >= fMixerChannelCount)
		return;
	if (gain < 0.0f)
		gain = 0.0f;
	fMixerChannelInfo[channel].gain = gain;
}
	
float
MixerInput::GetMixerChannelGain(int channel)
{
	if (channel < 0 || channel >= fMixerChannelCount)
		return 1.0;
	return fMixerChannelInfo[channel].gain;
}

void
MixerInput::SetMixBufferFormat(float samplerate, int32 frames, bigtime_t starttime)
{
	fMixBufferSampleRate = samplerate;
	fMixBufferFrames = frames;
	fMixBufferStartTime = starttime;

	if (fMixBuffer)
		rtm_free(fMixBuffer);
	fMixBuffer = (float *)rtm_alloc(NULL, sizeof(float) * fInputChannelCount * fMixBufferFrames);

	for (int i = 0; i < fInputChannelCount; i++)
		fInputChannelInfo[i].buffer_base = &fMixBuffer[i * fMixBufferFrames];
}

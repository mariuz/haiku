#include <MediaNode.h>
#include "MixerOutput.h"
#include "MixerCore.h"
#include "MixerUtils.h"
#include "debug.h"

MixerOutput::MixerOutput(MixerCore *core, const media_output &output)
 :	fCore(core),
	fOutput(output),
	fOutputChannelCount(0),
	fOutputChannelInfo(0),
	fOutputByteSwap(0)
{
	fix_multiaudio_format(&fOutput.format.u.raw_audio);
	PRINT_OUTPUT("MixerOutput::MixerOutput", fOutput);
	PRINT_CHANNEL_MASK(fOutput.format);

	UpdateOutputChannels();
	UpdateByteOrderSwap();
}

MixerOutput::~MixerOutput()
{
	delete fOutputChannelInfo;
	delete fOutputByteSwap;
}

media_output &
MixerOutput::MediaOutput()
{
	return fOutput;
}

void
MixerOutput::ChangeFormat(const media_multi_audio_format &format)
{
	fOutput.format.u.raw_audio = format;
	fix_multiaudio_format(&fOutput.format.u.raw_audio);
	
	PRINT_OUTPUT("MixerOutput::ChangeFormat", fOutput);
	PRINT_CHANNEL_MASK(fOutput.format);

	UpdateOutputChannels();
	UpdateByteOrderSwap();
}

void
MixerOutput::UpdateByteOrderSwap()
{
	delete fOutputByteSwap;
	fOutputByteSwap = 0;

	// perhaps we need byte swapping
	if (fOutput.format.u.raw_audio.byte_order != B_MEDIA_HOST_ENDIAN) {
		if (	fOutput.format.u.raw_audio.format == media_raw_audio_format::B_AUDIO_FLOAT
			 ||	fOutput.format.u.raw_audio.format == media_raw_audio_format::B_AUDIO_INT
			 ||	fOutput.format.u.raw_audio.format == media_raw_audio_format::B_AUDIO_SHORT) {
			fOutputByteSwap = new ByteSwap(fOutput.format.u.raw_audio.format);
		}
	}
}

void
MixerOutput::UpdateOutputChannels()
{
	output_chan_info *oldInfo = fOutputChannelInfo;
	uint32 oldCount = fOutputChannelCount;

	fOutputChannelCount = fOutput.format.u.raw_audio.channel_count;
	fOutputChannelInfo = new output_chan_info[fOutputChannelCount];
	for (int i = 0; i < fOutputChannelCount; i++) {
		fOutputChannelInfo[i].designation = GetChannelMask(i, fOutput.format.u.raw_audio.channel_mask);
		fOutputChannelInfo[i].gain = 1.0;
		fOutputChannelInfo[i].source_count = 1;
		fOutputChannelInfo[i].source_gain[0] = 1.0;
		fOutputChannelInfo[i].source_type[0] = ChannelMaskToChannelType(fOutputChannelInfo[i].designation);
	}
	AssignDefaultSources();
	
	// apply old gains and sources, overriding the 1.0 gain defaults for the old channels
	if (oldInfo != 0 && oldCount != 0) {
		for (int i = 0; i < fOutputChannelCount; i++) {
			for (int j = 0; j < oldCount; j++) {
				if (fOutputChannelInfo[i].designation == oldInfo[j].designation) {
					fOutputChannelInfo[i].gain == oldInfo[j].gain;
					fOutputChannelInfo[i].source_count == oldInfo[j].source_count;
					for (int k = 0; k < fOutputChannelInfo[i].source_count; k++) {
						fOutputChannelInfo[i].source_gain[k] == oldInfo[j].source_gain[k];
						fOutputChannelInfo[i].source_type[k] == oldInfo[j].source_type[k];
					}
					break;
				}
			}
		}
		// also delete the old info array
		delete [] oldInfo;
	}
	for (int i = 0; i < fOutputChannelCount; i++)
		printf("UpdateOutputChannels: output channel %d, des 0x%08X (type %2d), gain %.3f\n", i, fOutputChannelInfo[i].designation, ChannelMaskToChannelType(fOutputChannelInfo[i].designation), fOutputChannelInfo[i].gain);
}

void
MixerOutput::AssignDefaultSources()
{
	uint32 mask = fOutput.format.u.raw_audio.channel_mask;
	uint32 count = fOutputChannelCount;
	
	// assign default sources for a few known setups,
	// everything else is left unchanged (it already is 1:1)

	if (count == 1 && mask & (B_CHANNEL_LEFT | B_CHANNEL_RIGHT)) {
		// we have only one phycial output channel, and use it as a mix of
		// left, right, rear-left, rear-right, center and sub
		printf("AssignDefaultSources: 1 channel setup\n");
		fOutputChannelInfo[0].source_count = 6;
		fOutputChannelInfo[0].source_gain[0] = 1.0;
		fOutputChannelInfo[0].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_LEFT);
		fOutputChannelInfo[0].source_gain[1] = 1.0;
		fOutputChannelInfo[0].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_RIGHT);
		fOutputChannelInfo[0].source_gain[2] = 0.8;
		fOutputChannelInfo[0].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_REARLEFT);
		fOutputChannelInfo[0].source_gain[3] = 0.8;
		fOutputChannelInfo[0].source_type[3] = ChannelMaskToChannelType(B_CHANNEL_REARRIGHT);
		fOutputChannelInfo[0].source_gain[4] = 0.7;
		fOutputChannelInfo[0].source_type[4] = ChannelMaskToChannelType(B_CHANNEL_CENTER);
		fOutputChannelInfo[0].source_gain[5] = 0.6;
		fOutputChannelInfo[0].source_type[5] = ChannelMaskToChannelType(B_CHANNEL_SUB);
	} else if (count == 2 && mask == (B_CHANNEL_LEFT | B_CHANNEL_RIGHT)) {
		// we have have two phycial output channels
		printf("AssignDefaultSources: 2 channel setup\n");
		// left channel:
		fOutputChannelInfo[0].source_count = 4;
		fOutputChannelInfo[0].source_gain[0] = 1.0;
		fOutputChannelInfo[0].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_LEFT);
		fOutputChannelInfo[0].source_gain[1] = 0.8;
		fOutputChannelInfo[0].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_REARLEFT);
		fOutputChannelInfo[0].source_gain[2] = 0.7;
		fOutputChannelInfo[0].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_CENTER);
		fOutputChannelInfo[0].source_gain[3] = 0.6;
		fOutputChannelInfo[0].source_type[3] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		// right channel:
		fOutputChannelInfo[1].source_count = 4;
		fOutputChannelInfo[1].source_gain[0] = 1.0;
		fOutputChannelInfo[1].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_RIGHT);
		fOutputChannelInfo[1].source_gain[1] = 0.8;
		fOutputChannelInfo[1].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_REARRIGHT);
		fOutputChannelInfo[1].source_gain[2] = 0.7;
		fOutputChannelInfo[1].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_CENTER);
		fOutputChannelInfo[1].source_gain[3] = 0.6;
		fOutputChannelInfo[1].source_type[3] = ChannelMaskToChannelType(B_CHANNEL_SUB);
	} else if (count == 4 && mask == (B_CHANNEL_LEFT | B_CHANNEL_RIGHT | B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT)) {
		printf("AssignDefaultSources: 4 channel setup\n");
		// left channel:
		fOutputChannelInfo[0].source_count = 3;
		fOutputChannelInfo[0].source_gain[0] = 1.0;
		fOutputChannelInfo[0].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_LEFT);
		fOutputChannelInfo[0].source_gain[1] = 0.7;
		fOutputChannelInfo[0].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_CENTER);
		fOutputChannelInfo[0].source_gain[2] = 0.6;
		fOutputChannelInfo[0].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		// right channel:
		fOutputChannelInfo[1].source_count = 3;
		fOutputChannelInfo[1].source_gain[0] = 1.0;
		fOutputChannelInfo[1].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_RIGHT);
		fOutputChannelInfo[1].source_gain[1] = 0.7;
		fOutputChannelInfo[1].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_CENTER);
		fOutputChannelInfo[1].source_gain[2] = 0.6;
		fOutputChannelInfo[1].source_type[2] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		// rear-left channel:
		fOutputChannelInfo[2].source_count = 2;
		fOutputChannelInfo[2].source_gain[0] = 1.0;
		fOutputChannelInfo[2].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_REARLEFT);
		fOutputChannelInfo[2].source_gain[1] = 0.6;
		fOutputChannelInfo[2].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		// rear-right channel:
		fOutputChannelInfo[3].source_count = 2;
		fOutputChannelInfo[3].source_gain[0] = 1.0;
		fOutputChannelInfo[3].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_REARRIGHT);
		fOutputChannelInfo[3].source_gain[1] = 0.6;
		fOutputChannelInfo[3].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_SUB);
	} else if (count == 5 && mask == (B_CHANNEL_LEFT | B_CHANNEL_RIGHT | B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT | B_CHANNEL_CENTER)) {
		printf("AssignDefaultSources: 5 channel setup\n");
		// left channel:
		fOutputChannelInfo[0].source_count = 2;
		fOutputChannelInfo[0].source_gain[0] = 1.0;
		fOutputChannelInfo[0].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_LEFT);
		fOutputChannelInfo[0].source_gain[1] = 0.6;
		fOutputChannelInfo[0].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		// right channel:
		fOutputChannelInfo[1].source_count = 2;
		fOutputChannelInfo[1].source_gain[0] = 1.0;
		fOutputChannelInfo[1].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_RIGHT);
		fOutputChannelInfo[1].source_gain[1] = 0.6;
		fOutputChannelInfo[1].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		// rear-left channel:
		fOutputChannelInfo[2].source_count = 2;
		fOutputChannelInfo[2].source_gain[0] = 1.0;
		fOutputChannelInfo[2].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_REARLEFT);
		fOutputChannelInfo[2].source_gain[1] = 0.6;
		fOutputChannelInfo[2].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		// rear-right channel:
		fOutputChannelInfo[3].source_count = 2;
		fOutputChannelInfo[3].source_gain[0] = 1.0;
		fOutputChannelInfo[3].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_REARRIGHT);
		fOutputChannelInfo[3].source_gain[1] = 0.6;
		fOutputChannelInfo[3].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_SUB);
		// center channel:
		fOutputChannelInfo[4].source_count = 2;
		fOutputChannelInfo[4].source_gain[0] = 1.0;
		fOutputChannelInfo[4].source_type[0] = ChannelMaskToChannelType(B_CHANNEL_CENTER);
		fOutputChannelInfo[4].source_gain[1] = 0.5;
		fOutputChannelInfo[4].source_type[1] = ChannelMaskToChannelType(B_CHANNEL_SUB);
	} else {
		printf("AssignDefaultSources: no default setup\n");
	}

	for (int i = 0; i < fOutputChannelCount; i++) {
		for (int j = 0; j < fOutputChannelInfo[i].source_count; j++) {
			printf("AssignDefaultSources: output channel %d, source %d: des 0x%08X (type %2d), gain %.3f\n", i, j, ChannelTypeToChannelMask(fOutputChannelInfo[i].source_type[j]), fOutputChannelInfo[i].source_type[j], fOutputChannelInfo[i].source_gain[j]);
		}
	}
}

uint32
MixerOutput::GetOutputChannelDesignation(int channel)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return 0;
	return fOutputChannelInfo[channel].designation;
}

void
MixerOutput::SetOutputChannelGain(int channel, float gain)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return;
	fOutputChannelInfo[channel].gain = gain;
}

void
MixerOutput::AddOutputChannelSource(int channel, uint32 source_designation, float source_gain)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return;
	if (fOutputChannelInfo[channel].source_count == MAX_SOURCES)
		return;
	int source_type = ChannelMaskToChannelType(source_designation);
	for (int i = 0; i < fOutputChannelInfo[channel].source_count; i++) {
		if (fOutputChannelInfo[channel].source_type[i] == source_type)
			return;
	}
	fOutputChannelInfo[channel].source_type[fOutputChannelInfo[channel].source_count] = source_type;
	fOutputChannelInfo[channel].source_gain[fOutputChannelInfo[channel].source_count] = source_gain;
	fOutputChannelInfo[channel].source_count++;
}

void
MixerOutput::RemoveOutputChannelSource(int channel, uint32 source_designation)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return;
	int source_type = ChannelMaskToChannelType(source_designation);
	for (int i = 0; i < fOutputChannelInfo[channel].source_count; i++) {
		if (fOutputChannelInfo[channel].source_type[i] == source_type) {
			fOutputChannelInfo[channel].source_type[i] = fOutputChannelInfo[channel].source_type[fOutputChannelInfo[channel].source_count - 1];
			fOutputChannelInfo[channel].source_gain[i] = fOutputChannelInfo[channel].source_gain[fOutputChannelInfo[channel].source_count - 1];
			fOutputChannelInfo[channel].source_count--;
			return;
		}
	}
}

void
MixerOutput::SetOutputChannelSourceGain(int channel, uint32 source_designation, float source_gain)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return;
	int source_type = ChannelMaskToChannelType(source_designation);
	for (int i = 0; i < fOutputChannelInfo[channel].source_count; i++) {
		if (fOutputChannelInfo[channel].source_type[i] == source_type) {
			fOutputChannelInfo[channel].source_gain[i] = source_gain;
			return;
		}
	}
}

float
MixerOutput::GetOutputChannelSourceGain(int channel, uint32 source_designation)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return 1.0;
	int source_type = ChannelMaskToChannelType(source_designation);
	for (int i = 0; i < fOutputChannelInfo[channel].source_count; i++) {
		if (fOutputChannelInfo[channel].source_type[i] == source_type) {
			return fOutputChannelInfo[channel].source_gain[i];
		}
	}
	return 1.0;
}

void
MixerOutput::GetOutputChannelSourceAt(int channel, int index, uint32 *source_designation, float *source_gain)
{
	if (channel < 0 || channel >= fOutputChannelCount)
		return;
	if (index >= fOutputChannelInfo[channel].source_count) {
		// ERROR
		*source_gain = 1.0;
		*source_designation = 0;
		return;
	}
	*source_gain = fOutputChannelInfo[channel].source_gain[index];
	*source_designation = ChannelTypeToChannelMask(fOutputChannelInfo[channel].source_type[index]);
}

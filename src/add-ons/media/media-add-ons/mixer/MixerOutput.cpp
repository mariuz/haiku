#include <MediaNode.h>
#include "MixerOutput.h"
#include "MixerCore.h"
#include "MixerUtils.h"
#include "debug.h"

MixerOutput::MixerOutput(MixerCore *core, const media_output &output)
 :	fCore(core),
	fOutput(output)
{
	fix_multiaudio_format(&fOutput.format.u.raw_audio);
	PRINT_OUTPUT("MixerOutput::MixerOutput", fOutput);
	PRINT_CHANNEL_MASK(fOutput.format);
}

MixerOutput::~MixerOutput()
{
}

media_output &
MixerOutput::MediaOutput()
{
	return fOutput;
}

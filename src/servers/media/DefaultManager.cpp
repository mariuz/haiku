/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <OS.h>
#include <MediaNode.h>
#include <MediaRoster.h>
#include <TimeSource.h>
#include <string.h>
#include "DefaultManager.h"
#include "debug.h"

/* no locking used in this file, we assume that the caller (NodeManager) does it.
 */


#define MAX_NODE_INFOS 10

DefaultManager::DefaultManager()
 :	fMixerConnected(false),
 	fPhysicalVideoOut(-1),
	fPhysicalVideoIn(-1),
	fPhysicalAudioOut(-1),
	fPhysicalAudioIn(-1),
	fSystemTimeSource(-1),
	fTimeSource(-1),
	fAudioMixer(-1),
	fPhysicalAudioOutInputID(0)
{
	strcpy(fPhysicalAudioOutInputName, "default");
}

DefaultManager::~DefaultManager()
{
}

// this is called by the media_server *before* any add-ons have been loaded
status_t
DefaultManager::LoadState()
{
	return B_OK;
}

status_t
DefaultManager::SaveState()
{
	return B_OK;
}

status_t
DefaultManager::Set(node_type type, const media_node *node, const dormant_node_info *info, const media_input *input)
{
	switch (type) {
		case VIDEO_INPUT:
		case AUDIO_INPUT:
		case VIDEO_OUTPUT:
		case AUDIO_MIXER:
		case AUDIO_OUTPUT:
		case AUDIO_OUTPUT_EX:
		case TIME_SOURCE:
			return B_ERROR;
		
		case SYSTEM_TIME_SOURCE: //called by the media_server's ServerApp::StartSystemTimeSource()
		{
			ASSERT(fSystemTimeSource == -1);
			ASSERT(node != 0);
			fSystemTimeSource = node->node;
			return B_OK;
		}
		
		default:
		{
			FATAL("DefaultManager::Set Error: called with unknown type %d\n", type);
			return B_ERROR;
		}
	}
}

status_t
DefaultManager::Get(media_node_id *nodeid, char *input_name, int32 *inputid, node_type type)
{
	switch (type) {
		case VIDEO_INPUT: 		// output: nodeid
			if (fPhysicalVideoIn == -1)
				return B_ERROR;
			*nodeid = fPhysicalVideoIn;
			return B_OK;

		case AUDIO_INPUT: 		// output: nodeid
			if (fPhysicalAudioIn == -1)
				return B_ERROR;
			*nodeid = fPhysicalAudioIn;
			return B_OK;
			
		case VIDEO_OUTPUT: 		// output: nodeid
			if (fPhysicalVideoOut == -1)
				return B_ERROR;
			*nodeid = fPhysicalVideoOut;
			return B_OK;

		case AUDIO_OUTPUT:		// output: nodeid
			if (fPhysicalAudioOut == -1)
				return B_ERROR;
			*nodeid = fPhysicalAudioOut;
			return B_OK;

		case AUDIO_OUTPUT_EX:	// output: nodeid, input_name, input_id
			if (fPhysicalAudioOut == -1)
				return B_ERROR;
			*nodeid = fPhysicalAudioOut;
			*inputid = fPhysicalAudioOutInputID;
			strcpy(input_name, fPhysicalAudioOutInputName);
			return B_OK;

		case AUDIO_MIXER:		// output: nodeid
			if (fAudioMixer == -1)
				return B_ERROR;
			*nodeid = fAudioMixer;
			return B_OK;

		case TIME_SOURCE:
			if (fTimeSource != -1)
				*nodeid = fTimeSource;
			else
				*nodeid = fSystemTimeSource;
			return B_OK;
			
		case SYSTEM_TIME_SOURCE:
			*nodeid = fSystemTimeSource;
			return B_OK;

		default:
		{
			FATAL("DefaultManager::Get Error: called with unknown type %d\n", type);
			return B_ERROR;
		}
	}
}

// this is called by the media_server *after* the initial add-on loading has been done
status_t
DefaultManager::Rescan()
{
	thread_id	fThreadId = spawn_thread(rescan_thread, "rescan defaults", 8, this);
	resume_thread(fThreadId);
	return B_OK;
}

int32
DefaultManager::rescan_thread(void *arg)
{
	reinterpret_cast<DefaultManager *>(arg)->RescanThread();
	return 0;
}

void
DefaultManager::RescanThread()
{
	printf("DefaultManager::RescanThread() enter\n");
	
	// We do not search for the system time source,
	// it should already exist
	ASSERT(fSystemTimeSource != -1);

	if (fPhysicalVideoOut == -1)
		FindPhysicalVideoOut();
	if (fPhysicalVideoIn == -1)
		FindPhysicalVideoIn();
	if (fPhysicalAudioOut == -1)
		FindPhysicalAudioOut();
	if (fPhysicalAudioIn == -1)
		FindPhysicalAudioIn();
	if (fAudioMixer == -1)
		FindAudioMixer();

	// The normal time source is searched for after the
	// Physical Audio Out has been created.
	if (fTimeSource == -1)
		FindTimeSource();

	// Connect the mixer and physical audio out (soundcard)
	if (!fMixerConnected && fAudioMixer != -1 && fPhysicalAudioOut != -1) {
		fMixerConnected = B_OK == ConnectMixerToOutput();
		if (!fMixerConnected)
			FATAL("DefaultManager: failed to connect mixer and soundcard\n");
	} else {
		FATAL("DefaultManager: Did not try to connect mixer and soundcard\n");
	}

	printf("DefaultManager::RescanThread() leave\n");
}

void
DefaultManager::FindPhysicalVideoOut()
{
	live_node_info info;
	media_format input; /* a physical video output has a logical data input */
	int32 count;
	status_t rv;

	memset(&input, 0, sizeof(input));
	input.type = B_MEDIA_RAW_VIDEO;
	count = 1;
	rv = BMediaRoster::Roster()->GetLiveNodes(&info, &count, &input, NULL, NULL, B_BUFFER_CONSUMER | B_PHYSICAL_OUTPUT);
	if (rv != B_OK || count != 1) {
		printf("Couldn't find physical video output node\n");
		return;
	}
	printf("Default physical video output created!\n");
	fPhysicalVideoOut = info.node.node;
}

void
DefaultManager::FindPhysicalVideoIn()
{
	live_node_info info;
	media_format output; /* a physical video input has a logical data output */
	int32 count;
	status_t rv;

	memset(&output, 0, sizeof(output));
	output.type = B_MEDIA_RAW_VIDEO;
	count = 1;
	rv = BMediaRoster::Roster()->GetLiveNodes(&info, &count, NULL, &output, NULL, B_BUFFER_PRODUCER | B_PHYSICAL_INPUT);
	if (rv != B_OK || count != 1) {
		printf("Couldn't find physical video input node\n");
		return;
	}
	printf("Default physical video input created!\n");
	fPhysicalVideoIn = info.node.node;
}

void
DefaultManager::FindPhysicalAudioOut()
{
	live_node_info info[MAX_NODE_INFOS];
	media_format input; /* a physical audio output has a logical data input */
	int32 count;
	status_t rv;

	memset(&input, 0, sizeof(input));
	input.type = B_MEDIA_RAW_AUDIO;
	count = MAX_NODE_INFOS;
	rv = BMediaRoster::Roster()->GetLiveNodes(&info[0], &count, &input, NULL, NULL, B_BUFFER_CONSUMER | B_PHYSICAL_OUTPUT);
	if (rv != B_OK || count < 1) {
		printf("Couldn't find physical audio output node\n");
		return;
	}
	for (int i = 0; i < count; i++)
		printf("info[%d].name %s\n", i, info[i].name);
	
	for (int i = 0; i < count; i++) {
		if (0 == strcmp(info[i].name, "None Out")) // skip the Null audio driver
			continue;
		if (0 == strcmp(info[i].name, "DV Output")) // skip the Firewire audio driver
			continue;
		printf("Default physical audio output \"%s\" created!\n", info[i].name);
		fPhysicalAudioOut = info[i].node.node;
		return;
	}
}

void
DefaultManager::FindPhysicalAudioIn()
{
	live_node_info info[MAX_NODE_INFOS];
	media_format output; /* a physical audio input has a logical data output */
	int32 count;
	status_t rv;

	memset(&output, 0, sizeof(output));
	output.type = B_MEDIA_RAW_AUDIO;
	count = MAX_NODE_INFOS;
	rv = BMediaRoster::Roster()->GetLiveNodes(&info[0], &count, NULL, &output, NULL, B_BUFFER_PRODUCER | B_PHYSICAL_INPUT);
	if (rv != B_OK || count < 1) {
		printf("Couldn't find physical audio input node\n");
		return;
	}
	for (int i = 0; i < count; i++)
		printf("info[%d].name %s\n", i, info[i].name);
	
	for (int i = 0; i < count; i++) {
		if (0 == strcmp(info[i].name, "None In")) // skip the Null audio driver
			continue;
		if (0 != strstr(info[i].name, "DV Input")) // skip the Firewire audio driver
			continue;
		printf("Default physical audio input \"%s\" created!\n", info[i].name);
		fPhysicalAudioIn = info[i].node.node;
		return;
	}
}

void
DefaultManager::FindTimeSource()
{
	live_node_info info[MAX_NODE_INFOS];
	media_format input; /* a physical audio output has a logical data input (DAC)*/
	int32 count;
	status_t rv;
	
	/* First try to use the current default physical audio out
	 */
	if (fPhysicalAudioOut != -1) {
		media_node clone;
		if (B_OK == BMediaRoster::Roster()->GetNodeFor(fPhysicalAudioOut, &clone)) {
			if (clone.kind & B_TIME_SOURCE) {
				fTimeSource = clone.node;
				BMediaRoster::Roster()->ReleaseNode(clone);
				printf("Default DAC timesource created!\n");
				return;
			}
			BMediaRoster::Roster()->ReleaseNode(clone);
		} else {
			printf("Default DAC is not a timesource!\n");
		}
	} else {
		printf("Default DAC node does not exist!\n");
	}
	
	/* Now try to find another physical audio out node
	 */
	memset(&input, 0, sizeof(input));
	input.type = B_MEDIA_RAW_AUDIO;
	count = MAX_NODE_INFOS;
	rv = BMediaRoster::Roster()->GetLiveNodes(&info[0], &count, &input, NULL, NULL, B_TIME_SOURCE | B_PHYSICAL_OUTPUT);
	if (rv == B_OK && count >= 1) {
		for (int i = 0; i < count; i++)
			printf("info[%d].name %s\n", i, info[i].name);
	
		for (int i = 0; i < count; i++) {
			// The BeOS R5 None Out node pretend to be a physical time source, that is pretty dumb
			if (0 == strcmp(info[i].name, "None Out")) // skip the Null audio driver
				continue;
			if (0 != strstr(info[i].name, "DV Output")) // skip the Firewire audio driver
				continue;
			printf("Default DAC timesource \"%s\" created!\n", info[i].name);
			fTimeSource = info[i].node.node;
			return;
		}
	} else {
		printf("Couldn't find DAC timesource node\n");
	}	
	
	/* XXX we might use other audio or video clock timesources
	 */
}

void
DefaultManager::FindAudioMixer()
{
	live_node_info info;
	int32 count;
	status_t rv;

	count = 1;
	rv = BMediaRoster::Roster()->GetLiveNodes(&info, &count, NULL, NULL, NULL, B_BUFFER_PRODUCER | B_BUFFER_CONSUMER | B_SYSTEM_MIXER);
	if (rv != B_OK || count != 1) {
		printf("Couldn't find audio mixer node\n");
		return;
	}
	fAudioMixer = info.node.node;
	printf("Default audio mixer node created\n");
}

status_t
DefaultManager::ConnectMixerToOutput()
{
	BMediaRoster 		*roster;
	media_node 			timesource;
	media_node 			mixer;
	media_node 			soundcard;
	media_input 		input;
	media_output 		output;
	media_input 		newinput;
	media_output 		newoutput;
	media_format 		format;
	BTimeSource * 		ts;
	bigtime_t 			start_at;
	int32 				count;
	status_t 			rv;
	
	roster = BMediaRoster::Roster();

	// XXX this connects to *any* physical output, but not it's default logical input
	rv = roster->GetNodeFor(fPhysicalAudioOut, &soundcard);
	if (rv != B_OK) {
		printf("DefaultManager: failed to find soundcard (physical audio output)\n");
		return B_ERROR;
	}

	rv = roster->GetNodeFor(fAudioMixer, &mixer);
	if (rv != B_OK) {
		roster->ReleaseNode(soundcard);
		printf("DefaultManager: failed to find mixer\n");
		return B_ERROR;
	}

	// we now have the mixer and soundcard nodes,
	// find a free input/output and connect them

	rv = roster->GetFreeOutputsFor(mixer, &output, 1, &count, B_MEDIA_RAW_AUDIO);
	if (rv != B_OK || count != 1) {
		printf("DefaultManager: can't find free mixer output\n");
		rv = B_ERROR;
		goto finish;
	}

	rv = roster->GetFreeInputsFor(soundcard, &input, 1, &count, B_MEDIA_RAW_AUDIO);
	if (rv != B_OK || count != 1) {
		printf("DefaultManager: can't find free soundcard input\n");
		rv = B_ERROR;
		goto finish;
	}
	
	for (int i = 0; i < 5; i++) {
		switch (i) {
			case 0:
				printf("DefaultManager: Trying connect in native format\n");
				if (B_OK != roster->GetFormatFor(input, &format)) {
					FATAL("DefaultManager: GetFormatFor failed\n");
					continue;
				}
				break;
			
			case 1:
				printf("DefaultManager: Trying connect in format 1\n");
				memset(&format, 0, sizeof(format));
				format.type = B_MEDIA_RAW_AUDIO;
				format.u.raw_audio.frame_rate = 44100;
				format.u.raw_audio.channel_count = 2;
				format.u.raw_audio.format = 0x2;
				break;

			case 2:
				printf("DefaultManager: Trying connect in format 2\n");
				memset(&format, 0, sizeof(format));
				format.type = B_MEDIA_RAW_AUDIO;
				format.u.raw_audio.frame_rate = 48000;
				format.u.raw_audio.channel_count = 2;
				format.u.raw_audio.format = 0x2;
				break;

			case 3:
				printf("DefaultManager: Trying connect in format 3\n");
				memset(&format, 0, sizeof(format));
				format.type = B_MEDIA_RAW_AUDIO;
				break;

			case 4:
				printf("DefaultManager: Trying connect in format 4\n");
				memset(&format, 0, sizeof(format));
				break;
		}
		rv = roster->Connect(output.source, input.destination, &format, &newoutput, &newinput);
		if (rv == B_OK)
			break;
	}
	if (rv != B_OK) {
		FATAL("DefaultManager: connect failed\n");
		goto finish;
	}

	roster->SetRunModeNode(mixer, BMediaNode::B_INCREASE_LATENCY);
	roster->SetRunModeNode(soundcard, BMediaNode::B_RECORDING);

	roster->GetTimeSource(&timesource);
	roster->SetTimeSourceFor(mixer.node, timesource.node);
	roster->SetTimeSourceFor(soundcard.node, timesource.node);
	roster->PrerollNode(mixer);
	roster->PrerollNode(soundcard);
	
	ts = roster->MakeTimeSourceFor(mixer);
	start_at = ts->Now() + 50000;
	roster->StartNode(mixer, start_at);
	roster->StartNode(soundcard, start_at);
	ts->Release();
	
finish:
	roster->ReleaseNode(mixer);
	roster->ReleaseNode(soundcard);
	roster->ReleaseNode(timesource);
	return rv;
}

void
DefaultManager::Dump()
{
}

void
DefaultManager::CleanupTeam(team_id team)
{
}


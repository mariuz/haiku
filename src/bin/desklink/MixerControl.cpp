/*
 * Copyright 2003-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 */


#include "MixerControl.h"

#include <Debug.h>
#include <ParameterWeb.h>


MixerControl::MixerControl(int32 volumeWhich, float* _value,
		const char** _error)
	:
	fAudioMixerNode(NULL),
	fParameterWeb(NULL),
	fMixerParameter(NULL),
	fMin(0.0f),
	fMax(0.0f),
	fStep(0.0f)
{
	bool retrying = false;
	fAudioMixerNode = new media_node();

	status_t err = B_OK;
		/* BMediaRoster::Roster() doesn't set it if all is ok */
	const char* errorString = NULL;
	BMediaRoster* roster = BMediaRoster::Roster(&err);

retry:
	// Here we release the BMediaRoster once if we can't access the system
	// mixer, to make sure it really isn't there, and it's not BMediaRoster
	// that is messed up.
	if (retrying) {
		errorString = NULL;
		PRINT(("retrying to get a Media Roster\n"));
		/* BMediaRoster looks doomed */
		roster = BMediaRoster::CurrentRoster();
		if (roster) {
			roster->Lock();
			roster->Quit();
		}
		snooze(10000);
		roster = BMediaRoster::Roster(&err);
	}
	
	if (roster && err == B_OK) {
		switch (volumeWhich) {
			case VOLUME_USE_MIXER:
				err = roster->GetAudioMixer(fAudioMixerNode);
				break;
			case VOLUME_USE_PHYS_OUTPUT:
				err = roster->GetAudioOutput(fAudioMixerNode);
				break;
		}
		if (err == B_OK) {
			err = roster->GetParameterWebFor(*fAudioMixerNode, &fParameterWeb);
			if (err == B_OK) {
				// Finding the Mixer slider in the audio output ParameterWeb 
				int32 numParams = fParameterWeb->CountParameters();
				BParameter* p = NULL;
				bool foundMixerLabel = false;
				for (int i = 0; i < numParams; i++) {
					p = fParameterWeb->ParameterAt(i);
					PRINT(("BParameter[%i]: %s\n", i, p->Name()));
					if (volumeWhich == VOLUME_USE_MIXER) {
						if (!strcmp(p->Kind(), B_MASTER_GAIN))
							break;
					} else if (volumeWhich == VOLUME_USE_PHYS_OUTPUT) {
						/* not all cards use the same name, and
						 * they don't seem to use Kind() == B_MASTER_GAIN
						 */
						if (!strcmp(p->Kind(), B_MASTER_GAIN))
							break;
						PRINT(("not MASTER_GAIN \n"));

						/* some audio card
						 */
						if (!strcmp(p->Name(), "Master"))
							break;
						PRINT(("not 'Master' \n"));

						/* some Ensonic card have all controls names 'Volume', so
						 * need to fint the one that has the 'Mixer' text label
						 */
						if (foundMixerLabel && !strcmp(p->Name(), "Volume"))
							break;
						if (!strcmp(p->Name(), "Mixer"))
							foundMixerLabel = true;
						PRINT(("not 'Mixer' \n"));
					}
#if 0
					//if (!strcmp(p->Name(), "Master")) {
					if (!strcmp(p->Kind(), B_MASTER_GAIN)) {
						for (; i < numParams; i++) {
							p = fParamWeb->ParameterAt(i);
							if (strcmp(p->Kind(), B_MASTER_GAIN)) p=NULL;
							else break;
						}
						break;
					} else p = NULL;
#endif
					p = NULL;
				}
				if (p == NULL) {
					errorString = volumeWhich
						? "Could not find the soundcard":"Could not find the mixer";
				} else if (p->Type() != BParameter::B_CONTINUOUS_PARAMETER) {
					errorString = volumeWhich
						? "Soundcard control unknown":"Mixer control unknown";
				} else {
					fMixerParameter = dynamic_cast<BContinuousParameter*>(p);
					fMin = fMixerParameter->MinValue();
					fMax = fMixerParameter->MaxValue();
					fStep = fMixerParameter->ValueStep();

					if (_value != NULL) {
						float volume;
						bigtime_t lastChange;
						size_t size = sizeof(float);
						fMixerParameter->GetValue(&volume, &size, &lastChange);

						*_value = volume;
					}
				}
			} else {
				errorString = "No parameter web";
			}
		} else {
			if (!retrying) {
				retrying = true;
				goto retry;
			}
			errorString = volumeWhich ? "No Audio output" : "No Mixer";
		}
	} else {
		if (!retrying) {
			retrying = true;
			goto retry;
		}
		errorString = "No Media Roster";
	}

	if (err != B_OK) {
		delete fAudioMixerNode;
		fAudioMixerNode = NULL;
	}
	if (errorString) {
		fprintf(stderr, "MixerControl: %s.\n", errorString);
		if (_error)
			*_error = errorString;
	}
	if (fMixerParameter == NULL && _value != NULL)
		*_value = 0;
}


MixerControl::~MixerControl()
{
	delete fParameterWeb;

	BMediaRoster* roster = BMediaRoster::CurrentRoster();
	if (roster != NULL && fAudioMixerNode != NULL)
		roster->ReleaseNode(*fAudioMixerNode);
}


void
MixerControl::SetVolume(float volume)
{
	if (fMixerParameter == NULL)
		return;

	if (volume < fMin)
		volume = fMin;
	else if (volume > fMax)
		volume = fMax;

	if (volume != _GetVolume())
		fMixerParameter->SetValue(&volume, sizeof(float), system_time());
}


void
MixerControl::ChangeVolumeBy(float value)
{
	if (fMixerParameter == NULL || value == 0.0f)
		return;

	float volume = _GetVolume();
	SetVolume(volume + value);
}


float
MixerControl::_GetVolume()
{
	if (fMixerParameter == NULL)
		return 0.0f;

	float volume = 0;
	bigtime_t lastChange;
	size_t size = sizeof(float);
	fMixerParameter->GetValue(&volume, &size, &lastChange);

	return volume;
}

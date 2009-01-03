/*
 * Copyright 2007-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "driver.h"
#include "hda_codec_defs.h"


static const char* kPortConnector[] = {
	"Jack", "None", "Fixed", "Dual"
};

static const char* kDefaultDevice[] = {
	"Line Out", "Speaker", "HP Out", "CD", "SPDIF out", "Digital Other Out",
	"Modem Line Side", "Modem Hand Side", "Line In", "AUX", "Mic In",
	"Telephony", "SPDIF In", "Digital Other In", "Reserved", "Other"
};

static const char* kConnectionType[] = {
	"N/A", "1/8\"", "1/4\"", "ATAPI internal", "RCA", "Optical",
	"Other Digital", "Other Analog", "Multichannel Analog (DIN)",
	"XLR/Professional", "RJ-11 (Modem)", "Combination", "-", "-", "-", "Other"
};

static const char* kJackColor[] = {
	"N/A", "Black", "Grey", "Blue", "Green", "Red", "Orange", "Yellow",
	"Purple", "Pink", "-", "-", "-", "-", "White", "Other"
};


static const char*
get_widget_type_name(hda_widget_type type)
{
	switch (type) {
		case WT_AUDIO_OUTPUT:
			return "Audio Output";
		case WT_AUDIO_INPUT:
			return "Audio Input";
		case WT_AUDIO_MIXER:
			return "Audio Mixer";
		case WT_AUDIO_SELECTOR:
			return "Audio Selector";
		case WT_PIN_COMPLEX:
			return "Pin Complex";
		case WT_POWER:
			return "Power";
		case WT_VOLUME_KNOB:
			return "Volume Knob";
		case WT_BEEP_GENERATOR:
			return "Beep Generator";
		case WT_VENDOR_DEFINED:
			return "Vendor Defined";
		default:
			return "Unknown";
	}
}


static void
dump_widget_audio_capabilities(uint32 capabilities)
{
	static const struct {
		uint32		flag;
		const char*	name;
	} kFlags[] = {
		{AUDIO_CAP_LEFT_RIGHT_SWAP, "L-R Swap"},
		{AUDIO_CAP_POWER_CONTROL, "Power"},
		{AUDIO_CAP_DIGITAL, "Digital"},
		{AUDIO_CAP_CONNECTION_LIST, "Conn. List"},
		{AUDIO_CAP_UNSOLICITED_RESPONSES, "Unsol. Responses"},
		{AUDIO_CAP_PROCESSING_CONTROLS, "Proc Widget"},
		{AUDIO_CAP_STRIPE, "Stripe"},
		{AUDIO_CAP_FORMAT_OVERRIDE, "Format Override"},
		{AUDIO_CAP_AMPLIFIER_OVERRIDE, "Amplifier Override"},
		{AUDIO_CAP_OUTPUT_AMPLIFIER, "Out Amplifier"},
		{AUDIO_CAP_INPUT_AMPLIFIER, "In Amplifier"},
		{AUDIO_CAP_STEREO, "Stereo"},			
	};

	char buffer[256];
	int offset = 0;

	for (uint32 j = 0; j < sizeof(kFlags) / sizeof(kFlags[0]); j++) {
		if (capabilities & kFlags[j].flag)
			offset += sprintf(buffer + offset, "[%s] ", kFlags[j].name);
	}

	if (offset != 0)
		dprintf("\t%s\n", buffer);
}


static void
dump_widget_inputs(hda_widget& widget)
{
	// dump connections

	char buffer[256];
	int offset = 0;

	for (uint32 i = 0; i < widget.num_inputs; i++) {
		int32 input = widget.inputs[i];

		if ((int32)i != widget.active_input)
			offset += sprintf(buffer + offset, "%ld ", input);
		else
			offset += sprintf(buffer + offset, "<%ld> ", input);
	}

	if (offset != 0)
		dprintf("\tInputs: %s\n", buffer);
}


static void
dump_widget_amplifier_capabilities(hda_widget& widget, bool input)
{
	uint32 capabilities;
	if (input)
		capabilities = widget.capabilities.input_amplifier;
	else
		capabilities = widget.capabilities.output_amplifier;

	if (capabilities == 0)
		return;

	dprintf("\t%s Amp: %sstep size: %ld dB, # steps: %ld, offset to 0 dB: "
		"%ld\n", input ? "In" : "Out",
		(capabilities & AMP_CAP_MUTE) != 0 ? "supports mute, " : "",
		(((capabilities & AMP_CAP_STEP_SIZE_MASK)
			>> AMP_CAP_STEP_SIZE_SHIFT) + 1) / 4,
		(capabilities & AMP_CAP_NUM_STEPS_MASK) >> AMP_CAP_NUM_STEPS_SHIFT,
		capabilities & AMP_CAP_OFFSET_MASK);
}


static void
dump_widget_pm_support(hda_widget& widget)
{
	dprintf("\tSupported power states: %s%s%s%s\n",
		widget.pm & POWER_STATE_D0 ? "D0 " : "",
		widget.pm & POWER_STATE_D1 ? "D1 " : "",
		widget.pm & POWER_STATE_D2 ? "D2 " : "",
		widget.pm & POWER_STATE_D3 ? "D3 " : "");
}


static void
dump_audiogroup_widgets(hda_audio_group* audioGroup)
{
	dprintf("\tAudiogroup:\n");
	/* Iterate over all Widgets and collect info */
	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		hda_widget& widget = audioGroup->widgets[i];
		uint32 nodeID = audioGroup->widget_start + i;

		dprintf("%ld: %s\n", nodeID, get_widget_type_name(widget.type));

		switch (widget.type) {
			case WT_AUDIO_OUTPUT:
			case WT_AUDIO_INPUT:
				break;

			case WT_PIN_COMPLEX:
				dprintf("\t%s%s%s\n", PIN_CAP_IS_INPUT(widget.d.pin.capabilities) ? "[Input] " : "",
					PIN_CAP_IS_OUTPUT(widget.d.pin.capabilities) ? "[Output]" : "",
					PIN_CAP_IS_EAPD_CAP(widget.d.pin.capabilities) ? "[EAPD Cap]" : "");
				
				break;

			default:
				break;
		}

		dump_widget_pm_support(widget);
		dump_widget_audio_capabilities(widget.capabilities.audio);
		dump_widget_amplifier_capabilities(widget, true);
		dump_widget_amplifier_capabilities(widget, false);
		dump_widget_inputs(widget);
	}
}


//	#pragma mark -


static status_t
hda_get_pm_support(hda_codec* codec, uint32 nodeID, uint32* pm)
{
	corb_t verb = MAKE_VERB(codec->addr, nodeID, VID_GET_PARAMETER,
		PID_POWERSTATE_SUPPORT);
	status_t rc;
	uint32 response;

	if ((rc = hda_send_verbs(codec, &verb, &response, 1)) == B_OK) {
		*pm = response & 0xf;
	}

	return rc;
}


static status_t
hda_get_stream_support(hda_codec* codec, uint32 nodeID, uint32* formats,
	uint32* rates)
{
	corb_t verbs[2];
	uint32 resp[2];
	status_t status;

	verbs[0] = MAKE_VERB(codec->addr, nodeID, VID_GET_PARAMETER,
		PID_STREAM_SUPPORT);
	verbs[1] = MAKE_VERB(codec->addr, nodeID, VID_GET_PARAMETER,
		PID_PCM_SUPPORT);

	status = hda_send_verbs(codec, verbs, resp, 2);
	if (status != B_OK)
		return status;

	*formats = 0;
	*rates = 0;

	if ((resp[0] & (STREAM_FLOAT | STREAM_PCM)) != 0) {
		if (resp[1] & (1 << 0))
			*rates |= B_SR_8000;
		if (resp[1] & (1 << 1))
			*rates |= B_SR_11025;
		if (resp[1] & (1 << 2))
			*rates |= B_SR_16000;
		if (resp[1] & (1 << 3))
			*rates |= B_SR_22050;
		if (resp[1] & (1 << 4))
			*rates |= B_SR_32000;
		if (resp[1] & (1 << 5))
			*rates |= B_SR_44100;
		if (resp[1] & (1 << 6))
			*rates |= B_SR_48000;
		if (resp[1] & (1 << 7))
			*rates |= B_SR_88200;
		if (resp[1] & (1 << 8))
			*rates |= B_SR_96000;
		if (resp[1] & (1 << 9))
			*rates |= B_SR_176400;
		if (resp[1] & (1 << 10))
			*rates |= B_SR_192000;
		if (resp[1] & (1 << 11))
			*rates |= B_SR_384000;

		if (resp[1] & PCM_8_BIT)
			*formats |= B_FMT_8BIT_S;
		if (resp[1] & PCM_16_BIT)
			*formats |= B_FMT_16BIT;
		if (resp[1] & PCM_20_BIT)
			*formats |= B_FMT_20BIT;
		if (resp[1] & PCM_24_BIT)
			*formats |= B_FMT_24BIT;
		if (resp[1] & PCM_32_BIT)
			*formats |= B_FMT_32BIT;
	}
	if ((resp[0] & STREAM_FLOAT) != 0)
		*formats |= B_FMT_FLOAT;
	if ((resp[0] & STREAM_AC3) != 0) {
		*formats |= B_FMT_BITSTREAM;
	}

	return B_OK;
}


//	#pragma mark - widget functions


static status_t
hda_widget_get_pm_support(hda_audio_group* audioGroup, hda_widget* widget)
{
	return hda_get_pm_support(audioGroup->codec, widget->node_id,
		&widget->pm);
}


static status_t
hda_widget_get_stream_support(hda_audio_group* audioGroup, hda_widget* widget)
{
	if (audioGroup->widget.node_id != widget->node_id
		&& (widget->capabilities.audio & AUDIO_CAP_FORMAT_OVERRIDE) == 0) {
		// adopt capabilities of the audio group
		widget->d.io.formats = audioGroup->widget.d.io.formats;
		widget->d.io.rates = audioGroup->widget.d.io.rates;
		return B_OK;
	}

	return hda_get_stream_support(audioGroup->codec, widget->node_id,
		&widget->d.io.formats, &widget->d.io.rates);
}


static status_t
hda_widget_get_amplifier_capabilities(hda_audio_group* audioGroup,
	hda_widget* widget)
{
	uint32 response;
	corb_t verb;

	if ((widget->capabilities.audio & AUDIO_CAP_OUTPUT_AMPLIFIER) != 0
		|| audioGroup->widget.node_id == widget->node_id) {
		if ((widget->capabilities.audio & AUDIO_CAP_AMPLIFIER_OVERRIDE) != 0
			|| audioGroup->widget.node_id == widget->node_id) {
			verb = MAKE_VERB(audioGroup->codec->addr, widget->node_id,
				VID_GET_PARAMETER, PID_OUTPUT_AMPLIFIER_CAP);
			status_t status = hda_send_verbs(audioGroup->codec, &verb,
				&response, 1);
			if (status < B_OK)
				return status;

			widget->capabilities.output_amplifier = response;
		} else {
			// adopt capabilities from the audio function group
			widget->capabilities.output_amplifier
				= audioGroup->widget.capabilities.output_amplifier;
		}
	}

	if ((widget->capabilities.audio & AUDIO_CAP_INPUT_AMPLIFIER) != 0
		|| audioGroup->widget.node_id == widget->node_id) {
		if ((widget->capabilities.audio & AUDIO_CAP_AMPLIFIER_OVERRIDE
			|| audioGroup->widget.node_id == widget->node_id) != 0) {
			verb = MAKE_VERB(audioGroup->codec->addr, widget->node_id,
				VID_GET_PARAMETER, PID_INPUT_AMPLIFIER_CAP);
			status_t status = hda_send_verbs(audioGroup->codec, &verb,
				&response, 1);
			if (status < B_OK)
				return status;

			widget->capabilities.input_amplifier = response;
		} else {
			// adopt capabilities from the audio function group
			widget->capabilities.input_amplifier
				= audioGroup->widget.capabilities.input_amplifier;
		}
	}

	return B_OK;
}


hda_widget*
hda_audio_group_get_widget(hda_audio_group* audioGroup, uint32 nodeID)
{
	if (audioGroup->widget_start > nodeID
		|| audioGroup->widget_start + audioGroup->widget_count < nodeID)
		return NULL;

	return &audioGroup->widgets[nodeID - audioGroup->widget_start];
}


static status_t
hda_widget_get_connections(hda_audio_group* audioGroup, hda_widget* widget)
{
	uint32 verb = MAKE_VERB(audioGroup->codec->addr, widget->node_id,
		VID_GET_PARAMETER, PID_CONNECTION_LIST_LENGTH);
	uint32 response;

	if (hda_send_verbs(audioGroup->codec, &verb, &response, 1) != B_OK)
		return B_ERROR;

	uint32 listEntries = response & 0x7f;
	bool longForm = (response & 0xf0) != 0;

	if (listEntries == 0)
		return B_OK;

#if 1
	if (widget->num_inputs > 1) {
		// Get currently active connection
		verb = MAKE_VERB(audioGroup->codec->addr, widget->node_id,
			VID_GET_CONNECTION_SELECT, 0);
		if (hda_send_verbs(audioGroup->codec, &verb, &response, 1) == B_OK)
			widget->active_input = response & 0xff;
	}
#endif

	uint32 valuesPerEntry = longForm ? 2 : 4;
	uint32 shift = 32 / valuesPerEntry;
	uint32 rangeMask = (1 << (shift - 1));
	int32 previousInput = -1;
	uint32 numInputs = 0;

	for (uint32 i = 0; i < listEntries; i++) {
		if ((i % valuesPerEntry) == 0) {
			// We get 2 or 4 answers per call depending on if we're
			// in short or long list mode
			verb = MAKE_VERB(audioGroup->codec->addr, widget->node_id,
				VID_GET_CONNECTION_LIST_ENTRY, i);
			if (hda_send_verbs(audioGroup->codec, &verb, &response, 1)
					!= B_OK) {
				dprintf("hda: Error parsing inputs for widget %ld!\n",
					widget->node_id);
				break;
			}
		}

		int32 input = (response >> (shift * (i % valuesPerEntry)))
			& ((1 << shift) - 1);

		if (input & rangeMask) {
			// found range
			input &= ~rangeMask;

			if (input < previousInput || previousInput == -1) {
				dprintf("hda: invalid range from %ld to %ld\n", previousInput,
					input);
				continue;
			}

			for (int32 rangeInput = previousInput + 1; rangeInput <= input
					&& numInputs < MAX_INPUTS; rangeInput++) {
				widget->inputs[numInputs++] = rangeInput;
			}

			previousInput = -1;
		} else if (numInputs < MAX_INPUTS) {
			// standard value
			widget->inputs[numInputs++] = input;
			previousInput = input;
		}
	}

	widget->num_inputs = numInputs;

	if (widget->num_inputs == 1)
		widget->active_input = 0;

	return B_OK;
}


static status_t
hda_widget_get_associations(hda_audio_group* audioGroup)
{
	uint32 index = 0;
	for (uint32 i = 0; i < MAX_ASSOCIATIONS; i++) {
		for (uint32 j = 0; j < audioGroup->widget_count; j++) {
			hda_widget& widget = audioGroup->widgets[j];

			if (widget.type != WT_PIN_COMPLEX)
				continue;
			if (CONF_DEFAULT_ASSOCIATION(widget.d.pin.config) != i)
				continue;
			if (audioGroup->associations[index].pin_count == 0) {
				audioGroup->associations[index].index = index;
				audioGroup->associations[index].enabled = true;
			}
			uint32 sequence = CONF_DEFAULT_SEQUENCE(widget.d.pin.config);
			if (audioGroup->associations[index].pins[sequence] != 0) {
				audioGroup->associations[index].enabled = false;
			}
			audioGroup->associations[index].pins[sequence] = widget.node_id;
			audioGroup->associations[index].pin_count++;
			if (i == 15)
				index++;
		}
		if (i != 15 && audioGroup->associations[index].pin_count != 0)
			index++;
	}
	audioGroup->association_count = index;

	return B_OK;
}


//	#pragma mark - audio group functions


static status_t
hda_codec_parse_audio_group(hda_audio_group* audioGroup)
{
	corb_t verbs[3];
	uint32 resp[3];

	hda_widget_get_stream_support(audioGroup, &audioGroup->widget);
	hda_widget_get_pm_support(audioGroup, &audioGroup->widget);
	hda_widget_get_amplifier_capabilities(audioGroup, &audioGroup->widget);
	
	verbs[0] = MAKE_VERB(audioGroup->codec->addr, audioGroup->widget.node_id,
		VID_GET_PARAMETER, PID_AUDIO_GROUP_CAP);
	verbs[1] = MAKE_VERB(audioGroup->codec->addr, audioGroup->widget.node_id,
		VID_GET_PARAMETER, PID_GPIO_COUNT);
	verbs[2] = MAKE_VERB(audioGroup->codec->addr, audioGroup->widget.node_id,
		VID_GET_PARAMETER, PID_SUB_NODE_COUNT);

	if (hda_send_verbs(audioGroup->codec, verbs, resp, 3) != B_OK)
		return B_ERROR;

	dprintf("hda: Audio Group: Output delay: %ld samples, Input delay: %ld "
		"samples, Beep Generator: %s\n", AUDIO_GROUP_CAP_OUTPUT_DELAY(resp[0]),
		AUDIO_GROUP_CAP_INPUT_DELAY(resp[0]), 
		AUDIO_GROUP_CAP_BEEPGEN(resp[0]) ? "yes" : "no");

	dprintf("hda:   #GPIO: %ld, #GPO: %ld, #GPI: %ld, unsol: %s, wake: %s\n",
		GPIO_COUNT_NUM_GPIO(resp[1]), GPIO_COUNT_NUM_GPO(resp[1]),
		GPIO_COUNT_NUM_GPI(resp[1]), GPIO_COUNT_GPIUNSOL(resp[1]) ? "yes" : "no",
		GPIO_COUNT_GPIWAKE(resp[1]) ? "yes" : "no");

	audioGroup->widget_start = SUB_NODE_COUNT_START(resp[2]);
	audioGroup->widget_count = SUB_NODE_COUNT_TOTAL(resp[2]);

	dprintf("hda:   widget start %lu, count %lu\n", audioGroup->widget_start,
		audioGroup->widget_count);

	audioGroup->widgets = (hda_widget*)calloc(audioGroup->widget_count,
		sizeof(*audioGroup->widgets));
	if (audioGroup->widgets == NULL) {
		dprintf("ERROR: Not enough memory!\n");
		return B_NO_MEMORY;
	}

	/* Iterate over all Widgets and collect info */
	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		hda_widget& widget = audioGroup->widgets[i];
		uint32 nodeID = audioGroup->widget_start + i;
		uint32 capabilities;

		verbs[0] = MAKE_VERB(audioGroup->codec->addr, nodeID, VID_GET_PARAMETER,
			PID_AUDIO_WIDGET_CAP);
		if (hda_send_verbs(audioGroup->codec, verbs, &capabilities, 1) != B_OK)
			return B_ERROR;

		widget.type = (hda_widget_type)((capabilities & AUDIO_CAP_TYPE_MASK)
			>> AUDIO_CAP_TYPE_SHIFT);
		
		/* Check specific node ids declared as inputs as beepers */
		switch ((audioGroup->codec->vendor_id << 16) | audioGroup->codec->product_id) {
			case 0x11d41988:
			case 0x11d4198b:
				if (nodeID == 26)
					widget.type = WT_BEEP_GENERATOR;
				break;
			case 0x10ec0260:
				if (nodeID == 23)
					widget.type = WT_BEEP_GENERATOR;
				break;
			case 0x10ec0262:
			case 0x10ec0268:
			case 0x10ec0880:
			case 0x10ec0882:
			case 0x10ec0883:
			case 0x10ec0885:
			case 0x10ec0888:
			case 0x10ec0889:
				if (nodeID == 29)
					widget.type = WT_BEEP_GENERATOR;
				break;
		}
		widget.active_input = -1;
		widget.capabilities.audio = capabilities;
		widget.node_id = nodeID;

		if ((capabilities & AUDIO_CAP_POWER_CONTROL) != 0) {
			/* We support power; switch us on! */
			verbs[0] = MAKE_VERB(audioGroup->codec->addr, nodeID,
				VID_SET_POWER_STATE, 0);
			hda_send_verbs(audioGroup->codec, verbs, NULL, 1);

			snooze(1000);
		}
		if ((capabilities & (AUDIO_CAP_INPUT_AMPLIFIER
				| AUDIO_CAP_OUTPUT_AMPLIFIER)) != 0) {
			hda_widget_get_amplifier_capabilities(audioGroup, &widget);
		}

		dprintf("%ld: %s\n", nodeID, get_widget_type_name(widget.type));

		switch (widget.type) {
			case WT_AUDIO_OUTPUT:
			case WT_AUDIO_INPUT:
				hda_widget_get_stream_support(audioGroup, &widget);
				break;

			case WT_PIN_COMPLEX:
				verbs[0] = MAKE_VERB(audioGroup->codec->addr, nodeID,
					VID_GET_PARAMETER, PID_PIN_CAP);
				if (hda_send_verbs(audioGroup->codec, verbs, resp, 1) == B_OK) {
					widget.d.pin.capabilities = resp[0];
					
					dprintf("\t%s%s\n", PIN_CAP_IS_INPUT(resp[0]) ? "[Input] " : "",
						PIN_CAP_IS_OUTPUT(resp[0]) ? "[Output]" : "");
				} else {
					dprintf("%s: Error getting Pin Complex IO\n", __func__);
				}

				verbs[0] = MAKE_VERB(audioGroup->codec->addr, nodeID,
					VID_GET_CONFIGURATION_DEFAULT, 0);
				if (hda_send_verbs(audioGroup->codec, verbs, resp, 1) == B_OK) {
					widget.d.pin.config = resp[0];
					dprintf("\t%s, %s, %s, %s, Association:%ld\n",
						kPortConnector[CONF_DEFAULT_CONNECTIVITY(resp[0])],
						kDefaultDevice[CONF_DEFAULT_DEVICE(resp[0])],
						kConnectionType[CONF_DEFAULT_CONNTYPE(resp[0])],
						kJackColor[CONF_DEFAULT_COLOR(resp[0])],
						CONF_DEFAULT_ASSOCIATION(resp[0]));
				}
				break;

			default:
				break;
		}

		hda_widget_get_pm_support(audioGroup, &widget);
		hda_widget_get_connections(audioGroup, &widget);

		dump_widget_pm_support(widget);
		dump_widget_audio_capabilities(capabilities);
		dump_widget_amplifier_capabilities(widget, true);
		dump_widget_amplifier_capabilities(widget, false);
		dump_widget_inputs(widget);
	}

	hda_widget_get_associations(audioGroup);

	return B_OK;
}


/*! Find output path for widget */
static bool
hda_widget_find_output_path(hda_audio_group* audioGroup, hda_widget* widget,
	uint32 depth)
{
	if (widget == NULL || depth > 16)
		return false;

	switch (widget->type) {
		case WT_AUDIO_OUTPUT:
			widget->flags |= WIDGET_FLAG_OUTPUT_PATH;
dprintf("      %*soutput: added output widget %ld\n", (int)depth * 2, "", widget->node_id);
			return true;

		case WT_AUDIO_MIXER:
		case WT_AUDIO_SELECTOR:
		{
			// already used
			if (widget->flags & WIDGET_FLAG_OUTPUT_PATH)
				return false;
			
			// search for output in this path
			bool found = false;
			for (uint32 i = 0; i < widget->num_inputs; i++) {
				hda_widget* inputWidget = hda_audio_group_get_widget(audioGroup,
					widget->inputs[i]);

				if (hda_widget_find_output_path(audioGroup, inputWidget,
						depth + 1)) {
					if (widget->active_input == -1)
						widget->active_input = i;

					widget->flags |= WIDGET_FLAG_OUTPUT_PATH;
dprintf("      %*soutput: added mixer/selector widget %ld\n", (int)depth * 2, "", widget->node_id);
					found = true;
				}
			}
if (!found) dprintf("      %*soutput: not added mixer/selector widget %ld\n", (int)depth * 2, "", widget->node_id);
			return found;
		}

		default:
			return false;
	}
}


/*! Find input path for widget */
static bool
hda_widget_find_input_path(hda_audio_group* audioGroup, hda_widget* widget,
	uint32 depth)
{
	if (widget == NULL || depth > 16)
		return false;

	switch (widget->type) {
		case WT_PIN_COMPLEX:
			// already used
			if (widget->flags & WIDGET_FLAG_INPUT_PATH)
				return false;
			
			if (PIN_CAP_IS_INPUT(widget->d.pin.capabilities)) {
				switch (CONF_DEFAULT_DEVICE(widget->d.pin.config)) {
					case PIN_DEV_CD:
					case PIN_DEV_LINE_IN:
					case PIN_DEV_MIC_IN:
						widget->flags |= WIDGET_FLAG_INPUT_PATH;
dprintf("      %*sinput: added input widget %ld\n", (int)depth * 2, "", widget->node_id);
						return true;
					break;
				}
			}
			return false;
		case WT_AUDIO_INPUT:
		case WT_AUDIO_MIXER:
		case WT_AUDIO_SELECTOR:
		{
			// already used
			if (widget->flags & WIDGET_FLAG_INPUT_PATH)
				return false;
			
			// search for pin complex in this path
			bool found = false;
			for (uint32 i = 0; i < widget->num_inputs; i++) {
				hda_widget* inputWidget = hda_audio_group_get_widget(audioGroup,
					widget->inputs[i]);

				if (hda_widget_find_input_path(audioGroup, inputWidget,
						depth + 1)) {
					if (widget->active_input == -1)
						widget->active_input = i;

					widget->flags |= WIDGET_FLAG_INPUT_PATH;
dprintf("      %*sinput: added mixer/selector widget %ld\n", (int)depth * 2, "", widget->node_id);
					found = true;
				}
			}
if (!found) dprintf("      %*sinput: not added mixer/selector widget %ld\n", (int)depth * 2, "", widget->node_id);
			return found;
		}

		default:
			return false;
	}
}

static bool
hda_audio_group_build_output_tree(hda_audio_group* audioGroup, bool useMixer)
{
	bool found = false;

dprintf("build output tree: %suse mixer\n", useMixer ? "" : "don't ");
	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		hda_widget& widget = audioGroup->widgets[i];

		if (widget.type != WT_PIN_COMPLEX || !PIN_CAP_IS_OUTPUT(widget.d.pin.capabilities))
			continue;

		int device = CONF_DEFAULT_DEVICE(widget.d.pin.config);
		if (device != PIN_DEV_HEAD_PHONE_OUT
			&& device != PIN_DEV_SPEAKER
			&& device != PIN_DEV_LINE_OUT)
			continue;

dprintf("  look at pin widget %ld (%ld inputs)\n", widget.node_id, widget.num_inputs);
		for (uint32 j = 0; j < widget.num_inputs; j++) {
			hda_widget* inputWidget = hda_audio_group_get_widget(audioGroup,
				widget.inputs[j]);
dprintf("    try widget %ld: %p\n", widget.inputs[j], inputWidget);
			if (inputWidget == NULL)
				continue;

			if (useMixer && inputWidget->type != WT_AUDIO_MIXER
				&& inputWidget->type != WT_AUDIO_SELECTOR)
				continue;
dprintf("    widget %ld is candidate\n", inputWidget->node_id);

			if (hda_widget_find_output_path(audioGroup, inputWidget, 0)) {
dprintf("    add pin widget %ld\n", widget.node_id);
				if (widget.active_input == -1)
					widget.active_input = j;
				widget.flags |= WIDGET_FLAG_OUTPUT_PATH;
				found = true;
				break;
			}
		}
	}

	return found;
}


static bool
hda_audio_group_build_input_tree(hda_audio_group* audioGroup)
{
	bool found = false;

dprintf("build input tree\n");
	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		hda_widget& widget = audioGroup->widgets[i];

		if (widget.type != WT_AUDIO_INPUT)
			continue;

dprintf("  look at input widget %ld (%ld inputs)\n", widget.node_id, widget.num_inputs);
		for (uint32 j = 0; j < widget.num_inputs; j++) {
			hda_widget* inputWidget = hda_audio_group_get_widget(audioGroup,
				widget.inputs[j]);
dprintf("    try widget %ld: %p\n", widget.inputs[j], inputWidget);
			if (inputWidget == NULL)
				continue;
			
dprintf("    widget %ld is candidate\n", inputWidget->node_id);

			if (hda_widget_find_input_path(audioGroup, inputWidget, 0)) {
dprintf("    add pin widget %ld\n", widget.node_id);
				if (widget.active_input == -1)
					widget.active_input = j;
				widget.flags |= WIDGET_FLAG_INPUT_PATH;
				found = true;
				break;
			}
		}
	}

	return found;
}


static status_t
hda_audio_group_build_tree(hda_audio_group* audioGroup)
{
	if (!hda_audio_group_build_output_tree(audioGroup, true)) {
		// didn't find a mixer path, try again without
dprintf("try without mixer!\n");
		if (!hda_audio_group_build_output_tree(audioGroup, false))
			return ENODEV;
	}

	if (!hda_audio_group_build_input_tree(audioGroup)) {
		dprintf("hda: build input tree failed\n");
	}	

dprintf("build tree!\n");
	
	// select active connections

	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		hda_widget& widget = audioGroup->widgets[i];

		if (widget.active_input == -1)
			widget.active_input = 0;
		if (widget.num_inputs < 2)
			continue;

		corb_t verb = MAKE_VERB(audioGroup->codec->addr,
			widget.node_id, VID_SET_CONNECTION_SELECT, widget.active_input);
		if (hda_send_verbs(audioGroup->codec, &verb, NULL, 1) != B_OK) {
			dprintf("hda: Setting output selector %ld failed on widget %ld!\n",
				widget.active_input, widget.node_id);
		}
	}

	dump_audiogroup_widgets(audioGroup);

	return B_OK;
}


static void
hda_codec_delete_audio_group(hda_audio_group* audioGroup)
{
	if (audioGroup == NULL)
		return;

	if (audioGroup->playback_stream != NULL)
		hda_stream_delete(audioGroup->playback_stream);

	if (audioGroup->record_stream != NULL)
		hda_stream_delete(audioGroup->record_stream);
	free(audioGroup->multi);
	free(audioGroup->widgets);
	free(audioGroup);
}


static status_t
hda_codec_new_audio_group(hda_codec* codec, uint32 audioGroupNodeID)
{
	hda_audio_group* audioGroup = (hda_audio_group*)calloc(1,
		sizeof(hda_audio_group));
	if (audioGroup == NULL)
		return B_NO_MEMORY;

	/* Setup minimal info needed by hda_codec_parse_afg */
	audioGroup->widget.node_id = audioGroupNodeID;
	audioGroup->codec = codec;
	audioGroup->multi = (hda_multi*)calloc(1,
		sizeof(hda_multi));
	if (audioGroup->multi == NULL)
		return B_NO_MEMORY;
	audioGroup->multi->group = audioGroup;

	/* Parse all widgets in Audio Function Group */
	status_t status = hda_codec_parse_audio_group(audioGroup);
	if (status != B_OK)
		goto err;

	/* Setup for worst-case scenario; we cannot find any output Pin Widgets */
	status = ENODEV;

	if (hda_audio_group_build_tree(audioGroup) != B_OK)
		goto err;

	audioGroup->playback_stream = hda_stream_new(audioGroup, STREAM_PLAYBACK);
	audioGroup->record_stream = hda_stream_new(audioGroup, STREAM_RECORD);
	dprintf("hda: streams playback %p, record %p\n", audioGroup->playback_stream,
		audioGroup->record_stream);

	if (audioGroup->playback_stream != NULL
		|| audioGroup->record_stream != NULL) {
		codec->audio_groups[codec->num_audio_groups++] = audioGroup;
		return B_OK;
	}

err:
	free(audioGroup->widgets);
	free(audioGroup);
	return status;
}


//	#pragma mark -


status_t
hda_audio_group_get_widgets(hda_audio_group* audioGroup, hda_stream* stream)
{
	hda_widget_type type;
	uint32 flags;

	if (stream->type == STREAM_PLAYBACK) {
		type = WT_AUDIO_OUTPUT;
		flags = WIDGET_FLAG_OUTPUT_PATH;
	} else {
		// record
		type = WT_AUDIO_INPUT;
		flags = WIDGET_FLAG_INPUT_PATH;
	}

	uint32 count = 0;

	for (uint32 i = 0; i < audioGroup->widget_count && count < MAX_IO_WIDGETS;
			i++) {
		hda_widget& widget = audioGroup->widgets[i];

		if ((widget.flags & flags) != 0) {
			if (widget.type == WT_PIN_COMPLEX) {
				stream->pin_widget = widget.node_id;

dprintf("ENABLE pin widget %ld\n", widget.node_id);
				/* FIXME: Force Pin Widget to unmute; enable hp/output */
				uint32 verb = MAKE_VERB(audioGroup->codec->addr,
					widget.node_id,
					VID_SET_PIN_WIDGET_CONTROL,
					flags == WIDGET_FLAG_OUTPUT_PATH ? 
						PIN_ENABLE_HEAD_PHONE | PIN_ENABLE_OUTPUT
						: PIN_ENABLE_INPUT);
				hda_send_verbs(audioGroup->codec, &verb, NULL, 1);

				if (PIN_CAP_IS_EAPD_CAP(widget.d.pin.capabilities)) {
					uint32 result;
					verb = MAKE_VERB(audioGroup->codec->addr, 
						widget.node_id, VID_GET_EAPDBTL_EN, 0);
					if (hda_send_verbs(audioGroup->codec, &verb, 
						&result, 1) == B_OK) {
						result &= 0xff;
						verb = MAKE_VERB(audioGroup->codec->addr, 
							widget.node_id, VID_SET_EAPDBTL_EN,
							result | EAPDBTL_ENABLE_EAPD);
						hda_send_verbs(audioGroup->codec, 
							&verb, NULL, 1);
dprintf("ENABLE EAPD pin widget %ld\n", widget.node_id);
					}
				}				
			}

			if (widget.capabilities.output_amplifier != 0) {
dprintf("UNMUTE/SET OUTPUT GAIN widget %ld (offset %ld)\n", widget.node_id,
	widget.capabilities.output_amplifier & AMP_CAP_OFFSET_MASK);
				uint32 verb = MAKE_VERB(audioGroup->codec->addr,
					widget.node_id,
					VID_SET_AMPLIFIER_GAIN_MUTE,
					AMP_SET_OUTPUT | AMP_SET_LEFT_CHANNEL
						| AMP_SET_RIGHT_CHANNEL
						| (widget.capabilities.output_amplifier
							& AMP_CAP_OFFSET_MASK));
				hda_send_verbs(audioGroup->codec, &verb, NULL, 1);
			}
			if (widget.capabilities.input_amplifier != 0) {
dprintf("UNMUTE/SET INPUT GAIN widget %ld (offset %ld)\n", widget.node_id,
	widget.capabilities.input_amplifier & AMP_CAP_OFFSET_MASK);
				for (uint32 i = 0; i < widget.num_inputs; i++) {
					uint32 verb = MAKE_VERB(audioGroup->codec->addr,
						widget.node_id,
						VID_SET_AMPLIFIER_GAIN_MUTE,
						AMP_SET_INPUT | AMP_SET_LEFT_CHANNEL
							| AMP_SET_RIGHT_CHANNEL
							| AMP_SET_INPUT_INDEX(i)
							| (widget.capabilities.input_amplifier
								& AMP_CAP_OFFSET_MASK));
					hda_send_verbs(audioGroup->codec, &verb, NULL, 1);
				}
			}
		}

		if (widget.type != type || (widget.flags & flags) == 0
			|| (widget.capabilities.audio
				& (AUDIO_CAP_STEREO | AUDIO_CAP_DIGITAL)) != AUDIO_CAP_STEREO
			|| widget.d.io.formats == 0)
			continue;

		if (count == 0) {
			stream->sample_format = widget.d.io.formats;
			stream->sample_rate = widget.d.io.rates;
		} else {
			stream->sample_format &= widget.d.io.formats;
			stream->sample_rate &= widget.d.io.rates;
		}

		stream->io_widgets[count++] = widget.node_id;
	}

	if (count == 0)
		return B_ENTRY_NOT_FOUND;

	stream->num_io_widgets = count;
	return B_OK;
}


void
hda_codec_delete(hda_codec* codec)
{
	if (codec == NULL)
		return;

	delete_sem(codec->response_sem);

	for (uint32 i = 0; i < codec->num_audio_groups; i++) {
		hda_codec_delete_audio_group(codec->audio_groups[i]);
		codec->audio_groups[i] = NULL;
	}

	free(codec);
}


hda_codec*
hda_codec_new(hda_controller* controller, uint32 codecAddress)
{
	if (codecAddress > HDA_MAX_CODECS)
		return NULL;

	hda_codec* codec = (hda_codec*)calloc(1, sizeof(hda_codec));
	if (codec == NULL) {
		dprintf("hda: Failed to alloc a codec\n");
		return NULL;
	}

	codec->controller = controller;
	codec->addr = codecAddress;
	codec->response_sem = create_sem(0, "hda_codec_response_sem");
	controller->codecs[codecAddress] = codec;

	struct {
		uint32 device : 16;
		uint32 vendor : 16;
		uint32 stepping : 8;
		uint32 revision : 8;
		uint32 minor : 4;
		uint32 major : 4;
		uint32 _reserved0 : 8;
		uint32 count : 8;
		uint32 _reserved1 : 8;
		uint32 start : 8;
		uint32 _reserved2 : 8;
	} response;
	corb_t verbs[3];

	verbs[0] = MAKE_VERB(codecAddress, 0, VID_GET_PARAMETER, PID_VENDOR_ID);
	verbs[1] = MAKE_VERB(codecAddress, 0, VID_GET_PARAMETER, PID_REVISION_ID);
	verbs[2] = MAKE_VERB(codecAddress, 0, VID_GET_PARAMETER, 
		PID_SUB_NODE_COUNT);

	if (hda_send_verbs(codec, verbs, (uint32*)&response, 3) != B_OK) {
		dprintf("hda: Failed to get vendor and revision parameters\n");
		goto err;
	}

	codec->vendor_id = response.vendor;
	codec->product_id = response.device;
	codec->stepping = response.stepping;
	codec->revision = response.revision;
	codec->minor = response.minor;
	codec->major = response.major;

	dprintf("Codec %ld Vendor: %04lx Product: %04lx, Revision: "
		"%lu.%lu.%lu.%lu\n", codecAddress, response.vendor, response.device,
		response.major, response.minor, response.revision, response.stepping);

	for (uint32 nodeID = response.start; nodeID < response.start
			+ response.count; nodeID++) {
		uint32 groupType;
		verbs[0] = MAKE_VERB(codecAddress, nodeID, VID_GET_PARAMETER,
			PID_FUNCTION_GROUP_TYPE);

		if (hda_send_verbs(codec, verbs, &groupType, 1) != B_OK) {
			dprintf("hda: Failed to get function group type\n");
			goto err;
		}

		if ((groupType & FUNCTION_GROUP_NODETYPE_MASK) == FUNCTION_GROUP_NODETYPE_AUDIO) {
			/* Found an Audio Function Group! */
			status_t status = hda_codec_new_audio_group(codec, nodeID);
			if (status != B_OK) {
				dprintf("hda: Failed to setup new audio function group (%s)!\n",
					strerror(status));
				goto err;
			}
		}
	}

	return codec;

err:
	controller->codecs[codecAddress] = NULL;
	hda_codec_delete(codec);
	return NULL;
}

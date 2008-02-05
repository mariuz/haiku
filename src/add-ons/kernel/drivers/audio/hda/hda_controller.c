/*
 * Copyright 2007-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 */


#include "driver.h"
#include "hda_controller_defs.h"
#include "hda_codec_defs.h"


void
hda_stream_delete(hda_stream* stream)
{
	if (stream->buffer_ready_sem >= B_OK)
		delete_sem(stream->buffer_ready_sem);

	if (stream->buffer_area >= B_OK)
		delete_area(stream->buffer_area);

	if (stream->bdl_area >= B_OK)
		delete_area(stream->bdl_area);

	free(stream);
}


hda_stream*
hda_stream_new(hda_controller* controller, int type)
{
	hda_stream* stream = calloc(1, sizeof(hda_stream));
	if (stream == NULL)
		return NULL;

	stream->buffer_area = B_ERROR;
	stream->bdl_area = B_ERROR;

	switch (type) {
		case STRM_PLAYBACK:
			stream->buffer_ready_sem = create_sem(0, "hda_playback_sem");
			stream->id = 1;
			stream->off = (controller->num_input_streams * HDAC_SDSIZE);
			controller->streams[controller->num_input_streams] = stream;
			break;
			
		case STRM_RECORD:
			stream->buffer_area = B_ERROR;
			stream->bdl_area = B_ERROR;
			stream->buffer_ready_sem = create_sem(0, "hda_record_sem");
			stream->id = 2;
			stream->off = 0;
			controller->streams[0] = stream;
			break;

		default:
			dprintf("%s: Unknown stream type %d!\n", __func__, type);
			free(stream);
			stream = NULL;
			break;
	}

	return stream;
}


status_t
hda_stream_start(hda_controller* controller, hda_stream* stream)
{
	OREG8(controller, stream->off, CTL0) |= CTL0_RUN;

	while (!(OREG8(controller, stream->off, CTL0) & CTL0_RUN))
		snooze(1);

	stream->running = true;

	return B_OK;
}


status_t
hda_stream_check_intr(hda_controller* controller, hda_stream* stream)
{
	if (stream->running) {
		uint8 sts = OREG8(controller, stream->off, STS);
		if (sts) {
			cpu_status status;

			OREG8(controller, stream->off, STS) = sts;
			status = disable_interrupts();
			acquire_spinlock(&stream->lock);

			stream->real_time = system_time();
			stream->frames_count += stream->buffer_length;
			stream->buffer_cycle = (stream->buffer_cycle + 1)
				% stream->num_buffers;

			release_spinlock(&stream->lock);
			restore_interrupts(status);

			release_sem_etc(stream->buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);
		}
	}

	return B_OK;
}


status_t
hda_stream_stop(hda_controller* controller, hda_stream* stream)
{
	OREG8(controller, stream->off, CTL0) &= ~CTL0_RUN;

	while ((OREG8(controller, stream->off, CTL0) & CTL0_RUN) != 0)
		snooze(1);

	stream->running = false;

	return B_OK;
}


status_t
hda_stream_setup_buffers(hda_afg* afg, hda_stream* stream, const char* desc)
{
	uint32 bufferSize, bufferPhysicalAddress, alloc;
	uint32 response[2], index;
	physical_entry pe;
	bdl_entry_t* bdl;
	corb_t verb[2];
	uint8* buffer;
	status_t rc;
	uint16 wfmt;

	/* Clear previously allocated memory */
	if (stream->buffer_area >= B_OK) {
		delete_area(stream->buffer_area);
		stream->buffer_area = B_ERROR;
	}

	if (stream->bdl_area >= B_OK) {
		delete_area(stream->bdl_area);
		stream->bdl_area = B_ERROR;
	}

	/* Calculate size of buffer (aligned to 128 bytes) */		
	bufferSize = stream->sample_size * stream->num_channels
		* stream->buffer_length;
	bufferSize = (bufferSize + 127) & (~127);
	
	/* Calculate total size of all buffers (aligned to size of B_PAGE_SIZE) */
	alloc = bufferSize * stream->num_buffers;
	alloc = (alloc + B_PAGE_SIZE - 1) & (~(B_PAGE_SIZE -1));

	/* Allocate memory for buffers */
	stream->buffer_area = create_area("hda_buffers", (void**)&buffer,
		B_ANY_KERNEL_ADDRESS, alloc, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (stream->buffer_area < B_OK)
		return stream->buffer_area;

	/* Get the physical address of memory */
	rc = get_memory_map(buffer, alloc, &pe, 1);
	if (rc != B_OK) {
		delete_area(stream->buffer_area);
		return rc;
	}
	
	bufferPhysicalAddress = (uint32)pe.address;

	dprintf("%s(%s): Allocated %lu bytes for %ld buffers\n", __func__, desc,
		alloc, stream->num_buffers);

	/* Store pointers (both virtual/physical) */	
	for (index = 0; index < stream->num_buffers; index++) {
		stream->buffers[index] = buffer + (index * bufferSize);
		stream->buffers_pa[index] = bufferPhysicalAddress + (index * bufferSize);
	}

	/* Now allocate BDL for buffer range */
	alloc = stream->num_buffers * sizeof(bdl_entry_t);
	alloc = (alloc + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	stream->bdl_area = create_area("hda_bdl", (void**)&bdl,
		B_ANY_KERNEL_ADDRESS, alloc, B_CONTIGUOUS, 0);
	if (stream->bdl_area < B_OK) {
		delete_area(stream->buffer_area);
		return stream->bdl_area;
	}

	/* Get the physical address of memory */
	rc = get_memory_map(bdl, alloc, &pe, 1);
	if (rc != B_OK) {
		delete_area(stream->buffer_area);
		delete_area(stream->bdl_area);
		return rc;
	}

	stream->bdl_pa = (uint32)pe.address;

	dprintf("%s(%s): Allocated %ld bytes for %ld BDLEs\n", __func__, desc,
		alloc, stream->num_buffers);

	/* Setup BDL entries */	
	for (index = 0; index < stream->num_buffers; index++, bdl++) {
		bdl->address = stream->buffers_pa[index];
		bdl->length = bufferSize;
		bdl->ioc = 1;
	}

	/* Configure stream registers */
	wfmt = stream->num_channels -1;
	switch (stream->sampleformat) {
		case B_FMT_8BIT_S:	wfmt |= (0 << 4); stream->bps = 8; break;
		case B_FMT_16BIT:	wfmt |= (1 << 4); stream->bps = 16; break;
		case B_FMT_24BIT:	wfmt |= (3 << 4); stream->bps = 24; break;
		case B_FMT_32BIT:	wfmt |= (4 << 4); stream->bps = 32; break;

		default:
			dprintf("%s: Invalid sample format: 0x%lx\n", __func__,
				stream->sampleformat);
			break;
	}

	switch (stream->samplerate) {
		case B_SR_8000:
			wfmt |= (0 << 14) | (0 << 11) | (5 << 8);
			stream->rate = 8000;
			break;
		case B_SR_11025:
			wfmt |= (1 << 14) | (0 << 11) | (3 << 8);
			stream->rate = 11025;
			break;
		case B_SR_16000:
			wfmt |= (0 << 14) | (0 << 11) | (2 << 8);
			stream->rate = 16000;
			break;
		case B_SR_22050:
			wfmt |= (1 << 14) | (0 << 11) | (1 << 8);
			stream->rate = 22050;
			break;
		case B_SR_32000:
			wfmt |= (0 << 14) | (1 << 11) | (2 << 8);
			stream->rate = 32000;
			break;
		case B_SR_44100:
			wfmt |= (1 << 14) | (0 << 11) | (0 << 8);
			stream->rate = 44100;
			break;
		case B_SR_48000:
			wfmt |= (0 << 14) | (0 << 11) | (0 << 8);
			stream->rate = 48000;
			break;
		case B_SR_88200:
			wfmt |= (1 << 14) | (1 << 11) | (0 << 8);
			stream->rate = 88200;
			break;
		case B_SR_96000:
			wfmt |= (0 << 14) | (2 << 11) | (0 << 8);
			stream->rate = 96000;
			break;
		case B_SR_176400:
			wfmt |= (1 << 14) | (3 << 11) | (0 << 8);
			stream->rate = 176400;
			break;
		case B_SR_192000:
			wfmt |= (0 << 14) | (3 << 11) | (0 << 8);
			stream->rate = 192000;
			break;

		default:
			dprintf("%s: Invalid sample rate: 0x%lx\n", __func__,
				stream->samplerate);
			break;
	}

	dprintf("IRA: %s: setup stream %ld: SR=%ld, SF=%ld\n", __func__, stream->id,
		stream->rate, stream->bps);

	OREG16(afg->codec->controller, stream->off, FMT) = wfmt;
	OREG32(afg->codec->controller, stream->off, BDPL) = stream->bdl_pa;
	OREG32(afg->codec->controller, stream->off, BDPU) = 0;
	OREG16(afg->codec->controller, stream->off, LVI) = stream->num_buffers -1;
	/* total cyclic buffer size in _bytes_ */
	OREG32(afg->codec->controller, stream->off, CBL) = stream->sample_size
		* stream->num_channels * stream->num_buffers * stream->buffer_length;
	OREG8(afg->codec->controller, stream->off, CTL0)
		= CTL0_IOCE | CTL0_FEIE | CTL0_DEIE;
	OREG8(afg->codec->controller, stream->off, CTL2) = stream->id << 4;

	verb[0] = MAKE_VERB(afg->codec->addr, stream->io_wid, VID_SET_CONVFORMAT,
		wfmt);
	verb[1] = MAKE_VERB(afg->codec->addr, stream->io_wid, VID_SET_CVTSTRCHN,
		stream->id << 4);
	rc = hda_send_verbs(afg->codec, verb, response, 2);

	return rc;
}


//	#pragma mark -


status_t
hda_send_verbs(hda_codec* codec, corb_t* verbs, uint32* responses, int count)
{
	corb_t* corb = codec->controller->corb;
	status_t rc;

	codec->response_count = 0;
	memcpy(corb + (codec->controller->corbwp + 1), verbs,
		sizeof(corb_t) * count);
	REG16(codec->controller, CORBWP) = (codec->controller->corbwp += count);

	rc = acquire_sem_etc(codec->response_sem, count, /*B_CAN_INTERRUPT | */
		B_RELATIVE_TIMEOUT, 1000ULL * 50);
	if (rc == B_OK && responses != NULL)
		memcpy(responses, codec->responses, count * sizeof(uint32));

	return rc;
}


static int32
hda_interrupt_handler(hda_controller* controller)
{
	int32 rc = B_HANDLED_INTERRUPT;

	/* Check if this interrupt is ours */
	uint32 intsts = REG32(controller, INTSTS);
	if ((intsts & INTSTS_GIS) == 0)
		return B_UNHANDLED_INTERRUPT;

	/* Controller or stream related? */
	if (intsts & INTSTS_CIS) {
		uint32 statests = REG16(controller, STATESTS);
		uint8 rirbsts = REG8(controller, RIRBSTS);
		uint8 corbsts = REG8(controller, CORBSTS);

		if (statests) {
			/* Detected Codec state change */
			REG16(controller, STATESTS) = statests;
			controller->codecsts = statests;
		}

		/* Check for incoming responses */			
		if (rirbsts) {
			REG8(controller, RIRBSTS) = rirbsts;

			if (rirbsts & RIRBSTS_RINTFL) {
				uint16 rirbwp = REG16(controller, RIRBWP);
				while (controller->rirbrp <= rirbwp) {
					uint32 resp_ex
						= controller->rirb[controller->rirbrp].resp_ex;
					uint32 cad = resp_ex & HDA_MAXCODECS;
					hda_codec* codec = controller->codecs[cad];

					if (resp_ex & RESP_EX_UNSOL) {
						dprintf("%s: Unsolicited response: %08lx/%08lx\n",
							__func__,
							controller->rirb[controller->rirbrp].response,
							resp_ex);
					} else if (codec) {
						/* Store responses in codec */
						codec->responses[codec->response_count++]
							= controller->rirb[controller->rirbrp].response;
						release_sem_etc(codec->response_sem, 1,
							B_DO_NOT_RESCHEDULE);
						rc = B_INVOKE_SCHEDULER;
					} else {
						dprintf("%s: Response for unknown codec %ld: "
							"%08lx/%08lx\n", __func__, cad, 
							controller->rirb[controller->rirbrp].response,
								resp_ex);
					}

					++controller->rirbrp;
				}			
			}

			if (rirbsts & RIRBSTS_OIS)
				dprintf("%s: RIRB Overflow\n", __func__);
		}

		/* Check for sending errors */
		if (corbsts) {
			REG8(controller, CORBSTS) = corbsts;

			if (corbsts & CORBSTS_MEI)
				dprintf("%s: CORB Memory Error!\n", __func__);
		}
	}

	if (intsts & ~(INTSTS_CIS | INTSTS_GIS)) {
		int index;
		for (index = 0; index < HDA_MAXSTREAMS; index++) {
			if ((intsts & (1 << index)) != 0) {
				if (controller->streams[index])
					hda_stream_check_intr(controller, controller->streams[index]);
				else {
					dprintf("%s: Stream interrupt for unconfigured stream "
						"%d!\n", __func__, index);
				}
			}
		}
	}

	/* NOTE: See HDA001 => CIS/GIS cannot be cleared! */

	return rc;
}


static status_t
hda_hw_start(hda_controller* controller)
{
	int timeout = 10;

	/* Put controller out of reset mode */
	REG32(controller, GCTL) |= GCTL_CRST;

	do {
		snooze(100);
	} while (--timeout && !(REG32(controller, GCTL) & GCTL_CRST));	

	return timeout ? B_OK : B_TIMED_OUT;
}


static status_t
hda_hw_corb_rirb_init(hda_controller* controller)
{
	uint32 memsz, rirboff;
	uint8 corbsz, rirbsz;
	status_t rc = B_OK;
	physical_entry pe;

	/* Determine and set size of CORB */
	corbsz = REG8(controller, CORBSIZE);
	if (corbsz & CORBSIZE_CAP_256E) {
		controller->corblen = 256;
		REG8(controller, CORBSIZE) = CORBSIZE_SZ_256E;
	} else if (corbsz & CORBSIZE_CAP_16E) {
		controller->corblen = 16;
		REG8(controller, CORBSIZE) = CORBSIZE_SZ_16E;
	} else if (corbsz & CORBSIZE_CAP_2E) {
		controller->corblen = 2;
		REG8(controller, CORBSIZE) = CORBSIZE_SZ_2E;
	}

	/* Determine and set size of RIRB */
	rirbsz = REG8(controller, RIRBSIZE);
	if (rirbsz & RIRBSIZE_CAP_256E) {
		controller->rirblen = 256;
		REG8(controller, RIRBSIZE) = RIRBSIZE_SZ_256E;
	} else if (rirbsz & RIRBSIZE_CAP_16E) {
		controller->rirblen = 16;
		REG8(controller, RIRBSIZE) = RIRBSIZE_SZ_16E;
	} else if (rirbsz & RIRBSIZE_CAP_2E) {
		controller->rirblen = 2;
		REG8(controller, RIRBSIZE) = RIRBSIZE_SZ_2E;
	}

	/* Determine rirb offset in memory and total size of corb+alignment+rirb */
	rirboff = (controller->corblen * sizeof(corb_t) + 0x7f) & ~0x7f;
	memsz = (rirboff + controller->rirblen * sizeof(rirb_t) + B_PAGE_SIZE - 1)
		& ~(B_PAGE_SIZE - 1);

	/* Allocate memory area */
	controller->rb_area = create_area("hda_corb_rirb", (void**)&controller->corb,
		B_ANY_KERNEL_ADDRESS, memsz, B_CONTIGUOUS, 0);
	if (controller->rb_area < 0)
		return controller->rb_area;

	/* Rirb is after corb+aligment */
	controller->rirb = (rirb_t*)(((uint8*)controller->corb) + rirboff);

	if ((rc = get_memory_map(controller->corb, memsz, &pe, 1)) != B_OK) {
		delete_area(controller->rb_area);
		return rc;
	}

	/* Program CORB/RIRB for these locations */
	REG32(controller, CORBLBASE) = (uint32)pe.address;
	REG32(controller, RIRBLBASE) = (uint32)pe.address + rirboff;

	/* Reset CORB read pointer */
	/* NOTE: See HDA011 for corrected procedure! */
	REG16(controller, CORBRP) = CORBRP_RST;
	do {
		snooze(10);
	} while ( !(REG16(controller, CORBRP) & CORBRP_RST) );
	REG16(controller, CORBRP) = 0;

	/* Reset RIRB write pointer */
	REG16(controller, RIRBWP) = RIRBWP_RST;

	/* Generate interrupt for every response */
	REG16(controller, RINTCNT) = 1;

	/* Setup cached read/write indices */
	controller->rirbrp = 1;
	controller->corbwp = 0;

	/* Gentlemen, start your engines... */
	REG8(controller, CORBCTL) = CORBCTL_RUN | CORBCTL_MEIE;
	REG8(controller, RIRBCTL) = RIRBCTL_DMAEN | RIRBCTL_OIC | RIRBCTL_RINTCTL;

	return B_OK;
}


//	#pragma mark -


/*! Setup hardware for use; detect codecs; etc */
status_t
hda_hw_init(hda_controller* controller)
{
	status_t rc;
	uint16 gcap;
	uint32 index;

	/* Map MMIO registers */
	controller->regs_area = map_physical_memory("hda_hw_regs",
		(void*)controller->pci_info.u.h0.base_registers[0],
		controller->pci_info.u.h0.base_register_sizes[0], B_ANY_KERNEL_ADDRESS,
		0, (void**)&controller->regs);
	if (controller->regs_area < B_OK) {
		rc = controller->regs_area;
		goto error;
	}

	/* Absolute minimum hw is online; we can now install interrupt handler */
	controller->irq = controller->pci_info.u.h0.interrupt_line;
	rc = install_io_interrupt_handler(controller->irq,
		(interrupt_handler)hda_interrupt_handler, controller, 0);
	if (rc != B_OK)
		goto no_irq;

	/* show some hw features */
	gcap = REG16(controller, GCAP);
	dprintf("HDA: HDA v%d.%d, O:%d/I:%d/B:%d, #SDO:%d, 64bit:%s\n", 
		REG8(controller, VMAJ), REG8(controller, VMIN),
		GCAP_OSS(gcap), GCAP_ISS(gcap), GCAP_BSS(gcap),
		GCAP_NSDO(gcap) ? GCAP_NSDO(gcap) *2 : 1, 
		gcap & GCAP_64OK ? "yes" : "no" );

	controller->num_input_streams = GCAP_OSS(gcap);
	controller->num_output_streams = GCAP_ISS(gcap);
	controller->num_bidir_streams = GCAP_BSS(gcap);

	/* Get controller into valid state */
	rc = hda_hw_start(controller);
	if (rc != B_OK)
		goto reset_failed;

	/* Setup CORB/RIRB */
	rc = hda_hw_corb_rirb_init(controller);
	if (rc != B_OK)
		goto corb_rirb_failed;

	REG16(controller, WAKEEN) = 0x7fff;

	/* Enable controller interrupts */
	REG32(controller, INTCTL) = INTCTL_GIE | INTCTL_CIE | 0xffff;

	/* Wait for codecs to warm up */
	snooze(1000);

	if (!controller->codecsts) {	
		rc = ENODEV;
		goto corb_rirb_failed;
	}

	for (index = 0; index < HDA_MAXCODECS; index++) {
		if ((controller->codecsts & (1 << index)) != 0)
			hda_codec_new(controller, index);
	}

	for (index = 0; index < HDA_MAXCODECS; index++) {
		if (controller->codecs[index] && controller->codecs[index]->num_afgs) {
			controller->active_codec = controller->codecs[index];
			break;
		}
	}

	if (controller->active_codec != NULL)
		return B_OK;

	rc = ENODEV;

corb_rirb_failed:
	REG32(controller, INTCTL) = 0;

reset_failed:
	remove_io_interrupt_handler(controller->irq,
		(interrupt_handler)hda_interrupt_handler, controller);

no_irq:
	delete_area(controller->regs_area);
	controller->regs_area = B_ERROR;
	controller->regs = NULL;

error:
	dprintf("ERROR: %s(%ld)\n", strerror(rc), rc);

	return rc;
}


/*! Stop any activity */
void
hda_hw_stop(hda_controller* controller)
{
	int index;

	/* Stop all audio streams */
	for (index = 0; index < HDA_MAXSTREAMS; index++)
		if (controller->streams[index] && controller->streams[index]->running)
			hda_stream_stop(controller, controller->streams[index]);
}


/*! Free resources */
void
hda_hw_uninit(hda_controller* controller)
{
	uint32 index;

	if (controller == NULL)
		return;

	/* Stop all audio streams */
	hda_hw_stop(controller);

	/* Stop CORB/RIRB */
	REG8(controller, CORBCTL) = 0;
	REG8(controller, RIRBCTL) = 0;

	/* Disable interrupts and remove interrupt handler */
	REG32(controller, INTCTL) = 0;
	REG32(controller, GCTL) &= ~GCTL_CRST;
	remove_io_interrupt_handler(controller->irq,
		(interrupt_handler)hda_interrupt_handler, controller);

	/* Delete corb/rirb area */
	if (controller->rb_area >= 0) {
		delete_area(controller->rb_area);
		controller->rb_area = B_ERROR;
		controller->corb = NULL;
		controller->rirb = NULL;
	}

	/* Unmap registers */
	if (controller->regs_area >= 0) {
		delete_area(controller->regs_area);
		controller->regs_area = B_ERROR;
		controller->regs = NULL;
	}

	/* Now delete all codecs */
	for (index = 0; index < HDA_MAXCODECS; index++) {
		if (controller->codecs[index] != NULL)
			hda_codec_delete(controller->codecs[index]);
	}
}


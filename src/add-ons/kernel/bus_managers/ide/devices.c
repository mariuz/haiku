/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open IDE bus manager

	Device manager

	As the IDE bus manager is an SCSI to IDE translater, it
	has to know a bit more about connected devices then a standard
	SIM. This file contains device detection and classification.
*/

#include "ide_internal.h"
#include "ide_sim.h"
#include "ide_cmds.h"

#include <string.h>
#include <malloc.h>
#include <ByteOrder.h>


/** cleanup links devices on one bus when <device> is deleted */

static void
cleanup_device_links(ide_device_info *device)
{
	ide_bus_info *bus = device->bus;

	bus->devices[device->is_device1] = NULL;

	if (device->other_device) {
		if (device->other_device != device) {
			device->other_device->other_device = device->other_device;
			bus->first_device = device->other_device;
		} else
			bus->first_device = NULL;
	}

	device->other_device = NULL;
}


/** destroy device info */

static void
destroy_device(ide_device_info *device)
{
	SHOW_FLOW( 4, "%p", device );

	// paranoia
	device->exec_io = NULL;
	cancel_timer(&device->reconnect_timer.te);

	scsi->free_dpc(device->reconnect_timeout_dpc);

	cleanup_device_links(device);

	destroy_qreq_array(device);

	uninit_synced_pc(&device->reconnect_timeout_synced_pc);

	free(device);
}


/** setup links between the devices on one bus */

static void
setup_device_links(ide_bus_info *bus, ide_device_info *device)
{
	SHOW_FLOW0( 4, "" );

	device->bus = bus;
	bus->devices[device->is_device1] = device;

	device->other_device = device;

	if (device->is_device1) {
		if (bus->devices[0]) {
			device->other_device = bus->devices[0];
			bus->devices[0]->other_device = device;
		}
	} else {
		if (bus->devices[1]) {
			device->other_device = bus->devices[1];
			bus->devices[1]->other_device = device;
		}
	}

	if (bus->first_device == NULL)
		bus->first_device = device;
}


/** create device info */

static
ide_device_info *create_device(ide_bus_info *bus, bool is_device1)
{
	ide_device_info *device;

	SHOW_FLOW(4, "is_device1=%d", (int)is_device1);

	device = (ide_device_info *)malloc(sizeof(*device));
	if (device == NULL) 
		return NULL;

	memset(device, 0, sizeof(*device));

	device->is_device1 = is_device1;
	device->target_id = is_device1;

	setup_device_links(bus, device);

	device->DMA_failures = 0;
	device->CQ_failures = 0;
	device->num_failed_send = 0;

	device->combined_sense = 0;

	device->num_running_reqs = 0;

	device->reconnect_timer.device = device;

	init_synced_pc(&device->reconnect_timeout_synced_pc, 
		reconnect_timeout_worker);
	
	if (scsi->alloc_dpc(&device->reconnect_timeout_dpc) != B_OK)
		goto err;

	device->total_sectors = 0;
	return device;

err:
	destroy_device(device);
	return NULL;
}

#if B_HOST_IS_LENDIAN

#define B_BENDIAN_TO_HOST_MULTI(v, n) do {		\
	size_t __swap16_multi_n = (n);				\
	uint16 *__swap16_multi_v = (v);				\
												\
	while( __swap16_multi_n ) {					\
		*__swap16_multi_v = B_SWAP_INT16(*__swap16_multi_v); \
		__swap16_multi_v++;						\
		__swap16_multi_n--;						\
	}											\
} while (0)

#else

#define B_BENDIAN_TO_HOST_MULTI(v, n) 

#endif


/** prepare infoblock for further use, i.e. fix endianess */

static void
prep_infoblock(ide_device_info *device)
{
	ide_device_infoblock *infoblock = &device->infoblock;

	B_BENDIAN_TO_HOST_MULTI((uint16 *)infoblock->serial_number, 
		sizeof(infoblock->serial_number) / 2);

	B_BENDIAN_TO_HOST_MULTI( (uint16 *)infoblock->firmware_version, 
		sizeof(infoblock->firmware_version) / 2);

	B_BENDIAN_TO_HOST_MULTI( (uint16 *)infoblock->model_number, 
		sizeof(infoblock->model_number) / 2);

	infoblock->LBA_total_sectors = B_LENDIAN_TO_HOST_INT32(infoblock->LBA_total_sectors);
	infoblock->LBA48_total_sectors = B_LENDIAN_TO_HOST_INT64(infoblock->LBA48_total_sectors);
}


/** read info block of ATA or ATAPI device */

static bool
scan_device_int(ide_device_info *device, bool atapi)
{
	ide_bus_info *bus = device->bus;
	int status;

	FAST_LOG1(bus->log, ev_ide_scan_device_int, (uint32)atapi);

	device->tf_param_mask = 0;
	device->tf.write.command = atapi ? IDE_CMD_IDENTIFY_PACKET_DEVICE
		: IDE_CMD_IDENTIFY_DEVICE;

	// initialize device selection flags,
	// this is the only place where this bit gets initialized in the task file
	if (bus->controller->read_command_block_regs(bus->channel, &device->tf,
			ide_mask_device_head) != B_OK) 
		return false;

	device->tf.lba.device = device->is_device1;

	if (!send_command(device, NULL, atapi ? false : true, 20, ide_state_sync_waiting)) {
		FAST_LOG0(bus->log, ev_ide_scan_device_int_cant_send);
		return false;
	}

	// do a short wait first - if there's no device at all we could wait forever
	if (acquire_sem_etc(bus->sync_wait_sem, 1, B_RELATIVE_TIMEOUT, 1000/*100000*/) == B_TIMED_OUT) {
		bool cont;

		SHOW_FLOW0( 3, "no fast response to inquiry" );

		// check the busy flag - if it's still set, there's probably no device	
		IDE_LOCK(bus);

		status = bus->controller->get_altstatus(bus->channel);
		SHOW_FLOW(3, "status=%x", (int)status);
		cont = (status & ide_status_bsy) == ide_status_bsy;

		IDE_UNLOCK(bus);

		if (!cont) {
			SHOW_FLOW0( 3, "no busy bit set after 100ms - probably noone there" );
			// no reaction -> abort waiting
			cancel_irq_timeout(bus);

			// timeout or irq may have been fired, reset semaphore just is case
			acquire_sem_etc(bus->sync_wait_sem, 1, B_RELATIVE_TIMEOUT, 0);

			FAST_LOG0(bus->log, ev_ide_scan_device_int_keeps_busy);
			return false;
		}

		SHOW_FLOW0( 3, "busy bit set, give device more time" );
		// there is something, so wait for it
		acquire_sem(bus->sync_wait_sem);
	}

	// cancel the timeout manually; usuall this is done by wait_for_sync(), but
	// we've used the wait semaphore directly	
	cancel_irq_timeout(bus);

	if (bus->sync_wait_timeout)
		return false;

	ide_wait(device, ide_status_drq, ide_status_bsy, true, 1000);

	status = bus->controller->get_altstatus(bus->channel);

	if ((status & ide_status_err) != 0) {
		// if there's no device, all bits including the error bit are set
		SHOW_FLOW( 3, "err set - no device or wrong type (status: %x)", status );
		return false;
	}

	// get the infoblock		
	bus->controller->read_pio(bus->channel, (uint16 *)&device->infoblock, 
		sizeof(device->infoblock) / sizeof(uint16), false);

	if (!wait_for_drqdown(device))
		return false;

	SHOW_FLOW0( 3, "device found" );

	FAST_LOG0(bus->log, ev_ide_scan_device_int_found);

	prep_infoblock(device);
	return true;	
}


/** scan one device */

void
scan_device_worker(ide_bus_info *bus, void *arg)
{
	int is_device1 = (int)arg;
	ide_device_info *device;

	SHOW_FLOW(4, "is_device1: %d", is_device1);

	// forget everything we know about the device;
	// don't care about peripheral drivers using this device
	// as the device info is only used by us and not published
	// directly or indirectly to the SCSI bus manager
	if (bus->devices[is_device1])
		destroy_device(bus->devices[is_device1]);

	device = create_device(bus, is_device1);

	// reset status so we can see what goes wrong during detection
	device->subsys_status = SCSI_REQ_CMP;

	if (scan_device_int(device, false)) {
		if (!prep_ata(device))
			goto err;
	} else if (device->subsys_status != SCSI_TID_INVALID
				&& scan_device_int(device, true)) {
		// only try ATAPI if there is at least some device
		// (see send_command - this error code must be unique!)
		if (!prep_atapi(device))
			goto err;
	} else
		goto err;

	bus->state = ide_state_idle;
	release_sem(bus->scan_device_sem);
	return;

err:
	destroy_device(device);

	bus->state = ide_state_idle;
	release_sem(bus->scan_device_sem);
}

/*
	Authors:
	Rudolf Cornelissen 4/2003-
*/

#define MODULE_BIT 0x04000000

#include "acc_std.h"

/* Get some info about the device */
status_t GET_ACCELERANT_DEVICE_INFO(accelerant_device_info * adi) 
{
	/*no info on version is provided, so presumably this is for my info*/
	LOG(4,("DEVICE_INFO: version 0x%08x\n", adi->version));
	
	switch (si->ps.card_type)
	{
	case 0x01:
		sprintf(adi->name,"Neomagic Plain");
		break;
	case 0x02:
		sprintf(adi->name,"Neomagic MAX");
		break;
	}
	
	sprintf(adi->chipset,"Neomagic");

	sprintf(adi->serial_no,"01134"); /*FIXME*/

	adi->memory=si->ps.memory_size * 1024;
	
	adi->dac_speed=si->ps.max_dac1_clock;

	// apsed, TODO ?? GET_ACCELERANT_DEVICE_INFO never called and kind of cards
	LOG(2,("GET_ACCELERANT_DEVICE_INFO %20s 0x%08x %d\n", "version", adi->version, adi->version));
	LOG(2,("GET_ACCELERANT_DEVICE_INFO %20s %s\n", "name", adi->name));
	LOG(2,("GET_ACCELERANT_DEVICE_INFO %20s %s\n", "chipset", adi->chipset));
	LOG(2,("GET_ACCELERANT_DEVICE_INFO %20s %s\n", "serial_no", adi->serial_no));
	LOG(2,("GET_ACCELERANT_DEVICE_INFO %20s 0x%08x %d\n", "memory", adi->memory, adi->memory));
	LOG(2,("GET_ACCELERANT_DEVICE_INFO %20s %d\n", "dac_speed", adi->dac_speed));


	return B_OK;
}

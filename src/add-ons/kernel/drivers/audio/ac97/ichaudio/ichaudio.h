#include <SupportDefs.h>
#include "hardware.h"

typedef struct
{
	uint32	irq;
	uint32	nambar;
	uint32	nabmbar;
	area_id area_mmbar; // ich4
	area_id area_mbbar; // ich4
	void *	mmbar; // ich4
	void *	mbbar; // ich4

	uint32 codecoffset;
	uint32 input_rate;
	uint32 output_rate;

} ichaudio_cookie;


#define TYPE_ICH4			0x01
#define TYPE_SIS7012		0x02

/* The SIS7012 chipset has SR and PICB registers swapped when compared to Intel */
#define	GET_REG_X_PICB(cfg)		(((cfg)->type == TYPE_SIS7012) ? _ICH_REG_X_SR : _ICH_REG_X_PICB)
#define	GET_REG_X_SR(cfg)		(((cfg)->type == TYPE_SIS7012) ? _ICH_REG_X_PICB : _ICH_REG_X_SR)
/* Each 16 bit sample is counted as 1 in SIS7012 chipsets, 2 in all others */
#define GET_HW_SAMPLE_SIZE(cfg)	(((cfg)->type == TYPE_SIS7012) ? 1 : 2)


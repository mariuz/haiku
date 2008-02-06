/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _BLUETOOTH_ERROR_H
#define _BLUETOOTH_ERROR_H

#include <Errors.h>


#define BT_OK                               B_OK
#define BT_ERROR                            BT_UNSPECIFIED_ERROR

/* Official error code for Bluetooth V2.1 + EDR */
#define BT_UNKNOWN_COMMAND			        0x01
#define BT_NO_CONNECTION			        0x02
#define BT_HARDWARE_FAILURE			        0x03
#define BT_PAGE_TIMEOUT			            0x04
#define BT_AUTHENTICATION_FAILURE		    0x05
#define BT_PIN_OR_KEY_MISSING			    0x06
#define BT_MEMORY_FULL				        0x07
#define BT_CONNECTION_TIMEOUT			    0x08
#define BT_MAX_NUMBER_OF_CONNECTIONS		0x09
#define BT_MAX_NUMBER_OF_SCO_CONNECTIONS	0x0a
#define BT_ACL_CONNECTION_EXISTS		    0x0b
#define BT_COMMAND_DISALLOWED			    0x0c
#define BT_REJECTED_LIMITED_RESOURCES		0x0d
#define BT_REJECTED_SECURITY			    0x0e
#define BT_REJECTED_PERSONAL			    0x0f
#define BT_HOST_TIMEOUT			            0x10
#define BT_UNSUPPORTED_FEATURE			    0x11
#define BT_INVALID_PARAMETERS			    0x12
#define BT_OE_USER_ENDED_CONNECTION		    0x13
#define BT_OE_LOW_RESOURCES			        0x14
#define BT_OE_POWER_OFF			            0x15
#define BT_CONNECTION_TERMINATED		    0x16
#define BT_REPEATED_ATTEMPTS			    0x17
#define BT_PAIRING_NOT_ALLOWED			    0x18
#define BT_UNKNOWN_LMP_PDU			        0x19
#define BT_UNSUPPORTED_REMOTE_FEATURE		0x1a
#define BT_SCO_OFFSET_REJECTED			    0x1b
#define BT_SCO_INTERVAL_REJECTED		    0x1c
#define BT_AIR_MODE_REJECTED			    0x1d
#define BT_INVALID_LMP_PARAMETERS		    0x1e
#define BT_UNSPECIFIED_ERROR			    0x1f
#define BT_UNSUPPORTED_LMP_PARAMETER_VALUE	0x20
#define BT_ROLE_CHANGE_NOT_ALLOWED		    0x21
#define BT_LMP_RESPONSE_TIMEOUT		        0x22
#define BT_LMP_ERROR_TRANSACTION_COLLISION	0x23
#define BT_LMP_PDU_NOT_ALLOWED			    0x24
#define BT_ENCRYPTION_MODE_NOT_ACCEPTED	    0x25
#define BT_UNIT_LINK_KEY_USED			    0x26
#define BT_QOS_NOT_SUPPORTED			    0x27
#define BT_INSTANT_PASSED			        0x28
#define BT_PAIRING_NOT_SUPPORTED		    0x29
#define BT_TRANSACTION_COLLISION		    0x2a
#define BT_QOS_UNACCEPTABLE_PARAMETER	    0x2c
#define BT_QOS_REJECTED			            0x2d
#define BT_CLASSIFICATION_NOT_SUPPORTED	    0x2e
#define BT_INSUFFICIENT_SECURITY		    0x2f
#define BT_PARAMETER_OUT_OF_RANGE		    0x30
#define BT_ROLE_SWITCH_PENDING			    0x32
#define BT_SLOT_VIOLATION			        0x34
#define BT_ROLE_SWITCH_FAILED			    0x35

#define EXTENDED_INQUIRY_RESPONSE_TOO_LARGE 0x36
#define SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST 0x37
#define HOST_BUSY_PAIRING                   0x38


#endif

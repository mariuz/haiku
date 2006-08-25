/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

#ifndef _USB_P_
#define _USB_P_

#include <lock.h>
#include "usbspec_p.h"
#include "BeOSCompatibility.h"


//#define TRACE_USB
#ifdef TRACE_USB
#define TRACE(x)		dprintf x
#define TRACE_ERROR(x)	dprintf x
#else
#define TRACE(x)		/* nothing */
#define TRACE_ERROR(x)	dprintf x
#endif


class Hub;
class Stack;
class Device;
class Transfer;
class BusManager;
class ControlPipe;
class Object;


struct host_controller_info {
	module_info		info;
	status_t		(*control)(uint32 op, void *data, size_t length);
	status_t		(*add_to)(Stack *stack);
};


struct usb_driver_info {
	const char						*driver_name;
	usb_support_descriptor			*support_descriptors;
	uint32							support_descriptor_count;
	const char						*republish_driver_name;
	usb_notify_hooks				notify_hooks;
	usb_driver_info					*link;
};


#define USB_OBJECT_NONE					0x00000000
#define USB_OBJECT_PIPE					0x00000001
#define USB_OBJECT_CONTROL_PIPE			0x00000002
#define USB_OBJECT_INTERRUPT_PIPE		0x00000004
#define USB_OBJECT_BULK_PIPE			0x00000008
#define USB_OBJECT_ISO_PIPE				0x00000010
#define USB_OBJECT_INTERFACE			0x00000020
#define USB_OBJECT_DEVICE				0x00000040
#define USB_OBJECT_HUB					0x00000080


class Stack {
public:
										Stack();
										~Stack();

		status_t						InitCheck();

		bool							Lock();
		void							Unlock();

		usb_id							GetUSBID(Object *object);
		void							PutUSBID(usb_id id);
		Object							*GetObject(usb_id id);

		void							AddBusManager(BusManager *bus);
		int32							IndexOfBusManager(BusManager *bus);

		status_t						AllocateChunk(void **logicalAddress,
											void **physicalAddress, uint8 size);
		status_t						FreeChunk(void *logicalAddress,
											void *physicalAddress, uint8 size);

		area_id							AllocateArea(void **logicalAddress,
											void **physicalAddress,
											size_t size, const char *name);

		void							NotifyDeviceChange(Device *device,
											bool added);

		// USB API
		status_t						RegisterDriver(const char *driverName,
											const usb_support_descriptor *descriptors,
											size_t descriptorCount,
											const char *republishDriverName);

		status_t						InstallNotify(const char *driverName,
											const usb_notify_hooks *hooks);
		status_t						UninstallNotify(const char *driverName);

private:
		Vector<BusManager *>			fBusManagers;

		benaphore						fLock;
		area_id							fAreas[USB_MAX_AREAS];
		void							*fLogical[USB_MAX_AREAS];
		void							*fPhysical[USB_MAX_AREAS];
		uint16							fAreaFreeCount[USB_MAX_AREAS];

		addr_t							fListhead8;
		addr_t							fListhead16;
		addr_t							fListhead32;
		addr_t							fListhead64;

		uint32							fObjectIndex;
		uint32							fObjectMaxCount;
		Object							**fObjectArray;

		usb_driver_info					*fDriverList;
};


/*
 * This class manages a bus. It is created by the Stack object
 * after a host controller gives positive feedback on whether the hardware
 * is found. 
 */
class BusManager {
public:
										BusManager(Stack *stack);
virtual									~BusManager();

virtual	status_t						InitCheck();

		bool							Lock();
		void							Unlock();

		Device							*AllocateNewDevice(Device *parent,
											bool lowSpeed);

		int8							AllocateAddress();

virtual	status_t						Start();
virtual	status_t						Stop();

virtual	status_t						SubmitTransfer(Transfer *transfer);
virtual	status_t						SubmitRequest(Transfer *transfer);

		Stack							*GetStack() { return fStack; };
		void							SetStack(Stack *stack) { fStack = stack; };

		Hub								*GetRootHub() { return fRootHub; };
		void							SetRootHub(Hub *hub) { fRootHub = hub; };

protected:
		bool							fInitOK;

private:
static	int32							ExploreThread(void *data);

		benaphore						fLock;
		bool							fDeviceMap[128];
		ControlPipe						*fDefaultPipe;
		ControlPipe						*fDefaultPipeLowSpeed;
		Hub								*fRootHub;
		Stack							*fStack;
		thread_id						fExploreThread;
};


class Object {
public:
										Object(BusManager *bus);

		Stack							*GetStack() { return fStack; };

		usb_id							USBID() { return fUSBID; };
virtual	uint32							Type() { return USB_OBJECT_NONE; };

private:
		Stack							*fStack;
		usb_id							fUSBID;
};


/*
 * The Pipe class is the communication management between the hardware and
 * the stack. It creates packets, manages these and performs callbacks.
 */
class Pipe : public Object {
public:
enum	pipeDirection 	{ In, Out, Default };
enum	pipeSpeed		{ LowSpeed, NormalSpeed };

										Pipe(Device *device,
											pipeDirection direction,
											pipeSpeed speed,
											uint8 endpointAddress,
											size_t maxPacketSize);
										Pipe(BusManager *bus,
											int8 deviceAddress,
											pipeSpeed speed,
											size_t maxPacketSize);
virtual									~Pipe();

virtual	uint32							Type() { return USB_OBJECT_PIPE; };

		int8							DeviceAddress();
		pipeSpeed						Speed() { return fSpeed; };
		pipeDirection					Direction() { return fDirection; };
		int8							EndpointAddress() { return fEndpoint; };
		size_t							MaxPacketSize() { return fMaxPacketSize; };

virtual	bool							DataToggle() { return fDataToggle; };
virtual	void							SetDataToggle(bool toggle) { fDataToggle = toggle; };

		status_t						SubmitTransfer(Transfer *transfer);
		status_t						CancelQueuedTransfers();

		// Convenience functions for standard requests
virtual	status_t						SetFeature(uint16 selector);
virtual	status_t						ClearFeature(uint16 selector);
virtual	status_t						GetStatus(uint16 *status);

protected:
		Device							*fDevice;
		int8							fDeviceAddress;
		BusManager						*fBus;
		usb_id							fUSBID;
		pipeDirection					fDirection;
		pipeSpeed						fSpeed;
		uint8							fEndpoint;
		size_t							fMaxPacketSize;
		bool							fDataToggle;
};


class ControlPipe : public Pipe {
public:
										ControlPipe(Device *device,
											pipeSpeed speed,
											uint8 endpointAddress,
											size_t maxPacketSize);

										// Constructor for default control pipe
										ControlPipe(BusManager *bus,
											int8 deviceAddress,
											pipeSpeed speed,
											size_t maxPacketSize);

virtual	uint32							Type() { return USB_OBJECT_PIPE | USB_OBJECT_CONTROL_PIPE; };

										// The data toggle is not relevant
										// for control transfers, as they are
										// always enclosed by a setup and
										// status packet. The toggle always
										// starts at 1.
virtual	bool							DataToggle() { return true; };
virtual	void							SetDataToggle(bool toggle) {};

		status_t						SendRequest(uint8 requestType,
											uint8 request, uint16 value,
											uint16 index, uint16 length,
											void *data, size_t dataLength,
											size_t *actualLength);
static	void							SendRequestCallback(void *cookie,
											uint32 status, void *data,
											size_t actualLength);

		status_t						QueueRequest(uint8 requestType,
											uint8 request, uint16 value,
											uint16 index, uint16 length,
											void *data, size_t dataLength,
											usb_callback_func callback,
											void *callbackCookie);
};


class InterruptPipe : public Pipe {
public:
										InterruptPipe(Device *device,
											pipeDirection direction,
											pipeSpeed speed,
											uint8 endpointAddress,
											size_t maxPacketSize);

virtual	uint32							Type() { return USB_OBJECT_PIPE | USB_OBJECT_INTERRUPT_PIPE; };

		status_t						QueueInterrupt(void *data,
											size_t dataLength,
											usb_callback_func callback,
											void *callbackCookie);
};


class BulkPipe : public Pipe {
public:
										BulkPipe(Device *device,
											pipeDirection direction,
											pipeSpeed speed,
											uint8 endpointAddress,
											size_t maxPacketSize);

virtual	uint32							Type() { return USB_OBJECT_PIPE | USB_OBJECT_BULK_PIPE; };

		status_t						QueueBulk(void *data,
											size_t dataLength,
											usb_callback_func callback,
											void *callbackCookie);
		status_t						QueueBulkV(iovec *vector,
											size_t vectorCount,
											usb_callback_func callback,
											void *callbackCookie);
};


class IsochronousPipe : public Pipe {
public:
										IsochronousPipe(Device *device,
											pipeDirection direction,
											pipeSpeed speed,
											uint8 endpointAddress,
											size_t maxPacketSize);

virtual	uint32							Type() { return USB_OBJECT_PIPE | USB_OBJECT_ISO_PIPE; };

		status_t						QueueIsochronous(void *data,
											size_t dataLength,
											rlea *rleArray,
											uint16 bufferDurationMS,
											usb_callback_func callback,
											void *callbackCookie);
};


class Interface : public ControlPipe {
public:
										Interface(Device *device);

virtual	uint32							Type() { return USB_OBJECT_PIPE | USB_OBJECT_CONTROL_PIPE | USB_OBJECT_INTERFACE; };

		// Convenience functions for standard requests
virtual	status_t						SetFeature(uint16 selector);
virtual	status_t						ClearFeature(uint16 selector);
virtual	status_t						GetStatus(uint16 *status);
};


class Device : public ControlPipe {
public:
										Device(BusManager *bus, Device *parent,
											usb_device_descriptor &desc,
											int8 deviceAddress, bool lowSpeed);

		status_t						InitCheck();

virtual	uint32							Type() { return USB_OBJECT_PIPE | USB_OBJECT_CONTROL_PIPE | USB_OBJECT_DEVICE; };

		int8							Address() { return fDeviceAddress; };
		BusManager						*Manager() { return fBus; };

virtual	status_t						GetDescriptor(uint8 descriptorType,
											uint8 index, uint16 languageID,
											void *data, size_t dataLength,
											size_t *actualLength);

		const usb_device_descriptor		*DeviceDescriptor() const;

		const usb_configuration_info	*Configuration() const;
		const usb_configuration_info	*ConfigurationAt(uint8 index) const;
		status_t						SetConfiguration(const usb_configuration_info *configuration);
		status_t						SetConfigurationAt(uint8 index);

virtual	void							ReportDevice(
											usb_support_descriptor *supportDescriptors,
											uint32 supportDescriptorCount,
											const usb_notify_hooks *hooks,
											bool added);
virtual	status_t						BuildDeviceName(char *string,
											uint32 *index, size_t bufferSize,
											Device *device);

		// Convenience functions for standard requests
virtual	status_t						SetFeature(uint16 selector);
virtual	status_t						ClearFeature(uint16 selector);
virtual	status_t						GetStatus(uint16 *status);

protected:
		usb_device_descriptor			fDeviceDescriptor;
		usb_configuration_info			*fConfigurations;
		usb_configuration_info			*fCurrentConfiguration;
		bool							fInitOK;
		bool							fLowSpeed;
		BusManager						*fBus;
		Device							*fParent;
		int8							fDeviceAddress;
		size_t							fMaxPacketIn[16];
		size_t							fMaxPacketOut[16];
		sem_id							fLock;
		void							*fNotifyCookie;
};


class Hub : public Device {
public:
										Hub(BusManager *bus, Device *parent,
											usb_device_descriptor &desc,
											int8 deviceAddress, bool lowSpeed);

virtual	uint32							Type() { return USB_OBJECT_PIPE | USB_OBJECT_CONTROL_PIPE | USB_OBJECT_DEVICE | USB_OBJECT_HUB; };

virtual	status_t						GetDescriptor(uint8 descriptorType,
											uint8 index, uint16 languageID,
											void *data, size_t dataLength,
											size_t *actualLength);

		status_t						UpdatePortStatus(uint8 index);
		status_t						ResetPort(uint8 index);
		void							Explore();
static	void							InterruptCallback(void *cookie,
											uint32 status, void *data,
											uint32 actualLength);

virtual	void							ReportDevice(
											usb_support_descriptor *supportDescriptors,
											uint32 supportDescriptorCount,
											const usb_notify_hooks *hooks,
											bool added);
virtual	status_t						BuildDeviceName(char *string,
											uint32 *index, size_t bufferSize,
											Device *device);

private:
		InterruptPipe					*fInterruptPipe;
		usb_hub_descriptor				fHubDescriptor;

		usb_port_status					fInterruptStatus[8];
		usb_port_status					fPortStatus[8];
		Device							*fChildren[8];
};


/*
 * A Transfer is allocated on the heap and passed to the Host Controller in
 * SubmitTransfer(). It is generated for all queued transfers. If queuing
 * succeds (SubmitTransfer() returns with >= B_OK) the Host Controller takes
 * ownership of the Transfer and will delete it as soon as it has called the
 * set callback function. If SubmitTransfer() failes, the calling function is
 * responsible for deleting the Transfer.
 * Also, the transfer takes ownership of the usb_request_data passed to it in
 * SetRequestData(), but does not take ownership of the data buffer set by
 * SetData().
 */
class Transfer {
public:
									Transfer(Pipe *pipe);
									~Transfer();

		Pipe						*TransferPipe() { return fPipe; };

		void						SetRequestData(usb_request_data *data);
		usb_request_data			*RequestData() { return fRequestData; };

		void						SetData(uint8 *buffer, size_t length);
		uint8						*Data() { return (uint8 *)fData.iov_base; };
		size_t						DataLength() { return fData.iov_len; };

		void						SetVector(iovec *vector, size_t vectorCount);
		iovec						*Vector() { return fVector; };
		size_t						VectorCount() { return fVectorCount; };
		size_t						VectorLength();

		void						SetCallback(usb_callback_func callback,
										void *cookie);

		void						Finished(uint32 status, size_t actualLength);

private:
		// Data that is related to the transfer
		Pipe						*fPipe;
		iovec						fData;
		iovec						*fVector;
		size_t						fVectorCount;

		usb_callback_func			fCallback;
		void						*fCallbackCookie;

		// For control transfers
		usb_request_data			*fRequestData;
};

#endif

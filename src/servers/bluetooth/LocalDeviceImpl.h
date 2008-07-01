/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _LOCALDEVICE_IMPL_H_
#define _LOCALDEVICE_IMPL_H_

#include <String.h>

#include <bluetooth/bluetooth.h>

#include "LocalDeviceHandler.h"

#include "HCIDelegate.h"
#include "HCIControllerAccessor.h"
#include "HCITransportAccessor.h"

class LocalDeviceImpl : public LocalDeviceHandler {

private:
	LocalDeviceImpl(HCIDelegate* hd);	

public:

    // Factory methods
    static LocalDeviceImpl* CreateControllerAccessor(BPath* path);
    static LocalDeviceImpl* CreateTransportAccessor(BPath* path);		    

	void HandleEvent(struct hci_event_header* event);
    
    /* Request handling */	
	status_t GetAddress(bdaddr_t* bdaddr, BMessage* request);	
	status_t GetFriendlyName(BString str, BMessage* request);
	status_t ProcessSimpleRequest(BMessage* request);

    /* Events handling */	
    void CommandComplete(struct hci_ev_cmd_complete* event, BMessage* request, int32 index);
	void CommandStatus(struct hci_ev_cmd_status* event, BMessage* request, int32 index);
	
	// Inquiry
	void InquiryResult(uint8* numberOfResponses, BMessage* request);
	void InquiryComplete(uint8* status, BMessage* request);
	void RemoteNameRequestComplete(struct hci_remote_name_request_complete_reply* remotename, BMessage* request);
	
	// Connection
	void ConnectionComplete(struct hci_ev_conn_complete* event, BMessage* request);
	void PinCodeRequest(struct hci_ev_pin_code_req* event, BMessage* request);
};

#endif

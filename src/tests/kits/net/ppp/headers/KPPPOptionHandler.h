//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_OPTION_HANDLER__H
#define _K_PPP_OPTION_HANDLER__H

#include <KPPPDefs.h>

#ifndef _K_PPP_INTERFACE__H
#include <KPPPInterface.h>
#endif

class PPPConfigurePacket;


class PPPOptionHandler {
	public:
		PPPOptionHandler(const char *name, uint8 type, PPPInterface& interface,
			driver_parameter *settings);
		virtual ~PPPOptionHandler();
		
		virtual status_t InitCheck() const;
		
		const char *Name() const
			{ return fName; }
		
		uint8 Type() const
			{ return fType; }
		
		PPPInterface& Interface() const
			{ return fInterface; }
		driver_parameter *Settings() const
			{ return fSettings; }
		
		void SetEnabled(bool enabled = true)
			{ fEnabled = enabled; }
		bool IsEnabled() const
			{ return fEnabled; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		virtual status_t StackControl(uint32 op, void *data);
			// called by netstack (forwarded by PPPInterface)
		
		// we want to send a configure request or we received a reply
		virtual status_t AddToRequest(PPPConfigurePacket& request) = 0;
		virtual status_t ParseNak(const PPPConfigurePacket& nak) = 0;
			// create next request based on these and previous values
		virtual status_t ParseReject(const PPPConfigurePacket& reject) = 0;
			// create next request based on these and previous values
		virtual status_t ParseAck(const PPPConfigurePacket& ack) = 0;
			// this is called for all handlers
		
		// peer sent configure request
		virtual status_t ParseRequest(const PPPConfigurePacket& request,
			int32 index, PPPConfigurePacket& nak, PPPConfigurePacket& reject) = 0;
			// index may be behind the last item which means additional values can be
			// appended
		virtual status_t SendingAck(const PPPConfigurePacket& ack) = 0;
			// notification that we ack these values
		
		virtual void Reset() = 0;
			// e.g.: remove list of rejected values

	protected:
		status_t fInitStatus;

	private:
		char fName[PPP_HANDLER_NAME_LENGTH_LIMIT + 1];
		uint8 fType;
		PPPInterface& fInterface;
		driver_parameter *fSettings;
		
		bool fEnabled;
};


#endif

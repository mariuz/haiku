// Sun, 18 Jun 2000
// Y.Takagi

#include "LprTransport.h"
#include "DbgMsg.h"

LprTransport *transport = NULL;

extern "C" _EXPORT void exit_transport()
{
	DBGMSG(("> exit_transport\n"));
	if (transport) {
		delete transport;
		transport = NULL;
	}
	DBGMSG(("< exit_transport\n"));
}

extern "C" _EXPORT BDataIO *init_transport(BMessage *msg)
{
	DBGMSG(("> init_transport\n"));

	transport = new LprTransport(msg);

	if (transport->fail()) {
		exit_transport();
	}

	if (msg)
		msg->what = 'okok';

	DBGMSG(("< init_transport\n"));
	return transport;
}

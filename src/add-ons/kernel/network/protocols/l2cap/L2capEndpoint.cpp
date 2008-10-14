#include "L2capEndpoint.h"


#include <stdio.h>
#include <sys/stat.h>
#include <string.h>


#include "l2cap_internal.h"
#include "l2cap_address.h"

#include <bluetooth/L2CAP/btL2CAP.h>
#define BT_DEBUG_THIS_MODULE
#include <btDebug.h>


static inline bigtime_t
absolute_timeout(bigtime_t timeout)
{
	if (timeout == 0 || timeout == B_INFINITE_TIMEOUT)
		return timeout;

	// TODO: Make overflow safe!
	return timeout + system_time();
}


L2capEndpoint::L2capEndpoint(net_socket* socket)
	:
	ProtocolSocket(socket),
	fPeerEndpoint(NULL),
	fAcceptSemaphore(-1)
{
	debugf("[%ld] %p->L2capEndpoint::L2capEndpoint()\n", find_thread(NULL), this);


}


L2capEndpoint::~L2capEndpoint()
{
	debugf("[%ld] %p->L2capEndpoint::~L2capEndpoint()\n", find_thread(NULL), this);


}


status_t
L2capEndpoint::Init()
{
	debugf("[%ld] %p->L2capEndpoint::Init()\n", find_thread(NULL), this);

	return(B_OK);
}


void
L2capEndpoint::Uninit()
{
	debugf("[%ld] %p->L2capEndpoint::Uninit()\n", find_thread(NULL), this);


}


status_t
L2capEndpoint::Open()
{
	debugf("[%ld] %p->L2capEndpoint::Open()\n", find_thread(NULL), this);

    status_t error = ProtocolSocket::Open();
	if (error != B_OK)
		return(error);



	return(B_OK);
}


status_t
L2capEndpoint::Close()
{
	debugf("[%ld] %p->L2capEndpoint::Close()\n", find_thread(NULL), this);

	return(B_OK);
}


status_t
L2capEndpoint::Free()
{
	debugf("[%ld] %p->L2capEndpoint::Free()\n", find_thread(NULL), this);

	return(B_OK);
}


status_t
L2capEndpoint::Bind(const struct sockaddr *_address)
{
	if (_address == NULL)
		panic("null adrresss!");
	
	if (_address->sa_family != AF_BLUETOOTH )
		return(EAFNOSUPPORT);

	// TODO: Check socladdr_l2cap size

	// TODO: Check if that PSM is already bound
	// return EADDRINUSE;


	// TODO: Check if the PSM is valid, check assigned numbers document for valid
	// psm available to applications.
	// All PSM values shall be ODD, that is, the least significant bit of the least
	// significant octet must be ’1’. Also, all PSM values shall have the least 
	// significant bit of the most significant octet equal to ’0’. This allows the
	// PSM field to be extended beyond 16 bits.
	if ((((struct sockaddr_l2cap*)_address)->l2cap_psm & 1) == 0)
		return B_ERROR;
	
	flowf("\n")
	memcpy(&socket->address, _address, sizeof(struct sockaddr_l2cap));
	socket->address.ss_len = sizeof(struct sockaddr_l2cap);

	fState = LISTEN;

	return B_OK;

}


status_t
L2capEndpoint::Unbind()
{
	debugf("[%ld] %p->L2capEndpoint::Unbind()\n", find_thread(NULL), this);

	return(B_OK);
}


status_t
L2capEndpoint::Listen(int backlog)
{
	if (fState != CLOSED)
		return B_BAD_VALUE;

	fAcceptSemaphore = create_sem(0, "tcp accept");
	if (fAcceptSemaphore < B_OK)
		return ENOBUFS;

	fState = LISTEN;

	return B_OK;
}


status_t
L2capEndpoint::Connect(const struct sockaddr *_address)
{
	if (_address->sa_family != AF_BLUETOOTH)
		return(EAFNOSUPPORT);

	debugf("[%ld] %p->UnixEndpoint::Connect(\"%s\")\n", find_thread(NULL), this,
		ConstSocketAddress(&gL2cap4AddressModule, _address).AsString().Data());

	const sockaddr_l2cap* address = (const sockaddr_l2cap*)_address;

	/**/
	TOUCH(address);

	return B_OK;
}


status_t
L2capEndpoint::Accept(net_socket **_acceptedSocket)
{
	debugf("[%ld] %p->UnixEndpoint::Accept()\n", find_thread(NULL), this);

	bigtime_t timeout = absolute_timeout(socket->receive.timeout);

	TOUCH(timeout);
	return B_OK;
}


ssize_t
L2capEndpoint::Send(const iovec *vecs, size_t vecCount,
	ancillary_data_container *ancillaryData)
{
	debugf("[%ld] %p->UnixEndpoint::Send(%p, %ld, %p)\n", find_thread(NULL),
		this, vecs, vecCount, ancillaryData);

	return B_OK;
}


ssize_t
L2capEndpoint::Receive(const iovec *vecs, size_t vecCount,
	ancillary_data_container **_ancillaryData, struct sockaddr *_address,
	socklen_t *_addressLength)
{
	debugf("[%ld] %p->UnixEndpoint::Receive(%p, %ld)\n", find_thread(NULL),
		this, vecs, vecCount);


	return B_OK;
}


ssize_t
L2capEndpoint::Sendable()
{
	debugf("[%ld] %p->UnixEndpoint::Sendable()\n", find_thread(NULL), this);


	return 0;
}


ssize_t
L2capEndpoint::Receivable()
{
	debugf("[%ld] %p->UnixEndpoint::Receivable()\n", find_thread(NULL), this);

	return 0;
}


L2capEndpoint*
L2capEndpoint::ForPsm(uint16 psm)
{

	L2capEndpoint*	endpoint;

	DoublyLinkedList<L2capEndpoint>::Iterator iterator = EndpointList.GetIterator();

	while (iterator.HasNext()) {

		endpoint = iterator.Next();
		if (((struct sockaddr_l2cap*)&endpoint->socket->address)->l2cap_psm == psm && 
			endpoint->fState == LISTEN) {
			// TODO endpoint ocupied, lock it!
			return endpoint;
		}
	}

	return NULL;
}

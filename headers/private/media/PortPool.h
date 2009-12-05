/*
 * Copyright 2002, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _POOL_PORT_H_
#define _POOL_PORT_H_


class PortPool {
public:
	PortPool();
	~PortPool();

	port_id GetPort();
	void PutPort(port_id port);

private:
	void Lock();
	void Unlock();

	struct PortInfo {
		port_id port;
		bool used;
	};

	PortInfo *	pool;
	int 		count;
	int 		maxcount;
	int32		locker_atom;
	sem_id		locker_sem;
};

extern PortPool *_PortPool;

#endif

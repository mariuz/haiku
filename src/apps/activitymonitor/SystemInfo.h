/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H


#include <OS.h>

class SystemInfoHandler;


class SystemInfo {
public:
						SystemInfo(SystemInfoHandler *handler=NULL);
						~SystemInfo();

			uint64		CachedMemory() const;
			uint64		UsedMemory() const;
			uint64		MaxMemory() const;

			uint32		UsedSemaphores() const;
			uint32		MaxSemaphores() const;

			uint32		UsedPorts() const;
			uint32		MaxPorts() const;

			uint32		UsedThreads() const;
			uint32		MaxThreads() const;

			uint32		UsedTeams() const;
			uint32		MaxTeams() const;

			uint32		UsedRunningApps() const;
			uint32		MaxRunningApps() const;

			bigtime_t	Time() const { return fTime; }
			uint32		CPUCount() const { return fSystemInfo.cpu_count; }
			const system_info& Info() const { return fSystemInfo; }

			uint64		NetworkReceived();
			uint64		NetworkSent();

private:
			void		_RetrieveNetwork();

	system_info			fSystemInfo;
	bigtime_t			fTime;
	bool				fRetrievedNetwork;
	uint64				fBytesReceived;
	uint64				fBytesSent;
	uint32				fRunningApps;
};

#endif	// SYSTEM_INFO_H

/*
 * Copyright 2004, Jérôme Duval jerome.duval@free.fr. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <sys/utsname.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>


int
uname(struct utsname *info)
{
	system_info systemInfo;
	const char *platform;

	if (!info) {
		errno = B_BAD_VALUE;
		return -1;
	}

	get_system_info(&systemInfo);

	strlcpy(info->sysname, "Haiku", sizeof(info->sysname));
	strlcpy(info->version, systemInfo.kernel_build_date, sizeof(info->version));
	strlcat(info->version, systemInfo.kernel_build_time, sizeof(info->version));
	snprintf(info->release, sizeof(info->release), "%lld", systemInfo.kernel_version);

	// TODO: make this better
	switch (systemInfo.platform_type) {
		case B_BEBOX_PLATFORM:
			platform = "BeBox";
			break;
		case B_MAC_PLATFORM:
			platform = "BeMac";
			break;
		case B_AT_CLONE_PLATFORM:
			platform = "BePC";
			break;
		default:
			platform = "unknown";
			break;
	}
	strlcpy(info->machine, platform, sizeof(info->machine));

	// TODO fill nodename field when we have hostname info
	strlcpy(info->nodename, "unknown", sizeof(info->nodename));

	return 0;
}


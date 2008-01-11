/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <syscalls.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>


// ToDo: implement the user/group functions for real!


gid_t 
getegid(void)
{
	return 0;
}


uid_t 
geteuid(void)
{
	return 0;
}


gid_t 
getgid(void)
{
	return 0;
}


int 
getgroups(int groupSize, gid_t groupList[])
{
	return 0;
}


uid_t 
getuid(void)
{
	return 0;
}


int 
setgid(gid_t gid)
{
	if (gid == 0)
		return 0;

	return EPERM;
}


int 
setuid(uid_t uid)
{
	if (uid == 0)
		return 0;

	return EPERM;
}


int
setegid(gid_t gid)
{
	if (gid == 0)
		return 0;

	return EPERM;
}


int
seteuid(uid_t uid)
{
	if (uid == 0)
		return 0;

	return EPERM;
}


/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef SYSLOG_DAEMON_H
#define SYSLOG_DAEMON_H


#include <OS.h>


#define SYSLOG_PORT_NAME	"syslog_daemon"
#define SYSLOG_MESSAGE		'_Syl'

// This message is sent from both, the POSIX syslog API and the kernel's
// dprintf() logging facility if logging to syslog was enabled.

struct syslog_message {
	thread_id	from;
	time_t		when;
	int32		options;
	char		ident[B_OS_NAME_LENGTH];
	char		message[1];
};

#endif	/* SYSLOG_DAEMON_H */

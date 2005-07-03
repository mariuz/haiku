// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2002-2005, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the MIT license.
//
//  File:        shutdown.cpp
//  Author:      Francois Revol (mmu_man@users.sf.net)
//  Description: shuts down the system, either halting or rebooting.
//
//  Notes:
//  This program behaves identically as the BeOS R5 version, with these 
//  added arguments:
//  
//  -c cancels any running shutdown
//
//  Some code from Shard's Archiver from BeBits (was BSD/MIT too :).
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <syscalls.h>

#include <OS.h>
#include <Roster.h>
#include <RosterPrivate.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

uint32 gTimeToSleep = 0;
bool gReboot = false;

// we get here when shutdown is cancelled.
// then sleep() returns

void
handle_usr1(int sig)
{
	while (0);
}


bool
parseTime(char *arg, char *argv, int32 *_i)
{
	char *unit;

	if (isdigit(arg[0])) {
		gTimeToSleep = strtoul(arg, &unit, 10);
	} else if (argv && isdigit(argv[0])) {
		(*_i)++;
		gTimeToSleep = strtoul(argv, &unit, 10);
	} else
		return false;

	if (unit[0] == '\0' || !strcmp(unit, "s"))
		return true;
	if (!strcmp(unit, "m")) {
		gTimeToSleep *= 60;
		return true;
	}

	return false;
}


void
usage(const char *arg0)
{
	const char *program = strrchr(arg0, '/');
	if (program == NULL)
		program = arg0;
	else
		program++;

	fprintf(stderr, "usage: %s [-rqca] [-d time]\n"
		"\t-r reboot,\n"
		"\t-q quick shutdown (don't broadcast apps),\n"
		"\t-c cancel a running shutdown,\n"
		"\t-d delay shutdown by <time> seconds.\n", program);
	exit(1);
}


int
main(int argc, char **argv)
{
	bool quick = false;

	for (int32 i = 1; i < argc; i++) {
		char *arg = argv[i];
		if (arg[0] == '-') {
			if (!isalpha(arg[1]))
				usage(argv[0]);

			while (arg && isalpha((++arg)[0])) {
				switch (arg[0]) {
					case 'q':
						quick = true;
						break;
					case 'r':
						gReboot = true;
						break;
					case 'c':
					{
						// find all running shutdown command and signal its shutdown thread
	
						thread_info threadInfo;
						get_thread_info(find_thread(NULL), &threadInfo);
			
						team_id thisTeam = threadInfo.team;
			
						int32 team_cookie = 0;
						team_info teamInfo;
						while (get_next_team_info(&team_cookie, &teamInfo) == B_OK) {
							if (strstr(teamInfo.args, "shutdown") != NULL && teamInfo.team != thisTeam) {
								int32 thread_cookie = 0;
								while (get_next_thread_info(teamInfo.team, &thread_cookie, &threadInfo) == B_OK) {
									if (!strcmp(threadInfo.name, "shutdown"))
										kill(threadInfo.thread, SIGUSR1);
								}
							}
						}
						exit(0);
						break;
					}
					case 'd':
						if (parseTime(arg + 1, argv[i + 1], &i)) {
							arg = NULL;
							break;
						}
						// supposed to fall through

					default:
						usage(argv[0]);
				}
			}
		} else
			usage(argv[0]);
	}

	if (gTimeToSleep > 0) {
		int32 left;

		signal(SIGUSR1, handle_usr1);

		printf("Delaying %s by %lu seconds...\n", gReboot ? "reboot" : "shutdown", gTimeToSleep);

		left = sleep(gTimeToSleep);

		if (left > 0) {
			fprintf(stderr, "Shutdown cancelled.\n");
			exit(0);
		}
	}

	if (quick) {
		#ifdef __HAIKU__
		_kern_shutdown(gReboot);
		#endif // __HAIKU__
		fprintf(stderr, "Shutdown failed!\n");
		return 2;
	} else {
		BRoster roster;
		BRoster::Private rosterPrivate(roster);
		status_t error = rosterPrivate.ShutDown(gReboot);
		fprintf(stderr, "Shutdown failed: %s\n", strerror(error));
		return 2;
	}

	return 0;
}


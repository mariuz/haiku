/*
 *  Copyright (c) 2003, Intel Corporation. All rights reserved.
 *  Created by:  crystal.xiong REMOVE-THIS AT intel DOT com
 *  This file is licensed under the GPL license.  For the full content
 *  of this license, see the COPYING file at the top level of this
 *  source tree.
 */

/* Sun Mode
 * There are several threads that share a mutex, when the owner of mutex is 
 * dead, a waiter locks the mutex and will get EOWNERDEAD. In 
 * PTHREAD_MUTEX_ROBUST_SUN_NP Mode, if the owner didn't call 
 * pthread_mutex_consistent_np(), the mutex will be set to 
 * ENOTRECOVERABLE state when unlocked automatically.
 */ 
 
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "test.h"

#define THREAD_NUM		2	

pthread_mutex_t	mutex;

void *thread_1(void *arg)
{
 	pthread_mutex_lock(&mutex);
	DPRINTF(stdout,"Thread 1 locked the mutex\n");
	pthread_exit(NULL);
	return NULL;
}
void *thread_2(void *arg) 
{
	int rc = 0;
	pthread_t self = pthread_self();
	int policy = SCHED_FIFO;
	struct sched_param param;
	memset(&param, 0, sizeof(param));
	param.sched_priority = sched_get_priority_min(policy);

	rc = pthread_setschedparam(self, policy, &param); 
	if (rc != 0) {
	    EPRINTF("UNRESOLVED: pthread_setschedparam: %d %s",
	            rc, strerror(rc));
	    exit(UNRESOLVED);
	}

	if (pthread_mutex_lock(&mutex) != EOWNERDEAD)  {
		EPRINTF("FAIL: pthread_mutex_lock didn't return EOWNERDEAD");
		exit(FAIL);
	}
	DPRINTF(stdout,"Thread 2 lock the mutex and return EOWNERDEAD\n");
	pthread_mutex_unlock(&mutex);
	
	if (pthread_mutex_lock(&mutex) != ENOTRECOVERABLE) {
		EPRINTF("FAIL: The mutex did not transit to ENOTRECOVERABLE"
                       "state when unlocked in Sun compatibility mode");
                pthread_mutex_unlock(&mutex);
		exit(FAIL);
	}
	pthread_exit(NULL);
	return NULL;
}

int main() 
{
	pthread_mutexattr_t attr;
	pthread_t threads[THREAD_NUM];
	pthread_attr_t threadattr;
	int rc;

	rc = pthread_mutexattr_init(&attr);
	if (rc != 0) {
		EPRINTF("UNRESOLVED: pthread_mutexattr_init %d %s", 
			rc, strerror(rc));
		return UNRESOLVED;
	}
	rc = pthread_mutexattr_setrobust_np(&attr, PTHREAD_MUTEX_ROBUST_SUN_NP);
	if (rc != 0) {
		EPRINTF("UNRESOLVED: pthread_mutexattr_setrobust_np %d %s", 
			rc, strerror(rc));
		return UNRESOLVED;
	}
	rc = pthread_mutex_init(&mutex, &attr);
	if (rc != 0) {
		EPRINTF("UNRESOLVED: pthread_mutex_init %d %s", 
			rc, strerror(rc));
		return UNRESOLVED;
	}
	rc = pthread_attr_init(&threadattr);
	if (rc != 0) {
		EPRINTF("UNRESOLVED: pthread_attr_init %d %s", 
			rc, strerror(rc));
		return UNRESOLVED;
	}
	rc = pthread_create(&threads[0], &threadattr, thread_1, NULL);
	if (rc != 0) {
		EPRINTF("UNRESOLVED: pthread_create %d %s", 
			rc, strerror(rc));
		return UNRESOLVED;
	}
	pthread_join(threads[0], NULL);
	DPRINTF(stdout,"Thread 1 exit without unlock...\n");

	rc = pthread_create(&threads[1], &threadattr, thread_2, NULL);
	if (rc != 0) {
		EPRINTF("UNRESOLVED: pthread_create %d %s", 
			rc, strerror(rc));
		return UNRESOLVED;
	}
	pthread_join(threads[1], NULL);
	DPRINTF(stdout,"Thread 2 exit... \n");

	DPRINTF(stdout,"PASS: Test PASSED\n");
	return PASS;
	 
}

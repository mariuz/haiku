/* 
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <new>

#include <string.h>

#include <syscalls.h>

#include "Debug.h"
#include "MessagingService.h"

// constructor
MessagingArea::MessagingArea()
{
}

// destructor
MessagingArea::~MessagingArea()
{
	if (fID >= 0)
		delete_area(fID);
}

// Create
status_t
MessagingArea::Create(area_id kernelAreaID, sem_id lockSem, sem_id counterSem,
	MessagingArea *&area)
{
	// allocate the object on the heap
	area = new(nothrow) MessagingArea;
	if (!area)
		return B_NO_MEMORY;

	// clone the kernel area
	area_id areaID = clone_area("messaging", (void**)&area->fHeader,
		B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, kernelAreaID);
	if (areaID < 0) {
		delete area;
		return areaID;
	}

	// finish the initialization of the object
	area->fID = areaID;
	area->fSize = area->fHeader->size;
	area->fLockSem = lockSem;
	area->fCounterSem = counterSem;
	area->fNextArea = NULL;

	return B_OK;
}

// Lock
bool
MessagingArea::Lock()
{
	// benaphore-like locking
	if (atomic_add(&fHeader->lock_counter, 1) == 0)
		return true;

	return (acquire_sem(fLockSem) == B_OK);
}

// Unlock
void
MessagingArea::Unlock()
{
	if (atomic_add(&fHeader->lock_counter, -1) > 1)
		release_sem(fLockSem);
}

// ID
area_id
MessagingArea::ID() const
{
	return fID;
}

// Size
int32
MessagingArea::Size() const
{
	return fSize;
}

// CountCommands
int32
MessagingArea::CountCommands() const
{
	return fHeader->command_count;
}

// PopCommand
const messaging_command *
MessagingArea::PopCommand()
{
	if (fHeader->command_count == 0)
		return NULL;

	// get the command
	messaging_command *command
		= (messaging_command*)((char*)fHeader + fHeader->first_command);

	// remove it from the area
	// (as long as the area is still locked, noone will overwrite the contents)
	if (--fHeader->command_count == 0)
		fHeader->first_command = fHeader->last_command = 0;
	else
		fHeader->first_command = command->next_command;

	return command;
}

// Discard
void
MessagingArea::Discard()
{
	fHeader->size = 0;
}

// NextKernelAreaID
area_id
MessagingArea::NextKernelAreaID() const
{
	return fHeader->next_kernel_area;
}

// SetNextArea
void
MessagingArea::SetNextArea(MessagingArea *area)
{
	fNextArea = area;
}

// NextArea
MessagingArea *
MessagingArea::NextArea() const
{
	return fNextArea;
}


// #pragma mark -

// constructor
MessagingService::MessagingService()
	: fLockSem(-1),
	  fCounterSem(-1),
	  fFirstArea(NULL),
	  fCommandProcessor(-1),
	  fTerminating(false)
{
}

// destructor
MessagingService::~MessagingService()
{
	fTerminating = true;

	if (fLockSem >= 0)
		delete_sem(fLockSem);
	if (fCounterSem >= 0)
		delete_sem(fCounterSem);

	if (fCommandProcessor >= 0) {
		int32 result;
		wait_for_thread(fCommandProcessor, &result);
	}

	delete fFirstArea;
}

// Init
status_t
MessagingService::Init()
{
	// create the semaphores
	fLockSem = create_sem(0, "messaging lock");
	if (fLockSem < 0)
		return fLockSem;

	fCounterSem = create_sem(0, "messaging counter");
	if (fCounterSem < 0)
		return fCounterSem;

	// spawn the command processor
	fCommandProcessor = spawn_thread(MessagingService::_CommandProcessorEntry,
		"messaging command processor", B_DISPLAY_PRIORITY, this);
	if (fCommandProcessor < 0)
		return fCommandProcessor;

	// register with the kernel
	area_id areaID = _kern_register_messaging_service(fLockSem, fCounterSem);
	if (areaID < 0)
		return areaID;

	// create the area
	status_t error = MessagingArea::Create(areaID, fLockSem, fCounterSem,
		fFirstArea);
	if (error != B_OK) {
		_kern_unregister_messaging_service();
		return error;
	}

	return B_OK;
}

// _CommandProcessorEntry
int32
MessagingService::_CommandProcessorEntry(void *data)
{
	return ((MessagingService*)data)->_CommandProcessor();
}

// _CommandProcessor
int32
MessagingService::_CommandProcessor()
{
	while (!fTerminating) {
		// wait for the next command
		status_t error = acquire_sem(fCounterSem);
		if (error != B_OK)
			continue;

		// get it from the first area
		MessagingArea *area = fFirstArea;
		area->Lock();
		while (area->CountCommands() > 0) {
			const messaging_command *command = area->PopCommand();
			if (!command) {
				// something's seriously wrong
				ERROR(("MessagingService::_CommandProcessor(): area %p (%ld) "
					"has command count %ld, but doesn't return any more "
					"commands.", area, area->ID(), area->CountCommands()));
				break;
			}

			// dispatch the command
			// TODO: ...
		}

		// there is a new area we don't know yet
		if (!area->NextArea() && area->NextKernelAreaID() >= 0) {
			// create it
			MessagingArea *nextArea;
			status_t error = MessagingArea::Create(area->NextKernelAreaID(),
				fLockSem, fCounterSem, nextArea);
			if (error == B_OK) {
				area->SetNextArea(nextArea);
			} else {
				// Bad, but what can we do?
				ERROR(("MessagingService::_CommandProcessor(): Failed to clone "
					"kernel area %ld: %s\n", area->NextKernelAreaID(),
					strerror(error)));
			}

		}

		// if the current area is empty and there is a next one, we discard the
		// current one
		if (area->NextArea() && area->CountCommands() == 0) {
			fFirstArea = area->NextArea();
			area->Discard();
			area->Unlock();
			delete area;
		} else {
			area->Unlock();
		}
	}

	return 0;
}


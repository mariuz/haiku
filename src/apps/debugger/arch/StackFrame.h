/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STACK_FRAME_H
#define STACK_FRAME_H

#include <OS.h>

#include <Referenceable.h>
#include <util/DoublyLinkedList.h>

#include "ArchitectureTypes.h"


enum stack_frame_type {
	STACK_FRAME_TYPE_TOP,			// top-most frame
	STACK_FRAME_TYPE_STANDARD,		// non-top-most standard frame
	STACK_FRAME_TYPE_SIGNAL,		// signal handler frame
	STACK_FRAME_TYPE_FRAMELESS		// dummy frame for a frameless function
};


class CpuState;


class StackFrame : public Referenceable,
	public DoublyLinkedListLinkImpl<StackFrame> {
public:
	virtual						~StackFrame();

	virtual	stack_frame_type	Type() const = 0;

	virtual	CpuState*			GetCpuState() const = 0;

	virtual	target_addr_t		InstructionPointer() const = 0;
	virtual	target_addr_t		FrameAddress() const = 0;
	virtual	target_addr_t		ReturnAddress() const = 0;
	virtual	target_addr_t		PreviousFrameAddress() const = 0;
};


typedef DoublyLinkedList<StackFrame> StackFrameList;


#endif	// STACK_FRAME_H

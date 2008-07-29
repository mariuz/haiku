/*
 * Copyright (C) 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "TestWindow.h"

#include "Test.h"


TestView::TestView(BRect frame, Test* test, drawing_mode mode,
		const BMessenger& target)
	: BView(frame, "test view", B_FOLLOW_ALL, B_WILL_DRAW),
	  fTest(test),
	  fTarget(target)
{
	SetDrawingMode(mode);
}


void
TestView::AttachedToWindow()
{
	fTest->Prepare(this);
}


void
TestView::Draw(BRect updateRect)
{
	if (fTest->RunIteration(this)) {
		Invalidate();
		return;
	}

	fTarget.SendMessage(MSG_TEST_FINISHED);
}


TestWindow::TestWindow(BRect frame, Test* test, drawing_mode mode,
		const BMessenger& target)
	: BWindow(frame, "Test Window", B_TITLED_WINDOW_LOOK,
		B_FLOATING_ALL_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE),
	  fTarget(target),
	  fAllowedToQuit(false)
{
	TestView* view = new TestView(Bounds(), test, mode, target);
	AddChild(view);
	Show();
}


bool
TestWindow::QuitRequested()
{
	if (fAllowedToQuit)
		return true;

	fTarget.SendMessage(MSG_TEST_CANCELED);
	return false;
}


void
TestWindow::SetAllowedToQuit(bool allowed)
{
	fAllowedToQuit = allowed;
}


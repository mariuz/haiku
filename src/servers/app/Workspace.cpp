/*
 * Copyright 2005-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "Desktop.h"
#include "Workspace.h"
#include "WorkspacePrivate.h"
#include "WindowLayer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>


static RGBColor kDefaultColor = RGBColor(51, 102, 152);


Workspace::Private::Private()
{
	_SetDefaults();
}


Workspace::Private::~Private()
{
}


void
Workspace::Private::SetDisplaysFromDesktop(Desktop* desktop)
{
}


void
Workspace::Private::SetColor(const RGBColor& color)
{
	fColor = color;
}


void
Workspace::Private::RestoreConfiguration(const BMessage& settings)
{
	rgb_color color;
	if (settings.FindInt32("color", (int32 *)&color) == B_OK)
		fColor.SetColor(color);
}


/*!
        \brief Store the workspace configuration in a message
*/
void
Workspace::Private::StoreConfiguration(BMessage& settings)
{
	rgb_color color = fColor.GetColor32();
	settings.RemoveName("color");
	settings.AddInt32("color", *(int32 *)&color);
}


void
Workspace::Private::_SetDefaults()
{
	fColor.SetColor(kDefaultColor);
}


//	#pragma mark -


Workspace::Workspace(Desktop& desktop, int32 index)
	:
	fWorkspace(desktop.WorkspaceAt(index)),
	fDesktop(desktop),
	fCurrentWorkspace(index == desktop.CurrentWorkspace())
{
//	fDesktop.LockSingleWindow();
		// TODO: in which threads is this being used?
		// from my investigations, it is used in the
		// WorkspacesLayer::Draw(), which would have
		// to hold the read lock already
	RewindWindows();
}


Workspace::~Workspace()
{
//	fDesktop.UnlockSingleWindow();
}


const RGBColor&
Workspace::Color() const
{
	return fWorkspace.Color();
}


void
Workspace::SetColor(const RGBColor& color, bool makeDefault)
{
	if (color == Color())
		return;

	fWorkspace.SetColor(color);
	fDesktop.RedrawBackground();
	if (makeDefault)
		fDesktop.StoreWorkspaceConfiguration(fWorkspace.Index());
}


status_t
Workspace::GetNextWindow(WindowLayer*& _window, BPoint& _leftTop)
{
	if (fCurrent == NULL)
		fCurrent = fWorkspace.Windows().FirstWindow();
	else
		fCurrent = fCurrent->NextWindow(fWorkspace.Index());

	if (fCurrent == NULL)
		return B_ENTRY_NOT_FOUND;

	_window = fCurrent;

	if (fCurrentWorkspace)
		_leftTop = fCurrent->Frame().LeftTop();
	else
		_leftTop = fCurrent->Anchor(fWorkspace.Index()).position;

	return B_OK;
}


void
Workspace::RewindWindows()
{
	fCurrent = NULL;
}


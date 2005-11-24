//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Workspace.cpp
//	Author:			Adi Oanca <adioanca@cotty.iren.ro>
//	Description:	Tracks windows inside one workspace
//  
//  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  Notes:			IMPORTANT WARNING
//					This object does not use any locking mechanism. It is designed
//					to be used by RootLayer class ONLY. DO NOT USE from elsewhere!
//
//	Design Spec:
//			The purpose of this class it to have visible windows appear in the
// correct order as defined by GUI guidelines. Those define 3 main type of windows:
// normal windows, modal and floating windows. Aditionally there must be support
// for another 2 types of windows which will be used in special cases. We're talking
// about system front windows which always will be the front-most and have focus, and
// system last windows which will always be behind all other kind of windows.
//
//			Normal windows will always be in Workspace's list, be it they are hidden
// or not. They are added by hand (AddWindowLayer()) only. Same goes for system last
// windows, system first, modall all and floating all windows. Those remaining
// are: modal subset, modal app, floating subset and floating app. Those will be
// added and removed on-the-fly as they are needed.
//
//			To keep correct track of modal subset/app and floating subset/app windows
// we need to have them in a list. Also, we want every normal window to
// maintain its floating windows order. For that we define a list for every normal
// window in which we'll add floating subset & app windows alongside with subset
// modals. Application modals go in a separate list which is hold for every
// application (ServerApp) object. For normal window's list: when a floating win
// is need (when normal window becomes front) it is always removed and automaticaly
// added to workspace's list. When that normal looses front state, it reinserts
// all floating windows it has used back into its list, thus saving the exact order
// of floating windows. Modal windows are never removed from normal or application's
// list, but they are automaticaly added and removed from workspace's list as needed.
// (ex: a modal app won't appear until a normal window from the same app is visible)
//
//			Front status is a bit hard to explain. It's a state which helps keep
// the wanted order. For example, if a modal window is set to have front status,
// an automatic search is made to see if another modal was created after this
// one and thus needs to have front state (read: thus have to be in front this).
// Another use is to have floating windows pop up in front of a normal window when
// no modal exist to steal front state.
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <Window.h>
#include <TypeConstants.h>

#include "Workspace.h"
#include "Layer.h"
#include "WindowLayer.h"
#include "ServerWindow.h"
#include "ServerApp.h"
#include "RGBColor.h"
#include "SubWindowList.h"

//#define DEBUG_WORKSPACE

#ifdef DEBUG_WORKSPACE
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef DEBUG_WORKSPACE
#	include <stdio.h>
#	define STRACESTREAM() PrintToStream()
#else
#	define STRACESTREAM() ;
#endif

void
Workspace::State::PrintToStream()
{
	printf("WS::State - Front: %s\n", Front? Front->Name(): "NULL");
	printf("WS::State - Focus: %s\n", Focus? Focus->Name(): "NULL");
	printf("WS::State - Active: %s\n", Active? Active->Name(): "NULL");

	for (int32 i = 0; i < WindowList.CountItems(); ++i) {
		Layer *l = (Layer*)WindowList.ItemAt(i);
		if (l)
			printf("\t %ld - %s\n", i, l->Name());
	}
}

//----------------------------------------------------------------------------------
/*!
	\brief	Creates a new Workspace object which has its own resolution and background color.
*/
Workspace::Workspace(const int32 ID, const uint32 colorspace, const RGBColor& BGColor)
	: fBGColor(BGColor)
{
STRACE(("New Workspace(%ld)\n", fID));
	fID			= ID;
	
	fBottomItem	= NULL;
	fTopItem	= NULL;
	fFocusItem	= NULL;
	fFrontItem	= NULL;
	
	memset(&fDisplayMode, 0, sizeof(fDisplayMode));
	fDisplayMode.space = colorspace;
}

//----------------------------------------------------------------------------------
/*!
	\brief	Frees internal data.
*/
Workspace::~Workspace(void)
{
STRACE(("~Workspace(%ld) - say bye bye\n", fID));
	fFocusItem		= NULL;
	fFrontItem		= NULL;

	ListData		*toast;
	while(fBottomItem)
	{
		toast		= fBottomItem;
		fBottomItem	= fBottomItem->upperItem;
		
		fPool.ReleaseMemory(toast);
	}
	fBottomItem		= NULL;
	fTopItem		= NULL;
}


/*!
	\brief	Adds layer ptr to workspace's list of WindowLayers.
*/
void
Workspace::AddWindowLayer(WindowLayer *winBorder)
{
STRACE(("W(%ld)::AddWindowLayer(%s)\n", fID, winBorder?winBorder->Name():"NULL"));
	if (winBorder->Level() == B_FLOATING_APP) {
		// floating windows are automaticaly added when needed
		// you cannot add one by hand.
		return;
	}

	if (HasItem(winBorder)) {
		// NOTE: you may remove 'debugger' at Release Candidate time
		debugger("WindowLayer ALREADY in Workspace's list\n");
		return;
	}

	// allocate a new item
	ListData* item = fPool.GetCleanMemory(winBorder);

	// place this winBorder in what seems the most appropriate location.
	// Do not change front window
	placeInFront(item, false);
}


/*
	\brief	Removes a WindowLayer from workspace's list.
*/
void
Workspace::RemoveWindowLayer(WindowLayer *winBorder)
{
	STRACE(("W(%ld)::RemoveWindowLayer(%s)\n", fID, winBorder?winBorder->Name():"NULL"));
	ListData* item = HasItem(winBorder);

	if (item) {
		RemoveItem(item);
		fPool.ReleaseMemory(item);
	}
}


bool
Workspace::HasWindowLayer(const WindowLayer* winBorder) const
{
	return HasItem(winBorder) ? true: false;
}


WindowLayer *
Workspace::Focus() const
{
	return fFocusItem ? fFocusItem->layerPtr : NULL;
}


WindowLayer *
Workspace::Front() const
{
	return fFrontItem ? fFrontItem->layerPtr: NULL;
}


WindowLayer *
Workspace::Active() const
{
	// in case of a normal or modal window
	if (fFrontItem && fFrontItem == fFocusItem)
		return fFrontItem->layerPtr;

	// a floating window is considered active if it has focus.
	if (fFocusItem &&  (fFocusItem->layerPtr->Level() == B_FLOATING_APP ||
						fFocusItem->layerPtr->Level() == B_FLOATING_ALL))
		return fFocusItem->layerPtr;

	return NULL;
}

/*!
	\brief Method that returns the state of window manager.
	\param state - a pointer to a valid Workspace::State structure
	\return void

	Fills the state structure with the most important window manager attibutes:
front window, focus window, active window and the list of windows starting from
the backmost one at position 0 and ending with the most visible window.
*/
void
Workspace::GetState(Workspace::State *state) const
{
	state->Front = Front();
	state->Focus = Focus();
	state->Active = Active();

	ListData *cursor = fTopItem;
	while (cursor) {
		if (!cursor->layerPtr->IsHidden())
			state->WindowList.AddItem(cursor->layerPtr);
		cursor = cursor->lowerItem;
	}
}
bool
Workspace::AttemptToSetFront(WindowLayer *newFront)
{
	return MoveToFront(newFront);
}

int32
Workspace::AttemptToSetFocus(WindowLayer *newFocus)
{
	ListData* newFocusItem = HasItem(newFocus);

	return _SetFocus(newFocusItem);
}

bool
Workspace::AttemptToMoveToBack(WindowLayer *newBack)
{
	return MoveToBack(newBack);
}

bool
Workspace::AttemptToActivate(WindowLayer *toActivate)
{
	MoveToFront(toActivate);
	AttemptToSetFocus(toActivate);
	return Active() == toActivate;
}
/*
	\brief	This method provides you the list of visible windows in this workspace.
	\param	list The list of visible WindowLayers found in this workspace.
	\param	itemCount Number of WindowLayer pointers found in the list.
*/
bool
Workspace::GetWindowLayerList(void **list, int32 *itemCount ) const
{
	int32 count = 0;
	ListData* cursor;

	cursor = fBottomItem;
	while (cursor) {
		if (!cursor->layerPtr->IsHidden())
			count++;
		cursor = cursor->upperItem;
	}

	if (*itemCount < count) {
		// buffer not big enough. buffer must be count high
		*itemCount = count;
		return false;
	}

	if (list) {
		*itemCount = count;

		cursor = fBottomItem;
		while (cursor) {
			if (!cursor->layerPtr->IsHidden()) {
				*list = cursor->layerPtr;
				list++;
			}
			cursor = cursor->upperItem;
		}
	}

	return true;
}

/*!
	\brief	Makes the specified WindowLayer the focus one.
	\param	newFocus WindowLayer which will try to take focus state.
	\return	0 - setting focus failed, focus did not change.
			1 - the new focus WindowLayer is \a winBorder
			2 - focus changed but not to \a winBorder because in front of it there
				are other modal windows.

	Set a new focus WindowLayer if possible.
*/

int32
Workspace::_SetFocus(ListData *newFocusItem)
{
	if (!newFocusItem || newFocusItem == fFocusItem 
			|| (newFocusItem && !newFocusItem->layerPtr->IsHidden()
				&& newFocusItem->layerPtr->WindowFlags() & B_AVOID_FOCUS))
		return 0L;

	WindowLayer *newFocus = newFocusItem->layerPtr;
	bool rv = 1;

	switch(newFocus->Level()) {
		case B_MODAL_APP:
		case B_NORMAL: {
			ListData *item = newFocusItem->upperItem;
			while (	item &&
					!item->layerPtr->IsHidden() &&
					((item->layerPtr->Level() == B_MODAL_APP &&
						item->layerPtr->App()->ClientTeam() == newFocus->App()->ClientTeam()) ||
					 item->layerPtr->Level() >= B_MODAL_ALL))
			{
				if (item->layerPtr->WindowFlags() & B_AVOID_FOCUS)
					newFocusItem = NULL;
				else
					newFocusItem = item;
				rv = 2;
			}
		}
		break;

		case B_SYSTEM_FIRST:
		case B_MODAL_ALL: {
			ListData *item = newFocusItem->upperItem;			
			while (	item &&
					!item->layerPtr->IsHidden() &&
					item->layerPtr->Level() >= newFocus->Level())
			{
				if (item->layerPtr->WindowFlags() & B_AVOID_FOCUS)
					newFocusItem = NULL;
				else
					newFocusItem = item;
				rv = 2;
			}
		}
		break;

		default:
		break;
	}

	fFocusItem = newFocusItem;

	return rv;
}

/*!
	\brief	Makes the specified WindowLayer the front one.
	\param	newFront WindowLayer which will try to take front state.
	\param	doNotDisturb In case user is busy typing something, don't bring \a newFront
			in front stealing front&focus, but place imediately after.
	\return	True if the list of WindowLayers has changed, false otherwise.

	This method tries to make \a newFront the new front WindowLayer. "It tries" because
	if this a B_NORMAL window with subset or application modals those will be displayed
	in front and get the front state. If no subset or application modals exist, then this
	B_NORMAL window will get front (and focus) state and subset and application floating
	window will be shown in front.
	Note that floating windows cannot get/have front state.
*/
bool
Workspace::MoveToFront(WindowLayer *newFront, bool doNotDisturb)
{
	STRACE(("\nWks(%ld)::MoveToFront ~%s~ \n", fID, newFront?newFront->Name():"NULL"));
	if (!newFront)
		return false;

	if (newFront->IsHidden() || newFront->Level() == B_SYSTEM_LAST)
		return false;

	if (fFrontItem && newFront == fFrontItem->layerPtr) {
		// we didn't change windows order
		return false;
	}

	return ShowWindowLayer(newFront);
}


/*!
	\brief	Moves the specified WindowLayer in the back as it is possible.
	\param	newLast WindowLayer which will be placed in the back.
	\return	True if the list of WindowLayers has changed, false otherwise.

		WindowLayer \a newLast will go in the back as much as possible. Note that this
	action is tricky. While normal windows will always go into the back, front modal windows
	won't go into the back if the next front window will be a B_NORMAL or B_MODAL_APP part
	of the same team which was previously created. If it were possible it would
	undermine the role of modal windows in the system. Another example regards B_FLOATING_APP
	windows. These will ge in the back as possible, but never farther than the front
	B_NORMAL window in front of which they appear.
*/
bool
Workspace::MoveToBack(WindowLayer *newLast)
{
	STRACE(("Wks(%ld)::MoveToBack(%s) \n", fID, newLast? newLast->Name(): "NULL"));
	if (newLast->IsHidden())
		return false;

	ListData		*newLastItem;
	newLastItem		= HasItem(newLast);
	if (!newLastItem)
		return false;

	ListData	*previous = newLastItem->upperItem;
	bool		returnValue	= false;
	bool		changeFront	= false;
	int32		level		= newLast->Level();
	if (level > B_SYSTEM_FIRST)
		level = B_SYSTEM_FIRST;

	if (fFrontItem && fFrontItem == newLastItem)
		changeFront = true;

	switch(newLast->Level())
	{
		case B_MODAL_ALL:
		case B_SYSTEM_FIRST:
		case B_SYSTEM_LAST:
		{
			// these kind of windows do not change position
			return false;
		}
		break;
		case B_NORMAL:
		{
			ListData	*cursor = newLastItem->upperItem;

			// we are already the back-most window.
			if (!cursor || cursor->layerPtr->Level() == B_SYSTEM_LAST )
				return false;

			if (changeFront)
				saveFloatingWindows(fFrontItem);
		}
		// NOTE: no 'break;' here...
		case B_FLOATING_APP:
		case B_FLOATING_ALL:
		{
			returnValue = placeToBack(newLastItem);
		}
		break;
		case B_MODAL_APP:
		{
			ListData	*cursor = newLastItem->upperItem;

			// we are already the back-most window.
			if (!cursor || cursor->layerPtr->Level() == B_SYSTEM_LAST )
				return false;

			// this is a modal app
			if (newLast->App()->fAppSubWindowList.HasItem(newLast)) {
				ListData* before;

				// remove now to properly place later
				RemoveItem(newLastItem);

				while (cursor) {
					if (!cursor->layerPtr->IsHidden()
						&& cursor->layerPtr->Level() > B_SYSTEM_LAST
						&& cursor->layerPtr->App()->ClientTeam() == newLast->App()->ClientTeam())
						break;

					cursor	= cursor->upperItem;
				}

				if (cursor)
					before	= cursor->lowerItem;
				else
					before	= fTopItem;

				InsertItem(newLastItem, before);
			} else {
				// this is a modal subset
				// this subset modal is visible, it means its main window must be visible. search for it.
				ListData	*mainWindowItem, *before;
				int32		indexThis = 0, indexCursor;

				// search by going deep
				while (cursor) {
					if (cursor->layerPtr->Level() == B_NORMAL && !cursor->layerPtr->IsHidden()
						&& (indexThis = cursor->layerPtr->fSubWindowList.IndexOf(newLast)) >= 0)
						break;
					cursor	= cursor->upperItem;
				}
				
				if (!cursor)
					debugger("MoveToBack: SubsetWindow: can't find main Window!\n");

				RemoveItem(newLastItem);

				// good. found our main window. now go up and properly place.
				mainWindowItem = cursor;
				before = cursor->lowerItem;

				cursor = cursor->lowerItem;
				while (cursor) {
					if (cursor->layerPtr->Level() == B_MODAL_APP && !cursor->layerPtr->IsHidden()
						&& cursor->layerPtr->App()->ClientTeam() == newLast->App()->ClientTeam())
					{
						indexCursor	= mainWindowItem->layerPtr->fSubWindowList.IndexOf(cursor->layerPtr);
						if (indexCursor >= 0)
						{
							if (indexThis < indexCursor)
							{
								before = cursor;
								break;
							}
							else
							{
								before = cursor->lowerItem;
							}
						}
					}
					cursor	= cursor->lowerItem;
				}

				InsertItem(newLastItem, before);
			}
			returnValue = true;
		}
		break;
		default:
		{
			debugger("MoveToBack: unknown window feel\n");
			return false;
		}
	}

	if (previous == newLastItem->upperItem)
		returnValue = false;

	// The following applies ONLY to B_NORMAL and B_MODAL_APP windows.

	if (changeFront)
	{
		ListData	*newFront;
		newFront	= findNextFront();
		if (newFront)
			returnValue |= MoveToFront(newFront->layerPtr);
		else
			debugger("MoveToBack: can't find new front! We should find one!\n");
	}

	return returnValue;
}


/*!
	\brief	Hides a WindowLayer.
	\param	winBorder WindowLayer to be hidden.
	\return	True if the list of WindowLayers has changed, false otherwise.

	WindowLayer \a winBorder will be hidden. Some, like floating or subset modals
	may also be removed from Workspace's list.
	If \a winBorder if the front WindowLayer, another one (or none) will be automaticaly
	chosen. Same goes for focus.		
*/
bool
Workspace::HideWindowLayer(WindowLayer *winBorder)
{
	STRACE(("W(%ld)::HideWindowLayer(%s) \n", fID, winBorder? winBorder->Name(): "NULL"));
	bool returnValue = false;
	int32 level = winBorder->Level();
	bool changeFront = false;
	bool changeFocus = false;
	ListData* nextFocus = NULL;

	if (fFrontItem && fFrontItem->layerPtr == winBorder)
		changeFront = true;

	if (fFocusItem && fFocusItem->layerPtr == winBorder) {
		changeFocus = true;
		nextFocus = fFocusItem->lowerItem;
	}

	if (level > B_SYSTEM_FIRST)
		level = B_SYSTEM_FIRST;

	switch (level) {
		case B_MODAL_ALL:
		case B_SYSTEM_FIRST:
		case B_SYSTEM_LAST:
		case B_FLOATING_ALL:
			// window is just hidden. do nothing. its position is OK as it is now.
			returnValue = true;
			break;

		case B_FLOATING_APP:
			if (fFrontItem && fFrontItem->layerPtr->Level() == B_NORMAL) {
				ListData* item = HasItem(winBorder);
				if (item) {
					fFrontItem->layerPtr->fSubWindowList.AddWindowLayer(winBorder);

					RemoveItem(item);
					fPool.ReleaseMemory(item);

					returnValue = true;
				}
			}
			break;

		case B_NORMAL:
		{
			if (fFrontItem && fFrontItem->layerPtr == winBorder)
				saveFloatingWindows(fFrontItem);

			// remove B_MODAL_SUBSET windows present before this window.
			ListData* itemThis = HasItem(winBorder);
			ListData* toast;
			ListData* item = itemThis->lowerItem;
			while (item) {
				// if this modal subset is in our list ONLY (not in other visible normal window's one),
				// then remove from Workspace's list.
				if (item->layerPtr->Level() == B_MODAL_APP) {
					if (winBorder->fSubWindowList.HasItem(item->layerPtr)) {
						if (!searchFirstMainWindow(item->layerPtr)) {
							// if this modal subset has front state, make sure another window will get that status.
							if (fFrontItem == item)
								changeFront = true;

							toast = item;
							item = item->lowerItem;
							RemoveItem(toast);
							fPool.ReleaseMemory(toast);
						}
					} else if (!searchANormalWindow(item->layerPtr)
						&& !(item->layerPtr->Workspaces() & (0x00000001 << fID))) {
						// if this modal subset has front state, make sure another window will get that status.
						if (fFrontItem == item)
							changeFront = true;

						toast = item;
						item = item->lowerItem;
						RemoveItem(toast);
						fPool.ReleaseMemory(toast);
					} else
						item = item->lowerItem;
				} else
					item = item->lowerItem;
			}

			returnValue = true;
			break;
		}

		case B_MODAL_APP:
		{
			// if a subset modal, then remove from Workspace's list.
			if (winBorder->App()->fAppSubWindowList.HasItem(winBorder)) {
				ListData* toast = HasItem(winBorder);
				if (toast) {
					RemoveItem(toast);
					fPool.ReleaseMemory(toast);

					returnValue = true;
				}
			}
			break;
		}

		default:
			debugger("HideWindowLayer: what kind of window is this?\n");
	}

	// select a new Front if needed
	if (changeFront) {
		fFrontItem = NULL;

		ListData* newFront = findNextFront();
		if (newFront)
			MoveToFront(newFront->layerPtr);
	}

	if (!HasItem(fFocusItem))
		fFocusItem = NULL;

	// floating windows can have focus state. what if this removed window is
	// the focus window? There will be no focus anymore.
	// So, start a search to set the new focus
	if (!fFocusItem || changeFocus) {
		if (!HasItem(nextFocus))
			nextFocus = NULL;

		if (nextFocus == NULL) {
			nextFocus = fBottomItem;

			while (nextFocus) {
				if (!nextFocus->layerPtr->IsHidden()
					&& !(nextFocus->layerPtr->WindowFlags() & B_AVOID_FOCUS))
					break;
				else
					nextFocus = nextFocus->upperItem;
			}
		}

		fFocusItem = nextFocus;
	}

	return returnValue;
}

/*!
	\brief	Shows a WindowLayer.
	\param	winBorder WindowLayer to be show.
	\return	True if the list of WindowLayers has changed, false otherwise.

	WindowLayer \a winBorder will be shown. Other windows like floating or modal
	ones will be placed in front if needed. Front & Focus state will be given to \a winBorder
	unless a modal windows steals both.
*/
bool
Workspace::ShowWindowLayer(WindowLayer *winBorder, bool userBusy)
{
	STRACE(("W(%ld)::ShowWindowLayer(%s) \n", fID, winBorder? winBorder->Name(): "NULL"));
	bool returnValue = false;
	int32 level = winBorder->Level();
	if (level > B_SYSTEM_FIRST)
		level = B_SYSTEM_FIRST;

	// Before you go understand this method, please note that 'placeInFront' MUST be
	// called of EVERY window except B_FLOATING_APP when
	// ADDING a new window to Workspace's list!!!

	switch (level) {
		// B_MODAL_ALL, B_FLOATNG_ALL, B_SYSTEM_FIRST & B_SYSTEM_LAST
		// will be removed ONLY when are deleted!
		// ALSO, they will ALWAYS be the first/last windows in hierarchy, no matter
		// if they are hidden or not - they keep their order.

		case B_MODAL_ALL:
		case B_SYSTEM_FIRST:
		case B_SYSTEM_LAST:
		{
			// nothing special to be done. just compare indexes to see if front state will change.
			if (fFrontItem) {
				int32 reverseIndexThis, reverseIndexFront;
				ListData* itemThis = HasItem(winBorder, &reverseIndexThis);

				HasItem(fFrontItem->layerPtr, &reverseIndexFront);

				if (reverseIndexThis < reverseIndexFront) {
					if (fFrontItem->layerPtr->Level() == B_NORMAL)
						saveFloatingWindows(fFrontItem);

					fFrontItem = itemThis;
				}
			} else {
				// of course, if there is no front item, then set this one.
				fFrontItem	= HasItem(winBorder);
			}
			returnValue = true;
			break;
		}

		case B_FLOATING_ALL:
		{
			// simply relocate. A floating window can't have front state.
			ListData* itemThis = HasItem(winBorder);
			RemoveItem(itemThis);
			placeInFront(itemThis, userBusy);

			returnValue = true;
			break;
		}

		// FLOATING windows are always removed from Workspace's list when changing to a new front window.

		case B_FLOATING_APP:
		{
			// see if we have a front window which is a B_NORMAL window and who's list of floating
			// and modal windows contains our window.
			if (fFrontItem && fFrontItem->layerPtr->Level() == B_NORMAL) {
				// if this winBorder is the focus it is already the first among floating app windows.
				if (fFocusItem && fFocusItem->layerPtr == winBorder)
					break;

				ListData* itemThis = NULL;
				// remove from B_NORMAL's list.
				if (fFrontItem->layerPtr->fSubWindowList.RemoveItem(winBorder)) {
					// we need to add this window
					itemThis = fPool.GetCleanMemory(winBorder);
				} else {
					itemThis = HasItem(winBorder);
					// window is already in Workspace's list. Find and temporarly remove.
					if (itemThis)
						RemoveItem(itemThis);
				}

				if (itemThis) {
					// insert in front of other B_FLOATING_APP windows.
					ListData* item = fFrontItem->lowerItem;
					while (item && item->layerPtr->Level() == B_FLOATING_APP)
						item = item->lowerItem;

					InsertItem(itemThis, item);

					returnValue = true;
				}
			}
			break;
		}

		case B_NORMAL:
		{
			ListData* itemThis = HasItem(winBorder);

			if (!itemThis) {
				debugger("ShowWindowLayer: B_NORMAL window - cannot find specified window in workspace's list\n");
				return false;
			}

			// front status will change. if a normal window has front state,
			// remove and save floating windows order that may be above it.
			if (fFrontItem && fFrontItem->layerPtr->Level() == B_NORMAL)
				saveFloatingWindows(fFrontItem);

			RemoveItem(itemThis);

			placeInFront(itemThis, userBusy);

			ListData* newFront = itemThis;

			if (windowHasVisibleModals(winBorder))
				newFront = putModalsInFront(itemThis);

			if (fFrontItem) {
				if (!userBusy) {
					int32 revFrontItemIndex, revNewFrontIndex;

					HasItem(fFrontItem, &revFrontItemIndex);
					HasItem(newFront, &revNewFrontIndex);
					if (revNewFrontIndex < revFrontItemIndex)
						fFrontItem	= newFront;
				}
			} else {
				fFrontItem	= newFront;
			}

			if (fFrontItem->layerPtr->Level() == B_NORMAL)
				putFloatingInFront(fFrontItem);

			returnValue = true;
			break;
		}

		// MODAL windows usualy stay in Workspace's list, but they are scatered, so we must gather them
		// when needed.

		case B_MODAL_APP:
		{
			// build a list of modal windows to know what windows should be placed before this one.
			BList tempList;

			// APP modal
			if (winBorder->App()->fAppSubWindowList.HasItem(winBorder)) {
				// take only application's modals
				tempList.AddList(&winBorder->App()->fAppSubWindowList);
				if (fFrontItem
					&& fFrontItem->layerPtr->App()->ClientTeam() == winBorder->App()->ClientTeam())
					userBusy = false;
			} else {
				// SUBSET modal
				WindowLayer	*mainWindow = searchFirstMainWindow(winBorder);
				if (mainWindow) {
					// add both mainWindow's subset modals and application's modals
					tempList.AddList(&mainWindow->fSubWindowList);
					tempList.AddList(&winBorder->App()->fAppSubWindowList);
					if (fFrontItem && fFrontItem->layerPtr == mainWindow)
						userBusy = false;
				} else {
					// none of the unhiden normal windows havs this window as part of its subset.
					// as a result this window won't be added to Workspace's list for it to be shown.
					return false;
				}
			}

			// === list ready ===

			// front status will change. if a normal window has front state,
			// remove and save floating windows order that may be above it.
			if (fFrontItem && fFrontItem->layerPtr->Level() == B_NORMAL)
				saveFloatingWindows(fFrontItem);

			// find and remove the Workspace's entry for this WindowLayer.
			ListData	*itemThis;
			itemThis	= HasItem(winBorder);
			if (itemThis) {
				RemoveItem(itemThis);
			} else {
				// not found? no problem. create a new entry.
				itemThis = fPool.GetCleanMemory(winBorder);
			}

			placeInFront(itemThis, userBusy);

			// now place other modals windows above
			int32		revIndexThis, revIndexItem;
			ListData	*newFront	= itemThis;

			// find the index of this window in Workspace's list. It will be used to place higher
			// indexed windows above, if it's the case.
			HasItem(itemThis, &revIndexThis);

			{
				ListData* before = itemThis->lowerItem;
				int32 i, count;
				WindowLayer** wbList;
				ListData* itemX;
				int32 indexThisInTempList;

				indexThisInTempList	= tempList.IndexOf(winBorder);
				if (indexThisInTempList < 0)
					debugger("ShowWindowLayer: modal window: design flaw!!!\n");

				count = tempList.CountItems();
				wbList = (WindowLayer**)tempList.Items();
				for (i = indexThisInTempList; i < count; i++) {
					if (!wbList[i]->IsHidden()) {
						itemX = HasItem(wbList[i], &revIndexItem);
						if (itemX && revIndexItem > revIndexThis) {
							removeAndPlaceBefore(itemX, before);
							newFront = itemX;
						}
					}
				}
			}

			if (fFrontItem) {
				if (!userBusy) {
					int32 revFrontItemIndex, revNewFrontIndex;

					HasItem(fFrontItem, &revFrontItemIndex);
					HasItem(newFront, &revNewFrontIndex);
					if (revNewFrontIndex < revFrontItemIndex)
						fFrontItem	= newFront;
				}

				if (fFrontItem->layerPtr->Level() == B_NORMAL)
					putFloatingInFront(fFrontItem);
			} else {
				fFrontItem	= newFront;
			}

			returnValue = true;
			break;
		}

		default:
			debugger("What kind of window is this???\n");
	}

	if (!HasItem(fFocusItem))
		fFocusItem = NULL;

	// set new Focus if needed
	if (Focus() == NULL) {
		ListData* cursor = fBottomItem;

		fFocusItem = NULL;
		while (cursor != NULL && fFocusItem == NULL) {
			if (!cursor->layerPtr->IsHidden()
				&& !(cursor->layerPtr->WindowFlags() & B_AVOID_FOCUS)) {
				if (cursor->layerPtr->Level() == B_FLOATING_APP
					|| cursor->layerPtr->Level() == B_FLOATING_ALL) {
					// set focus to floating windows only if directly targeted
					if (cursor->layerPtr == winBorder) {
						fFocusItem = cursor;
					} else
						cursor = cursor->upperItem;
				} else
					fFocusItem = cursor;
			} else
				cursor = cursor->upperItem;
		}
	}

	return returnValue;
}


status_t
Workspace::SetDisplayMode(const display_mode &mode)
{
	fDisplayMode = mode;
	return B_OK;
}


status_t
Workspace::GetDisplayMode(display_mode &mode) const
{
	mode = fDisplayMode;
	return B_OK;
}


void
Workspace::SetBGColor(const RGBColor &c)
{
	fBGColor = c;
}


RGBColor
Workspace::BGColor(void) const
{
	return fBGColor;
}


/*!
	\brief Retrieves settings from a container message passed to PutSettings
	\param A BMessage containing data from a PutSettings() call
	
	This function will place default values whenever a particular setting cannot
	be found.
*/
void
Workspace::GetSettings(const BMessage &msg)
{
	BMessage container;
	rgb_color *color;
	ssize_t size;
	
	char fieldname[4];
	sprintf(fieldname,"%d",(uint8)fID);
	
	// First, find the container message corresponding to the workspace's index
	if(msg.FindMessage(fieldname,&container)!=B_OK)
	{
		GetDefaultSettings();
		return;
	}
	
	if(container.FindInt32("timing_pixel_clock",(int32*)&fDisplayMode.timing.pixel_clock) != B_OK)
		fDisplayMode.timing.pixel_clock=25175;
	if(container.FindInt16("timing_h_display",(int16*)&fDisplayMode.timing.h_display)!=B_OK)
		fDisplayMode.timing.h_display=640;
	if(container.FindInt16("timing_h_sync_start",(int16*)&fDisplayMode.timing.h_sync_start)!=B_OK)
		fDisplayMode.timing.h_sync_start=656;
	if(container.FindInt16("timing_h_sync_end",(int16*)&fDisplayMode.timing.h_sync_end)!=B_OK)
		fDisplayMode.timing.h_sync_end=752;
	if(container.FindInt16("timing_h_total",(int16*)&fDisplayMode.timing.h_total)!=B_OK)
		fDisplayMode.timing.h_total=800;
	if(container.FindInt16("timing_v_display",(int16*)&fDisplayMode.timing.v_display)!=B_OK)
		fDisplayMode.timing.v_display=480;
	if(container.FindInt16("timing_v_sync_start",(int16*)&fDisplayMode.timing.v_sync_start)!=B_OK)
		fDisplayMode.timing.v_sync_start=490;
	if(container.FindInt16("timing_v_sync_end",(int16*)&fDisplayMode.timing.v_sync_end)!=B_OK)
		fDisplayMode.timing.v_sync_end=492;
	if(container.FindInt16("timing_v_total",(int16*)&fDisplayMode.timing.v_total)!=B_OK)
		fDisplayMode.timing.v_total=525;
	if(container.FindInt32("timing_flags",(int32*)&fDisplayMode.timing.flags)!=B_OK)
		fDisplayMode.timing.flags=0;
	
	if(container.FindInt32("color_space", (int32*)&fDisplayMode.space) != B_OK)
		fDisplayMode.space = B_CMAP8;
	
	if(container.FindData("bgcolor",B_RGB_COLOR_TYPE,(const void **)&color,&size)==B_OK)
		fBGColor.SetColor(*color);
	else
		fBGColor.SetColor(0,0,0);
	
	if(container.FindInt16("virtual_width", (int16 *)&fDisplayMode.virtual_width) != B_OK)
		fDisplayMode.virtual_width = 640;
	if(container.FindInt16("virtual_height", (int16 *)&fDisplayMode.virtual_height) != B_OK)
		fDisplayMode.virtual_height = 480;
}

//----------------------------------------------------------------------------------
//! Sets workspace settings to defaults
void Workspace::GetDefaultSettings(void)
{
	fBGColor.SetColor(0,0,0);
	
	fDisplayMode.timing.pixel_clock = 25175;
	fDisplayMode.timing.h_display = 640;
	fDisplayMode.timing.h_sync_start = 656;
	fDisplayMode.timing.h_sync_end = 752;
	fDisplayMode.timing.h_total = 800;
	fDisplayMode.timing.v_display = 480;
	fDisplayMode.timing.v_sync_start = 490;
	fDisplayMode.timing.v_sync_end = 492;
	fDisplayMode.timing.v_total = 525;
	fDisplayMode.timing.flags = 0;
	fDisplayMode.space = B_CMAP8;
		
	fDisplayMode.virtual_width = 640;
	fDisplayMode.virtual_height = 480;
}

//----------------------------------------------------------------------------------
/*!
	\brief Places the screen settings for the workspace in the passed BMessage
	\param msg BMessage pointer to receive the settings
	\param index The index number of the workspace in the desktop
	
	This function will fail if passed a NULL pointer. The settings for the workspace are 
	saved in a BMessage in a BMessage.
	
	The message itself is placed in a field string based on its index - "1", "2", etc.
	
	The format is as follows:
	display_timing "timing_XXX" -> fDisplayTiming members (see Accelerant.h)
	uint32 "color_space" -> color space of the workspace
	rgb_color "bgcolor" -> background color of the workspace
	int16 "virtual_width" -> virtual width of the workspace
	int16 "virtual_height" -> virtual height of the workspace
*/
void Workspace::PutSettings(BMessage *msg, const uint8 &index) const
{
	if(!msg)
		return;
	
	BMessage container;
	rgb_color color=fBGColor.GetColor32();
	
	// a little longer than we need in case someone tries to pass index=255 or something
	char fieldname[4];
	
	container.AddInt32("timing_pixel_clock",fDisplayMode.timing.pixel_clock);
	container.AddInt16("timing_h_display",fDisplayMode.timing.h_display);
	container.AddInt16("timing_h_sync_start",fDisplayMode.timing.h_sync_start);
	container.AddInt16("timing_h_sync_end",fDisplayMode.timing.h_sync_end);
	container.AddInt16("timing_h_total",fDisplayMode.timing.h_total);
	container.AddInt16("timing_v_display",fDisplayMode.timing.v_display);
	container.AddInt16("timing_v_sync_start",fDisplayMode.timing.v_sync_start);
	container.AddInt16("timing_v_sync_end",fDisplayMode.timing.v_sync_end);
	container.AddInt16("timing_v_total",fDisplayMode.timing.v_total);
	container.AddInt32("timing_flags",fDisplayMode.timing.flags);
	
	container.AddInt32("color_space", fDisplayMode.space);
	container.AddData("bgcolor", B_RGB_COLOR_TYPE, &color, sizeof(rgb_color));
	
	container.AddInt16("virtual_width", fDisplayMode.virtual_width);
	container.AddInt16("virtual_height", fDisplayMode.virtual_height);
	
	sprintf(fieldname,"%d",index);
	
	// Just in case...
	msg->RemoveName(fieldname);
	
	msg->AddMessage(fieldname,&container);
}

//----------------------------------------------------------------------------------
/*!
	\brief Places default settings for the workspace in the passed BMessage
	\param msg BMessage pointer to receive the settings
	\param index The index number of the workspace in the desktop
*/
void Workspace::PutDefaultSettings(BMessage *msg, const uint8 &index)
{
	if(!msg)
		return;
	
	BMessage container;
	rgb_color color={ 0,0,0,255 };
	
	// a little longer than we need in case someone tries to pass index=255 or something
	char fieldname[4];
	
	// These default settings the same ones as found in ~/config/settings/
	// app_server_settings on R5
	
	container.AddInt32("timing_pixel_clock",25175);
	container.AddInt16("timing_h_display",640);
	container.AddInt16("timing_h_sync_start",656);
	container.AddInt16("timing_h_sync_end",752);
	container.AddInt16("timing_h_total",800);
	container.AddInt16("timing_v_display",480);
	container.AddInt16("timing_v_sync_start",490);
	container.AddInt16("timing_v_sync_end",492);
	container.AddInt16("timing_v_total",525);
	container.AddInt32("timing_flags",0);
	
	container.AddInt32("color_space",B_CMAP8);
	container.AddData("bgcolor",B_RGB_COLOR_TYPE,&color,sizeof(rgb_color));
	
	container.AddInt16("virtual_width",640);
	container.AddInt16("virtual_height",480);
	
	sprintf(fieldname,"%d",index);
	
	// Just in case...
	msg->RemoveName(fieldname);
	
	msg->AddMessage(fieldname, &container);
}

//----------------------------------------------------------------------------------
// Debug method

void
Workspace::PrintToStream() const
{
	printf("Workspace %ld hierarchy shown from back to front:\n", fID);
	for (ListData *item = fTopItem; item != NULL; item = item->lowerItem)
	{
		WindowLayer		*wb = (WindowLayer*)item->layerPtr;
		printf("\tName: %s\t%s", wb->Name(), wb->IsHidden()?"Hidden\t": "Visible\t");
		if(wb->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL)
			printf("\t%s\n", "B_FLOATING_SUBSET_WINDOW_FEEL");
		if(wb->Feel() == B_FLOATING_APP_WINDOW_FEEL)
			printf("\t%s\n", "B_FLOATING_APP_WINDOW_FEEL");
		if(wb->Feel() == B_FLOATING_ALL_WINDOW_FEEL)
			printf("\t%s\n", "B_FLOATING_ALL_WINDOW_FEEL");
		if(wb->Feel() == B_MODAL_SUBSET_WINDOW_FEEL)
			printf("\t%s\n", "B_MODAL_SUBSET_WINDOW_FEEL");
		if(wb->Feel() == B_MODAL_APP_WINDOW_FEEL)
			printf("\t%s\n", "B_MODAL_APP_WINDOW_FEEL");
		if(wb->Feel() == B_MODAL_ALL_WINDOW_FEEL)
			printf("\t%s\n", "B_MODAL_ALL_WINDOW_FEEL");
		if(wb->Feel() == B_NORMAL_WINDOW_FEEL)
			printf("\t%s\n", "B_NORMAL_WINDOW_FEEL");
		if(wb->Feel() == B_SYSTEM_LAST)
			printf("\t%s\n", "B_SYSTEM_LAST");
		if(wb->Feel() >= B_SYSTEM_FIRST)
			printf("\t%s\n", "B_SYSTEM_FIRST");
			
	}
	printf("Focus Layer:\t%s\n", fFocusItem? fFocusItem->layerPtr->Name(): "NULL");
	printf("Front Layer:\t%s\n\n", fFrontItem? fFrontItem->layerPtr->Name(): "NULL");
}


void
Workspace::PrintItem(ListData *item) const
{
	printf("ListData members:\n");
	if(item)
	{
		printf("WindowLayer:\t%s\n", item->layerPtr? item->layerPtr->Name(): "NULL");
		printf("UpperItem:\t%s\n", item->upperItem? item->upperItem->layerPtr->Name(): "NULL");
		printf("LowerItem:\t%s\n", item->lowerItem? item->lowerItem->layerPtr->Name(): "NULL");
	}
	else
	{
		printf("NULL item\n");
	}
}

//----------------------------------------------------------------------------------
// PRIVATE
//----------------------------------------------------------------------------------
/*
	Insert item in the top-bottom direction.
*/
void
Workspace::InsertItem(ListData *item, ListData *before)
{
	// insert before one other item;
	if (before) {
		if (before->upperItem)
			before->upperItem->lowerItem = item;

		item->upperItem = before->upperItem;
		item->lowerItem = before;
		before->upperItem = item;

		if (fTopItem == before)
			fTopItem = item;
	} else {
		// insert item at the end.
		item->upperItem = fBottomItem;
		if (fBottomItem)
			fBottomItem->lowerItem = item;

		fBottomItem = item;

		if (!fTopItem)
			fTopItem = item;
	}
}


void
Workspace::RemoveItem(ListData *item)
{
	if (!item)
		return;

	if (fBottomItem == item)
		fBottomItem = item->upperItem;
	else
		item->lowerItem->upperItem	= item->upperItem;
	
	if (fTopItem == item)
		fTopItem = item->lowerItem;
	else
		item->upperItem->lowerItem	= item->lowerItem;
	
	// set all these to NULL to avoid confusion later.

	item->upperItem	= NULL;
	item->lowerItem	= NULL;
	
	if (fFocusItem == item)
		fFocusItem = NULL;

	if (fFrontItem == item)
		fFrontItem = NULL;
}


ListData*
Workspace::HasItem(const ListData *item, int32 *index) const
{
	int32 idx = 0;
	ListData* itemX;

	for (itemX = fBottomItem; itemX != NULL; itemX = itemX->upperItem) {
		if (item == itemX)
			break;

		idx++;
	}

	if (index && itemX)
		*index = idx;

	return itemX;
}


ListData*
Workspace::HasItem(const WindowLayer *layer, int32 *index) const
{
	int32 idx = 0;
	ListData* itemX;

	for (itemX = fBottomItem; itemX != NULL; itemX = itemX->upperItem) {
		if (layer == itemX->layerPtr)
			break;

		idx++;
	}

	if (index && itemX)
		*index = idx;

	return itemX;
}


/*!
	\brief Returns the index of the specified item starting from the back-most window.
*/
int32
Workspace::IndexOf(const ListData *item) const
{
	if (!item)
		return -1;

	int32 index = 0;
	for (ListData *itemX = fTopItem; itemX != NULL; itemX = itemX->lowerItem) {
		if (itemX->layerPtr == item->layerPtr)
			return index;
		index++;
	}
	return -1;
}


inline bool
Workspace::placeToBack(ListData *newLast)
{
	int32 level = newLast->layerPtr->Level();
	ListData* cursor = newLast->upperItem;

	switch (level) {
		case B_FLOATING_ALL:
		case B_FLOATING_APP:
		{
			int32 count = 0;
			while (cursor && cursor->layerPtr->Level() == level) {
				if (!cursor->layerPtr->IsHidden())
					count++;
				cursor = cursor->upperItem;
			}

			// we're already the last floating window
			if (count == 0)
				return false;
			else {
				bool changeFocus = false;

				if (fFocusItem == newLast)
					changeFocus = true;

				if (changeFocus) {
					ListData* cursor = newLast->upperItem;
					while (cursor
						&& (cursor->layerPtr->IsHidden()
							|| cursor->layerPtr->WindowFlags() & B_AVOID_FOCUS)
						&& cursor->layerPtr->Level() == level) {
						cursor	= cursor->upperItem;
					}

					// give focus only if a unhidden floating window could be selected
					// otherwise this('newLast') window keeps focus
					if (cursor->layerPtr->Level() == level)
						fFocusItem	= cursor;
				}

				RemoveItem(newLast);
				InsertItem(newLast, cursor ? cursor->lowerItem : fTopItem);

				return true;
			}
			break;
		}

		case B_NORMAL:
		{
			int32 count = 0;
			int32 cursorLevel;
			while (cursor) {
				cursorLevel	= cursor->layerPtr->Level();
				if (cursorLevel == B_MODAL_APP)
				 	cursorLevel = B_NORMAL;

				if (cursorLevel < level) {
					break;
				} else {
					count++;
					cursor	= cursor->upperItem;
				}
			}

			// we're already the last normal window
			if (count == 0)
				return false;
			else {
				RemoveItem(newLast);
				InsertItem(newLast, cursor ? cursor->lowerItem : fTopItem);
				return true;
			}
			break;
		}
	}

	return false;
}

//----------------------------------------------------------------------------------
/*!
	\brief Based on it's WindowLayer type, places this item in front as it is possible.
*/
void
Workspace::placeInFront(ListData *item, const bool userBusy)
{
	if (!item)
		return;

	int32 level = item->layerPtr->Level();
	ListData* cursor = fBottomItem;
	int32 cursorLevel;

	// make MODAL windows act just like normal ones.
	if (level == B_MODAL_APP)
		level = B_NORMAL;

	// B_SYSTEM_LAST - always place (the most) last
	if (level == B_SYSTEM_LAST) {
		InsertItem(item, fTopItem);
		return;
	}

	// search for the exact place...
	while (cursor) {
		cursorLevel = cursor->layerPtr->Level();

		// make MODAL windows act just like normal ones.
		if (cursorLevel == B_MODAL_APP)
			cursorLevel = B_NORMAL;

		if (level < cursorLevel) {
			cursor = cursor->upperItem;
			continue;
		} else {
			// that's it, we've found the proper place.
			break;
		}
	}

	if (cursor) {
		// if user is busy typing something, or has an opened menu...
		if (userBusy && cursor == fFrontItem)
			InsertItem(item, cursor);
		else
			InsertItem(item, cursor->lowerItem);
	} else
		InsertItem(item, fTopItem);
}


inline bool
Workspace::removeAndPlaceBefore(ListData *item, ListData *beforeItem)
{
	if (item && item != beforeItem) {
		RemoveItem(item);
		// insert into proper place.
		InsertItem(item, beforeItem);
		return true;
	}
	return false;
}

/*!
	\brief Insert the specified WindowLayer before given item. First search the
			specified WindowLayer in Workspace's list an remove it.
	\resolution: private
*/
inline bool
Workspace::removeAndPlaceBefore(const WindowLayer *wb, ListData *beforeItem)
{
	return removeAndPlaceBefore(HasItem(wb), beforeItem);
}


inline WindowLayer*
Workspace::searchANormalWindow(WindowLayer *wb) const
{
	ListData* listItem = fBottomItem;
	while (listItem) {
		if (listItem->layerPtr->Level() == B_NORMAL && !listItem->layerPtr->IsHidden()
			&& listItem->layerPtr->App()->ClientTeam() == wb->App()->ClientTeam())
			return listItem->layerPtr;

		listItem = listItem->upperItem;
	}
	return NULL;
}


inline WindowLayer*
Workspace::searchFirstMainWindow(WindowLayer *wb) const
{
	ListData* listItem = fBottomItem;
	while (listItem) {
		if (listItem->layerPtr->Level() == B_NORMAL && !listItem->layerPtr->IsHidden()
			&& listItem->layerPtr->App()->ClientTeam() == wb->App()->ClientTeam()
			&& listItem->layerPtr->fSubWindowList.HasItem(wb))
			return listItem->layerPtr;

		listItem = listItem->upperItem;
	}
	return NULL;
}

//----------------------------------------------------------------------------------

inline
bool Workspace::windowHasVisibleModals(const WindowLayer *winBorder) const
{
	int32		i, count;
	WindowLayer	**wbList;

	// check window's list
	count		= winBorder->fSubWindowList.CountItems();
	wbList		= (WindowLayer**)winBorder->fSubWindowList.Items();
	for(i = 0; i < count; i++)
	{
		if (wbList[i]->Level() == B_MODAL_APP && !wbList[i]->IsHidden())
			return true;
	}

	// application's list only has modal windows.
	count		= winBorder->App()->fAppSubWindowList.CountItems();
	wbList		= (WindowLayer**)winBorder->App()->fAppSubWindowList.Items();
	for(i = 0; i < count; i++)
	{
		if (!wbList[i]->IsHidden())
			return true;
	}

	return false;
}

//----------------------------------------------------------------------------------

inline
ListData* Workspace::putModalsInFront(ListData *item)
{
	int32		i, count, revIndex, revIndexItem;
	WindowLayer	**wbList;
	ListData	*itemX;
	ListData	*lastPlaced = NULL;
	ListData	*before = item->lowerItem;

	HasItem(item, &revIndex);

	// check window's list
	count		= item->layerPtr->fSubWindowList.CountItems();
	wbList		= (WindowLayer**)item->layerPtr->fSubWindowList.Items();
	for(i = 0; i < count; i++)
	{
		if (wbList[i]->Level() == B_MODAL_APP && !wbList[i]->IsHidden())
		{
			itemX	= HasItem(wbList[i], &revIndexItem);
			if (!itemX)
			{
				itemX		= fPool.GetCleanMemory(wbList[i]);

				InsertItem(itemX, before);
				lastPlaced	= itemX;
			}
			else if (revIndexItem > revIndex)
			{
				removeAndPlaceBefore(itemX, before);
				lastPlaced	= itemX;
			}
		}
	}

	// application's list only has modal windows.
	count		= item->layerPtr->App()->fAppSubWindowList.CountItems();
	wbList		= (WindowLayer**)item->layerPtr->App()->fAppSubWindowList.Items();
	for(i = 0; i < count; i++)
	{
		if (!wbList[i]->IsHidden())
		{
			itemX	= HasItem(wbList[i], &revIndexItem);
			if (!itemX)
			{
				itemX		= fPool.GetCleanMemory(wbList[i]);

				InsertItem(itemX, before);
				lastPlaced	= itemX;
			}
			else if (revIndexItem > revIndex)
			{
				removeAndPlaceBefore(itemX, before);
				lastPlaced	= itemX;
			}
		}
	}

	return lastPlaced;
}


void
Workspace::putFloatingInFront(ListData *item)
{
	int32		i;
	ListData	*newItem;
	ListData	*before = item->lowerItem;
	WindowLayer	*wb;

	i = 0;
	while ((wb = (WindowLayer*)item->layerPtr->fSubWindowList.ItemAt(i)))
	{
		if (wb->Level() == B_MODAL_APP)
		{
			break;
		}
		else if (!wb->IsHidden())
		{
			newItem				= fPool.GetCleanMemory(wb);

			InsertItem(newItem, before);

			item->layerPtr->fSubWindowList.RemoveItem(i);
		}
		else
			i++;		
	}
}

//----------------------------------------------------------------------------------

inline
void Workspace::saveFloatingWindows(ListData *itemNormal)
{
	ListData		*item = itemNormal->lowerItem;
	ListData		*toast;
	while(item)
	{
		if (item->layerPtr->Level() == B_FLOATING_APP)
		{
			itemNormal->layerPtr->fSubWindowList.AddWindowLayer(item->layerPtr);

			toast	= item;
			item	= item->lowerItem;
			RemoveItem(toast);
			fPool.ReleaseMemory(toast);
		}
		else
			break;
	}
}

//----------------------------------------------------------------------------------

inline
ListData* Workspace::findNextFront() const
{
	ListData	*item = fBottomItem;

	while(item)
	{
		if (!item->layerPtr->IsHidden()
			&& item->layerPtr->Level() != B_FLOATING_ALL
			&& item->layerPtr->Level() != B_FLOATING_APP
			&& !(item->layerPtr->WindowFlags() & B_AVOID_FRONT))
		{
			return item;
		}
		item = item->upperItem;
	}

	// we cannot ignore anymore B_AVOID_FRONT windows.

	item	= fBottomItem;
	while(item)
	{
		if (!item->layerPtr->IsHidden()
			&& item->layerPtr->Level() != B_FLOATING_ALL
			&& item->layerPtr->Level() != B_FLOATING_APP)
		{
			return item;
		}
		item = item->upperItem;
	}

	return NULL;
}




// TODO: BAD, bad memory manager!!! replace!!!
Workspace::MemoryPool::MemoryPool()
{
}

Workspace::MemoryPool::~MemoryPool()
{
}

inline
ListData* Workspace::MemoryPool::GetCleanMemory(WindowLayer *winBorder)
{
	ListData	*item = (ListData*)malloc(sizeof(ListData));
	item->layerPtr = winBorder;
	item->upperItem = NULL;
	item->lowerItem = NULL;
	return item;
}

inline
void Workspace::MemoryPool::ReleaseMemory(ListData *mem)
{
	free(mem);
}

void Workspace::MemoryPool::expandBuffer(int32 start)
{
}

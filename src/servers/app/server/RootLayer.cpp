//------------------------------------------------------------------------------
//	Copyright (c) 2001-2003, Haiku, Inc.
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
//	File Name:		RootLayer.cpp
//	Author:			Gabe Yoder <gyoder@stny.rr.com>
//					DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@myrealbox.com>
//	Description:	Class used for the top layer of each workspace's Layer tree
//  
//------------------------------------------------------------------------------

#include <stdio.h>
#include <Window.h>
#include <List.h>
#include <Message.h>
#include <Entry.h>
#include <File.h>
#include <PortLink.h>

#include "Globals.h"
#include "RootLayer.h"
#include "Layer.h"
#include "Workspace.h"
#include "ServerScreen.h"
#include "WinBorder.h"
#include "ServerWindow.h"
#include "ServerApp.h"
#include "Desktop.h"
#include "ServerConfig.h"
#include "FMWList.h"
#include "DisplayDriver.h"
#include "ServerProtocol.h"

//#define DEBUG_ROOTLAYER

#ifdef DEBUG_ROOTLAYER
	#define STRACE(a) printf(a)
#else
	#define STRACE(a) /* nothing */
#endif

//#define DISPLAYDRIVER_TEST_HACK


RootLayer::RootLayer(const char *name, int32 workspaceCount, 
		Desktop *desktop, DisplayDriver *driver)
 : Layer(BRect(0,0,0,0), name, 0, B_FOLLOW_ALL, B_WILL_DRAW, driver)
{
	fDesktop = desktop;

	fMouseTarget		= NULL;
	fDragMessage		= NULL;
	fScreenShotIndex	= 1;

	fQuiting			= false;

	//NOTE: be careful about this one.
	fRootLayer = this;
	fActiveWorkspace = NULL;
	fRows = 0;
	fColumns = 0;
	
	// TODO: this should eventually be replaced by a method to convert the monitor
	// number to an index in the name, i.e. workspace_settings_1 for screen #1
	ReadWorkspaceData(WORKSPACE_DATA_LIST);

	// TODO: read these 3 from a configuration file.
	fScreenXResolution = 0;
	fScreenYResolution = 0;
	fColorSpace = B_RGB32;

	// easy way to identify this class.
	fClassID = AS_ROOTLAYER_CLASS;
	fHidden	= false;

	fListenPort = find_port(SERVER_INPUT_PORT);
	if (fListenPort == B_NAME_NOT_FOUND)
		return;

	// Spawn our working thread
	fThreadID= spawn_thread(WorkingThread, name, B_REAL_TIME_DISPLAY_PRIORITY, this);
	if (fThreadID >= 0)
		resume_thread(fThreadID);

}

RootLayer::~RootLayer()
{
	int32		exitValue;
	fQuiting	= true;
	BPortLink	msg(fListenPort, -1);
	msg.StartMessage(B_QUIT_REQUESTED);
	msg.EndMessage();
	msg.Flush();

	wait_for_thread(fThreadID, &exitValue);

	if (fDragMessage)
		delete fDragMessage;

	// RootLayer object just uses Screen objects, it is not allowed to delete them.
}

/*!
	\brief Thread function for handling input messages and calculating visible regions.
	\param data Pointer to the app_server to which the thread belongs
	\return Throwaway value - always 0
*/
int32 RootLayer::WorkingThread(void *data)
{
	int32		code = 0;
	status_t	err = B_OK;
	RootLayer	*oneRootLayer	= (RootLayer*)data;
	BPortLink	messageQueue(-1, oneRootLayer->fListenPort);
	
	for(;;)
	{
		STRACE(("info: RootLayer(%s)::WorkingThread listening on port %ld.\n", oneRootLayer->GetName(), oneRootLayer->fListenPort));
		err = messageQueue.GetNextReply(&code);
		
		if(err < B_OK)
		{
			STRACE(("WorkingThread: messageQueue.GetNextReply failed\n"));
			continue;
		}
		
		switch(code)
		{
			// We don't need to do anything with these two, so just pass them
			// onto the active application. Eventually, we will end up passing 
			// them onto the window which is currently under the cursor.
			case B_MOUSE_DOWN:
			case B_MOUSE_UP:
			case B_MOUSE_WHEEL_CHANGED:
			case B_MOUSE_MOVED:
				oneRootLayer->MouseEventHandler(code, messageQueue);
				break;

			case B_KEY_DOWN:
			case B_KEY_UP:
			case B_UNMAPPED_KEY_DOWN:
			case B_UNMAPPED_KEY_UP:
			case B_MODIFIERS_CHANGED:
				oneRootLayer->KeyboardEventHandler(code, messageQueue);
				break;

			case B_QUIT_REQUESTED:
				exit_thread(0);
				break;

			default:
				STRACE(("RootLayer(%s)::WorkingThread received unexpected code %lx\n",oneRootLayer->GetName(), oneRootLayer->code));
				break;
		}

		// if we still have other messages in our queue, but we really want to quit
		if (oneRootLayer->fQuiting)
			break;
	}
	return 0;
}

void RootLayer::MoveBy(float x, float y)
{
}

void RootLayer::ResizeBy(float x, float y)
{
	// TODO: implement
}

Layer* RootLayer::VirtualTopChild() const
{
	return fActiveWorkspace->GoToTopItem();
}

Layer* RootLayer::VirtualLowerSibling() const
{
	return fActiveWorkspace->GoToLowerItem();
}

Layer* RootLayer::VirtualUpperSibling() const
{
	return fActiveWorkspace->GoToUpperItem();
}

Layer* RootLayer::VirtualBottomChild() const
{
	return fActiveWorkspace->GoToBottomItem();
}

void RootLayer::ReadWorkspaceData(const char *path)
{
	BMessage msg, settings;
	BFile file(path,B_READ_ONLY);
	char string[20];
	
	if(file.InitCheck()==B_OK && msg.Unflatten(&file)==B_OK)
	{
		int32 count;
		
		if(msg.FindInt32("workspace_count",&count)!=B_OK)
			count=9;
		
		SetWorkspaceCount(count);
		
		for(int32 i=0; i<count; i++)
		{
			Workspace *ws=(Workspace*)fWorkspaceList.ItemAt(i);
			if(!ws)
				continue;
			
			sprintf(string,"workspace %ld",i);
			
			if(msg.FindMessage(string,&settings)==B_OK)
			{
				ws->GetSettings(settings);
				settings.MakeEmpty();
			}
			else
				ws->GetDefaultSettings();
		}
	}
	else
	{
		SetWorkspaceCount(9);
		
		for(int32 i=0; i<9; i++)
		{
			Workspace *ws=(Workspace*)fWorkspaceList.ItemAt(i);
			if(!ws)
				continue;
			
			ws->GetDefaultSettings();
		}
	}
}

void RootLayer::SaveWorkspaceData(const char *path)
{
	BMessage msg,dummy;
	BFile file(path,B_READ_WRITE | B_CREATE_FILE);
	
	if(file.InitCheck()!=B_OK)
	{
		printf("ERROR: Couldn't save workspace data in RootLayer\n");
		return;
	}
	
	char string[20];
	int32 count=fWorkspaceList.CountItems();

	if(msg.Unflatten(&file)==B_OK)
	{
		// if we were successful in unflattening the file, it means we're
		// going to need to save over the existing data
		for(int32 i=0; i<count; i++)
		{
			sprintf(string,"workspace %ld",i);
			if(msg.FindMessage(string,&dummy)==B_OK)
				msg.RemoveName(string);
		}
	}
		
	for(int32 i=0; i<count; i++)
	{
		sprintf(string,"workspace %ld",i);

		Workspace *ws=(Workspace*)fWorkspaceList.ItemAt(i);
		
		if(!ws)
		{
			dummy.MakeEmpty();
			ws->PutSettings(&dummy,i);
			msg.AddMessage(string,&dummy);
		}
		else
		{
			// We're not supposed to have this happen, but we'll suck it up, anyway. :P
			Workspace::PutDefaultSettings(&msg,i);
		}
	}
}

void RootLayer::AddWinBorderToWorkspaces(WinBorder* winBorder, uint32 wks)
{
	if (!(fMainLock.IsLocked()))
		debugger("RootLayer::AddWinBorderToWorkspaces - fMainLock has to be locked!\n");

	for( int32 i=0; i < 32; i++)
	{
		if( wks & (0x00000001 << i) && i < WorkspaceCount())
			WorkspaceAt(i+1)->AddWinBorder(winBorder);
	}
}

void RootLayer::AddWinBorder(WinBorder* winBorder)
{
	// The main job of this function, besides adding winBorder as a child, is to determine
	// in which workspaces to add this window.

	STRACE(("*RootLayer::AddWinBorder(%s)\n", winBorder->GetName()));
	desktop->fGeneralLock.Lock();
	
	STRACE(("*RootLayer::AddWinBorder(%s) - General lock acquired\n", winBorder->GetName()));
	fMainLock.Lock();

	STRACE(("*RootLayer::AddWinBorder(%s) - Main lock acquired\n", winBorder->GetName()));

	// in case we want to be added to the current workspace
	if (winBorder->Window()->Workspaces() == B_CURRENT_WORKSPACE)
		winBorder->Window()->QuietlySetWorkspaces(0x00000001 << (ActiveWorkspaceIndex()-1));

	// add winBorder to the known list of WinBorders so we can keep track of it.
	AddChild(winBorder, winBorder->Window());

	// add winBorder to the desired workspaces
	switch(winBorder->Window()->Feel())
	{
		case B_MODAL_SUBSET_WINDOW_FEEL:
		case B_FLOATING_SUBSET_WINDOW_FEEL:
		{
			// this kind of window isn't added anywhere. It will be added
			//	to main window's subset when winBorder::AddToSubsetOf(main)
			//	will be called.
			break;
		}
		case B_MODAL_APP_WINDOW_FEEL:
		case B_FLOATING_APP_WINDOW_FEEL:
		{
			// add to app's list of Floating/Modal windows (as opposed to the system's)
			winBorder->Window()->App()->fAppFMWList.AddItem(winBorder);

			// determine in which workspaces to add this winBorder object.
			uint32		wks = 0;
			for (int32 i=0; i<WorkspaceCount(); i++)
			{
				// if we find a window belonging to winBorder's team, add winBorder to that workspace.
				Workspace	*ws = WorkspaceAt(i+1);
				for (WinBorder *wb = ws->GoToBottomItem(); wb!=NULL; wb = ws->GoToUpperItem())
				{
					if ( winBorder->Window()->ClientTeamID() == wb->Window()->ClientTeamID())
					{
						wks = wks | winBorder->Window()->Workspaces();
						break;
					}
				}
			}

			AddWinBorderToWorkspaces(winBorder, wks);
			break;
		}
				
		case B_MODAL_ALL_WINDOW_FEEL:
		case B_FLOATING_ALL_WINDOW_FEEL:
		{
			// add to system's list of Floating/Modal Windows
			fMainFMWList.AddItem(winBorder);
			
			// add this winBorder to all workspaces
			AddWinBorderToWorkspaces(winBorder, 0xffffffffUL);
			break;
		}
		
		case B_NORMAL_WINDOW_FEEL:
		{
			// add this winBorder to the specified workspaces
			AddWinBorderToWorkspaces(winBorder, winBorder->Window()->Workspaces());
			break;
		}

		case B_SYSTEM_LAST:
		case B_SYSTEM_FIRST:
		{
			// add this winBorder to all workspaces
			AddWinBorderToWorkspaces(winBorder, 0xffffffffUL);
			break;
		}
		default:{
			debugger("RootLayer::AddWinBorder() - what kind of window is this?");
			break;
		}
	}	// end switch(winborder->Feel())

	fMainLock.Unlock();
	STRACE(("*RootLayer::AddWinBorder(%s) - Main lock released\n", winBorder->GetName()));
	
	desktop->fGeneralLock.Unlock();
	STRACE(("*RootLayer::AddWinBorder(%s) - General lock released\n", winBorder->GetName()));
	
	STRACE(("#RootLayer::AddWinBorder(%s)\n", winBorder->GetName()));
}

void RootLayer::RemoveWinBorder(WinBorder* winBorder)
{
	// This method does 3 things:
	// 1) Removes MODAL/SUBSET windows from system/app/window subset list.
	// 2) Removes this window from any workspace it appears in.
	// 3) Removes this window from RootLayer's list of children.

	desktop->fGeneralLock.Lock();
	fMainLock.Lock();
	
	int32 feel = winBorder->Window()->Feel();
	if(feel == B_MODAL_SUBSET_WINDOW_FEEL || feel == B_FLOATING_SUBSET_WINDOW_FEEL)
	{
		desktop->RemoveSubsetWindow(winBorder);
	}
	else
	if (feel == B_MODAL_APP_WINDOW_FEEL || feel == B_FLOATING_APP_WINDOW_FEEL)
	{
		RemoveAppWindow(winBorder);
	}
	else
	if(feel == B_MODAL_ALL_WINDOW_FEEL || feel == B_FLOATING_ALL_WINDOW_FEEL
			|| feel == B_SYSTEM_FIRST || feel == B_SYSTEM_LAST)
	{
		if(feel == B_MODAL_ALL_WINDOW_FEEL || feel == B_FLOATING_ALL_WINDOW_FEEL)
			fMainFMWList.RemoveItem(winBorder);

		int32 count = WorkspaceCount();
		for(int32 i=0; i < count; i++)
			WorkspaceAt(i+1)->RemoveWinBorder(winBorder);
	}
	else
	{	// for B_NORMAL_WINDOW_FEEL
		uint32 workspaces = winBorder->Window()->Workspaces();
		int32 count = WorkspaceCount();
		for( int32 i=0; i < 32 && i < count; i++)
		{
			if( workspaces & (0x00000001UL << i))
				WorkspaceAt(i+1)->RemoveWinBorder(winBorder);
		}
	}
	
	RemoveChild(winBorder);
	
	fMainLock.Unlock();
	desktop->fGeneralLock.Unlock();
}

WinBorder* RootLayer::WinBorderAt(const BPoint& pt){
	WinBorder		*target = NULL;
	WinBorder		*wb = NULL;

	for( wb = fActiveWorkspace->GoToBottomItem(); wb; wb = fActiveWorkspace->GoToUpperItem())
	{
		if(!wb->IsHidden() && wb->HasPoint(pt))
		{
			target	= wb;
			break;
		}
	}
	return target;
}

void RootLayer::ChangeWorkspacesFor(WinBorder* winBorder, uint32 newWorkspaces)
{
	// only normal windows are affected by this change
	if(!winBorder->fLevel != B_NORMAL_FEEL)
		return;

	uint32 oldWorkspaces = winBorder->Window()->Workspaces();
	for(int32 i=0; i < WorkspaceCount(); i++)
	{
		if ((oldWorkspaces & (0x00000001 << i)) && (newWorkspaces & (0x00000001 << i)))
		{
			// do nothing.
		}
		else
		if(oldWorkspaces & (0x00000001 << i))
		{
			WorkspaceAt(i+1)->RemoveWinBorder(winBorder);
		}
		else
		if (newWorkspaces & (0x00000001 << i))
		{
			WorkspaceAt(i+1)->AddWinBorder(winBorder);
		}
	}
}

bool RootLayer::SetFrontWinBorder(WinBorder* winBorder)
{
	if(!winBorder)
		return false;
	
	STRACE(("*RootLayer::SetFrontWinBorder(%s)\n", winBorder? winBorder->GetName():"NULL"));
	
	fMainLock.Lock();
	STRACE(("*RootLayer::SetFrontWinBorder(%s) - main lock acquired\n", winBorder? winBorder->GetName():"NULL"));
	
	if (!winBorder)
	{
		ActiveWorkspace()->SearchAndSetNewFront(NULL);
		return true;
	}
	
	uint32 workspaces	= winBorder->Window()->Workspaces();
	int32 count		= WorkspaceCount();
	int32 newWorkspace= 0;
	
	if (workspaces & (0x00000001UL << (ActiveWorkspaceIndex()-1)) )
	{
		newWorkspace = ActiveWorkspaceIndex();
	}
	else
	{
		int32	i;
		for( i = 0; i < 32 && i < count; i++)
		{
			if( workspaces & (0x00000001UL << i))
			{
				newWorkspace	= i+1;
				break;
			}
		}
		
		if (i == count || i == 32)
			newWorkspace = ActiveWorkspaceIndex();
	}

	if(newWorkspace != ActiveWorkspaceIndex())
	{
		WorkspaceAt(newWorkspace)->SearchAndSetNewFront(winBorder);
		SetActiveWorkspaceByIndex(newWorkspace);
	}
	else
	{
		ActiveWorkspace()->SearchAndSetNewFront(winBorder);
	}
	
	fMainLock.Unlock();
	STRACE(("*RootLayer::SetFrontWinBorder(%s) - main lock released\n", winBorder? winBorder->GetName():"NULL"));
	STRACE(("#RootLayer::SetFrontWinBorder(%s)\n", winBorder? winBorder->GetName():"NULL"));
	return true;
}

void RootLayer::SetScreens(Screen *screen[], int32 rows, int32 columns)
{
	// NOTE: All screens *must* have the same resolution
	fScreenPtrList.MakeEmpty();
	BRect	newFrame(0, 0, 0, 0);
	for (int32 i=0; i < rows; i++)
	{
		if (i==0)
		{
			for(int32 j=0; j < columns; j++)
			{
				fScreenPtrList.AddItem(screen[i*rows + j]);
				newFrame.right += screen[i*rows + j]->Resolution().x;
			}
		}
		newFrame.bottom		+= screen[i*rows]->Resolution().y;
	}
	
	newFrame.right	-= 1;
	newFrame.bottom	-= 1;
	
	fFrame = newFrame;
	fRows = rows;
	fColumns = columns;
	fScreenXResolution = (int32)(screen[0]->Resolution().x);
	fScreenYResolution = (int32)(screen[0]->Resolution().y);

	// NOTE: a RebuildFullRegion() followed by FullInvalidate() are required after calling 
	// this method!
}

Screen **RootLayer::Screens()
{
	return (Screen**)fScreenPtrList.Items();
}

bool RootLayer::SetScreenResolution(int32 width, int32 height, uint32 colorspace)
{
	if (fScreenXResolution == width && fScreenYResolution == height &&
		fColorSpace == colorspace)
		return false;
	
	bool accepted = true;
	
	for (int i=0; i < fScreenPtrList.CountItems(); i++)
	{
		Screen *screen;
		screen = static_cast<Screen*>(fScreenPtrList.ItemAt(i));
		
		if ( !(screen->SupportsResolution(BPoint(width, height), colorspace)) )
			accepted = false;
	}
	
	if (accepted)
	{
		for (int i=0; i < fScreenPtrList.CountItems(); i++)
		{
			Screen *screen;
			screen = static_cast<Screen*>(fScreenPtrList.ItemAt(i));
			
			screen->SetResolution(BPoint(width, height), colorspace);
		}
		
		Screen **screens = (Screen**)fScreenPtrList.Items();
		SetScreens(screens, fRows, fColumns);
		
		return true;
	}
	
	return false;
}

void RootLayer::SetWorkspaceCount(const int32 count)
{
	STRACE(("*RootLayer::SetWorkspaceCount(%ld)\n", count));
	
	if ((count < 1 && count > 32) || count == WorkspaceCount())
		return;
	
	int32 	exActiveWorkspaceIndex = ActiveWorkspaceIndex();
	
	BList newWSPtrList;
	void *workspacePtr;
	
	fMainLock.Lock();
	STRACE(("*RootLayer::SetWorkspaceCount(%ld) - main lock acquired\n", count));
	
	for(int i=0; i < count; i++)
	{
		workspacePtr	= fWorkspaceList.ItemAt(i);
		if (workspacePtr)
			newWSPtrList.AddItem(workspacePtr);
		else
		{
			Workspace	*ws;
			ws = new Workspace(fColorSpace, i+1, BGColor());
			newWSPtrList.AddItem(ws);
		}
	}
	
	// delete other Workspace objects if the count is smaller than current one.
	for (int j=count; j < fWorkspaceList.CountItems(); j++)
	{
		Workspace	*ws = NULL;
		ws = static_cast<Workspace*>(fWorkspaceList.ItemAt(j));
		if (ws)
			delete ws;
		else
		{
			STRACE(("\nSERVER: PANIC: in RootLayer::SetWorkspaceCount()\n\n"));
			return;
		}
	}
	
	fWorkspaceList = newWSPtrList;

	fMainLock.Unlock();
	STRACE(("*RootLayer::SetWorkspaceCount(%ld) - main lock released\n", count));
	
	if (exActiveWorkspaceIndex > count)
		SetActiveWorkspaceByIndex(count); 
	
	// if true, this is the first time this method is called.
	if (exActiveWorkspaceIndex == -1)
		SetActiveWorkspaceByIndex(1); 		
	
	STRACE(("#RootLayer::SetWorkspaceCount(%ld)\n", count));
}

int32 RootLayer::WorkspaceCount() const
{
	return fWorkspaceList.CountItems();
}

Workspace* RootLayer::WorkspaceAt(const int32 index) const
{
	Workspace *ws = NULL;
	ws = static_cast<Workspace*>(fWorkspaceList.ItemAt(index-1));
	
	return ws;
}

void RootLayer::SetActiveWorkspaceByIndex(const int32 index)
{
	Workspace *ws = NULL;
	ws = static_cast<Workspace*>(fWorkspaceList.ItemAt(index-1));
	if (ws)
		SetActiveWorkspace(ws);
}

void RootLayer::SetActiveWorkspace(Workspace *ws)
{
	if (fActiveWorkspace == ws || !ws)
		return;
	
	int32 index;
	WinBorder *winborder;
	
	// Notify windows in current workspace of change
	if(fActiveWorkspace)
	{
		index=fWorkspaceList.IndexOf(fActiveWorkspace);
		winborder=fActiveWorkspace->GoToTopItem();
		while(winborder)
		{
			winborder->Window()->WorkspaceActivated(index,false);
			winborder=(WinBorder*)winborder->fLowerSibling;
		}
	}
	
	fActiveWorkspace	= ws;
	
	// Notify windows in new workspace of change
	index=fWorkspaceList.IndexOf(fActiveWorkspace);
	winborder=fActiveWorkspace->GoToTopItem();
	while(winborder)
	{
		winborder->Window()->WorkspaceActivated(index,true);
		winborder=(WinBorder*)winborder->fLowerSibling;
	}
}

int32 RootLayer::ActiveWorkspaceIndex() const{
	if (fActiveWorkspace)
		return fActiveWorkspace->ID();
	else
		return -1;
}

Workspace* RootLayer::ActiveWorkspace() const
{
	return fActiveWorkspace;
}

void RootLayer::SetBGColor(const RGBColor &col)
{
	ActiveWorkspace()->SetBGColor(col);
	
	fLayerData->viewcolor	= col;
}

RGBColor RootLayer::BGColor(void) const
{
	return fLayerData->viewcolor;
}

void RootLayer::RemoveAppWindow(WinBorder *wb)
{
	wb->Window()->App()->fAppFMWList.RemoveItem(wb);

	int32 count = WorkspaceCount();
	for(int32 i=0; i < count; i++)
		WorkspaceAt(i+1)->RemoveWinBorder(wb);
}

//---------------------------------------------------------------------------
//				Input related methods
//---------------------------------------------------------------------------
void RootLayer::MouseEventHandler(int32 code, BPortLink& msg)
{
	// TODO: locking mechanism needs SERIOUS rethought
	switch(code)
	{
		case B_MOUSE_DOWN:
		{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - modifier keys down
			// 5) int32 - buttons down
			// 6) int32 - clicks

			PointerEvent evt;	
			evt.code = B_MOUSE_DOWN;
			msg.Read<int64>(&evt.when);
			msg.Read<float>(&evt.where.x);
			msg.Read<float>(&evt.where.y);
			msg.Read<int32>(&evt.modifiers);
			msg.Read<int32>(&evt.buttons);
			msg.Read<int32>(&evt.clicks);
			
			// printf("MOUSE DOWN: at (%f, %f)\n", evt.where.x, evt.where.y);
			
			WinBorder	*target=NULL;
			Workspace	*ws=NULL;
			ws			= ActiveWorkspace();
			target		= WinBorderAt(evt.where);
			if (target)
			{
				desktop->fGeneralLock.Lock();
				fMainLock.Lock();
				
				STRACE(("Target: %s\n", target->GetName()));
				STRACE(("Front: %s\n", ws->FrontLayer()->GetName()));
				STRACE(("Focus: %s\n", ws->FocusLayer()->GetName()));
				
				WinBorder		*previousFocus=NULL;
				WinBorder		*activeFocus=NULL;
				BRegion			invalidRegion;
				
				BMessage downmsg(B_MOUSE_DOWN);
				downmsg.AddInt64("when",evt.when);
				downmsg.AddPoint("where",evt.where);
				downmsg.AddInt32("modifiers",evt.modifiers);
				downmsg.AddInt32("buttons",evt.buttons);
				downmsg.AddInt32("clicks",evt.clicks);
				
				if (target!=ws->FrontLayer())
				{
					ws->BringToFrontANormalWindow(target);
					ws->SearchAndSetNewFront(target);
					previousFocus	= ws->FocusLayer();
					ws->SearchAndSetNewFocus(target);
					activeFocus		= ws->FocusLayer();

					activeFocus->Window()->Lock();

					if (target == activeFocus && target->Window()->Flags() & B_WILL_ACCEPT_FIRST_CLICK)
						target->MouseDown(evt, true);
					else
						target->MouseDown(evt, false);

					// may or may not be empty.
					
					// TODO: B_MOUSE_DOWN: what if modal of floating windows are in front of us?
					invalidRegion.Include(&(activeFocus->fFull));
					invalidRegion.Include(&(activeFocus->fTopLayer->fFull));
					activeFocus->fParent->RebuildAndForceRedraw(invalidRegion, activeFocus);

					if (previousFocus != activeFocus && previousFocus)
					{
						if (previousFocus->fVisible.CountRects() > 0)
						{
							invalidRegion.MakeEmpty();
							invalidRegion.Include(&(previousFocus->fVisible));
							activeFocus->fParent->Invalidate(invalidRegion);
						}
					}

					fMouseTarget = target;
					
					STRACE(("2Target: %s\n", target->GetName()));
					STRACE(("2Front: %s\n", ws->FrontLayer()->GetName()));
					STRACE(("2Focus: %s\n", ws->FocusLayer()->GetName()));

					fMouseTarget->Window()->SendMessageToClient(&downmsg);
					activeFocus->Window()->Unlock();
				}
				else // target == ws->FrontLayer()
				{
					// only if target has the focus!
					if (target == ws->FocusLayer())
					{
						target->Window()->Lock();
						target->MouseDown(evt, true);
						target->Window()->SendMessageToClient(&downmsg);
						target->Window()->Unlock();
					}
					else
					{
						previousFocus	= ws->FocusLayer();
						ws->SearchAndSetNewFocus(target);
						activeFocus		= ws->FocusLayer();
					
						activeFocus->Window()->Lock();
	
						if (target == activeFocus && target->Window()->Flags() & B_WILL_ACCEPT_FIRST_CLICK)
							target->MouseDown(evt, true);
						else
							target->MouseDown(evt, false);
	
						// may or may not be empty.
						
						// TODO: B_MOUSE_DOWN: what if modal of floating windows are in front of us?
						invalidRegion.Include(&(activeFocus->fFull));
						invalidRegion.Include(&(activeFocus->fTopLayer->fFull));
						activeFocus->fParent->RebuildAndForceRedraw(invalidRegion, activeFocus);
	
						if (previousFocus != activeFocus && previousFocus)
						{
							if (previousFocus->fVisible.CountRects() > 0)
							{
								invalidRegion.MakeEmpty();
								invalidRegion.Include(&(previousFocus->fVisible));
								activeFocus->fParent->Invalidate(invalidRegion);
							}
						}
						fMouseTarget = target;
						fMouseTarget->Window()->SendMessageToClient(&downmsg);
						
						activeFocus->Window()->Unlock();
					}
				}

				fMainLock.Unlock();
				desktop->fGeneralLock.Unlock();
			}
			else // target == NULL
			{
			}
			break;
		}
		case B_MOUSE_UP:
		{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - modifier keys down

			PointerEvent evt;	
			evt.code = B_MOUSE_UP;
			msg.Read<int64>(&evt.when);
			msg.Read<float>(&evt.where.x);
			msg.Read<float>(&evt.where.y);
			msg.Read<int32>(&evt.modifiers);

			if (!fMouseTarget)
			{
				WinBorder *target = WinBorderAt(BPoint(evt.where.x, evt.where.y));
				if(!target)
					break;
				fMouseTarget=target;
			}

			fMouseTarget->Window()->Lock();
			fMouseTarget->MouseUp(evt);
			
			BMessage upmsg(B_MOUSE_UP);
			upmsg.AddInt64("when",evt.when);
			upmsg.AddPoint("where",evt.where);
			upmsg.AddInt32("modifiers",evt.modifiers);
			
			fMouseTarget->Window()->SendMessageToClient(&upmsg);
			fMouseTarget->Window()->Unlock();

			STRACE(("MOUSE UP: at (%f, %f)\n", evt.where.x, evt.where.y));

			break;
		}
		case B_MOUSE_MOVED:
		{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - buttons down
			
			PointerEvent evt;	
			evt.code = B_MOUSE_MOVED;
			msg.Read<int64>(&evt.when);
			msg.Read<float>(&evt.where.x);
			msg.Read<float>(&evt.where.y);
			msg.Read<int32>(&evt.buttons);

			GetDisplayDriver()->MoveCursorTo(evt.where.x, evt.where.y);
			
			if (!fMouseTarget)
			{
				WinBorder *target = WinBorderAt(BPoint(evt.where.x, evt.where.y));
				if(!target)
					break;
				fMouseTarget=target;
			}
			
			fMouseTarget->Window()->Lock();
			fMouseTarget->MouseMoved(evt);
			
			BMessage movemsg(B_MOUSE_MOVED);
			movemsg.AddInt64("when",evt.when);
			movemsg.AddPoint("where",evt.where);
			movemsg.AddInt32("buttons",evt.buttons);
			
			fMouseTarget->Window()->SendMessageToClient(&movemsg);
			fMouseTarget->Window()->Unlock();

			break;
		}
		case B_MOUSE_WHEEL_CHANGED:
		{
			// FEATURE: This is a tentative change: mouse wheel messages are always sent to the window
			// under the cursor. It's pretty stupid to send it to the active window unless a particular
			// view has locked focus via SetMouseEventMask
						
			PointerEvent evt;	
			evt.code = B_MOUSE_WHEEL_CHANGED;
			msg.Read<int64>(&evt.when);
			msg.Read<float>(&evt.wheel_delta_x);
			msg.Read<float>(&evt.wheel_delta_y);
			
			BMessage wheelmsg(B_MOUSE_WHEEL_CHANGED);
			wheelmsg.AddInt64("when",evt.when);
			wheelmsg.AddFloat("be:wheel_delta_x",evt.wheel_delta_x);
			wheelmsg.AddFloat("be:wheel_delta_y",evt.wheel_delta_y);
			
			if(!fMouseTarget)
			{
				fMouseTarget = WinBorderAt(GetDisplayDriver()->GetCursorPosition());
				
				// We do nothing because there ain't a window to receive the message
				if(!fMouseTarget)
					break;
			}
			
			fMouseTarget->Window()->Lock();
			fMouseTarget->Window()->SendMessageToClient(&wheelmsg);
			fMouseTarget->Window()->Unlock();
			
			break;
		}
		default:
		{
			printf("\nDesktop::MouseEventHandler(): WARNING: unknown message\n\n");
			break;
		}
	}
}

void RootLayer::KeyboardEventHandler(int32 code, BPortLink& msg)
{

	switch(code)
	{
		case B_KEY_DOWN:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifier-independent ASCII code for the character
			// 4) int32 repeat count
			// 5) int32 modifiers
			// 6) int8[3] UTF-8 data generated
			// 7) int8 number of bytes to follow containing the 
			//		generated string
			// 8) Character string generated by the keystroke
			// 9) int8[16] state of all keys
	
			bigtime_t time;
			int32 scancode, modifiers;
			int8 utf[3];
			char *string = NULL;
			int32 keystate;
	
			msg.Read<bigtime_t>(&time);
			msg.Read<int32>(&scancode);
			msg.Read<int32>(&modifiers);
			msg.Read(utf, sizeof(utf));
			msg.ReadString(&string);
			msg.Read<int32>(&keystate);
			if (string)
				free(string);
	
			if(DISPLAYDRIVER==HWDRIVER)
			{
				// Check for workspace change or safe video mode
				if(scancode>0x01 && scancode<0x0e)
				{
					if(scancode==0x0d)
					{
						if(modifiers & (B_LEFT_COMMAND_KEY |
							B_LEFT_CONTROL_KEY | B_LEFT_SHIFT_KEY))
						{
							// TODO: Set to Safe Mode in KeyboardEventHandler:B_KEY_DOWN. (DisplayDriver API change)
							STRACE(("Safe Video Mode invoked - code unimplemented\n"));
							break;
						}
					}
				}
			
				if(modifiers & B_CONTROL_KEY)
				{
					STRACE(("Set Workspace %ld\n",scancode-1));
					
					//TODO: SetWorkspace in KeyboardEventHandler
					//SetWorkspace(scancode-2);
					break;
				}	

				// Tab key
				if(scancode==0x26 && (modifiers & B_CONTROL_KEY))
				{
					//ServerApp *deskbar=app_server->FindApp("application/x-vnd.Be-TSKB");
					//if(deskbar)
					//{
						printf("Send Twitcher message key to Deskbar - unimplmemented\n");
						break;
					//}
				}

				// PrintScreen
				if(scancode==0xe)
				{
					if(GetDisplayDriver())
					{
						char filename[128];
						BEntry entry;
						
						sprintf(filename,"/boot/home/screen%ld.png",fScreenShotIndex);
						entry.SetTo(filename);
						
						while(entry.Exists())
						{
							fScreenShotIndex++;
							sprintf(filename,"/boot/home/screen%ld.png",fScreenShotIndex);
						}
						fScreenShotIndex++;
						GetDisplayDriver()->DumpToFile(filename);
						break;
					}
				}
			}
			else
			{
				// F12
				if(scancode>0x1 && scancode<0xe)
				{
					if(scancode==0xd)
					{
						if(modifiers & (B_LEFT_CONTROL_KEY | B_LEFT_SHIFT_KEY | B_LEFT_OPTION_KEY))
						{
							// TODO: Set to Safe Mode in KeyboardEventHandler:B_KEY_DOWN. (DisplayDriver API change)
							STRACE(("Safe Video Mode invoked - code unimplemented\n"));
							break;
						}
					}
					if(modifiers & (B_LEFT_SHIFT_KEY | B_LEFT_CONTROL_KEY))
					{
						STRACE(("Set Workspace %ld\n",scancode-1));
						//TODO: SetWorkspace in KeyboardEventHandler
						//SetWorkspace(scancode-2);
						break;
					}	
				}
				
				//Tab
				if(scancode==0x26 && (modifiers & B_SHIFT_KEY))
				{
					STRACE(("Twitcher\n"));
					//ServerApp *deskbar=app_server->FindApp("application/x-vnd.Be-TSKB");
					//if(deskbar)
					//{
						printf("Send Twitcher message key to Deskbar - unimplmemented\n");
						break;
					//}
				}

				// Pause/Break
				if(scancode==0x7f)
				{
					if(GetDisplayDriver())
					{
						char filename[128];
						BEntry entry;
						
						sprintf(filename,"/boot/home/screen%ld.png",fScreenShotIndex);
						entry.SetTo(filename);
						
						while(entry.Exists())
						{
							fScreenShotIndex++;
							sprintf(filename,"/boot/home/screen%ld.png",fScreenShotIndex);
						}
						fScreenShotIndex++;

						GetDisplayDriver()->DumpToFile(filename);
						break;
					}
				}
			}
			
			// We got this far, so apparently it's safe to pass to the active
			// window.

			// TODO: Pass on key down message to client window with the focus
			break;
		}
		case B_KEY_UP:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifier-independent ASCII code for the character
			// 4) int32 modifiers
			// 5) int8[3] UTF-8 data generated
			// 6) int8 number of bytes to follow containing the 
			//		generated string
			// 7) Character string generated by the keystroke
			// 8) int8[16] state of all keys
			
			bigtime_t time;
			int32 scancode;
			int32 ascii;
			int32 modifiers;
			int8 utf[3];
			int8 bytes;
			char *string;
			int8 keystate[16];
			
			msg.Read<bigtime_t>(&time);
			msg.Read<int32>(&scancode);
			msg.Read<int32>(&ascii);
			msg.Read<int32>(&modifiers);
			msg.Read(utf, sizeof(utf));
			msg.Read<int8>(&bytes);
			msg.ReadString(&string);
			msg.Read(keystate, sizeof(keystate));
			if (string)
				free(string);
	
			STRACE(("Key Up: 0x%lx\n",scancode));
			
			if(DISPLAYDRIVER==HWDRIVER)
			{
				// Tab key
				if(scancode==0x26 && (modifiers & B_CONTROL_KEY))
				{
					//ServerApp *deskbar=app_server->FindApp("application/x-vnd.Be-TSKB");
					//if(deskbar)
					//{
						printf("Send Twitcher message key to Deskbar - unimplmemented\n");
						break;
					//}
				}
			}
			else
			{
				if(scancode==0x26 && (modifiers & B_LEFT_SHIFT_KEY))
				{
					//ServerApp *deskbar=app_server->FindApp("application/x-vnd.Be-TSKB");
					//if(deskbar)
					//{
						printf("Send Twitcher message key to Deskbar - unimplmemented\n");
						break;
					//}
				}
			}

			// We got this far, so apparently it's safe to pass to the active
			// window.
			
			// TODO: Pass on key up message to client window with the focus
			break;
		}
		case B_UNMAPPED_KEY_DOWN:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifiers
			// 4) int32 number of elements in the key state array to follow
			// 5) int8 state of all keys

			bigtime_t time;
			int32 scancode;
			int32 modifiers;
			int32 elements;
			//int8 keystate[16];
			
			msg.Read<bigtime_t>(&time);
			msg.Read<int32>(&scancode);
			msg.Read<int32>(&modifiers);
			msg.Read<int32>(&elements);
			//msg.Read(keystate, elements);

			#ifdef DEBUG_KEYHANDLING
			printf("Unmapped Key Down: 0x%lx\n", scancode);
			#endif
			
			// TODO: Pass on unmapped key down message to client window with the focus
			break;
		}
		case B_UNMAPPED_KEY_UP:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifiers
			// 4) int32 number of elements in the key state array to follow
			// 5) int8 state of all keys

			bigtime_t time;
			int32 scancode;
			int32 modifiers;
			int32 elements;
			//int8 keystate[16];
			
			msg.Read<bigtime_t>(&time);
			msg.Read<int32>(&scancode);
			msg.Read<int32>(&modifiers);
			msg.Read<int32>(&elements);
			//msg.Read(keystate, elements);

			#ifdef DEBUG_KEYHANDLING
			printf("Unmapped Key Up: 0x%lx\n", scancode);
			#endif

			// TODO: Pass on unmapped key up message to client window with the focus
			break;
		}
		case B_MODIFIERS_CHANGED:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 modifiers
			// 3) int32 old modifiers
			// 4) int32 number of elements in the key state array to follow
			// 5) int8 state of all keys
			
			bigtime_t time;
			int32 scancode;
			int32 modifiers;
			int32 elements;
			//int8 keystate[16];
			
			msg.Read<bigtime_t>(&time);
			msg.Read<int32>(&scancode);
			msg.Read<int32>(&modifiers);
			msg.Read<int32>(&elements);
			//msg.Read(keystate, elements);

			#ifdef DEBUG_KEYHANDLING
			printf("Modifiers Changed\n");
			#endif

			// TODO: Pass on modifier change message to client window with the focus
			break;
		}
		default:
			break;
	}
}

void RootLayer::SetDragMessage(BMessage* msg)
{
	if (fDragMessage)
	{
		delete fDragMessage;
		fDragMessage = NULL;
	}

	if (msg)
		fDragMessage	= new BMessage(*msg);
}

BMessage* RootLayer::DragMessage(void) const
{
	return fDragMessage;
}

// DEBUG methods

void RootLayer::PrintToStream()
{
	printf("\nRootLayer '%s' internals:\n", GetName());
	printf("Screen list:\n");
	for(int32 i=0; i<fScreenPtrList.CountItems(); i++)
		printf("\t%ld\n", ((Screen*)fScreenPtrList.ItemAt(i))->ScreenNumber());

	printf("Screen rows: %ld\nScreen columns: %ld\n", fRows, fColumns);
	printf("Resolution for all Screens: %ldx%ldx%ld\n", fScreenXResolution, fScreenYResolution, fColorSpace);
	printf("Workspace list:\n");
	for(int32 i=0; i<fWorkspaceList.CountItems(); i++)
	{
		printf("\t~~~Workspace: %ld\n", ((Workspace*)fWorkspaceList.ItemAt(i))->ID());
		((Workspace*)fWorkspaceList.ItemAt(i))->PrintToStream();
		printf("~~~~~~~~\n");
	}
	printf("Active Workspace: %ld\n", fActiveWorkspace? fActiveWorkspace->ID(): -1);
}

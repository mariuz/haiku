// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  Program:	 desklink
//  Author:      Jérôme DUVAL
//  Description: VolumeControl and link items in Deskbar
//  Created :    October 20, 2003
//	Modified by: Jérome Duval
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <MediaRoster.h>
#include <MediaTheme.h>
#include <MultiChannelControl.h>
#include <Screen.h>
#include <Beep.h>
#include <Box.h>
#include <stdio.h>
#include <Debug.h>

#include "VolumeSlider.h"
#include "iconfile.h"

#define VOLUME_CHANGED 'vlcg'

VolumeSlider::VolumeSlider(BRect frame)
	: BWindow(frame, "VolumeSlider", B_BORDERED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_WILL_ACCEPT_FIRST_CLICK, 0),
	aOutNode(NULL),
	paramWeb(NULL),
	mixerParam(NULL)
{
	//Make sure it's not outside the screen.
	const int32 kMargin = 3;
	BRect windowRect=ConvertToScreen(Bounds());
	BRect screenFrame(BScreen(B_MAIN_SCREEN_ID).Frame());
	if (screenFrame.right < windowRect.right + kMargin)
		MoveBy(- kMargin - windowRect.right + screenFrame.right, 0);
	if (screenFrame.bottom < windowRect.bottom + kMargin)
		MoveBy(0, - kMargin - windowRect.bottom + screenFrame.bottom);
	if (screenFrame.left > windowRect.left - kMargin)
		MoveBy(kMargin + screenFrame.left - windowRect.left, 0);
	if (screenFrame.top > windowRect.top - kMargin)
		MoveBy(0, kMargin + screenFrame.top - windowRect.top);

	float value = 0.0;
	
	aOutNode = new media_node();

	status_t err;
	BMediaRoster* roster = BMediaRoster::Roster(&err);
	
	if(roster && (err==B_OK)) {
		if((err = roster->GetAudioOutput(aOutNode)) == B_OK) {
			if((err = roster->GetParameterWebFor(*aOutNode, &paramWeb)) == B_OK) {
			
				//Finding the Mixer slider in the audio output ParameterWeb 
				int32 numParams = paramWeb->CountParameters();
				BParameter* p = NULL;
				for (int i = 0; i < numParams; i++) {
					p = paramWeb->ParameterAt(i);
					printf("%i %s\n", i, p->Name());
					if (!strcmp(p->Name(), "Master")) {
						for (; i < numParams; i++) {
							p = paramWeb->ParameterAt(i);
							if (strcmp(p->Kind(), B_MASTER_GAIN)) p=NULL;
							else break;
						}
						break;
					} else p = NULL;
				}
				if (p==NULL) {
					printf("Could not find the mixer.\n");
					exit(1);
				} else if(p->Type()!=BParameter::B_CONTINUOUS_PARAMETER) {
					printf("Mixer is unknown.\n");
					exit(2);
				}
			
				mixerParam= dynamic_cast<BContinuousParameter*>(p);
				min = mixerParam->MinValue();
				max = mixerParam->MaxValue();
				step = mixerParam->ValueStep();
				
				float chanData[2];
				bigtime_t lastChange;
				size_t size = sizeof(chanData);
						
				mixerParam->GetValue( &chanData, &size, &lastChange );
				
				value = (chanData[0]-min)*100/(max-min);
			} else {
				printf("No parameter web\n");
			}
		} else {
			printf("No Audio output\n");
		}
	} else {
		printf("No Media Roster\n");			
	}
	
	if(err!=B_OK) {
		delete aOutNode;
		aOutNode = NULL;
	}
	
	BBox *box = new BBox(Bounds(), "sliderbox", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	AddChild(box);
	
	slider = new SliderView(box->Bounds().InsetByCopy(1, 1), new BMessage(VOLUME_CHANGED), 
		mixerParam ? "Volume" : "No Media Server", B_FOLLOW_LEFT | B_FOLLOW_TOP, value);
	box->AddChild(slider);
	
	slider->SetTarget(this);
	
	SetPulseRate(100);	
}


VolumeSlider::~VolumeSlider()
{
	delete paramWeb;
	BMediaRoster* roster = BMediaRoster::CurrentRoster();
	if(roster && aOutNode)
		roster->ReleaseNode(*aOutNode);
}


void
VolumeSlider::WindowActivated(bool active)
{
	if (!active) {
		Lock();
		Quit();
	}
}


void 
VolumeSlider::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case VOLUME_CHANGED:
		{
			printf("VOLUME_CHANGED\n");
			if(mixerParam) {
				float chanData[2];
				bigtime_t lastChange;
				size_t size = sizeof(chanData);
				
				mixerParam->GetValue( &chanData, &size, &lastChange );
		
				for( int i=0; i<2; i++) {
					chanData[i] = (slider->Value() * (max - min) / 100) / step * step + min;
				}
				
				PRINT(("Min value: %f      Max Value: %f\nData: %f     %f\n", mixerParam->MinValue(), mixerParam->MaxValue(), chanData[0], chanData[1]));
				mixerParam->SetValue(&chanData, sizeof(chanData), 0);
				
				beep();
			}
			
			Quit();
			break;
		}
		default:
			BWindow::MessageReceived(msg);		// not a slider message, not our problem
	} 
}

#define REDZONESTART 151


SliderView::SliderView(BRect rect, BMessage *msg, const char *title, uint32 resizeFlags, int32 value)
	: BControl(rect, "slider", NULL, msg, resizeFlags, B_WILL_DRAW | B_PULSE_NEEDED),
	leftBitmap(BRect(0, 0, kLeftWidth - 1, kLeftHeight - 1), B_CMAP8),
	rightBitmap(BRect(0, 0, kRightWidth - 1, kRightHeight - 1), B_CMAP8),
	buttonBitmap(BRect(0, 0, kButtonWidth - 1, kButtonHeight - 1), B_CMAP8),
	fTrackingX(11 + (192-11) * value / 100),
	fTitle(title)
{
	leftBitmap.SetBits(kLeftBits, kLeftWidth * kLeftHeight, 0, B_CMAP8);
	rightBitmap.SetBits(kRightBits, kRightWidth * kRightHeight, 0, B_CMAP8);
	buttonBitmap.SetBits(kButtonBits, kButtonWidth * kButtonHeight, 0, B_CMAP8);
	
	SetTracking(true);
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	SetValue(value);
}


SliderView::~SliderView()
{
	
}


void
SliderView::Pulse()
{
	uint32 mouseButtons;
	BPoint where;
	GetMouse(&where, &mouseButtons, true);
	
	// button not pressed, exit
	if (! (mouseButtons & B_PRIMARY_MOUSE_BUTTON)) {
		SetTracking(false);
		SetValue( (fTrackingX - 11) / (192-11) * 100 );
		Invoke();
	}
}


void
SliderView::Draw(BRect updateRect)
{
	SetHighColor(189,186,189);
	StrokeLine(BPoint(11,1), BPoint(192,1));
	SetHighColor(0,0,0);
	StrokeLine(BPoint(11,2), BPoint(192,2));
	SetHighColor(255,255,255);
	StrokeLine(BPoint(11,14), BPoint(192,14));
	SetHighColor(231,227,231);
	StrokeLine(BPoint(11,15), BPoint(192,15));
	
	SetLowColor(ViewColor());

	SetDrawingMode(B_OP_OVER);

	DrawBitmapAsync(&leftBitmap, BPoint(5,1));
	DrawBitmapAsync(&rightBitmap, BPoint(193,1));
	
	float right = (fTrackingX < REDZONESTART) ? fTrackingX : REDZONESTART;
	SetHighColor(99,151,99);
	FillRect(BRect(11,3,right,4));
	SetHighColor(156,203,156);
	FillRect(BRect(11,5,right,13));
	if(right == REDZONESTART) {
		SetHighColor(156,101,99);
		FillRect(BRect(REDZONESTART,3,fTrackingX,4));
		SetHighColor(255,154,156);
		FillRect(BRect(REDZONESTART,5,fTrackingX,13));
	}		
	SetHighColor(156,154,156);
	FillRect(BRect(fTrackingX,3,192,13));
	
	BFont font;
	float width = font.StringWidth(fTitle);
	
	SetHighColor(49,154,49);
	DrawString(fTitle, BPoint(11 + (192-11-width)/2, 12));
		
	DrawBitmapAsync(&buttonBitmap, BPoint(fTrackingX-5,3));
	
	Sync();
	
	SetDrawingMode(B_OP_COPY);
}


void 
SliderView::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (!IsTracking())
		return;
		
	uint32 mouseButtons;
	BPoint where;
	GetMouse(&where, &mouseButtons, true);
	
	// button not pressed, exit
	if (! (mouseButtons & B_PRIMARY_MOUSE_BUTTON)) {
		Invoke();
		SetTracking(false);
	}
		
	if (!Bounds().InsetBySelf(2,2).Contains(point))
		return;
		
	fTrackingX = point.x;
	if(fTrackingX < 11)
		fTrackingX = 11;
	if(fTrackingX > 192)
		fTrackingX = 192;	
	Draw(Bounds());
	Flush();
	
}


void
SliderView::MouseUp(BPoint point)
{
	if (!IsTracking())
		return;
	
	SetValue( (point.x - 11) / (192-11) * 100 );
	if(Value()<0)
		SetValue(0);
	if(Value()>100)
		SetValue(100);
	
	Invoke();
	SetTracking(false);
	fTrackingX = point.x;
	if(fTrackingX < 11)
		fTrackingX = 11;
	if(fTrackingX > 192)
		fTrackingX = 192;	
	Draw(Bounds());
	Flush();
}

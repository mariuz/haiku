// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        MediaViews.cpp
//  Author:      Sikosis, Jérôme Duval
//  Description: Media Preferences
//  Created :    June 25, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~


// Includes -------------------------------------------------------------------------------------------------- //
#include <Box.h>
#include <Button.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <MediaRoster.h>
#include <Deskbar.h>
#include <Entry.h>
#include <stdio.h>
#include <MediaAddOn.h>
#include <String.h>
#include <TextView.h>
#include <GroupView.h>
#include <SpaceLayoutItem.h>

#include "MediaViews.h"

BarView::BarView(BRect frame) 
 : BView (frame, "barView", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW ),
 	fDisplay(true)
{
}

void 
BarView::Draw(BRect updateRect)
{
	BRect r = Bounds();
	
	if (fDisplay) {
		// Display the 3D Look Divider Bar
		SetHighColor(140,140,140,0);
		StrokeLine(BPoint(r.left,r.top),BPoint(r.right,r.top));
		SetHighColor(255,255,255,0);
		StrokeLine(BPoint(r.left,r.bottom),BPoint(r.right,r.bottom));
	} else {
		SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		StrokeLine(BPoint(r.left,r.top),BPoint(r.right,r.top));
		StrokeLine(BPoint(r.left,r.bottom),BPoint(r.right,r.bottom));
	}
}


SettingsView::SettingsView (BRect frame, bool isVideo)
 : BView (frame, "SettingsView", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW ),
	fIsVideo(isVideo)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BRect rect(frame);
	rect.left += 10;
	rect.top += 12;
	rect.right -=21;
	rect.bottom = rect.top + 104;
	BBox *defaultsBox = new BBox(rect, "defaults");
	defaultsBox->SetLabel(fIsVideo ? "Default Nodes" : "Defaults");
	
	// create the default box
	BGroupLayout* defaultBoxLayout = new BGroupLayout(B_VERTICAL, 5);
	float inset = defaultsBox->Frame().left - defaultsBox->InnerFrame().left;
	defaultBoxLayout->SetInsets(inset, defaultsBox->TopBorderOffset() * 2, inset, inset);
	defaultsBox->SetLayout(defaultBoxLayout);
	defaultBoxLayout->AddItem(BSpaceLayoutItem::CreateVerticalStrut(5));

	BGroupView* inputField = new BGroupView(B_HORIZONTAL);
	BGroupView* outputField = new BGroupView(B_HORIZONTAL);
	defaultsBox->GetLayout()->AddView(inputField);
	defaultsBox->GetLayout()->AddView(outputField);

	BRect defaultRect(20, 22, 250, 40);
	float divider = StringWidth(fIsVideo ? "Video Output:" : "Audio Output:") + 5;
	fMenu1 = new BPopUpMenu("<none>");
	fMenu1->SetLabelFromMarked(true);
	BMenuField *menuField1 = new BMenuField(defaultRect, "menuField1",
		fIsVideo ? "Video Input:" : "Audio Input:", fMenu1);
	menuField1->SetDivider(divider);
	
	defaultRect.OffsetBy(0, 26);
	fMenu2 = new BPopUpMenu("<none>");
	fMenu2->SetLabelFromMarked(true);
	BMenuField *menuField2 = new BMenuField(defaultRect, "menuField2",
		fIsVideo ? "Video Output:" : "Audio Output:", fMenu2);
	menuField2->SetDivider(divider);
	
	inputField->GroupLayout()->AddView(menuField1);
	outputField->GroupLayout()->AddView(menuField2);

	BMenuField *menuField3 = NULL;
	if (!fIsVideo) {
		defaultRect.OffsetBy(186, 0);
		defaultRect.right -= 30;
		fMenu3 = new BPopUpMenu("<none>");
		fMenu3->SetLabelFromMarked(true);
		menuField3 = new BMenuField(defaultRect, "menuField3",
			"Channel:", fMenu3);
		outputField->GroupLayout()->AddView(menuField3);
		menuField3->SetDivider(StringWidth("Channel:")+5);
		defaultRect.right += 30;
		defaultRect.OffsetBy(-186, 0);
	}
	
	defaultRect.OffsetBy(0, 32);
	defaultRect.right += 100;
	rgb_color red_color = {222, 32, 33};
	fRestartView = new BStringView(defaultRect, "restartStringView", "Restart the Media Server to apply changes.",
		B_FOLLOW_ALL, B_WILL_DRAW);
	fRestartView->SetHighColor(red_color);
	defaultsBox->AddChild(fRestartView);
	fRestartView->Hide();
	
	// create the realtime box
	rect.top = rect.bottom + 10;
	rect.bottom = rect.top + 162;
	BBox *realtimeBox = new BBox(rect, "realtime");
	realtimeBox->SetLabel("Real-Time");
	
	BMessage *message = new BMessage(ML_ENABLE_REAL_TIME);
	message->AddBool("isVideo", fIsVideo);
	BRect rect2(22,20, frame.Width() - 22, 40);
	fRealtimeCheckBox = new BCheckBox(rect2, "realtimeCheckBox",
		fIsVideo ? "Enable Real-Time Video" : "Enable Real-Time Audio", message);
	
	uint32 flags;
	BMediaRoster::Roster()->GetRealtimeFlags(&flags);
	if (flags & (fIsVideo ? B_MEDIA_REALTIME_VIDEO : B_MEDIA_REALTIME_AUDIO))
		fRealtimeCheckBox->SetValue(B_CONTROL_ON);
		
	rect2.top += 26;
	rect2.bottom = rect.Height() - 5;
	BRect textRect(3, 3, rect2.Width() - 3, rect2.Height() - 3);
	BTextView *textView = new BTextView(rect2, "stringView", textRect, B_FOLLOW_ALL, B_WILL_DRAW);
	textView->Insert(fIsVideo ? "Enabling Real-Time Video allows the BeOS to perform video operations as fast and smoothly as possible.  It achieves optimum performance by using more RAM."
		"\n\nOnly enable this feature if you need the lowest latency possible."
		: "Enabling Real-time Audio allows BeOS to record and play audio as fast as possible.  It achieves this performance by using more CPU and RAM."
		"\n\nOnly enable this feature if you need the lowest latency possible.");
	textView->MakeEditable(false);
	textView->MakeSelectable(false);
	textView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BGroupLayout* realtimeBoxLayout = new BGroupLayout(B_VERTICAL, 5);
	inset = realtimeBox->Frame().left - realtimeBox->InnerFrame().left;
	realtimeBoxLayout->SetInsets(inset, defaultsBox->TopBorderOffset() * 2, inset, inset);
	realtimeBox->SetLayout(realtimeBoxLayout);
	
	realtimeBoxLayout->AddItem(BSpaceLayoutItem::CreateVerticalStrut(5));
	realtimeBoxLayout->AddView(fRealtimeCheckBox);
	realtimeBoxLayout->AddView(textView);

	// create the bottom line: volumen in deskbar checkbox and restart button
	BGroupView* bottomView = new BGroupView(B_HORIZONTAL);
	rect.top = rect.bottom + 11;
	rect.bottom = rect.top + 20;
	rect.left = rect.right - StringWidth("Restart Media Services") - 20;
	BButton *restartButton = new BButton(rect, "restartButton", 
		"Restart Media Services", new BMessage(ML_RESTART_MEDIA_SERVER));
	
	if (!fIsVideo) {
		rect.right = rect.left - 10;
		rect.top += 4;
		rect.left = frame.left + 33;
		if (StringWidth("Show Volume Control on Deskbar") > rect.Width() - 30)
			rect.left -= 10;
		fVolumeCheckBox = new BCheckBox(rect, "volumeCheckBox", 
			"Show Volume Control on Deskbar", new BMessage(ML_SHOW_VOLUME_CONTROL));
		bottomView->GroupLayout()->AddView(fVolumeCheckBox);
		if (BDeskbar().HasItem("MediaReplicant"))
			fVolumeCheckBox->SetValue(B_CONTROL_ON);	
	}
	else{
		bottomView->GroupLayout()->AddItem(BSpaceLayoutItem::CreateGlue());
	}
	bottomView->GroupLayout()->AddView(restartButton);
	
	// compose all stuff
	BGroupLayout* rootlayout = new BGroupLayout(B_VERTICAL, 5);
	SetLayout(rootlayout);

	rootlayout->AddView(defaultsBox);
	rootlayout->AddView(realtimeBox);
	rootlayout->AddView(bottomView);
}

void
SettingsView::AddNodes(BList &list, bool isInput)
{
	BMenu *menu = isInput ? fMenu1 : fMenu2;
	void *item;
	while ((item = menu->RemoveItem((int32)0)) != NULL)
		delete static_cast<dormant_node_info *>(item);

	BMessage message(ML_DEFAULT_CHANGE);
	message.AddBool("isVideo", fIsVideo);
	message.AddBool("isInput", isInput);

	for (int32 i = 0; i < list.CountItems(); i++) {
		dormant_node_info *info = static_cast<dormant_node_info *>(list.ItemAt(i));
		menu->AddItem(new SettingsItem(info, new BMessage(message)));
	}
}

void
SettingsView::SetDefault(dormant_node_info &info, bool isInput, int32 outputID)
{
	BMenu *menu = isInput ? fMenu1 : fMenu2;
		
	for (int32 i = 0; i < menu->CountItems(); i++) {
		SettingsItem *item = static_cast<SettingsItem *>(menu->ItemAt(i));
		if (item->fInfo && item->fInfo->addon == info.addon && item->fInfo->flavor_id == info.flavor_id) {
			item->SetMarked(true);
			break;
		}
	}
	
	if (!fIsVideo&&!isInput&&outputID>-1) {
		BMenuItem *item;
		while ((item = fMenu3->RemoveItem((int32)0)) != NULL)
			delete item;
		BMediaRoster *roster = BMediaRoster::Roster();
		media_node node;
		media_node_id node_id;
		status_t err;
		if (roster->GetInstancesFor(info.addon, info.flavor_id, &node_id)!=B_OK) 
			err = roster->InstantiateDormantNode(info, &node, B_FLAVOR_IS_GLOBAL);
		else
			err = roster->GetNodeFor(node_id, &node);
		
		if (err == B_OK) {	
			media_input inputs[16];
			int32 inputCount = 16;
			if (roster->GetAllInputsFor(node, inputs, 16, &inputCount)==B_OK) {
				BMessage message(ML_DEFAULTOUTPUT_CHANGE);

				for (int32 i = 0; i < inputCount; i++) {
					media_input *input = new media_input();
					memcpy(input, &inputs[i], sizeof(*input));
					fMenu3->AddItem(item = new Settings2Item(&info, input, new BMessage(message)));
					if (inputs[i].destination.id == outputID)
						item->SetMarked(true);
				}
			}
		}
	}
}

SettingsItem::SettingsItem(dormant_node_info *info, BMessage *message, 
			char shortcut, uint32 modifiers)
	: BMenuItem(info->name, message, shortcut, modifiers),
	fInfo(info)
{
	
}


status_t
SettingsItem::Invoke(BMessage *message)
{
	if (IsMarked())
		return B_OK;
	return BMenuItem::Invoke(message);
}


Settings2Item::Settings2Item(dormant_node_info *info, media_input *input, BMessage *message, 
			char shortcut, uint32 modifiers)
	: BMenuItem(input->name, message, shortcut, modifiers),
	fInfo(info),
	fInput(input)
{
	
}


Settings2Item::~Settings2Item()
{
	delete fInput;
}


status_t
Settings2Item::Invoke(BMessage *message)
{
	if (IsMarked())
		return B_OK;
	return BMenuItem::Invoke(message);
}


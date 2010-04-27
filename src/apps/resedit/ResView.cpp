/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#include "ResView.h"

#include <Application.h>
#include <File.h>
#include <ScrollView.h>
#include <Menu.h>
#include <MenuItem.h>
#include <TranslatorRoster.h>
#include <TypeConstants.h>

#include <stdio.h>
#include <stdlib.h>

#include "App.h"
#include "ColumnTypes.h"
#include "ResourceData.h"
#include "ResFields.h"
#include "ResListView.h"
#include "ResWindow.h"
#include "PreviewColumn.h"
#include "Editor.h"

static int32 sUntitled = 1;

ResourceRoster gResRoster;

enum {
	M_NEW_FILE = 'nwfl',
	M_OPEN_FILE,
	M_SAVE_FILE,
	M_SAVE_FILE_AS,
	M_QUIT,
	M_SELECT_FILE,
	M_DELETE_RESOURCE,
	M_EDIT_RESOURCE
};

ResView::ResView(const BRect &frame, const char *name, const int32 &resize,
				const int32 &flags, const entry_ref *ref)
  :	BView(frame, name, resize, flags),
  	fIsDirty(false)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	if (ref) {
		fRef = new entry_ref;
		*fRef = *ref;
		fFileName = fRef->name;
	} else {
		fRef = NULL;
		fFileName = "Untitled ";
		fFileName << sUntitled;
		sUntitled++;
	}
	
	BRect r(Bounds());
	r.bottom = 16;
	fBar = new BMenuBar(r, "bar");
	AddChild(fBar);
	
	BuildMenus(fBar);
	
	r = Bounds();
	r.top = fBar->Frame().bottom + 4;
	fListView = new ResListView(r, "gridview", B_FOLLOW_ALL, B_WILL_DRAW, B_FANCY_BORDER);
	AddChild(fListView);
	
	rgb_color white = { 255, 255, 255, 255 };
	fListView->SetColor(B_COLOR_BACKGROUND, white);
	fListView->SetColor(B_COLOR_SELECTION, ui_color(B_MENU_BACKGROUND_COLOR));
	
	float width = be_plain_font->StringWidth("00000") + 20;
	fListView->AddColumn(new BStringColumn("ID", width, width, 100, B_TRUNCATE_END), 0);
	
	fListView->AddColumn(new BStringColumn("Type", width, width, 100, B_TRUNCATE_END), 1);
	fListView->AddColumn(new BStringColumn("Name", 150, 50, 300, B_TRUNCATE_END), 2);
	fListView->AddColumn(new PreviewColumn("Data", 150, 50, 300), 3);
	
	// Editing is disabled for now
	fListView->SetInvocationMessage(new BMessage(M_EDIT_RESOURCE));
	
	width = be_plain_font->StringWidth("1000 bytes") + 20;
	fListView->AddColumn(new BSizeColumn("Size", width, 10, 100), 4);
	
	fFilePanel = new BFilePanel(B_OPEN_PANEL);
	if (ref)
		OpenFile(*ref);
}


ResView::~ResView(void)
{
	EmptyDataList();
	delete fRef;
	delete fFilePanel;
}


void
ResView::AttachedToWindow(void)
{
	for(int32 i = 0; i < fBar->CountItems(); i++)
		fBar->SubmenuAt(i)->SetTargetForItems(this);
	fListView->SetTarget(this);
	fFilePanel->SetTarget(BMessenger(this));
}


void
ResView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_NEW_FILE: {
			BRect r(100, 100, 400, 400);
			if (Window())
				r = Window()->Frame().OffsetByCopy(10, 10);
			ResWindow *win = new ResWindow(r);
			win->Show();
			break;
		}
		case M_OPEN_FILE: {
			be_app->PostMessage(M_SHOW_OPEN_PANEL);
			break;
		}
		case M_QUIT: {
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case B_REFS_RECEIVED: {
			int32 i = 0;
			entry_ref ref;
			while (msg->FindRef("refs", i++, &ref) == B_OK)
				AddResource(ref);
			break;
		}
		case M_SELECT_FILE: {
			fFilePanel->Show();
			break;
		}
		case M_DELETE_RESOURCE: {
			DeleteSelectedResources();
			break;
		}
		case M_EDIT_RESOURCE: {
			BRow *row = fListView->CurrentSelection();
			TypeCodeField *field = (TypeCodeField*)row->GetField(1);
			gResRoster.SpawnEditor(field->GetResourceData(), this);
			break;
		}
		case M_UPDATE_RESOURCE: {
			ResourceData *item;
			if (msg->FindPointer("item", (void **)&item) != B_OK)
				break;
			
			for (int32 i = 0; i < fListView->CountRows(); i++) {
				BRow *row = fListView->RowAt(i);
				TypeCodeField *field = (TypeCodeField*)row->GetField(1);
				if (!field || field->GetResourceData() != item)
					continue;
				
				UpdateRow(row);
				break;
			}
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}


void
ResView::OpenFile(const entry_ref &ref)
{
	// Add all the 133t resources and attributes of the file
	BFile file(&ref, B_READ_ONLY);
	BResources resources;
	if (resources.SetTo(&file) != B_OK)
		return;
	file.Unset();
	
	resources.PreloadResourceType();
	
	int32 index = 0;
	ResDataRow *row;
	ResourceData *resData = new ResourceData();
	while (resData->SetFromResource(index, resources)) {
		row = new ResDataRow(resData);
		fListView->AddRow(row);
		fDataList.AddItem(resData);
		resData = new ResourceData();
		index++;
	}
	delete resData;

	BNode node;
	if (node.SetTo(&ref) == B_OK) {
		char attrName[B_ATTR_NAME_LENGTH];
		node.RewindAttrs();
		resData = new ResourceData();
		while (node.GetNextAttrName(attrName) == B_OK) {
			if (resData->SetFromAttribute(attrName, node)) {
				row = new ResDataRow(resData);
				fListView->AddRow(row);
				fDataList.AddItem(resData);
				resData = new ResourceData();
			}
		}
		delete resData;
	}
}


void
ResView::BuildMenus(BMenuBar *menuBar)
{
	BMenu *menu = new BMenu("File");
	menu->AddItem(new BMenuItem("New" B_UTF8_ELLIPSIS, new BMessage(M_NEW_FILE), 'N'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Open" B_UTF8_ELLIPSIS, new BMessage(M_OPEN_FILE), 'O'));
	menuBar->AddItem(menu);
	
	menu = new BMenu("Resource");
	
	menu->AddItem(new BMenuItem("Add" B_UTF8_ELLIPSIS, new BMessage(M_SELECT_FILE), 'F'));
	menu->AddItem(new BMenuItem("Delete", new BMessage(M_DELETE_RESOURCE), 'D'));
	
	
	menuBar->AddItem(menu);
}


void
ResView::EmptyDataList(void)
{
	for (int32 i = 0; i < fDataList.CountItems(); i++) {
		ResourceData *data = (ResourceData*) fDataList.ItemAt(i);
		delete data;
	}
	fDataList.MakeEmpty();
}


void
ResView::UpdateRow(BRow *row)
{
	TypeCodeField *typeField = (TypeCodeField*) row->GetField(1);
	ResourceData *resData = typeField->GetResourceData();
	BStringField *strField = (BStringField *)row->GetField(0);
	
	if (strcmp("(attr)", strField->String()) != 0)
		strField->SetString(resData->GetIDString());
	
	strField = (BStringField *)row->GetField(2);
	strField->SetString(resData->GetName());
	
	PreviewField *preField = (PreviewField*)row->GetField(3);
	preField->SetData(resData->GetData(), resData->GetLength());
	
	BSizeField *sizeField = (BSizeField*)row->GetField(4);
	sizeField->SetSize(resData->GetLength());
}


void
ResView::AddResource(const entry_ref &ref)
{
	BFile file(&ref, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return;
	
	BString mime;
	file.ReadAttrString("BEOS:TYPE", &mime);
	
	if (mime == "application/x-be-resource") {
		BMessage msg(B_REFS_RECEIVED);
		msg.AddRef("refs", &ref);
		be_app->PostMessage(&msg);
		return;
	}
	
	type_code fileType = 0;
	
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	translator_info info;
	if (roster->Identify(&file, NULL, &info, 0, mime.String()) == B_OK)
		fileType = info.type;
	else
		fileType = B_RAW_TYPE;
	
	int32 lastID = -1;
	for (int32 i = 0; i < fDataList.CountItems(); i++) {
		ResourceData *resData = (ResourceData*)fDataList.ItemAt(i);
		if (resData->GetType() == fileType && resData->GetID() > lastID)
			lastID = resData->GetID();
	}
	
	off_t fileSize;
	file.GetSize(&fileSize);
	
	if (fileSize < 1)
		return;
	
	char *fileData = (char *)malloc(fileSize);
	file.Read(fileData, fileSize);
	
	ResourceData *resData = new ResourceData(fileType, lastID + 1, ref.name,
											fileData, fileSize);
	fDataList.AddItem(resData);
	
	ResDataRow *row = new ResDataRow(resData);
	fListView->AddRow(row);
	
	fIsDirty = true;
}


void
ResView::DeleteSelectedResources(void)
{
	ResDataRow *selection = (ResDataRow*)fListView->CurrentSelection();
	
	if (selection)
		fIsDirty = true;
	
	while (selection) {
		ResourceData *data = selection->GetData();
		fListView->RemoveRow(selection);
		fDataList.RemoveItem(data);
		delete data;
		selection = (ResDataRow*)fListView->CurrentSelection();
	}
}


ResDataRow::ResDataRow(ResourceData *data)
  :	fResData(data)
{
	if (data) {
		SetField(new BStringField(fResData->GetIDString()), 0);
		SetField(new TypeCodeField(fResData->GetType(), fResData), 1);
		SetField(new BStringField(fResData->GetName()), 2);
		BField *field = gResRoster.MakeFieldForType(fResData->GetType(),
													fResData->GetData(),
													fResData->GetLength());
		if (field)
			SetField(field, 3);
		SetField(new BSizeField(fResData->GetLength()), 4);
	}
}
	

ResourceData *
ResDataRow::GetData(void) const
{
	return fResData;
}

/*
 * Copyright 2004-2006, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ExpanderApp.h"
#include "ExpanderWindow.h"
#include "ExpanderThread.h"
#include "ExpanderPreferences.h"


#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <Entry.h>
#include <File.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextView.h>


const uint32 MSG_SOURCE			= 'mSOU';
const uint32 MSG_DEST			= 'mDES';
const uint32 MSG_EXPAND			= 'mEXP';
const uint32 MSG_SHOW			= 'mSHO';
const uint32 MSG_STOP			= 'mSTO';
const uint32 MSG_PREFERENCES	= 'mPRE';
const uint32 MSG_SOURCETEXT		= 'mSTX';
const uint32 MSG_DESTTEXT		= 'mDTX';
const uint32 MSG_SHOWCONTENTS	= 'mSCT';


ExpanderWindow::ExpanderWindow(BRect frame, const entry_ref* ref,
		BMessage* settings)
	:
	BWindow(frame, "Expander", B_TITLED_WINDOW, B_NOT_ZOOMABLE),
	fSourcePanel(NULL),
	fDestPanel(NULL),
	fSourceChanged(true),
	fListingThread(NULL),
	fListingStarted(false),
	fExpandingThread(NULL),
	fExpandingStarted(false),
	fSettings(*settings),
	fPreferences(NULL)
{
	// create menu bar
	fBar = new BMenuBar(BRect(0, 0, Bounds().right, 20), "menu_bar");
	BMenu* menu = new BMenu("File");
	BMenuItem* item;
	menu->AddItem(item = new BMenuItem("About Expander" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));
	item->SetTarget(be_app_messenger);
	menu->AddSeparatorItem();
	menu->AddItem(fSourceItem = new BMenuItem("Set source" B_UTF8_ELLIPSIS,
		new BMessage(MSG_SOURCE), 'O'));
	menu->AddItem(fDestItem = new BMenuItem("Set destination" B_UTF8_ELLIPSIS,
		new BMessage(MSG_DEST), 'D'));
	menu->AddSeparatorItem();
	menu->AddItem(fExpandItem = new BMenuItem("Expand", new BMessage(MSG_EXPAND),
		'E'));
	fExpandItem->SetEnabled(false);
	menu->AddItem(fShowItem = new BMenuItem("Show contents",
		new BMessage(MSG_SHOW), 'L'));
	fShowItem->SetEnabled(false);
	menu->AddSeparatorItem();
	menu->AddItem(fStopItem = new BMenuItem("Stop", new BMessage(MSG_STOP), 'K'));
	fStopItem->SetEnabled(false);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W'));
	fBar->AddItem(menu);

	menu = new BMenu("Settings");
	menu->AddItem(fPreferencesItem = new BMenuItem("Settings" B_UTF8_ELLIPSIS,
		new BMessage(MSG_PREFERENCES), 'S'));
	fBar->AddItem(menu);
	AddChild(fBar);

	BRect rect = Bounds();
	rect.top += fBar->Bounds().Height() + 1;
	BView* topView = new BView(rect, "background", B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	// Buttons

	rect = topView->Bounds().InsetByCopy(8, 8);
	fDestButton = new BButton(rect, "destButton", "Destination",
		new BMessage(MSG_DEST), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fDestButton->ResizeToPreferred();

	rect = fDestButton->Frame();
	fDestButton->MoveBy(0, rect.Height() + 4);

	fSourceButton = new BButton(rect, "sourceButton", "Source",
		new BMessage(MSG_SOURCE), B_FOLLOW_LEFT | B_FOLLOW_TOP);

	rect = fDestButton->Frame();
	rect.OffsetBy(0, rect.Height() + 4);
	fExpandButton = new BButton(rect, "expandButton", "Expand",
		new BMessage(MSG_EXPAND), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fExpandButton->SetEnabled(false);

	// TextControls

	rect = fSourceButton->Frame();
	rect.left = rect.right + 8;
	rect.right = frame.Width() - 8;
	fSourceText = new BTextControl(rect, "sourceText", "", NULL,
		new BMessage(MSG_SOURCETEXT), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fSourceText->SetDivider(0);
	float width, height;
	fSourceText->GetPreferredSize(&width, &height);
	fSourceText->ResizeTo(rect.Width(), height);
	float offset = (rect.Height() - fSourceText->Bounds().Height()) / 2;
	fSourceText->MoveBy(0, offset);
	topView->AddChild(fSourceButton);
	topView->AddChild(fSourceText);

	rect = fSourceText->Frame();
	rect.OffsetBy(0, fSourceButton->Bounds().Height() + 4);
	fDestText = new BTextControl(rect, "destText", "", NULL,
		new BMessage(MSG_DESTTEXT), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fDestText->SetDivider(0);
	topView->AddChild(fDestButton);
	topView->AddChild(fDestText);

	rect.OffsetBy(0, fSourceButton->Bounds().Height() + 4 + offset);
	fShowContents = new BCheckBox(rect, "showContents", "Show contents",
		new BMessage(MSG_SHOWCONTENTS), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	fShowContents->ResizeToPreferred();
	fShowContents->MoveTo(Bounds().right - 8 - fShowContents->Bounds().Width(),
		rect.top);
	topView->AddChild(fExpandButton);
	topView->AddChild(fShowContents);
	fShowContents->SetEnabled(false);

	rect.left = fExpandButton->Frame().right + 8;
	rect.right = fShowContents->Frame().left - 4;
	fStatusView = new BStringView(rect, "statusView", "",
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS);
	topView->AddChild(fStatusView);

	ResizeTo(frame.Width(), topView->Frame().top + fExpandButton->Frame().bottom + 8);
	SetSizeLimits(fExpandButton->Bounds().Width() * 3 + fShowContents->Bounds().Width() + 24,
		32767.0f, Frame().Height(), Frame().Height());

	rect = topView->Bounds();
	rect.InsetBy(10, 0);
	rect.top = rect.bottom + 2;
	rect.bottom -= 10;
	rect.right -= B_V_SCROLL_BAR_WIDTH;

	BRect textRect = rect;
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(1, 1);
	fListingText = new BTextView(rect, "listingText", textRect,
		be_fixed_font, NULL, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);
	fListingText->SetText("");
	fListingText->MakeEditable(false);
	fListingScroll = new BScrollView("listingScroll", fListingText,
		B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS, false, true);
	topView->AddChild(fListingScroll);
	fListingScroll->Hide();

	// finish creating window
	Show();
}


ExpanderWindow::~ExpanderWindow()
{
	if (fDestPanel && fDestPanel->RefFilter())
		delete fDestPanel->RefFilter();

	if (fSourcePanel && fSourcePanel->RefFilter())
		delete fSourcePanel->RefFilter();

	delete fDestPanel;
	delete fSourcePanel;
}


bool 
ExpanderWindow::ValidateDest()
{
	BEntry entry(fDestText->Text(), true);
	BVolume volume;
	if (!entry.Exists()) {
		BAlert* alert = new BAlert("destAlert", "The destination"
			" folder does not exist.", "Cancel", NULL, NULL,
			B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
		alert->Go();
		return false;
	} else if (!entry.IsDirectory()) {
		(new BAlert("destAlert", "The destination"
			" is not a folder.", "Cancel", NULL, NULL,
			B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT))->Go();
		return false;
	} else if (entry.GetVolume(&volume) != B_OK || volume.IsReadOnly()) {
		(new BAlert("destAlert", "The destination is read only.",
			"Cancel", NULL, NULL, B_WIDTH_AS_USUAL, B_EVEN_SPACING,
			B_WARNING_ALERT))->Go();
		return false;
	} else {
		entry.GetRef(&fDestRef);
		return true;
	}
}


void
ExpanderWindow::FrameResized(float width, float height)
{
	if (fListingText->DoesWordWrap()) {
		BRect textRect;
		textRect = fListingText->Bounds();
		textRect.OffsetTo(B_ORIGIN);
		textRect.InsetBy(1, 1);
		fListingText->SetTextRect(textRect);
	}
}


void
ExpanderWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_SOURCE:
		{
			BEntry entry(fSourceText->Text(), true);
			entry_ref srcRef;
			if (entry.Exists() && entry.IsDirectory())
				entry.GetRef(&srcRef);
			if (!fSourcePanel) {
				BMessenger messenger(this);
				fSourcePanel = new BFilePanel(B_OPEN_PANEL, &messenger, &srcRef,
					B_FILE_NODE, false, NULL, new RuleRefFilter(fRules), true);
			} else
				fSourcePanel->SetPanelDirectory(&srcRef);
			fSourcePanel->Show();
			break;
		}

		case MSG_DEST:
		{	
			BEntry entry(fDestText->Text(), true);
			entry_ref destRef;
			if (entry.Exists() && entry.IsDirectory())
				entry.GetRef(&destRef);
			if (!fDestPanel) {
				BMessenger messenger(this);
				fDestPanel = new DirectoryFilePanel(B_OPEN_PANEL, &messenger,
					&destRef, B_DIRECTORY_NODE, false, NULL,
					new DirectoryRefFilter(), true);
			} else 
				fDestPanel->SetPanelDirectory(&destRef);
			fDestPanel->Show();
			break;
		}

		case MSG_DIRECTORY:
		{
			entry_ref ref;
			fDestPanel->GetPanelDirectory(&ref);
			fDestRef = ref;
			BEntry entry(&ref);
			BPath path(&entry);
			fDestText->SetText(path.Path());
			fDestPanel->Hide();
			break;
		}

		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			RefsReceived(msg);
			break;

		case MSG_EXPAND:
			if (!ValidateDest())
				break;
			if (!fExpandingStarted) {
				StartExpanding();
				break;
			}
			// supposed to fall through
		case MSG_STOP:
			if (fExpandingStarted) {
				fExpandingThread->SuspendExternalExpander();
				BAlert* alert = new BAlert("stopAlert", "Are you sure you want "
					"to stop expanding this\narchive? The expanded items may "
					"not be complete.", "Stop", "Continue", NULL,
					B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
				if (alert->Go() == 0) {
					fExpandingThread->ResumeExternalExpander();
					StopExpanding();
				} else
					fExpandingThread->ResumeExternalExpander();
			}
			break;

		case MSG_SHOW:
			fShowContents->SetValue(fShowContents->Value() == B_CONTROL_OFF
				? B_CONTROL_ON : B_CONTROL_OFF);
			// supposed to fall through
		case MSG_SHOWCONTENTS:
			// change menu item label
			fShowItem->SetLabel(fShowContents->Value() == B_CONTROL_OFF
				? "Show contents" : "Hide contents");

			if (fShowContents->Value() == B_CONTROL_OFF) {
				if (fListingStarted)
					StopListing();

				_UpdateWindowSize(false);
			} else
				StartListing();
			break;

		case MSG_SOURCETEXT:
		{
			BEntry entry(fSourceText->Text(), true);
			if (!entry.Exists()) {
				BAlert* alert = new BAlert("srcAlert", "The file doesn't exist",
					"Cancel", NULL, NULL,
					B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
				alert->Go();
				break;
			}

			entry_ref ref;
			entry.GetRef(&ref);
			ExpanderRule* rule = fRules.MatchingRule(&ref);
			if (rule) {
				fSourceChanged = true;
				fSourceRef = ref;
				fShowContents->SetEnabled(true);
				fExpandButton->SetEnabled(true);
				fExpandItem->SetEnabled(true);
				fShowItem->SetEnabled(true);
				break;
			}

			BString string = "The file : ";
			string += fSourceText->Text();
			string += " is not supported";
			BAlert* alert = new BAlert("srcAlert", string.String(), "Cancel",
				NULL, NULL, B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_INFO_ALERT);
			alert->Go();

			fShowContents->SetEnabled(false);
			fExpandButton->SetEnabled(false);
			fExpandItem->SetEnabled(false);
			fShowItem->SetEnabled(false);
		}
		break;

		case MSG_DESTTEXT:
			ValidateDest();
			break;

		case MSG_PREFERENCES:
			if (!fPreferences)
				fPreferences = new ExpanderPreferences(&fSettings);
			fPreferences->Show();
			break;

		case 'outp':
			if (!fExpandingStarted && fListingStarted) {
				BString string;
				int32 i = 0;
				while (msg->FindString("output", i++, &string) == B_OK) {
					// expand the window if we need...
					float delta = fListingText->StringWidth(string.String())
						- fListingText->Frame().Width();
					if (delta > fLargestDelta) {
						fLargestDelta = delta;
					}
					fListingText->Insert(string.String());
				}
				fListingText->ScrollToSelection();
			}
			break;

		case 'exit':
			// thread has finished		(finished, quit, killed, we don't know)
			// reset window state
			if (fExpandingStarted) {
				fStatusView->SetText("File expanded");
				StopExpanding();
				OpenDestFolder();
				CloseWindowOrKeepOpen();
			} else if (fListingStarted){
				fSourceChanged = false;
				StopListing();
				if (fLargestDelta > 0.0f)
					ResizeBy(fLargestDelta, 0.0f);
				fLargestDelta = 0.0f;
			} else
				fStatusView->SetText("");
			break;

		case 'exrr':	// thread has finished
			// reset window state

			fStatusView->SetText("Error when expanding archive");
			CloseWindowOrKeepOpen();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


bool
ExpanderWindow::CanQuit()
{
	if ((fSourcePanel && fSourcePanel->IsShowing())
		|| (fDestPanel && fDestPanel->IsShowing()))
		return false;

	if (fExpandingStarted) {
		fExpandingThread->SuspendExternalExpander();
		BAlert* alert = new BAlert("stopAlert", "Are you sure you want to stop "
			"expanding this\narchive? The expanded items may not be complete.",
			"Stop", "Continue", NULL, B_WIDTH_AS_USUAL, B_EVEN_SPACING,
			B_WARNING_ALERT);
		if (alert->Go() == 0) {
			fExpandingThread->ResumeExternalExpander();
			StopExpanding();
		} else {
			fExpandingThread->ResumeExternalExpander();
			return false;
		}
	}
	return true;
}


bool
ExpanderWindow::QuitRequested()
{
	if (!CanQuit())
		return false;

	if (fListingStarted)
		StopListing();

	be_app->PostMessage(B_QUIT_REQUESTED);
	fSettings.ReplacePoint("window_position", Frame().LeftTop());
	((ExpanderApp*)(be_app))->UpdateSettingsFrom(&fSettings);
	return true;
}


void
ExpanderWindow::RefsReceived(BMessage* msg)
{
	entry_ref ref;
	int32 i = 0;
	int8 destination_folder = 0x63;
	fSettings.FindInt8("destination_folder", &destination_folder);

	while (msg->FindRef("refs", i++, &ref) == B_OK) {
		BEntry entry(&ref, true);
		BPath path(&entry);
		BNode node(&entry);

		if (node.IsFile()) {
			fSourceChanged = true;
			fSourceRef = ref;
			fSourceText->SetText(path.Path());
			if (destination_folder == 0x63) {
				BPath parent;
				path.GetParent(&parent);
				fDestText->SetText(parent.Path());
				get_ref_for_path(parent.Path(), &fDestRef);
			} else if (destination_folder == 0x65) {
				fSettings.FindRef("destination_folder_use", &fDestRef);
				BEntry dEntry(&fDestRef, true);
				BPath dPath(&dEntry);
				fDestText->SetText(dPath.Path());
			}

			BEntry dEntry(&fDestRef, true);
			if (dEntry.Exists()) {
				fExpandButton->SetEnabled(true);
				fExpandItem->SetEnabled(true);
			}

			if (fShowContents->Value() == B_CONTROL_ON) {
				StopListing();
				StartListing();
			} else {
				fShowContents->SetEnabled(true);
				fShowItem->SetEnabled(true);
			}

			bool fromApp;
			if (msg->FindBool("fromApp", &fromApp) == B_OK) {
				AutoExpand();
			} else
				AutoListing();
		} else if (node.IsDirectory()) {
			fDestRef = ref;
			fDestText->SetText(path.Path());
		}
	}
}


void
ExpanderWindow::StartExpanding()
{
	ExpanderRule* rule = fRules.MatchingRule(&fSourceRef);
	if (!rule)
		return;

	BEntry destEntry(fDestText->Text(), true);
	if (!destEntry.Exists()) {
		BAlert* alert = new BAlert("destAlert", "The folder was either moved, "
			"renamed or not\nsupported.", "Cancel", NULL, NULL,
			B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
		alert->Go();
		return;
	}

	BMessage message;
	message.AddString("cmd", rule->ExpandCmd());
	message.AddRef("srcRef", &fSourceRef);
	message.AddRef("destRef", &fDestRef);

	fExpandButton->SetLabel("Stop");
	fSourceButton->SetEnabled(false);
	fDestButton->SetEnabled(false);
	fShowContents->SetEnabled(false);
	fSourceItem->SetEnabled(false);
	fDestItem->SetEnabled(false);
	fExpandItem->SetEnabled(false);
	fShowItem->SetEnabled(false);
	fStopItem->SetEnabled(true);
	fPreferencesItem->SetEnabled(false);

	BEntry entry(&fSourceRef);
	BPath path(&entry);
	BString text("Expanding file ");
	text.Append(path.Leaf());
	fStatusView->SetText(text.String());

	fExpandingThread = new ExpanderThread(&message, new BMessenger(this));
	fExpandingThread->Start();

	fExpandingStarted = true;
}


void
ExpanderWindow::StopExpanding(void)
{
	if (fExpandingThread) {
		fExpandingThread->InterruptExternalExpander();
		fExpandingThread = NULL;
	}

	fExpandingStarted = false;

	fExpandButton->SetLabel("Expand");
	fSourceButton->SetEnabled(true);
	fDestButton->SetEnabled(true);
	fShowContents->SetEnabled(true);
	fSourceItem->SetEnabled(true);
	fDestItem->SetEnabled(true);
	fExpandItem->SetEnabled(true);
	fShowItem->SetEnabled(true);
	fStopItem->SetEnabled(false);
	fPreferencesItem->SetEnabled(true);
}


void
ExpanderWindow::_UpdateWindowSize(bool showContents)
{
	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);

	float bottom = fExpandButton->Parent()->Frame().top + fExpandButton->Frame().bottom + 8;

	if (showContents) {
		font_height fontHeight;
		be_plain_font->GetHeight(&fontHeight);
		float lineHeight = ceilf(fontHeight.ascent + fontHeight.descent + fontHeight.leading);

		minHeight = bottom + 14 + 4 * lineHeight;
		maxHeight = 32767.0;
		bottom = minHeight + 10 * lineHeight;
	} else {
		minHeight = bottom;
		maxHeight = bottom;
	}

	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);
	ResizeTo(Frame().Width(), bottom);
}


void
ExpanderWindow::StartListing()
{
	_UpdateWindowSize(true);

	fLargestDelta = 0.0f;

	if (fListingScroll->IsHidden())
		fListingScroll->Show();

	if (!fSourceChanged)
		return;

	ExpanderRule* rule = fRules.MatchingRule(&fSourceRef);
	if (!rule)
		return;

	BMessage message;
	message.AddString("cmd", rule->ListingCmd());
	message.AddRef("srcRef", &fSourceRef);

	fShowContents->SetEnabled(true);
	fSourceItem->SetEnabled(false);
	fDestItem->SetEnabled(false);
	fExpandItem->SetEnabled(false);
	fShowItem->SetEnabled(true);
	fShowItem->SetLabel("Hide contents");
	fStopItem->SetEnabled(false);
	fPreferencesItem->SetEnabled(false);

	fSourceButton->SetEnabled(false);
	fDestButton->SetEnabled(false);
	fExpandButton->SetEnabled(false);

	BEntry entry(&fSourceRef);
	BPath path(&entry);
	BString text("Creating listing for ");
	text.Append(path.Leaf());
	fStatusView->SetText(text.String());
	fListingText->SetText("");

	fListingThread = new ExpanderThread(&message, new BMessenger(this));
	fListingThread->Start();

	fListingStarted = true;
}


void
ExpanderWindow::StopListing(void)
{
	if (fListingThread) {
		fListingThread->InterruptExternalExpander();
		fListingThread = NULL;
	}

	fListingStarted = false;

	fShowContents->SetEnabled(true);
	fSourceItem->SetEnabled(true);
	fDestItem->SetEnabled(true);
	fExpandItem->SetEnabled(true);
	fShowItem->SetEnabled(true);
	fStopItem->SetEnabled(false);
	fPreferencesItem->SetEnabled(true);

	fSourceButton->SetEnabled(true);
	fDestButton->SetEnabled(true);
	fExpandButton->SetEnabled(true);
	fStatusView->SetText("");
}


void
ExpanderWindow::CloseWindowOrKeepOpen()
{
	bool expandFiles = false;
	fSettings.FindBool("automatically_expand_files", &expandFiles);

	bool closeWhenDone = false;
	fSettings.FindBool("close_when_done", &closeWhenDone);

	if (expandFiles || closeWhenDone)
		PostMessage(B_QUIT_REQUESTED);
}


void
ExpanderWindow::OpenDestFolder()
{
	bool openFolder = true;
	fSettings.FindBool("open_destination_folder", &openFolder);

	if (!openFolder)
		return;

	BMessage* message = new BMessage(B_REFS_RECEIVED);
	message->AddRef("refs", &fDestRef);
	BPath path(&fDestRef);
	BMessenger tracker("application/x-vnd.Be-TRAK");
	tracker.SendMessage(message);
}


void
ExpanderWindow::AutoListing()
{
	bool showContents = false;
	fSettings.FindBool("show_contents_listing", &showContents);

	if (!showContents)
		return;

	fShowContents->SetValue(B_CONTROL_ON);
	fShowContents->Invoke();
}


void
ExpanderWindow::AutoExpand()
{
	bool expandFiles = false;
	fSettings.FindBool("automatically_expand_files", &expandFiles);

	if (!expandFiles) {
		AutoListing();
		return;
	}

	fExpandButton->Invoke();
}

/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "ProbeView.h"
#include "DataView.h"

#define BEOS_R5_COMPATIBLE
	// for SetLimits()

#include <Window.h>
#include <TextControl.h>
#include <StringView.h>
#include <Slider.h>
#include <Bitmap.h>
#include <Box.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollView.h>
#include <String.h>
#include <Entry.h>
#include <Path.h>
#include <NodeInfo.h>
#include <Node.h>
#include <NodeMonitor.h>

#include <stdio.h>


#define DRAW_SLIDER_BAR
	// if this is defined, the standard slider bar is replaced with
	// one that looks exactly like the one in the original DiskProbe
	// (even in Dano/Zeta)

static const uint32 kMsgSliderUpdate = 'slup';
static const uint32 kMsgPositionUpdate = 'poup';


class IconView : public BView {
	public:
		IconView(BRect frame, entry_ref *ref, bool isDevice);
		virtual ~IconView();

		virtual void AttachedToWindow();
		virtual void Draw(BRect updateRect);

		void UpdateIcon();

	private:
		entry_ref	fRef;
		bool		fIsDevice;
		BBitmap		*fBitmap;
};


class PositionSlider : public BSlider {
	public:
		PositionSlider(BRect rect, const char *name, BMessage *message,
			off_t size, uint32 blockSize);
		virtual ~PositionSlider();

#ifdef DRAW_SLIDER_BAR
		virtual void DrawBar();
#endif

		off_t Position() const;
		off_t Size() const { return fSize; }
		uint32 BlockSize() const { return fBlockSize; }

		void SetPosition(off_t position);
		void SetSize(off_t size);
		void SetBlockSize(uint32 blockSize);

	private:
		void Reset();

		static const int32 kMaxSliderLimit = 0x7fffff80;
			// this is the maximum value that BSlider seem to work with fine

		off_t	fSize;
		uint32	fBlockSize;
};


class HeaderView : public BView, public BInvoker {
	public:
		HeaderView(BRect frame, entry_ref *ref, DataEditor &editor);
		virtual ~HeaderView();

		virtual void AttachedToWindow();
		virtual void Draw(BRect updateRect);
		virtual void GetPreferredSize(float *_width, float *_height);
		virtual void MessageReceived(BMessage *message);

		base_type Base() const { return fBase; }
		void SetBase(base_type);

	private:
		void FormatValue(char *buffer, size_t bufferSize, off_t value);
		void UpdatePositionViews(off_t position);
		void UpdateOffsetViews(off_t position, bool all = true);
		void UpdateFileSizeView();

		const char		*fAttribute;
		off_t			fFileSize;
		uint32			fBlockSize;
		off_t			fOffset;
		base_type		fBase;

		BTextControl	*fPositionControl;
		BStringView		*fPathView;
		BStringView		*fSizeView;
		BStringView		*fOffsetView;
		BStringView		*fFileOffsetView;
		PositionSlider	*fPositionSlider;
		IconView		*fIconView;
};


//----------------------


IconView::IconView(BRect rect, entry_ref *ref, bool isDevice)
	: BView(rect, NULL, B_FOLLOW_NONE, B_WILL_DRAW),
	fRef(*ref),
	fIsDevice(isDevice),
	fBitmap(NULL)
{
	UpdateIcon();
}


IconView::~IconView()
{
	delete fBitmap;
}


void 
IconView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void 
IconView::Draw(BRect updateRect)
{
	if (fBitmap == NULL)
		return;

	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fBitmap, updateRect, updateRect);
	SetDrawingMode(B_OP_COPY);
}


void 
IconView::UpdateIcon()
{
	if (fBitmap == NULL)
		fBitmap = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);

	if (fBitmap != NULL) {
		status_t status = B_ERROR;

		if (fIsDevice) {
			BPath path(&fRef);
			status = get_device_icon(path.Path(), fBitmap->Bits(), B_LARGE_ICON);
		} else
			status = BNodeInfo::GetTrackerIcon(&fRef, fBitmap);

		if (status != B_OK) {
			// ToDo: get a standard generic icon here?
			delete fBitmap;
			fBitmap = NULL;
		}

		Invalidate();
	}
}


//	#pragma mark -


PositionSlider::PositionSlider(BRect rect, const char *name, BMessage *message,
	off_t size, uint32 blockSize)
	: BSlider(rect, name, NULL, message, 0, kMaxSliderLimit, B_HORIZONTAL,
		B_TRIANGLE_THUMB, B_FOLLOW_LEFT_RIGHT),
	fSize(size),
	fBlockSize(blockSize)
{
	Reset();

#ifndef DRAW_SLIDER_BAR
	rgb_color color =  ui_color(B_CONTROL_HIGHLIGHT_COLOR);
	UseFillColor(true, &color);
#endif
}


PositionSlider::~PositionSlider()
{
}


#ifdef DRAW_SLIDER_BAR
void 
PositionSlider::DrawBar()
{
	BView *view = OffscreenView();

	BRect barFrame = BarFrame();
	BRect frame = barFrame.InsetByCopy(1, 1);
	frame.top++;
	frame.left++;
	frame.right = ThumbFrame().left + ThumbFrame().Width() / 2;
	view->SetHighColor(ui_color(B_CONTROL_HIGHLIGHT_COLOR)); //102, 152, 203);
		// ToDo: the color should probably be retrieved from one of the ui colors
	view->FillRect(frame);

	frame.left = frame.right + 1;
	frame.right = barFrame.right - 1;
	view->SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	view->FillRect(frame);

	rgb_color cornerColor = tint_color(ViewColor(), B_DARKEN_1_TINT);
	rgb_color darkColor = tint_color(ViewColor(), B_DARKEN_3_TINT);

	view->BeginLineArray(9);

	// the corners
	view->AddLine(barFrame.LeftTop(), barFrame.LeftTop(), cornerColor);
	view->AddLine(barFrame.LeftBottom(), barFrame.LeftBottom(), cornerColor);
	view->AddLine(barFrame.RightTop(), barFrame.RightTop(), cornerColor);

	// the edges
	view->AddLine(BPoint(barFrame.left, barFrame.top + 1),
		BPoint(barFrame.left, barFrame.bottom - 1), darkColor);
	view->AddLine(BPoint(barFrame.right, barFrame.top + 1),
		BPoint(barFrame.right, barFrame.bottom), ui_color(B_SHINE_COLOR));

	barFrame.left++;
	barFrame.right--;
	view->AddLine(barFrame.LeftTop(), barFrame.RightTop(), darkColor);
	view->AddLine(barFrame.LeftBottom(), barFrame.RightBottom(), ui_color(B_SHINE_COLOR));

	// the inner edges
	barFrame.top++;
	view->AddLine(barFrame.LeftTop(), barFrame.RightTop(), ui_color(B_SHADOW_COLOR));
	view->AddLine(BPoint(barFrame.left, barFrame.top + 1),
		BPoint(barFrame.left, barFrame.bottom - 1), ui_color(B_SHADOW_COLOR));

	view->EndLineArray();
}
#endif	// DRAW_SLIDER_BAR


void
PositionSlider::Reset()
{
	SetKeyIncrementValue(int32(1.0 * kMaxSliderLimit / ((fSize - 1) / fBlockSize) + 0.5));
}


off_t
PositionSlider::Position() const
{
	// ToDo:
	// Note: this code is far from being perfect: depending on the file size, it has
	//	a maxium granularity that might be less than the actual block size demands...
	//	The only way to work around this that I can think of, is to replace the slider
	//	class completely with one that understands off_t values.
	//	For example, with a block size of 512 bytes, it should be good enough for about
	//	1024 GB - and that's not really that far away these days.

	return (off_t(1.0 * (fSize - 1) * Value() / kMaxSliderLimit + 0.5) / fBlockSize) * fBlockSize;
}


void
PositionSlider::SetPosition(off_t position)
{
	position /= fBlockSize;
	SetValue(int32(1.0 * kMaxSliderLimit * position / ((fSize - 1) / fBlockSize) + 0.5));
}


void
PositionSlider::SetSize(off_t size)
{
	if (size == fSize)
		return;

	off_t position = Position();
	if (position >= size)
		position = size - 1;

	fSize = size;
	Reset();
	SetPosition(position);
}


void 
PositionSlider::SetBlockSize(uint32 blockSize)
{
	if (blockSize == fBlockSize)
		return;

	off_t position = Position();
	fBlockSize = blockSize;
	Reset();
	SetPosition(position);
}


//	#pragma mark -


HeaderView::HeaderView(BRect frame, entry_ref *ref, DataEditor &editor)
	: BView(frame, "probeHeader", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW),
	fAttribute(editor.Attribute()),
	fFileSize(editor.FileSize()),
	fBlockSize(editor.BlockSize()),
	fOffset(0),
	fBase(kHexBase)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fIconView = new IconView(BRect(10, 10, 41, 41), ref, editor.IsDevice());
	AddChild(fIconView);

	BFont boldFont = *be_bold_font;
	boldFont.SetSize(10.0);
	BFont plainFont = *be_plain_font;
	plainFont.SetSize(10.0);

	BStringView *stringView = new BStringView(BRect(50, 6, frame.right, 20),
		B_EMPTY_STRING, editor.IsAttribute() ? "Attribute: " : editor.IsDevice() ? "Device: " : "File: ");
	stringView->SetFont(&boldFont);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	BPath path(ref);
	BString string = path.Path();
	if (fAttribute != NULL) {
		string.Prepend(" (");
		string.Prepend(fAttribute);
		string.Append(")");
	}
	BRect rect = stringView->Frame();
	rect.left = rect.right;
	rect.right = frame.right;
	fPathView = new BStringView(rect, B_EMPTY_STRING, string.String());
	fPathView->SetFont(&plainFont);
	AddChild(fPathView);

	stringView = new BStringView(BRect(50, 27, frame.right, 50), B_EMPTY_STRING, "Block: ");
	stringView->SetFont(&boldFont);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect = stringView->Frame();
	rect.left = rect.right;
	rect.right += 75;
	rect.OffsetBy(0, -2);
		// BTextControl oddities
	fPositionControl = new BTextControl(rect, B_EMPTY_STRING, NULL, "0x0", new BMessage(kMsgPositionUpdate));
	fPositionControl->SetDivider(0.0);
	fPositionControl->SetFont(&plainFont);
	fPositionControl->TextView()->SetFontAndColor(&plainFont);
	fPositionControl->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	AddChild(fPositionControl);

	rect.left = rect.right + 4;
	rect.right = rect.left + 75;
	rect.OffsetBy(0, 2);
	fSizeView = new BStringView(rect, B_EMPTY_STRING, "of 0x0");
	fSizeView->SetFont(&plainFont);
	AddChild(fSizeView);
	UpdateFileSizeView();

	rect.left = rect.right + 4;
	rect.right = frame.right;
	stringView = new BStringView(rect, B_EMPTY_STRING, "Offset: ");
	stringView->SetFont(&boldFont);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect = stringView->Frame();
	rect.left = rect.right;
	rect.right = rect.left + 40;
	fOffsetView = new BStringView(rect, B_EMPTY_STRING, "0x0");
	fOffsetView->SetFont(&plainFont);
	AddChild(fOffsetView);
	UpdateOffsetViews(0LL, false);

	rect.left = rect.right + 4;
	rect.right = frame.right;
	stringView = new BStringView(rect, B_EMPTY_STRING,
		editor.IsAttribute() ? "Attribute Offset: " : editor.IsDevice() ? "Device Offset: " : "File Offset: ");
	stringView->SetFont(&boldFont);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect = stringView->Frame();
	rect.left = rect.right;
	rect.right = rect.left + 70;
	fFileOffsetView = new BStringView(rect, B_EMPTY_STRING, "0x0");
	fFileOffsetView->SetFont(&plainFont);
	AddChild(fFileOffsetView);

	rect = Bounds();
	rect.InsetBy(3, 0);
	rect.top = 48;
	rect.bottom = 60;
	fPositionSlider = new PositionSlider(rect, "slider", new BMessage(kMsgSliderUpdate),
		editor.FileSize(), editor.BlockSize());
	fPositionSlider->SetModificationMessage(new BMessage(kMsgSliderUpdate));
	fPositionSlider->SetBarThickness(8);
	fPositionSlider->ResizeToPreferred();
	AddChild(fPositionSlider);
}


HeaderView::~HeaderView()
{
}


void 
HeaderView::AttachedToWindow()
{
	SetTarget(Window());

	fPositionControl->SetTarget(this);
	fPositionSlider->SetTarget(this);
}


void 
HeaderView::Draw(BRect updateRect)
{
	BRect rect = Bounds();

	SetHighColor(ui_color(B_SHINE_COLOR));
	StrokeLine(rect.LeftTop(), rect.LeftBottom());
	StrokeLine(rect.LeftTop(), rect.RightTop());

	// the gradient at the bottom is drawn by the BScrollView
}


void 
HeaderView::GetPreferredSize(float *_width, float *_height)
{
	if (_width)
		*_width = Bounds().Width();
	if (_height)
		*_height = fPositionSlider->Frame().bottom + 2;
}


void 
HeaderView::FormatValue(char *buffer, size_t bufferSize, off_t value)
{
	snprintf(buffer, bufferSize, fBase == kHexBase ? "0x%Lx" : "%Ld", value);
}


void
HeaderView::UpdatePositionViews(off_t position)
{
	char buffer[64];
	FormatValue(buffer, sizeof(buffer), position / fBlockSize);
	fPositionControl->SetText(buffer);

	FormatValue(buffer, sizeof(buffer), position + fOffset);
	fFileOffsetView->SetText(buffer);
}


void 
HeaderView::UpdateOffsetViews(off_t position, bool all)
{
	char buffer[64];
	FormatValue(buffer, sizeof(buffer), fOffset);
	fOffsetView->SetText(buffer);

	if (all) {
		FormatValue(buffer, sizeof(buffer), position + fOffset);
		fFileOffsetView->SetText(buffer);
	}
}


void 
HeaderView::UpdateFileSizeView()
{
	char buffer[64];
	strcpy(buffer, "of ");
	FormatValue(buffer + 3, sizeof(buffer) - 3, fFileSize / fBlockSize);
	fSizeView->SetText(buffer);
}


void 
HeaderView::SetBase(base_type type)
{
	if (fBase == type)
		return;

	fBase = type;

	off_t position = fPositionSlider->Position();
	UpdatePositionViews(position);
	UpdateOffsetViews(position, false);
	UpdateFileSizeView();
}


void 
HeaderView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgSliderUpdate:
		{
			off_t position = fPositionSlider->Position();

			// update position text control
			UpdatePositionViews(position);

			// notify our target
			BMessage update(kMsgPositionUpdate);
			update.AddInt64("position", position);
			Messenger().SendMessage(&update);
			break;
		}

		case kMsgPositionUpdate:
		{
			off_t position = strtoll(fPositionControl->Text(), NULL, 0) * fBlockSize;

			// update views
			UpdatePositionViews(position);
			fPositionSlider->SetPosition(position);

			// notify our target
			BMessage update(kMsgPositionUpdate);
			update.AddInt64("position", position);
			Messenger().SendMessage(&update);
			break;
		}

		case kMsgBaseType:
		{
			int32 type;
			if (message->FindInt32("base", &type) != B_OK)
				break;

			SetBase((base_type)type);
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}


//	#pragma mark -


ProbeView::ProbeView(BRect rect, entry_ref *ref, const char *attribute)
	: BView(rect, "probeView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fEditor.SetTo(*ref, attribute);

	rect = Bounds();
	fHeaderView = new HeaderView(rect, ref, fEditor);
	fHeaderView->ResizeToPreferred();
	AddChild(fHeaderView);

	rect = fHeaderView->Frame();
	rect.top = rect.bottom + 3;
	rect.bottom = Bounds().bottom - B_H_SCROLL_BAR_HEIGHT;
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	fDataView = new DataView(rect, fEditor);

	fScrollView = new BScrollView("scroller", fDataView, B_FOLLOW_ALL, B_WILL_DRAW, true, true);
	AddChild(fScrollView);

	fDataView->UpdateScroller();
}


ProbeView::~ProbeView()
{
}


void 
ProbeView::UpdateSizeLimits()
{
	if (Window() == NULL)
		return;

	float width, height;
	fDataView->GetPreferredSize(&width, &height);

	Window()->SetSizeLimits(200, width + B_V_SCROLL_BAR_WIDTH,
		200, height + fHeaderView->Frame().bottom + 4 + B_H_SCROLL_BAR_HEIGHT + Frame().top);
}


void 
ProbeView::DetachedFromWindow()
{
	fEditor.StopWatching(this);
}


void 
ProbeView::AddFileMenuItems(BMenu *menu, int32 index)
{
	BMenuItem *item;
	menu->AddItem(item = new BMenuItem("Page Setup" B_UTF8_ELLIPSIS, NULL), index++);
	item->SetEnabled(false);
	menu->AddItem(item = new BMenuItem("Print" B_UTF8_ELLIPSIS, NULL, 'P', B_COMMAND_KEY), index++);
	item->SetEnabled(false);
}


void 
ProbeView::AttachedToWindow()
{
	fEditor.StartWatching(this);

	// Add menu to window

	BMenuBar *bar = Window()->KeyMenuBar();
	if (bar == NULL) {
		// there is none? Well, but we really want to have one
		bar = new BMenuBar(BRect(0, 0, 0, 0), NULL);
		Window()->AddChild(bar);

		MoveBy(0, bar->Bounds().Height());
		ResizeBy(0, -bar->Bounds().Height());

		BMenu *menu = new BMenu(fEditor.IsAttribute() ? "Attribute" : fEditor.IsDevice() ? "Device" : "File");
		AddFileMenuItems(menu, 0);
		menu->AddSeparatorItem();

		menu->AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W', B_COMMAND_KEY));
		bar->AddItem(menu);
	}

	BMenu *menu = new BMenu("Edit");
	menu->AddItem(new BMenuItem("Undo", NULL, 'Z', B_COMMAND_KEY));
	menu->AddItem(new BMenuItem("Redo", NULL, 'Z', B_COMMAND_KEY | B_SHIFT_KEY));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Copy", NULL, 'C', B_COMMAND_KEY));
	menu->AddItem(new BMenuItem("Paste", NULL, 'V', B_COMMAND_KEY));
	menu->AddItem(new BMenuItem("Select All", NULL, 'A', B_COMMAND_KEY));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Find" B_UTF8_ELLIPSIS, NULL, 'F', B_COMMAND_KEY));
	menu->AddItem(new BMenuItem("Find Again", NULL, 'G', B_COMMAND_KEY));
	bar->AddItem(menu);

	menu = new BMenu("Block");
	menu->AddItem(new BMenuItem("Next", NULL, B_RIGHT_ARROW, B_COMMAND_KEY));
	menu->AddItem(new BMenuItem("Previous", NULL, B_LEFT_ARROW, B_COMMAND_KEY));
	menu->AddItem(new BMenuItem("Back", NULL, 'J', B_COMMAND_KEY));

	BMenu *subMenu = new BMenu("Selection");
	menu->AddItem(new BMenuItem(subMenu));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Write", NULL, 'J', B_COMMAND_KEY));
	menu->AddSeparatorItem();

	subMenu = new BMenu("Bookmarks");
	subMenu->AddItem(new BMenuItem("Add", NULL, 'B', B_COMMAND_KEY));
	menu->AddItem(new BMenuItem(subMenu));
	bar->AddItem(menu);

	// Number Base (hex/decimal)

	menu = new BMenu("View");
	subMenu = new BMenu("Base");
	BMenuItem *item;
	BMessage *message = new BMessage(kMsgBaseType);
	message->AddInt32("base", kDecimalBase);
	subMenu->AddItem(item = new BMenuItem("Decimal", message, 'D', B_COMMAND_KEY));
	item->SetTarget(this);
	if (fHeaderView->Base() == kDecimalBase)
		item->SetMarked(true);

	message = new BMessage(kMsgBaseType);
	message->AddInt32("base", kHexBase);
	subMenu->AddItem(item = new BMenuItem("Hex", message, 'H', B_COMMAND_KEY));
	item->SetTarget(this);
	if (fHeaderView->Base() == kHexBase)
		item->SetMarked(true);

	subMenu->SetRadioMode(true);
	menu->AddItem(new BMenuItem(subMenu));

	// Block Size

	subMenu = new BMenu("BlockSize");
	subMenu->AddItem(new BMenuItem("512", NULL));
	menu->AddItem(new BMenuItem(subMenu));
	menu->AddSeparatorItem();

	// Font Size

	subMenu = new BMenu("Font Size");
	const int32 fontSizes[] = {9, 10, 12, 14, 18, 24, 36, 48};
	for (uint32 i = 0; i < sizeof(fontSizes) / sizeof(fontSizes[0]); i++) {
		char buffer[16];
		snprintf(buffer, sizeof(buffer), "%ld", fontSizes[i]);
		subMenu->AddItem(new BMenuItem(buffer, NULL));
	}
	subMenu->AddSeparatorItem();
	subMenu->AddItem(new BMenuItem("Fit", NULL));

	menu->AddItem(new BMenuItem(subMenu));
	bar->AddItem(menu);
}


void 
ProbeView::AllAttached()
{
	fHeaderView->SetTarget(this);
}


void 
ProbeView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgBaseType:
		{
			int32 type;
			if (message->FindInt32("base", &type) != B_OK)
				break;

			fHeaderView->SetBase((base_type)type);
			fDataView->SetBase((base_type)type);
			break;
		}

		case kMsgPositionUpdate:
			// ToDo: update data
			break;

		default:
			BView::MessageReceived(message);
	}
}


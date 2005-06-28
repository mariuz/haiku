/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <PropertyInfo.h>
#include <Handler.h>
#include <View.h>
#include <Window.h>
#include <Message.h>
#include <MessageQueue.h>
#include <Rect.h>
#include <Point.h>
#include <Region.h>
#include <Font.h>
#include <ScrollBar.h>
#include <Cursor.h>
#include <Bitmap.h>
#include <Picture.h>
#include <Polygon.h>
#include <Shape.h>
#include <Button.h>
#include <Shelf.h>
#include <MenuBar.h>
#include <String.h>
#include <SupportDefs.h>
#include <Application.h>

#include <AppMisc.h>
#include <ViewAux.h>
#include <TokenSpace.h>
#include <MessageUtils.h>
#include <ColorUtils.h>
#include <AppServerLink.h>
#include <PortLink.h>
#include <ServerProtocol.h>

#include <stdio.h>


//#define DEBUG_BVIEW
#ifdef DEBUG_BVIEW
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef DEBUG_BVIEW
#	define BVTRACE PrintToStream()
#else
#	define BVTRACE ;
#endif

#define MAX_ATTACHMENT_SIZE 49152


static property_info sViewPropInfo[] = {
	{ "Frame", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the view's frame rectangle.",0 },

	{ "Frame", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the view's frame rectangle.",0 },

	{ "Hidden", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns true if the view is hidden; false otherwise.",0 },

	{ "Hidden", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Hides or shows the view.",0 },

	{ "Shelf", { 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Directs the scripting message to the shelf.",0 },

	{ "View", { B_COUNT_PROPERTIES, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the number of of child views.",0 },

	{ "View", { 0 },
		{ B_INDEX_SPECIFIER, 0 }, "Directs the scripting message to the specified view.",0 },

	{ "View", { 0 },
		{ B_REVERSE_INDEX_SPECIFIER, 0 }, "Directs the scripting message to the specified view.",0 },

	{ "View", { 0 },
		{ B_NAME_SPECIFIER, 0 }, "Directs the scripting message to the specified view.",0 },

	{ 0, { 0 }, { 0 }, 0, 0 }
}; 


// General Functions
//------------------------------------------------------------------------------


static inline uint32
get_uint32_color(rgb_color color)
{
	return *(uint32 *)&color;
}


static inline void
set_rgb_color(rgb_color &color, uint8 r, uint8 g, uint8 b, uint8 a = 255)
{
	color.red = r;
	color.green = g;
	color.blue = b;
	color.alpha = a;
}


//	#pragma mark -


namespace BPrivate {

ViewState::ViewState()
{
	pen_location.Set(0, 0);
	pen_size = 1.0;

	// This probably needs to be set to bounds by the owning BView
	clipping_region.MakeEmpty();

	set_rgb_color(high_color, 0, 0, 0);
	set_rgb_color(low_color, 255, 255, 255);
	view_color = low_color;

	pattern = B_SOLID_HIGH;
	drawing_mode = B_OP_COPY;

	origin.Set(0, 0);

	line_join = B_BEVEL_JOIN;
	line_cap = B_BUTT_CAP;
	miter_limit = B_DEFAULT_MITER_LIMIT;

	alpha_source_mode = B_PIXEL_ALPHA;
	alpha_function_mode = B_ALPHA_OVERLAY;

	scale = 1.0;

	font = *be_plain_font;
	font_flags = font.Flags();
	font_aliasing = false;

	/*
		INFO: We include(invalidate) only B_VIEW_CLIP_REGION_BIT flag
		because we should get the clipping region from app_server.
		The other flags do not need to be included because the data they
		represent is already in sync with app_server - app_server uses the
		same init(default) values.
	*/
	valid_flags = ~B_VIEW_CLIP_REGION_BIT;

	archiving_flags = B_VIEW_COORD_BIT;
}


void
ViewState::UpdateServerFontState(BPrivate::PortLink &link)
{
	link.StartMessage(AS_LAYER_SET_FONT_STATE);
	link.Attach<uint16>(font_flags);

	// always present.
	if (font_flags & B_FONT_FAMILY_AND_STYLE) {
		uint32 fontID;
		fontID = font.FamilyAndStyle();

		link.Attach<uint32>(fontID);
	}

	if (font_flags & B_FONT_SIZE)
		link.Attach<float>(font.Size());

	if (font_flags & B_FONT_SHEAR)
		link.Attach<float>(font.Shear());

	if (font_flags & B_FONT_ROTATION)
		link.Attach<float>(font.Rotation());

	if (font_flags & B_FONT_SPACING)
		link.Attach<uint8>(font.Spacing());

	if (font_flags & B_FONT_ENCODING)
		link.Attach<uint8>(font.Encoding());

	if (font_flags & B_FONT_FACE)
		link.Attach<uint16>(font.Face());

	if (font_flags & B_FONT_FLAGS)
		link.Attach<uint32>(font.Flags());
}


void
ViewState::UpdateServerState(BPrivate::PortLink &link)
{
	UpdateServerFontState(link);

	link.StartMessage(AS_LAYER_SET_STATE);
	link.Attach<BPoint>(pen_location);
	link.Attach<float>(pen_size);
	link.Attach<rgb_color>(high_color);
	link.Attach<rgb_color>(low_color);
	link.Attach< ::pattern>(pattern);
	link.Attach<int8>((int8)drawing_mode);
	link.Attach<BPoint>(origin);
	link.Attach<int8>((int8)line_join);
	link.Attach<int8>((int8)line_cap);
	link.Attach<float>(miter_limit);
	link.Attach<int8>((int8)alpha_source_mode);
	link.Attach<int8>((int8)alpha_function_mode);
	link.Attach<float>(scale);
	link.Attach<bool>(font_aliasing);

	// we send the 'local' clipping region... if we have one...
	int32 count = clipping_region.CountRects();

	link.Attach<int32>(count);
	for (int32 i = 0; i < count; i++)
		link.Attach<BRect>(clipping_region.RectAt(i));

	//	Although we might have a 'local' clipping region, when we call
	//	BView::GetClippingRegion() we ask for the 'global' one and it
	//	is kept on server, so we must invalidate B_VIEW_CLIP_REGION_BIT flag

	valid_flags = ~B_VIEW_CLIP_REGION_BIT;
}

}	// namespace BPrivate


//	#pragma mark -


BView::BView(BRect frame, const char *name, uint32 resizingMode, uint32 flags)
	: BHandler(name)
{
	InitData(frame, name, resizingMode, flags);
}


BView::BView(BMessage *archive)
	: BHandler(archive)
{
	uint32 resizingMode;
	uint32 flags;
	BRect frame;

	archive->FindRect("_frame", &frame);
	if (archive->FindInt32("_resize_mode", (int32 *)&resizingMode) != B_OK)
		resizingMode = 0;

	if (archive->FindInt32("_flags", (int32 *)&flags) != B_OK)
		flags = 0;

	InitData(frame, Name(), resizingMode, flags);

	font_family family;
	font_style style;
	if (archive->FindString("_fname", 0, (const char **)&family) == B_OK
		&& archive->FindString("_fname", 1, (const char **)&style) == B_OK) {
		BFont font;
		font.SetFamilyAndStyle(family, style);

		float size;
		if (archive->FindFloat("_fflt", 0, &size) == B_OK)
			font.SetSize(size);

		float shear;
		if (archive->FindFloat("_fflt", 1, &shear) == B_OK)
			font.SetShear(shear);

		float rotation;
		if (archive->FindFloat("_fflt", 2, &rotation) == B_OK)
		 font.SetRotation(rotation);

		SetFont(&font, B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE
			| B_FONT_SHEAR | B_FONT_ROTATION);
	}

	rgb_color color;
	if (archive->FindInt32("_color", 0, (int32 *)&color) == B_OK)
		SetHighColor(color);
	if (archive->FindInt32("_color", 1, (int32 *)&color) == B_OK)
		SetLowColor(color);
	if (archive->FindInt32("_color", 2, (int32 *)&color) == B_OK)
		SetViewColor(color);

	uint32 evMask;
	uint32 options;
	if (archive->FindInt32("_evmask", 0, (int32 *)&evMask) == B_OK
		&& archive->FindInt32("_evmask", 1, (int32 *)&options) == B_OK)
		SetEventMask(evMask, options);

	BPoint origin;
	if (archive->FindPoint("_origin", &origin) == B_OK)
		SetOrigin(origin);

	float penSize;
	if (archive->FindFloat("_psize", &penSize) == B_OK)
		SetPenSize(penSize);

	BPoint penLocation;
	if ( archive->FindPoint("_ploc", &penLocation) == B_OK )
		MovePenTo( penLocation );

	int16 lineCap;
	int16 lineJoin;
	float lineMiter;
	if (archive->FindInt16("_lmcapjoin", 0, &lineCap) == B_OK
		&& archive->FindInt16("_lmcapjoin", 1, &lineJoin) == B_OK
		&& archive->FindFloat("_lmmiter", &lineMiter) == B_OK)
		SetLineMode((cap_mode)lineCap, (join_mode)lineJoin, lineMiter);

	int16 alphaBlend;
	int16 modeBlend;
	if (archive->FindInt16("_blend", 0, &alphaBlend) == B_OK
		&& archive->FindInt16("_blend", 1, &modeBlend) == B_OK)
		SetBlendingMode( (source_alpha)alphaBlend, (alpha_function)modeBlend);

	uint32 drawingMode;
	if (archive->FindInt32("_dmod", (int32 *)&drawingMode) == B_OK)
		SetDrawingMode((drawing_mode)drawingMode);

	BMessage msg;
	int32 i = 0;
	while (archive->FindMessage("_views", i++, &msg) == B_OK) {
		BArchivable *object = instantiate_object(&msg);
		BView *child = dynamic_cast<BView *>(object);
		if (child)
			AddChild(child);
	}
}


BArchivable *
BView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data , "BView"))
		return NULL;

	return new BView(data);
}


status_t
BView::Archive(BMessage *data, bool deep) const
{
	status_t retval = BHandler::Archive(data, deep);
	if (retval != B_OK)
		return retval;

	if (fState->archiving_flags & B_VIEW_COORD_BIT)
		data->AddRect("_frame", Bounds().OffsetToCopy(fParentOffset));

	if (fState->archiving_flags & B_VIEW_RESIZE_BIT)
		data->AddInt32("_resize_mode", ResizingMode());

	if (fState->archiving_flags & B_VIEW_FLAGS_BIT)
		data->AddInt32("_flags", Flags());

	if (fState->archiving_flags & B_VIEW_EVENT_MASK_BIT) {
		data->AddInt32("_evmask", fEventMask);
		data->AddInt32("_evmask", fEventOptions);
	}

	if (fState->archiving_flags & B_VIEW_FONT_BIT) {
		BFont font;
		GetFont(&font);

		font_family family;
		font_style style;
		font.GetFamilyAndStyle(&family, &style);
		data->AddString("_fname", family);
		data->AddString("_fname", style);

		data->AddFloat("_fflt", font.Size());
		data->AddFloat("_fflt", font.Shear());
		data->AddFloat("_fflt", font.Rotation());
	}

	// colors

	if (fState->archiving_flags & B_VIEW_HIGH_COLOR_BIT)
		data->AddInt32("_color", get_uint32_color(HighColor()));

	if (fState->archiving_flags & B_VIEW_LOW_COLOR_BIT)
		data->AddInt32("_color", get_uint32_color(LowColor()));

	if (fState->archiving_flags & B_VIEW_VIEW_COLOR_BIT)
		data->AddInt32("_color", get_uint32_color(ViewColor()));

//	NOTE: we do not use this flag any more
//	if ( 1 ){
//		data->AddInt32("_dbuf", 1);
//	}

	if (fState->archiving_flags & B_VIEW_ORIGIN_BIT)
		data->AddPoint("_origin", Origin());

	if (fState->archiving_flags & B_VIEW_PEN_SIZE_BIT)
		data->AddFloat("_psize", PenSize());

	if (fState->archiving_flags & B_VIEW_PEN_LOCATION_BIT)
		data->AddPoint("_ploc", PenLocation());

	if (fState->archiving_flags & B_VIEW_LINE_MODES_BIT) {
		data->AddInt16("_lmcapjoin", (int16)LineCapMode());
		data->AddInt16("_lmcapjoin", (int16)LineJoinMode());
		data->AddFloat("_lmmiter", LineMiterLimit());
	}

	if (fState->archiving_flags & B_VIEW_BLENDING_BIT) {
		source_alpha alphaSourceMode;
		alpha_function alphaFunctionMode;
		GetBlendingMode(&alphaSourceMode, &alphaFunctionMode);

		data->AddInt16("_blend", (int16)alphaSourceMode);
		data->AddInt16("_blend", (int16)alphaFunctionMode);
	}

	if (fState->archiving_flags & B_VIEW_DRAWING_MODE_BIT)
		data->AddInt32("_dmod", DrawingMode());

	if (deep) {
		int32 i = 0;
		BView *child;

		while ((child = ChildAt(i++)) != NULL) {
			BMessage childArchive;

			retval = child->Archive(&childArchive, deep);
			if (retval == B_OK)
				data->AddMessage("_views", &childArchive);
		}
	}

	return retval;
}


BView::~BView()
{
	STRACE(("BView(%s)::~BView()\n", this->Name()));

	if (owner)
		debugger("Trying to delete a view that belongs to a window. Call RemoveSelf first.");

	removeSelf();

	// TODO: see about BShelf! must I delete it here? is it deleted by the window?
	
	// we also delete all its childern

	BView *child = first_child;
	while (child) {
		BView *nextChild = child->next_sibling;

		deleteView(child);
		child = nextChild;
	}

	if (fVerScroller)
		fVerScroller->SetTarget((BView *)NULL);

	if (fHorScroller)
		fHorScroller->SetTarget((BView *)NULL);

	SetName(NULL);

	delete fPermanentState;
	delete fState;
	free(pr_state);
}


BRect
BView::Bounds() const
{
	// do we need to update our bounds?
	if (!fState->IsValid(B_VIEW_COORD_BIT) && owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_GET_COORD);

		int32 code;
		if (owner->fLink->FlushWithReply(code) == B_OK
			&& code == SERVER_TRUE) {
			owner->fLink->Read<BPoint>(const_cast<BPoint *>(&fParentOffset));
			owner->fLink->Read<BRect>(const_cast<BRect *>(&fBounds));
			fState->valid_flags |= B_VIEW_COORD_BIT;
		}
	}

	return fBounds;
}


void
BView::ConvertToParent(BPoint *point) const
{
	if (!parent)
		return;

	check_lock_no_pick();

	// TODO: handle scale

	// our local coordinate transformation
	*point -= Origin();

	// our bounds location within the parent
	*point += fParentOffset;
}


BPoint
BView::ConvertToParent(BPoint point) const
{
	ConvertToParent(&point);

	return point;
}


void
BView::ConvertFromParent(BPoint *point) const
{
	if (!parent)
		return;

	check_lock_no_pick();

	// TODO: handle scale

	// our bounds location within the parent
	*point -= fParentOffset;

	// our local coordinate transformation
	*point += Origin();
}


BPoint
BView::ConvertFromParent(BPoint point) const
{
	ConvertFromParent(&point);

	return point;
}


void
BView::ConvertToParent(BRect *rect) const
{
	if (!parent)
		return;

	check_lock_no_pick();

	// TODO: handle scale

	BPoint origin = Origin();

	// our local coordinate transformation
	rect->OffsetBy(-origin.x, -origin.y);

	// our bounds location within the parent
	rect->OffsetBy(fParentOffset);
}


BRect
BView::ConvertToParent(BRect rect) const
{
	ConvertToParent(&rect);

	return rect;
}


void
BView::ConvertFromParent(BRect *rect) const
{
	if (!parent)
		return;

	check_lock_no_pick();

	// TODO: handle scale

	// our bounds location within the parent
	rect->OffsetBy(-fParentOffset.x, -fParentOffset.y);

	// our local coordinate transformation
	rect->OffsetBy(Origin());
}


BRect
BView::ConvertFromParent(BRect rect) const
{
	ConvertFromParent(&rect);

	return rect;
}


void
BView::ConvertToScreen(BPoint *pt) const
{
	if (!parent) {
		if (owner)
			owner->ConvertToScreen(pt);

		return;
	}

	do_owner_check_no_pick();

	ConvertToParent(pt);
	parent->ConvertToScreen(pt);
}


BPoint
BView::ConvertToScreen(BPoint pt) const
{
	ConvertToScreen(&pt);

	return pt;
}


void
BView::ConvertFromScreen(BPoint *pt) const
{
	if (!parent) {
		if (owner)
			owner->ConvertFromScreen(pt);

		return;
	}

	do_owner_check_no_pick();

	ConvertFromParent(pt);
	parent->ConvertFromScreen(pt);
}


BPoint
BView::ConvertFromScreen(BPoint pt) const
{
	ConvertFromScreen(&pt);

	return pt;
}


void
BView::ConvertToScreen(BRect *rect) const
{
	if (!parent)
		return;

	do_owner_check_no_pick();

	ConvertToParent(rect);
	parent->ConvertToScreen(rect);
}


BRect
BView::ConvertToScreen(BRect rect) const
{
	ConvertToScreen(&rect);

	return rect;
}


void
BView::ConvertFromScreen(BRect *rect) const
{
	if (!parent)
		return;

	do_owner_check_no_pick();

	ConvertFromParent(rect);
	parent->ConvertFromScreen(rect);
}


BRect
BView::ConvertFromScreen(BRect rect) const
{
	ConvertFromScreen(&rect);

	return rect;
}


uint32
BView::Flags() const 
{
	check_lock_no_pick();
	return fFlags & ~_RESIZE_MASK_;
}


void
BView::SetFlags(uint32 flags)
{
	if (Flags() == flags)
		return;

	if (owner) {
		if (flags & B_PULSE_NEEDED) {
			check_lock_no_pick();
			if (!owner->fPulseEnabled)
				owner->SetPulseRate(500000);
		}

		if (flags & (B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE
					| B_FRAME_EVENTS | B_SUBPIXEL_PRECISE)) {
			check_lock();

			owner->fLink->StartMessage(AS_LAYER_SET_FLAGS);
			owner->fLink->Attach<uint32>(flags);
		}
	}

	/* Some useful info:
		fFlags is a unsigned long (32 bits)
		* bits 1-16 are used for BView's flags
		* bits 17-32 are used for BView' resize mask
		* _RESIZE_MASK_ is used for that. Look into View.h to see how
			it's defined
	*/
	fFlags = (flags & ~_RESIZE_MASK_) | (fFlags & _RESIZE_MASK_);

	fState->archiving_flags |= B_VIEW_FLAGS_BIT;
}


BRect
BView::Frame() const 
{
	check_lock_no_pick();

	return Bounds().OffsetToCopy(fParentOffset.x, fParentOffset.y);
}


void
BView::Hide()
{
	if (owner && fShowLevel == 0) {
		check_lock();
		owner->fLink->StartMessage(AS_LAYER_HIDE);
	}
	fShowLevel++;
}


void
BView::Show()
{
	fShowLevel--;
	if (owner && fShowLevel == 0) {
		check_lock();
		owner->fLink->StartMessage(AS_LAYER_SHOW);
	}
}


bool
BView::IsFocus() const 
{
	if (owner) {
		check_lock_no_pick();
		return owner->CurrentFocus() == this;
	} else
		return false;
}


bool 
BView::IsHidden(const BView *lookingFrom) const
{
	if (fShowLevel > 0)
		return true;

	// may we be egocentric?
	if (lookingFrom == this)
		return false;

	// we have the same visibility state as our
	// parent, if there is one
	if (parent)
		return parent->IsHidden(lookingFrom);

	// if we're the top view, and we're interested
	// in the "global" view, we're inheriting the
	// state of the window's visibility
	if (owner && lookingFrom == NULL)
		return owner->IsHidden();

	return false;
}


bool
BView::IsHidden() const
{
	return IsHidden(NULL);
}


bool
BView::IsPrinting() const 
{
	return f_is_printing;
}


BPoint
BView::LeftTop() const 
{
	return Bounds().LeftTop();
}


void
BView::SetOrigin(BPoint pt) 
{
	SetOrigin(pt.x, pt.y);
}


void
BView::SetOrigin(float x, float y)
{
	if (fState->IsValid(B_VIEW_ORIGIN_BIT)
		&& x == fState->origin.x && y == fState->origin.y)
		return;

	if (do_owner_check()) {
		owner->fLink->StartMessage(AS_LAYER_SET_ORIGIN);
		owner->fLink->Attach<float>(x);
		owner->fLink->Attach<float>(y);

		fState->valid_flags |= B_VIEW_ORIGIN_BIT;
	}

// TODO: Bounds() is effected by SetOrigin() (?),
// so should we set the COORD_BIT too?

	// our local coord system origin has changed, so when archiving we'll add this too
	fState->archiving_flags |= B_VIEW_ORIGIN_BIT;
}


BPoint
BView::Origin() const
{
	if (!fState->IsValid(B_VIEW_ORIGIN_BIT)) {
		do_owner_check();

		owner->fLink->StartMessage(AS_LAYER_GET_ORIGIN);

		int32 code;
		if (owner->fLink->FlushWithReply(code) == B_OK
			&& code == SERVER_TRUE) {
			owner->fLink->Read<BPoint>(&fState->origin);

			fState->valid_flags |= B_VIEW_ORIGIN_BIT;
		}
	}

	return fState->origin;
}


void
BView::SetResizingMode(uint32 mode) 
{
	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_RESIZE_MODE);
		owner->fLink->Attach<uint32>(mode);
	}

	// look at SetFlags() for more info on the below line
	fFlags = (fFlags & ~_RESIZE_MASK_) | (mode & _RESIZE_MASK_);

	// our resize mode has changed, so when archiving we'll add this too
	fState->archiving_flags |= B_VIEW_RESIZE_BIT;
}


uint32
BView::ResizingMode() const
{
	return fFlags & _RESIZE_MASK_;
}


void
BView::SetViewCursor(const BCursor *cursor, bool sync)
{
	if (!cursor)
		return;

	if (!owner)
		debugger("View method requires owner and doesn't have one");

	check_lock();

	owner->fLink->StartMessage(AS_LAYER_CURSOR);
	owner->fLink->Attach<int32>(cursor->m_serverToken);

	if (sync)
		owner->fLink->Flush();
}


void
BView::Flush(void) const 
{
	if (owner)
		owner->Flush();
}


void
BView::Sync(void) const 
{
	do_owner_check_no_pick();
	if (owner)
		owner->Sync();
}


BWindow *
BView::Window() const 
{
	return owner;
}


//	#pragma mark -
// Hook Functions


void
BView::AttachedToWindow()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::AttachedToWindow()\n", Name()));
}


void
BView::AllAttached()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::AllAttached()\n", Name()));
}


void
BView::DetachedFromWindow()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::DetachedFromWindow()\n", Name()));
}


void
BView::AllDetached()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::AllDetached()\n", Name()));
}


void
BView::Draw(BRect updateRect)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::Draw()\n", Name()));
}


void
BView::DrawAfterChildren(BRect r)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::DrawAfterChildren()\n", Name()));
}


void
BView::FrameMoved(BPoint newPosition)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::FrameMoved()\n", Name()));
}


void
BView::FrameResized(float newWidth, float newHeight)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::FrameResized()\n", Name()));
}


void
BView::GetPreferredSize(float* _width, float* _height)
{
	STRACE(("\tHOOK: BView(%s)::GetPreferredSize()\n", Name()));

	*_width = fBounds.Width();
	*_height = fBounds.Height();
}


void
BView::ResizeToPreferred()
{
	STRACE(("\tHOOK: BView(%s)::ResizeToPreferred()\n", Name()));

	float width;
	float height;
	GetPreferredSize(&width, &height);

	ResizeTo(width, height); 
}


void
BView::KeyDown(const char* bytes, int32 numBytes)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::KeyDown()\n", Name()));
	if (numBytes > 0 && bytes[0] == B_TAB) {
		// TODO: handle tab navigation here instead of in BWindow
	}
}


void
BView::KeyUp(const char* bytes, int32 numBytes)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::KeyUp()\n", Name()));
}


void
BView::MouseDown(BPoint where)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::MouseDown()\n", Name()));
}


void
BView::MouseUp(BPoint where)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::MouseUp()\n", Name()));
}


void
BView::MouseMoved(BPoint where, uint32 code, const BMessage* a_message)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::MouseMoved()\n", Name()));
}


void
BView::Pulse()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::Pulse()\n", Name()));
}


void
BView::TargetedByScrollView(BScrollView* scroll_view)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::TargetedByScrollView()\n", Name()));
}


void
BView::WindowActivated(bool state)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::WindowActivated()\n", Name()));
}


//	#pragma mark -
//	Input Functions


void
BView::BeginRectTracking(BRect startRect, uint32 style)
{
	if (do_owner_check()) {
		owner->fLink->StartMessage(AS_LAYER_BEGIN_RECT_TRACK);
		owner->fLink->Attach<BRect>(startRect);
		owner->fLink->Attach<int32>(style);
	}
}


void
BView::EndRectTracking()
{
	if (do_owner_check())
		owner->fLink->StartMessage(AS_LAYER_END_RECT_TRACK);
}


void
BView::DragMessage(BMessage *message, BRect dragRect, BHandler *replyTo)
{
	if (!message || !dragRect.IsValid())
		return;

	if (replyTo == NULL)
		replyTo = this;
	
	if (replyTo->Looper() == NULL)
		debugger("DragMessage: warning - the Handler needs a looper");

	do_owner_check_no_pick();

	BPoint offset;

	if (!message->HasInt32("buttons")) {
		BMessage *msg = owner->CurrentMessage();
		uint32 buttons;

		if (msg) {
			if (msg->FindInt32("buttons", (int32 *)&buttons) != B_OK) {
				BPoint point;
				GetMouse(&point, &buttons, false);
			}

			BPoint point;
			if (msg->FindPoint("be:view_where", &point) == B_OK)
				offset = point - dragRect.LeftTop();
		} else {
			BPoint point;
			GetMouse(&point, &buttons, false);
		}

		message->AddInt32("buttons", buttons);		
	}

	_set_message_reply_(message, BMessenger(replyTo, replyTo->Looper()));

	int32 bufferSize = message->FlattenedSize();
	char *buffer = new char[bufferSize];
	message->Flatten(buffer, bufferSize);

	owner->fLink->StartMessage(AS_LAYER_DRAG_RECT);
	owner->fLink->Attach<BRect>(dragRect);
	owner->fLink->Attach<BPoint>(offset);	
	owner->fLink->Attach<int32>(bufferSize);
	owner->fLink->Attach(buffer, bufferSize);
	owner->fLink->Flush();

	delete [] buffer;
}


void
BView::DragMessage(BMessage *message, BBitmap *image, BPoint offset,
	BHandler *replyTo)
{
	DragMessage(message, image, B_OP_COPY, offset, replyTo);
}


void
BView::DragMessage(BMessage *message, BBitmap *image,
	drawing_mode dragMode, BPoint offset, BHandler *replyTo)
{
	// ToDo: is this correct? Isn't \a image allowed to be NULL?
	if (message == NULL || image == NULL)
		return;

	if (replyTo == NULL)
		replyTo = this;

	if (replyTo->Looper() == NULL)
		debugger("DragMessage: warning - the Handler needs a looper");

	do_owner_check_no_pick();

	if (!message->HasInt32("buttons")) {
		BMessage *msg = owner->CurrentMessage();
		uint32 buttons;

		if (msg == NULL || msg->FindInt32("buttons", (int32 *)&buttons) != B_OK) {
			BPoint point;
			GetMouse(&point, &buttons, false);
		}

		message->AddInt32("buttons", buttons);		
	}

	_set_message_reply_(message, BMessenger(replyTo, replyTo->Looper()));

	int32 bufferSize = message->FlattenedSize();
	char *buffer = new char[bufferSize];
	message->Flatten(buffer, bufferSize);

	owner->fLink->StartMessage(AS_LAYER_DRAG_IMAGE);
	owner->fLink->Attach<int32>(image->get_server_token());
	owner->fLink->Attach<int32>((int32)dragMode);
	owner->fLink->Attach<BPoint>(offset);
	owner->fLink->Attach<int32>(bufferSize);
	owner->fLink->Attach(buffer, bufferSize);

	delete [] buffer;

	// TODO: in app_server the bitmap refCount must be incremented
	// WRITE this into specs!!!!

	delete image;
}


void
BView::GetMouse(BPoint *location, uint32 *buttons, bool checkMessageQueue)
{
	do_owner_check();

	if (checkMessageQueue) {
		BMessageQueue *queue = Window()->MessageQueue();
		queue->Lock();

		Window()->UpdateIfNeeded();

		// Look out for mouse update messages

		BMessage *msg;
		for (int32 i = 0; (msg = queue->FindMessage(i)) != NULL; i++) {
			switch (msg->what) {
				case B_MOUSE_UP:
				case B_MOUSE_DOWN:
				case B_MOUSE_MOVED:
				{
					msg->FindPoint("where", location);
					msg->FindInt32("buttons", (int32 *)buttons);

					// ToDo: the "where" coordinates are screen coordinates
					//	unlike R5 - this might break applications!
					ConvertFromScreen(location);

					queue->RemoveMessage(msg);
					delete msg;

					queue->Unlock();
					return;
				}
			}
		}
		queue->Unlock();
	}

	// If no mouse update message has been found in the message queue, 
	// we get the current mouse location and buttons from the app_server

	owner->fLink->StartMessage(AS_LAYER_GET_MOUSE_COORDS);
	
	int32 code;
	if (owner->fLink->FlushWithReply(code) == B_OK
		&& code == SERVER_TRUE) {
		owner->fLink->Read<BPoint>(location);
		owner->fLink->Read<uint32>(buttons);

		// TODO: See above comment about coordinates
		ConvertFromScreen(location);
	} else
		buttons = 0;
}


void
BView::MakeFocus(bool focusState)
{
	if (owner) {
		// TODO: If this view has focus and focusState==false,
		// will there really be no other view with focus? No
		// cycling to the next one?
		BView *focus = owner->CurrentFocus();
		if (focusState) {
			// Unfocus a previous focus view
			if (focus && focus != this)
				focus->MakeFocus(false);
			// if we want to make this view the current focus view
			owner->fFocus = this;
			owner->SetPreferredHandler(this);
		} else {
			// we want to unfocus this view, but only if it actually has focus
			if (focus == this) {
				owner->fFocus = NULL;
				owner->SetPreferredHandler(NULL);
			}
		}
	}
}


BScrollBar *
BView::ScrollBar(orientation posture) const
{
	switch (posture) {
		case B_VERTICAL:
			return fVerScroller;

		case B_HORIZONTAL:
			return fHorScroller;

		default:
			return NULL;
	}
}


void
BView::ScrollBy(float dh, float dv)
{
	// no reason to process this further if no scroll is intended.
	if (dh == 0 && dv == 0)
		return;

	check_lock();

	// if we're attached to a window tell app_server about this change
	if (owner) {
		owner->fLink->StartMessage(AS_LAYER_SCROLL);
		owner->fLink->Attach<float>(dh);
		owner->fLink->Attach<float>(dv);

		owner->fLink->Flush();

		fState->valid_flags &= ~(B_VIEW_COORD_BIT | B_VIEW_ORIGIN_BIT);
	}

	// we modify our bounds rectangle by dh/dv coord units hor/ver.
	fBounds.OffsetBy(dh, dv);

	// then set the new values of the scrollbars
	if (fHorScroller)
		fHorScroller->SetValue(fBounds.top);
	if (fVerScroller)		
		fVerScroller->SetValue(fBounds.left);
}


void
BView::ScrollTo(BPoint where)
{
	ScrollBy(where.x - fBounds.left, where.y - fBounds.top);
}


status_t
BView::SetEventMask(uint32 mask, uint32 options)
{
	if (fEventMask == mask && fEventOptions == options)
		return B_ERROR;

	fEventMask = mask | (fEventMask & 0xFFFF0000);
	fEventOptions = options;

	fState->archiving_flags |= B_VIEW_EVENT_MASK_BIT;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_SET_EVENT_MASK);
		owner->fLink->Attach<uint32>(mask);
		owner->fLink->Attach<uint32>(options);
	}

	return B_OK;
}


uint32
BView::EventMask()
{
	return fEventMask;
}


status_t
BView::SetMouseEventMask(uint32 mask, uint32 options)
{
//	fEventMask = (mask << 16) | (fEventMask & 0x0000FFFF);
//	fEventOptions = (options << 16) | (options & 0x0000FFFF);
	
	if (do_owner_check()
		&& owner->CurrentMessage()
		&& owner->CurrentMessage()->what == B_MOUSE_DOWN) {
		owner->fLink->StartMessage(AS_LAYER_SET_MOUSE_EVENT_MASK);
		owner->fLink->Attach<uint32>(mask);
		owner->fLink->Attach<uint32>(options);

		return B_OK;
	}
	return B_ERROR;
}


//	#pragma mark -
//	Graphic State Functions


void
BView::SetLineMode(cap_mode lineCap, join_mode lineJoin, float miterLimit)
{
	if (fState->IsValid(B_VIEW_LINE_MODES_BIT)
		&& lineCap == fState->line_cap && lineJoin == fState->line_join
		&& miterLimit == fState->miter_limit)
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_SET_LINE_MODE);
		owner->fLink->Attach<int8>((int8)lineCap);
		owner->fLink->Attach<int8>((int8)lineJoin);
		owner->fLink->Attach<float>(miterLimit);

		fState->valid_flags |= B_VIEW_LINE_MODES_BIT;
	}

	fState->line_cap = lineCap;
	fState->line_join = lineJoin;
	fState->miter_limit = miterLimit;

	fState->archiving_flags |= B_VIEW_LINE_MODES_BIT;
}	


join_mode
BView::LineJoinMode() const
{
	// This will update the current state, if necessary
	if (!fState->IsValid(B_VIEW_LINE_MODES_BIT))
		LineMiterLimit();

	return fState->line_join;
}


cap_mode
BView::LineCapMode() const
{
	// This will update the current state, if necessary
	if (!fState->IsValid(B_VIEW_LINE_MODES_BIT))
		LineMiterLimit();

	return fState->line_cap;
}


float
BView::LineMiterLimit() const
{
	if (!fState->IsValid(B_VIEW_LINE_MODES_BIT) && owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_GET_LINE_MODE);

		int32 code;
		if (owner->fLink->FlushWithReply(code) == B_OK
			&& code == SERVER_TRUE) {
			int8 cap, join;
			owner->fLink->Read<int8>((int8 *)&cap);
			owner->fLink->Read<int8>((int8 *)&join);
			owner->fLink->Read<float>(&fState->miter_limit);

			fState->line_cap = (cap_mode)cap;
			fState->line_join = (join_mode)join;
		}

		fState->valid_flags |= B_VIEW_LINE_MODES_BIT;
	}

	return fState->miter_limit;
}


void
BView::PushState()
{
	do_owner_check();

	owner->fLink->StartMessage(AS_LAYER_PUSH_STATE);

	// initialize origin and scale
	fState->valid_flags |= B_VIEW_SCALE_BIT | B_VIEW_ORIGIN_BIT;
	fState->scale = 1.0f;
	fState->origin.Set(0, 0);
}


void
BView::PopState()
{
	do_owner_check();

	owner->fLink->StartMessage(AS_LAYER_POP_STATE);

	// invalidate all flags (except those that are not part of pop/push)
	fState->valid_flags = B_VIEW_VIEW_COLOR_BIT;
}


void
BView::SetScale(float scale) const
{
	if (fState->IsValid(B_VIEW_SCALE_BIT) && scale == fState->scale)
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_SET_SCALE);
		owner->fLink->Attach<float>(scale);

		fState->valid_flags |= B_VIEW_SCALE_BIT;
	}

	fState->scale = scale;
	fState->archiving_flags |= B_VIEW_SCALE_BIT;
}


float
BView::Scale() const
{
	if (!fState->IsValid(B_VIEW_SCALE_BIT) && owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_GET_SCALE);

 		int32 code;
		if (owner->fLink->FlushWithReply(code) == B_OK
			&& code == SERVER_TRUE)
			owner->fLink->Read<float>(&fState->scale);

		fState->valid_flags |= B_VIEW_SCALE_BIT;
	}

	return fState->scale;
}


void
BView::SetDrawingMode(drawing_mode mode)
{
	if (fState->IsValid(B_VIEW_DRAWING_MODE_BIT)
		&& mode == fState->drawing_mode)
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_SET_DRAW_MODE);
		owner->fLink->Attach<int8>((int8)mode);

		fState->valid_flags |= B_VIEW_DRAWING_MODE_BIT;
	}

	fState->drawing_mode = mode;
	fState->archiving_flags |= B_VIEW_DRAWING_MODE_BIT;
}


drawing_mode
BView::DrawingMode() const
{
	if (!fState->IsValid(B_VIEW_DRAWING_MODE_BIT) && owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_GET_DRAW_MODE);

		int32 code;
		if (owner->fLink->FlushWithReply(code) == B_OK
			&& code == SERVER_TRUE) {
			int8 drawingMode;
			owner->fLink->Read<int8>(&drawingMode);

			fState->drawing_mode = (drawing_mode)drawingMode;
			fState->valid_flags |= B_VIEW_DRAWING_MODE_BIT;
		}
	}

	return fState->drawing_mode;
}


void
BView::SetBlendingMode(source_alpha sourceAlpha, alpha_function alphaFunction)
{
	if (fState->IsValid(B_VIEW_BLENDING_BIT)
		&& sourceAlpha == fState->alpha_source_mode
		&& alphaFunction == fState->alpha_function_mode)
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_SET_BLEND_MODE);
		owner->fLink->Attach<int8>((int8)sourceAlpha);
		owner->fLink->Attach<int8>((int8)alphaFunction);

		fState->valid_flags |= B_VIEW_BLENDING_BIT;
	}

	fState->alpha_source_mode = sourceAlpha;
	fState->alpha_function_mode = alphaFunction;

	fState->archiving_flags |= B_VIEW_BLENDING_BIT;
}


void
BView::GetBlendingMode(source_alpha *_sourceAlpha,
	alpha_function *_alphaFunction) const
{
	if (!fState->IsValid(B_VIEW_BLENDING_BIT) && owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_GET_BLEND_MODE);

		int32 code;
 		if (owner->fLink->FlushWithReply(code) == B_OK
 			&& code == SERVER_TRUE) {
			int8 alphaSourceMode, alphaFunctionMode;
			owner->fLink->Read<int8>(&alphaSourceMode);
			owner->fLink->Read<int8>(&alphaFunctionMode);

			fState->alpha_source_mode = (source_alpha)alphaSourceMode;
			fState->alpha_function_mode = (alpha_function)alphaFunctionMode;

			fState->valid_flags |= B_VIEW_BLENDING_BIT;
		}
	}

	if (_sourceAlpha)
		*_sourceAlpha = fState->alpha_source_mode;

	if (_alphaFunction)
		*_alphaFunction = fState->alpha_function_mode;
}


void
BView::MovePenTo(BPoint point)
{
	MovePenTo(point.x, point.y);
}


void
BView::MovePenTo(float x, float y)
{
	if (fState->IsValid(B_VIEW_PEN_LOCATION_BIT)
		&& x == fState->pen_location.x && y == fState->pen_location.y)
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_SET_PEN_LOC);
		owner->fLink->Attach<float>(x);
		owner->fLink->Attach<float>(y);

		fState->valid_flags |= B_VIEW_PEN_LOCATION_BIT;
	}

	fState->pen_location.x = x;
	fState->pen_location.y = y;	

	fState->archiving_flags |= B_VIEW_PEN_LOCATION_BIT;
}


void
BView::MovePenBy(float x, float y)
{
	// this will update the pen location if necessary
	if (!fState->IsValid(B_VIEW_PEN_LOCATION_BIT))
		PenLocation();

	MovePenTo(fState->pen_location.x + x, fState->pen_location.y + y);
}


BPoint
BView::PenLocation() const
{
	if (!fState->IsValid(B_VIEW_PEN_LOCATION_BIT) && owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_GET_PEN_LOC);

		int32 code;
		if (owner->fLink->FlushWithReply(code) == B_OK
			&& code == SERVER_TRUE) {
			owner->fLink->Read<BPoint>(&fState->pen_location);

			fState->valid_flags |= B_VIEW_PEN_LOCATION_BIT;
		}
	}

	return fState->pen_location;
}


void
BView::SetPenSize(float size)
{
	if (fState->IsValid(B_VIEW_PEN_SIZE_BIT) && size == fState->pen_size)
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_SET_PEN_SIZE);
		owner->fLink->Attach<float>(size);

		fState->valid_flags |= B_VIEW_PEN_SIZE_BIT;	
	}

	fState->pen_size = size;
	fState->archiving_flags	|= B_VIEW_PEN_SIZE_BIT;
}


float
BView::PenSize() const
{
	if (!fState->IsValid(B_VIEW_PEN_SIZE_BIT) && owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_GET_PEN_SIZE);

		int32 code;
		if (owner->fLink->FlushWithReply(code) == B_OK
			&& code == SERVER_TRUE) {
			owner->fLink->Read<float>(&fState->pen_size);

			fState->valid_flags |= B_VIEW_PEN_SIZE_BIT;
		}
	}

	return fState->pen_size;
}


void
BView::SetHighColor(rgb_color color)
{
	// are we up-to-date already?
	if (fState->IsValid(B_VIEW_HIGH_COLOR_BIT)
		&& fState->high_color == color)
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_SET_HIGH_COLOR);
		owner->fLink->Attach<rgb_color>(color);

		fState->valid_flags |= B_VIEW_HIGH_COLOR_BIT;	
	}

	set_rgb_color(fState->high_color, color.red, color.green,
		color.blue, color.alpha);

	fState->archiving_flags |= B_VIEW_HIGH_COLOR_BIT;
}


rgb_color
BView::HighColor() const
{
	if (!fState->IsValid(B_VIEW_HIGH_COLOR_BIT) && owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_GET_HIGH_COLOR);

		int32 code;
		if (owner->fLink->FlushWithReply(code) == B_OK
			&& code == SERVER_TRUE) {
			owner->fLink->Read<rgb_color>(&fState->high_color);

			fState->valid_flags |= B_VIEW_HIGH_COLOR_BIT;
		}
	}

	return fState->high_color;
}


void
BView::SetLowColor(rgb_color color)
{
	if (fState->IsValid(B_VIEW_LOW_COLOR_BIT)
		&& fState->low_color == color)
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_SET_LOW_COLOR);
		owner->fLink->Attach<rgb_color>(color);

		fState->valid_flags |= B_VIEW_LOW_COLOR_BIT;
	}

	set_rgb_color(fState->low_color, color.red, color.green,
		color.blue, color.alpha);

	fState->archiving_flags |= B_VIEW_LOW_COLOR_BIT;
}


rgb_color
BView::LowColor() const
{
	if (!fState->IsValid(B_VIEW_LOW_COLOR_BIT) && owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_GET_LOW_COLOR);

		int32 code;
		if (owner->fLink->FlushWithReply(code) == B_OK
			&& code == SERVER_TRUE) {
			owner->fLink->Read<rgb_color>(&fState->low_color);

			fState->valid_flags |= B_VIEW_LOW_COLOR_BIT;
		}
	}

	return fState->low_color;
}


void
BView::SetViewColor(rgb_color color)
{
	if (fState->IsValid(B_VIEW_VIEW_COLOR_BIT) && fState->view_color == color)
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_SET_VIEW_COLOR);
		owner->fLink->Attach<rgb_color>(color);

		fState->valid_flags |= B_VIEW_VIEW_COLOR_BIT;
	}

	set_rgb_color(fState->view_color, color.red, color.green,
		color.blue, color.alpha);

	fState->archiving_flags |= B_VIEW_VIEW_COLOR_BIT;
}


rgb_color
BView::ViewColor() const
{
	if (!fState->IsValid(B_VIEW_VIEW_COLOR_BIT) && owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_GET_VIEW_COLOR);

		int32 code;
		if (owner->fLink->FlushWithReply(code) == B_OK
			&& code == SERVER_TRUE) {
			owner->fLink->Read<rgb_color>(&fState->view_color);

			fState->valid_flags |= B_VIEW_VIEW_COLOR_BIT;
		}
	}

	return fState->view_color;
}


void
BView::ForceFontAliasing(bool enable)
{
	if (fState->IsValid(B_VIEW_FONT_ALIASING_BIT) && enable == fState->font_aliasing)
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_PRINT_ALIASING);
		owner->fLink->Attach<bool>(enable);

		fState->valid_flags |= B_VIEW_FONT_ALIASING_BIT;
	}

	fState->font_aliasing = enable;
	fState->archiving_flags |= B_VIEW_FONT_ALIASING_BIT;
}


void
BView::SetFont(const BFont* font, uint32 mask)
{
	if (!font || mask == 0)
		return;

	if (mask == B_FONT_ALL) {
		fState->font = *font;
	} else {
		if (mask & B_FONT_FAMILY_AND_STYLE)
			fState->font.SetFamilyAndStyle(font->FamilyAndStyle());

		if (mask & B_FONT_SIZE)
			fState->font.SetSize(font->Size());

		if (mask & B_FONT_SHEAR)
			fState->font.SetShear(font->Shear());

		if (mask & B_FONT_ROTATION)
			fState->font.SetRotation(font->Rotation());		

		if (mask & B_FONT_SPACING)
			fState->font.SetSpacing(font->Spacing());

		if (mask & B_FONT_ENCODING)
			fState->font.SetEncoding(font->Encoding());		

		if (mask & B_FONT_FACE)
			fState->font.SetFace(font->Face());

		if (mask & B_FONT_FLAGS)
			fState->font.SetFlags(font->Flags());
	}

	fState->font_flags = mask;

	if (owner) {
		check_lock();
		do_owner_check();

		fState->UpdateServerFontState(*owner->fLink);
	}
}


#if !_PR3_COMPATIBLE_
void
BView::GetFont(BFont *font) const
#else
void
BView:GetFont(BFont *font)
#endif
{
	*font = fState->font;
}


void
BView::GetFontHeight(font_height *height) const
{
	fState->font.GetHeight(height);
}


void
BView::SetFontSize(float size)
{
	fState->font.SetSize(size);
}


float
BView::StringWidth(const char *string) const
{
	return fState->font.StringWidth(string);
}


float
BView::StringWidth(const char* string, int32 length) const
{
	return fState->font.StringWidth(string, length);
}


void
BView::GetStringWidths(char *stringArray[],int32 lengthArray[],
	int32 numStrings, float widthArray[]) const
{
	fState->font.GetStringWidths(const_cast<const char **>(stringArray),
		const_cast<const int32 *>(lengthArray), numStrings, widthArray);
}


void
BView::TruncateString(BString *in_out, uint32 mode, float width) const
{
	fState->font.TruncateString(in_out, mode, width);
}


void
BView::ClipToPicture(BPicture *picture, BPoint where, bool sync)
{
	DoPictureClip(picture, where, false, sync);
}


void
BView::ClipToInversePicture(BPicture *picture,
	BPoint where, bool sync)
{
	DoPictureClip(picture, where, true, sync);
}


void
BView::GetClippingRegion(BRegion* region) const
{
	if (!region)
		return;

	if (!fState->IsValid(B_VIEW_CLIP_REGION_BIT) && do_owner_check()) {
		owner->fLink->StartMessage(AS_LAYER_GET_CLIP_REGION);

 		int32 code;
 		if (owner->fLink->FlushWithReply(code) == B_OK
 			&& code == SERVER_TRUE) {
			int32 count;
			owner->fLink->Read<int32>(&count);

			fState->clipping_region.MakeEmpty();

			for (int32 i = 0; i < count; i++) {
				BRect rect;
				owner->fLink->Read<BRect>(&rect);

				fState->clipping_region.Include(rect);
			}
			fState->valid_flags |= B_VIEW_CLIP_REGION_BIT;
		}
	}

	*region = fState->clipping_region;
}


void
BView::ConstrainClippingRegion(BRegion* region)
{
	if (do_owner_check()) {
		int32 count = 0;
		if (region)
			count = region->CountRects();

		owner->fLink->StartMessage(AS_LAYER_SET_CLIP_REGION);

		// '0' means that in the app_server, there won't be any 'local'
		// clipping region (it will be = NULL)

		// TODO: note this in the specs
		owner->fLink->Attach<int32>(count);

		for (int32 i = 0; i < count; i++)
			owner->fLink->Attach<BRect>(region->RectAt(i));

		// we flush here because app_server waits for all the rects
		owner->fLink->Flush();

		fState->valid_flags &= ~B_VIEW_CLIP_REGION_BIT;
		fState->archiving_flags |= B_VIEW_CLIP_REGION_BIT;
	}
}


//	#pragma mark - Drawing Functions
//---------------------------------------------------------------------------


void
BView::DrawBitmapAsync(const BBitmap *bitmap, BRect srcRect, BRect dstRect)
{
	if (!bitmap || !srcRect.IsValid() || !dstRect.IsValid())
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_DRAW_BITMAP_ASYNC_IN_RECT);
		owner->fLink->Attach<int32>(bitmap->get_server_token());
		owner->fLink->Attach<BRect>(srcRect);
		owner->fLink->Attach<BRect>(dstRect);
	}
}


void
BView::DrawBitmapAsync(const BBitmap *bitmap, BRect dstRect)
{
	if (!bitmap || !dstRect.IsValid())
		return;

	DrawBitmapAsync(bitmap, bitmap->Bounds(), dstRect);
}


void
BView::DrawBitmapAsync(const BBitmap *bitmap)
{
	DrawBitmapAsync(bitmap, PenLocation());
}


void
BView::DrawBitmapAsync(const BBitmap *bitmap, BPoint where)
{
	if (bitmap == NULL)
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_DRAW_BITMAP_ASYNC_AT_POINT);
		owner->fLink->Attach<int32>(bitmap->get_server_token());
		owner->fLink->Attach<BPoint>(where);
	}
}


void
BView::DrawBitmap(const BBitmap *bitmap)
{
	DrawBitmap(bitmap, PenLocation());
}


void
BView::DrawBitmap(const BBitmap *bitmap, BPoint where)
{
	if (bitmap == NULL)
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_DRAW_BITMAP_SYNC_AT_POINT);
		owner->fLink->Attach<int32>(bitmap->get_server_token());
		owner->fLink->Attach<BPoint>(where);
		owner->fLink->Flush();
	}
}


void
BView::DrawBitmap(const BBitmap *bitmap, BRect dstRect)
{
	if (!bitmap || !dstRect.IsValid())
		return;

	DrawBitmap(bitmap, bitmap->Bounds(), dstRect);
}


void
BView::DrawBitmap(const BBitmap *bitmap, BRect srcRect, BRect dstRect)
{
	if ( !bitmap || !srcRect.IsValid() || !dstRect.IsValid())
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_DRAW_BITMAP_SYNC_IN_RECT);
		owner->fLink->Attach<int32>(bitmap->get_server_token());
		owner->fLink->Attach<BRect>(dstRect);
		owner->fLink->Attach<BRect>(srcRect);
		owner->fLink->Flush();
	}
}


void
BView::DrawChar(char c)
{
	DrawString(&c, 1, PenLocation());
}


void
BView::DrawChar(char c, BPoint location)
{
	DrawString(&c, 1, location);
}


void
BView::DrawString(const char *string, escapement_delta *delta)
{
	DrawString(string, strlen(string), PenLocation());
}


void
BView::DrawString(const char *string, BPoint location, escapement_delta *delta)
{
	DrawString(string, strlen(string), location);
}


void
BView::DrawString(const char *string, int32 length, escapement_delta *delta)
{
	DrawString(string, length, PenLocation());
}


void
BView::DrawString(const char *string, int32 length, BPoint location,
	escapement_delta *delta)
{
	if (string == NULL || length < 1)
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_DRAW_STRING);
		owner->fLink->Attach<int32>(length);
		owner->fLink->Attach<BPoint>(location);

		// Quite often delta will be NULL, so we have to accomodate this.
		if (delta)
			owner->fLink->Attach<escapement_delta>(*delta);
		else {
			escapement_delta tdelta;
			tdelta.space = 0;
			tdelta.nonspace = 0;

			owner->fLink->Attach<escapement_delta>(tdelta);
		}

		owner->fLink->AttachString(string, length);

		// this modifies our pen location, so we invalidate the flag.
		fState->valid_flags &= ~B_VIEW_PEN_LOCATION_BIT;
	}
}


void
BView::StrokeEllipse(BPoint center, float xRadius, float yRadius,
	pattern p)
{
	StrokeEllipse(BRect(center.x - xRadius, center.y - yRadius, center.x + xRadius,
		center.y + yRadius), p);
}


void
BView::StrokeEllipse(BRect rect, ::pattern pattern)
{
	if (owner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	owner->fLink->StartMessage(AS_STROKE_ELLIPSE);
	owner->fLink->Attach<BRect>(rect);
}


void
BView::FillEllipse(BPoint center, float xRadius, float yRadius,
	::pattern pattern)
{
	FillEllipse(BRect(center.x - xRadius, center.y - yRadius,
		center.x + xRadius, center.y + yRadius), pattern);
}


void
BView::FillEllipse(BRect rect, ::pattern pattern) 
{
	if (owner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	owner->fLink->StartMessage(AS_FILL_ELLIPSE);
	owner->fLink->Attach<BRect>(rect);
}


void
BView::StrokeArc(BPoint center, float xRadius, float yRadius,
	float startAngle, float arcAngle, pattern p)
{
	StrokeArc(BRect(center.x - xRadius, center.y - yRadius, center.x + xRadius,
		center.y + yRadius), startAngle, arcAngle, p);
}


void
BView::StrokeArc(BRect rect, float startAngle, float arcAngle,
	::pattern pattern)
{
	if (owner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	owner->fLink->StartMessage(AS_STROKE_ARC);
	owner->fLink->Attach<BRect>(rect);
	owner->fLink->Attach<float>(startAngle);
	owner->fLink->Attach<float>(arcAngle);
}


void
BView::FillArc(BPoint center,float xRadius, float yRadius,
	float startAngle, float arcAngle, ::pattern pattern)
{
	FillArc(BRect(center.x - xRadius, center.y - yRadius, center.x + xRadius,
		center.y + yRadius), startAngle, arcAngle, pattern);
}


void
BView::FillArc(BRect rect, float startAngle, float arcAngle,
	::pattern pattern)
{
	if (owner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	owner->fLink->StartMessage(AS_FILL_ARC);
	owner->fLink->Attach<BRect>(rect);
	owner->fLink->Attach<float>(startAngle);
	owner->fLink->Attach<float>(arcAngle);
}


void
BView::StrokeBezier(BPoint *controlPoints, ::pattern pattern)
{
	if (owner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	owner->fLink->StartMessage(AS_STROKE_BEZIER);
	owner->fLink->Attach<BPoint>(controlPoints[0]);
	owner->fLink->Attach<BPoint>(controlPoints[1]);
	owner->fLink->Attach<BPoint>(controlPoints[2]);
	owner->fLink->Attach<BPoint>(controlPoints[3]);
}


void
BView::FillBezier(BPoint *controlPoints, ::pattern pattern)
{
	if (owner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	owner->fLink->StartMessage(AS_FILL_BEZIER);
	owner->fLink->Attach<BPoint>(controlPoints[0]);
	owner->fLink->Attach<BPoint>(controlPoints[1]);
	owner->fLink->Attach<BPoint>(controlPoints[2]);
	owner->fLink->Attach<BPoint>(controlPoints[3]);
}


void
BView::StrokePolygon(const BPolygon *polygon, bool closed, pattern p)
{
	if (!polygon)
		return;

	StrokePolygon(polygon->fPts, polygon->fCount, polygon->Frame(), closed, p);
}


void
BView::StrokePolygon(const BPoint *ptArray, int32 numPoints, bool closed, pattern p)
{
	BPolygon polygon(ptArray, numPoints);
	
	StrokePolygon(polygon.fPts, polygon.fCount, polygon.Frame(), closed, p);
}


void
BView::StrokePolygon(const BPoint *ptArray, int32 numPoints, BRect bounds,
	bool closed, ::pattern pattern)
{
	if (!ptArray
		|| numPoints <= 2
		|| owner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	BPolygon polygon(ptArray, numPoints);
	polygon.MapTo(polygon.Frame(), bounds);

	if (owner->fLink->StartMessage(AS_STROKE_POLYGON,
			polygon.fCount * sizeof(BPoint) + sizeof(BRect) + sizeof(bool) + sizeof(int32))
				== B_OK) {
		owner->fLink->Attach<BRect>(polygon.Frame());
		owner->fLink->Attach<bool>(closed);
		owner->fLink->Attach<int32>(polygon.fCount);
		owner->fLink->Attach(polygon.fPts, polygon.fCount * sizeof(BPoint));
	} else {
		// TODO: send via an area
		fprintf(stderr, "ERROR: polygon to big for BPortLink!\n");
	}
}


void
BView::FillPolygon(const BPolygon *polygon, ::pattern pattern)
{
	if (polygon == NULL
		|| polygon->fCount <= 2
		|| owner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	if (owner->fLink->StartMessage(AS_FILL_POLYGON,
			polygon->fCount * sizeof(BPoint) + sizeof(int32)) == B_OK) {
		owner->fLink->Attach<int32>(polygon->fCount);
		owner->fLink->Attach(polygon->fPts, polygon->fCount * sizeof(BPoint));
	} else {
		// TODO: send via an area
		fprintf(stderr, "ERROR: polygon to big for BPortLink!\n");
	}
}


void
BView::FillPolygon(const BPoint *ptArray, int32 numPts, ::pattern pattern)
{
	if (!ptArray)
		return;

	BPolygon polygon(ptArray, numPts);
	FillPolygon(&polygon, pattern);
}


void
BView::FillPolygon(const BPoint *ptArray, int32 numPts, BRect bounds,
	pattern p)
{
	if (!ptArray)
		return;

	BPolygon polygon(ptArray, numPts);

	polygon.MapTo(polygon.Frame(), bounds);
	FillPolygon(&polygon, p);
}


void
BView::StrokeRect(BRect rect, ::pattern pattern)
{
	if (owner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	owner->fLink->StartMessage(AS_STROKE_RECT);
	owner->fLink->Attach<BRect>(rect);
}


void
BView::FillRect(BRect rect, ::pattern pattern)
{
	if (owner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	owner->fLink->StartMessage(AS_FILL_RECT);
	owner->fLink->Attach<BRect>(rect);
}


void
BView::StrokeRoundRect(BRect rect, float xRadius, float yRadius,
	::pattern pattern)
{
	if (owner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	owner->fLink->StartMessage(AS_STROKE_ROUNDRECT);
	owner->fLink->Attach<BRect>(rect);
	owner->fLink->Attach<float>(xRadius);
	owner->fLink->Attach<float>(yRadius);
}


void
BView::FillRoundRect(BRect rect, float xRadius, float yRadius,
	::pattern pattern)
{
	if (owner == NULL)
		return;

	check_lock();

	_UpdatePattern(pattern);

	owner->fLink->StartMessage(AS_FILL_ROUNDRECT);
	owner->fLink->Attach<BRect>(rect);
	owner->fLink->Attach<float>(xRadius);
	owner->fLink->Attach<float>(yRadius);
}


void
BView::FillRegion(BRegion *region, ::pattern pattern)
{
	if (region == NULL || owner == NULL)
		return;

	check_lock();

	_UpdatePattern(pattern);

	int32 count = region->CountRects();

	if (count * sizeof(BRect) < MAX_ATTACHMENT_SIZE) {
		owner->fLink->StartMessage(AS_FILL_REGION);
		owner->fLink->Attach<int32>(count);

		for (int32 i = 0; i < count; i++)
			owner->fLink->Attach<BRect>(region->RectAt(i));
	} else {
		// TODO: send via area
	}
}


void
BView::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
	BRect bounds, ::pattern pattern)
{
	if (owner == NULL)
		return;

	check_lock();

	_UpdatePattern(pattern);

	owner->fLink->StartMessage(AS_STROKE_TRIANGLE);
	owner->fLink->Attach<BPoint>(pt1);
	owner->fLink->Attach<BPoint>(pt2);
	owner->fLink->Attach<BPoint>(pt3);
	owner->fLink->Attach<BRect>(bounds);
}


void
BView::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3, pattern p)
{
	if (owner) {
		// we construct the smallest rectangle that contains the 3 points
		// for the 1st point
		BRect bounds(pt1, pt1);

		// for the 2nd point		
		if (pt2.x < bounds.left)
			bounds.left = pt2.x;

		if (pt2.y < bounds.top)
			bounds.top = pt2.y;

		if (pt2.x > bounds.right)
			bounds.right = pt2.x;

		if (pt2.y > bounds.bottom)
			bounds.bottom = pt2.y;

		// for the 3rd point
		if (pt3.x < bounds.left)
			bounds.left = pt3.x;

		if (pt3.y < bounds.top)
			bounds.top = pt3.y;

		if (pt3.x > bounds.right)
			bounds.right = pt3.x;

		if (pt3.y > bounds.bottom)
			bounds.bottom = pt3.y; 		

		StrokeTriangle(pt1, pt2, pt3, bounds, p);
	}
}


void
BView::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3, pattern p)
{
	if (owner) {
		// we construct the smallest rectangle that contains the 3 points
		// for the 1st point
		BRect bounds(pt1, pt1);

		// for the 2nd point		
		if (pt2.x < bounds.left)
			bounds.left = pt2.x;

		if (pt2.y < bounds.top)
			bounds.top = pt2.y;

		if (pt2.x > bounds.right)
			bounds.right = pt2.x;

		if (pt2.y > bounds.bottom)
			bounds.bottom = pt2.y;
			
		// for the 3rd point
		if (pt3.x < bounds.left)
			bounds.left = pt3.x;

		if (pt3.y < bounds.top)
			bounds.top = pt3.y;

		if (pt3.x > bounds.right)
			bounds.right = pt3.x;

		if (pt3.y > bounds.bottom)
			bounds.bottom = pt3.y; 		

		FillTriangle(pt1, pt2, pt3, bounds, p);
	}
}


void
BView::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
	BRect bounds, ::pattern pattern)
{
	if (owner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	owner->fLink->StartMessage(AS_FILL_TRIANGLE);
	owner->fLink->Attach<BPoint>(pt1);
	owner->fLink->Attach<BPoint>(pt2);
	owner->fLink->Attach<BPoint>(pt3);
	owner->fLink->Attach<BRect>(bounds);
}


void
BView::StrokeLine(BPoint toPt, pattern p)
{
	StrokeLine(PenLocation(), toPt, p);
}


void
BView::StrokeLine(BPoint pt0, BPoint pt1, ::pattern pattern)
{
	if (owner == NULL)
		return;

	check_lock();
	_UpdatePattern(pattern);

	owner->fLink->StartMessage(AS_STROKE_LINE);
	owner->fLink->Attach<BPoint>(pt0);
	owner->fLink->Attach<BPoint>(pt1);

	// this modifies our pen location, so we invalidate the flag.
	fState->valid_flags &= ~B_VIEW_PEN_LOCATION_BIT;
}


void
BView::StrokeShape(BShape *shape, ::pattern pattern)
{
	if (shape == NULL || owner == NULL)
		return;

	shape_data *sd = (shape_data *)shape->fPrivateData;
	if (sd->opCount == 0 || sd->ptCount == 0)
		return;

	check_lock();
	_UpdatePattern(pattern);

	if ((sd->opCount * sizeof(uint32)) + (sd->ptCount * sizeof(BPoint)) < MAX_ATTACHMENT_SIZE) {
		owner->fLink->StartMessage(AS_STROKE_SHAPE);
		owner->fLink->Attach<BRect>(shape->Bounds());
		owner->fLink->Attach<int32>(sd->opCount);
		owner->fLink->Attach<int32>(sd->ptCount);
		owner->fLink->Attach(sd->opList, sd->opCount);
		owner->fLink->Attach(sd->ptList, sd->ptCount);
	} else {
		// TODO: send via an area
	}
}


void
BView::FillShape(BShape *shape, ::pattern pattern)
{
	if (shape == NULL || owner == NULL)
		return;

	shape_data *sd = (shape_data *)(shape->fPrivateData);
	if (sd->opCount == 0 || sd->ptCount == 0)
		return;

	check_lock();
	_UpdatePattern(pattern);

	if ((sd->opCount * sizeof(uint32)) + (sd->ptCount * sizeof(BPoint)) < MAX_ATTACHMENT_SIZE) {
		owner->fLink->StartMessage(AS_FILL_SHAPE);
		owner->fLink->Attach<BRect>(shape->Bounds());
		owner->fLink->Attach<int32>(sd->opCount);
		owner->fLink->Attach<int32>(sd->ptCount);
		owner->fLink->Attach(sd->opList, sd->opCount);
		owner->fLink->Attach(sd->ptList, sd->ptCount);
	} else {
		// TODO: send via an area
		// BTW, in a perfect world, the fLink API would take care of that -- axeld.
	}
}


void
BView::BeginLineArray(int32 count)
{
	if (owner == NULL)
		return;

	if (count <= 0)
		debugger("Calling BeginLineArray with a count <= 0");

	check_lock_no_pick();

	if (comm) {
		delete [] comm->array;
		delete comm;
	}

	comm = new _array_data_;

	comm->maxCount = count;
	comm->count = 0;
	comm->array = new _array_hdr_[count];
}


void
BView::AddLine(BPoint pt0, BPoint pt1, rgb_color col)
{
	if (owner == NULL)
		return;

	if (!comm)
		debugger("BeginLineArray must be called before using AddLine");

	check_lock_no_pick();

	if (comm->count < comm->maxCount) {
		comm->array[comm->count].startX = pt0.x;
		comm->array[comm->count].startY = pt0.y;
		comm->array[comm->count].endX = pt1.x;
		comm->array[comm->count].endY = pt1.y;

		set_rgb_color(comm->array[comm->count].color,
			col.red, col.green, col.blue, col.alpha);

		comm->count++;
	}
}


void
BView::EndLineArray()
{
	if (owner == NULL)
		return;

	if (!comm)
		debugger("Can't call EndLineArray before BeginLineArray");

	check_lock();

	owner->fLink->StartMessage(AS_STROKE_LINEARRAY);
	owner->fLink->Attach<int32>(comm->count);
	owner->fLink->Attach(comm->array, comm->count * sizeof(_array_hdr_));

	delete [] comm->array;
	delete comm;
	comm = NULL;
}


void
BView::BeginPicture(BPicture *picture)
{
	if (do_owner_check() && picture && picture->usurped == NULL) {
		picture->usurp(cpicture);
		cpicture = picture;

		owner->fLink->StartMessage(AS_LAYER_BEGIN_PICTURE);
	}
}


void
BView::AppendToPicture(BPicture *picture)
{
	check_lock();

	if (picture && picture->usurped == NULL) {
		int32 token = picture->token;

		if (token == -1) {
			BeginPicture(picture);
		} else {
			picture->usurped = cpicture;
			picture->set_token(-1);
			owner->fLink->StartMessage(AS_LAYER_APPEND_TO_PICTURE);
			owner->fLink->Attach<int32>(token);
		}
	}
}


BPicture *
BView::EndPicture()
{
	if (do_owner_check() && cpicture) {
		int32 token;

		owner->fLink->StartMessage(AS_LAYER_END_PICTURE);

		int32 code;
		if (owner->fLink->FlushWithReply(code) == B_OK
			&& code == SERVER_TRUE
			&& owner->fLink->Read<int32>(&token) == B_OK) {
			BPicture *picture = cpicture;
			cpicture = picture->step_down();
			picture->set_token(token);

			return picture;
		}
	}

	return NULL;
}


void
BView::SetViewBitmap(const BBitmap *bitmap, BRect srcRect, BRect dstRect,
	uint32 followFlags, uint32 options)
{
	setViewImage(bitmap, srcRect, dstRect, followFlags, options);
}


void
BView::SetViewBitmap(const BBitmap *bitmap, uint32 followFlags, uint32 options)
{
	BRect rect;
 	if (bitmap)
		rect = bitmap->Bounds();

 	rect.OffsetTo(0, 0);

	setViewImage(bitmap, rect, rect, followFlags, options);
}


void
BView::ClearViewBitmap()
{
	setViewImage(NULL, BRect(), BRect(), 0, 0);
}


status_t
BView::SetViewOverlay(const BBitmap *overlay, BRect srcRect, BRect dstRect,
	rgb_color *colorKey, uint32 followFlags, uint32 options)
{
	status_t err = setViewImage(overlay, srcRect, dstRect, followFlags,
		options | 0x4);

	// TODO: Incomplete?

	// read the color that will be treated as transparent
	owner->fLink->Read<rgb_color>(colorKey);

	return err;
}


status_t
BView::SetViewOverlay(const BBitmap *overlay, rgb_color *colorKey,
	uint32 followFlags, uint32 options)
{
	BRect rect;
 	if (overlay)
		rect = overlay->Bounds();

 	rect.OffsetTo(0, 0);

	status_t err = setViewImage(overlay, rect, rect, followFlags,
			options | 0x4);

	// TODO: Incomplete?

	// read the color that will be treated as transparent
	owner->fLink->Read<rgb_color>(colorKey);

	return err;
}


void
BView::ClearViewOverlay()
{
	setViewImage(NULL, BRect(), BRect(), 0, 0);
}


void
BView::CopyBits(BRect src, BRect dst)
{
	if (!src.IsValid() || !dst.IsValid())
		return;

	if (do_owner_check()) {
		owner->fLink->StartMessage(AS_LAYER_COPY_BITS);
		owner->fLink->Attach<BRect>(src);
		owner->fLink->Attach<BRect>(dst);
	}
}


void
BView::DrawPicture(const BPicture *picture)
{
	if (picture == NULL)
		return;

	DrawPictureAsync(picture, PenLocation());
	owner->fLink->Attach<int32>(SERVER_TRUE);

	// ToDo: why a reply?
	int32 code;
	if (owner->fLink->FlushWithReply(code) == B_OK
		&& code == SERVER_TRUE) {
		status_t err;
		owner->fLink->Read<int32>(&err);
	}
}


void
BView::DrawPicture(const BPicture *picture, BPoint where)
{
	if (picture == NULL)
		return;

	DrawPictureAsync(picture, where);
	owner->fLink->Attach<int32>(SERVER_TRUE);

	// ToDo: why a reply?
	int32 code;
	if (owner->fLink->FlushWithReply(code) == B_OK
		&& code == SERVER_TRUE) {
		status_t err;
		owner->fLink->Read<int32>(&err);
	}
}


void
BView::DrawPicture(const char *filename, long offset, BPoint where)
{
	if (!filename)
		return;

	DrawPictureAsync(filename, offset, where);
	owner->fLink->Attach<int32>(SERVER_TRUE);

	// ToDo: why a reply?
	int32 code;
	if (owner->fLink->FlushWithReply(code) == B_OK
		&& code == SERVER_TRUE) {
		status_t err;
		owner->fLink->Read<int32>(&err);
	}
}


void
BView::DrawPictureAsync(const BPicture *picture)
{
	if (picture == NULL)
		return;

	DrawPictureAsync(picture, PenLocation());
}


void
BView::DrawPictureAsync(const BPicture *picture, BPoint where)
{
	if (picture == NULL)
		return;

	if (do_owner_check() && picture->token > 0) {
		owner->fLink->StartMessage(AS_LAYER_DRAW_PICTURE);
		owner->fLink->Attach<int32>(picture->token);
		owner->fLink->Attach<BPoint>(where);
	}
}


void
BView::DrawPictureAsync(const char *filename, long offset, BPoint where)
{
	if (!filename)
		return;

	// TODO: test and implement
}


void
BView::Invalidate(BRect invalRect)
{
	if (!invalRect.IsValid() || owner == NULL)
		return;

	check_lock();

	owner->fLink->StartMessage(AS_LAYER_INVAL_RECT);
	owner->fLink->Attach<BRect>(invalRect);
	owner->fLink->Flush();
}


void
BView::Invalidate(const BRegion *invalRegion)
{
	if (invalRegion == NULL || owner == NULL)
		return;

	check_lock();

	int32 count = 0;
	count = const_cast<BRegion*>(invalRegion)->CountRects();

	owner->fLink->StartMessage(AS_LAYER_INVAL_REGION);
	owner->fLink->Attach<int32>(count);

	for (int32 i = 0; i < count; i++)
		owner->fLink->Attach<BRect>( const_cast<BRegion *>(invalRegion)->RectAt(i));

	owner->fLink->Flush();
}


void
BView::Invalidate()
{
	Invalidate(Bounds());
}


void
BView::InvertRect(BRect rect)
{
	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_INVERT_RECT);
		owner->fLink->Attach<BRect>(rect);
	}
}


//	#pragma mark -
//	View Hierarchy Functions


void
BView::AddChild(BView *child, BView *before)
{
	STRACE(("BView(%s)::AddChild(child='%s' before='%s')\n",
 		this->Name() ? this->Name(): "NULL",
 		child && child->Name() ? child->Name(): "NULL",
 		before && before->Name() ? before->Name(): "NULL"));

	if (!child)
		return;

	if (child->parent != NULL)
		debugger("AddChild failed - the view already has a parent.");

	bool lockedByAddChild = false;
	if (owner && !owner->IsLocked()) {
		owner->Lock();
		lockedByAddChild = true;
	}

	if (!addToList(child, before))
		debugger("AddChild failed - cannot find 'before' view.");	

	if (owner) {
		check_lock();

 		STRACE(("BView(%s)::AddChild(child='%s' before='%s')... contacting app_server\n",
 				this->Name() ? this->Name(): "NULL",
 				child && child->Name() ? child->Name(): "NULL",
 				before && before->Name() ? before->Name(): "NULL"));

		child->setOwner(this->owner);
		attachView(child);
		callAttachHooks(child);

		if (lockedByAddChild)
			owner->Unlock();
	}

//	BVTRACE;
//	PrintTree();
//	PrintToStream();
}


bool
BView::RemoveChild(BView *child)
{
	STRACE(("BView(%s)::RemoveChild(%s)\n", this->Name(), child->Name()));
	if (!child)
		return false;

	bool rv = child->removeSelf();

//	BVTRACE;
//	PrintTree();	

	return rv;
}


int32
BView::CountChildren() const
{
	check_lock_no_pick();

	uint32 count = 0;
	BView *child = first_child;

	while (child != NULL) {
		count++;
		child = child->next_sibling;
	}

	return count;
}


BView *
BView::ChildAt(int32 index) const
{
	check_lock_no_pick();

	BView *child = first_child;
	while (child != NULL && index-- > 0) {
		child = child->next_sibling;
	}

	return child;
}


BView *
BView::NextSibling() const
{
	return next_sibling;
}


BView *
BView::PreviousSibling() const
{
	return prev_sibling;	
}


bool
BView::RemoveSelf()
{
	return removeSelf();
}


BView *
BView::Parent() const
{
	if (parent && parent->top_level_view)
		return NULL;

	return parent;
}


BView *
BView::FindView(const char *name) const
{
	if (name == NULL)
		return NULL;

	if (Name() != NULL && !strcmp(Name(), name))
		return const_cast<BView *>(this);

	BView *child = first_child;
	while (child != NULL) {
		BView *view = child->FindView(name);
		if (view != NULL)
			return view;

		child = child->next_sibling; 
	}

	return NULL;
}


void
BView::MoveBy(float deltaX, float deltaY)
{
	MoveTo(fParentOffset.x + deltaX, fParentOffset.y + deltaY);
}


void
BView::MoveTo(BPoint where)
{
	MoveTo(where.x, where.y);
}


void
BView::MoveTo(float x, float y)
{
	if (x == fParentOffset.x && y == fParentOffset.y)
		return;

	// BeBook says we should do this. We'll do it without. So...
	//	x = roundf(x);
	//	y = roundf(y);

	if (owner) {
		check_lock();
		owner->fLink->StartMessage(AS_LAYER_MOVE_TO);
		owner->fLink->Attach<float>(x);
		owner->fLink->Attach<float>(y);

		fState->valid_flags |= B_VIEW_COORD_BIT;
	}

	fParentOffset.Set(x, y);
}


void
BView::ResizeBy(float deltaWidth, float deltaHeight)
{
	ResizeTo(fBounds.right + deltaWidth, fBounds.bottom + deltaHeight);
}


void
BView::ResizeTo(float width, float height)
{
	// NOTE: I think this check makes sense, but I didn't
	// test what R5 does.
	if (width < 0.0)
		width = 0.0;
	if (height < 0.0)
		height = 0.0;
	
	if (width == fBounds.Width() && height == fBounds.Height())
		return;

	// BeBook says we should do this. We'll do it without. So...
	//	width = roundf(width);
	//	height = roundf(height);

	if (owner) {
		check_lock();
		owner->fLink->StartMessage(AS_LAYER_RESIZE_TO);
		owner->fLink->Attach<float>(width);
		owner->fLink->Attach<float>(height);

		fState->valid_flags |= B_VIEW_COORD_BIT;
	}

	fBounds.right = fBounds.left + width;
	fBounds.bottom = fBounds.top + height;
}


//	#pragma mark -
//	Inherited Methods (from BHandler)


status_t
BView::GetSupportedSuites(BMessage *data)
{
	if (data == NULL)
		return B_BAD_VALUE;

	status_t status = data->AddString("Suites", "suite/vnd.Be-view");
	if (status == B_OK) {
		BPropertyInfo propertyInfo(sViewPropInfo);

		status = data->AddFlat("message", &propertyInfo);
		if (status == B_OK)
			status = BHandler::GetSupportedSuites(data);
	}
	return status;
}


BHandler *
BView::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier,
	int32 what,	const char *property)
{
	if (msg->what == B_WINDOW_MOVE_BY
		|| msg->what == B_WINDOW_MOVE_TO)
		return this;

	BPropertyInfo propertyInfo(sViewPropInfo);

	switch (propertyInfo.FindMatch(msg, index, specifier, what, property)) {
		case B_ERROR:
			break;

		case 0:
		case 1:
		case 2:
		case 3:
		case 5:
			return this;

		case 4:
			if (fShelf) {
				msg->PopSpecifier();
				return fShelf;
			} else {
				BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);

				replyMsg.AddInt32("error", B_NAME_NOT_FOUND);
				replyMsg.AddString("message", "This window doesn't have a self");
				msg->SendReply(&replyMsg);
				return NULL;
			}
			break;

		case 6:
		case 7:
		case 8:
		{
			if (first_child) {
				BView *child;
				switch (msg->what) {
					case B_INDEX_SPECIFIER:
					{
						int32 index;
						msg->FindInt32("data", &index);
						child = ChildAt(index);
						break;
					}
					case B_REVERSE_INDEX_SPECIFIER:
					{
						int32 rindex;
						msg->FindInt32("data", &rindex);
						child = ChildAt(CountChildren() - rindex);
						break;
					}
					case B_NAME_SPECIFIER:
					{
						const char *name;
						msg->FindString("data", &name);
						child = FindView(name);
						break;
					}

					default:
						child = NULL;
						break;
				}

				if (child != NULL) {
					msg->PopSpecifier();
					return child;
				} else {
					BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
					replyMsg.AddInt32("error", B_BAD_INDEX);
					replyMsg.AddString("message", "Cannot find view at/with specified index/name.");
					msg->SendReply(&replyMsg);
					return NULL;
				}
			} else {
				BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
				replyMsg.AddInt32("error", B_NAME_NOT_FOUND);
				replyMsg.AddString("message", "This window doesn't have children.");
				msg->SendReply(&replyMsg);
				return NULL;
			}
			break;
		}

		default:
			break;
	}

	return BHandler::ResolveSpecifier(msg, index, specifier, what, property);
}


void
BView::MessageReceived(BMessage *msg)
{ 
	BMessage specifier;
	int32 what;
	const char *prop;
	int32 index;
	status_t err;

	if (!msg->HasSpecifiers()) {
		BHandler::MessageReceived(msg);
		return;
	}

	err = msg->GetCurrentSpecifier(&index, &specifier, &what, &prop);
	if (err == B_OK) {
		BMessage replyMsg;

		switch (msg->what) {
			case B_GET_PROPERTY:
			{
				replyMsg.what = B_NO_ERROR;
				replyMsg.AddInt32("error", B_OK);

				if (strcmp(prop, "Frame") == 0)
					replyMsg.AddRect("result", Frame());
				else if (strcmp(prop, "Hidden") == 0)
					replyMsg.AddBool( "result", IsHidden());
				break;
			}

			case B_SET_PROPERTY:
			{
				if (strcmp(prop, "Frame") == 0) {
					BRect newFrame;
					if (msg->FindRect("data", &newFrame) == B_OK) {
						MoveTo(newFrame.LeftTop());
						ResizeTo(newFrame.right, newFrame.bottom);

						replyMsg.what = B_NO_ERROR;
						replyMsg.AddInt32("error", B_OK);
					} else {
						replyMsg.what = B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						replyMsg.AddString("message", "Didn't understand the specifier(s)");
					}
				} else if (strcmp(prop, "Hidden") == 0) {
					bool newHiddenState;
					if (msg->FindBool("data", &newHiddenState) == B_OK) {
						if (!IsHidden() && newHiddenState == true) {
							Hide();

							replyMsg.what = B_NO_ERROR;
							replyMsg.AddInt32( "error", B_OK);
						}
						else if (IsHidden() && newHiddenState == false) {
							Show();

							replyMsg.what = B_NO_ERROR;
							replyMsg.AddInt32("error", B_OK);
						} else {
							replyMsg.what = B_MESSAGE_NOT_UNDERSTOOD;
							replyMsg.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
							replyMsg.AddString("message", "Didn't understand the specifier(s)");
						}
					} else {
						replyMsg.what = B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
						replyMsg.AddString("message", "Didn't understand the specifier(s)");
					}
				}
				break;
			}

			case B_COUNT_PROPERTIES:
				if (strcmp(prop, "View") == 0) {
					replyMsg.what = B_NO_ERROR;
					replyMsg.AddInt32("error", B_OK);
					replyMsg.AddInt32("result", CountChildren());
				}
				break;
		}

		msg->SendReply(&replyMsg);
	} else {
		BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
		replyMsg.AddInt32("error" , B_BAD_SCRIPT_SYNTAX);
		replyMsg.AddString("message", "Didn't understand the specifier(s)");

		msg->SendReply(&replyMsg);
	}
} 


status_t
BView::Perform(perform_code d, void* arg)
{
	return B_BAD_VALUE;
}


//	#pragma mark -
//	Private Functions


void
BView::InitData(BRect frame, const char *name, uint32 resizingMode, uint32 flags)
{
	// Info: The name of the view is set by BHandler constructor
	
	STRACE(("BView::InitData: enter\n"));
	
	// initialize members		
	fFlags = (resizingMode & _RESIZE_MASK_) | (flags & ~_RESIZE_MASK_);

	fParentOffset.Set(frame.left, frame.top);

	owner = NULL;
	parent = NULL;
	next_sibling = NULL;
	prev_sibling = NULL;
	first_child = NULL;

	fShowLevel = 0;
	top_level_view = false;

	cpicture = NULL;
	comm = NULL;

	fVerScroller = NULL;
	fHorScroller = NULL;

	f_is_printing = false;

	fPermanentState = NULL;
	fState = new BPrivate::ViewState;

	fBounds = frame.OffsetToCopy(0.0, 0.0);
	fShelf = NULL;
	pr_state = NULL;

	fEventMask = 0;
	fEventOptions = 0;
}


void
BView::removeCommArray()
{
	if (comm) {
		delete [] comm->array;
		delete comm;
		comm = NULL;
	}
}


void
BView::setOwner(BWindow *theOwner)
{
	if (!theOwner)
		removeCommArray();

	if (owner != theOwner && owner) {
		if (owner->fFocus == this)
			MakeFocus(false);

		if (owner->fLastMouseMovedView == this)
			owner->fLastMouseMovedView = NULL;

		owner->RemoveHandler(this);
		if (fShelf)
			owner->RemoveHandler(fShelf);
	}

	if (theOwner && theOwner != owner) {
		theOwner->AddHandler(this);
		if (fShelf)
			theOwner->AddHandler(fShelf);

		if (top_level_view)
			SetNextHandler(theOwner);
		else
			SetNextHandler(parent);
	}

	owner = theOwner;

	for (BView *child = first_child; child != NULL; child = child->next_sibling)
		child->setOwner(theOwner);
}


void
BView::DoPictureClip(BPicture *picture, BPoint where,
	bool invert, bool sync)
{
	if (!picture)
		return;

	if (do_owner_check()) {
		owner->fLink->StartMessage(AS_LAYER_CLIP_TO_PICTURE);
		owner->fLink->Attach<int32>(picture->token);
		owner->fLink->Attach<BPoint>(where);
		owner->fLink->Attach<bool>(invert);

		// TODO: I think that "sync" means another thing here:
		// the bebook, at least, says so.
		if (sync)
			owner->fLink->Flush();

		fState->valid_flags &= ~B_VIEW_CLIP_REGION_BIT;
	}

	fState->archiving_flags |= B_VIEW_CLIP_REGION_BIT;
}


bool
BView::removeSelf()
{
	STRACE(("BView(%s)::removeSelf()...\n", this->Name() ));
	
/*
	# check for dirty flags 						- by updateCachedState()
	# check for dirty flags on 'child' children		- by updateCachedState()
	# handle if in middle of Begin/EndLineArray()	- by setOwner(NULL)
	# remove trom the main tree						- by removeFromList()
	# handle if child is the default button			- HERE
	# handle if child is the focus view				- by setOwner(NULL)
	# handle if child is the menu bar				- HERE
	# handle if child token is = fLastViewToken		- by setOwner(NULL)
	# contact app_server							- HERE
	# set a new owner = NULL						- by setOwner(NULL)
*/
 	bool returnValue = true;

	if (!parent) {
		STRACE(("BView(%s)::removeSelf()... NO parent\n", this->Name()));
		return false;
	}

	if (owner) {
		check_lock();

		if (owner->fDefaultButton == this)
			owner->SetDefaultButton(NULL);

		if (owner->fKeyMenuBar == this)
			owner->fKeyMenuBar = NULL;

		if (owner->fLastViewToken == _get_object_token_(this))
			owner->fLastViewToken = B_NULL_TOKEN;

		callDetachHooks(this);

		BWindow *ownerZ = owner;

		setOwner(NULL);

		ownerZ->fLink->StartMessage(AS_LAYER_DELETE);
	}

	returnValue = removeFromList();

	parent = NULL;
	next_sibling = NULL;
	prev_sibling = NULL;

	STRACE(("DONE: BView(%s)::removeSelf()\n", this->Name()));

	return returnValue;
}


void
BView::callDetachHooks(BView *view)
{
	view->DetachedFromWindow();

	BView *child = view->first_child;	
	while (child != NULL) {
		view->callDetachHooks(child);
		child = child->next_sibling;
	}

	view->AllDetached();
}


bool
BView::removeFromList()
{
	if (parent->first_child == this) {
		parent->first_child = next_sibling;

		if (next_sibling)
			next_sibling->prev_sibling	= NULL;
	} else {
		prev_sibling->next_sibling	= next_sibling;

		if (next_sibling)
			next_sibling->prev_sibling	= prev_sibling;
	}
	return true;
}


bool
BView::addToList(BView *view, BView *before)
{
	if (!view)
		return false;

	BView *current = first_child;
	BView *last = current;

	while (current && current != before) {
		last = current;
		current = current->next_sibling;
	}

	if (!current && before)
		return false;

	// we're at begining of the list, OR between two elements
	if (current) {
		if (current == first_child) {
			view->next_sibling = current;
			current->prev_sibling = view;
			first_child = view;
		} else {
			view->next_sibling = current;
			view->prev_sibling = current->prev_sibling;
			current->prev_sibling->next_sibling = view;
			current->prev_sibling = view;
		}
	} else {
		// we have reached the end of the list

		// if last!=NULL then we add to the end. Otherwise, view is the
		// first chiild in the list
		if (last) {
			last->next_sibling = view;
			view->prev_sibling = last;
		} else
			first_child = view;
	}

	view->parent = this;
	return true;
}

void
BView::callAttachHooks(BView *view)
{
	view->AttachedToWindow();

	BView *child = view->first_child;
	while (child != NULL) {
		view->callAttachHooks(child);
		child = child->next_sibling;
	}

	view->AllAttached();
}


bool
BView::attachView(BView *view)
{
	// AS_LAYER_CREATE & AS_LAYER_CREATE_ROOT do not use the
	// current view mechanism via check_lock() - the token
	// of the view and its parent are both send to the server.

	if (view->top_level_view)
		owner->fLink->StartMessage(AS_LAYER_CREATE_ROOT);
	else
 		owner->fLink->StartMessage(AS_LAYER_CREATE);

	owner->fLink->Attach<int32>(_get_object_token_(view));
	owner->fLink->AttachString(view->Name());
	owner->fLink->Attach<BRect>(view->Frame());
	owner->fLink->Attach<uint32>(view->ResizingMode());
	owner->fLink->Attach<uint32>(view->fEventMask);
	owner->fLink->Attach<uint32>(view->fEventOptions);
	owner->fLink->Attach<uint32>(view->Flags());
	owner->fLink->Attach<bool>(view->IsHidden(view));
	owner->fLink->Attach<rgb_color>(view->fState->view_color);
	owner->fLink->Attach<int32>(_get_object_token_(this));
	owner->fLink->Flush();

	view->do_owner_check();
	view->fState->UpdateServerState(*owner->fLink);

	// we attach all its children

	BView *child = view->first_child;
	while (child != NULL) {
		view->attachView(child);
		child = child->next_sibling;
	}

	owner->fLink->Flush();

	return true;
}


void
BView::deleteView(BView* view)
{
	BView *child = view->first_child;
	while (child != NULL) {
		BView *nextChild = child->next_sibling;
		deleteView(child);
		child = nextChild;
	}

	delete view;
}


inline void
BView::_UpdatePattern(::pattern pattern)
{
	if (!fState->IsValid(B_VIEW_PATTERN_BIT) || pattern != fState->pattern)
		SetPattern(pattern);
}


BShelf *
BView::shelf() const
{
	return fShelf;
}


void
BView::set_shelf(BShelf *shelf)
{
	// TODO: is this all that needs done?
	fShelf = shelf;
}

#if 0
void
BView::updateCachedState()
{
	STRACE(("BView(%s)::updateCachedState()\n", Name() ));

	// fail if we do not have an owner
	do_owner_check();

	owner->fLink->StartMessage(AS_LAYER_GET_STATE);

	int32 code;
	if (owner->fLink->FlushWithReply(code) != B_OK
		|| code != SERVER_TRUE)
		return;

	uint32 fontID;
	float size;
	float shear;
	float rotation;
	uint8 spacing;
	uint8 encoding;
	uint16 face;
	uint32 flags;
	int32 noOfRects;
	BRect rect;	

	// read and set the font state
	owner->fLink->Read<int32>((int32 *)&fontID);
	owner->fLink->Read<float>(&size);
	owner->fLink->Read<float>(&shear);
	owner->fLink->Read<float>(&rotation);
	owner->fLink->Read<int8>((int8 *)&spacing);
	owner->fLink->Read<int8>((int8 *)&encoding);
	owner->fLink->Read<int16>((int16 *)&face);
	owner->fLink->Read<int32>((int32 *)&flags);

	fState->fontFlags = B_FONT_ALL;
	fState->font.SetFamilyAndStyle(fontID);
	fState->font.SetSize(size);
	fState->font.SetShear(shear);
	fState->font.SetRotation(rotation);
	fState->font.SetSpacing(spacing);
	fState->font.SetEncoding(encoding);
	fState->font.SetFace(face);
	fState->font.SetFlags(flags);

	// read and set view's state						 	
	owner->fLink->Read<BPoint>(&fState->penPosition);
	owner->fLink->Read<float>(&fState->penSize);	
	owner->fLink->Read<rgb_color>(&fState->highColor);
	owner->fLink->Read<rgb_color>(&fState->lowColor);
	owner->fLink->Read<pattern>(&fState->patt);
	owner->fLink->Read<BPoint>(&fState->coordSysOrigin);
	owner->fLink->Read<int8>((int8 *)&fState->drawingMode);
	owner->fLink->Read<int8>((int8 *)&fState->lineCap);
	owner->fLink->Read<int8>((int8 *)&fState->lineJoin);
	owner->fLink->Read<float>(&fState->miterLimit);
	owner->fLink->Read<int8>((int8 *)&fState->alphaSrcMode);
	owner->fLink->Read<int8>((int8 *)&fState->alphaFncMode);
	owner->fLink->Read<float>(&fState->scale);
	owner->fLink->Read<bool>(&fState->fontAliasing);

	owner->fLink->Read<int32>(&noOfRects);

	fState->clippingRegion.MakeEmpty();
	for (int32 i = 0; i < noOfRects; i++) {
		owner->fLink->Read<BRect>(&rect);
		fState->clippingRegion.Include(rect);
	}

	owner->fLink->Read<float>(&originX);
	owner->fLink->Read<float>(&originY);
	owner->fLink->Read<BRect>(&fBounds);

	fState->flags = B_VIEW_CLIP_REGION_BIT;

	STRACE(("BView(%s)::updateCachedState() - DONE\n", Name()));
}
#endif

status_t
BView::setViewImage(const BBitmap *bitmap, BRect srcRect,
	BRect dstRect, uint32 followFlags, uint32 options)
{
	if (!do_owner_check())
		return B_ERROR;

	int32 serverToken = bitmap ? bitmap->get_server_token() : -1;

	owner->fLink->StartMessage(AS_LAYER_SET_VIEW_IMAGE);
	owner->fLink->Attach<int32>(serverToken);
	owner->fLink->Attach<BRect>(srcRect);
	owner->fLink->Attach<BRect>(dstRect);
	owner->fLink->Attach<int32>(followFlags);
	owner->fLink->Attach<int32>(options);

	// TODO: this needs fixed between here and the server.
	// The server should return whatever error code is needed, whether it
	// is B_OK or whatever, not SERVER_TRUE.

	status_t status = B_ERROR;
	int32 code;
	if (owner->fLink->FlushWithReply(code) == B_OK
		&& code == SERVER_TRUE)
		owner->fLink->Read<status_t>(&status);

	return status;
}
 

void
BView::SetPattern(::pattern pattern)
{
	if (fState->IsValid(B_VIEW_PATTERN_BIT) && pattern == fState->pattern)
		return;

	if (owner) {
		check_lock();

		owner->fLink->StartMessage(AS_LAYER_SET_PATTERN);
		owner->fLink->Attach< ::pattern>(pattern);

		fState->valid_flags |= B_VIEW_PATTERN_BIT;
	}

	fState->pattern = pattern;
}
 

bool
BView::do_owner_check() const
{
	STRACE(("BView(%s)::do_owner_check()...", Name()));

	int32 serverToken = _get_object_token_(this);

	if (owner == NULL) {
		debugger("View method requires owner and doesn't have one.");
		return false;
	}

	owner->check_lock();

	if (owner->fLastViewToken != serverToken) {
		STRACE(("contacting app_server... sending token: %ld\n", serverToken));
		owner->fLink->StartMessage(AS_SET_CURRENT_LAYER);
		owner->fLink->Attach<int32>(serverToken);

		owner->fLastViewToken = serverToken;
	} else
		STRACE(("this is the lastViewToken\n"));

	return true;
}


void
BView::check_lock() const
{
	STRACE(("BView(%s)::check_lock()...", Name() ? Name(): "NULL"));

	if (!owner)
		return;

	owner->check_lock();

	int32 serverToken = _get_object_token_(this);

	if (owner->fLastViewToken != serverToken) {
		STRACE(("contacting app_server... sending token: %ld\n", serverToken));
		owner->fLink->StartMessage(AS_SET_CURRENT_LAYER);
		owner->fLink->Attach<int32>(serverToken);

		owner->fLastViewToken = serverToken;
	} else {
		STRACE(("quiet2\n"));
	}
}


void
BView::check_lock_no_pick() const
{
	if (owner)
		owner->check_lock();
}


bool
BView::do_owner_check_no_pick() const
{
	if (owner) {
		owner->check_lock();
		return true;
	} else {
		debugger("View method requires owner and doesn't have one.");
		return false;
	}
}


void BView::_ReservedView2(){}
void BView::_ReservedView3(){}
void BView::_ReservedView4(){}
void BView::_ReservedView5(){}
void BView::_ReservedView6(){}
void BView::_ReservedView7(){}
void BView::_ReservedView8(){}

#if !_PR3_COMPATIBLE_
void BView::_ReservedView9(){}
void BView::_ReservedView10(){}
void BView::_ReservedView11(){}
void BView::_ReservedView12(){}
void BView::_ReservedView13(){}
void BView::_ReservedView14(){}
void BView::_ReservedView15(){}
void BView::_ReservedView16(){}
#endif


BView::BView(const BView &other)
	: BHandler()
{
	// this is private and not functional, but exported
}


BView &
BView::operator=(const BView &other)
{
	// this is private and not functional, but exported
	return *this;
}


void
BView::PrintToStream()
{
	printf("BView::PrintToStream()\n");
	printf("\tName: %s\
\tParent: %s\
\tFirstChild: %s\
\tNextSibling: %s\
\tPrevSibling: %s\
\tOwner(Window): %s\
\tToken: %ld\
\tFlags: %ld\
\tView origin: (%f,%f)\
\tView Bounds rectangle: (%f,%f,%f,%f)\
\tShow level: %d\
\tTopView?: %s\
\tBPicture: %s\
\tVertical Scrollbar %s\
\tHorizontal Scrollbar %s\
\tIs Printing?: %s\
\tShelf?: %s\
\tEventMask: %ld\
\tEventOptions: %ld\n",
	Name(),
	parent? parent->Name() : "NULL",
	first_child? first_child->Name() : "NULL",
	next_sibling? next_sibling->Name() : "NULL",
	prev_sibling? prev_sibling->Name() : "NULL",
	owner? owner->Name() : "NULL",
	_get_object_token_(this),
	fFlags,
	fParentOffset.x, fParentOffset.y,
	fBounds.left, fBounds.top, fBounds.right, fBounds.bottom,
	fShowLevel,
	top_level_view? "YES" : "NO",
	cpicture? "YES" : "NULL",
	fVerScroller? "YES" : "NULL",
	fHorScroller? "YES" : "NULL",
	f_is_printing? "YES" : "NO",
	fShelf? "YES" : "NO",
	fEventMask,
	fEventOptions);
	
	printf("\tState status:\
\t\tLocalCoordianteSystem: (%f,%f)\
\t\tPenLocation: (%f,%f)\
\t\tPenSize: %f\
\t\tHighColor: [%d,%d,%d,%d]\
\t\tLowColor: [%d,%d,%d,%d]\
\t\tViewColor: [%d,%d,%d,%d]\
\t\tPattern: %llx\
\t\tDrawingMode: %d\
\t\tLineJoinMode: %d\
\t\tLineCapMode: %d\
\t\tMiterLimit: %f\
\t\tAlphaSource: %d\
\t\tAlphaFuntion: %d\
\t\tScale: %f\
\t\t(Print)FontAliasing: %s\
\t\tFont Info:\n",
	fState->origin.x, fState->origin.y,
	fState->pen_location.x, fState->pen_location.y,
	fState->pen_size,
	fState->high_color.red, fState->high_color.blue, fState->high_color.green, fState->high_color.alpha,
	fState->low_color.red, fState->low_color.blue, fState->low_color.green, fState->low_color.alpha,
	fState->view_color.red, fState->view_color.blue, fState->view_color.green, fState->view_color.alpha,
	*((uint64*)&(fState->pattern)),
	fState->drawing_mode,
	fState->line_join,
	fState->line_cap,
	fState->miter_limit,
	fState->alpha_source_mode,
	fState->alpha_function_mode,
	fState->scale,
	fState->font_aliasing? "YES" : "NO");

	fState->font.PrintToStream();
	
	// TODO: also print the line array.
}


void
BView::PrintTree()
{
	int32 spaces = 2;
	BView *c = first_child; //c = short for: current
	printf( "'%s'\n", Name() );
	if (c != NULL) {
		while(true) {
			// action block
			{
				for (int i = 0; i < spaces; i++)
					printf(" ");

				printf( "'%s'\n", c->Name() );
			}

			// go deep
			if (c->first_child) {
				c = c->first_child;
				spaces += 2;
			} else {
				// go right
				if (c->next_sibling) {
					c = c->next_sibling;
				} else {
					// go up
					while (!c->parent->next_sibling && c->parent != this) {
						c = c->parent;
						spaces -= 2;
					}

					// that enough! We've reached this view.
					if (c->parent == this)
						break;

					c = c->parent->next_sibling;
					spaces -= 2;
				}
			}
		}
	}
}


/* TODO:
 	-implement SetDiskMode(). What's with this method? What does it do? test!
 		does it has something to do with DrawPictureAsync( filename* .. )?
	-implement DrawAfterChildren()
*/

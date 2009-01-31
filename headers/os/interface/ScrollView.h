/*
 * Copyright 2004-2008, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _SCROLL_VIEW_H
#define _SCROLL_VIEW_H


#include <ScrollBar.h>


/*!	The BScrollView is a convenience class to add a scrolling
	mechanism to the target view.
*/
class BScrollView : public BView {
public:
							BScrollView(const char* name, BView* target,
								uint32 resizingMode
									= B_FOLLOW_LEFT | B_FOLLOW_TOP,
								uint32 flags = 0, bool horizontal = false,
								bool vertical = false,
								border_style border = B_FANCY_BORDER);
							BScrollView(const char* name, BView* target,
								uint32 flags, bool horizontal, bool vertical,
								border_style border = B_FANCY_BORDER);
							BScrollView(BMessage* archive);
	virtual					~BScrollView();

	static	BArchivable*	Instantiate(BMessage* archive);
	virtual	status_t		Archive(BMessage* archive, bool deep = true) const;

	virtual void			Draw(BRect updateRect);
	virtual void			AttachedToWindow();

			BScrollBar*		ScrollBar(orientation posture) const;

	virtual void			SetBorder(border_style border);
			border_style	Border() const;

	virtual status_t		SetBorderHighlighted(bool state);
			bool			IsBorderHighlighted() const;

			void			SetTarget(BView* target);
			BView*			Target() const;

	virtual void			MessageReceived(BMessage* message);
	virtual void			MouseDown(BPoint point);

	virtual void			WindowActivated(bool active);
	virtual	void			MouseUp(BPoint point);
	virtual	void			MouseMoved(BPoint point, uint32 code,
								const BMessage* dragMessage);

	virtual	void			DetachedFromWindow();
	virtual	void			AllAttached();
	virtual	void			AllDetached();

	virtual	void			FrameMoved(BPoint position);
	virtual	void			FrameResized(float width, float height);

	virtual BHandler*		ResolveSpecifier(BMessage* message, int32 index,
								BMessage* specifier, int32 form,
								const char* property);

	virtual void			ResizeToPreferred();
	virtual void			GetPreferredSize(float* _width, float* _height);
	virtual void			MakeFocus(bool state = true);
	virtual status_t		GetSupportedSuites(BMessage* data);

	virtual	BSize			MinSize();
	virtual	BSize			PreferredSize();

	virtual	void			InvalidateLayout(bool descendants = false);
	virtual	void			DoLayout();

	// private or reserved methods are following

	virtual status_t		Perform(perform_code d, void* arg);

private:
	friend class BView;

	virtual	void			_ReservedScrollView1();
	virtual	void			_ReservedScrollView2();
	virtual	void			_ReservedScrollView3();
	virtual	void			_ReservedScrollView4();

			BScrollView&	operator=(const BScrollView& other);

			void			_Init(bool horizontal, bool vertical);
			float			_BorderSize() const;
			BRect			_InnerFrame() const;
			BSize			_ComputeSize(BSize targetSize) const;
			BRect			_ComputeFrame(BRect targetRect) const;
			void			_AlignScrollBars(bool horizontal, bool vertical,
								BRect targetFrame);

	static	BRect			_ComputeFrame(BRect frame, bool horizontal,
								bool vertical, border_style border);
	static	BRect			_ComputeFrame(BView* target, bool horizontal,
								bool vertical, border_style border);
	static	float			_BorderSize(border_style border);
	static	int32			_ModifyFlags(int32 flags, border_style border);

			BView*			fTarget;
			BScrollBar*		fHorizontalScrollBar;
			BScrollBar*		fVerticalScrollBar;
			border_style	fBorder;
			uint16			fPreviousWidth;
			uint16			fPreviousHeight;
			bool			fHighlighted;

			uint32			_reserved[3];
};

#endif  /* _SCROLL_VIEW_H */

/*
 * Copyright 2001-2007, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Gabe Yoder <gyoder@stny.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef DRAWING_ENGINE_H_
#define DRAWING_ENGINE_H_


#include <Accelerant.h>
#include <Font.h>
#include <Locker.h>
#include <Point.h>

#include "HWInterface.h"

class BPoint;
class BRect;
class BRegion;

class DrawState;
class Painter;
class RGBColor;
class ServerBitmap;
class ServerCursor;
class ServerFont;

typedef struct {
	BPoint pt1;
	BPoint pt2;
	rgb_color color;

} LineArrayData;

class DrawingEngine : public HWInterfaceListener {
public:
							DrawingEngine(HWInterface* interface = NULL);
	virtual					~DrawingEngine();

	// HWInterfaceListener interface
	virtual	void			FrameBufferChanged();

			// for "changing" hardware
			void			SetHWInterface(HWInterface* interface);

			// locking
			bool			LockParallelAccess();
			bool			IsParallelAccessLocked();
			void			UnlockParallelAccess();

			bool			LockExclusiveAccess();
			bool			IsExclusiveAccessLocked();
			void			UnlockExclusiveAccess();

			// for screen shots
			bool			DumpToFile(const char *path);
			ServerBitmap*	DumpToBitmap();
			status_t		ReadBitmap(ServerBitmap *bitmap, bool drawCursor,
								BRect bounds);

			// clipping for all drawing functions, passing a NULL region
			// will remove any clipping (drawing allowed everywhere)
			void			ConstrainClippingRegion(const BRegion* region);

			void			SetDrawState(const DrawState* state,
								int32 xOffset = 0, int32 yOffset = 0);

			void			SetHighColor(const rgb_color& color);
			void			SetLowColor(const rgb_color& color);
			void			SetPenSize(float size);
			void			SetPattern(const struct pattern& pattern);
			void			SetDrawingMode(drawing_mode mode);

			void			SuspendAutoSync();
			void			Sync();

			// drawing functions
			void			CopyRegion(/*const*/ BRegion* region,
								int32 xOffset, int32 yOffset);

			void			InvertRect(BRect r);

			void			DrawBitmap(ServerBitmap *bitmap,
								const BRect &source, const BRect &dest,
								const DrawState *d);
			// drawing primitives

			void			DrawArc(BRect r, const float &angle,
								const float &span,
								const DrawState *d,
								bool filled);

			void			DrawBezier(BPoint *pts, const DrawState *d,
								bool filled);

			void			DrawEllipse(BRect r, const DrawState *d,
								bool filled);

			void			DrawPolygon(BPoint *ptlist, int32 numpts,
								BRect bounds, const DrawState *d,
								bool filled, bool closed);

			// these RGBColor versions are used internally by the server
			void			StrokePoint(const BPoint& pt,
								const RGBColor& color);
			void			StrokeRect(BRect r, const RGBColor &color);
			void			FillRect(BRect r, const RGBColor &color);
			void			FillRegion(BRegion& r, const RGBColor& color);

			void			StrokeRect(BRect r, const DrawState *d);
			void			FillRect(BRect r, const DrawState *d);

			void			FillRegion(BRegion& r, const DrawState *d);

			void			DrawRoundRect(BRect r, float xrad,
								float yrad, const DrawState *d,
								bool filled);

			void			DrawShape(const BRect& bounds,
								int32 opcount, const uint32* oplist, 
								int32 ptcount, const BPoint* ptlist,
								const DrawState* d, bool filled);

			void			DrawTriangle(BPoint* pts, const BRect& bounds,
								const DrawState* d, bool filled);

			// this version used by Decorator
			void			StrokeLine(const BPoint& start,
								const BPoint& end, const RGBColor& color);

			void			StrokeLine(const BPoint& start,
								const BPoint& end, DrawState* d);

			void			StrokeLineArray(int32 numlines,
								const LineArrayData* data,
								const DrawState* d);

			// -------- text related calls

			// DrawState is NOT const because this call updates the
			// pen position in the passed DrawState
			BPoint			DrawString(const char* string, int32 length,
								const BPoint& pt, DrawState* d,
								escapement_delta* delta = NULL);

			float			StringWidth(const char* string, int32 length,
								const DrawState* d,
								escapement_delta* delta = NULL);

			float			StringWidth(const char* string,
								int32 length, const ServerFont& font,
								escapement_delta* delta = NULL);

			float			StringHeight(const char* string,
								int32 length, const DrawState* d);

 private:
			BRect			_CopyRect(BRect r, int32 xOffset,
								int32 yOffset) const;

			void			_CopyRect(uint8* bits, uint32 width,
								uint32 height, uint32 bytesPerRow,
								int32 xOffset, int32 yOffset) const;

			Painter*		fPainter;
			HWInterface*	fGraphicsCard;
			uint32			fAvailableHWAccleration;
			int32			fSuspendSyncLevel;
};

#endif // DRAWING_ENGINE_H_

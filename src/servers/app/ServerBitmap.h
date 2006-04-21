/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef SERVER_BITMAP_H
#define SERVER_BITMAP_H


#include <GraphicsDefs.h>
#include <Rect.h>
#include <OS.h>

#include <video_overlay.h>


class BitmapManager;
class ClientMemoryAllocator;
class HWInterface;

/*!
	\class ServerBitmap ServerBitmap.h
	\brief Bitmap class used inside the server.
	
	This class is not directly allocated or freed. Instead, it is 
	managed by the BitmapManager class. It is also the base class for 
	all cursors. Every BBitmap has a shadow ServerBitmap object.
*/
class ServerBitmap {
 public:
	inline	bool			IsValid() const
								{ return fInitialized; }

			void			Acquire();

	inline	uint8*			Bits() const
								{ return fBuffer; }
	inline	uint32			BitsLength() const
								{ return (uint32)(fBytesPerRow * fHeight); }

	inline	BRect			Bounds() const
								{ return BRect(0, 0, fWidth - 1, fHeight - 1); }
	inline	int32			Width() const
								{ return fWidth; }
	inline	int32			Height() const
								{ return fHeight; }

	inline	int32			BytesPerRow() const
								{ return fBytesPerRow; }
	inline	uint8			BitsPerPixel() const
								{ return fBitsPerPixel; } 
	inline	color_space		ColorSpace() const
								{ return fSpace; }

	//! Returns the identifier token for the bitmap
	inline	int32			Token() const
								{ return fToken; }

	inline	void*			AllocationCookie() const
								{ return fAllocationCookie; }

			area_id			Area() const;
			uint32			AreaOffset() const;

	const	overlay_buffer*	OverlayBuffer() const;

	//! Does a shallow copy of the bitmap passed to it
	inline	void			ShallowCopy(const ServerBitmap *from);

			status_t		ImportBits(const void *bits, int32 bitsLength,
								int32 bytesPerRow, color_space colorSpace);
			status_t		ImportBits(const void *bits, int32 bitsLength,
								int32 bytesPerRow, color_space colorSpace,
								BPoint from, BPoint to, int32 width,
								int32 height);

			void			PrintToStream();

protected:
	friend class BitmapManager;
	friend class PicturePlayer;

							ServerBitmap(BRect rect,
										 color_space space,
										 int32 flags,
										 int32 bytesPerRow = -1,
										 screen_id screen = B_MAIN_SCREEN_ID);
							ServerBitmap(const ServerBitmap* bmp);
	virtual					~ServerBitmap();

	//! used by the BitmapManager
//	inline	void			_SetBuffer(void *ptr)
//								{ fBuffer = (uint8*)ptr; }

			bool			_Release();

			void			_AllocateBuffer();

			void			_HandleSpace(color_space space,
										 int32 bytesperline = -1);

			bool			fInitialized;
			ClientMemoryAllocator* fAllocator;
			void*			fAllocationCookie;
			uint8*			fBuffer;
			int32			fReferenceCount;

			int32			fWidth;
			int32			fHeight;
			int32			fBytesPerRow;
			color_space		fSpace;
			int32			fFlags;
			int				fBitsPerPixel;
			int32			fToken;
};

class UtilityBitmap : public ServerBitmap {
 public:
							UtilityBitmap(BRect rect,
										  color_space space,
										  int32 flags,
										  int32 bytesperline = -1,
										  screen_id screen = B_MAIN_SCREEN_ID);
							UtilityBitmap(const ServerBitmap* bmp);

							UtilityBitmap(const uint8* alreadyPaddedData,
										  uint32 width,
										  uint32 height,
										  color_space format);


	virtual					~UtilityBitmap();
};

//! An allocation cookie for overlays
class OverlayCookie {
	public:
		OverlayCookie(HWInterface& interface);
		~OverlayCookie();

		void SetOverlayBuffer(const overlay_buffer* overlayBuffer);
		const overlay_buffer* OverlayBuffer() const;

		void SetOverlayToken(overlay_token token);
		overlay_token OverlayToken() const;

	private:
		HWInterface&			fHWInterface;
		const overlay_buffer*	fOverlayBuffer;
		overlay_token			fOverlayToken;
};

// ShallowCopy
void
ServerBitmap::ShallowCopy(const ServerBitmap* from)
{
	if (!from)
		return;

	fInitialized	= from->fInitialized;
	fBuffer			= from->fBuffer;
	fWidth			= from->fWidth;
	fHeight			= from->fHeight;
	fBytesPerRow	= from->fBytesPerRow;
	fSpace			= from->fSpace;
	fFlags			= from->fFlags;
	fBitsPerPixel	= from->fBitsPerPixel;
	fToken			= from->fToken;
}

#endif	// SERVER_BITMAP_H

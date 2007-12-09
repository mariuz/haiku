/*
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "DiskView.h"

#include <DiskDeviceVisitor.h>
#include <HashMap.h>

using BPrivate::HashMap;
using BPrivate::HashKey32;


static const pattern	kStripes		= { { 0xc7, 0x8f, 0x1f, 0x3e,
											  0x7c, 0xf8, 0xf1, 0xe3 } };
static const rgb_color	kInvalidColor	= { 0, 0, 0, 0 };
static const rgb_color	kStripesHigh	= { 112, 112, 112, 255 };
static const rgb_color	kStripesLow		= { 104, 104, 104, 255 };

static const int32		kMaxPartitionColors = 256;
static rgb_color		kPartitionColors[kMaxPartitionColors];


struct PartitionBox {
	PartitionBox()
		: frame(0, 0, -1, -1)
		, color(kInvalidColor)
	{
	}

	PartitionBox(const BRect& frame, const rgb_color& color)
		: frame(frame)
		, color(color)
	{
	}

	PartitionBox(const PartitionBox& other)
		: frame(other.frame)
		, color(other.color)
	{
	}

	~PartitionBox()
	{
	}

	PartitionBox& operator=(const PartitionBox& other)
	{
		frame = other.frame;
		color = other.color;
		return *this;
	}

	BRect		frame;
	rgb_color	color;
};


class PartitionDrawer : public BDiskDeviceVisitor {
public:
	PartitionDrawer(BView* view)
		: fView(view)
		, fBoxMap()
		, fColorIndex(0)
	{
	}

	virtual bool Visit(BDiskDevice* device)
	{
		fBoxMap.Put(device->ID(), PartitionBox(fView->Bounds(), _NextColor()));
		return true;
	}

	virtual bool Visit(BPartition* partition, int32 level)
	{
		if (!partition->Parent() || !fBoxMap.ContainsKey(partition->Parent()->ID()))
			return false;

		PartitionBox parent = fBoxMap.Get(partition->Parent()->ID());
		BRect frame = parent.frame;
		// TODO .... calculate frame within parent frame (inset...)

		fBoxMap.Put(partition->ID(), PartitionBox(frame, _NextColor()));
		return true;
	}

 private:

	rgb_color _NextColor()
	{
		fColorIndex++;
		if (fColorIndex > kMaxPartitionColors)
			fColorIndex = 0;
		return kPartitionColors[fColorIndex];
	}

	typedef	HashKey32<partition_id>					PartitionKey;
	typedef HashMap<PartitionKey, PartitionBox >	PartitionBoxMap;

	BView*				fView;
	PartitionBoxMap		fBoxMap;
	int32				fColorIndex;
};


// #pragma mark -


DiskView::DiskView(const BRect& frame, uint32 resizeMode)
	: Inherited(frame, "diskview", resizeMode, B_WILL_DRAW)
	, fDisk(NULL)
	, fSelectedParition(-1)
{
	for (int32 i = 0; i < 256; i++) {
		kPartitionColors[i].red = (i * 1675) & 0xff;
		kPartitionColors[i].green = (i * 318) & 0xff;
		kPartitionColors[i].blue = (i * 9328) & 0xff;
		kPartitionColors[i].alpha = 255;
	}
}


DiskView::~DiskView()
{
	SetDisk(NULL, -1);
}


void
DiskView::Draw(BRect updateRect)
{
	BRect bounds(Bounds());

	if (!fDisk) {
		SetHighColor(kStripesHigh);
		SetLowColor(kStripesLow);
		FillRect(bounds, kStripes);
		return;
	}

	// TODO: render the partitions (use PartitionDrawer to iterate our disk)
	SetHighColor(255, 255, 120);
	FillRect(bounds);
}


void
DiskView::SetDisk(BDiskDevice* disk, partition_id selectedPartition)
{
	delete fDisk;
	fDisk = disk;
	fSelectedParition = selectedPartition;

	Invalidate();
}

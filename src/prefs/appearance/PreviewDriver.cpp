//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		PreviewDriver.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class to display decorators from regular BApplications
//  
//------------------------------------------------------------------------------

/*
	PreviewDriver:
		Module based on app_server's ViewDriver for the purpose of Decorator previews.
		
		The concept is to have a view draw a bitmap, which is a "frame buffer" 
		of sorts, utilize a second view to write to it. This cuts out
		the most problems with having a crapload of code to get just right without
		having to write a bunch of unit tests
		
		Components:		3 classes, VDView, VDWindow, and ViewDriver
		
		PreviewDriver - a wrapper class which mostly posts messages to the VDWindow
		PDView - doesn't do all that much except display the rendered bitmap
*/

//#define DEBUG_DRIVER_MODULE
//#define DEBUG_SERVER_EMU

//#define DISABLE_SERVER_EMU

#include <stdio.h>
#include <iostream.h>
#include <Message.h>
#include <Region.h>
#include <Bitmap.h>
#include <OS.h>
#include <Font.h>
#include <GraphicsDefs.h>
#include "PortLink.h"
#include "PatternHandler.h"
#include "PreviewDriver.h"
#include "LayerData.h"
#include "ColorSet.h"

BRect preview_bounds(0,0,150,150);;

PVView::PVView(const BRect &bounds)
	: BView(bounds,"previewdriver_view",B_FOLLOW_NONE, B_WILL_DRAW)
{
	viewbmp=new BBitmap(bounds,B_RGB32,true);
}

PVView::~PVView(void)
{
	delete viewbmp;
}

void PVView::AttachedToWindow(void)
{
	win=Window();
}

void PVView::DetachedFromWindow(void)
{
	win=NULL;
}

void PVView::Draw(BRect rect)
{
	if(viewbmp)
	{
		DrawBitmapAsync(viewbmp,rect,rect);
		Sync();
	}
}

PreviewDriver::PreviewDriver(void)
{
	view=new PVView(preview_bounds);
	drawview=new BView(preview_bounds,"drawview",B_FOLLOW_ALL, 0);
}

PreviewDriver::~PreviewDriver(void)
{
	delete view;
	delete drawview;
}

bool PreviewDriver::Initialize(void)
{
	view->viewbmp->Lock();
	view->viewbmp->AddChild(drawview);
	view->viewbmp->Unlock();
	return true;
}

void PreviewDriver::Shutdown(void)
{
	view->viewbmp->Lock();
	view->viewbmp->RemoveChild(drawview);
	view->viewbmp->Unlock();
}

void PreviewDriver::DrawBitmap(ServerBitmap *bmp, const BRect &src, const BRect &dest, const DrawData *d)
{
}

void PreviewDriver::InvertRect(const BRect &r)
{
	view->viewbmp->Lock();
	drawview->InvertRect(r);
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeLineArray(BPoint *pts, const int32 &numlines, const DrawData *d, RGBColor *colors)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->BeginLineArray(numlines);
	for(int32 i=0; i<numlines; i++)
		drawview->AddLine(pts[2*i],pts[(2*i)+1],colors[i].GetColor32());
	drawview->EndLineArray();
	drawview->Sync();
	view->Invalidate();
	view->viewbmp->Unlock();
}

void PreviewDriver::SetMode(const int32 &mode)
{
}

void PreviewDriver::SetMode(const display_mode &mode)
{
}

void PreviewDriver::FillSolidRect(const BRect &rect, RGBColor &color)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(color.GetColor32());
	drawview->FillRect(rect);
	drawview->Sync();
	view->Invalidate(rect);
	view->viewbmp->Unlock();
}

void PreviewDriver::FillPatternRect(const BRect &rect, const DrawData *d)
{
	if(!d)
		return;
	
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->FillRect(rect,*((pattern*)d->patt.GetInt8()));
	drawview->Sync();
	view->Invalidate(rect);
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeSolidLine(const BPoint &start, const BPoint &end, RGBColor &color)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(color.GetColor32());
	drawview->StrokeLine(start,end);
	drawview->Sync();
	view->Invalidate(BRect(start,end));
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokePatternLine(const BPoint &start, const BPoint &end, const DrawData *d)
{
	if(!d)
		return;
	
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->StrokeLine(start,end,*((pattern*)d->patt.GetInt8()));
	drawview->Sync();
	view->Invalidate(BRect(start,end));
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeSolidRect(const BRect &rect, RGBColor &color)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(color.GetColor32());
	drawview->StrokeRect(rect);
	drawview->Sync();
	view->Invalidate(rect);
	view->viewbmp->Unlock();
}


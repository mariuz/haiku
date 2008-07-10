/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_SUBTRACT on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_SUBTRACT_SUBPIX_H
#define DRAWING_MODE_SUBTRACT_SUBPIX_H

#include <SupportDefs.h>

#include "DrawingMode.h"
#include "PatternHandler.h"

// BLEND_SUBTRACT_SUBPIX
#define BLEND_SUBTRACT_SUBPIX(d, r, g, b, a1, a2, a3) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	uint8 rt = max_c(0, _p.data8[2] - (r)); \
	uint8 gt = max_c(0, _p.data8[1] - (g)); \
	uint8 bt = max_c(0, _p.data8[0] - (b)); \
	BLEND_SUBPIX(d, rt, gt, bt, a1, a2, a3); \
}


// BLEND_SUBTRACT
#define BLEND_SUBTRACT(d, r, g, b, a) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	uint8 rt = max_c(0, _p.data8[2] - (r)); \
	uint8 gt = max_c(0, _p.data8[1] - (g)); \
	uint8 bt = max_c(0, _p.data8[0] - (b)); \
	BLEND(d, rt, gt, bt, a); \
}


// ASSIGN_SUBTRACT
#define ASSIGN_SUBTRACT(d, r, g, b) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	d[0] = max_c(0, _p.data8[0] - (b)); \
	d[1] = max_c(0, _p.data8[1] - (g)); \
	d[2] = max_c(0, _p.data8[2] - (r)); \
	d[3] = 255; \
}


// blend_hline_subtract_subpix
void
blend_hline_subtract_subpix(int x, int y, unsigned len, const color_type& c,
	uint8 cover, agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	if (cover == 255) {
		do {
			rgb_color color = pattern->ColorAt(x, y);

			ASSIGN_SUBTRACT(p, color.red, color.green, color.blue);

			p += 4;
			x++;
			len -= 3;
		} while (len);
	} else {
		do {
			rgb_color color = pattern->ColorAt(x, y);

			BLEND_SUBTRACT(p, color.red, color.green, color.blue, cover);

			x++;
			p += 4;
			len -= 3;
		} while (len);
	}
}


// blend_solid_hspan_subtract_subpix
void
blend_solid_hspan_subtract_subpix(int x, int y, unsigned len,
	const color_type& c, const uint8* covers, agg_buffer* buffer,
	const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	do {
		rgb_color color = pattern->ColorAt(x, y);
		BLEND_SUBTRACT_SUBPIX(p, color.red, color.green, color.blue,
			covers[2], covers[1], covers[0]);
		covers += 3;
		p += 4;
		x++;
		len -= 3;
	} while (len);
}

#endif // DRAWING_MODE_SUBTRACT_SUBPIX_H


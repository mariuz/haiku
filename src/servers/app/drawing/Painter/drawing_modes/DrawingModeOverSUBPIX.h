/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_OVER on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_OVER_SUBPIX_H
#define DRAWING_MODE_OVER_SUBPIX_H

#include "DrawingMode.h"

// BLEND_OVER_SUBPIX
#define BLEND_OVER_SUBPIX(d, r, g, b, a1, a2, a3) \
{ \
	BLEND_SUBPIX(d, r, g, b, a1, a2, a3) \
}


// BLEND_OVER
#define BLEND_OVER(d, r, g, b, a) \
{ \
	BLEND(d, r, g, b, a) \
}


// ASSIGN_OVER
#define ASSIGN_OVER(d, r, g, b) \
{ \
	d[0] = (b); \
	d[1] = (g); \
	d[2] = (r); \
	d[3] = 255; \
}


// blend_hline_over_subpix
void
blend_hline_over_subpix(int x, int y, unsigned len, const color_type& c,
	uint8 cover, agg_buffer* buffer, const PatternHandler* pattern)
{
	if (cover == 255) {
		rgb_color color = pattern->HighColor();
		uint32 v;
		uint8* p8 = (uint8*)&v;
		p8[0] = (uint8)color.blue;
		p8[1] = (uint8)color.green;
		p8[2] = (uint8)color.red;
		p8[3] = 255;
		uint32* p32 = (uint32*)(buffer->row_ptr(y)) + x;
		do {
			if (pattern->IsHighColor(x, y))
				*p32 = v;
			p32++;
			x++;
			len -= 3;
		} while (len);
	} else {
		uint8* p = buffer->row_ptr(y) + (x << 2);
		rgb_color color = pattern->HighColor();
		do {
			if (pattern->IsHighColor(x, y))
				BLEND_OVER(p, color.red, color.green, color.blue, cover);
			x++;
			p += 4;
			len -= 3;
		} while (len);
	}
}


// blend_solid_hspan_over_subpix
void
blend_solid_hspan_over_subpix(int x, int y, unsigned len, const color_type& c,
	const uint8* covers, agg_buffer* buffer, const PatternHandler* pattern)
{
	uint8* p = buffer->row_ptr(y) + (x << 2);
	rgb_color color = pattern->HighColor();
	do {
		if (pattern->IsHighColor(x, y)) {
			BLEND_OVER_SUBPIX(p, color.red, color.green, color.blue,
				covers[2], covers[1], covers[0]);
		}
		covers += 3;
		p += 4;
		x++;
		len -= 3;
	} while (len);
}

#endif // DRAWING_MODE_OVER_SUBPIX_H


/*
 * Copyright 2006-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#ifndef _GRADIENT_DIAMOND_H
#define _GRADIENT_DIAMOND_H

#include <Gradient.h>

class BPoint;

class BGradientDiamond : public BGradient {
public:
	BGradientDiamond();
	BGradientDiamond(const BPoint& center);
	BGradientDiamond(float cx, float cy);
	
	BPoint Center() const;
	void SetCenter(const BPoint& center);
	void SetCenter(float cx, float cy);
};

#endif // _GRADIENT_DIAMOND_H

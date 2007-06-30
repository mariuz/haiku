/*
 * Copyright 2003-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */

const int32 kSpeakerWidth = 16;
const int32 kSpeakerHeight = 16;
const color_space kSpeakerColorSpace = B_CMAP8;

const unsigned char kSpeakerBits [] = {
	0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0x00,0x3f,0x1c,0x1c,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0x00,0x3f,0x1c,0x1c,0x1c,0x1c,0x1c,0x00,0x00,0xff,0xff,0xff,0xff,
	0xff,0xff,0x00,0x3f,0x3f,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x11,0x00,0xff,0xff,0xff,
	0xff,0xff,0x00,0x17,0x18,0x3f,0x3f,0x1c,0x1c,0x1c,0x11,0x0f,0x00,0xff,0xff,0xff,
	0xff,0xff,0x00,0x17,0x17,0x18,0x17,0x3f,0x3f,0x11,0x0f,0x0f,0x00,0xff,0xff,0xff,
	0xff,0xff,0x00,0x11,0x00,0x00,0x00,0x17,0x11,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,
	0xff,0xff,0x00,0x00,0x08,0x09,0x00,0x11,0x11,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,
	0xff,0xff,0x00,0x00,0x08,0x0f,0x0b,0x08,0x11,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,
	0xff,0xff,0x00,0x11,0x00,0x0a,0x0f,0x09,0x11,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,
	0xff,0xff,0x00,0x17,0x11,0x08,0x09,0x11,0x11,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,
	0xff,0xff,0x00,0x17,0x17,0x17,0x17,0x17,0x11,0x0f,0x0f,0x0f,0x00,0x0f,0xff,0xff,
	0xff,0xff,0x00,0x00,0x17,0x17,0x2d,0x18,0x11,0x0f,0x0f,0x00,0x0f,0x0f,0xff,0xff,
	0xff,0xff,0xff,0xff,0x00,0x00,0x17,0x17,0x11,0x0f,0x00,0x0f,0x0f,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x11,0x00,0x0f,0x0f,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x0f,0x0f,0xff,0xff,0xff,0xff,0xff
};

const int32 kButtonWidth = 16;
const int32 kButtonHeight = 11;
const color_space kButtonColorSpace = B_CMAP8;

const unsigned char kButtonBits [] = {
	0xff,0xff,0xff,0x18,0x1d,0x3f,0x1d,0x19,0x12,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0x15,0x1d,0x1e,0x1e,0x1d,0x1e,0x1b,0x19,0x12,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0x1e,0x1e,0x1d,0x1d,0x3f,0x1d,0x1d,0x1b,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x1a,0x1e,0x1d,0x1d,0x1d,0xff,0x1d,0x1d,0x1d,0x19,0x12,0xff,0xff,0xff,0xff,0xff,
	0x1e,0x1e,0x1d,0x1d,0x1d,0x3f,0x1d,0x1d,0x1d,0x19,0x11,0xff,0xff,0xff,0xff,0xff,
	0x3f,0x1d,0x1d,0x1d,0x1d,0xff,0x1d,0x1d,0x1d,0x1c,0x0e,0xff,0xff,0xff,0xff,0xff,
	0x1e,0x1e,0x1d,0x1d,0x1d,0x3f,0x1d,0x1d,0x1d,0x19,0x11,0xff,0xff,0xff,0xff,0xff,
	0x1a,0x1d,0x1d,0x1d,0x1d,0xff,0x1d,0x1d,0x1d,0x15,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0x1a,0x19,0x1d,0x1d,0x1d,0x1d,0x1d,0x17,0x12,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0x16,0xff,0x15,0x19,0x1c,0x19,0x15,0x12,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0x14,0x11,0x0e,0x11,0x14,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

const int32 kLeftWidth = 16;
const int32 kLeftHeight = 15;
const color_space kLeftColorSpace = B_CMAP8;

const unsigned char kLeftBits [] = {
	0xff,0xff,0xff,0xff,0x19,0x19,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0x1a,0x18,0x11,0x0b,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0x19,0x15,0x0b,0x06,0x0a,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x1a,0x15,0x09,0x09,0xb6,0x8f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x18,0x0b,0x09,0x8f,0x8f,0x8f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x11,0x07,0xb6,0x8f,0x8f,0x68,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x0c,0x0c,0x8f,0x8f,0x68,0x68,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x07,0x10,0x8f,0x68,0x68,0x68,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x16,0x12,0x8f,0x68,0x68,0x68,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x1a,0x16,0x11,0x68,0x68,0x68,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0x1a,0x18,0x68,0x68,0x68,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0x1c,0x19,0x68,0x68,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0x1d,0x1c,0x19,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0x1d,0x1e,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0x1c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

const int32 kRightWidth = 16;
const int32 kRightHeight = 15;
const color_space kRightColorSpace = B_CMAP8;

const unsigned char kRightBits [] = {
	0x19,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x11,0x18,0x1a,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x08,0x0d,0x16,0x1a,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x11,0x0c,0x0f,0x17,0x1a,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x13,0x13,0x10,0x14,0x19,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x13,0x13,0x13,0x15,0x19,0x1a,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x13,0x13,0x13,0x16,0x1a,0x1a,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x13,0x13,0x13,0x15,0x1c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x13,0x13,0x13,0x19,0x1e,0x1c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x13,0x13,0x15,0x1d,0x1d,0x1c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x13,0x14,0x1a,0x1e,0x1c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x15,0xff,0x3f,0x1d,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x1d,0x1e,0x1d,0x1c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x1d,0x1c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x1c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};


/*
 * OpenBeOS floppy driver
 * (c) 2003, OpenBeOS project.
 * François Revol, revol@free.fr
 */

const char floppy_icon[] = {
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0x0e, 
0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0a, 0x00, 0x00, 0x0f, 
0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0a, 0x0b, 0x04, 0x00, 0x00, 
0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0a, 0x0b, 0x04, 0x00, 0x2d, 0x2e, 
0x2d, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0a, 0x0b, 0x04, 0x00, 0x3f, 0x3f, 0x2d, 
0x2e, 0x2d, 0xd2, 0x00, 0x00, 0x0e, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0a, 0x0b, 0x04, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 
0x3f, 0xd2, 0xd2, 0xd2, 0xd2, 0x00, 0x00, 0x0e, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0a, 0x0b, 0x04, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 
0x3f, 0x3f, 0x3f, 0xd2, 0xd2, 0xd2, 0xd2, 0x00, 0x00, 0x0e, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0a, 0x0b, 0x04, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 
0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0xd2, 0xd2, 0x00, 0x17, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0e, 0xff, 
0xff, 0xff, 0xff, 0xff, 0x00, 0x0a, 0x0b, 0x0a, 0x0b, 0x17, 0x00, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 
0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x17, 0x0b, 0x0a, 0x0b, 0x00, 0x0f, 0x0f, 0x0e, 0xff, 
0xff, 0xff, 0xff, 0x00, 0x0a, 0x0b, 0x0a, 0x0b, 0x0b, 0x0a, 0x18, 0x17, 0x00, 0x00, 0x3f, 0x3f, 
0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x17, 0x0b, 0x0a, 0x0b, 0x04, 0x00, 0x0f, 0x0f, 0x0f, 0xff, 
0xff, 0xff, 0x00, 0x0a, 0x0b, 0x0a, 0x0b, 0x0b, 0x0a, 0x0b, 0x0b, 0x0a, 0x18, 0x17, 0x00, 0x00, 
0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x17, 0x0b, 0x0a, 0x0b, 0x04, 0x04, 0x00, 0x0f, 0x0f, 0xff, 0xff, 
0xff, 0x00, 0x0a, 0x0b, 0x0a, 0x0b, 0x15, 0x00, 0x00, 0x0b, 0x0b, 0x0a, 0x0b, 0x0a, 0x18, 0x17, 
0x00, 0x00, 0x3f, 0x00, 0x17, 0x0b, 0x0a, 0x0b, 0x04, 0x04, 0x00, 0x0f, 0x0f, 0xff, 0xff, 0xff, 
0x00, 0x0a, 0x0b, 0x0a, 0x0b, 0x15, 0x00, 0x18, 0x17, 0x00, 0x00, 0x0b, 0x0a, 0x0b, 0x0b, 0x0a, 
0x18, 0x17, 0x00, 0x17, 0x0b, 0x0b, 0x0a, 0x04, 0x05, 0x00, 0x0f, 0x0e, 0xff, 0xff, 0xff, 0xff, 
0x00, 0x3f, 0x3f, 0x0a, 0x15, 0x00, 0x18, 0x17, 0x04, 0x0b, 0x17, 0x00, 0x00, 0x0b, 0x0b, 0x0a, 
0x0b, 0x0b, 0x17, 0x0b, 0x0a, 0x0b, 0x04, 0x04, 0x00, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 
0x00, 0x15, 0x15, 0x3f, 0x00, 0x17, 0x17, 0x05, 0x00, 0x04, 0x17, 0x18, 0x17, 0x00, 0x00, 0x0b, 
0x0a, 0x0b, 0x0a, 0x0b, 0x0b, 0x04, 0x04, 0x00, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0x00, 0x00, 0x15, 0x00, 0x3f, 0x3f, 0x00, 0x04, 0x17, 0x18, 0x17, 0x17, 0x18, 0x17, 0x00, 
0x0b, 0x0a, 0x0b, 0x0b, 0x04, 0x04, 0x00, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0x00, 0x00, 0x15, 0x15, 0x3f, 0x3f, 0x17, 0x17, 0x18, 0x17, 0x17, 0x00, 0x18, 
0x0a, 0x0b, 0x0b, 0x04, 0x04, 0x00, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x15, 0x15, 0x3f, 0x3f, 0x17, 0x17, 0x00, 0x18, 0x0a, 
0x0b, 0x0b, 0x04, 0x04, 0x00, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x15, 0x15, 0x3f, 0x00, 0x17, 0x0b, 0x0a, 
0x0b, 0x04, 0x04, 0x00, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x15, 0x00, 0x3f, 0x3f, 0x0a, 
0x04, 0x05, 0x00, 0x0f, 0x0e, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x15, 0x15, 0x04, 
0x05, 0x00, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x04, 
0x00, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 
0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

const char floppy_mini_icon[] = {
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0e, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x0e, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0x00, 0x0a, 0x00, 0x2d, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0x00, 0x0a, 0x00, 0x3f, 0x2d, 0x2e, 0x2d, 0x00, 0x00, 0x0e, 0x0f, 0x0f, 0xff, 
0xff, 0xff, 0x00, 0x0a, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0xd2, 0xd2, 0xd2, 0x00, 0x00, 0x0e, 0x0f, 
0xff, 0x00, 0x0a, 0x0b, 0x0a, 0x00, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x0a, 0x00, 0x0f, 0x0f, 
0x00, 0x0b, 0x0a, 0x00, 0x00, 0x0b, 0x0a, 0x00, 0x00, 0x3f, 0x00, 0x0a, 0x04, 0x00, 0x0f, 0x0f, 
0x00, 0x3f, 0x00, 0x17, 0x17, 0x00, 0x00, 0x0b, 0x0b, 0x00, 0x0a, 0x04, 0x00, 0x0f, 0x0f, 0xff, 
0x00, 0x15, 0x00, 0x3f, 0x3f, 0x17, 0x17, 0x00, 0x0b, 0x0b, 0x04, 0x00, 0x0f, 0x0f, 0xff, 0xff, 
0xff, 0x00, 0x00, 0x15, 0x15, 0x3f, 0x3f, 0x00, 0x0a, 0x04, 0x00, 0x0f, 0x0f, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0x00, 0x00, 0x15, 0x00, 0x0a, 0x05, 0x00, 0x0f, 0x0e, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x04, 0x00, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

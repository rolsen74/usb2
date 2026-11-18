
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2_XBOX360_H
#define USB2_XBOX360_H

// --

#include <dos/dos.h>
#include <proto/exec.h>
#include <interfaces/usb2.h>

// --

#define XBOX360_DRIVER "Xbox360.Driver"
#define XBOX360_VERSION 1
#define XBOX360_REVISION 0
#define XBOX360_ID XBOX360_DRIVER " 1.0\r\n"

#define XBOX360_SUBCLASS 0x5d
#define XBOX360_PROTOCOL 0x01
#define XBOX360_REPORT_SIZE 20
#define XBOX360_AXIS_COUNT 4
#define XBOX360_READ_ERRORS_MAX 3

// --

struct Xbox360Base
{
	struct Library		Library;
	PTR					SegList;
};

struct Xbox360State
{
	U16 Buttons;		// A, B, X, Y, LB, RB, Back, Start, LS, RS, Guide
	S8 HatX;		// Left = -1, right = 1
	S8 HatY;		// Up = -1, down = 1
	U8 TriggerLeft;
	U8 TriggerRight;
	S16 Axis[XBOX360_AXIS_COUNT];
};

// --

extern const PTR Xbox360_Interfaces[];

struct Xbox360Base *Xbox360_Init( PTR unused, PTR seglist, struct ExecBase *sysbase );
PTR Xbox360_Open( struct LibraryManagerInterface *Self, U32 version );
PTR Xbox360_Close( struct LibraryManagerInterface *Self );
PTR Xbox360_Expunge( struct LibraryManagerInterface *Self );
U32 Xbox360_Obtain( struct Interface *Self );
U32 Xbox360_Release( struct Interface *Self );

U32 Xbox360_Entry( struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg );
void Xbox360_Poll( struct USB2IFace *usb, struct USB2_Register *reg, struct USB2_EPResource *input, U8 interface );
void Xbox360_SetPlayerLED( struct USB2IFace *usb, struct USB2_Register *reg, U8 interface );
S32 Xbox360_DecodeReport( const U8 *report, U32 length, struct Xbox360State *state );

// --

#endif


/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "Xbox360.h"

// --

/*
 * FreeBSD report mapping, copyright 2000 Nick Hibma, 2005 Ed Schouten,
 * and 2020 Vladimir Kondratyev and Val Packett.
 * See LICENSE.FreeBSD for the BSD-2-Clause terms.
 */

// --

S32
Xbox360_DecodeReport( const U8 *report, U32 length, struct Xbox360State *state )
{
	// HID button order from FreeBSD's Xbox 360 report descriptor.
	static const U16 button_masks[] =
	{
		0x1000, 0x2000, 0x4000, 0x8000,	// A, B, X, Y
		0x0100, 0x0200,					// Bumpers
		0x0020, 0x0010,					// Back, Start
		0x0040, 0x0080, 0x0400			// Stick clicks, Guide
	};

	U32 button;
	U32 axis;
	U32 offset;
	U16 buttons;
	S32 raw;

	// The original driver identified input packets by the 00 14 header.
	if ( ( ! report ) || ( ! state ) || ( length < XBOX360_REPORT_SIZE ) ||
		( report[0] != 0 ) || ( report[1] != XBOX360_REPORT_SIZE ) )
	{
		return ( FALSE );
	}

	buttons = report[2] | ( (U16) report[3] << 8 );
	state->Buttons = 0;
	state->HatX = !! ( buttons & 0x0008 ) - !! ( buttons & 0x0004 );
	state->HatY = !! ( buttons & 0x0002 ) - !! ( buttons & 0x0001 );
	state->TriggerLeft = report[4];
	state->TriggerRight = report[5];

	for( button = 0; button < sizeof( button_masks ) / sizeof( button_masks[0] ); button++ )
	{
		if ( buttons & button_masks[button] )
		{
			state->Buttons |= 1U << button;
		}
	}

	for( axis = 0; axis < XBOX360_AXIS_COUNT; axis++ )
	{
		offset = 6 + axis * 2;
		raw = report[offset] | ( (U16) report[offset + 1] << 8 );
		raw = ( raw ^ 0x8000 ) - 0x8000;

		// Vertical axes follow FreeBSD, with negative values pointing up.
		state->Axis[axis] = ( axis & 1 ) ? -1 - raw : raw;
	}

	return ( TRUE );
}

// --

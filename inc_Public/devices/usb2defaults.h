/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef DEVICES_USB2DEFAULTS_H
#define DEVICES_USB2DEFAULTS_H

#include <devices/usb2.h>
#include <dos/var.h>
#include <interfaces/dos.h>
#include <interfaces/usb2units.h>

#define USB2_PREFS_VAR "USB2-Prefs"
#define USB2_DEFAULTS_VAR "USB2-DefaultUnits"
#define USB2_DEFAULT_UNIT 65536UL
#define USB2_UNIT_AUTOMATIC (-1)
#define USB2_UNIT_FIRST (-2)
#define USB2_MAX_DEFAULT_UNIT 65535
#define USB2_PREF_AUTO "automatic"
#define USB2_PREF_FIRST "first"
#define USB2_PREF_SELECTOR_SIZE 96
#define USB2_PREFS_TEXT_SIZE ( 2 * USB2_PREF_SELECTOR_SIZE + 40 )

enum USB2DefaultDevice
{
	USB2_DEFAULT_SERIAL,
	USB2_DEFAULT_PARALLEL,
	USB2_DEFAULT_VIDEO,
	USB2_DEFAULT_PRINTER,
	USB2_DEFAULT_BLUETOOTH,
	USB2_DEFAULT_COUNT
};

struct USB2Prefs
{
	S32 Units[USB2_DEFAULT_COUNT];
	char Audio[USB2_PREF_SELECTOR_SIZE];
	char Ethernet[USB2_PREF_SELECTOR_SIZE];
};

#pragma push_macro("GetVar")
#undef GetVar

static inline S32
USB2_ReadPrefs( struct DOSIFace *dos, struct USB2Prefs *prefs )
{
	char text[USB2_PREFS_TEXT_SIZE];
	const char *cursor;
	char *selector;
	S32 parsed[USB2_DEFAULT_COUNT];
	S32 count;
	S32 value;
	S32 current;
	U32 index;
	U32 length;

	for( index = 0; index < USB2_DEFAULT_COUNT; index++ )
	{
		prefs->Units[index] = USB2_UNIT_AUTOMATIC;
	}

	prefs->Audio[0] = 0;
	prefs->Ethernet[0] = 0;

	if ( ! dos )
	{
		return ( FALSE );
	}

	count = dos->GetVar( USB2_PREFS_VAR, text, sizeof( text ), GVF_GLOBAL_ONLY );

	current = ( count >= 0 );

	if ( ! current )
	{
		count = dos->GetVar( USB2_DEFAULTS_VAR, text, sizeof( text ), GVF_GLOBAL_ONLY );
	}

	if ( ( count <= 0 ) || ( (U32) count >= sizeof( text ) ) )
	{
		return ( FALSE );
	}

	text[count] = 0;
	cursor = text;

	for( index = 0; index < USB2_DEFAULT_COUNT; index++ )
	{
		if ( ( cursor[0] == '-' ) && ( ( cursor[1] == '1' ) || ( ( current ) && ( cursor[1] == '2' ) ) ) )
		{
			value = '0' - cursor[1];
			cursor += 2;
		}
		else
		{
			value = 0;

			if ( ( *cursor < '0' ) || ( *cursor > '9' ) )
			{
				return ( FALSE );
			}

			while( ( *cursor >= '0' ) && ( *cursor <= '9' ) )
			{
				value = value * 10 + *cursor++ - '0';

				if ( value > USB2_MAX_DEFAULT_UNIT )
				{
					return ( FALSE );
				}
			}
		}

		if ( *cursor != ( ( ! current ) && ( index + 1 == USB2_DEFAULT_COUNT ) ? 0 : ' ' ) )
		{
			return ( FALSE );
		}

		parsed[index] = value;
		cursor++;
	}

	if ( current )
	{
		for( index = 0; index < 2; index++ )
		{
			selector = index ? prefs->Ethernet : prefs->Audio;
			length = 0;

			while( ( *cursor ) && ( *cursor != ' ' ) && ( length < USB2_PREF_SELECTOR_SIZE - 1 ) )
			{
				selector[length++] = *cursor++;
			}

			selector[length] = 0;

			if ( ( ! length ) || ( *cursor != ( index ? 0 : ' ' ) ) )
			{
				prefs->Audio[0] = 0;
				prefs->Ethernet[0] = 0;

				return ( FALSE );
			}

			cursor++;
		}
	}

	for( index = 0; index < USB2_DEFAULT_COUNT; index++ )
	{
		prefs->Units[index] = parsed[index];
	}

	return ( current );
}

#pragma pop_macro("GetVar")

#endif

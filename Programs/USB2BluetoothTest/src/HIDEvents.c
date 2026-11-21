
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <devices/inputevent.h>
#include "HIDEvents.h"

#define USB2_KEYMAP_STORAGE static
#include "../../../Drivers/HID/KeyboardMap.h"
#undef USB2_KEYMAP_STORAGE

// --

struct BTHIDFlag
{
	uint8_t Mask;
	uint16_t Qualifier;
	uint16_t Code;
};

static const struct BTHIDFlag Modifiers[] =
{
	{ 0x02, IEQUALIFIER_LSHIFT, 0x60 },
	{ 0x20, IEQUALIFIER_RSHIFT, 0x61 },
	{ 0x11, IEQUALIFIER_CONTROL, 0x63 },
	{ 0x04, IEQUALIFIER_LALT, 0x64 },
	{ 0x40, IEQUALIFIER_RALT, 0x65 },
	{ 0x08, IEQUALIFIER_LCOMMAND, 0x66 },
	{ 0x80, IEQUALIFIER_RCOMMAND, 0x67 },
};

static const struct BTHIDFlag Buttons[] =
{
	{ 1, IEQUALIFIER_LEFTBUTTON, IECODE_LBUTTON },
	{ 2, IEQUALIFIER_RBUTTON, IECODE_RBUTTON },
	{ 4, IEQUALIFIER_MIDBUTTON, IECODE_MBUTTON },
};

// --

static void
Emit( struct BTHIDEvents *events, uint8_t kind, uint16_t code, uint16_t qualifier, int16_t x, int16_t y )
{
	if ( ! events->Emit( events->Context, kind, code, qualifier, x, y ) )
	{
		events->Failed = 1;
	}
}

// --

static void
UpdateFlags( struct BTHIDEvents *events, const struct BTHIDFlag *flags, uint32_t count,
	uint8_t kind, uint8_t old, uint8_t current )
{
	uint32_t index;
	uint16_t code;
	int down;

	for( index = 0; index < count; index++ )
	{
		down = ( current & flags[index].Mask ) != 0;

		if ( down == ( ( old & flags[index].Mask ) != 0 ) )
		{
			continue;
		}

		code = flags[index].Code;

		if ( down )
		{
			events->Qualifier |= flags[index].Qualifier;
		}
		else
		{
			events->Qualifier &= ~flags[index].Qualifier;
			code |= IECODE_UP_PREFIX;
		}

		Emit( events, kind, code, events->Qualifier, 0, 0 );
	}
}

// --

static int
KeyIndex( const uint8_t *keys, uint32_t count, uint8_t usage )
{
	uint32_t index;

	for( index = 0; index < count; index++ )
	{
		if ( keys[index] == usage )
		{
			return ( index );
		}
	}

	return ( -1 );
}

// --

static void
Keyboard( struct BTHIDEvents *events, const uint8_t *data )
{
	uint8_t keys[6];
	int16_t codes[6];
	uint32_t index;
	int previous;
	int16_t raw;
	uint8_t usage;
	int rollover;

	rollover = 0;

	for( index = 0; index < 6; index++ )
	{
		usage = data[index + 2];
		keys[index] = 0;
		codes[index] = -1;

		if ( ( usage ) && ( usage <= 3 ) )
		{
			rollover = 1;
		}
		else if ( ( usage ) && ( KeyIndex( keys, index, usage ) < 0 ) )
		{
			keys[index] = usage;
		}
	}

	if ( ! rollover )
	{
		for( index = 0; index < 6; index++ )
		{
			if ( ( events->Keys[index] ) && ( events->Codes[index] >= 0 ) &&
				( KeyIndex( keys, 6, events->Keys[index] ) < 0 ) )
			{
				Emit( events, IECLASS_RAWKEY, events->Codes[index] | IECODE_UP_PREFIX, events->Qualifier, 0, 0 );
			}
		}
	}

	UpdateFlags( events, Modifiers, sizeof( Modifiers ) / sizeof( Modifiers[0] ),
		IECLASS_RAWKEY, events->Modifiers, data[0] );

	events->Modifiers = data[0];

	if ( rollover )
	{
		return;
	}

	for( index = 0; index < 6; index++ )
	{
		usage = keys[index];

		if ( ! usage )
		{
			continue;
		}

		previous = KeyIndex( events->Keys, 6, usage );

		if ( previous >= 0 )
		{
			codes[index] = events->Codes[previous];

			continue;
		}

		if ( usage == 83 )
		{
			events->NumLock ^= 1;

			continue;
		}

		if ( usage == 57 )
		{
			events->Qualifier ^= IEQUALIFIER_CAPSLOCK;
		}

		raw = events->NumLock ? USBKeyMap1[usage] : USBKeyMap2[usage];
		codes[index] = raw;

		if ( raw >= 0 )
		{
			Emit( events, IECLASS_RAWKEY, raw, events->Qualifier, 0, 0 );
		}
	}

	for( index = 0; index < 6; index++ )
	{
		events->Keys[index] = keys[index];
		events->Codes[index] = codes[index];
	}
}

// --

static int16_t
MouseDelta( uint8_t byte )
{
	return ( byte < 128 ? (int16_t) byte : (int16_t) byte - 256 );
}

// --

static void
Mouse( struct BTHIDEvents *events, const uint8_t *data )
{
	int16_t x;
	int16_t y;

	UpdateFlags( events, Buttons, sizeof( Buttons ) / sizeof( Buttons[0] ),
		IECLASS_RAWMOUSE, events->Buttons, data[0] );

	events->Buttons = data[0] & 7;

	x = MouseDelta( data[1] );

	y = MouseDelta( data[2] );

	if ( ( x ) || ( y ) )
	{
		Emit( events, IECLASS_RAWMOUSE, IECODE_NOBUTTON, events->Qualifier | IEQUALIFIER_RELATIVEMOUSE, x, y );
	}
}

// --

int32_t
ApplyHIDReport( struct BTHIDEvents *events, const struct BTHIDReport *report )
{
	static const uint8_t empty[8] = { 0 };

	events->Failed = 0;

	if ( ! report )
	{
		Keyboard( events, empty );

		Mouse( events, empty );
	}
	else if ( ( report->ID == BT_HID_BOOT_KEYBOARD ) && ( report->Length >= 8 ) )
	{
		Keyboard( events, report->Data );
	}
	else if ( ( report->ID == BT_HID_BOOT_MOUSE ) && ( report->Length >= 3 ) )
	{
		Mouse( events, report->Data );
	}

	return ( ! events->Failed );
}

// --

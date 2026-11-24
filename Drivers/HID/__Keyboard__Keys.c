
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HID.h"

// --

SEC_RODATA static const struct
{
	U8 Mask;
	U8 Raw;
	U16 Qualifier;
} Modifiers[] =
{
	{ USBHID_BOOTKBD_MODF_LCTRL | USBHID_BOOTKBD_MODF_RCTRL, 0x63, IEQUALIFIER_CONTROL },
	{ USBHID_BOOTKBD_MODF_LSHIFT, 0x60, IEQUALIFIER_LSHIFT },
	{ USBHID_BOOTKBD_MODF_RSHIFT, 0x61, IEQUALIFIER_RSHIFT },
	{ USBHID_BOOTKBD_MODF_LALT, 0x64, IEQUALIFIER_LALT },
	{ USBHID_BOOTKBD_MODF_RALT, 0x65, IEQUALIFIER_RALT },
	{ USBHID_BOOTKBD_MODF_LCOMMAND, 0x66, IEQUALIFIER_LCOMMAND },
	{ USBHID_BOOTKBD_MODF_RCOMMAND, 0x67, IEQUALIFIER_RCOMMAND }
};

// --

SEC_CODE static void
RepeatDelay( struct USBBase *usbbase, struct HID_ReportNode *rn, struct TimeVal *delay )
{
	if ( ( delay->Seconds ) || ( delay->Microseconds ) )
	{
		rn->Timer_IOReq.Time = *delay;
		rn->Timer_IOReq.Request.io_Command = TR_ADDREQUEST;
		rn->Timer_Added = TRUE;

		IO_SEND( &rn->Timer_IOReq );
	}
}

// --

SEC_CODE void
__Keyboard_Timer( struct USBBase *usbbase, struct HID_ReportNode *rn )
{
	S16 raw;

	if ( ! rn->Keyboard_Repeat )
	{
		return;
	}

	raw = rn->RawKeys[rn->Keyboard_Repeat];

	SEMAPHORE_OBTAIN( &usbbase->usb_HID_Semaphore );

	Input_Raw_Key( usbbase, rn, raw, usbbase->usb_HID_Qualifiers | IEQUALIFIER_REPEAT );

	SEMAPHORE_RELEASE( &usbbase->usb_HID_Semaphore );

	RepeatDelay( usbbase, rn, &rn->Time_Repeat );
}

// --

SEC_CODE static void
UpdateModifiers( struct USBBase *usbbase, struct HID_ReportNode *rn, U8 old, U8 keys )
{
	U32 index;
	U32 down;
	U32 raw;

	for ( index = 0; index < sizeof( Modifiers ) / sizeof( Modifiers[0] ); index++ )
	{
		down = ( keys & Modifiers[index].Mask ) != 0;
		raw = Modifiers[index].Raw;

		if ( ( ( ( old & Modifiers[index].Mask ) != 0 ) != down ) &&
			( HID_Hold_Input( usbbase, raw, Modifiers[index].Qualifier, down ) ) )
		{
			Input_Raw_Key( usbbase, rn, raw | ( down ? 0 : IECODE_UP_PREFIX ), usbbase->usb_HID_Qualifiers );
		}
	}
}

// --

SEC_CODE void
__Keyboard_Keys( struct USBBase *usbbase, struct HID_ReportNode *rn, const U8 *keys )
{
	U32 usage;
	U32 bit;
	U16 leds;
	S16 raw;
	S16 repeat;

	leds = rn->Keyboard_LEDs;
	repeat = rn->Keyboard_Repeat;

	SEMAPHORE_OBTAIN( &usbbase->usb_HID_Semaphore );

	UpdateModifiers( usbbase, rn, rn->Keys[0xe0 / 8], keys[0xe0 / 8] );

	// Release using the raw mapping recorded at key-down.
	for ( usage = 4; usage < 256; usage++ )
	{
		bit = 1U << ( usage & 7 );

		if ( ( rn->Keys[usage / 8] & bit ) && ( ! ( keys[usage / 8] & bit ) ) )
		{
			raw = rn->RawKeys[usage];

			if ( usage == (U32) repeat )
			{
				repeat = 0;
			}

			if ( ( raw >= 0 ) && ( HID_Hold_Input( usbbase, raw, 0, FALSE ) ) )
			{
				Input_Raw_Key( usbbase, rn, raw | IECODE_UP_PREFIX, usbbase->usb_HID_Qualifiers );
			}
		}
	}

	for ( usage = 4; usage < 256; usage++ )
	{
		bit = 1U << ( usage & 7 );

		if ( ( keys[usage / 8] & bit ) && ( ! ( rn->Keys[usage / 8] & bit ) ) )
		{
			switch ( usage )
			{
				case 57:
					usbbase->usb_HID_Qualifiers ^= IEQUALIFIER_CAPSLOCK;
					rn->Keyboard_LEDs ^= HID_LED_CapsLock;
					break;

				case 71:
					rn->Keyboard_LEDs ^= HID_LED_ScrollLock;
					break;

				case 83:
					usbbase->usb_HID_Qualifiers ^= IEQUALIFIER_NUMERICPAD;
					rn->Keyboard_LEDs ^= HID_LED_NumLock;
					break;
			}

			raw = ( usbbase->usb_HID_Qualifiers & IEQUALIFIER_NUMERICPAD ) ? USBKeyMap1[usage] : USBKeyMap2[usage];
			rn->RawKeys[usage] = raw;

			if ( raw >= 0 )
			{
				if ( ( usage != 57 ) && ( usage != 71 ) && ( usage != 83 ) )
				{
					repeat = usage;
				}

				if ( HID_Hold_Input( usbbase, raw, 0, TRUE ) )
				{
					Input_Raw_Key( usbbase, rn, raw, usbbase->usb_HID_Qualifiers );
				}
			}
		}
	}

	SEMAPHORE_RELEASE( &usbbase->usb_HID_Semaphore );

	for ( usage = 0; usage < sizeof( rn->Keys ); usage++ )
	{
		rn->Keys[usage] = keys[usage];
	}

	if ( repeat != rn->Keyboard_Repeat )
	{
		rn->Keyboard_Repeat = repeat;

		if ( rn->Timer_Added )
		{
			IO_STOP( &rn->Timer_IOReq );

			rn->Timer_Added = FALSE;
		}

		if ( ( repeat ) && ( rn->Running ) )
		{
			if ( MISC_GETINTUIPREFS( &rn->Prefs_Buffer, 20 ) )
			{
				rn->Time_Thresh = rn->Prefs_Buffer.KeyRptDelay;
				rn->Time_Repeat = rn->Prefs_Buffer.KeyRptSpeed;
			}

			RepeatDelay( usbbase, rn, &rn->Time_Thresh );
		}
	}

	if ( leds != rn->Keyboard_LEDs )
	{
		USB_Send_LED( usbbase, rn, rn->Keyboard_LEDs );
	}
}

// --

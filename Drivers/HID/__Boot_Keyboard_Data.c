
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


// --
// Handle Key Repeat

SEC_CODE static void __KeyRepeat( 
	struct USBBase *usbbase, 
	struct HID_ReportNode *rn )
{
S16 repeat;
S16 last;

	TASK_NAME_ENTER( "__KeyRepeat" );

	// Note:
	//  Last - We want to make this one active .. if not clear
	//  repeat - this is the active key
	last	= rn->Boot_Keyboard_Last;
	repeat	= rn->Boot_Keyboard_Repeat;

//	USBDEBUG( "Last: $%04lx, Repeat: $%04lx", (U32) last, (U32) repeat );

	if ( last != repeat )
	{
		// Start or Change Key Repeat
		rn->Boot_Keyboard_Repeat = last;

		// Stop timer, if active
		if ( rn->Timer_Added )
		{
			rn->Timer_Added = FALSE;
			// Stop Timer
			IO_STOP( & rn->Timer_IOReq );
			// Clear Timer signal, avoid triggering a repeat
			TASK_SETSIGNAL( 0, rn->Timer_MsgPort.ump_Signal.sig_Signal_Mask );
		}

		if ( last )
		{
			// We only need the first 20 bytes to get Delay and Repeat times
			if ( MISC_GETINTUIPREFS( & rn->Prefs_Buffer, 20 ))
			{
				rn->Time_Thresh = rn->Prefs_Buffer.KeyRptDelay;
				rn->Time_Repeat = rn->Prefs_Buffer.KeyRptSpeed;
			}

			// Start Timer
			if (( rn->Time_Thresh.Seconds )
			||	( rn->Time_Thresh.Microseconds ))
			{
				rn->Timer_IOReq.Time = rn->Time_Thresh;
				rn->Timer_IOReq.Request.io_Command = TR_ADDREQUEST;
				rn->Timer_Added = TRUE;
				IO_SEND( & rn->Timer_IOReq );
			}
			else
			{
				USBDEBUG( "__KeyRepeat : Time_Thresh time is Zero" );
			}
		}
	}

	TASK_NAME_LEAVE();
}

// --
// Check for Key Press

SEC_CODE static void __KeyPress( 
	struct USBBase *usbbase, 
	struct HID_ReportNode *rn, 
	struct USB2_BootKeyboard *oldreport,
	struct USB2_BootKeyboard *newreport )
{
struct HIDData *hd;
S16 amikey;
S16 usbkey;
U32 newpos;

	TASK_NAME_ENTER( "__Kbd_KeyPress" );

	hd = rn->HIDData;

	for( newpos=0 ; newpos<6 ; newpos++ )
	{
		usbkey = newreport->Keycode[newpos];

		// 0 = SKip, 1 = Roll Over, 2+ = Valid Key
		if ( usbkey < 2 )
		{
			continue;
		}

		// Now check if its a new one
		if (( oldreport->Keycode[0] == usbkey )
		||	( oldreport->Keycode[1] == usbkey )
		||	( oldreport->Keycode[2] == usbkey )
		||	( oldreport->Keycode[3] == usbkey )
		||	( oldreport->Keycode[4] == usbkey )
		||	( oldreport->Keycode[5] == usbkey ))
		{
			// No we have handled this one before
			continue;
		}

		// --
		// not sure this is the best place to handle 'Locks'

		if ( usbkey == 57 )
		{
			// Handle CapsLock... 
			usbbase->usb_HID_Qualifiers ^= IEQUALIFIER_CAPSLOCK;
			rn->Boot_Keyboard_LEDs ^= HID_LED_CapsLock;
			USB_Send_LED( usbbase, hd, rn->ReportID, rn->Boot_Keyboard_LEDs );
		}

		if ( usbkey == 71 )
		{
			// Handle ScrollLock
			rn->Boot_Keyboard_LEDs ^= HID_LED_ScrollLock;
			USB_Send_LED( usbbase, hd, rn->ReportID, rn->Boot_Keyboard_LEDs );
		}

		if ( usbkey == 83 )
		{
			// Handle NumLock
			usbbase->usb_HID_Qualifiers ^= IEQUALIFIER_NUMERICPAD;
			rn->Boot_Keyboard_LEDs ^= HID_LED_NumLock;
			USB_Send_LED( usbbase, hd, rn->ReportID, rn->Boot_Keyboard_LEDs );
		}

		// Conver USB Keycode to Amiga Keycode
		amikey = ( usbbase->usb_HID_Qualifiers & IEQUALIFIER_NUMERICPAD ) ? USBKeyMap1[ usbkey ] : USBKeyMap2[ usbkey ] ;

		// a Nagative, mean key is not mapped
		if ( amikey < 0 )
		{
			continue;
		}

		// Remeber Last key for Repeat
		rn->Boot_Keyboard_Last = usbkey;

		Input_Raw_Key( usbbase, rn, (U16) amikey );
	}

	TASK_NAME_LEAVE();
}

// --
// Check for Key Releases

SEC_CODE static void __KeyRelease( 
	struct USBBase *usbbase, 
	struct HID_ReportNode *rn, 
	struct USB2_BootKeyboard *oldreport,
	struct USB2_BootKeyboard *newreport )
{
U32 oldpos;
S16 amikey;
S16 usbkey;

	TASK_NAME_ENTER( "__KeyRelease" );

	for( oldpos=0 ; oldpos<6 ; oldpos++ )
	{
		usbkey = oldreport->Keycode[oldpos];

		// 0 = Skip, 1 = Roll Over, 2+ = Valid Key
		if ( usbkey < 2 )
		{
			continue;
		}

		// Now check if its a still there
		if (( newreport->Keycode[0] == usbkey )
		||	( newreport->Keycode[1] == usbkey )
		||	( newreport->Keycode[2] == usbkey )
		||	( newreport->Keycode[3] == usbkey )
		||	( newreport->Keycode[4] == usbkey )
		||	( newreport->Keycode[5] == usbkey ))
		{
			// key still pressed
			continue;
		}

		// Check if the released key is the repeating one
		if ( rn->Boot_Keyboard_Repeat == usbkey )
		{
			// Stop current repeat key,
			// another key may start repearting (the last one)
			rn->Boot_Keyboard_Last = 0;
		}

		// Map key press to Amiga raw key code
		amikey = ( usbbase->usb_HID_Qualifiers & IEQUALIFIER_NUMERICPAD ) ? USBKeyMap1[ usbkey ] : USBKeyMap2[ usbkey ] ;

		// a Nagative, mean key is not mapped
		if ( amikey < 0 )
		{
			continue;
		}

		Input_Raw_Key( usbbase, rn, (U16) amikey | IECODE_UP_PREFIX );
	}

	TASK_NAME_LEAVE();
}

// --
// Check for changed Modifier

SEC_CODE static void __Modifiers( struct USBBase *usbbase, struct HID_ReportNode *rn, U8 newmod )
{
U32 pos;
U16 key;
U8 mask1;
U8 mask2;

	TASK_NAME_ENTER( "__Modifiers" );

	pos = 0;

	while( TRUE )
	{
		if ( ! KeyboardData[pos].Mask1 )
		{
			break;
		}

		mask1 = ( KeyboardData[pos].Mask1 & newmod ) ? 1 : 0;
		mask2 = ( KeyboardData[pos].Mask2 & usbbase->usb_HID_Qualifiers ) ? 1 : 0;

		if ( mask1 != mask2 )
		{
			if ( mask1 )
			{
				key = KeyboardData[pos].KeyCode;
				usbbase->usb_HID_Qualifiers |= KeyboardData[pos].Mask2;
			}
			else
			{
				key = KeyboardData[pos].KeyCode | IECODE_UP_PREFIX;
				usbbase->usb_HID_Qualifiers &= ~KeyboardData[pos].Mask2;
			}

			Input_Raw_Key( usbbase, rn, (U16) key );
		}

		pos++;
	}

	TASK_NAME_LEAVE();
}

// --
// Check if Keyboard buffer triggered a rollover

SEC_CODE static S32 __RollOver( struct USB2_BootKeyboard *report )
{
S32 retval;

	TASK_NAME_ENTER( "__RollOver" );

	retval = ( report->Keycode[0] == 0x01 ) ? TRUE : FALSE ;

	TASK_NAME_LEAVE();

	return( retval );
}

// --

SEC_CODE void __Boot_Keyboard_Timer( 
	struct USBBase *usbbase, 
	struct HID_ReportNode *rn )
{
S16 repeat;
S16 amikey;

	TASK_NAME_ENTER( "__Boot_Keyboard_Timer" );

	// 0 = None, 1 = Rollover, 2+ = Good
	repeat = rn->Boot_Keyboard_Repeat;

	// 0 = Skip, 1 = Roll Over, 2+ = Valid Key
	if ( repeat < 2 )
	{
		goto bailout;
	}

	// -1 = Key not Mapped
	amikey = ( usbbase->usb_HID_Qualifiers & IEQUALIFIER_NUMERICPAD ) ? USBKeyMap1[ repeat ] : USBKeyMap2[ repeat ] ;

	// a Nagative, mean key is not mapped
	if ( amikey < 0 )
	{
		goto bailout;
	}

	// Start Repeat Timer
	if (( rn->Time_Repeat.Seconds )
	||	( rn->Time_Repeat.Microseconds ))
	{
		rn->Timer_IOReq.Time = rn->Time_Repeat;
		rn->Timer_IOReq.Request.io_Command = TR_ADDREQUEST;
		rn->Timer_Added = TRUE;
		IO_SEND( & rn->Timer_IOReq );
	}
	else
	{
		USBDEBUG( "__Boot_Keyboard_Timer : Time_Repeat time is Zero" );
	}

	// Insert repeat key

	usbbase->usb_HID_Qualifiers |= IEQUALIFIER_REPEAT;

	Input_Raw_Key( usbbase, rn, (U16) amikey );

	usbbase->usb_HID_Qualifiers &= ~IEQUALIFIER_REPEAT;

bailout:

	TASK_NAME_LEAVE();
}

// --

SEC_CODE void __Boot_Keyboard_Data( 
	struct USBBase *usbbase, 
	struct HID_ReportNode *rn, 
	struct RealRequest *ioreq )
{
struct USB2_BootKeyboard *newreport;
struct USB2_BootKeyboard *oldreport;

	TASK_NAME_ENTER( "__Boot_Keyboard_Data" );

	newreport = (PTR) ioreq->req_Public.io_Data;
	oldreport = & rn->Boot_Keyboard_Data;

	#if 0
	usbbase->usb_IExec->DebugPrintF( "Last ... : $%04lx\n", (U32) rn->Boot_Keyboard_Last );
	usbbase->usb_IExec->DebugPrintF( "Repeat . : $%04lx\n", (U32) rn->Boot_Keyboard_Repeat );
	usbbase->usb_IExec->DebugPrintF( "HIDKBD . : New : %02lx %02lx %02lx %02lx %02lx %02lx\n",
		(U32) newreport->Keycode[0],
		(U32) newreport->Keycode[1],
		(U32) newreport->Keycode[2],
		(U32) newreport->Keycode[3],
		(U32) newreport->Keycode[4],
		(U32) newreport->Keycode[5]
	);

	usbbase->usb_IExec->DebugPrintF( "HIDKBD . : Old : %02lx %02lx %02lx %02lx %02lx %02lx\n",
		(U32) oldreport->Keycode[0],
		(U32) oldreport->Keycode[1],
		(U32) oldreport->Keycode[2],
		(U32) oldreport->Keycode[3],
		(U32) oldreport->Keycode[4],
		(U32) oldreport->Keycode[5]
	);
	#endif

	// -- Check for RollOver

	if ( __RollOver( newreport ))
	{
		USBDEBUG( "__Boot_Keyboard_Data : RollOver" );
		return;
	}

	// -- Process Modifiers

	if ( oldreport->Modifier != newreport->Modifier )
	{
		__Modifiers( usbbase, rn, newreport->Modifier );
	}

	// -- Check for key presses

	__KeyPress( usbbase, rn, oldreport, newreport );	

	// -- Check for key releases

	__KeyRelease( usbbase, rn, oldreport, newreport );	

	// -- Setup key-repeat timer and related state info

	__KeyRepeat( usbbase, rn );

	// -- And now copy Report for next time

	MEM_COPY( newreport, oldreport, sizeof( struct USB2_BootKeyboard ));

	TASK_NAME_LEAVE();
}

// --

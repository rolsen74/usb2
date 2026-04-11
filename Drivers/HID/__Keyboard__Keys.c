
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

SEC_CODE void __Keyboard_Timer( 
	struct USBBase *usbbase, 
	struct HID_ReportNode *rn )
{
S16 repeat;
S16 amikey;

	TASK_NAME_ENTER( "__Keyboard_Timer" );

	// 0 = None, 1 = Rollover, 2+ = Good
	repeat = rn->Keyboard_Repeat;

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
		USBDEBUG( "__Keyboard_Timer : Time_Repeat time is Zero" );
	}

	// Insert repeat key

	usbbase->usb_HID_Qualifiers |= IEQUALIFIER_REPEAT;

	Input_Raw_Key( usbbase, rn, (U16) amikey );

	usbbase->usb_HID_Qualifiers &= ~IEQUALIFIER_REPEAT;

bailout:

	TASK_NAME_LEAVE();
}

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
	last	= rn->Keyboard_Last;
	repeat	= rn->Keyboard_Repeat;

//	USBDEBUG( "Last: $%04lx, Repeat: $%04lx", (U32) last, (U32) repeat );

	if ( last != repeat )
	{
		// Start or Change Key Repeat
		rn->Keyboard_Repeat = last;

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
// Check if Keyboard buffer triggered a rollover

SEC_CODE static S32 __RollOver( U8 *report )
{
S32 retval;

	TASK_NAME_ENTER( "HID_Keyboard : myHID_Kbd_RollOver" );

	retval = ( report[0] == 0x01 ) ? TRUE : FALSE ;

	TASK_NAME_LEAVE();

	return( retval );
}

// --
// Check for Key Press

SEC_CODE static void __KeyPress( 
	struct USBBase *usbbase, 
	struct HID_ReportNode *rn,
	U8 *oldreport,
	U8 *newreport )
{
struct HIDData *hd;
S16 amikey;
S16 usbkey;
U32 newpos;

	TASK_NAME_ENTER( "HID_Keyboard : myHID_Kbd_KeyPress" );

	hd = rn->HIDData;

	for( newpos=0 ; newpos<6 ; newpos++ )
	{
		usbkey = newreport[newpos];

		// 0 = SKip, 1 = Roll Over, 2+ = Valid Key
		if ( usbkey < 2 )
		{
			continue;
		}

		// Now check if its a new one
		if (( oldreport[0] == usbkey )
		||	( oldreport[1] == usbkey )
		||	( oldreport[2] == usbkey )
		||	( oldreport[3] == usbkey )
		||	( oldreport[4] == usbkey )
		||	( oldreport[5] == usbkey ))
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
			rn->Keyboard_LEDs ^= HID_LED_CapsLock;
			USB_Send_LED( usbbase, hd, rn->ReportID, rn->Keyboard_LEDs );
		}

		if ( usbkey == 71 )
		{
			// Handle ScrollLock
			rn->Keyboard_LEDs ^= HID_LED_ScrollLock;
			USB_Send_LED( usbbase, hd, rn->ReportID, rn->Keyboard_LEDs );
		}

		if ( usbkey == 83 )
		{
			// Handle NumLock
			usbbase->usb_HID_Qualifiers ^= IEQUALIFIER_NUMERICPAD;
			rn->Keyboard_LEDs ^= HID_LED_NumLock;
			USB_Send_LED( usbbase, hd, rn->ReportID, rn->Keyboard_LEDs );
		}

		// Conver USB Keycode to Amiga Keycode
		amikey = ( usbbase->usb_HID_Qualifiers & IEQUALIFIER_NUMERICPAD ) ? USBKeyMap1[ usbkey ] : USBKeyMap2[ usbkey ] ;

		// a Nagative, mean key is not mapped
		if ( amikey < 0 )
		{
			continue;
		}

		// Remeber Last key for Repeat
		rn->Keyboard_Last = usbkey;

		Input_Raw_Key( usbbase, rn, (U16) amikey );
	}

	TASK_NAME_LEAVE();
}

// --
// Check for Key Releases

SEC_CODE static void __KeyRelease( 
	struct USBBase *usbbase, 
	struct HID_ReportNode *rn,
	U8 *oldreport,
	U8 *newreport )
{
U32 oldpos;
S16 amikey;
S16 usbkey;

	TASK_NAME_ENTER( "HID_Keyboard : myHID_Kbd_KeyRelease" );

	for( oldpos=0 ; oldpos<6 ; oldpos++ )
	{
		usbkey = oldreport[oldpos];

		// 0 = Skip, 1 = Roll Over, 2+ = Valid Key
		if ( usbkey < 2 )
		{
			continue;
		}

		// Now check if its a still there
		if (( newreport[0] == usbkey )
		||	( newreport[1] == usbkey )
		||	( newreport[2] == usbkey )
		||	( newreport[3] == usbkey )
		||	( newreport[4] == usbkey )
		||	( newreport[5] == usbkey ))
		{
			// key still pressed
			continue;
		}

		// Check if the released key is the repeating one
		if ( rn->Keyboard_Repeat == usbkey )
		{
			// Stop current repeat key,
			// another key may start repearting (the last one)
			rn->Keyboard_Last = 0;
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

SEC_CODE void __Keyboard_Keys( 
	struct USBBase *usbbase,
	struct HID_ReportNode *rn,
	struct RealRequest *ioreq,
	struct HID_BitNode *bn,
	U32 idx,
	U32 id UNUSED )
{
U32 cnt;
U8 newdata[6];
// S32 sss[6];

	TASK_NAME_ENTER( "__Keyboard_Keys" );

	for( cnt=0 ; cnt<6 ; cnt++ )
	{
//		sss[cnt] = idx + ( cnt * bn->BitSize );
		newdata[cnt] = Report_Read_Value( usbbase, ioreq, bn, idx + ( cnt * bn->BitSize ), FALSE );
	}

	#if 0

	usbbase->usb_IExec->DebugPrintF( "HIDKBD . : Pos : %02ld %02ld %02ld %02ld %02ld %02ld\n",
		sss[0],
		sss[1],
		sss[2],
		sss[3],
		sss[4],
		sss[5]
	);

	usbbase->usb_IExec->DebugPrintF( "HIDKBD . : New : %02lx %02lx %02lx %02lx %02lx %02lx\n",
		(U32) newdata[0],
		(U32) newdata[1],
		(U32) newdata[2],
		(U32) newdata[3],
		(U32) newdata[4],
		(U32) newdata[5]
	);

	usbbase->usb_IExec->DebugPrintF( "HIDKBD . : Old : %02lx %02lx %02lx %02lx %02lx %02lx\n",
		(U32) rn->Keyboard_Data[0],
		(U32) rn->Keyboard_Data[1],
		(U32) rn->Keyboard_Data[2],
		(U32) rn->Keyboard_Data[3],
		(U32) rn->Keyboard_Data[4],
		(U32) rn->Keyboard_Data[5]
	);

	#endif

	// -- Check for RollOver

	if ( __RollOver( newdata ))
	{
		USBDEBUG( "__Keyboard_Keys : RollOver" );
		return;
	}

	// -- Check for key presses

	__KeyPress( usbbase, rn, rn->Keyboard_Data, newdata );	

	// -- Check for key releases

	__KeyRelease( usbbase, rn, rn->Keyboard_Data, newdata );	

	// -- Setup key-repeat timer and related state info

	__KeyRepeat( usbbase, rn );

	// -- And now copy Report for next time

	MEM_COPY( newdata, rn->Keyboard_Data, 6 );

	TASK_NAME_LEAVE();
}

// --

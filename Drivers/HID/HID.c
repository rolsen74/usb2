
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

#define USB2_KEYMAP_STORAGE SEC_RODATA
#include "KeyboardMap.h"
#undef USB2_KEYMAP_STORAGE

// --

SEC_CODE U32 HID_Entry( struct USBBase *usbbase, struct USB2_DriverMessage *msg )
{
struct HIDData *hd;
U32 retval;
S32 promoted;

	// --

	TASK_NAME_ENTER( "HID_Entry" );

	struct RealFunctionNode *fn UNUSED = (PTR) msg->Function;

	#ifdef DO_DEBUG
	U32 startadr = fn->fkt_Address;
	usbbase->usb_IExec->DebugPrintF( "\n##\n## HID_Entry : Adr #%lu : Enter\n##\n\n", fn->fkt_Address );
	#endif

	// --

	retval = TASK_Return_Error;

	#ifdef DO_INFO
	struct ExecIFace *IExec = usbbase->usb_IExec;
	IExec->Disable();
	IExec->DebugPrintF( "\nStartup new HID\n" );
	IExec->DebugPrintF( "HCD Index   : %ld\n", fn->fkt_HCD->hn_HCDIndex );
	IExec->DebugPrintF( "Tier        : %ld\n", fn->fkt_Tier );
	IExec->DebugPrintF( "Addr Nr     : %ld\n", fn->fkt_Address );
	IExec->DebugPrintF( "Port Nr     : %ld\n\n", fn->fkt_PortNr );
	IExec->Enable();
	#endif

	// --

	hd = MEM_ALLOCVEC( sizeof( struct HIDData ), TRUE );

	if ( hd )
	{
		#ifdef DO_DEBUG
		hd->StructID = ID_IN_HID;
		#endif
		hd->StartMsg = msg;

		if ( HID_Init( usbbase, hd ))
		{
			retval = HID_Main( usbbase, hd, &promoted );

			if ( promoted )
			{
				TASK_NAME_LEAVE();

				return ( retval );
			}
		}
		else
		{
			USBDEBUG( "HID_Entry : Error init failed" );
		}

		HID_Free( usbbase, hd );
		MEM_FREEVEC( hd );
	}
	else
	{
		USBDEBUG( "HID_Entry : Error allocating memory" );
	}

	// --

	#ifdef DO_DEBUG
	usbbase->usb_IExec->DebugPrintF( "\n##\n## HID_Entry : Adr #%lu (%lu) : FN    %p : Locks %2ld : Leave\n##\n\n", fn->fkt_Address, startadr, fn, fn->fkt_Locks );
	#endif

	TASK_NAME_LEAVE();

	return( retval );
}

// --

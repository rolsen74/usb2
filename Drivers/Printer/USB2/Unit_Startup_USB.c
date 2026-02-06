
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32 Unit_Startup_USB( struct PrinterUnit *unit )
{
S32 retval;
S32 stat;

	USBDEBUG( "Printer : Unit_Startup_USB : Enter" );

	retval = FALSE;

	// --
	// Small speedup.. only check if IUSB2 is null

	if ( ! IUSB2 )
	{
		// --
		// Check if usb2 is running

		if ( ! FindName( & ((struct ExecBase *)SysBase)->DeviceList, "usb2.device" ))
		{
			USBERROR( "Printer : Error USB2 stack not running" );

			if ( ! unit->unit_NotRunning )
			{
				// Only print one pr Opener
				unit->unit_NotRunning = TRUE;	

				TimedDosRequesterTags( 
					TDR_TitleString, DEVNAME,
					TDR_GadgetString, "_Okay",
					TDR_ImageType, TDRIMAGE_ERROR,
					TDR_FormatString, "Error USB2 stack\nnot running",
					TAG_END
				);
			}
			goto bailout;
		}

		if ( ! USB2MsgPort )
		{
			// We do not alloc a signal,
			// IOReq much NOT be used data transfer
			USB2MsgPort = AllocSysObjectTags( ASOT_PORT,
				ASOPORT_AllocSig, FALSE,
				TAG_END
			);

			if ( ! USB2MsgPort )
			{
				USBERROR( "Printer : Error creating USB2 MsgPort" );
				goto bailout;
			}

			USBERROR( "MsgPort sig %ld", USB2MsgPort->mp_SigBit );
		}
		// --
		// Check IOReq

		if ( ! USB2IOReq )
		{
			USB2IOReq = AllocSysObjectTags( ASOT_IOREQUEST,
				ASOIOR_Size, sizeof( struct IORequest ),
				ASOIOR_ReplyPort, USB2MsgPort,
				TAG_END
			);

			if ( ! USB2IOReq )
			{
				USBERROR( "Printer : Error creating USB2 IORequest" );
				goto bailout;
			}
		}

		// --
		// Check Open?

		if ( ! USB2Base )
		{
			stat = OpenDevice( "usb2.device", 0, USB2IOReq, 0 );

			if ( stat )
			{
				USBERROR( "Printer : Error opening USB2 Device" );
				goto bailout;
			}

			USB2Base = (PTR) USB2IOReq->io_Device;
		}

		// --
		// Check Interface

//		if ( ! IUSB2 )
		{
			IUSB2 = (PTR) GetInterface( (PTR) USB2Base, "main", 1, NULL );

			if ( ! IUSB2 )
			{
				USBERROR( "Printer : Error getting USB2 Interface" );
				goto bailout;
			}
		}
	}

	// --

	retval = TRUE;

bailout:

	USBDEBUG( "Printer : Unit_Startup_USB : Leave : Retval %ld", retval );

	return( retval );
}

/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

#include "MSD.h"

// --

SEC_CODE U32 MSD_Entry( struct USBBase *usbbase, struct USB2_DriverMessage *msg )
{
struct MSDDevice *msddev;
U32 retval;

	// --

	TASK_NAME_ENTER( "MSD_Entry" );

	retval = TASK_Return_Error;

	// --

	#if 0
{
	struct ExecIFace *IExec = usbbase->usb_IExec;
	struct RealFunctionNode *fn = (PTR) msg->Function;

	IExec->Disable();
	IExec->DebugPrintF( "\n##\n## MSD_Entry : Adr #%lu : Enter\n##\n\n", fn->fkt_Address );
	IExec->DebugPrintF( "HCD Index   : %ld\n", fn->fkt_HCD->hn_HCDIndex );
	IExec->DebugPrintF( "Tier        : %ld\n", fn->fkt_Tier );
	IExec->DebugPrintF( "Addr Nr     : %ld\n", fn->fkt_Address );
	IExec->DebugPrintF( "Port Nr     : %ld\n\n", fn->fkt_PortNr );
	IExec->Enable();

}
	#endif

	// --
	if ( ! MSD_CreateDevice( usbbase ) )
	{
		USBERROR( "MSD : Could not initialize mounter or usb2disk.device" );
		goto bailout;
	}

	// --

	msddev = MSD_Device_Alloc( usbbase, msg );

	if ( msddev )
	{
		MSD_Main( usbbase, msddev );
	}

	MSD_Device_Free( usbbase, msddev );

	// --

bailout:

	TASK_NAME_LEAVE();
	return( retval );
}

// --


/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static void ROMExit( struct USBBase *usbbase, STR file UNUSED )
{
struct ExecIFace *IExec;
PTR node;

	USBINFO( "Freeing Resources" );

	if ( ! usbbase )
	{
		return;
	}

	IExec = usbbase->usb_IExec;

	// --

	IExec->DebugPrintF( "ROMExit :: 1 :: (%s)\n", (file)?file:"<NULL>" );

	HCD_CONTROLLERS_STOP();

	//	HCD_FREE_CONTROLLERS

	IExec->DebugPrintF( "ROMExit :: 2 ::\n" );

	ASYNC_FREE( & usbbase->usb_HCDASync );

	IExec->DebugPrintF( "ROMExit :: 3 ::\n" );

	MASTER_SHUTDOWN();

	// -- Free OpenTimer() resources

	if ( usbbase->usb_TimeRequest )
	{
		IExec->DebugPrintF( "ROMExit :: 4 ::\n" );

		IExec->FreeSysObject( ASOT_IOREQUEST, usbbase->usb_TimeRequest );
		usbbase->usb_TimeRequest = NULL;
	}

	// --

	IExec->DebugPrintF( "ROMExit :: 5 ::\n" );

	while( TRUE )
	{
		node = NODE_REMHEAD( & usbbase->usb_FDriver_Header );

		if ( ! node )
		{
			break;
		}

		IExec->DebugPrintF( "ROMExit :: 8 ::\n" );

		FDRIVER_FREE( node );
	}

	// --

	IExec->DebugPrintF( "ROMExit :: 9 ::\n" );

	MEMORY_CLEANUP();

	// -- Close Libraries

	IExec->DebugPrintF( "ROMExit :: 10 ::\n" );

	#ifndef DO_DEMO_MODE

	if ( usbbase->usb_IDOS )
	{
		IExec->DropInterface( (PTR) usbbase->usb_IDOS );
		usbbase->usb_IDOS = NULL;
	}

	if ( usbbase->usb_DOSBase )
	{
		IExec->CloseLibrary( (PTR) usbbase->usb_DOSBase );
		usbbase->usb_DOSBase = NULL;
	}

	#endif

	// --

	if ( usbbase->usb_IUtility )
	{
		IExec->DropInterface( (PTR) usbbase->usb_IUtility );
		usbbase->usb_IUtility = NULL;
	}

	if ( usbbase->usb_UtilityBase )
	{
		IExec->CloseLibrary( (PTR) usbbase->usb_UtilityBase );
		usbbase->usb_UtilityBase = NULL;
	}

	// --

	if ( usbbase->usb_IExpansion )
	{
		IExec->DropInterface( (PTR) usbbase->usb_IExpansion );
		usbbase->usb_IExpansion = NULL;
	}

	if ( usbbase->usb_ExpansionBase )
	{
		IExec->CloseLibrary( (PTR) usbbase->usb_ExpansionBase );
		usbbase->usb_ExpansionBase = NULL;
	}

	// --

	if ( usbbase->usb_IPCI )
	{
		IExec->DropInterface( (PTR) usbbase->usb_IPCI );
		usbbase->usb_IPCI = NULL;
	}

	if ( usbbase->usb_IMMU )
	{
		IExec->DropInterface( (PTR) usbbase->usb_IMMU );
		usbbase->usb_IMMU = NULL;
	}

	// --

	if ( usbbase->usb_IInput )
	{
		IExec->DropInterface( (PTR) usbbase->usb_IInput );
		usbbase->usb_IInput = NULL;
	}

	if ( usbbase->usb_InputBase )
	{
		IExec->CloseDevice( (PTR) usbbase->usb_InputIORequest );
		usbbase->usb_InputBase = NULL;
	}

	if ( usbbase->usb_InputIORequest )
	{
		IExec->FreeSysObject( ASOT_IOREQUEST, usbbase->usb_InputIORequest );
		usbbase->usb_InputIORequest = NULL;
	}

	// --

	if ( usbbase->usb_ITimer )
	{
		IExec->DropInterface( (PTR) usbbase->usb_ITimer );
		usbbase->usb_ITimer = NULL;
	}

	if ( usbbase->usb_TimerBase )
	{
		// We did not open it
		usbbase->usb_TimerBase = NULL;
	}

	// --

	IExec->DebugPrintF( "Freeing Resources done\n" );
}

// --

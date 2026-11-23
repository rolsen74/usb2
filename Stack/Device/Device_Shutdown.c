
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static void
ROMExit ( struct USBBase * usbbase, STR file UNUSED )
{
	struct ExecIFace * IExec;
	PTR				   node;

	if ( ! usbbase )
	{
		return;
	}

	USBINFO ( "Freeing Resources" );

	IExec = usbbase->usb_IExec;

	USB2_FullBoot_Complete ( usbbase, FALSE );

	if ( usbbase->usb_HCDASync.ua_StructID == ID_USB2_ASYNC )
	{
		HCD_CONTROLLERS_STOP ();

		ASYNC_FREE ( &usbbase->usb_HCDASync );
	}

	MASTER_SHUTDOWN ();

	if ( usbbase->usb_Master_Intern )
	{
		MEM_FREEVEC ( usbbase->usb_Master_Intern );

		usbbase->usb_Master_Intern = NULL;
	}

	if ( usbbase->usb_DriverDirLock )
	{
		usbbase->usb_IDOS->UnLock ( usbbase->usb_DriverDirLock );

		usbbase->usb_DriverDirLock = 0;
	}

	// --

	while ( TRUE )
	{
		node = Header_Head ( &usbbase->usb_FDriver_Header );

		if ( ! node )
		{
			break;
		}

		NODE_REMNODE ( &usbbase->usb_FDriver_Header, node );

		FDRIVER_FREE ( node );
	}

	// --

	MEMORY_CLEANUP ();

	// -- Close Libraries

	if ( usbbase->usb_IMounterPriv )
	{
		IExec->DropInterface ( (PTR)usbbase->usb_IMounterPriv );
	}

	if ( usbbase->usb_IMounter )
	{
		IExec->DropInterface ( (PTR)usbbase->usb_IMounter );
	}

	if ( usbbase->usb_MounterBase )
	{
		IExec->CloseLibrary ( (PTR)usbbase->usb_MounterBase );
	}

	if ( usbbase->usb_IIntuition )
	{
		IExec->DropInterface ( (PTR)usbbase->usb_IIntuition );
	}

	if ( usbbase->usb_IntuitionBase )
	{
		IExec->CloseLibrary ( (PTR)usbbase->usb_IntuitionBase );
	}

	if ( usbbase->usb_IUSB2 )
	{
		IExec->DropInterface ( (PTR)usbbase->usb_IUSB2 );
	}

	if ( usbbase->usb_IDOS )
	{
		IExec->DropInterface ( (PTR)usbbase->usb_IDOS );
		usbbase->usb_IDOS = NULL;
	}

	if ( usbbase->usb_DOSBase )
	{
		IExec->CloseLibrary ( (PTR)usbbase->usb_DOSBase );
		usbbase->usb_DOSBase = NULL;
	}

	// --

	if ( usbbase->usb_IUtility )
	{
		IExec->DropInterface ( (PTR)usbbase->usb_IUtility );
		usbbase->usb_IUtility = NULL;
	}

	if ( usbbase->usb_UtilityBase )
	{
		IExec->CloseLibrary ( (PTR)usbbase->usb_UtilityBase );
		usbbase->usb_UtilityBase = NULL;
	}

	// --

	if ( usbbase->usb_IExpansion )
	{
		IExec->DropInterface ( (PTR)usbbase->usb_IExpansion );
		usbbase->usb_IExpansion = NULL;
	}

	if ( usbbase->usb_ExpansionBase )
	{
		IExec->CloseLibrary ( (PTR)usbbase->usb_ExpansionBase );
		usbbase->usb_ExpansionBase = NULL;
	}

	// --

	if ( usbbase->usb_IPCI )
	{
		IExec->DropInterface ( (PTR)usbbase->usb_IPCI );
		usbbase->usb_IPCI = NULL;
	}

	if ( usbbase->usb_IMMU )
	{
		IExec->DropInterface ( (PTR)usbbase->usb_IMMU );
		usbbase->usb_IMMU = NULL;
	}

	// --

	if ( usbbase->usb_IInput )
	{
		IExec->DropInterface ( (PTR)usbbase->usb_IInput );
		usbbase->usb_IInput = NULL;
	}

	if ( usbbase->usb_InputBase )
	{
		IExec->CloseDevice ( (PTR)usbbase->usb_InputIORequest );
		usbbase->usb_InputBase = NULL;
	}

	if ( usbbase->usb_InputIORequest )
	{
		IExec->FreeSysObject ( ASOT_IOREQUEST, usbbase->usb_InputIORequest );
		usbbase->usb_InputIORequest = NULL;
	}

	// --

	if ( usbbase->usb_ITimer )
	{
		IExec->DropInterface ( (PTR)usbbase->usb_ITimer );
		usbbase->usb_ITimer = NULL;
	}

	if ( usbbase->usb_TimeRequest )
	{
		IExec->CloseDevice ( (PTR)usbbase->usb_TimeRequest );

		IExec->FreeSysObject ( ASOT_IOREQUEST, usbbase->usb_TimeRequest );

		usbbase->usb_TimeRequest = NULL;
	}

	if ( usbbase->usb_TimerBase )
	{
		// We did not open it
		usbbase->usb_TimerBase = NULL;
	}

	// --

	return;
}

// --

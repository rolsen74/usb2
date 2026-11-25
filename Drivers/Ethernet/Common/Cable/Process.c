
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "EthClass.h"

// --

extern const struct USB2EthOps EthClass_Ops;

// --

U32
EthClass_Entry( struct USB2DriverIFace *Self UNUSED, struct USB2_DriverMessage *msg )
{
	struct devUnit *unit;
	struct EthClassPort port;

	S32 registered;

	if ( ( ! msg ) || ( ! msg->IUSB2 ) || ( ! msg->Interface ) )
	{
		return ( FALSE );
	}

	IExec->DebugPrintF( WIRED_NAME ": probing %04lX:%04lX interface %lu\n",
		(U32) msg->VendorID, (U32) msg->DeviceID, (U32) msg->Interface->IFC_Num );

	if ( msg->Interface->IFC_Num != 0 )
	{
		IExec->DebugPrintF( WIRED_NAME ": interface %lu skipped, only interface 0 is supported\n",
			(U32) msg->Interface->IFC_Num );

		return ( FALSE );
	}

	unit = IExec->AllocVecTags( sizeof( *unit ), AVT_Clear, 0, TAG_END );

	if ( ! unit )
	{
		return ( FALSE );
	}

	IExec->InitSemaphore( & unit->unit_DispatchLock );

	IExec->NewList( & unit->unit_Requests );

	unit->unit_IUSB2 = msg->IUSB2;
	unit->unit_USBInterface = msg->Interface;
	registered = FALSE;

	memset( & port, 0, sizeof( port ) );

	if ( unit_Process_Init( unit, msg ) )
	{
		unit->unit_Running = TRUE;
		unit->unit_AcceptIO = TRUE;

		registered = EthClass_Register( & port, & EthClass_Ops, unit,
			& unit->unit_UnitNumber, WIRED_ADAPTER, msg );

		if ( registered )
		{
			unit_Process_MainLoop( unit );
		}
	}

	IExec->ObtainSemaphore( & unit->unit_DispatchLock );

	unit->unit_AcceptIO = FALSE;

	IExec->ReleaseSemaphore( & unit->unit_DispatchLock );

	unit_Process_Free( unit );

	if ( registered )
	{
		EthClass_Unregister( & port );
	}
	else
	{
		IExec->FreeVec( unit );
	}

	return ( registered );
}

// --

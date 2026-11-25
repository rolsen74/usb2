
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
	struct EthernetUnit *unit;
	struct EthClassPort port;

	S32 registered;

	if ( ( ! msg ) || ( ! msg->IUSB2 ) || ( ! msg->Interface ) ||
		( msg->Interface->IFC_Num != 0 ) )
	{
		return ( FALSE );
	}

	unit = Unit_Alloc();

	if ( ! unit )
	{
		return ( FALSE );
	}

	IUSB2 = msg->IUSB2;

	unit->unit_TaskAdr = FindTask( NULL );

	memset( & port, 0, sizeof( port ) );

	registered = FALSE;

	if ( Process_Init( unit, msg ) )
	{
		registered = EthClass_Register( & port, & EthClass_Ops, unit,
			& unit->unit_UnitNr, ETH_CLASS_ADAPTER, msg );

		if ( registered )
		{
			Process_Main( unit );
		}
	}

	Process_Free( unit );

	if ( registered )
	{
		DebugPrintF( "USB2 Ethernet: %s detached from unit %lu\n", DEVNAME, unit->unit_UnitNr );

		EthClass_Unregister( & port );
	}
	else
	{
		Unit_Free( unit );
	}

	return ( registered );
}

// --

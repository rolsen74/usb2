
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef __USE_INLINE__
#define __USE_INLINE__
#endif

#include "All.h"
#include "Version.h"
#include "EthClass.h"

// --

S32
EthClass_Register( struct EthClassPort *port, const struct USB2EthOps *ops, PTR adapter,
	ULONG *number, ULONG type, const struct USB2_DriverMessage *msg )
{
	const char *stage;

	stage = "open " USB2_ETH_DEVICE;
	port->Request.io_Message.mn_Length = sizeof( port->Request );

	if ( OpenDevice( USB2_ETH_DEVICE, USB2_ETH_BACKEND_UNIT, & port->Request, 0 ) )
	{
		port->Request.io_Device = NULL;

		goto bailout;
	}

	stage = "obtain the adapter registration interface";

	if ( port->Request.io_Device->dd_Library.lib_Revision >= USB2ETH_ADAPTER_REVISION )
	{
		port->Interface = (PTR) GetInterface( (PTR) port->Request.io_Device, "main", 1, NULL );
	}

	if ( ! port->Interface )
	{
		goto bailout;
	}

	stage = "register the Ethernet unit";

	port->Unit = port->Interface->RegisterAdapter( ops, adapter, number, type );

	if ( port->Unit )
	{
		DebugPrintF( "USB2 Ethernet: %s %04lX:%04lX registered " USB2_ETH_DEVICE " unit %lu\n",
			DEVNAME, (ULONG) msg->VendorID, (ULONG) msg->DeviceID, *number );

		return ( TRUE );
	}

bailout:

	DebugPrintF( "USB2 Ethernet: %s %04lX:%04lX could not %s\n",
		DEVNAME, (ULONG) msg->VendorID, (ULONG) msg->DeviceID, stage );

	EthClass_Unregister( port );

	return ( FALSE );
}

// --

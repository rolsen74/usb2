
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <proto/exec.h>
#include "BluetoothTest.h"

// --

LONG
L2CAPEcho( struct IORequest *request, ULONG unit, const char *address )
{
	struct Bluetooth2IFace *l2cap;
	struct BTHCIError error;
	enum BTHCIResult result;

	uint8_t peer[6];

	if ( ! ParseAddress( address, peer ) )
	{
		Printf( "ECHO requires a Bluetooth address such as 01:23:45:67:89:AB.\n" );

		return ( RETURN_FAIL );
	}

	l2cap = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_INTERFACE,
		BLUETOOTH2_INTERFACE_VERSION, NULL );

	if ( ! l2cap )
	{
		Printf( "This operation requires bluetooth2.device main interface version %lu.\n",
			(ULONG) BLUETOOTH2_INTERFACE_VERSION );

		return ( RETURN_FAIL );
	}

	Printf( "Sending an L2CAP echo to %s from unit %lu.\n", address, unit );

	result = l2cap->l2cap_EchoController( unit, peer, & error );

	DropInterface( (APTR) l2cap );

	switch( result )
	{
		case BT_HCI_OK:
			Printf( "L2CAP echo reply received from %s.\n", address );

			return ( RETURN_OK );

		case BT_HCI_NO_CONTROLLER:
			Printf( "No Bluetooth controller available for unit %lu.\n", unit );

			break;

		case BT_HCI_NO_MEMORY:
			Printf( "Could not allocate a Bluetooth echo reply port.\n" );

			break;

		case BT_HCI_UNSUPPORTED:
			Printf( "ECHO requires cls_Bluetooth.Driver 1.6 and controller support for ACL data.\n" );

			break;

		case BT_ACL_READ_ERROR:
			if ( error.TransportError == BT_HCI_TRANSPORT_TIMEOUT )
			{
				Printf( "The Classic link opened but the L2CAP echo reply timed out.\n" );

				break;
			}

			ReportHCIError( result, & error );

			break;

		default:
			ReportHCIError( result, & error );

			break;
	}

	return ( RETURN_FAIL );
}

// --

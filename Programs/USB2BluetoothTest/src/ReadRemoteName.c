
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
ReadRemoteName( struct IORequest *request, ULONG unit, const char *address )
{
	struct Bluetooth2IFace *names;
	struct BTRemoteName remote;
	struct BTHCIError error;
	enum BTHCIResult result;

	ULONG byte;
	uint8_t character;

	if ( ! ParseAddress( address, remote.Address ) )
	{
		Printf( "NAME requires a Bluetooth address such as 01:23:45:67:89:AB.\n" );

		return ( RETURN_FAIL );
	}

	names = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_INTERFACE,
		BLUETOOTH2_INTERFACE_VERSION, NULL );

	if ( ! names )
	{
		Printf( "This operation requires bluetooth2.device main interface version %lu.\n",
			(ULONG) BLUETOOTH2_INTERFACE_VERSION );

		return ( RETURN_FAIL );
	}

	Printf( "Looking up the Classic Bluetooth name for %s on unit %lu.\n", address, unit );

	result = names->names_ReadName( unit, & remote, & error );

	DropInterface( (APTR) names );

	switch( result )
	{
		case BT_HCI_OK:
			// Remote names can contain terminal control bytes.
			for( byte = 0; remote.Name[byte]; byte++ )
			{
				character = remote.Name[byte];

				if ( ( character < 32 ) || ( character > 126 ) )
				{
					remote.Name[byte] = '?';
				}
			}

			Printf( "%s  %s\n", address, remote.Name[0] ? remote.Name : "(no name provided)" );

			return ( RETURN_OK );

		case BT_HCI_NO_CONTROLLER:
			Printf( "No Bluetooth controller available for unit %lu.\n", unit );

			break;

		case BT_HCI_NO_MEMORY:
			Printf( "Could not allocate a Bluetooth name lookup reply port.\n" );

			break;

		case BT_HCI_UNSUPPORTED:
			Printf( "NAME requires cls_Bluetooth.Driver 1.4 or later.\n" );

			break;

		default:
			ReportHCIError( result, & error );

			break;
	}

	return ( RETURN_FAIL );
}

// --

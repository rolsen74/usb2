
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <string.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "BluetoothTest.h"

// --

static void
PrintService( struct BTServiceRecord *record )
{
	ULONG byte;
	uint8_t character;

	Printf( "$%08lx  ", (ULONG) record->Handle );

	if ( record->UUIDLength )
	{
		Printf( "UUID $" );

		for( byte = 0; byte < record->UUIDLength; byte++ )
		{
			Printf( "%02lx", (ULONG) record->UUID[byte] );
		}
	}

	if ( record->RFCOMMChannel )
	{
		Printf( "  RFCOMM channel %lu", (ULONG) record->RFCOMMChannel );
	}

	for( byte = 0; record->Name[byte]; byte++ )
	{
		character = record->Name[byte];

		if ( ( character < 32 ) || ( character > 126 ) )
		{
			record->Name[byte] = '?';
		}
	}

	Printf( "  %s\n", record->Name );
}

// --

LONG
BrowseServices( struct IORequest *request, ULONG unit, const char *address )
{
	struct Bluetooth2IFace *sdp;
	struct BTServiceQuery query;
	struct BTServiceRecord records[32];
	struct BTHCIError error;
	enum BTHCIResult result;

	ULONG index;

	if ( ! ParseAddress( address, query.Address ) )
	{
		Printf( "SERVICES requires a Bluetooth address such as 01:23:45:67:89:AB.\n" );

		return ( RETURN_FAIL );
	}

	sdp = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_INTERFACE,
		BLUETOOTH2_INTERFACE_VERSION, NULL );

	if ( ! sdp )
	{
		Printf( "This operation requires bluetooth2.device main interface version %lu.\n",
			(ULONG) BLUETOOTH2_INTERFACE_VERSION );

		return ( RETURN_FAIL );
	}

	memset( records, 0, sizeof( records ) );

	query.Records = records;
	query.Capacity = sizeof( records ) / sizeof( records[0] );

	Printf( "Browsing Classic Bluetooth services on %s from unit %lu.\n", address, unit );

	result = sdp->sdp_BrowseController( unit, & query, & error );

	DropInterface( (APTR) sdp );

	switch( result )
	{
		case BT_HCI_OK:
			for( index = 0; index < query.Count; index++ )
			{
				PrintService( & records[index] );
			}

			Printf( "%lu public service records%s.\n", (ULONG) query.Count, query.Truncated ? " (output limit reached)" : "" );

			return ( RETURN_OK );

		case BT_HCI_NO_CONTROLLER:
			Printf( "No Bluetooth controller available for unit %lu.\n", unit );

			break;

		case BT_HCI_NO_MEMORY:
			Printf( "Could not allocate a Bluetooth service discovery reply port.\n" );

			break;

		case BT_HCI_UNSUPPORTED:
			Printf( "Service discovery requires cls_Bluetooth.Driver 1.7 and a Classic Basic Mode L2CAP channel.\n" );

			break;

		case BT_SDP_ERROR:
			Printf( "The peer returned SDP error $%04lx.\n", (ULONG) query.Status );

			break;

		default:
			ReportHCIError( result, & error );

			break;
	}

	return ( RETURN_FAIL );
}

// --

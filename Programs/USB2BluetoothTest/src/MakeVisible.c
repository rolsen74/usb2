
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

LONG
MakeVisible( struct IORequest *request, ULONG unit, const char *name, LONG seconds )
{
	struct Bluetooth2VisibilityIFace *interface;
	struct BTVisibility visibility;
	struct BTHCIError error;
	enum BTHCIResult result;

	ULONG length;

	length = strlen( name );

	if ( ( ! length ) || ( length > BT_REMOTE_NAME_MAX ) )
	{
		Printf( "AS requires a name of 1 to %lu bytes.\n", (ULONG) BT_REMOTE_NAME_MAX );

		return ( RETURN_FAIL );
	}

	if ( ( seconds < 1 ) || ( seconds > BT_VISIBLE_MAX_SECONDS ) )
	{
		Printf( "SECONDS must be 1 to %lu.\n", (ULONG) BT_VISIBLE_MAX_SECONDS );

		return ( RETURN_FAIL );
	}

	interface = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_VISIBILITY_INTERFACE,
		BLUETOOTH2_VISIBILITY_VERSION, NULL );

	if ( ! interface )
	{
		Printf( "VISIBLE requires Bluetooth2 1.7 or later.\n" );

		return ( RETURN_FAIL );
	}

	visibility.Seconds = seconds;

	memset( visibility.Name, 0, sizeof( visibility.Name ) );

	memcpy( visibility.Name, name, length );

	Printf( "Making unit %lu discoverable as %s for %ld seconds.\n", unit, name, seconds );

	result = interface->MakeVisible( unit, & visibility, & error );

	DropInterface( (APTR) interface );

	switch( result )
	{
		case BT_HCI_OK:
			Printf( "Discoverable period ended.\n" );

			return ( RETURN_OK );

		case BT_HCI_NO_CONTROLLER:
			Printf( "No Bluetooth controller available for unit %lu.\n", unit );

			break;

		case BT_HCI_UNSUPPORTED:
			Printf( "VISIBLE requires cls_Bluetooth.Driver 1.5 or later.\n" );

			break;

		case BT_HCI_NO_MEMORY:
		case BT_HCI_TIMER_ERROR:
			Printf( "Could not open the resources for timed discovery.\n" );

			break;

		default:
			ReportHCIError( result, & error );

			break;
	}

	return ( RETURN_FAIL );
}

// --

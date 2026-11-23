
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

#define LE_SCAN_CAPACITY 64

// --

static void
PrintDevice( struct BTLEDevice *device )
{
	static const char *const types[] = { "public", "random", "public identity", "random identity" };
	static const char hex[] = "0123456789abcdef";
	const char *type;
	char name[4 * BT_LE_NAME_MAX + 1];
	ULONG used;
	ULONG byte;
	UBYTE ch;

	type = device->AddressType < sizeof( types ) / sizeof( types[0] ) ? types[device->AddressType] : "unknown";

	Printf( "%02lx:%02lx:%02lx:%02lx:%02lx:%02lx (%s)",
		(ULONG) device->Address[0], (ULONG) device->Address[1], (ULONG) device->Address[2],
		(ULONG) device->Address[3], (ULONG) device->Address[4], (ULONG) device->Address[5], type );

	if ( device->RSSI != BT_LE_RSSI_UNKNOWN )
	{
		Printf( ", %ld dBm", (LONG) device->RSSI );
	}

	if ( device->Name[0] )
	{
		used = 0;

		for( byte = 0; device->Name[byte]; byte++ )
		{
			ch = device->Name[byte];

			if ( ( ch < 0x20 ) || ( ch >= 0x7f ) || ( ch == '"' ) || ( ch == '\\' ) )
			{
				name[used++] = '\\';
				name[used++] = 'x';
				name[used++] = hex[ch >> 4];
				name[used++] = hex[ch & 15];
			}
			else
			{
				name[used++] = ch;
			}
		}

		name[used] = 0;

		Printf( ", \"%s\"", name );
	}

	Printf( "\n" );
}

// --

LONG
RunLE( struct IORequest *request, ULONG unit, enum BTLEMode mode, const char *name, LONG seconds )
{
	struct Bluetooth2LEIFace *interface;
	struct BTLERequest operation;
	struct BTHCIError error;
	enum BTHCIResult result;

	ULONG length;
	ULONG index;

	if ( ( seconds < 1 ) || ( seconds > BT_LE_MAX_SECONDS ) )
	{
		Printf( "SECONDS must be 1 to %lu.\n", (ULONG) BT_LE_MAX_SECONDS );

		return ( RETURN_FAIL );
	}

	memset( & operation, 0, sizeof( operation ) );

	operation.Mode = mode;
	operation.Seconds = seconds;

	if ( mode == BT_LE_ADVERTISE )
	{
		length = strlen( name );

		if ( ( ! length ) || ( length > BT_LE_NAME_MAX ) )
		{
			Printf( "AS requires a name of 1 to %lu bytes.\n", (ULONG) BT_LE_NAME_MAX );

			return ( RETURN_FAIL );
		}

		memcpy( operation.Name, name, length );
	}

	interface = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_LE_INTERFACE, BLUETOOTH2_LE_VERSION, NULL );

	if ( ! interface )
	{
		Printf( "LE operations require Bluetooth2 1.15 or later.\n" );

		return ( RETURN_FAIL );
	}

	if ( mode == BT_LE_ADVERTISE )
	{
		Printf( "Advertising unit %lu as %s for %ld seconds. Ctrl-C stops advertising.\n", unit, name, seconds );
	}
	else
	{
		operation.Capacity = LE_SCAN_CAPACITY;

		operation.Devices = AllocVecTags( sizeof( *operation.Devices ) * operation.Capacity, TAG_END );

		if ( ! operation.Devices )
		{
			DropInterface( (APTR) interface );

			Printf( "Could not allocate the LE scan results.\n" );

			return ( RETURN_FAIL );
		}

		Printf( "Scanning for LE advertisements on unit %lu for %ld seconds. Ctrl-C stops the scan.\n", unit, seconds );
	}

	result = interface->RunController( unit, & operation, & error );

	if ( operation.Devices )
	{
		for( index = 0; index < operation.Count; index++ )
		{
			PrintDevice( & operation.Devices[index] );
		}

		FreeVec( operation.Devices );
	}

	DropInterface( (APTR) interface );

	switch( result )
	{
		case BT_HCI_OK:
		case BT_LE_CANCELLED:
			if ( mode == BT_LE_ADVERTISE )
			{
				Printf( "Advertising ended.\n" );
			}
			else
			{
				Printf( "Found %lu LE advertisers%s.\n",
					(ULONG) operation.Count, operation.Truncated ? " (results truncated)" : "" );
			}

			return ( RETURN_OK );

		case BT_HCI_NO_CONTROLLER:
			Printf( "No Bluetooth controller available for unit %lu.\n", unit );

			break;

		case BT_HCI_UNSUPPORTED:
			Printf( "LE operations require cls_Bluetooth.Driver 1.13 or later.\n" );

			break;

		case BT_LE_NOT_SUPPORTED:
			Printf( "This controller does not support the requested Bluetooth LE operation.\n" );

			break;

		case BT_HCI_NO_MEMORY:
		case BT_HCI_TIMER_ERROR:
			Printf( "Could not open the resources for the timed LE operation.\n" );

			break;

		default:
			ReportHCIError( result, & error );

			break;
	}

	return ( RETURN_FAIL );
}

// --

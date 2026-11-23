
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

#define HID_MONITOR_CAPACITY 32

// --

static void
PrintReport( const struct BTHIDReport *report )
{
	const uint8_t *data;
	ULONG index;
	LONG x;
	LONG y;

	data = report->Data;

	if ( report->ID == BT_HID_BOOT_KEYBOARD )
	{
		Printf( "Keyboard modifiers $%02lx, key usages", (ULONG) data[0] );

		for( index = 2; index < 8; index++ )
		{
			Printf( " $%02lx", (ULONG) data[index] );
		}
	}
	else
	{
		x = data[1] < 128 ? (LONG) data[1] : (LONG) data[1] - 256;
		y = data[2] < 128 ? (LONG) data[2] : (LONG) data[2] - 256;

		Printf( "Mouse buttons $%02lx, movement %ld,%ld", (ULONG) ( data[0] & 7 ), x, y );
	}

	Printf( "\n" );
}

// --

LONG
MonitorBluetoothHID( struct IORequest *request, ULONG unit, const char *address, LONG seconds )
{
	struct Bluetooth2HIDIFace *interface;
	struct BTHIDMonitor monitor;
	struct BTHCIError error;
	enum BTHCIResult result;

	ULONG index;

	if ( ! ParseAddress( address, monitor.Address ) )
	{
		Printf( "HID requires a Bluetooth address such as 01:23:45:67:89:AB.\n" );

		return ( RETURN_FAIL );
	}

	if ( ( seconds < 1 ) || ( seconds > BT_HID_MAX_SECONDS ) )
	{
		Printf( "SECONDS must be 1 to %lu.\n", (ULONG) BT_HID_MAX_SECONDS );

		return ( RETURN_FAIL );
	}

	interface = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_HID_INTERFACE, BLUETOOTH2_HID_VERSION, NULL );

	if ( ! interface )
	{
		Printf( "HID monitoring requires Bluetooth2 1.21 or later.\n" );

		return ( RETURN_FAIL );
	}

	monitor.Capacity = HID_MONITOR_CAPACITY;
	monitor.Seconds = seconds;
	monitor.Count = 0;
	monitor.Handshake = 0xff;

	monitor.Reports = AllocVecTags( sizeof( *monitor.Reports ) * monitor.Capacity, TAG_END );

	if ( ! monitor.Reports )
	{
		DropInterface( (APTR) interface );

		return ( RETURN_FAIL );
	}

	Printf( "Monitoring Classic HID on %s for %ld seconds or %lu reports. Ctrl-C stops.\n",
		address, seconds, (ULONG) monitor.Capacity );

	result = interface->MonitorController( unit, & monitor, & error );

	DropInterface( (APTR) interface );

	for( index = 0; index < monitor.Count; index++ )
	{
		PrintReport( & monitor.Reports[index] );
	}

	FreeVec( monitor.Reports );

	if ( monitor.Handshake == 0 )
	{
		Printf( "Boot protocol accepted. %lu input reports captured.\n", (ULONG) monitor.Count );
	}

	switch( result )
	{
		case BT_HCI_OK:
		case BT_RFCOMM_CANCELLED:
			return ( RETURN_OK );

		case BT_HID_DISCONNECTED:
			Printf( "The HID device disconnected.\n" );

			return ( monitor.Count ? RETURN_OK : RETURN_WARN );

		case BT_HID_REJECTED:
			Printf( "The device rejected HID Boot Protocol with handshake $%02lx.\n", (ULONG) monitor.Handshake );

			break;

		case BT_HCI_NO_CONTROLLER:
			Printf( "No Bluetooth controller is available on unit %lu.\n", unit );

			break;

		case BT_HCI_UNSUPPORTED:
			Printf( "HID monitoring requires cls_Bluetooth.Driver 1.17 and a Classic Basic Mode L2CAP connection.\n" );

			break;

		case BT_HCI_NO_MEMORY:
		case BT_HCI_TIMER_ERROR:
			Printf( "Could not allocate the HID monitoring resources.\n" );

			break;

		case BT_HCI_BOND_IO_ERROR:
			Printf( "Could not read the saved Bluetooth link key.\n" );

			break;

		default:
			ReportHCIError( result, & error );

			break;
	}

	return ( RETURN_FAIL );
}

// --

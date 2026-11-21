
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
PrintReply( const struct BTRFCOMMTransfer *transfer )
{
	ULONG byte;
	uint8_t character;
	char printable[2];

	Printf( "Received %lu bytes%s.\n", (ULONG) transfer->ReplyLength, transfer->ReplyTruncated ? " (truncated)" : "" );

	printable[1] = 0;

	for( byte = 0; byte < transfer->ReplyLength; byte++ )
	{
		character = transfer->Reply[byte];

		if ( ( character >= 32 ) && ( character <= 126 ) && ( character != '\\' ) )
		{
			printable[0] = character;

			PutStr( printable );
		}
		else
		{
			Printf( "\\x%02lx", (ULONG) character );
		}
	}

	Printf( "\n" );
}

// --

void
ReportRFCOMMStage( uint8_t stage )
{
	static const char *const stages[] = { "Classic link and authentication", "L2CAP channel", "RFCOMM multiplexer",
		"RFCOMM parameters", "serial channel", "modem status", "disconnect", "serial data" };

	if ( stage < sizeof( stages ) / sizeof( stages[0] ) )
	{
		Printf( "RFCOMM test stopped at %s.\n", stages[stage] );
	}
}

// --

LONG
TestRFCOMM( struct IORequest *request, ULONG unit, const char *address, LONG channel, const char *text, LONG reply )
{
	struct Bluetooth2RFCOMMIFace *rfcomm;
	struct Bluetooth2SerialIFace *serial;
	struct BTRFCOMMTransfer transfer;
	struct BTHCIError error;
	enum BTHCIResult result;

	uint8_t received[127];

	if ( ! ParseAddress( address, transfer.Peer.Address ) )
	{
		Printf( "RFCOMM requires a Bluetooth address such as 01:23:45:67:89:AB.\n" );

		return ( RETURN_FAIL );
	}

	if ( ( channel < 1 ) || ( channel > 30 ) )
	{
		Printf( "Enter CHANNEL=1 through CHANNEL=30. SERVICES can show the peer's channel.\n" );

		return ( RETURN_FAIL );
	}

	transfer.Peer.Channel = channel;
	transfer.Data = (const uint8_t *) text;
	transfer.Length = text ? strlen( text ) : 0;
	transfer.Sent = 0;
	transfer.Reply = received;
	transfer.ReplyCapacity = reply ? sizeof( received ) : 0;
	transfer.ReplyLength = 0;
	transfer.ReplyTruncated = 0;

	if ( ( text ) && ( ( ! transfer.Length ) || ( transfer.Length > BT_RFCOMM_TEST_DATA_MAX ) ) )
	{
		Printf( "TEXT must contain 1 to %lu bytes.\n", (ULONG) BT_RFCOMM_TEST_DATA_MAX );

		return ( RETURN_FAIL );
	}

	Printf( "Testing RFCOMM channel %ld on %s from unit %lu.\n", channel, address, unit );

	if ( ( text ) || ( reply ) )
	{
		serial = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_SERIAL_INTERFACE,
			BLUETOOTH2_SERIAL_VERSION, NULL );

		if ( ! serial )
		{
			Printf( "TEXT and REPLY require Bluetooth2 1.11 or later.\n" );

			return ( RETURN_FAIL );
		}

		result = serial->ExchangeController( unit, & transfer, & error );

		DropInterface( (APTR) serial );

		if ( transfer.Sent )
		{
			Printf( "Sent %lu of %lu bytes.\n", (ULONG) transfer.Sent, (ULONG) transfer.Length );
		}

		if ( transfer.ReplyLength )
		{
			PrintReply( & transfer );
		}
	}
	else
	{
		rfcomm = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_RFCOMM_INTERFACE,
			BLUETOOTH2_RFCOMM_VERSION, NULL );

		if ( ! rfcomm )
		{
			Printf( "RFCOMM requires Bluetooth2 1.10 or later.\n" );

			return ( RETURN_FAIL );
		}

		result = rfcomm->TestController( unit, & transfer.Peer, & error );

		DropInterface( (APTR) rfcomm );
	}

	switch( result )
	{
		case BT_HCI_OK:
			Printf( "RFCOMM serial channel opened and closed successfully.\n" );

			return ( RETURN_OK );

		case BT_HCI_NO_CONTROLLER:
			Printf( "No Bluetooth controller available for unit %lu.\n", unit );

			break;

		case BT_HCI_NO_MEMORY:
			Printf( "Could not allocate a Bluetooth RFCOMM reply port.\n" );

			break;

		case BT_HCI_UNSUPPORTED:
			Printf( "This test requires Bluetooth.Driver %s and a Basic Mode L2CAP channel.\n",
				( text || reply ) ? "1.9" : "1.8" );

			break;

		case BT_HCI_BOND_IO_ERROR:
			Printf( "Could not read the saved Bluetooth link key.\n" );

			break;

		default:
			ReportRFCOMMStage( transfer.Peer.Stage );

			ReportHCIError( result, & error );

			break;
	}

	return ( RETURN_FAIL );
}

// --

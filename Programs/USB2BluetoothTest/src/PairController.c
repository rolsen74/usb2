
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stdio.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "BluetoothTest.h"

// --

static LONG
ReadPIN( struct BTPairRequest *pair )
{
	BPTR input;
	ULONG excess;
	uint8_t character;
	LONG entered;

	input = Input();

	if ( ( ! IsInteractive( input ) ) || ( ! SetMode( input, 1 ) ) )
	{
		Printf( "PIN entry requires an interactive Shell.\n" );

		return ( FALSE );
	}

	fputs( "Bluetooth PIN (hidden): ", stdout );

	fflush( stdout );

	pair->PINLength = 0;
	excess = 0;
	entered = FALSE;

	while( ( ! ( SetSignal( 0, SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C ) ) && ( Read( input, & character, 1 ) == 1 ) )
	{
		if ( ( character == '\r' ) || ( character == '\n' ) )
		{
			entered = TRUE;

			break;
		}

		if ( ( character == 3 ) || ( character == 27 ) || ( character == 0x9b ) )
		{
			break;
		}

		if ( ( character == 8 ) || ( character == 127 ) )
		{
			if ( excess )
			{
				excess--;
			}
			else if ( pair->PINLength )
			{
				pair->PIN[--pair->PINLength] = 0;
			}
		}
		else if ( ( character >= 32 ) && ( character <= 126 ) )
		{
			if ( pair->PINLength < BT_PIN_MAX )
			{
				pair->PIN[pair->PINLength++] = character;
			}
			else
			{
				excess++;
			}
		}
	}

	SetMode( input, 0 );

	puts( "" );

	if ( ( entered ) && ( ( ! pair->PINLength ) || ( excess ) ) )
	{
		Printf( "Enter a PIN of 1 to 16 characters.\n" );

		entered = FALSE;
	}

	return ( entered );
}

// --

LONG
PairController( struct IORequest *request, ULONG unit, const char *address, enum BTPairOperation operation )
{
	struct Bluetooth2IFace *bt2;
	struct BTBondRequest bond;
	struct BTHCIError error;
	enum BTHCIResult result;

	ULONG byte;

	if ( ! ParseAddress( address, bond.Peer.Address ) )
	{
		Printf( "Enter a Bluetooth address such as 01:23:45:67:89:AB.\n" );

		return ( RETURN_FAIL );
	}

	bond.Peer.PINLength = 0;
	bond.Reconnect = operation == BT_TEST_RECONNECT;
	bond.SentSavedKey = 0;

	bt2 = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_INTERFACE,
		BLUETOOTH2_INTERFACE_VERSION, NULL );

	if ( ! bt2 )
	{
		Printf( "This operation requires bluetooth2.device main interface version %lu.\n",
			(ULONG) BLUETOOTH2_INTERFACE_VERSION );

		return ( RETURN_FAIL );
	}

	result = BT_HCI_INVALID_ARGUMENT;

	if ( ( bond.Reconnect ) || ( ReadPIN( & bond.Peer ) ) )
	{
		Printf( "Opening a Classic Bluetooth connection to %s on unit %lu.\n", address, unit );

		result = operation != BT_TEST_PAIR ? bt2->bonding_BondController( unit, & bond, & error ) :
			bt2->pairing_PairController( unit, & bond.Peer, & error );
	}

	for( byte = 0; byte < BT_PIN_MAX; byte++ )
	{
		((volatile uint8_t *)bond.Peer.PIN)[byte] = 0;
	}

	DropInterface( (APTR) bt2 );

	switch( result )
	{
		case BT_HCI_OK:
			Printf( "Authenticated and encrypted link verified, then disconnected.\n" );

			if ( operation == BT_TEST_BOND )
			{
				Printf( "Link key saved for this adapter and peer.\n" );
			}
			else if ( bond.Reconnect )
			{
				if ( bond.SentSavedKey )
				{
					Printf( "Reconnected using the saved link key.\n" );
				}
				else
				{
					Printf( "The controller authenticated without requesting the saved key.\n" );
				}
			}

			return ( RETURN_OK );

		case BT_HCI_NO_CONTROLLER:
			Printf( "No Bluetooth controller available for unit %lu.\n", unit );

			break;

		case BT_HCI_NO_MEMORY:
			Printf( "Could not allocate a Bluetooth pairing reply port.\n" );

			break;

		case BT_HCI_UNSUPPORTED:
			Printf( "This operation requires cls_Bluetooth.Driver %s or later.\n",
				operation == BT_TEST_PAIR ? "1.2" : "1.3" );

			break;

		case BT_HCI_NO_SAVED_KEY:
			Printf( "No saved bond for this adapter and peer. Use BOND first.\n" );

			break;

		case BT_HCI_NO_NEW_KEY:
			Printf( "Link verified, but the controller did not report a new key to save.\n" );

			break;

		case BT_HCI_BOND_IO_ERROR:
			Printf( "Could not %s the saved key.\n", bond.Reconnect ? "read" : "write" );

			break;

		default:
			ReportHCIError( result, & error );

			break;
	}

	return ( RETURN_FAIL );
}

// --

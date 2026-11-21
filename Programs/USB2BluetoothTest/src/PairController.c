
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stdio.h>
#include <string.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "BluetoothTest.h"

// --

static LONG
HexDigit( char digit )
{
	if ( ( digit >= '0' ) && ( digit <= '9' ) )
	{
		return ( digit - '0' );
	}

	digit |= 0x20;

	if ( ( digit >= 'a' ) && ( digit <= 'f' ) )
	{
		return ( digit - 'a' + 10 );
	}

	return ( -1 );
}

// --

static LONG
ParseAddress( const char *text, uint8_t *address )
{
	ULONG byte;
	LONG high;
	LONG low;

	if ( strlen( text ) != 17 )
	{
		return ( FALSE );
	}

	for( byte = 0; byte < 6; byte++ )
	{
		high = HexDigit( text[byte * 3] );

		low = HexDigit( text[byte * 3 + 1] );

		if ( ( high < 0 ) || ( low < 0 ) || ( ( byte < 5 ) && ( text[byte * 3 + 2] != ':' ) ) )
		{
			return ( FALSE );
		}

		address[byte] = ( high << 4 ) | low;
	}

	return ( TRUE );
}

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
	struct Bluetooth2PairingIFace *pairing;
	struct Bluetooth2BondingIFace *bonding;
	struct BTBondRequest bond;
	struct BTHCIError error;
	enum BTHCIResult result;

	ULONG byte;

	if ( ! ParseAddress( address, bond.Peer.Address ) )
	{
		Printf( "Enter a Bluetooth address such as 01:23:45:67:89:AB.\n" );

		return ( RETURN_FAIL );
	}

	pairing = NULL;
	bonding = NULL;
	bond.Peer.PINLength = 0;
	bond.Reconnect = operation == BT_TEST_RECONNECT;
	bond.SentSavedKey = 0;

	if ( operation == BT_TEST_PAIR )
	{
		pairing = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_PAIRING_INTERFACE,
			BLUETOOTH2_PAIRING_VERSION, NULL );

		if ( ! pairing )
		{
			Printf( "PAIR requires Bluetooth2 1.4 or later.\n" );

			return ( RETURN_FAIL );
		}
	}
	else
	{
		bonding = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_BONDING_INTERFACE,
			BLUETOOTH2_BONDING_VERSION, NULL );

		if ( ! bonding )
		{
			Printf( "BOND and RECONNECT require Bluetooth2 1.5 or later.\n" );

			return ( RETURN_FAIL );
		}
	}

	result = BT_HCI_INVALID_ARGUMENT;

	if ( ( bond.Reconnect ) || ( ReadPIN( & bond.Peer ) ) )
	{
		Printf( "Opening a Classic Bluetooth connection to %s on unit %lu.\n", address, unit );

		result = bonding ? bonding->BondController( unit, & bond, & error ) :
			pairing->PairController( unit, & bond.Peer, & error );
	}

	for( byte = 0; byte < BT_PIN_MAX; byte++ )
	{
		((volatile uint8_t *)bond.Peer.PIN)[byte] = 0;
	}

	DropInterface( bonding ? (APTR) bonding : (APTR) pairing );

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
			Printf( "This operation requires Bluetooth.Driver %s or later.\n",
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

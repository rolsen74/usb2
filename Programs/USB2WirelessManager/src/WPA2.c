
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static void
Clear_Secret( void *memory, U32 length )
{
	volatile U8 *data;

	data = memory;

	while( length-- )
	{
		*data++ = 0;
	}
}

// --

static enum WirelessResult
Read_Password( char *password, U32 *length )
{
	enum WirelessResult result;

	BPTR input;
	U32 used;
	U32 excess;
	U8 character;

	input = Input();
	used = 0;
	excess = 0;

	if ( ( ! IsInteractive( input ) ) || ( ! SetMode( input, 1 ) ) )
	{
		fprintf( stderr, "WPA2 requires an interactive Shell for password entry.\n" );

		return ( WIRELESS_IO_ERROR );
	}

	fputs( "WPA2 passphrase: ", stdout );

	fflush( stdout );

	result = WIRELESS_STOPPED;

	while( ( ! ( SetSignal( 0, SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C ) ) && ( Read( input, & character, 1 ) == 1 ) )
	{
		if ( ( character == '\r' ) || ( character == '\n' ) )
		{
			result = WIRELESS_MATCH;
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
			else if ( used )
			{
				password[--used] = 0;
			}
		}
		else if ( ( character >= 32 ) && ( character <= 126 ) )
		{
			if ( used < 63 )
			{
				password[used++] = character;
			}
			else
			{
				excess++;
			}
		}
	}

	SetMode( input, 0 );

	puts( "" );

	if ( ( result == WIRELESS_MATCH ) && ( ( used < 8 ) || ( excess ) ) )
	{
		fprintf( stderr, "Enter a WPA2 passphrase of 8 to 63 printable characters.\n" );

		return ( WIRELESS_IO_ERROR );
	}

	*length = used;

	return ( result );
}

// --

enum WirelessResult
WPA2_Open( struct Wireless *wireless )
{
	struct USB2WPA2Network network;
	enum WirelessResult result;

	char password[63];
	U32 length;
	S32 error;

	if ( ! wireless->AP.RSNLength )
	{
		fprintf( stderr, "This access point does not advertise WPA2.\n" );

		return ( WIRELESS_IO_ERROR );
	}

	wireless->WPA2Base = OpenLibrary( USB2WPA2_NAME, USB2WPA2_VERSION );

	if ( ! wireless->WPA2Base )
	{
		fprintf( stderr, "Could not open LIBS:%s.\n", USB2WPA2_NAME );

		return ( WIRELESS_IO_ERROR );
	}

	wireless->IWPA2 = (struct USB2WPA2IFace *) GetInterface( wireless->WPA2Base, "main", 1, NULL );

	if ( ! wireless->IWPA2 )
	{
		fprintf( stderr, "Could not open the WPA2 library interface.\n" );

		return ( WIRELESS_IO_ERROR );
	}

	result = Read_Password( password, & length );

	if ( result != WIRELESS_MATCH )
	{
		Clear_Secret( password, sizeof( password ) );

		return ( result );
	}

	network.SSID = (const U8 *) wireless->SSID;
	network.SSIDLength = wireless->SSIDLength;
	network.RSN = wireless->AP.RSN;
	network.RSNLength = wireless->AP.RSNLength;

	memcpy( network.BSSID, wireless->AP.BSSID, sizeof( network.BSSID ) );

	memcpy( network.Station, wireless->Station, sizeof( network.Station ) );

	wireless->WPA2 = wireless->IWPA2->Create( & network, password, length, & error );

	Clear_Secret( password, sizeof( password ) );

	if ( ! wireless->WPA2 )
	{
		if ( error == USB2WPA2_ERR_NETWORK )
		{
			fprintf( stderr, "The access point must offer WPA2-Personal with CCMP and optional management frame protection.\n" );
		}
		else if ( error == USB2WPA2_ERR_CRYPTO )
		{
			fprintf( stderr, "Could not initialize WPA2 cryptography. Check the AmiSSL v5 (5.16+) or v6 (6.1+) installation.\n" );
		}
		else
		{
			fprintf( stderr, "Could not create a WPA2 session (%ld).\n", (long) error );
		}

		return ( WIRELESS_IO_ERROR );
	}

	return ( Wireless_Set_Copy_Hook( wireless ) ? WIRELESS_MATCH : WIRELESS_IO_ERROR );
}

// --

void
WPA2_Close( struct Wireless *wireless )
{
	if ( wireless->WPA2 )
	{
		wireless->IWPA2->Destroy( wireless->WPA2 );
	}

	if ( wireless->IWPA2 )
	{
		DropInterface( (struct Interface *) wireless->IWPA2 );
	}

	if ( wireless->WPA2Base )
	{
		CloseLibrary( wireless->WPA2Base );
	}
}

// --

static S32
Process_EAPOL( struct Wireless *wireless, const U8 *frame, U32 length )
{
	wireless->WPA2Result = wireless->IWPA2->Process( wireless->WPA2, frame, length, & wireless->WPA2Reply );

	return ( wireless->WPA2Result != 0 );
}

// --

static enum WirelessResult
Exchange_EAPOL( struct Wireless *wireless, U32 micros, S32 *authorized )
{
	struct USB2WPA2Reply *reply;
	enum WirelessResult result;

	U32 flags;

	reply = & wireless->WPA2Reply;
	result = Wireless_Wait_Frame( wireless, micros, NULL, 0, Process_EAPOL, TRUE );

	if ( result != WIRELESS_MATCH )
	{
		goto done;
	}

	flags = reply->Flags;
	result = WIRELESS_IO_ERROR;

	if ( wireless->WPA2Result < 0 )
	{
		fprintf( stderr, "WPA2 cryptography failed.\n" );

		goto done;
	}

	if ( ( flags & USB2WPA2_INSTALL_GROUP ) &&
		( ! Wireless_Set_Key( wireless, reply->GroupIndex, reply->GroupKey, reply->GroupRSC ) ) )
	{
		goto done;
	}

	if ( ( flags & USB2WPA2_INSTALL_PAIRWISE ) && ( ! Wireless_Set_Key( wireless, 0, reply->PairwiseKey, NULL ) ) )
	{
		goto done;
	}

	if ( ! Wireless_Send_EAPOL( wireless, reply ) )
	{
		goto done;
	}

	if ( flags & USB2WPA2_AUTHORIZE )
	{
		if ( ! Wireless_Set_Options( wireless, 0, wireless->AP.BSSID, wireless->AssocID, S2ENC_CCMP, TRUE ) )
		{
			goto done;
		}

		*authorized = TRUE;
	}

	result = WIRELESS_MATCH;

done:

	Clear_Secret( reply, sizeof( *reply ) );

	return ( result );
}

// --

enum WirelessResult
WPA2_Handshake( struct Wireless *wireless )
{
	enum WirelessResult result;

	U32 tries;
	S32 authorized;

	authorized = FALSE;

	for( tries = 0; tries < 8; tries++ )
	{
		result = Exchange_EAPOL( wireless, 4000000, & authorized );

		if ( ( result != WIRELESS_MATCH ) || ( authorized ) )
		{
			break;
		}
	}

	if ( ( ! authorized ) && ( ( result == WIRELESS_TIMEOUT ) || ( result == WIRELESS_MATCH ) ) )
	{
		fprintf( stderr, "WPA2 key exchange timed out. Check the passphrase and access point settings.\n" );

		result = WIRELESS_TIMEOUT;
	}

	return ( result );
}

// --

S32
WPA2_Wait_Link( struct Wireless *wireless )
{
	enum WirelessResult result;

	S32 authorized;

	authorized = TRUE;

	do
	{
		result = Exchange_EAPOL( wireless, 0, & authorized );
	}
	while( result == WIRELESS_MATCH );

	return ( result == WIRELESS_STOPPED );
}

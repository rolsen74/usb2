
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
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
Read_Password( char *password, U32 *length, const char *prompt, S32 sae )
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
		fprintf( stderr, "Passphrase entry requires an interactive Shell.\n" );

		return ( WIRELESS_IO_ERROR );
	}

	fputs( prompt, stdout );

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

	if ( ( result == WIRELESS_MATCH ) && ( ( used < ( sae ? 1U : 8U ) ) || ( excess ) ) )
	{
		fprintf( stderr, "Enter a passphrase of %u to 63 printable characters.\n", sae ? 1U : 8U );

		return ( WIRELESS_IO_ERROR );
	}

	*length = used;

	return ( result );
}

// --

static S32
WPA2_OpenInterface( struct Wireless *wireless )
{
	if ( ! wireless->AP.RSNLength )
	{
		fprintf( stderr, "This access point does not advertise WPA2.\n" );

		return ( FALSE );
	}

	wireless->WPA2Base = OpenLibrary( USB2WPA2_NAME, USB2WPA2_VERSION );

	if ( ! wireless->WPA2Base )
	{
		fprintf( stderr, "Could not open LIBS:%s.\n", USB2WPA2_NAME );

		return ( FALSE );
	}

	wireless->IWPA2 = (struct USB2WPA2IFace *) GetInterface( wireless->WPA2Base, "main", 1, NULL );

	if ( ! wireless->IWPA2 )
	{
		fprintf( stderr, "Could not open the WPA2 library interface.\n" );

		return ( FALSE );
	}

	return ( TRUE );
}

// --

static S32
Match_FirstKey( struct Wireless *wireless, const U8 *frame, U32 length )
{
	U32 bodyLength;
	U16 info;

	(void) wireless;

	// EAPOL-Key for RSN, with pairwise and ACK set and without a MIC.
	if ( ( length < 99 ) || ( frame[1] != 3 ) || ( frame[4] != 2 ) )
	{
		return ( FALSE );
	}

	bodyLength = ( (U32) frame[2] << 8 ) | frame[3];
	info = ( (U16) frame[5] << 8 ) | frame[6];

	return ( ( bodyLength == length - 4 ) && ( ( info & 0x03c8 ) == 0x0088 ) );
}

// --

enum WirelessResult
WPA2_Test_Association( struct Wireless *wireless )
{
	enum WirelessResult result;

	const U8 *associationIE;

	if ( ! WPA2_OpenInterface( wireless ) )
	{
		return ( WIRELESS_IO_ERROR );
	}

	result = Wireless_Authenticate( wireless );

	if ( result != WIRELESS_MATCH )
	{
		return ( result );
	}

	associationIE = wireless->IWPA2->AssociationIE();

	result = Wireless_Associate( wireless, associationIE, USB2WPA2_ASSOC_IE_SIZE );

	if ( result != WIRELESS_MATCH )
	{
		return ( result );
	}

	printf( "802.11 association accepted on channel %u (AID %u).\n",
		(unsigned) wireless->AP.Channel, (unsigned) wireless->AssocID );

	result = Wireless_Wait_Frame( wireless, 2500000, NULL, 0, Match_FirstKey, TRUE );

	if ( result == WIRELESS_MATCH )
	{
		puts( "Received WPA2 handshake message 1." );
	}
	else if ( result == WIRELESS_TIMEOUT )
	{
		fprintf( stderr, "WPA2 handshake message 1 timed out.\n" );
	}

	return ( result );
}

// --

enum WirelessResult
WPA_Read_Passphrase( struct Wireless *wireless, char *input, char **password, U32 *length )
{
	if ( wireless->Auto )
	{
		return ( WiFi_Passphrase( wireless, input, password, length ) );
	}

	*password = wireless->Password;

	if ( *password )
	{
		*length = strlen( *password );
		wireless->Password = NULL;

		return ( WIRELESS_MATCH );
	}

	*password = input;

	return ( Read_Password( input, length, wireless->UseSAE ? "WPA3 passphrase: " :
		( wireless->UseWPA ? "WPA passphrase: " : "WPA2 passphrase: " ), wireless->UseSAE ) );
}

// --

static S32
WPA_OpenInterface( struct Wireless *wireless )
{
	wireless->WPA2Base = OpenLibrary( USB2WPA_NAME, USB2WPA_VERSION );

	if ( ! wireless->WPA2Base )
	{
		fprintf( stderr, "Could not open LIBS:%s.\n", USB2WPA_NAME );

		return ( FALSE );
	}

	wireless->IWPA = (struct USB2WPAIFace *) GetInterface( wireless->WPA2Base, "main", 1, NULL );

	if ( ! wireless->IWPA )
	{
		fprintf( stderr, "Could not open the WPA library interface.\n" );

		return ( FALSE );
	}

	return ( TRUE );
}

// --

enum WirelessResult
PSK_Open( struct Wireless *wireless )
{
	struct USB2WPA2Network network;
	struct USB2WPANetwork legacy;
	enum WirelessResult result;

	char input[63];
	char *password;
	U32 length;
	S32 error;

	if ( ! ( wireless->UseWPA ? WPA_OpenInterface( wireless ) : WPA2_OpenInterface( wireless ) ) )
	{
		return ( WIRELESS_IO_ERROR );
	}

	result = WPA_Read_Passphrase( wireless, input, & password, & length );

	if ( result != WIRELESS_MATCH )
	{
		Clear_Secret( input, sizeof( input ) );

		return ( result );
	}

	if ( wireless->UseWPA )
	{
		legacy.SSID = (const U8 *) wireless->SSID;
		legacy.SSIDLength = wireless->SSIDLength;
		legacy.WPAIE = wireless->AP.WPAIE;
		legacy.WPAIELength = wireless->AP.WPAIELength;

		memcpy( legacy.BSSID, wireless->AP.BSSID, sizeof( legacy.BSSID ) );

		memcpy( legacy.Station, wireless->Station, sizeof( legacy.Station ) );

		wireless->WPA = wireless->IWPA->Create( & legacy, password, length, & error );

	}
	else
	{
		network.SSID = (const U8 *) wireless->SSID;
		network.SSIDLength = wireless->SSIDLength;
		network.RSN = wireless->AP.RSN;
		network.RSNLength = wireless->AP.RSNLength;

		memcpy( network.BSSID, wireless->AP.BSSID, sizeof( network.BSSID ) );

		memcpy( network.Station, wireless->Station, sizeof( network.Station ) );

		wireless->WPA2 = wireless->IWPA2->Create( & network, password, length, & error );

	}

	Clear_Secret( password, password == input ? sizeof( input ) : length );

	if ( wireless->UseWPA && ! wireless->WPA )
	{
		fprintf( stderr, "Could not create a WPA/TKIP session (%ld).\n", (long) error );

		return ( WIRELESS_IO_ERROR );
	}

	if ( ( ! wireless->UseWPA ) && ( ! wireless->WPA2 ) )
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

	return ( WIRELESS_MATCH );
}

// --

void
PSK_Close( struct Wireless *wireless )
{
	if ( wireless->Password )
	{
		Clear_Secret( wireless->Password, strlen( wireless->Password ) );

		wireless->Password = NULL;
	}

	if ( wireless->WPA2 )
	{
		wireless->IWPA2->Destroy( wireless->WPA2 );
	}

	if ( wireless->IWPA2 )
	{
		DropInterface( (struct Interface *) wireless->IWPA2 );
	}

	if ( wireless->WPA )
	{
		wireless->IWPA->Destroy( wireless->WPA );
	}

	if ( wireless->IWPA )
	{
		DropInterface( (struct Interface *) wireless->IWPA );
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
	if ( wireless->UseWPA )
	{
		wireless->WPA2Result = wireless->IWPA->Process( wireless->WPA, frame, length, & wireless->PSKReply.WPA );
	}
	else
	{
		wireless->WPA2Result = wireless->IWPA2->Process( wireless->WPA2, frame, length, & wireless->PSKReply.WPA2 );
	}

	return ( wireless->WPA2Result != 0 );
}

// --

static enum WirelessResult
Exchange_EAPOL( struct Wireless *wireless, U32 micros, S32 *authorized )
{
	struct USB2WPA2Reply *wpa2;
	struct USB2WPAReply *wpa;
	enum WirelessResult result;

	const U8 *frame;
	const U8 *groupKey;
	const U8 *pairwiseKey;
	const U8 *rsc;
	U32 flags;
	U32 groupIndex;
	U32 cipher;
	U32 keyLength;
	U32 frameLength;

	wpa2 = & wireless->PSKReply.WPA2;
	wpa = & wireless->PSKReply.WPA;

	result = Wireless_Wait_Frame( wireless, micros, NULL, 0, Process_EAPOL, TRUE );

	if ( result != WIRELESS_MATCH )
	{
		goto done;
	}

	result = WIRELESS_IO_ERROR;

	if ( wireless->UseWPA )
	{
		flags = wpa->Flags;
		frame = wpa->Frame;
		frameLength = wpa->Length;
		groupIndex = wpa->GroupIndex ? wpa->GroupIndex : USB2WIRELESS_TKIP_GROUP_ZERO;
		groupKey = wpa->GroupKey;
		pairwiseKey = wpa->PairwiseKey;
		rsc = wpa->GroupRSC;
		cipher = S2ENC_TKIP;
		keyLength = sizeof( wpa->GroupKey );
	}
	else
	{
		flags = wpa2->Flags;
		frame = wpa2->Frame;
		frameLength = wpa2->Length;
		groupIndex = wpa2->GroupIndex;
		groupKey = wpa2->GroupKey;
		pairwiseKey = wpa2->PairwiseKey;
		rsc = wpa2->GroupRSC;
		cipher = S2ENC_CCMP;
		keyLength = sizeof( wpa2->GroupKey );
	}

	if ( wireless->WPA2Result < 0 )
	{
		fprintf( stderr, "%s cryptography failed.\n", wireless->UseWPA ? "WPA" : "WPA2" );

		goto done;
	}

	if ( ( flags & USB2WPA2_INSTALL_GROUP ) &&
		( ! Wireless_Set_Key( wireless, groupIndex, groupKey, rsc, cipher, keyLength ) ) )
	{
		goto done;
	}

	if ( ( flags & USB2WPA2_INSTALL_PAIRWISE ) && ( ! Wireless_Set_Key( wireless, 0, pairwiseKey, NULL, cipher, keyLength ) ) )
	{
		goto done;
	}

	if ( ! Wireless_Send_EAPOL( wireless, frame, frameLength, flags ) )
	{
		goto done;
	}

	if ( flags & USB2WPA2_AUTHORIZE )
	{
		if ( ! Wireless_Set_Options( wireless, 0, wireless->AP.BSSID, wireless->AssocID, cipher, TRUE ) )
		{
			goto done;
		}

		*authorized = TRUE;
	}

	result = WIRELESS_MATCH;

done:

	Clear_Secret( & wireless->PSKReply, sizeof( wireless->PSKReply ) );

	return ( result );
}

// --

enum WirelessResult
PSK_Handshake( struct Wireless *wireless )
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
		fprintf( stderr, "%s key exchange timed out. Check the passphrase and access point settings.\n",
			wireless->UseWPA ? "WPA" : "WPA2" );

		result = WIRELESS_TIMEOUT;
	}

	return ( result );
}

// --

S32
PSK_Wait_Link( struct Wireless *wireless )
{
	enum WirelessResult result;

	S32 authorized;

	authorized = TRUE;

	do
	{
		result = Exchange_EAPOL( wireless, 0, & authorized );
	}
	while( result == WIRELESS_MATCH );

	if ( result == WIRELESS_DISCONNECTED )
	{
		fprintf( stderr, "Wi-Fi link disconnected.\n" );
	}

	return ( result == WIRELESS_STOPPED );
}

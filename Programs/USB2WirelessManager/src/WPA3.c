
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define SAE_AUTH_ALGORITHM 3
#define SAE_AUTH_HASH_TO_ELEMENT 126
#define SAE_AUTH_HEADER 30

// --

static S32
MatchSAE( struct Wireless *wireless, const U8 *frame, U32 length )
{
	U32 copied;

	if ( ( ! Wireless_Reply_From_AP( wireless, frame, length, 0xb0 ) ) ||
		( frame[24] != SAE_AUTH_ALGORITHM ) || ( frame[25] ) ||
		( frame[26] != wireless->SAETransaction ) || ( frame[27] ) )
	{
		return ( FALSE );
	}

	wireless->Status = frame[28] | ( (U16) frame[29] << 8 );
	wireless->SAELength = length - SAE_AUTH_HEADER;

	copied = wireless->SAELength;

	if ( copied > sizeof( wireless->SAEFrame ) )
	{
		copied = sizeof( wireless->SAEFrame );
	}

	memcpy( wireless->SAEFrame, frame + SAE_AUTH_HEADER, copied );

	return ( TRUE );
}

// --

static enum WirelessResult
ExchangeSAE( struct Wireless *wireless, U16 transaction, U16 status, U8 *payload, U32 length,
	struct USB2SAEAuth *auth )
{
	enum WirelessResult result;

	U8 frame[SAE_AUTH_HEADER + USB2SAE_COMMIT_SIZE];
	U32 attempt;

	wireless->SAETransaction = transaction;
	result = WIRELESS_TIMEOUT;

	for( attempt = 0; attempt < 3; attempt++ )
	{
		if ( auth )
		{
			if ( ! wireless->IWPA3->SAECommit( auth, wireless->SAEFrame, wireless->SAELength, payload ) )
			{
				fprintf( stderr, "The access point sent an unsupported or invalid SAE commit.\n" );

				return ( WIRELESS_IO_ERROR );
			}

			printf( "Sending SAE confirm %u.\n", (unsigned) ( payload[0] | ( (U16) payload[1] << 8 ) ) );
		}

		Wireless_Frame_Header( wireless, frame, 0xb0 );

		frame[24] = SAE_AUTH_ALGORITHM;
		frame[25] = 0;
		frame[26] = transaction;
		frame[27] = 0;
		frame[28] = status;
		frame[29] = 0;

		memcpy( frame + SAE_AUTH_HEADER, payload, length );

		result = Wireless_Wait_Frame( wireless, 2000000, frame, SAE_AUTH_HEADER + length, MatchSAE, FALSE );

		if ( result != WIRELESS_TIMEOUT )
		{
			break;
		}
	}

	if ( result == WIRELESS_TIMEOUT )
	{
		wireless->RetryPassword = TRUE;

		fprintf( stderr, "SAE %s timed out.\n", transaction == 1 ? "commit" : "confirm" );
	}
	else if ( ( result == WIRELESS_MATCH ) && ( wireless->Status != status ) )
	{
		wireless->RetryPassword = TRUE;

		fprintf( stderr, "SAE %s rejected (status %u).\n",
			transaction == 1 ? "commit" : "confirm", (unsigned) wireless->Status );

		result = WIRELESS_IO_ERROR;
	}

	return ( result );
}

// --

enum WirelessResult
WPA3_Authenticate( struct Wireless *wireless )
{
	struct USB2WPA3Network network;
	struct USB2SAEAuth *auth;
	enum WirelessResult result;

	char input[63];
	char *password;
	U8 commit[USB2SAE_COMMIT_SIZE];
	U8 confirm[USB2SAE_CONFIRM_SIZE];
	U8 pmk[32];
	U8 pmkid[16];
	U32 length;
	U16 commitStatus;
	S32 h2e;
	S32 error;

	if ( ( wireless->AP.RSNLength < 2 ) ||
		( ! RSN_Supports_SAE( wireless->AP.RSN + 2, wireless->AP.RSNLength - 2 ) ) )
	{
		fprintf( stderr, "WPA3 needs an access point offering SAE, CCMP-128 and BIP-CMAC-128.\n" );

		return ( WIRELESS_IO_ERROR );
	}

	h2e = RSNX_Supports_H2E( wireless->AP.RSNX, wireless->AP.RSNXLength );
	commitStatus = h2e ? SAE_AUTH_HASH_TO_ELEMENT : 0;

	wireless->WPA3Base = OpenLibrary( USB2WPA3_NAME, USB2WPA3_VERSION );

	if ( ( ! wireless->WPA3Base ) || ( wireless->WPA3Base->lib_Revision < USB2WPA3_SAE_CONFIRM_REVISION ) )
	{
		fprintf( stderr, "Install LIBS:%s 1.4 or later.\n", USB2WPA3_NAME );

		return ( WIRELESS_IO_ERROR );
	}

	wireless->IWPA3 = (struct USB2WPA3IFace *) GetInterface( wireless->WPA3Base, "main", 1, NULL );

	if ( ! wireless->IWPA3 )
	{
		fprintf( stderr, "Could not open the WPA3 library interface.\n" );

		return ( WIRELESS_IO_ERROR );
	}

	result = WPA_Read_Passphrase( wireless, input, & password, & length );

	if ( result != WIRELESS_MATCH )
	{
		Clear_Secret( input, sizeof( input ) );

		return ( result );
	}

	printf( "SAE method: %s.\n", h2e ? "Hash-to-Element" : "Hunting-and-Pecking" );

	if ( h2e )
	{
		auth = wireless->IWPA3->SAEStart( (const U8 *) wireless->SSID, wireless->SSIDLength,
			password, length, wireless->Station, wireless->AP.BSSID, commit );
	}
	else
	{
		auth = wireless->IWPA3->SAEStartMode( (const U8 *) wireless->SSID, wireless->SSIDLength,
			password, length, wireless->Station, wireless->AP.BSSID, commit, USB2WPA3_SAE_HNP );
	}

	Clear_Secret( password, password == input ? sizeof( input ) : length );

	if ( ! auth )
	{
		fprintf( stderr, "Could not initialize SAE. Check the AmiSSL installation.\n" );

		return ( WIRELESS_IO_ERROR );
	}

	result = ExchangeSAE( wireless, 1, commitStatus, commit, sizeof( commit ), NULL );

	if ( result != WIRELESS_MATCH )
	{
		goto done;
	}

	result = ExchangeSAE( wireless, 2, 0, confirm, sizeof( confirm ), auth );

	if ( result != WIRELESS_MATCH )
	{
		goto done;
	}

	if ( wireless->SAELength < USB2SAE_CONFIRM_SIZE )
	{
		fprintf( stderr, "SAE confirm reply is too short (%lu bytes).\n", (unsigned long) wireless->SAELength );
		result = WIRELESS_IO_ERROR;

		goto done;
	}

	printf( "Received SAE confirm %u (%lu bytes).\n",
		(unsigned) ( wireless->SAEFrame[0] | ( (U16) wireless->SAEFrame[1] << 8 ) ),
		(unsigned long) wireless->SAELength );

	if ( ! wireless->IWPA3->SAEConfirm( auth, wireless->SAEFrame, USB2SAE_CONFIRM_SIZE, pmk, pmkid ) )
	{
		fprintf( stderr, "Could not authenticate the access point's SAE confirm.\n" );

		wireless->RetryPassword = TRUE;
		result = WIRELESS_IO_ERROR;

		goto done;
	}

	puts( "SAE confirmation verified." );

	wireless->IWPA3->SAEDestroy( auth );

	auth = NULL;
	network.RSN = wireless->AP.RSN;
	network.RSNLength = wireless->AP.RSNLength;

	memcpy( network.BSSID, wireless->AP.BSSID, sizeof( network.BSSID ) );

	memcpy( network.Station, wireless->Station, sizeof( network.Station ) );

	wireless->WPA3 = wireless->IWPA3->Create( & network, wireless->AP.RSNX,
		wireless->AP.RSNXLength, pmk, pmkid, & error );

	if ( ! wireless->WPA3 )
	{
		if ( error == USB2WPA3_ERR_CRYPTO )
		{
			fprintf( stderr, "SAE succeeded but AmiSSL could not start WPA3 key exchange.\n" );
		}
		else
		{
			fprintf( stderr, "SAE succeeded but the WPA3 key exchange session could not be created (%ld).\n", (long) error );
		}
		result = WIRELESS_IO_ERROR;
	}

done:

	Clear_Secret( pmk, sizeof( pmk ) );

	Clear_Secret( pmkid, sizeof( pmkid ) );

	Clear_Secret( confirm, sizeof( confirm ) );

	Clear_Secret( wireless->SAEFrame, sizeof( wireless->SAEFrame ) );

	if ( auth )
	{
		wireless->IWPA3->SAEDestroy( auth );
	}

	return ( result );
}

// --

static S32
ProcessEAPOL( struct Wireless *wireless, const U8 *frame, U32 length )
{
	wireless->WPA2Result = wireless->IWPA3->Process( wireless->WPA3, frame, length, & wireless->WPA3Reply );

	return ( wireless->WPA2Result != 0 );
}

// --

static enum WirelessResult
ExchangeEAPOL( struct Wireless *wireless, U32 micros, S32 *authorized )
{
	struct USB2WPA3Reply *reply;
	enum WirelessResult result;

	U32 flags;

	reply = & wireless->WPA3Reply;

	result = Wireless_Wait_Frame( wireless, micros, NULL, 0, ProcessEAPOL, TRUE );

	if ( result != WIRELESS_MATCH )
	{
		goto done;
	}

	flags = reply->Flags;
	result = WIRELESS_IO_ERROR;

	if ( wireless->WPA2Result < 0 )
	{
		fprintf( stderr, "WPA3 key exchange failed in AmiSSL.\n" );

		goto done;
	}

	if ( ( flags & USB2WPA3_INSTALL_GROUP ) &&
		( ! Wireless_Set_Key( wireless, reply->GroupIndex, reply->GroupKey, reply->GroupRSC, S2ENC_CCMP, sizeof( reply->GroupKey ) ) ) )
	{
		goto done;
	}

	if ( ( flags & USB2WPA3_INSTALL_PAIRWISE ) &&
		( ! Wireless_Set_Key( wireless, 0, reply->PairwiseKey, NULL, S2ENC_CCMP, sizeof( reply->PairwiseKey ) ) ) )
	{
		goto done;
	}

	if ( ( flags & USB2WPA3_INSTALL_IGTK ) &&
		( ! Wireless_Set_Key( wireless, reply->IGTKIndex, reply->IGTK, reply->IGTKIPN, USB2WIRELESSENC_BIP_CMAC, sizeof( reply->IGTK ) ) ) )
	{
		goto done;
	}

	if ( ! Wireless_Send_EAPOL( wireless, reply->Frame, reply->Length, flags ) )
	{
		goto done;
	}

	if ( flags & USB2WPA3_AUTHORIZE )
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
WPA3_Handshake( struct Wireless *wireless )
{
	enum WirelessResult result;

	U32 attempt;
	S32 authorized;

	authorized = FALSE;
	result = WIRELESS_TIMEOUT;

	for( attempt = 0; attempt < 8; attempt++ )
	{
		result = ExchangeEAPOL( wireless, 4000000, & authorized );

		if ( ( result != WIRELESS_MATCH ) || ( authorized ) )
		{
			break;
		}
	}

	if ( ( ! authorized ) && ( ( result == WIRELESS_TIMEOUT ) || ( result == WIRELESS_MATCH ) ) )
	{
		fprintf( stderr, "WPA3 four-way handshake timed out.\n" );

		result = WIRELESS_TIMEOUT;
	}

	return ( result );
}

// --

S32
WPA3_Wait_Link( struct Wireless *wireless )
{
	enum WirelessResult result;

	S32 authorized;

	authorized = TRUE;

	do
	{
		result = ExchangeEAPOL( wireless, 0, & authorized );
	}
	while( result == WIRELESS_MATCH );

	if ( result == WIRELESS_DISCONNECTED )
	{
		fprintf( stderr, "Wi-Fi link disconnected.\n" );
	}

	return ( result == WIRELESS_STOPPED );
}

// --

void
WPA3_Close( struct Wireless *wireless )
{
	if ( wireless->WPA3 )
	{
		wireless->IWPA3->Destroy( wireless->WPA3 );
	}

	if ( wireless->IWPA3 )
	{
		DropInterface( (struct Interface *) wireless->IWPA3 );
	}

	if ( wireless->WPA3Base )
	{
		CloseLibrary( wireless->WPA3Base );
	}
}

// --


/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

struct USB2WPA2Session *
WPA2_Create( struct USB2WPA2IFace *self __attribute__((unused)), const struct USB2WPA2Network *network,
	const char *password, U32 length, S32 *error )
{
	struct WirelessKeySession *session;

	U32 index;

	*error = USB2WPA2_ERR_PASSWORD;

	if ( ( ! password ) || ( length < 8 ) || ( length > 63 ) )
	{
		return ( NULL );
	}

	for( index = 0; index < length; index++ )
	{
		if ( ( (U8) password[index] < 32 ) || ( (U8) password[index] > 126 ) )
		{
			return ( NULL );
		}
	}

	*error = USB2WPA2_ERR_NETWORK;

	if ( ( ! network ) || ( ! network->SSID ) || ( ! network->SSIDLength ) ||
		( network->SSIDLength > 32 ) || ( ! network->RSN ) || ( network->RSNLength < 2 ) ||
		( network->RSNLength > WPA2_RSN_MAX ) || ( network->RSN[0] != 48 ) ||
		( network->RSN[1] + 2U != network->RSNLength ) ||
		( ! RSN_Supports_WPA2( network->RSN + 2, network->RSNLength - 2 ) ) )
	{
		return ( NULL );
	}

	*error = USB2WPA2_ERR_MEMORY;

	session = AllocVecTags( sizeof( *session ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! session )
	{
		return ( NULL );
	}

	*error = USB2WPA2_ERR_CRYPTO;

	if ( ( ! Crypto_Open( & session->Crypto ) ) ||
		( ! Crypto_PMK( & session->Crypto, password, length, network->SSID, network->SSIDLength, session->PMK ) ) )
	{
		Destroy_KeySession( session );

		return ( NULL );
	}

	memcpy( session->AP, network->BSSID, sizeof( session->AP ) );

	memcpy( session->Station, network->Station, sizeof( session->Station ) );

	memcpy( session->RSN, network->RSN, network->RSNLength );

	session->RSNLength = network->RSNLength;
	*error = 0;

	return ( (struct USB2WPA2Session *) session );
}

// --

void
WPA2_Destroy( struct USB2WPA2IFace *self __attribute__((unused)), struct USB2WPA2Session *session )
{
	Destroy_KeySession( (struct WirelessKeySession *) session );
}

// --

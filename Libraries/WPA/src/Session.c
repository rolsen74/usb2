
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "WPA.h"

// --

struct USB2WPASession *
WPA_Create( struct USB2WPAIFace *self __attribute__((unused)), const struct USB2WPANetwork *network,
	const char *password, U32 length, S32 *error )
{
	struct WirelessKeySession *session;

	U32 index;

	*error = USB2WPA_ERR_PASSWORD;

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

	*error = USB2WPA_ERR_NETWORK;

	if ( ( ! network ) || ( ! network->SSID ) || ( ! network->SSIDLength ) ||
		( network->SSIDLength > 32 ) || ( ! network->WPAIE ) || ( network->WPAIELength < 2 ) ||
		( network->WPAIELength > WPA2_RSN_MAX ) ||
		( ! WPA_Supports_TKIP( network->WPAIE, network->WPAIELength ) ) )
	{
		return ( NULL );
	}

	*error = USB2WPA_ERR_MEMORY;

	session = AllocVecTags( sizeof( *session ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! session )
	{
		return ( NULL );
	}

	*error = USB2WPA_ERR_CRYPTO;

	if ( ( ! Crypto_Open( & session->Crypto ) ) ||
		( ! Crypto_PMK( & session->Crypto, password, length, network->SSID, network->SSIDLength, session->PMK ) ) )
	{
		Destroy_KeySession( session );

		return ( NULL );
	}

	memcpy( session->AP, network->BSSID, sizeof( session->AP ) );

	memcpy( session->Station, network->Station, sizeof( session->Station ) );

	memcpy( session->RSN, network->WPAIE, network->WPAIELength );

	session->RSNLength = network->WPAIELength;
	*error = 0;

	return ( (struct USB2WPASession *) session );
}

// --

void
WPA_Destroy( struct USB2WPAIFace *self __attribute__((unused)), struct USB2WPASession *session )
{
	Destroy_KeySession( (struct WirelessKeySession *) session );
}

// --

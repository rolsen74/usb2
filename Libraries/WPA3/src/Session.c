
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define SAE_RSNXE 244
#define SAE_RSNXE_H2E 0x20

// --

struct USB2WPA3Session *
WPA3_Create( struct USB2WPA3IFace *self __attribute__((unused)), const struct USB2WPA3Network *network,
	const U8 *rsnxe, U32 rsnxeLength, const U8 *pmk, const U8 *pmkid, S32 *error )
{
	struct WirelessKeySession *session;

	static const U8 association[] =
	{
		48, 42, 1, 0,
		0, 0x0f, 0xac, 4, // Group CCMP-128
		1, 0, 0, 0x0f, 0xac, 4, // Pairwise CCMP-128
		1, 0, 0, 0x0f, 0xac, 8, // SAE
		0xc0, 0, // Management protection capable and required
		1, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // PMKID
		0, 0x0f, 0xac, 6 // BIP-CMAC-128
	};

	*error = USB2WPA3_ERR_NETWORK;

	if ( ( ! network ) || ( ! pmk ) || ( ! pmkid ) ||
		( ! network->RSN ) || ( network->RSNLength < 2 ) ||
		( network->RSNLength > WPA2_RSN_MAX ) || ( network->RSN[0] != 48 ) ||
		( network->RSN[1] + 2U != network->RSNLength ) ||
		( ! RSN_Supports_SAE( network->RSN + 2, network->RSNLength - 2 ) ) ||
		( rsnxeLength && ( ! rsnxe || rsnxeLength < 3 || rsnxeLength > sizeof( session->RSNX ) ||
			rsnxe[0] != SAE_RSNXE || rsnxe[1] + 2U != rsnxeLength ||
			( rsnxe[2] & 0x0f ) + 3U != rsnxeLength ) ) )
	{
		return ( NULL );
	}

	*error = USB2WPA3_ERR_MEMORY;

	session = AllocVecTags( sizeof( *session ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! session )
	{
		return ( NULL );
	}

	*error = USB2WPA3_ERR_CRYPTO;

	if ( ! Crypto_Open( & session->Crypto ) )
	{
		Destroy_KeySession( session );

		return ( NULL );
	}

	memcpy( session->PMK, pmk, sizeof( session->PMK ) );

	memcpy( session->AP, network->BSSID, sizeof( session->AP ) );

	memcpy( session->Station, network->Station, sizeof( session->Station ) );

	memcpy( session->RSN, network->RSN, network->RSNLength );

	memcpy( session->AssocIEs, association, sizeof( association ) );

	memcpy( session->AssocIEs + 24, pmkid, 16 );

	session->RSNLength = network->RSNLength;
	session->AssocLength = sizeof( association );

	if ( rsnxeLength )
	{
		memcpy( session->RSNX, rsnxe, rsnxeLength );

		session->RSNXLength = rsnxeLength;

		if ( rsnxe[2] & SAE_RSNXE_H2E )
		{
			session->AssocIEs[session->AssocLength++] = SAE_RSNXE;
			session->AssocIEs[session->AssocLength++] = 1;
			session->AssocIEs[session->AssocLength++] = SAE_RSNXE_H2E;
		}
	}

	session->SAE = TRUE;
	*error = 0;

	return ( (struct USB2WPA3Session *) session );
}

// --

void
WPA3_Destroy( struct USB2WPA3IFace *self __attribute__((unused)), struct USB2WPA3Session *session )
{
	Destroy_KeySession( (struct WirelessKeySession *) session );
}

// --

const U8 *
WPA3_AssociationIEs( struct USB2WPA3IFace *self __attribute__((unused)),
	const struct USB2WPA3Session *handle, U32 *length )
{
	const struct WirelessKeySession *session;

	session = (const struct WirelessKeySession *) handle;
	*length = session->SAE ? session->AssocLength : 0;

	return ( *length ? session->AssocIEs : NULL );
}

// --

S32
WPA3_Process( struct USB2WPA3IFace *self __attribute__((unused)), struct USB2WPA3Session *handle,
	const U8 *frame, U32 length, struct USB2WPA3Reply *reply )
{
	struct WirelessKeySession *session;

	S32 status;

	session = (struct WirelessKeySession *) handle;

	if ( ! session->SAE )
	{
		memset( reply, 0, sizeof( *reply ) );

		return ( 0 );
	}

	status = Process_Key( session, frame, length, reply );

	if ( status <= 0 )
	{
		Clear_Secret( reply, sizeof( *reply ) );
	}

	return ( status );
}

// --


/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <string.h>
#include <usb2wpa.h>
#include <usb2wpa2.h>
#include "RSN.h"

// --

static const uint8_t CCMP[] = { 0, 0x0f, 0xac, 4 };
static const uint8_t PSK[] = { 0, 0x0f, 0xac, 2 };

const uint8_t AssociationRSN[USB2WPA2_ASSOC_IE_SIZE] =
{
	48, 20, 1, 0,
	0, 0x0f, 0xac, 4, // Group cipher
	1, 0, 0, 0x0f, 0xac, 4, // Pairwise cipher
	1, 0, 0, 0x0f, 0xac, 2, // PSK
	0, 0 // RSN capabilities
};

// --

static int32_t
Contains_Suite( const uint8_t **data, uint32_t *length, const uint8_t *suite )
{
	uint32_t count;
	int32_t found;

	if ( *length < 2 )
	{
		return ( 0 );
	}

	count = ( *data )[0] | ( (uint32_t) ( *data )[1] << 8 );
	*data += 2;
	*length -= 2;

	if ( count > *length / 4 )
	{
		return ( 0 );
	}

	found = 0;

	while( count-- )
	{
		found |= memcmp( *data, suite, 4 ) == 0;

		*data += 4;
		*length -= 4;
	}

	return ( found );
}

// --

int32_t
RSN_Supports_WPA2( const uint8_t *data, uint32_t length )
{
	if ( ( length < 8 ) || ( data[0] != 1 ) || ( data[1] ) || ( memcmp( data + 2, CCMP, sizeof( CCMP ) ) ) )
	{
		return ( 0 );
	}

	data += 6;
	length -= 6;

	if ( ( ! Contains_Suite( & data, & length, CCMP ) ) || ( ! Contains_Suite( & data, & length, PSK ) ) )
	{
		return ( 0 );
	}

	// Required protected management frames need a different AKM and IGTK support.
	return ( ( ! length ) || ( ( length >= 2 ) && ( ! ( data[0] & 0x40 ) ) ) );
}

// --

int32_t
RSN_Supports_SAE( const uint8_t *data, uint32_t length )
{
	static const uint8_t sae[] = { 0, 0x0f, 0xac, 8 };
	static const uint8_t bip[] = { 0, 0x0f, 0xac, 6 };
	uint32_t count;

	if ( ( length < 8 ) || ( data[0] != 1 ) || ( data[1] ) ||
		( memcmp( data + 2, CCMP, sizeof( CCMP ) ) ) )
	{
		return ( 0 );
	}

	data += 6;
	length -= 6;

	if ( ( ! Contains_Suite( & data, & length, CCMP ) ) ||
		( ! Contains_Suite( & data, & length, sae ) ) ||
		( length < 2 ) || ( ! ( data[0] & 0x80 ) ) )
	{
		return ( 0 );
	}

	data += 2;
	length -= 2;

	if ( ! length )
	{
		return ( 1 ); // The default group management cipher is BIP-CMAC-128.
	}

	if ( length < 2 )
	{
		return ( 0 );
	}

	count = data[0] | ( (uint32_t) data[1] << 8 );

	if ( count > ( length - 2 ) / 16 )
	{
		return ( 0 );
	}

	data += 2 + 16 * count;
	length -= 2 + 16 * count;

	return ( ( ! length ) || ( ( length == 4 ) && ( ! memcmp( data, bip, sizeof( bip ) ) ) ) );
}


// --

int32_t
RSNX_Supports_H2E( const uint8_t *data, uint32_t length )
{
	return ( ( length >= 3 ) && ( length <= 18 ) && ( data[0] == 244 ) &&
		( data[1] + 2U == length ) && ( ( data[2] & 0x0f ) + 3U == length ) && ( data[2] & 0x20 ) );
}

// --
const uint8_t AssociationWPA[USB2WPA_ASSOC_IE_SIZE] =
{
	221, 22, 0, 0x50, 0xf2, 1, 1, 0,
	0, 0x50, 0xf2, 2, // Group TKIP
	1, 0, 0, 0x50, 0xf2, 2, // Pairwise TKIP
	1, 0, 0, 0x50, 0xf2, 2 // PSK
};

// --

int32_t
WPA_Supports_TKIP( const uint8_t *data, uint32_t length )
{
	static const uint8_t suite[] = { 0, 0x50, 0xf2, 2 };

	if ( ( length < USB2WPA_ASSOC_IE_SIZE ) || ( data[0] != 221 ) || ( data[1] + 2U != length ) ||
		( memcmp( data + 2, AssociationWPA + 2, 10 ) ) )
	{
		return ( 0 );
	}

	data += 12;
	length -= 12;

	return ( ( Contains_Suite( & data, & length, suite ) ) &&
		( Contains_Suite( & data, & length, suite ) ) && ( ! length || length == 2 ) );
}

// --

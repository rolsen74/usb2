
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define DHCP_OPTIONS 240
#define DHCP_MESSAGE_TYPE 53
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

// --

U32
RT2870_DHCPType( const U8 *packet, U32 length, const U8 *station, S32 receive )
{
	static const U8 cookie[] = { 99, 130, 83, 99 };
	const U8 *udp;
	const U8 *bootp;
	U32 header;
	U32 total;
	U32 source;
	U32 destination;
	U32 offset;
	U32 option;
	U32 size;

	if ( ( length < 20 ) || ( ( packet[0] >> 4 ) != 4 ) || ( packet[9] != 17 ) ||
		( packet[6] & 0x3f ) || ( packet[7] ) )
	{
		return ( 0 );
	}

	header = ( packet[0] & 15 ) * 4;
	total = ( (U32) packet[2] << 8 ) | packet[3];

	if ( ( header < 20 ) || ( total > length ) || ( total < header + 8 + DHCP_OPTIONS ) )
	{
		return ( 0 );
	}

	udp = packet + header;
	source = ( (U32) udp[0] << 8 ) | udp[1];
	destination = ( (U32) udp[2] << 8 ) | udp[3];
	length = ( (U32) udp[4] << 8 ) | udp[5];

	if ( ( source != ( receive ? DHCP_SERVER_PORT : DHCP_CLIENT_PORT ) ) ||
		( destination != ( receive ? DHCP_CLIENT_PORT : DHCP_SERVER_PORT ) ) ||
		( length < 8 + DHCP_OPTIONS ) || ( length > total - header ) )
	{
		return ( 0 );
	}

	bootp = udp + 8;
	length -= 8;

	if ( ( bootp[0] != ( receive ? 2 : 1 ) ) || ( bootp[1] != 1 ) || ( bootp[2] != 6 ) ||
		( memcmp( bootp + 28, station, 6 ) ) || ( memcmp( bootp + 236, cookie, sizeof( cookie ) ) ) )
	{
		return ( 0 );
	}

	offset = DHCP_OPTIONS;

	while( offset < length )
	{
		option = bootp[offset++];

		if ( option == 0 )
		{
			continue;
		}

		if ( ( option == 255 ) || ( offset == length ) )
		{
			break;
		}

		size = bootp[offset++];

		if ( size > length - offset )
		{
			break;
		}

		if ( ( option == DHCP_MESSAGE_TYPE ) && ( size == 1 ) && ( bootp[offset] <= 8 ) )
		{
			return ( bootp[offset] );
		}

		offset += size;
	}

	return ( 0 );
}

// --

void
RT2870_ReportDHCP( U32 *seen, U32 type, const char *result )
{
	static const char * const names[] = { "", "DISCOVER", "OFFER", "REQUEST", "DECLINE", "ACK", "NAK", "RELEASE", "INFORM" };

	if ( ( type ) && ( ! ( *seen & ( 1U << type ) ) ) )
	{
		*seen |= 1U << type;

		USBINFO( "RT2870: DHCP %s %s", names[type], result );
	}
}

// --


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
PrintSuite( const U8 *suite, const U8 *oui, S32 akm )
{
	const char *name;

	name = NULL;

	if ( ! memcmp( suite, oui, 3 ) )
	{
		if ( akm )
		{
			switch( suite[3] )
			{
				case 1: name = "802.1X"; break;
				case 2: name = "PSK"; break;
				case 8: name = "SAE"; break;
			}
		}
		else
		{
			switch( suite[3] )
			{
				case 1: name = "WEP-40"; break;
				case 2: name = "TKIP"; break;
				case 4: name = "CCMP"; break;
				case 5: name = "WEP-104"; break;
				case 8: name = "GCMP"; break;
			}
		}
	}

	if ( name )
	{
		fputs( name, stderr );
	}
	else
	{
		fprintf( stderr, "%02x-%02x-%02x/%u", (unsigned) suite[0], (unsigned) suite[1],
			(unsigned) suite[2], (unsigned) suite[3] );
	}
}

// --

static S32
PrintSuiteList( const U8 **data, U32 *length, const U8 *oui, S32 akm )
{
	U32 count;
	U32 i;

	if ( *length < 2 )
	{
		return ( FALSE );
	}

	count = ( *data )[0] | ( (U32) ( *data )[1] << 8 );
	*data += 2;
	*length -= 2;

	if ( ( ! count ) || ( count > *length / 4 ) )
	{
		return ( FALSE );
	}

	for( i = 0; i < count; i++ )
	{
		if ( i )
		{
			fputs( ", ", stderr );
		}

		PrintSuite( *data, oui, akm );

		*data += 4;
		*length -= 4;
	}

	return ( TRUE );
}

// --

static void
PrintSecurityIE( const char *name, const U8 *ie, U32 length, U32 version, const U8 *oui )
{
	fprintf( stderr, "%s: ", name );

	if ( ! length )
	{
		fputs( "not advertised\n", stderr );

		return;
	}

	if ( ( length < version + 6 ) || ( ie[version] != 1 ) || ( ie[version + 1] ) )
	{
		fputs( "malformed security element\n", stderr );

		return;
	}

	ie += version + 2;
	length -= version + 2;

	fputs( "group ", stderr );

	PrintSuite( ie, oui, FALSE );

	ie += 4;
	length -= 4;

	fputs( ", pairwise ", stderr );

	if ( ! PrintSuiteList( & ie, & length, oui, FALSE ) )
	{
		fputs( "malformed\n", stderr );

		return;
	}

	fputs( ", authentication ", stderr );

	if ( ! PrintSuiteList( & ie, & length, oui, TRUE ) )
	{
		fputs( "malformed", stderr );
	}

	fputc( '\n', stderr );
}

// --

void
Wireless_Report_Security( const struct Wireless *wireless )
{
	static const U8 wpa[] = { 0, 0x50, 0xf2 };
	static const U8 rsn[] = { 0, 0x0f, 0xac };
	const struct AccessPoint *ap;

	ap = & wireless->RejectedAP;

	fprintf( stderr, "Security advertised by %02x:%02x:%02x:%02x:%02x:%02x on channel %u\n",
		(unsigned) ap->BSSID[0], (unsigned) ap->BSSID[1], (unsigned) ap->BSSID[2],
		(unsigned) ap->BSSID[3], (unsigned) ap->BSSID[4], (unsigned) ap->BSSID[5],
		(unsigned) ap->Channel );

	PrintSecurityIE( "WPA", ap->WPAIE, ap->WPAIELength, 6, wpa );

	PrintSecurityIE( "RSN (WPA2/WPA3)", ap->RSN, ap->RSNLength, 2, rsn );

	if ( ( wireless->UseWPA ) && ( ap->RSNLength >= 2 ) &&
		( RSN_Supports_WPA2( ap->RSN + 2, ap->RSNLength - 2 ) ) )
	{
		fputs( "This access point offers WPA2-Personal with CCMP. Use WPA2 to select it.\n", stderr );
	}
}

// --

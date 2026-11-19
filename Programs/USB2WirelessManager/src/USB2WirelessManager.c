
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static const char version[] __attribute__((used)) =
	"\0$VER: USB2WirelessManager 0.2 (19.11.2026)";

// --

static S32
Run_Wireless( struct Wireless *wireless, U32 channel, S32 open, S32 wpa2 )
{
	static const U8 empty[WIRELESS_ADDR_SIZE] = { 0 };

	enum WirelessResult result;
	U32 last;
	U32 index;

	last = channel ? channel : WIRELESS_MAX_CHANNEL;

	for( index = channel ? channel : 1; index <= last; index++ )
	{
		wireless->AP.Channel = index;

		if ( ! Wireless_Set_Options( wireless, index, empty, 0, S2ENC_NONE, FALSE ) )
		{
			return ( RETURN_FAIL );
		}

		result = Wireless_Wait_Frame( wireless, 450000, NULL, 0, Wireless_Match_Beacon, FALSE );

		if ( result == WIRELESS_MATCH )
		{
			break;
		}

		if ( result != WIRELESS_TIMEOUT )
		{
			return ( result == WIRELESS_STOPPED ? RETURN_OK : RETURN_FAIL );
		}
	}

	if ( index > last )
	{
		fprintf( stderr, "No matching access point heard on the selected channels.\n" );

		return ( RETURN_WARN );
	}

	printf( "Found %02x:%02x:%02x:%02x:%02x:%02x on channel %u (%s).\n",
		(unsigned) wireless->AP.BSSID[0], (unsigned) wireless->AP.BSSID[1], (unsigned) wireless->AP.BSSID[2],
		(unsigned) wireless->AP.BSSID[3], (unsigned) wireless->AP.BSSID[4], (unsigned) wireless->AP.BSSID[5],
		(unsigned) index, wireless->AP.Protected ? "protected" : "open" );

	if ( ( ! open ) && ( ! wpa2 ) )
	{
		return ( RETURN_OK );
	}

	if ( ( open ) && ( wireless->AP.Protected ) )
	{
		fprintf( stderr, "This network is protected. Use WPA2 for WPA2-Personal.\n" );

		return ( RETURN_WARN );
	}

	if ( ! wireless->AP.RatesSupported )
	{
		fprintf( stderr, "This access point requires rates beyond the current RTL8188EU CCK support.\n" );

		return ( RETURN_WARN );
	}

	if ( wpa2 )
	{
		result = WPA2_Open( wireless );

		if ( result != WIRELESS_MATCH )
		{
			return ( result == WIRELESS_STOPPED ? RETURN_OK : RETURN_FAIL );
		}
	}

	memcpy( wireless->Command->ios2_SrcAddr, wireless->Station, sizeof( wireless->Station ) );

	if ( ! Wireless_Command( wireless, S2_CONFIGINTERFACE ) )
	{
		return ( RETURN_FAIL );
	}

	result = Wireless_Connect( wireless, index );

	if ( result != WIRELESS_MATCH )
	{
		return ( result == WIRELESS_STOPPED ? RETURN_OK : RETURN_FAIL );
	}

	if ( wpa2 )
	{
		result = WPA2_Handshake( wireless );

		if ( result != WIRELESS_MATCH )
		{
			return ( result == WIRELESS_STOPPED ? RETURN_OK : RETURN_FAIL );
		}
	}

	printf( "%s on channel %u. Start the TCP/IP interface for %s unit %lu. Ctrl-C disconnects.\n",
		wpa2 ? "WPA2 connected" : "Associated", (unsigned) index, WIRELESS_DEVICE,
		(unsigned long) wireless->UnitNumber );

	return ( ( wpa2 ? WPA2_Wait_Link( wireless ) : Wireless_Wait_Link( wireless ) ) ? RETURN_OK : RETURN_WARN );
}

// --

int
main( void )
{
	struct RDArgs *args;
	struct Wireless wireless;

	LONG values[5];
	LONG channel;
	LONG unit;
	S32 result;

	memset( values, 0, sizeof( values ) );

	args = ReadArgs( "SSID/A,OPEN/S,WPA2/S,CHANNEL/K/N,UNIT/K/N", values, NULL );

	if ( ! args )
	{
		PrintFault( IoErr(), "USB2WirelessManager" );

		return ( RETURN_FAIL );
	}

	memset( & wireless, 0, sizeof( wireless ) );

	wireless.SSID = (const char *) values[0];
	wireless.SSIDLength = strlen( wireless.SSID );
	channel = values[3] ? *(const LONG *) values[3] : 0;
	unit = values[4] ? *(const LONG *) values[4] : 0;
	result = RETURN_FAIL;

	if ( ( ! wireless.SSIDLength ) || ( wireless.SSIDLength > WIRELESS_MAX_SSID ) ||
		( channel < 0 ) || ( channel > WIRELESS_MAX_CHANNEL ) || ( unit < 0 ) || ( unit > 31 ) )
	{
		fprintf( stderr, "Use an SSID of 1 to 32 bytes, channel 1 to 11, and unit 0 to 31.\n" );
	}
	else if ( ( values[1] ) && ( values[2] ) )
	{
		fprintf( stderr, "Choose either OPEN or WPA2.\n" );
	}
	else
	{
		wireless.UnitNumber = unit;

		if ( Wireless_Open( & wireless ) )
		{
			result = Run_Wireless( & wireless, channel, values[1] != 0, values[2] != 0 );
		}
	}

	Wireless_Close( & wireless );

	WPA2_Close( & wireless );

	FreeArgs( args );

	return ( result );
}

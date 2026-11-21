
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#ifndef USB2WIFI_EMBEDDED
static const char version[] __attribute__((used)) =
	"\0$VER: USB2WirelessManager 0.34 (21.11.2026)";
#endif

// --

S32
Valid_Password( const char *password, U32 minimum )
{
	U32 length;

	length = 0;

	while( password[length] )
	{
		if ( ( length == 63 ) || ( (U8) password[length] < 32 ) || ( (U8) password[length] > 126 ) )
		{
			return ( FALSE );
		}

		length++;
	}

	return ( length >= minimum );
}

// --

S32
Run_Wireless( struct Wireless *wireless, U32 channel, U32 maxChannel, S32 open, S32 auth, S32 associate )
{
	static const U8 empty[WIRELESS_ADDR_SIZE] = { 0 };

	enum WirelessResult result;
	U32 listenTime;
	U32 last;
	U32 index;
	U32 scan;
	U32 preferred;
	U32 probeLength;
	U8 probe[WIRELESS_80211_HEADER + 2 + WIRELESS_MAX_SSID + 16];
	S32 wpa2;

	if ( ( wireless->Hidden ) && ( wireless->ScanRT2870 ) &&
		( wireless->Command->ios2_Req.io_Device->dd_Library.lib_Version == 1 ) &&
		( wireless->Command->ios2_Req.io_Device->dd_Library.lib_Revision < 25 ) )
	{
		fprintf( stderr, "Hidden networks need usb2rt2870.device 1.25 or later.\n" );

		return ( RETURN_FAIL );
	}

	preferred = wireless->Auto ? channel : 0;
	last = ( channel && ! preferred ) ? channel : maxChannel;
	index = 0;
	listenTime = wireless->ScanRT2870 ? 900000 : 450000;

	for( scan = preferred ? 0 : ( channel ? channel : 1 ); scan <= last; scan++ )
	{
		if ( ( preferred ) && ( scan == preferred ) )
		{
			continue;
		}

		index = scan ? scan : preferred;
		wireless->AP.Channel = index;

		if ( ! Wireless_Set_Options( wireless, index, empty, 0, S2ENC_NONE, FALSE ) )
		{
			return ( RETURN_FAIL );
		}

		probeLength = wireless->Hidden ? Wireless_ProbeRequest( wireless, probe ) : 0;

		result = Wireless_Wait_Frame( wireless, listenTime, probeLength ? probe : NULL, probeLength, Wireless_Match_Beacon, FALSE );

		if ( result == WIRELESS_MATCH )
		{
			break;
		}

		if ( result != WIRELESS_TIMEOUT )
		{
			return ( result == WIRELESS_STOPPED ? RETURN_OK : RETURN_FAIL );
		}
	}

	if ( scan > last )
	{
		if ( wireless->ScanRT2870 )
		{
			Wireless_Report_Scan( wireless );
		}

		if ( wireless->ScanSecurityMismatch )
		{
			fprintf( stderr, "No matching access point offers the requested Wi-Fi security.\n" );

			Wireless_Report_Security( wireless );
		}
		else
		{
			fprintf( stderr, "No matching access point heard on the selected channels.\n" );
		}

		return ( RETURN_WARN );
	}

	printf( "Found %02x:%02x:%02x:%02x:%02x:%02x on channel %u (%s).\n",
		(unsigned) wireless->AP.BSSID[0], (unsigned) wireless->AP.BSSID[1], (unsigned) wireless->AP.BSSID[2],
		(unsigned) wireless->AP.BSSID[3], (unsigned) wireless->AP.BSSID[4], (unsigned) wireless->AP.BSSID[5],
		(unsigned) index, wireless->AP.Protected ? "protected" : "open" );

	if ( associate )
	{
		result = WPA2_Test_Association( wireless );

		return ( ( result == WIRELESS_MATCH || result == WIRELESS_STOPPED ) ? RETURN_OK : RETURN_FAIL );
	}

	if ( auth )
	{
		result = Wireless_Authenticate( wireless );

		if ( result == WIRELESS_MATCH )
		{
			printf( "802.11 authentication accepted on channel %u.\n", (unsigned) index );
		}

		return ( ( result == WIRELESS_MATCH || result == WIRELESS_STOPPED ) ? RETURN_OK : RETURN_FAIL );
	}

	if ( wireless->DetectSecurity )
	{
		open = wireless->Auto->Profile.Security == USB2WIFI_OPEN;

		if ( ( ! open ) && ( wireless->Auto->Profile.Password[0] ) &&
			( ! Valid_Password( wireless->Auto->Profile.Password, wireless->UseSAE ? 1 : 8 ) ) )
		{
			wireless->RetryPassword = TRUE;

			return ( RETURN_FAIL );
		}
	}

	wpa2 = wireless->AllowWPA2 && ! wireless->UseSAE;

	if ( ( ! open ) && ( ! wpa2 ) && ( ! wireless->UseSAE ) && ( ! wireless->UseWPA ) )
	{
		return ( RETURN_OK );
	}

	if ( ( open ) && ( wireless->AP.Protected ) )
	{
		fprintf( stderr, "This network is protected. Use WPA, WPA2 or WPA3 to match its security.\n" );

		return ( RETURN_WARN );
	}

	if ( ( ! wireless->ScanRT2870 ) && ( ! wireless->AP.RatesSupported ) )
	{
		fprintf( stderr, "This access point requires rates beyond the current RTL8188EU CCK support.\n" );

		return ( RETURN_WARN );
	}

	if ( ( wireless->AllowWPA2 ) && ( wireless->AllowWPA3 ) )
	{
		printf( "Selected %s.\n", wireless->UseSAE ? "WPA3" : "WPA2" );

		if ( ( wpa2 ) && ( wireless->Password ) && ( ! Valid_Password( wireless->Password, 8 ) ) )
		{
			fprintf( stderr, "WPA2 requires a passphrase of at least 8 characters.\n" );

			return ( RETURN_FAIL );
		}
	}

	if ( ( wpa2 ) || ( wireless->UseWPA ) )
	{
		result = PSK_Open( wireless );

		if ( result != WIRELESS_MATCH )
		{
			return ( result == WIRELESS_STOPPED ? RETURN_OK : RETURN_FAIL );
		}
	}

	if ( ( ! wireless->ScanRT2870 ) && ( wpa2 || wireless->UseSAE || wireless->UseWPA ) &&
		( ! Wireless_Set_Copy_Hook( wireless ) ) )
	{
		return ( RETURN_FAIL );
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

	if ( ( wpa2 ) || ( wireless->UseSAE ) || ( wireless->UseWPA ) )
	{
		result = wireless->UseSAE ? WPA3_Handshake( wireless ) : PSK_Handshake( wireless );
		wireless->RetryPassword = result == WIRELESS_TIMEOUT;

		if ( result != WIRELESS_MATCH )
		{
			return ( result == WIRELESS_STOPPED ? RETURN_OK : RETURN_FAIL );
		}
	}

	if ( wireless->Auto )
	{
		WiFi_Connected( wireless, index );
	}

	printf( "%s on channel %u. Start the TCP/IP interface for %s unit %lu. Ctrl-C disconnects.\n",
		wireless->UseSAE ? "WPA3 connected" : ( wireless->UseWPA ? "WPA connected" : ( wpa2 ? "WPA2 connected" : "Associated" ) ), (unsigned) index, wireless->DeviceName,
		(unsigned long) wireless->UnitNumber );

	if ( wireless->UseSAE )
	{
		return ( WPA3_Wait_Link( wireless ) ? RETURN_OK : RETURN_WARN );
	}

	return ( ( ( wpa2 || wireless->UseWPA ) ? PSK_Wait_Link( wireless ) : Wireless_Wait_Link( wireless ) ) ? RETURN_OK : RETURN_WARN );
}

// --

#ifndef USB2WIFI_EMBEDDED
static S32
EditProfiles( void )
{
	struct Library *base;
	struct USB2WiFiIFace *wifi;

	S32 result;

	base = OpenLibrary( USB2WIFI_NAME, USB2WIFI_VERSION );
	result = RETURN_FAIL;

	if ( ( base ) && ( ( base->lib_Version > USB2WIFI_VERSION ) || ( base->lib_Revision >= USB2WIFI_EDITOR_REVISION ) ) )
	{
		wifi = (struct USB2WiFiIFace *)GetInterface( base, "main", 1, NULL );

		if ( wifi )
		{
			result = wifi->EditProfiles();

			DropInterface( (struct Interface *)wifi );
		}
	}
	else
	{
		fprintf( stderr, "Install LIBS:%s 1.24 or later.\n", USB2WIFI_NAME );
	}

	if ( base )
	{
		CloseLibrary( base );
	}

	return ( result );
}

// --

int
main( int argc, char **argv __attribute__((unused)) )
{
	struct RDArgs *args;
	struct Wireless wireless;

	const char *device;
	LONG values[13];
	LONG maxChannel;
	LONG channel;
	LONG unit;
	S32 rt2870;
	S32 result;

	if ( argc <= 1 )
	{
		return ( EditProfiles() );
	}

	memset( values, 0, sizeof( values ) );

	args = ReadArgs( "SSID,OPEN/S,WPA2/S,CHANNEL/K/N,UNIT/K/N,DEVICE/K,AUTH/S,ASSOCIATE/S,PASSWORD/K,WPA3/S,WPA/S,AUTO/S,SETUP/S", values, NULL );

	if ( ! args )
	{
		PrintFault( IoErr(), "USB2WirelessManager" );

		return ( RETURN_FAIL );
	}

	if ( ( values[11] ) || ( values[12] ) )
	{
		device = values[5] ? (const char *) values[5] : WIRELESS_DEVICE;
		unit = values[4] ? *(const LONG *) values[4] : 0;

		if ( ( values[11] && values[12] ) || ( values[0] ) || ( values[1] ) || ( values[2] ) ||
			( values[3] ) || ( values[6] ) || ( values[7] ) || ( values[8] ) || ( values[9] ) || ( values[10] ) ||
			( unit < 0 ) || ( unit > 31 ) ||
			( strcmp( device, WIRELESS_DEVICE ) && strcmp( device, WIRELESS_RT2870_DEVICE ) ) )
		{
			fprintf( stderr, "Use AUTO or SETUP with only DEVICE and UNIT.\n" );

			result = RETURN_FAIL;
		}
		else
		{
			result = WiFi_Run( device, unit, values[12] != 0 );
		}

		FreeArgs( args );

		return ( result );
	}

	if ( ! values[0] )
	{
		fprintf( stderr, "Specify an SSID, AUTO or SETUP.\n" );

		FreeArgs( args );

		return ( RETURN_FAIL );
	}

	memset( & wireless, 0, sizeof( wireless ) );

	wireless.SSID = (const char *) values[0];
	wireless.SSIDLength = strlen( wireless.SSID );
	wireless.Password = (char *) values[8];
	wireless.AllowWPA2 = values[2] != 0;
	wireless.AllowWPA3 = values[9] != 0;
	wireless.UseWPA = values[10] != 0;
	wireless.UseSAE = wireless.AllowWPA3;
	channel = values[3] ? *(const LONG *) values[3] : 0;
	unit = values[4] ? *(const LONG *) values[4] : 0;
	device = values[5] ? (const char *) values[5] : WIRELESS_DEVICE;
	wireless.DeviceName = device;
	rt2870 = ! strcmp( device, WIRELESS_RT2870_DEVICE );
	wireless.ScanRT2870 = rt2870;
	maxChannel = rt2870 ? 13 : WIRELESS_MAX_CHANNEL;
	result = RETURN_FAIL;

	if ( ( ! wireless.SSIDLength ) || ( wireless.SSIDLength > WIRELESS_MAX_SSID ) ||
		( channel < 0 ) || ( channel > maxChannel ) || ( unit < 0 ) || ( unit > 31 ) )
	{
		fprintf( stderr, "Use an SSID of 1 to 32 bytes, channel 1 to %ld, and unit 0 to 31.\n", (long) maxChannel );
	}
	else if ( ( wireless.UseWPA ) &&
		( ( values[1] ) || ( values[6] ) || ( values[7] ) || ( wireless.AllowWPA2 ) || ( wireless.AllowWPA3 ) ) )
	{
		fprintf( stderr, "Use WPA on its own.\n" );
	}
	else if ( ( values[1] ) && ( ( wireless.AllowWPA2 ) || ( wireless.AllowWPA3 ) ) )
	{
		fprintf( stderr, "OPEN cannot be combined with WPA2 or WPA3.\n" );
	}
	else if ( ( wireless.Password ) && ( ! wireless.UseWPA ) && ( ! wireless.AllowWPA2 ) && ( ! wireless.AllowWPA3 ) )
	{
		fprintf( stderr, "PASSWORD requires WPA, WPA2 or WPA3.\n" );
	}
	else if ( ( wireless.Password ) && ( ! Valid_Password( wireless.Password, wireless.UseSAE ? 1 : 8 ) ) )
	{
		fprintf( stderr, "PASSWORD must contain %u to 63 printable ASCII characters.\n", wireless.UseSAE ? 1U : 8U );
	}
	else if ( ( ! rt2870 ) && ( strcmp( device, WIRELESS_DEVICE ) ) )
	{
		fprintf( stderr, "DEVICE must be %s or %s.\n", WIRELESS_DEVICE, WIRELESS_RT2870_DEVICE );
	}
	else if ( ( ( values[6] ) || ( values[7] ) ) &&
		( ( ! rt2870 ) || ( values[1] ) || ( wireless.AllowWPA2 ) || ( wireless.AllowWPA3 ) || ( ( values[6] ) && ( values[7] ) ) ) )
	{
		fprintf( stderr, "Use either AUTH or ASSOCIATE on its own with the RT2870.\n" );
	}
	else if ( ( rt2870 ) && ( values[1] ) )
	{
		fprintf( stderr, "%s supports scans, AUTH, ASSOCIATE, WPA2 and WPA3. OPEN is unavailable.\n", device );
	}
	else
	{
		wireless.UnitNumber = unit;

		if ( Wireless_Open( & wireless ) )
		{
			result = Run_Wireless( & wireless, channel, maxChannel, values[1] != 0, values[6] != 0, values[7] != 0 );
		}
	}

	Wireless_Close( & wireless );

	WPA3_Close( & wireless );

	PSK_Close( & wireless );

	FreeArgs( args );

	return ( result );
}

#endif

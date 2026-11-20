
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

enum WirelessResult
WiFi_Passphrase( struct Wireless *wireless, char *input, char **password, U32 *length )
{
	struct AutoWiFi *wifi;

	U32 action;
	S32 result;

	wifi = wireless->Auto;

	if ( ! wifi->Profile.Password[0] )
	{
		result = Wireless_PromptPassword( wireless, & action );

		if ( result != USB2WIFI_OK )
		{
			return ( result == USB2WIFI_MISSING ? WIRELESS_STOPPED : WIRELESS_IO_ERROR );
		}

		wifi->SavePassword = action == USB2WIFI_SAVE_PASSWORD;
	}

	*length = strlen( wifi->Profile.Password );
	*password = input;

	memcpy( input, wifi->Profile.Password, *length );

	return ( WIRELESS_MATCH );
}

// --

void
WiFi_Connected( struct Wireless *wireless, U32 channel )
{
	struct AutoWiFi *wifi;

	S32 result;

	wifi = wireless->Auto;
	wifi->Profile.Channel = channel;

	if ( wifi->SetupConnect )
	{
		result = wifi->Interface->Save( wifi->Adapter, wifi->Unit, & wifi->Profile,
			wifi->SavePassword ? USB2WIFI_SAVE_PASSWORD : USB2WIFI_KEEP_PASSWORD );

		wifi->SetupConnect = FALSE;
	}
	else
	{
		result = wifi->Interface->Connected( wifi->Adapter, wifi->Unit, & wifi->Profile, wifi->SavePassword );
	}

	if ( result == USB2WIFI_ERROR )
	{
		fprintf( stderr, "Connected, but could not update ENV:usb2wifi.\n" );
	}
	else if ( ( result == USB2WIFI_MISSING ) && ( wifi->SavePassword ) )
	{
		fprintf( stderr, "Connected, but the saved Wi-Fi profile changed. The password was kept in memory.\n" );
	}

	wifi->SavePassword = FALSE;
}

// --

static S32
ConnectProfile( struct AutoWiFi *wifi, const char *device, U32 action )
{
	struct Wireless wireless;

	S32 result;
	U32 maxChannel;

	wifi->SavePassword = action == USB2WIFI_SAVE_PASSWORD;
	wifi->SetupConnect = TRUE;
	wifi->DetectSecurity = wifi->Profile.Security == WIFI_SECURITY_UNSUPPORTED;

	printf( "Connecting using %s unit %lu for SSID \"%s\".\n", device,
		(unsigned long) wifi->Unit, wifi->Profile.SSID );

	memset( & wireless, 0, sizeof( wireless ) );

	wireless.Auto = wifi;
	wireless.UnitNumber = wifi->Unit;
	wireless.DeviceName = device;
	wireless.SSID = wifi->Profile.SSID;
	wireless.SSIDLength = strlen( wifi->Profile.SSID );
	wireless.ScanRT2870 = wifi->Adapter == USB2WIFI_RT2870;
	wireless.UseWPA = wifi->Profile.Security == USB2WIFI_WPA;
	wireless.AllowWPA2 = wifi->Profile.Security == USB2WIFI_WPA2;
	wireless.AllowWPA3 = wifi->Profile.Security == USB2WIFI_WPA3;
	wireless.UseSAE = wireless.AllowWPA3;
	wireless.Hidden = ( wifi->Profile.Options & USB2WIFI_PROFILE_HIDDEN ) != 0;
	wireless.DetectSecurity = wifi->DetectSecurity;
	maxChannel = wireless.ScanRT2870 ? 13 : WIRELESS_MAX_CHANNEL;
	result = RETURN_FAIL;

	if ( Wireless_Open( & wireless ) )
	{
		result = Run_Wireless( & wireless, wifi->Profile.Channel, maxChannel,
			wifi->Profile.Security == USB2WIFI_OPEN, FALSE, FALSE );
	}

	Wireless_Close( & wireless );

	WPA3_Close( & wireless );

	PSK_Close( & wireless );

	wifi->Retry = wireless.RetryPassword;

	if ( wifi->Retry )
	{
		wifi->SavePassword = FALSE;

		Clear_Secret( wifi->Profile.Password, sizeof( wifi->Profile.Password ) );
	}

	return ( result );
}

// --

S32
WiFi_Run( const char *device, U32 unit )
{
	struct AutoWiFi wifi;
	struct Library *base;

	U32 action;
	S32 retryPicker;
	S32 result;
	S32 status;

	memset( & wifi, 0, sizeof( wifi ) );

	base = OpenLibrary( USB2WIFI_NAME, USB2WIFI_VERSION );

	if ( ( ! base ) || ( base->lib_Version == USB2WIFI_VERSION && base->lib_Revision < USB2WIFI_PROFILE_REVISION ) )
	{
		fprintf( stderr, "Install LIBS:%s 1.4 or later.\n", USB2WIFI_NAME );

		if ( base )
		{
			CloseLibrary( base );
		}

		return ( RETURN_FAIL );
	}

	wifi.Interface = (struct USB2WiFiIFace *) GetInterface( base, "main", 1, NULL );
	result = RETURN_FAIL;

	if ( ! wifi.Interface )
	{
		fprintf( stderr, "Could not open the Wi-Fi library interface.\n" );

		goto done;
	}

	retryPicker = base->lib_Version > USB2WIFI_VERSION || base->lib_Revision >= USB2WIFI_RETRY_PICKER_REVISION;
	wifi.Adapter = ! strcmp( device, WIRELESS_RT2870_DEVICE ) ? USB2WIFI_RT2870 : USB2WIFI_RTL8188;
	wifi.Unit = unit;

	wifi.Claimed = wifi.Interface->Claim( wifi.Adapter, unit );

	if ( ! wifi.Claimed )
	{
		fprintf( stderr, "Wi-Fi setup or connection is already running for %s unit %lu.\n", device, (unsigned long) unit );
		result = RETURN_WARN;

		goto done;
	}

	status = wifi.Interface->Load( wifi.Adapter, unit, & wifi.Profile );

	if ( status == USB2WIFI_ERROR )
	{
		fprintf( stderr, "Could not read ENV:usb2wifi.\n" );

		goto done;
	}

	if ( status == USB2WIFI_MISSING )
	{
		wifi.Profile.Security = USB2WIFI_WPA3;
	}

	for( ;; )
	{
		status = ( wifi.Retry && retryPicker ) ?
			wifi.Interface->SetupRetry( & wifi.Profile, wifi.Adapter, unit, & action ) :
			wifi.Interface->Setup( & wifi.Profile, wifi.Adapter, unit, & action );

		if ( status != USB2WIFI_CONNECT_NOW )
		{
			break;
		}

		do
		{
			result = ConnectProfile( & wifi, device, action );
			action = USB2WIFI_KEEP_PASSWORD;
		}
		while( ( wifi.Retry ) && ( ! retryPicker ) );

		if ( ! wifi.Retry )
		{
			goto done;
		}
	}

	if ( status == USB2WIFI_OK )
	{
		status = wifi.Interface->Save( wifi.Adapter, unit, & wifi.Profile, action );

		if ( status != USB2WIFI_OK )
		{
			fprintf( stderr, "Could not save ENV:usb2wifi.\n" );
		}
	}

	result = status == USB2WIFI_ERROR ? RETURN_FAIL : RETURN_OK;

done:

	Clear_Secret( & wifi.Profile, sizeof( wifi.Profile ) );

	if ( wifi.Claimed )
	{
		wifi.Interface->Unclaim( wifi.Adapter, unit );
	}

	if ( wifi.Interface )
	{
		DropInterface( (struct Interface *) wifi.Interface );
	}

	CloseLibrary( base );

	return ( result );
}

// --

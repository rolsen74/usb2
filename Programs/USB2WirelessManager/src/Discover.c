
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
WiFi_Discover( U32 adapter, U32 unit, struct WiFiScan *scan )
{
	static const U8 empty[WIRELESS_ADDR_SIZE] = { 0 };
	struct Wireless wireless;
	enum WirelessResult result;

	U32 channel;
	U32 last;

	memset( & wireless, 0, sizeof( wireless ) );

	scan->Changed = 0;
	wireless.ScanNetworks = scan;
	wireless.UnitNumber = unit;
	wireless.ScanRT2870 = adapter == USB2WIFI_RT2870;
	wireless.DeviceName = wireless.ScanRT2870 ? WIRELESS_RT2870_DEVICE : WIRELESS_DEVICE;
	wireless.SSID = "";
	last = wireless.ScanRT2870 ? 13 : WIRELESS_MAX_CHANNEL;
	result = WIRELESS_IO_ERROR;

	if ( Wireless_Open( & wireless ) )
	{
		wireless.Command->ios2_Req.io_Command = S2_ONEVENT;
		wireless.Command->ios2_WireError = S2EVENT_ONLINE | S2EVENT_OFFLINE;

		DoIO( & wireless.Command->ios2_Req );

		if ( ( wireless.Command->ios2_Req.io_Error ) || ( wireless.Command->ios2_WireError & S2EVENT_ONLINE ) )
		{
			Wireless_Close( & wireless );

			return ( USB2WIFI_ERROR );
		}

		for( channel = 1; channel <= last; channel++ )
		{
			if ( ( scan->Update ) && ( ! scan->Update( scan->Context ) ) )
			{
				result = WIRELESS_STOPPED;

				break;
			}

			wireless.AP.Channel = channel;

			if ( ! Wireless_Set_Options( & wireless, channel, empty, 0, S2ENC_NONE, FALSE ) )
			{
				result = WIRELESS_IO_ERROR;
				break;
			}

			result = Wireless_Wait_Frame( & wireless, 340000, NULL, 0, Wireless_Match_Beacon, FALSE );

			if ( result != WIRELESS_TIMEOUT )
			{
				break;
			}
		}
	}

	Wireless_Close( & wireless );

	return ( result == WIRELESS_STOPPED ? USB2WIFI_MISSING :
		( result == WIRELESS_TIMEOUT ? USB2WIFI_OK : USB2WIFI_ERROR ) );
}

// --

#ifndef USB2WIFI_EMBEDDED

S32
Wireless_ScanSiS163U( U32 unit, U32 channel )
{
	struct Wireless wireless;
	struct WiFiScan scan;
	struct TagItem tags[2];
	enum WirelessResult result;

	U32 current;
	U32 last;
	S32 status;

	memset( & wireless, 0, sizeof( wireless ) );

	memset( & scan, 0, sizeof( scan ) );

	wireless.DeviceName = WIRELESS_SIS163U_DEVICE;
	wireless.UnitNumber = unit;
	wireless.ScanNetworks = & scan;
	wireless.SSID = "";
	last = channel ? channel : 13;
	status = RETURN_FAIL;

	if ( ! Wireless_Open( & wireless ) )
	{
		goto done;
	}

	if ( ( wireless.Command->ios2_Req.io_Device->dd_Library.lib_Version == 1 ) &&
		( wireless.Command->ios2_Req.io_Device->dd_Library.lib_Revision < 12 ) )
	{
		fprintf( stderr, "SCAN requires usb2sis163u.device 1.12 or later.\n" );

		goto done;
	}

	tags[0].ti_Tag = S2INFO_Channel;
	tags[1].ti_Tag = TAG_END;
	tags[1].ti_Data = 0;

	for( current = channel ? channel : 1; current <= last; current++ )
	{
		wireless.AP.Channel = current;
		tags[0].ti_Data = current;
		wireless.Command->ios2_Data = tags;

		if ( ! Wireless_Command( & wireless, S2_SETOPTIONS ) )
		{
			goto done;
		}

		result = Wireless_Wait_Frame( & wireless, 650000, NULL, 0, Wireless_Match_Beacon, FALSE );

		if ( result == WIRELESS_STOPPED )
		{
			fprintf( stderr, "Scan stopped.\n" );

			status = RETURN_OK;
			goto done;
		}

		if ( result != WIRELESS_TIMEOUT )
		{
			goto done;
		}
	}

	fprintf( stderr, "SiS163U passive scan found %lu SSIDs.\n", (unsigned long) scan.Count );

	status = scan.Count ? RETURN_OK : RETURN_WARN;

done:
	Wireless_Report_Networks( & scan );

	Wireless_Close( & wireless );

	return ( status );
}

#endif

// --


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

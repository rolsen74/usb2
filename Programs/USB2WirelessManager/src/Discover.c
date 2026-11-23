
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#ifdef USB2WIFI_EMBEDDED

static S32
WaitForPicker( struct Wireless *wireless, struct WiFiScan *scan )
{
	struct IOSana2Req *event;

	U32 mask;
	S32 result;

	event = wireless->Receive;
	mask = ( 1UL << wireless->ReceivePort->mp_SigBit ) | scan->InputMask | SIGBREAKF_CTRL_C;
	result = USB2WIFI_OK;
	event->ios2_Req.io_Command = S2_ONEVENT;
	event->ios2_Req.io_Flags = 0;
	event->ios2_WireError = S2EVENT_HARDWARE;

	SendIO( & event->ios2_Req );

	while ( ! CheckIO( & event->ios2_Req ) )
	{
		if ( ! scan->Update( scan->Context ) )
		{
			break;
		}

		if ( Wait( mask ) & SIGBREAKF_CTRL_C )
		{
			result = USB2WIFI_MISSING;
			break;
		}
	}

	if ( CheckIO( & event->ios2_Req ) )
	{
		result = USB2WIFI_MISSING;
	}
	else
	{
		AbortIO( & event->ios2_Req );
	}

	WaitIO( & event->ios2_Req );

	// A hardware event can win the race with closing the picker.
	if ( ( ( ! event->ios2_Req.io_Error ) && ( event->ios2_WireError & S2EVENT_HARDWARE ) ) ||
		( ( event->ios2_Req.io_Error == S2ERR_OUTOFSERVICE ) &&
			( event->ios2_WireError == S2WERR_UNIT_DISCONNECTED ) ) )
	{
		result = USB2WIFI_MISSING;
	}

	return ( result );
}

// --

S32
WiFi_PickNetwork( U32 adapter, U32 unit, struct WiFiScan *scan )
{
	static const U8 empty[WIRELESS_ADDR_SIZE] = { 0 };
	struct Wireless wireless;
	enum WirelessResult scanResult;

	U32 channel;
	U32 last;
	S32 result;

	memset( & wireless, 0, sizeof( wireless ) );

	scan->Changed = 0;
	wireless.ScanNetworks = scan;
	wireless.UnitNumber = unit;
	wireless.ScanRT2870 = adapter == USB2WIFI_RT2870;
	wireless.AdapterType = wireless.ScanRT2870 ? USB2ETH_ADAPTER_RT2870 : USB2ETH_ADAPTER_RTL8188;
	wireless.SSID = "";
	last = wireless.ScanRT2870 ? 13 : WIRELESS_MAX_CHANNEL;
	result = USB2WIFI_MISSING;
	scanResult = WIRELESS_TIMEOUT;

	if ( Wireless_Open( & wireless ) )
	{
		wireless.Command->ios2_Req.io_Command = S2_ONEVENT;
		wireless.Command->ios2_WireError = S2EVENT_ONLINE | S2EVENT_OFFLINE;

		DoIO( & wireless.Command->ios2_Req );

		if ( ( ! wireless.Command->ios2_Req.io_Error ) && ( ! ( wireless.Command->ios2_WireError & S2EVENT_ONLINE ) ) )
		{
			for( channel = 1; channel <= last; channel++ )
			{
				if ( ! scan->Update( scan->Context ) )
				{
					scanResult = WIRELESS_STOPPED;

					break;
				}

				wireless.AP.Channel = channel;

				if ( ! Wireless_Set_Options( & wireless, channel, empty, 0, S2ENC_NONE, FALSE ) )
				{
					break;
				}

				scanResult = Wireless_Wait_Frame( & wireless, 340000, NULL, 0, Wireless_Match_Beacon, FALSE );

				if ( scanResult != WIRELESS_TIMEOUT )
				{
					break;
				}
			}
		}

		if ( scanResult == WIRELESS_STOPPED )
		{
			result = USB2WIFI_OK;
		}
		else
		{
			Wireless_Reset_Options( & wireless );

			// Keep this open handle so a replacement on the same unit cannot own the picker.
			result = WaitForPicker( & wireless, scan );
		}
	}

	Wireless_Close( & wireless );

	return ( result );
}

// --

#else

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

	wireless.AdapterType = USB2ETH_ADAPTER_SIS163U;
	wireless.UnitNumber = unit;
	wireless.ScanNetworks = & scan;
	wireless.SSID = "";
	last = channel ? channel : 13;
	status = RETURN_FAIL;

	if ( ! Wireless_Open( & wireless ) )
	{
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

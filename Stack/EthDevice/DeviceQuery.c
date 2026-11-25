
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "EthDevice.h"

// --

// Baseline command set for discovery without a bound adapter.
SEC_RODATA static const U16 EthernetCommands[] =
{
	NSCMD_DEVICEQUERY,
	CMD_READ,
	CMD_WRITE,
	CMD_FLUSH,
	S2_DEVICEQUERY,
	S2_GETSTATIONADDRESS,
	S2_CONFIGINTERFACE,
	S2_ADDMULTICASTADDRESS,
	S2_DELMULTICASTADDRESS,
	S2_MULTICAST,
	S2_BROADCAST,
	S2_GETSPECIALSTATS,
	S2_GETGLOBALSTATS,
	S2_ADDMULTICASTADDRESSES,
	S2_DELMULTICASTADDRESSES,
	S2_TRACKTYPE,
	S2_UNTRACKTYPE,
	S2_GETTYPESTATS,
	S2_ONEVENT,
	S2_READORPHAN,
	S2_ONLINE,
	S2_OFFLINE,
	S2_GETEXTENDEDGLOBALSTATS,
	S2_SAMPLE_THROUGHPUT,
	S2_SANA2HOOK,
	0
};

// --

SEC_CODE S32
EthDevice_KnowsCommand( const U16 *commands, U16 command )
{
	if ( ! commands )
	{
		commands = EthernetCommands;
	}

	while ( *commands )
	{
		if ( *commands++ == command )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

SEC_CODE void
EthDevice_Query( struct IOStdReq *request, const U16 *commands )
{
	USB2_DeviceQuery( request, NSDEVTYPE_SANA2, commands ? commands : EthernetCommands );
}

// --

SEC_CODE void
EthDevice_Sana2Query( struct USBBase *usbbase, struct IOSana2Req *request )
{
	static const struct Sana2DeviceQuery info =
	{
		.AddrFieldSize = 48,
		.MTU = 1500,
		.HardwareType = S2WireType_Ethernet,
		.RawMTU = 1514
	};

	struct Sana2DeviceQuery *query;

	query = request->ios2_StatData;
	request->ios2_Req.io_Error = 0;
	request->ios2_WireError = 0;

	if ( ! query )
	{
		request->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		request->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	if ( query->SizeAvailable < offsetof( struct Sana2DeviceQuery, AddrFieldSize ) )
	{
		request->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		request->ios2_WireError = S2WERR_BAD_STATDATA;

		return;
	}

	query->SizeSupplied = query->SizeAvailable;

	if ( query->SizeSupplied > sizeof( info ) )
	{
		query->SizeSupplied = sizeof( info );
	}

	MEM_COPY( (PTR) & info.DevQueryFormat, & query->DevQueryFormat,
		query->SizeSupplied - offsetof( struct Sana2DeviceQuery, DevQueryFormat ) );
}

// --

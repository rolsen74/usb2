
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "EthDevice.h"

// --

// Commands common to the wired and wireless backends.
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

SEC_CODE void
EthDevice_Query( struct IOStdReq *request, const U16 *commands )
{
	USB2_DeviceQuery( request, NSDEVTYPE_SANA2, commands ? commands : EthernetCommands );
}

// --

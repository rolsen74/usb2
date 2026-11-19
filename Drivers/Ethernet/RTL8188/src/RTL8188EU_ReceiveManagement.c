
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Radio.h"

// --

void
RTL8188EU_ReceiveManagement( struct EthernetUnit *unit, U8 *frame, U32 length )
{
	struct IOSana2Req *request;

	if ( ( ( frame[0] == 0xa0 ) || ( frame[0] == 0xc0 ) ) && ( length >= 26 ) &&
		( ! memcmp( frame + 4, unit->unit_StationAddress, 6 ) ) &&
		( ! memcmp( frame + 10, unit->unit_BSSID, 6 ) ) &&
		( ! memcmp( frame + 16, unit->unit_BSSID, 6 ) ) )
	{
		USBINFO( "RTL8188EU: AP %s, reason %lu", frame[0] == 0xa0 ? "disassociated" : "deauthenticated",
			(U32) frame[24] | ( (U32) frame[25] << 8 ) );

		RTL8188EU_DropLink( unit );
	}

	request = (PTR) GetHead( & unit->unit_Pending );

	while( request )
	{
		if ( request->ios2_Req.io_Command == S2_READMGMT )
		{
			break;
		}

		request = (PTR) GetSucc( (PTR) request );
	}

	if ( ! request )
	{
		return;
	}

	Remove( (PTR) request );

	if ( request->ios2_DataLength < length )
	{
		request->ios2_Req.io_Error = IOERR_BADLENGTH;
		request->ios2_DataLength = 0;
	}
	else
	{
		memcpy( request->ios2_Data, frame, length );

		request->ios2_DataLength = length;
	}

	ReplyMsg( (PTR) request );
}

// --

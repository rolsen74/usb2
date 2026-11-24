
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE S32
Video_Stream_Open( struct VideoUnit *unit, const struct VideoMode *mode )
{
	struct USB2_DriverMessage *msg;

	U32 speed;
	U32 hcd;

	msg = unit->StartMsg;
	unit->Format = mode->Format;
	unit->FormatIndex = mode->FormatIndex;
	unit->FrameIndex = mode->FrameIndex;
	unit->EndPoint = unit->BulkEndPoint;
	unit->StreamAlt = 0;
	unit->StreamInterval = 0;
	unit->StreamPacketSize = 0;
	unit->IsoPacketCount = 0;
	unit->StreamConfigured = FALSE;

	if ( ! Video_Probe( unit ) )
	{
		return ( FALSE );
	}

	if ( ! unit->BulkEndPoint )
	{
		// Older stacks allocate shorter requests without isochronous results.
		if ( ( unit->Control->IORequests[0]->io_Message.mn_Length < sizeof( struct USB2_IORequest ) ) ||
			( ! Video_Select_Stream( unit, unit->PayloadSize ) ) )
		{
			return ( FALSE );
		}

		unit->IsoPacketCount = VIDEO_ISO_PACKETS;

		// Limit each high-speed batch to 32 ms regardless of the service interval.
		if ( ( unit->StreamInterval <= 3 ) &&
			( USB2_Attr_Get( msg->Function->FKT_ID, USB2Tag_Attr_Speed,
				& speed, sizeof( speed ) ) == sizeof( speed ) ) &&
			( speed == USBSPEED_High ) )
		{
			unit->IsoPacketCount = VIDEO_ISO_MAX_PACKETS >> ( unit->StreamInterval - 1 );

			if ( ( USB2_Attr_Get( msg->Function->FKT_ID, USB2Tag_Attr_HCDType,
				& hcd, sizeof( hcd ) ) != sizeof( hcd ) ) || ( hcd == HCDTYPE_XHCI ) )
			{
				unit->IsoPacketCount = MIN( unit->IsoPacketCount, 254 );
			}
		}
	}

	unit->StreamConfigured = TRUE;

	return ( TRUE );
}

// --

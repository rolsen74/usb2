
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

	msg = unit->StartMsg;
	unit->Format = mode->Format;
	unit->FormatIndex = mode->FormatIndex;
	unit->FrameIndex = mode->FrameIndex;
	unit->EndPoint = unit->BulkEndPoint;
	unit->StreamAlt = 0;
	unit->IsoPacketCount = 0;

	if ( ! Video_Probe( unit ) )
	{
		return ( FALSE );
	}

	if ( ! unit->BulkEndPoint )
	{
		// Older stacks allocate shorter requests without isochronous results.
		if ( ( unit->Control->IORequests[0]->io_Message.mn_Length < sizeof( struct USB2_IORequest ) ) ||
			( ! Video_Select_Stream( unit, unit->PayloadSize ) ) ||
			( ! USB2_Ifc_SetAlt( unit->Register, msg->Interface, unit->StreamAlt ) ) )
		{
			return ( FALSE );
		}

		unit->StreamAltActive = TRUE;
		unit->IsoPacketCount = VIDEO_ISO_PACKETS;

		// Short USB 2.0 reads leave gaps while EHCI schedules the next request.
		if ( ( unit->StreamInterval <= 3 ) &&
			( USB2_Attr_Get( msg->Function->FKT_ID, USB2Tag_Attr_Speed,
				& speed, sizeof( speed ) ) == sizeof( speed ) ) &&
			( speed == USBSPEED_High ) )
		{
			unit->IsoPacketCount = VIDEO_ISO_MAX_PACKETS;
		}
	}

	unit->Stream = USB2_EPRes_ObtainTags( unit->Register,
		USB2Tag_EPRes_MsgPort, unit->StreamPort,
		USB2Tag_EPRes_NrOfIORequest, VIDEO_USB_READS,
		USB2Tag_EPRes_InterfaceNr, unit->InterfaceNr,
		USB2Tag_EPRes_EndPointNr, unit->EndPoint,
		USB2Tag_EPRes_BufferSize, unit->StreamAlt ? unit->StreamPacketSize * unit->IsoPacketCount : unit->PayloadSize,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		TAG_END );

	if ( ! unit->Stream )
	{
		return ( FALSE );
	}

	unit->FrameDiscard = TRUE;

	return ( TRUE );
}

// --

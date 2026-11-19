
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
Video_Stream_Start( struct VideoUnit *unit )
{
	struct USB2_IORequest *request;

	U32 bufferSize;

	if ( unit->Stream )
	{
		return ( TRUE );
	}

	if ( ( ! unit->StreamConfigured ) || ( ( unit->StreamAltActive ) && ( ! Video_Stream_Close( unit ) ) ) )
	{
		return ( FALSE );
	}

	// Commit once at capture start, while the streaming interface is still idle.
	if ( ! Video_Control( unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
		0x01, 0x0200, unit->Commit, unit->ProbeLength ) )
	{
		request = unit->Control->IORequests[0];

		DebugPrintF( "USB Video: %04lx:%04lx unit %lu could not commit video mode, USB error %ld\n",
			unit->Format.VendorID, unit->Format.DeviceID, unit->UnitNr, (S32) request->io_Error );

		if ( request->io_Error == USB2Err_Host_Stall )
		{
			USB2_EPRes_Destall( unit->Control );
		}

		return ( FALSE );
	}

	bufferSize = unit->PayloadSize;

	if ( unit->StreamAlt )
	{
		if ( ! Video_Stream_SetAlt( unit, unit->StreamAlt ) )
		{
			return ( FALSE );
		}

		unit->StreamAltActive = TRUE;
		bufferSize = unit->StreamPacketSize * unit->IsoPacketCount;
	}

	unit->Stream = USB2_EPRes_ObtainTags( unit->Register,
		USB2Tag_EPRes_MsgPort, unit->StreamPort,
		USB2Tag_EPRes_NrOfIORequest, VIDEO_USB_READS,
		USB2Tag_EPRes_InterfaceNr, unit->InterfaceNr,
		USB2Tag_EPRes_EndPointNr, unit->EndPoint,
		USB2Tag_EPRes_BufferSize, bufferSize,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		TAG_END );

	if ( ! unit->Stream )
	{
		Video_Stream_Close( unit );

		return ( FALSE );
	}

	unit->FrameDiscard = TRUE;

	return ( TRUE );
}

// --

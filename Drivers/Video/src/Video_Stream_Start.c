
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE S8
Video_Stream_Start( struct VideoUnit *unit )
{
	static const struct VideoCaptureStats emptyStats;

	U32 bufferSize;
	U32 speed;
	S8 error;

	if ( unit->USBDetached )
	{
		return ( USB2Err_Device_Detached );
	}

	if ( unit->Stream )
	{
		return ( 0 );
	}

	if ( ! unit->StreamConfigured )
	{
		return ( IOERR_OPENFAIL );
	}

	if ( ( unit->StreamAltActive ) || ( unit->OV534StopPending ) )
	{
		error = Video_Stream_Close( unit );

		if ( error )
		{
			return ( error );
		}
	}

	// Commit once at capture start, while the streaming interface is still idle.
	if ( ( ! unit->OV534 ) &&
		( ! Video_Control( unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
			0x01, 0x0200, unit->Commit, unit->ProbeLength ) ) )
	{
		error = unit->Control->IORequests[0]->io_Error;

		DebugPrintF( "USB Video: %04lx:%04lx unit %lu could not commit video mode, USB error %ld\n",
			unit->Format.VendorID, unit->Format.DeviceID, unit->UnitNr, (S32) error );

		if ( error == USB2Err_Host_Stall )
		{
			USB2_EPRes_Destall( unit->Control );
		}

		return ( error == USB2Err_Device_Detached ? error : IOERR_OPENFAIL );
	}

	bufferSize = unit->PayloadSize;
	unit->ReadCount = VIDEO_USB_READS;

	if ( unit->StreamAlt )
	{
		if ( ! Video_Stream_SetAlt( unit, unit->StreamAlt ) )
		{
			return ( IOERR_OPENFAIL );
		}

		unit->StreamAltActive = TRUE;
		bufferSize = unit->IsoBuffers.Size ? 0 : unit->StreamPacketSize * unit->IsoPacketCount;

		if ( ( unit->IsoBuffers.Size ) && ( ! unit->IsoBuffers.Buffer[2] ) )
		{
			unit->ReadCount = 2;
		}
	}

	unit->Stream = USB2_EPRes_ObtainTags( unit->Register,
		USB2Tag_EPRes_MsgPort, unit->StreamPort,
		USB2Tag_EPRes_NrOfIORequest, unit->ReadCount,
		USB2Tag_EPRes_InterfaceNr, unit->InterfaceNr,
		USB2Tag_EPRes_EndPointNr, unit->EndPoint,
		USB2Tag_EPRes_BufferSize, bufferSize,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		TAG_END );

	if ( ! unit->Stream )
	{
		Video_Stream_Close( unit );

		return ( IOERR_OPENFAIL );
	}

	if ( unit->OV534 )
	{
		error = Video_OV534_Start( unit );

		if ( error )
		{
			DebugPrintF( "USB Video: PlayStation Eye stream start failed, USB error %ld\n", (S32) error );

			Video_Stream_Close( unit );

			return ( unit->USBDetached ? USB2Err_Device_Detached : IOERR_OPENFAIL );
		}
	}

	unit->FrameDiscard = TRUE;

	CopyMem( & emptyStats, & unit->CaptureStats, sizeof( emptyStats ) );

	unit->CaptureStats.FrameInterval = unit->Format.FrameInterval;
	unit->CaptureStats.LinkSpeed = ~0U;
	unit->CaptureStats.IsoBatchPackets = unit->IsoPacketCount;

	if ( USB2_Attr_Get( unit->StartMsg->Function->FKT_ID, USB2Tag_Attr_Speed, & speed, sizeof( speed ) ) == sizeof( speed ) )
	{
		unit->CaptureStats.LinkSpeed = speed;

		if ( ( unit->StreamAlt ) && ( speed >= USBSPEED_Full ) && ( speed <= USBSPEED_Super ) )
		{
			unit->CaptureStats.IsoServiceUS = ( ( speed == USBSPEED_Full ) ? 1000U : 125U ) <<
				( unit->StreamInterval - 1 );
		}
	}

	return ( 0 );
}

// --

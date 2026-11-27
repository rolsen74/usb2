
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
Video_Set_Mode( struct VideoUnit *unit, U32 number )
{
	struct VideoMode requested;
	struct VideoMode previous;

	S8 error;

	if ( unit->USBDetached )
	{
		return ( USB2Err_Device_Detached );
	}

	if ( unit->ReadActive )
	{
		return ( IOERR_UNITBUSY );
	}

	if ( ! number )
	{
		requested = unit->DefaultMode;
	}
	else if ( ! Video_Parse( unit, number, & requested ) )
	{
		return ( IOERR_BADADDRESS );
	}

	if ( ( unit->StreamConfigured ) &&
		( ( ( ! unit->StreamAltActive ) && ( ! unit->OV534StopPending ) ) || ( unit->Stream ) ) &&
		( unit->FormatIndex == requested.FormatIndex ) &&
		( unit->FrameIndex == requested.FrameIndex ) &&
		( unit->Format.FrameInterval == requested.Format.FrameInterval ) )
	{
		return ( 0 );
	}

	previous.Format = unit->Format;
	previous.FormatIndex = unit->FormatIndex;
	previous.FrameIndex = unit->FrameIndex;

	error = Video_Stream_Close( unit );

	unit->IsoBuffers.Size = 0;

	if ( error )
	{
		return ( error );
	}

	if ( Video_Stream_Open( unit, & requested ) )
	{
		if ( unit->Format.FrameInterval != requested.Format.FrameInterval )
		{
			DebugPrintF( "USB Video: %04lx:%04lx unit %lu requested interval %lu, camera set %lu\n",
				unit->Format.VendorID, unit->Format.DeviceID, unit->UnitNr,
				requested.Format.FrameInterval, unit->Format.FrameInterval );
		}

		return ( 0 );
	}

	if ( unit->USBDetached )
	{
		return ( USB2Err_Device_Detached );
	}

	// Restore the previous stream if the camera rejects the new mode.
	if ( ! Video_Stream_Open( unit, & previous ) )
	{
		unit->Format = previous.Format;
		unit->FormatIndex = previous.FormatIndex;
		unit->FrameIndex = previous.FrameIndex;
	}

	return ( unit->USBDetached ? USB2Err_Device_Detached : IOERR_OPENFAIL );
}

// --

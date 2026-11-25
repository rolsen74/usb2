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
Video_Control_Index( struct VideoUnit *unit, U8 type, U8 code, U16 value, U16 index,
	PTR data, U16 length, U32 timeout )
{
struct USB2_IORequest *ioreq;
struct USB2_SetupData *setup;
struct TimeVal time;

U64 now;

	ioreq = unit->Control->IORequests[0];

	if ( unit->USBDetached )
	{
		ioreq->io_Error = USB2Err_Device_Detached;
		ioreq->io_Actual = 0;

		return ( FALSE );
	}

	setup = unit->Control->SetupData;
	setup->RequestType = type;
	setup->RequestCode = code;
	setup->Value = LE_SWAP16( value );
	setup->Index = LE_SWAP16( index );
	setup->Length = LE_SWAP16( length );

	ioreq->io_Command = ( type & REQTYPE_Read ) ? CMD_READ : CMD_WRITE;
	ioreq->io_Data = data;
	ioreq->io_Length = length;
	ioreq->io_SetupData = setup;
	ioreq->io_SetupLength = sizeof( struct USB2_SetupData );
	ioreq->io_TimeOut = timeout;

	if ( unit->ProbeClock )
	{
		unit->ProbeClock->GetUpTime( & time );

		now = (U64) time.Seconds * 1000000 + time.Microseconds;

		if ( now >= unit->ProbeDeadline )
		{
			ioreq->io_Error = USB2Err_Host_Timeout;

			return ( FALSE );
		}

		if ( ( ! timeout ) || ( unit->ProbeDeadline - now < timeout ) )
		{
			ioreq->io_TimeOut = unit->ProbeDeadline - now;
		}
	}

	DoIO( (PTR) ioreq );

	if ( ioreq->io_Error == USB2Err_Device_Detached )
	{
		unit->USBDetached = TRUE;
	}

	return ( ( ioreq->io_Error == USB2Err_NoError ) && ( ioreq->io_Actual == length ) );
}

// --

SEC_CODE S32
Video_Control( struct VideoUnit *unit, U8 type, U8 code, U16 value, PTR data, U16 length )
{
	return ( Video_Control_Index( unit, type, code, value, unit->InterfaceNr, data, length, 0 ) );
}

// --

SEC_CODE S32
Video_Stream_SetAlt( struct VideoUnit *unit, U32 alt )
{
	if ( USB2_Ifc_SetAlt( unit->Register, unit->StartMsg->Interface, alt ) )
	{
		return ( TRUE );
	}

	DebugPrintF( "USB Video: %04lx:%04lx unit %lu could not select alternate %lu\n",
		unit->Format.VendorID, unit->Format.DeviceID, unit->UnitNr, alt );

	return ( FALSE );
}

// --

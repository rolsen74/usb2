/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE S32 Video_Control( struct VideoUnit *unit, U8 type, U8 code, U16 value, PTR data, U16 length )
{
struct USB2_IORequest *ioreq;
struct USB2_SetupData *setup;
struct TimeVal time;

U64 now;

	ioreq = unit->Control->IORequests[0];
	setup = unit->Control->SetupData;
	setup->RequestType = type;
	setup->RequestCode = code;
	setup->Value = LE_SWAP16( value );
	setup->Index = LE_SWAP16( unit->InterfaceNr );
	setup->Length = LE_SWAP16( length );

	ioreq->io_Command = ( type & REQTYPE_Read ) ? CMD_READ : CMD_WRITE;
	ioreq->io_Data = data;
	ioreq->io_Length = length;
	ioreq->io_SetupData = setup;
	ioreq->io_SetupLength = sizeof( struct USB2_SetupData );

	if ( unit->ProbeClock )
	{
		unit->ProbeClock->GetUpTime( & time );

		now = (U64) time.Seconds * 1000000 + time.Microseconds;

		if ( now >= unit->ProbeDeadline )
		{
			ioreq->io_Error = USB2Err_Host_Timeout;

			return ( FALSE );
		}

		ioreq->io_TimeOut = unit->ProbeDeadline - now;
	}

	DoIO( (PTR) ioreq );

	return ( ( ioreq->io_Error == USB2Err_NoError ) && ( ioreq->io_Actual == length ) );
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

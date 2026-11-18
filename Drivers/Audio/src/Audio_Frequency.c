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
Audio_Frequency( struct AudioUnit *unit, U32 sample_rate )
{
struct USB2_IORequest *ioreq;
struct USB2_SetupData *setup;

U8 rate[3];
U32 actual;

	if ( ! unit->FrequencyControl )
	{
		return ( TRUE );
	}

	ioreq = unit->Register->Res_Control->IORequests[0];
	setup = unit->Register->Res_Control->SetupData;
	rate[0] = sample_rate;
	rate[1] = sample_rate >> 8;
	rate[2] = sample_rate >> 16;
	setup->RequestType = REQTYPE_Write | REQTYPE_Class | REQTYPE_EndPoint;
	setup->RequestCode = 0x01;
	setup->Value = LE_SWAP16( 0x0100 );
	setup->Index = LE_SWAP16( unit->EndPoint );
	setup->Length = LE_SWAP16( 3 );
	ioreq->io_Command = CMD_WRITE;
	ioreq->io_Data = rate;
	ioreq->io_Length = 3;
	ioreq->io_SetupData = setup;
	ioreq->io_SetupLength = sizeof( struct USB2_SetupData );

	DoIO( (PTR) ioreq );

	if ( ( ioreq->io_Error != USB2Err_NoError ) || ( ioreq->io_Actual != sizeof( rate ) ) )
	{
		return ( FALSE );
	}

	setup->RequestType = REQTYPE_Read | REQTYPE_Class | REQTYPE_EndPoint;
	setup->RequestCode = 0x81;
	ioreq->io_Command = CMD_READ;

	DoIO( (PTR) ioreq );

	if ( ioreq->io_Error == USB2Err_Host_Stall )
	{
		return ( USB2_EPRes_Destall( unit->Register->Res_Control ) );
	}

	actual = (U32) rate[0] | ( (U32) rate[1] << 8 ) | ( (U32) rate[2] << 16 );

	return ( ( ioreq->io_Error == USB2Err_NoError ) && ( ioreq->io_Actual == sizeof( rate ) ) &&
		( actual == sample_rate ) );
}

// --

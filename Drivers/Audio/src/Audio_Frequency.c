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
S32 fixed;

	if ( ! unit->FrequencyControl )
	{
		return ( TRUE );
	}

	fixed = ( unit->EndPoint & EPADR_Dir_Mask ) && ( unit->RateDescriptor[7] == 1 );

	if ( ( fixed ) && ( ! Audio_Rate_Valid( unit, sample_rate ) ) )
	{
		return ( FALSE );
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
		AUDIO_TRACE( "endpoint $%02lx SET_CUR %lu Hz failed, USB error %ld, bytes %lu\n",
			(U32) unit->EndPoint, sample_rate, (S32) ioreq->io_Error, ioreq->io_Actual );

		return ( FALSE );
	}

	// A fixed capture rate needs no readback once the endpoint accepts SET_CUR.
	if ( fixed )
	{
		AUDIO_TRACE( "endpoint $%02lx accepted fixed capture rate %lu Hz\n",
			(U32) unit->EndPoint, sample_rate );

		return ( TRUE );
	}

	setup->RequestType = REQTYPE_Read | REQTYPE_Class | REQTYPE_EndPoint;
	setup->RequestCode = 0x81;
	ioreq->io_Command = CMD_READ;

	DoIO( (PTR) ioreq );

	if ( ioreq->io_Error == USB2Err_Host_Stall )
	{
		AUDIO_TRACE( "endpoint $%02lx GET_CUR stalled after SET_CUR\n", (U32) unit->EndPoint );

		return ( USB2_EPRes_Destall( unit->Register->Res_Control ) );
	}

	actual = (U32) rate[0] | ( (U32) rate[1] << 8 ) | ( (U32) rate[2] << 16 );

	AUDIO_TRACE( "endpoint $%02lx GET_CUR %lu Hz, USB error %ld, bytes %lu\n",
		(U32) unit->EndPoint, actual, (S32) ioreq->io_Error, ioreq->io_Actual );

	return ( ( ioreq->io_Error == USB2Err_NoError ) && ( ioreq->io_Actual == sizeof( rate ) ) &&
		( actual == sample_rate ) );
}

// --

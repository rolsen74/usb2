
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static S32
GainControl( struct AudioUnit *unit, U8 code, S32 *value )
{
	struct USB2_EPResource *control;
	struct USB2_IORequest *request;
	struct USB2_SetupData *setup;

	U8 data[2];
	U32 word;
	S32 error;

	control = unit->Register->Res_Control;
	request = control->IORequests[0];
	setup = control->SetupData;
	word = (U16) *value;
	data[0] = word;
	data[1] = word >> 8;
	setup->RequestType = ( code & REQTYPE_Read ) | REQTYPE_Class | REQTYPE_Interface;
	setup->RequestCode = code;
	setup->Value = LE_SWAP16( 0x0200 | unit->GainChannel );
	setup->Index = LE_SWAP16( ( (U16) unit->GainEntity << 8 ) | unit->GainInterface );
	setup->Length = LE_SWAP16( sizeof( data ) );
	request->io_Command = ( code & REQTYPE_Read ) ? CMD_READ : CMD_WRITE;
	request->io_Data = data;
	request->io_Length = sizeof( data );
	request->io_SetupData = setup;
	request->io_SetupLength = sizeof( *setup );

	DoIO( (PTR) request );

	error = request->io_Error;
	request->io_Data = NULL;

	if ( error == USB2Err_Host_Stall )
	{
		USB2_EPRes_Destall( control );
	}

	if ( ( error ) || ( request->io_Actual != sizeof( data ) ) )
	{
		return ( FALSE );
	}

	word = data[0] | ( (U32) data[1] << 8 );
	*value = ( word & 0x8000 ) ? (S32) word - 65536 : (S32) word;

	return ( TRUE );
}

// --

SEC_CODE void
Audio_GainCommand( struct AudioUnit *unit, struct IOStdReq *ioreq )
{
	struct AudioGain *gain;

	S32 value;
	U32 size;

	if ( ! unit->GainEntity )
	{
		ioreq->io_Error = IOERR_NOCMD;

		return;
	}

	if ( unit->ReadActive )
	{
		ioreq->io_Error = IOERR_UNITBUSY;

		return;
	}

	size = ( ioreq->io_Command == AUDIO_CMD_GET_GAIN ) ? sizeof( *gain ) : sizeof( value );

	if ( ( ! ioreq->io_Data ) || ( ioreq->io_Length < size ) )
	{
		ioreq->io_Error = IOERR_BADLENGTH;

		return;
	}

	gain = & unit->Gain;

	if ( ! unit->GainValid )
	{
		if ( ( ! GainControl( unit, 0x82, & gain->Minimum ) ) ||
			( ! GainControl( unit, 0x83, & gain->Maximum ) ) ||
			( ! GainControl( unit, 0x84, & gain->Step ) ) ||
			( gain->Minimum >= gain->Maximum ) || ( gain->Step <= 0 ) )
		{
			ioreq->io_Error = IOERR_NOCMD;

			return;
		}

		unit->GainValid = TRUE;
	}

	if ( ioreq->io_Command == AUDIO_CMD_GET_GAIN )
	{
		if ( ! GainControl( unit, 0x81, & gain->Current ) )
		{
			ioreq->io_Error = IOERR_NOCMD;

			return;
		}

		CopyMem( gain, ioreq->io_Data, size );
	}
	else
	{
		CopyMem( ioreq->io_Data, & value, size );

		if ( ( value < gain->Minimum ) || ( value > gain->Maximum ) ||
			( ( value - gain->Minimum ) % gain->Step ) )
		{
			ioreq->io_Error = IOERR_BADADDRESS;

			return;
		}

		if ( ! GainControl( unit, 0x01, & value ) )
		{
			ioreq->io_Error = IOERR_UNITBUSY;

			return;
		}
	}

	ioreq->io_Actual = size;
}

// --

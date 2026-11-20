
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <proto/exec.h>
#include "SDRTest.h"

// --

#define SDR_BULK_MARGIN_US	4000000U

// --

static S32
ReadIQ( struct USB2_EPResource *control, struct USB2_EPResource *bulk, U8 *samples, U32 bytes )
{
	struct USB2_IORequest *request;

	U32 signals;
	S32 started;
	S32 interrupted;
	S32 error;

	request = bulk->IORequests[0];
	interrupted = FALSE;
	request->io_Command = CMD_READ;
	request->io_Data = samples;
	request->io_Length = bytes;
	request->io_TimeOut = ( bytes / SDR_IQ_BYTES_PER_SECOND ) * 1000000U + SDR_BULK_MARGIN_US;
	request->io_AllowShortPackets = FALSE;

	// Queue the full capture before releasing the FIFO to avoid gaps at startup.
	SendIO( (PTR) request );

	started = ! SDR_HoldFIFO( control, FALSE );

	if ( ( ! started ) && ( ! CheckIO( (PTR) request ) ) )
	{
		AbortIO( (PTR) request );
	}

	while ( ( started ) && ( ! CheckIO( (PTR) request ) ) )
	{
		signals = Wait( bulk->MsgPortBit | SIGBREAKF_CTRL_C );

		if ( signals & SIGBREAKF_CTRL_C )
		{
			interrupted = TRUE;

			AbortIO( (PTR) request );

			break;
		}
	}

	error = WaitIO( (PTR) request );

	request->io_Data = NULL;

	if ( ! started )
	{
		return ( FALSE );
	}

	if ( interrupted )
	{
		Printf( "SDR capture interrupted.\n" );

		return ( FALSE );
	}

	if ( ( error ) || ( request->io_Actual != bytes ) )
	{
		Printf( "SDR bulk read failed (USB error %ld, %lu of %lu bytes).\n",
			error, request->io_Actual, bytes );

		return ( FALSE );
	}

	return ( TRUE );
}

// --

S32
SDR_CaptureIQ( struct USB2_Register *reg, U8 endpoint, U32 frequency, U8 *samples, U32 bytes )
{
	struct USB2_EPResource *control;
	struct USB2_EPResource *bulk;

	U32 index;
	U8 low;
	U8 high;
	S32 captured;
	S32 stopped;

	bulk = SDR_OpenIQ( reg, endpoint, frequency, 1 );

	if ( ! bulk )
	{
		return ( RETURN_FAIL );
	}

	control = reg->Res_Control;

	Printf( "Capturing %lu s at %lu Hz, %lu samples/s...\n",
		(U32) ( bytes / SDR_IQ_BYTES_PER_SECOND ), frequency, (U32) SDR_SAMPLE_RATE );

	captured = ReadIQ( control, bulk, samples, bytes );

	stopped = SDR_CloseIQ( control, bulk );

	if ( ! captured )
	{
		return ( RETURN_FAIL );
	}

	low = 255;
	high = 0;

	for ( index = 0; index < bytes; index++ )
	{
		low = MIN( low, samples[index] );
		high = MAX( high, samples[index] );
	}

	Printf( "Captured %lu bytes of unsigned 8-bit I/Q (sample range %lu-%lu).\n",
		bytes, (U32) low, (U32) high );

	return ( ( ( stopped ) && ( low != high ) ) ? RETURN_OK : RETURN_WARN );
}

// --

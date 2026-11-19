
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static void
Cancel_IO( struct IORequest *io )
{
	if ( ! CheckIO( io ) )
	{
		AbortIO( io );
	}

	WaitIO( io );
}

// --

enum WirelessResult
Wireless_Wait_Frame( struct Wireless *wireless, U32 micros,
	const U8 *transmit, U32 length, FrameMatch match, S32 eapol )
{
	struct IOSana2Req *rx;
	struct TimeRequest *timer;
	enum WirelessResult result;

	U32 mask;
	U32 signals;
	U8 frame[WIRELESS_FRAME_SIZE];
	S32 pending;

	rx = wireless->Receive;
	timer = wireless->Timer;
	mask = ( 1UL << wireless->ReceivePort->mp_SigBit ) | SIGBREAKF_CTRL_C;
	pending = FALSE;
	result = WIRELESS_TIMEOUT;

	if ( micros )
	{
		mask |= 1UL << wireless->TimerPort->mp_SigBit;
		timer->Request.io_Command = TR_ADDREQUEST;
		timer->Request.io_Flags = 0;
		timer->Time.Seconds = micros / 1000000;
		timer->Time.Microseconds = micros % 1000000;

		SendIO( & timer->Request );
	}

	for( ;; )
	{
		rx->ios2_Req.io_Command = eapol ? CMD_READ : S2_READMGMT;
		rx->ios2_Req.io_Flags = 0;
		rx->ios2_PacketType = WIRELESS_EAPOL;
		rx->ios2_Data = frame;
		rx->ios2_DataLength = sizeof( frame );

		SendIO( & rx->ios2_Req );

		pending = TRUE;

		if ( transmit )
		{
			wireless->Command->ios2_Data = (void *) transmit;
			wireless->Command->ios2_DataLength = length;

			if ( ! Wireless_Command( wireless, S2_WRITEMGMT ) )
			{
				result = WIRELESS_IO_ERROR;
				break;
			}

			transmit = NULL;
		}

		while( ( ! CheckIO( & rx->ios2_Req ) ) && ( ( ! micros ) || ( ! CheckIO( & timer->Request ) ) ) )
		{
			signals = Wait( mask );

			if ( signals & SIGBREAKF_CTRL_C )
			{
				result = WIRELESS_STOPPED;
				goto done;
			}
		}

		if ( ( micros ) && ( CheckIO( & timer->Request ) ) )
		{
			break;
		}

		WaitIO( & rx->ios2_Req );

		pending = FALSE;

		if ( rx->ios2_Req.io_Error )
		{
			fprintf( stderr, "%s receive failed (%d, wire error %lu).\n", eapol ? "EAPOL" : "Management",
				(int) rx->ios2_Req.io_Error, (unsigned long) rx->ios2_WireError );

			result = WIRELESS_IO_ERROR;
			break;
		}

		if ( ( eapol ) &&
			( ( memcmp( rx->ios2_SrcAddr, wireless->AP.BSSID, WIRELESS_ADDR_SIZE ) ) ||
			( memcmp( rx->ios2_DstAddr, wireless->Station, WIRELESS_ADDR_SIZE ) ) ) )
		{
			continue;
		}

		if ( ( rx->ios2_DataLength <= sizeof( frame ) ) && ( match( wireless, frame, rx->ios2_DataLength ) ) )
		{
			result = WIRELESS_MATCH;
			break;
		}
	}

done:

	if ( pending )
	{
		Cancel_IO( & rx->ios2_Req );
	}

	if ( micros )
	{
		Cancel_IO( & timer->Request );
	}

	rx->ios2_Data = NULL;
	wireless->Command->ios2_Data = NULL;

	return ( result );
}

// --

S32
Wireless_Wait_Link( struct Wireless *wireless )
{
	struct IOSana2Req *io;

	U32 mask;
	U32 signals;

	io = wireless->Receive;
	mask = ( 1UL << wireless->ReceivePort->mp_SigBit ) | SIGBREAKF_CTRL_C;
	io->ios2_Req.io_Command = S2_ONEVENT;
	io->ios2_Req.io_Flags = 0;
	io->ios2_WireError = S2EVENT_OFFLINE | S2EVENT_HARDWARE;

	SendIO( & io->ios2_Req );

	while( ! CheckIO( & io->ios2_Req ) )
	{
		signals = Wait( mask );

		if ( signals & SIGBREAKF_CTRL_C )
		{
			Cancel_IO( & io->ios2_Req );

			return ( TRUE );
		}
	}

	WaitIO( & io->ios2_Req );

	fprintf( stderr, "Wireless link ended (error %d, event 0x%lx).\n",
		(int) io->ios2_Req.io_Error, (unsigned long) io->ios2_WireError );

	return ( FALSE );
}


/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
BTUSB_StopError( struct BTUSB *usb )
{
	struct USB2_NotifyMessage *message;

	while(( message = (PTR) GetMsg( usb->Registration->Stack_MsgPort ) ))
	{
		if ( message->nm_Command == USBNC_Detached )
		{
			usb->Detached = TRUE;
		}

		ReplyMsg( & message->nm_Message );
	}

	if ( SetSignal( 0, SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C )
	{
		usb->Stopping = TRUE;
	}

	if ( usb->Detached )
	{
		return ( USB2Err_Device_Detached );
	}

	if ( ( usb->Stopping ) || ( ( usb->Cancel ) && ( __atomic_load_n( usb->Cancel, __ATOMIC_RELAXED ) ) ) )
	{
		return ( IOERR_ABORTED );
	}

	return ( 0 );
}

// --

S32
BTUSB_Wait( struct BTUSB *usb, struct USB2_IORequest *request )
{
	U32 mask;
	U32 signals;
	S32 stop;
	S32 error;

	mask = ( 1UL << request->io_Message.mn_ReplyPort->mp_SigBit ) |
		usb->Registration->Stack_MsgPortBit | usb->RequestSignal | SIGBREAKF_CTRL_C;

	for( ;; )
	{
		stop = BTUSB_StopError( usb );

		if ( ( stop ) || ( CheckIO( (PTR) request ) ) )
		{
			break;
		}

		signals = Wait( mask );

		if ( signals & SIGBREAKF_CTRL_C )
		{
			usb->Stopping = TRUE;
		}
	}

	if ( ( stop ) && ( ! CheckIO( (PTR) request ) ) )
	{
		AbortIO( (PTR) request );
	}

	error = WaitIO( (PTR) request );

	return ( stop ? stop : error );
}

// --

S32
BTUSB_Do( struct BTUSB *usb, struct USB2_IORequest *request )
{
	S32 error;

	error = BTUSB_StopError( usb );

	if ( error )
	{
		return ( error );
	}

	SendIO( (PTR) request );

	return ( BTUSB_Wait( usb, request ) );
}

// --


/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/SmartCard.h"
#include <dos/dos.h>

// --

void
SmartCard_Notice( struct SmartCardReader *reader, U32 signals )
{
	struct USB2_NotifyMessage *notice;

	if ( signals & SIGBREAKF_CTRL_C )
	{
		reader->Stopped = TRUE;
	}

	while(( notice = (PTR) IExec->GetMsg( reader->Registration->Stack_MsgPort ) ))
	{
		if ( notice->nm_Command == USBNC_Detached )
		{
			reader->Stopped = TRUE;
		}

		IExec->ReplyMsg( (PTR) notice );
	}
}

// --

U32
SmartCard_Now( struct SmartCardReader *reader )
{
	struct TimeVal now;

	reader->Clock->GetUpTime( &now );

	return ( now.Seconds * 1000000U + now.Microseconds );
}

// --

S32
SmartCard_Transfer( struct SmartCardReader *reader, struct USB2_EPResource *endpoint,
	U16 operation, U8 *buffer, U32 length, U32 start, U32 budget, U32 timeout, U32 *actual )
{
	struct USB2_IORequest *request;

	U32 elapsed;
	U32 signals;

	*actual = 0;
	signals = IExec->SetSignal( 0, SIGBREAKF_CTRL_C );

	SmartCard_Notice( reader, signals );

	if ( reader->Stopped )
	{
		return ( USB2Err_Device_Detached );
	}

	elapsed = SmartCard_Now( reader ) - start;

	if ( elapsed >= budget )
	{
		return ( USB2Err_Host_Timeout );
	}

	request = endpoint->IORequests[0];
	request->io_Command = operation;
	request->io_Data = buffer;
	request->io_Length = length;
	request->io_Actual = 0;
	request->io_TimeOut = ( timeout < budget - elapsed ) ? timeout : budget - elapsed;

	IExec->SendIO( (PTR) request );

	while( ! IExec->CheckIO( (PTR) request ) )
	{
		signals = IExec->Wait( endpoint->MsgPortBit | reader->Registration->Stack_MsgPortBit | SIGBREAKF_CTRL_C );

		SmartCard_Notice( reader, signals );

		if ( reader->Stopped )
		{
			if ( ! IExec->CheckIO( (PTR) request ) )
			{
				IExec->AbortIO( (PTR) request );
			}

			break;
		}
	}

	IExec->WaitIO( (PTR) request );

	*actual = request->io_Actual;

	if ( ( reader->Stopped ) || ( request->io_Error == USB2Err_Device_Detached ) )
	{
		reader->Stopped = TRUE;

		return ( USB2Err_Device_Detached );
	}

	return ( request->io_Error );
}

// --

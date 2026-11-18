
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "Xbox360.h"

// --

static void
LogState( const struct Xbox360State *state, struct Xbox360State *previous, S32 *have_state )
{
	if ( ( *have_state ) && ( state->Buttons == previous->Buttons ) &&
		( state->HatX == previous->HatX ) && ( state->HatY == previous->HatY ) &&
		( state->TriggerLeft == previous->TriggerLeft ) && ( state->TriggerRight == previous->TriggerRight ) &&
		( state->Axis[0] == previous->Axis[0] ) && ( state->Axis[1] == previous->Axis[1] ) &&
		( state->Axis[2] == previous->Axis[2] ) && ( state->Axis[3] == previous->Axis[3] ) )
	{
		return;
	}

	*previous = *state;
	*have_state = TRUE;

	DebugPrintF( "Xbox360: buttons=%04lx dpad=%ld,%ld triggers=%lu,%lu sticks=%ld,%ld %ld,%ld\n",
		(U32) state->Buttons, (S32) state->HatX, (S32) state->HatY,
		(U32) state->TriggerLeft, (U32) state->TriggerRight,
		(S32) state->Axis[0], (S32) state->Axis[1], (S32) state->Axis[2], (S32) state->Axis[3] );
}

// --

void
Xbox360_Poll( struct USB2IFace *usb, struct USB2_Register *reg, struct USB2_EPResource *input, U8 interface )
{
	struct USB2_NotifyMessage *msg;
	struct USB2_IORequest *ioreq;
	struct Xbox360State state;
	struct Xbox360State previous;

	U32 signals;
	U32 errors;
	U32 waitmask;
	S32 stopped;
	S32 pending;
	S32 have_state;

	ioreq = input->IORequests[0];
	waitmask = reg->Stack_MsgPortBit | input->MsgPortBit | SIGBREAKF_CTRL_C;
	errors = 0;
	stopped = FALSE;
	pending = FALSE;
	have_state = FALSE;

	while( ! stopped )
	{
		if ( ! pending )
		{
			ioreq->io_Command = CMD_READ;
			ioreq->io_Data = input->Buffers[0];
			ioreq->io_Length = input->BufferSize;

			SendIO( (PTR) ioreq );

			pending = TRUE;
		}

		signals = Wait( waitmask );

		if ( signals & reg->Stack_MsgPortBit )
		{
			while(( msg = (PTR) GetMsg( reg->Stack_MsgPort ) ))
			{
				if ( msg->nm_Command == USBNC_Detached )
				{
					stopped = TRUE;
				}

				ReplyMsg( (PTR) msg );
			}
		}

		if ( ( stopped ) || ( signals & SIGBREAKF_CTRL_C ) )
		{
			break;
		}

		if ( ( ! ( signals & input->MsgPortBit ) ) || ( ! GetMsg( input->MsgPort ) ) )
		{
			continue;
		}

		pending = FALSE;

		if ( ioreq->io_Error == USB2Err_NoError )
		{
			errors = 0;

			if ( ( ioreq->io_Actual <= input->BufferSize ) &&
				( Xbox360_DecodeReport( ioreq->io_Data, ioreq->io_Actual, & state ) ) )
			{
				if ( ! have_state )
				{
					Xbox360_SetPlayerLED( usb, reg, interface );
				}

				LogState( & state, & previous, & have_state );
			}
		}
		else
		{
			errors++;

			if ( ioreq->io_Error == USB2Err_Device_Detached )
			{
				break;
			}

			if ( errors >= XBOX360_READ_ERRORS_MAX )
			{
				DebugPrintF( "Xbox360: stopped polling after repeated USB errors (%ld)\n", (S32) ioreq->io_Error );

				break;
			}

			if ( ( ioreq->io_Error == USB2Err_Host_Stall ) && ( ! usb->USB2_EPRes_Destall( input ) ) )
			{
				DebugPrintF( "Xbox360: could not clear interrupt IN stall\n" );

				break;
			}
		}
	}

	if ( pending )
	{
		if ( ! CheckIO( (PTR) ioreq ) )
		{
			AbortIO( (PTR) ioreq );
		}

		WaitIO( (PTR) ioreq );
	}
}

// --

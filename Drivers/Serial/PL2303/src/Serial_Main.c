/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Main( struct SerialContext *ctx, struct SerialUnit *unit )
{
struct USB2_NotifyMessage *msg;

U32 waitmask;
U32 mask;

	Serial_Read_Start( ctx, unit );
	Serial_Interrupt_Start( ctx, unit );

	waitmask = unit->Register->Stack_MsgPortBit;
	waitmask |= ( 1UL << unit->BeginPort->mp_SigBit );
	waitmask |= ( 1UL << unit->AbortPort->mp_SigBit );
	waitmask |= unit->BulkIn->MsgPortBit;
	waitmask |= unit->BulkOut->MsgPortBit;
	waitmask |= SIGBREAKF_CTRL_C;

	if ( unit->Interrupt )
	{
		waitmask |= unit->Interrupt->MsgPortBit;
	}

	while( ! unit->Detached )
	{
		mask = Wait( waitmask );

		if ( mask & unit->Register->Stack_MsgPortBit )
		{
			while(( msg = (PTR) GetMsg( unit->Register->Stack_MsgPort ) ))
			{
				if ( msg->nm_Command == USBNC_Detached )
				{
					unit->Detached = TRUE;
				}

				ReplyMsg( (PTR) msg );
			}
		}

		if ( unit->Detached )
		{
			break;
		}

		if ( mask & ( 1UL << unit->AbortPort->mp_SigBit ) )
		{
			Serial_Abort( ctx, unit );
		}

		if ( mask & ( 1UL << unit->BeginPort->mp_SigBit ) )
		{
			Serial_Begin( ctx, unit );
		}

		if ( mask & unit->BulkIn->MsgPortBit )
		{
			Serial_Read_Done( ctx, unit );
		}

		if ( mask & unit->BulkOut->MsgPortBit )
		{
			Serial_Write_Done( ctx, unit );
		}

		if ( ( unit->Interrupt ) && ( mask & unit->Interrupt->MsgPortBit ) )
		{
			Serial_Interrupt_Done( ctx, unit );
		}

		if ( mask & SIGBREAKF_CTRL_C )
		{
			unit->Detached = TRUE;
		}
	}
}

// --

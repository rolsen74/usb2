/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Video_Main( struct VideoUnit *unit )
{
struct USB2_NotifyMessage *msg;

U32 waitmask;
U32 mask;

	waitmask = unit->Register->Stack_MsgPortBit;
	waitmask |= 1UL << unit->BeginPort->mp_SigBit;
	waitmask |= 1UL << unit->StreamPort->mp_SigBit;
	waitmask |= SIGBREAKF_CTRL_C;

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

		if ( mask & ( 1UL << unit->BeginPort->mp_SigBit ) )
		{
			Video_Abort( unit );
			Video_Begin( unit );
		}

		if ( ( unit->Stream ) && ( mask & ( 1UL << unit->StreamPort->mp_SigBit ) ) )
		{
			Video_Read_Done( unit );
		}

		if ( mask & SIGBREAKF_CTRL_C )
		{
			unit->Detached = TRUE;
		}
	}
}

// --

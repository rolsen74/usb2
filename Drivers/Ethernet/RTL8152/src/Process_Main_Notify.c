
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
Handle_USBFunction( struct devUnit *unit )
{
	struct USB2_NotifyMessage *msg;

	while(( msg = (PTR) IExec->GetMsg( unit->unit_USBRegister->Stack_MsgPort ) ))
	{
		if ( msg->nm_Command == USBNC_Detached )
		{
			unit->unit_Running = FALSE;
		}

		IExec->ReplyMsg( (PTR) msg );
	}
}

// --

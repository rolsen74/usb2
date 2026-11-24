
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <devices/timer.h>
#include "All.h"

// --

S32
RTL8152_OpenLinkTimer( struct devUnit *unit )
{
	unit->unit_LinkPort = IExec->AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! unit->unit_LinkPort )
	{
		return ( FALSE );
	}

	unit->unit_LinkTimer = IExec->AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_Size, sizeof( *unit->unit_LinkTimer ), ASOIOR_ReplyPort, unit->unit_LinkPort, TAG_END );

	if ( ! unit->unit_LinkTimer )
	{
		return ( FALSE );
	}

	if ( IExec->OpenDevice( TIMERNAME, UNIT_MICROHZ, & unit->unit_LinkTimer->Request, 0 ) )
	{
		unit->unit_LinkTimer->Request.io_Device = NULL;

		return ( FALSE );
	}

	return ( TRUE );
}

// --

void
RTL8152_ArmLinkTimer( struct devUnit *unit )
{
	unit->unit_LinkTimer->Request.io_Command = TR_ADDREQUEST;
	unit->unit_LinkTimer->Time.Seconds = 0;
	unit->unit_LinkTimer->Time.Microseconds = 500000;
	unit->unit_LinkTimerPending = TRUE;

	IExec->SendIO( & unit->unit_LinkTimer->Request );
}

// --

void
RTL8152_CloseLinkTimer( struct devUnit *unit )
{
	struct IORequest *request;

	if ( unit->unit_LinkTimer )
	{
		request = & unit->unit_LinkTimer->Request;

		if ( unit->unit_LinkTimerPending )
		{
			if ( ! IExec->CheckIO( request ) )
			{
				IExec->AbortIO( request );
			}

			IExec->WaitIO( request );

			unit->unit_LinkTimerPending = FALSE;
		}

		if ( request->io_Device )
		{
			IExec->CloseDevice( request );
		}

		IExec->FreeSysObject( ASOT_IOREQUEST, request );
	}

	if ( unit->unit_LinkPort )
	{
		IExec->FreeSysObject( ASOT_PORT, unit->unit_LinkPort );
	}
}

// --

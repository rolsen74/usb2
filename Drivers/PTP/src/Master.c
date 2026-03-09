
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void Master_Entry( void )
{
struct TaskStartMsg *msg;
struct Task *parent;
struct Task *self;
struct intern in;
U32 mask;
S32 stat;

	self	= FindTask( NULL );
	msg		= (PTR) self->tc_UserData;
	parent	= msg->Parent;
	mask	= msg->Mask;

	// --

	MYERROR( "PTP : Master_Entry : Enter" );

	in.Task_State = TASK_State_Starting;

	stat = Master_Init( & in );

	if ( stat )
	{
		in.Task_Adr = self;

		msg->Result = TRUE;
		Signal( parent, mask );

		// --

		in.Task_State = TASK_State_Running;

		Master_Loop( & in );

		in.Task_State = TASK_State_Stopping;

		// --

		parent	= in.Exit_Parent;
		mask	= in.Exit_Mask;

		in.Task_Adr = NULL;
	}

	Master_Free( & in );

	// --

	Disable();

	in.Task_State = TASK_State_Stopped;

	MYERROR( "PTP : Master_Entry : Leave" );

	// --
	// Parent can be NULL

	if ( parent )
	{
		Signal( parent, mask );
	}
}

// --

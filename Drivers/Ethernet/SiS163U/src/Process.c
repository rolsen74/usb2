
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
Ethernet_Process_Entry( void )
{
	struct TaskStartMsg *msg;
	struct EthernetUnit *unit;
	struct Task *parent;
	struct Task *self;

	U32 mask;
	S32 stat;

	USBDEBUG( "SiS163U : Ethernet_Process_Entry : Enter" );

	self	= FindTask( NULL );
	msg		= (PTR) self->tc_UserData;
	parent	= msg->Parent;
	mask	= msg->Mask;
	unit	= msg->Unit;

	// --

	unit->unit_TaskState = TASK_State_Starting;

	stat = Process_Init( unit );

	if ( stat )
	{
		unit->unit_TaskAdr = self;
		unit->unit_TaskState = TASK_State_Running;
		msg->Result = TRUE;
		Signal( parent, mask );
		parent = NULL;

		// --

		Process_Main( unit );

		unit->unit_TaskState = TASK_State_Stopping;

	}

	Process_Free( unit );

	// --

	Forbid();

	unit->unit_TaskAdr = NULL;
	unit->unit_TaskState = TASK_State_Stopped;

	USBDEBUG( "SiS163U : Ethernet_Process_Entry : Leave" );

	// --
	// Parent can be NULL

	if ( parent )
	{
		Signal( parent, mask );
	}
}

// --


/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "All.h"

// --

#include "Process_Init_1_Get_Config.c"
#include "Process_Init.c"
#include "Process_Free.c"

// --

void Process_Main( struct PrinterUnit *unit );

void Printer_Process_Entry( void )
{
struct TaskStartMsg *msg;
struct PrinterUnit *unit;
struct Task *parent;
struct Task *self;
U32 mask;
S32 stat;

	USBERROR( "Printer : Printer_Process_Entry : Enter" );

	self	= FindTask( NULL );
	msg		= (PTR) self->tc_UserData;
	parent	= msg->Parent;
	mask	= msg->Mask;
	unit	= msg->Unit;

	// --

	unit->unit_TaskState = TASK_State_Starting;

	stat = __myProcess_Init( unit );

	if ( stat )
	{
		unit->unit_TaskAdr = self;

		msg->Result = TRUE;
		Signal( parent, mask );

		// --

		unit->unit_TaskState = TASK_State_Running;

		Process_Main( unit );

		unit->unit_TaskState = TASK_State_Stopping;

		// --

		parent	= unit->unit_ExitParent;
		mask	= unit->unit_ExitMask;

		unit->unit_TaskAdr = NULL;
	}

	__myProcess_Free( unit );

	// --

	Disable();

	unit->unit_TaskState = TASK_State_Stopped;

	USBERROR( "Printer : Printer_Process_Entry : Leave" );

	// --
	// Parent can be NULL

	if ( parent )
	{
		Signal( parent, mask );
	}
}

// --

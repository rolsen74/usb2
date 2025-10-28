
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#ifndef INC_TASK_H
#define INC_TASK_H

// --

#include "All.h"

// --

enum TaskState
{
	TASK_State_Stopped,
	TASK_State_Starting,
	TASK_State_Running,
	TASK_State_Stopping,
};

// --
struct PrinterUnit;

struct TaskStartMsg
{
	U32						Result;
	PTR						Parent;
	U32						Mask;
	struct PrinterUnit *	Unit;
};

// --

#endif

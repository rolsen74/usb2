
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
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
struct EthernetUnit;

struct TaskStartMsg
{
	U32						Result;
	PTR						Parent;
	U32						Mask;
	struct EthernetUnit *	Unit;
};

// --

#endif

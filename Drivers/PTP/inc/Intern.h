
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INC_ALL_H
#error Include "inc/All.h" first
#endif

#ifndef INC_INTERN_H
#define INC_INTERN_H

// --

enum
{
	TASK_State_Starting,
	TASK_State_Running,
	TASK_State_Stopping,
	TASK_State_Stopped,
};

struct intern
{
	struct Task *		Exit_Parent;
	U32					Exit_Mask;

	struct Task *		Task_Adr;
	U32					Task_State;

	S32					FSStatus;
	struct MsgPort *	FSMsgPort;

	S32					Running;
};

// --

#endif

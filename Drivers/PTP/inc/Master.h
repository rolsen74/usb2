
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

#ifndef INC_MASTER_H
#define INC_MASTER_H

// --

enum
{
	TASK_State_Starting,
	TASK_State_Running,
	TASK_State_Stopping,
	TASK_State_Stopped,
};

struct M_Struct
{
	struct Task *		Exit_Parent;
	U32					Exit_Mask;

	struct Task *		Task_Adr;
	U32					Task_State;

	S32					FSStatus;
	struct MsgPort *	FSMsgPort;

	struct MsgPort *	CmdMsgPort;

	S32					Running;
};

// --

#endif


/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#ifndef _STACK_MASTER_H_
#define _STACK_MASTER_H_

#define MAIN_RETURN_UNSET		0
#define MAIN_RETURN_EXIT		1
#define MAIN_RETURN_PROMOTE		2

struct intern
{
	#ifdef DO_DEBUG
	U32						StructID;
	#endif

	S32						Stopping;

	// -- Command MsgPort for Extern Commands
	struct USB2_MsgPort		Cmd_MsgPort;

	// -- (Sub) Tasks Shutdown -- TaskMsg's
	struct USB2_Header		Shutdown_TM_List;
	struct USB2_Signal		Task_Shutdown_Signal;
	struct USB2_MsgPort		Task_Shutdown_MsgPort;

	// -- Timer Tick
	struct USB2_MsgPort		Tick_MsgPort;
	S32						Tick_TimerAdded;
	struct TimeRequest		Tick_TimeRequest;
};

// --

struct MasterStartMsg
{
	PTR		usbbase;
	PTR		Parent;
	U32		Result;
	U32		Mask;
};

#endif

// --

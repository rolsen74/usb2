
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

#ifndef INC_PRIVATE_USB2_ALL_H
#error Include "usb2_all.h" first
#endif

#ifndef STACK_HCD_HCD_H
#define STACK_HCD_HCD_H

/***************************************************************************/

struct intern
{
	#ifdef DO_DEBUG
	U32						StructID;
	#endif

	// -- Timer Tick
	struct USB2_MsgPort		Tick_MsgPort;
	S32						Tick_TimerAdded;
	struct TimeRequest		Tick_TimeRequest;
};

/***************************************************************************/

#endif // STACK_HCD_HCD_H

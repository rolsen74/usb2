
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

#ifndef INC_PRIVATE_USB2_ASYNC_H
#define INC_PRIVATE_USB2_ASYNC_H

/***************************************************************************/

struct USB2_ASync
{
	U32								ua_StructID;
	struct USB2_Semaphore			ua_Semaphore;
	struct Task *					ua_Parent;			// used for ASync_Sub .. gets CTRL+D
	struct USB2_Signal				ua_Signal;			// used for ASync_Wait
	struct Task *					ua_Task;			// used for ASync_Wait .. gets ua_Signal
	S32								ua_Counter;
};

/***************************************************************************/

#endif // INC_PRIVATE_USB2_ASYNC_H

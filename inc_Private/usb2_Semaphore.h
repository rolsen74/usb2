
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

#ifndef INC_PRIVATE_USB2_SEMAPHORE_H
#define INC_PRIVATE_USB2_SEMAPHORE_H

/***************************************************************************/

struct USB2_Semaphore
{
	U32								us_StructID;
	struct SignalSemaphore			us_Semaphore;
};

/***************************************************************************/

#endif // INC_PRIVATE_USB2_SEMAPHORE_H

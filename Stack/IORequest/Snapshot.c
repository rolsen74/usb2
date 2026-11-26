
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE U32
USB2_IORequest_Snapshot( struct USBBase *usbbase, struct USB2_ActiveIORequest *buffer, U32 bytes )
{
	struct USB2_Node *node;
	const struct RealRequest *ioreq;
	const struct USB2_InterfaceHeader *interface;
	enum USB2_IORequestState state;

	U32 count;
	U32 capacity;

	count = 0;
	capacity = bytes / sizeof( *buffer );

	SEMAPHORE_OBTAIN( &usbbase->usb_LockSemaphore );

	for( node = usbbase->usb_IORequests.uh_Head; node; node = Node_Next( node ) )
	{
		ioreq = node->un_Data;

		switch( ioreq->req_PublicStat )
		{
			case IORS_HCD_Pending: state = USB2IO_Pending; break;
			case IORS_HCD_Queued: state = USB2IO_Queued; break;
			case IORS_HCD: state = USB2IO_Processing; break;
			case IORS_HCD_Active: state = USB2IO_Active; break;
			default: continue;
		}

		if ( ( buffer ) && ( count < capacity ) )
		{
			interface = USB2_IORequest_Interface( ioreq );

			USB2_IORequest_Describe( ioreq, interface, &buffer[count].Info );

			buffer[count].Info.nm_State = state;
			buffer[count].FunctionID = ioreq->req_Function->fkt_NotifyID;
		}

		count++;
	}

	SEMAPHORE_RELEASE( &usbbase->usb_LockSemaphore );

	return ( count * sizeof( *buffer ) );
}

// --
